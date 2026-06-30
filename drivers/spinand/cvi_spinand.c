#include <spinand/cvi_spinand.h>
#include "mmio.h"
#include <cpu.h>
#include <assert.h>
#include <delay_timer.h>
#include <errno.h>
#include <mmio.h>
#include <platform.h>
#include <string.h>
#include <platform.h>
#include <security/security.h>

static unsigned long spi_nand_ctrl_base = SPINAND_BASE;
/* default value */
static struct spi_nand_info_t spinand_info;

static uint8_t ecc_en_status = 0xff;
#define BASE_SYSDMAC  0x04330000

#define SPI_NAND_INTERRUPT_UDELAY_TIME  10
#define SPI_NAND_INTERRUPT_UDELAY_COUNT (100000000 / SPI_NAND_INTERRUPT_UDELAY_TIME)
#define SPI_NAND_PLANE_BIT_OFFSET       BIT(12)

static int spi_nand_set_feature(uint8_t fe, uint8_t val)
{
        uint32_t fe_set = fe | (val << 8);
        uint32_t retry = 0;

        mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2, 2 << TRX_CMD_CONT_SIZE_SHIFT);
        mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, BIT_REG_TRX_RW);
        mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0,
                (fe_set << TRX_CMD_CONT0_SHIFT) | SPI_NAND_CMD_SET_FEATURE);

        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

        uint32_t dma_retry = 0;
        while ((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BITS_REG_TRX_DMA_DONE_INT) == 0) {
                if (dma_retry++ > SPI_NAND_INTERRUPT_UDELAY_COUNT) {
                        ERROR("SPINAND DMA done timeout, %d %s\n", dma_retry, __func__);
                        return -EIO;
                }
                udelay(1);
        }

        if (retry > SPI_NAND_INTERRUPT_UDELAY_COUNT) {
                ERROR("SPINAND command error, no interrupt, %d %s\n", retry, __func__);
                //assert(0);
        }

        // clr trx done intr
        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT);

        return 0;
}

static void spi_nand_setup_intr(void)
{
        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_EN, 3);
        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BITS_SPI_NAND_INT_CLR_ALL);
        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_MASK, 0x00000f00);
}

static uint8_t spi_nand_get_feature(uint32_t fe)
{
	uint32_t val = 0;
	uint32_t retry = 0;

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2,
			1 << TRX_DATA_SIZE_SHIFT | 1 << TRX_CMD_CONT_SIZE_SHIFT);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0, fe << 8 | SPI_NAND_CMD_GET_FEATURE);

	spi_nand_setup_intr();

	// Trigger communication start
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

	while(((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0)
			&& (retry++ < SPI_NAND_INTERRUPT_UDELAY_COUNT))
		udelay(1);

	if (retry > SPI_NAND_INTERRUPT_UDELAY_COUNT) {
		ERROR("SPINAND command error, no interrupt, %d %s\n", retry, __func__);
	}

	val = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_RX_DATA);

	uint32_t intr = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT);
	mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, intr);

	return (val & 0xff);
}

void spi_nand_rw_dma_setup(void *buf, uint32_t len, uint32_t rw)
{
        uint32_t ch = 0;
        VERBOSE("%s: buf %p, len %d\n", __func__, buf, len);

        mmio_write_32(BASE_SYSDMAC + 0x010, 0x00000003);
        mmio_write_32(BASE_SYSDMAC + 0x018, 0x00000f00);

        mmio_write_32(BASE_SYSDMAC + 0x110 + ch * (0x100), (len / 4) - 1);
        mmio_write_32(BASE_SYSDMAC + 0x11C + ch * (0x100), 0x000f0792);

        buf = (void *)phys_to_dma((uintptr_t) buf);

        if (rw) {
                // for dma write
                mmio_write_64(BASE_SYSDMAC + 0x100 + ch * (0x100), (uint64_t) buf);
                mmio_write_64(BASE_SYSDMAC + 0x108 + ch * (0x100), spi_nand_ctrl_base + 0x800);
                mmio_write_32(BASE_SYSDMAC + 0x118 + ch * (0x100), 0x00045441);

                // [0:2] = 1 : MEM_TO_PER_DMAC, PER dst = 0
                mmio_write_32(BASE_SYSDMAC + 0x124 + ch * (0x100), 0x00000001);
                mmio_write_32(0x03000154, 0x2a); //PER dst = 0 => remap ch0
                mmio_setbits_32(0x03000154, BIT(31)); // set sdma remap update bit

                VERBOSE("0x100: 0x%lx\n", mmio_read_64(BASE_SYSDMAC + 0x100 + ch * (0x100)));
                VERBOSE("0x108: 0x%lx\n", mmio_read_64(BASE_SYSDMAC + 0x108 + ch * (0x100)));
        } else {
                mmio_write_64(BASE_SYSDMAC + 0x100 + ch * (0x100), spi_nand_ctrl_base + 0xC00);
                mmio_write_64(BASE_SYSDMAC + 0x108 + ch * (0x100), (uint64_t)buf);
                mmio_write_32(BASE_SYSDMAC + 0x118 + ch * (0x100), 0x00046214);
                // [0:2] = 2 : PER_TO_MEM_DMAC, PER src = 0
                mmio_write_32(BASE_SYSDMAC + 0x124 + ch * (0x100), 0x00000002);
                mmio_write_32(0x03000154, 0x2a); //PER src = 0 => remap ch0
                mmio_setbits_32(0x03000154, BIT(31)); // set sdma remap update bit

                VERBOSE("0x100: 0x%lx\n", mmio_read_64(BASE_SYSDMAC + 0x100 + ch * (0x100)));
                VERBOSE("0x108: 0x%lx\n", mmio_read_64(BASE_SYSDMAC + 0x108 + ch * (0x100)));
        }

        mmio_write_32(BASE_SYSDMAC + 0x120 + ch * (0x100), 0x0);
        mmio_write_32(BASE_SYSDMAC + 0x018, 0x00000f0f);
}

static void spi_nand_polling_oip(void)
{
        uint32_t status = 0xff;
        uint32_t retry = 0;

        while ((status & SPI_NAND_STATUS0_OIP) && (retry++ < (SPI_NAND_INTERRUPT_UDELAY_COUNT * 10))) {
                status = spi_nand_get_feature(spinand_info.ecc_status_offset);
                udelay(1);
        }

        if (retry > (SPI_NAND_INTERRUPT_UDELAY_COUNT * 10)) {
                ERROR("SPINAND is still busy over 10 sec after reseting spinand %d %s\n", retry, __func__);
                //assert(0);
        }
}

static void spi_nand_ctrl_ecc(uint8_t enable)
{
        uint8_t status;

        if ((ecc_en_status == 1) && (enable == 1)) {
                VERBOSE("NAND_DEBUG: ecc is already enabled\n");
                return;
        } else if ((ecc_en_status == 0) && (enable == 0)) {
                VERBOSE("NAND_DEBUG: ecc is already disabled\n");
                return;
        }

        status = spi_nand_get_feature(spinand_info.ecc_en_feature_offset);

        if (enable) {
                spi_nand_set_feature(spinand_info.ecc_en_feature_offset,
                        status | spinand_info.ecc_en_mask); // set ECC_EN to enable
                ecc_en_status = 1;
        } else {
                spi_nand_set_feature(spinand_info.ecc_en_feature_offset,
                        status & (~spinand_info.ecc_en_mask)); // set ECC_EN to disable
                ecc_en_status = 0;
        }
}

static void spi_nand_qe_enable(void)
{
        uint8_t status;

	status = spi_nand_get_feature(0xB0);
	spi_nand_set_feature(0xB0, status | 0x1);
}

static int spi_nand_read_page_by_row_addr(uint32_t row_addr, void *buf, uint32_t len)
{
        uint32_t retry = 0;

        uint32_t r_row_addr = ((row_addr & 0xff0000) >> 16) | (row_addr & 0xff00) | ((row_addr & 0xff) << 16);
        VERBOSE("%s r_row_addr 0x%x\n", __func__, r_row_addr);

        mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2,
                0 << TRX_DATA_SIZE_SHIFT | 3 << TRX_CMD_CONT_SIZE_SHIFT); // 0x8

        mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3, 0);
        mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0,
                r_row_addr << TRX_CMD_CONT0_SHIFT | SPI_NAND_CMD_PAGE_READ_TO_CACHE);

        // Trigger communication start
        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

        while(((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BIT_REG_TRX_DONE_INT) == 0)
                && (retry++ < SPI_NAND_INTERRUPT_UDELAY_COUNT))
                udelay(1);

        if (retry > SPI_NAND_INTERRUPT_UDELAY_COUNT) {
                ERROR("SPINAND command error, no interrupt, %d %s\n", retry, __func__);
                //assert(0);
        }

        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, BIT_REG_TRX_DONE_INT);

        spi_nand_polling_oip();

        return 0;
}

static void spi_nand_set_read_from_cache_mode(uint32_t col_addr)
{

        if (spinand_info.flags & FLAGS_ENABLE_X4_BIT) {
                VERBOSE("NAND_DEBUG: Read by 4 bit mode\n");
                mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3,
                        BIT_REG_TRX_DMA_EN | SPI_NAND_CTRL3_IO_TYPE_X4_MODE | BIT_REG_TRX_DUMMY_HIZ);
                mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0,
                        col_addr << TRX_CMD_CONT0_SHIFT | SPI_NAND_CMD_READ_FROM_CACHEX4);
        } else if (spinand_info.flags & FLAGS_ENABLE_X2_BIT) {
                VERBOSE("NAND_DEBUG: Read by 2 bit mode\n");
                mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3,
                        BIT_REG_TRX_DMA_EN | SPI_NAND_CTRL3_IO_TYPE_X2_MODE | BIT_REG_TRX_DUMMY_HIZ);
                mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0,
                        col_addr << TRX_CMD_CONT0_SHIFT | SPI_NAND_CMD_READ_FROM_CACHEX2);
        } else {
                VERBOSE("NAND_DEBUG: Read by 1 bit mode\n");
                mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL3,
                        BIT_REG_TRX_DMA_EN | BIT_REG_TRX_DUMMY_HIZ);
                mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CMD0,
                        col_addr << TRX_CMD_CONT0_SHIFT | SPI_NAND_CMD_READ_FROM_CACHE);
        }

        return;
}

static int spi_nand_read_from_cache(uint32_t blk_id, uint32_t page_nr, uint32_t col_addr,
                                    uint32_t len, void *buf)
{
        uint32_t r_col_addr = ((col_addr & 0xff00) >> 8) | ((col_addr & 0xff) << 8);

        VERBOSE("%s col_addr 0x%x, r_col_addr 0x%x\n", __func__, col_addr, r_col_addr);

        spi_nand_rw_dma_setup(buf, len, 0);

        mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL2,
                len << TRX_DATA_SIZE_SHIFT | 3 << TRX_CMD_CONT_SIZE_SHIFT);

        spi_nand_set_read_from_cache_mode(r_col_addr);

        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL0, BIT_REG_TRX_START);

        while ((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT) & BITS_REG_TRX_DMA_DONE_INT) == 0);

        uint32_t intr = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_INT);
        mmio_setbits_32(spi_nand_ctrl_base + REG_SPI_NAND_INT_CLR, intr);

        __asm__ volatile("dsb sy" ::: "memory");
        inv_dcache_range((uintptr_t)buf, len);

        return 0;
}

#define ECC_NOERR  0
#define ECC_CORR   1
#define ECC_UNCORR 2

uint32_t spi_nand_parsing_ecc_info(uint32_t page)
{
        uint32_t statusc0 = spi_nand_get_feature(spinand_info.ecc_status_offset);
        uint8_t ecc_sts = (statusc0 & spinand_info.ecc_status_mask) >> spinand_info.ecc_status_shift;

        if (ecc_sts) {
                VERBOSE("statusc0 0x%x\n", statusc0);
                INFO("block %d, page %d, ecc sts : %d!!\n",
                       page >> spinand_info.pages_per_block_shift, page % spinand_info.pages_per_block, ecc_sts);
        }

        if (ecc_sts == spinand_info.ecc_status_uncorr_val) {
                INFO("ECC UNCORR\n");
                return ECC_UNCORR;
        }

        return ECC_NOERR;
}

static void dump_nand_info(void)
{
        INFO("NAND_DEBUG: NAND version=0x%x, id=0x%x\n", spinand_info.version, spinand_info.id);
        INFO("NAND_DEBUG: page size=%d, block size=%d, page per block=%d, badblock_pos=%d\n",
                spinand_info.page_size, spinand_info.block_size,
                spinand_info.pages_per_block, spinand_info.badblock_pos);
        VERBOSE("NAND_DEBUG: fip block count=%d, page per block shift=%d, flags=0x%x\n", spinand_info.fip_block_cnt,
                spinand_info.pages_per_block_shift, spinand_info.flags);
        VERBOSE("NAND_DEBUG: ECC feature offset=0x%x, en mask=0x%x, status offset=0x%x, status mask=0x%x\n",
                spinand_info.ecc_en_feature_offset, spinand_info.ecc_en_mask,
                spinand_info.ecc_status_offset, spinand_info.ecc_status_mask);
        VERBOSE("NAND_DEBUG: ECC status shift=%d, uncorr_val=0x%x, erase count=%d\n", spinand_info.ecc_status_shift,
                spinand_info.ecc_status_uncorr_val, spinand_info.erase_count);
        VERBOSE("NAND_DEBUG: sck_l=%d, sck_h=0x%x, max_freq=%d\n", spinand_info.sck_l,
                spinand_info.sck_h, spinand_info.max_freq);
        VERBOSE("NAND_DEBUG: xtal_switch=%d\n", spinand_info.xtal_switch);
}

int get_nand_info(void)
{
	uint32_t ret;
	uint8_t page_buff[4096] = {0};
	struct fip_param1 *fip_param1 = (void *)PARAM1_BASE;

	spinand_info.id = 0x0;
	spinand_info.page_size = 2048;
	spinand_info.spare_size = 64;
	spinand_info.block_size = 0x20000;
	spinand_info.pages_per_block = 64;
	spinand_info.fip_block_cnt = 1024;
	spinand_info.pages_per_block_shift = 6;
	spinand_info.flags = 0;
	spinand_info.badblock_pos = BBP_FIRST_PAGE;
	spinand_info.ecc_en_feature_offset = SPI_NAND_FEATURE_FEATURE0;
	spinand_info.ecc_en_mask = SPI_NAND_FEATURE0_ECC_EN;
	spinand_info.ecc_status_offset = SPI_NAND_FEATURE_STATUS0;
	spinand_info.ecc_status_mask = 0x30;
	spinand_info.ecc_status_shift = 4;
	spinand_info.ecc_status_uncorr_val = 0x2;
	spinand_info.erase_count = 1;
	spinand_info.sck_l = 2;
	spinand_info.sck_h = 2;
	spinand_info.max_freq = 0;
	spinand_info.sample_param = 0x100;
	spinand_info.xtal_switch = 0;

	/* update spinand info */
	memcpy(&spinand_info, &fip_param1->nand_info, sizeof(struct spi_nand_info_t));

	if (spinand_info.flags & FLAGS_ENABLE_X4_BIT) {

		if (spinand_info.flags & (FLAGS_ENABLE_X4_BIT | FLAGS_SET_QE_BIT))
			spi_nand_qe_enable();

		ret = cv_spi_nand_read_page_by_row_addr(0, page_buff, 4096);
		if (ret)
			ERROR("Scan fip_header failed!\n");

		if (memcmp(page_buff, page_buff + 2048, 2048))
			spinand_info.flags &= ~(FLAGS_ENABLE_X4_BIT | FLAGS_ENABLE_X2_BIT);
	}
	dump_nand_info();
	return 0;
}

uint32_t cv_spi_nand_read_page_by_row_addr(uint32_t row_addr, void *buf, uint32_t len)
{
        uint32_t col_addr = 0;
        uint32_t blk_id = row_addr >> 6;

        VERBOSE("%s, row_addr 0x%x, buf %p, len %d, spinand_info.flags 0x%x\n", __func__, row_addr, buf, len,
             spinand_info.flags);

        spi_nand_ctrl_ecc(1);

        spi_nand_read_page_by_row_addr(row_addr, buf, len);

        if ((spinand_info.flags & FLAGS_SET_PLANE_BIT) && (blk_id % 2 == 1))
                col_addr |= SPI_NAND_PLANE_BIT_OFFSET;

        flush_dcache_range((uintptr_t)buf, len);
        spi_nand_read_from_cache(0, 0, col_addr, len, buf);

        return spi_nand_parsing_ecc_info(row_addr);
}

uint32_t cv_spi_nand_read_oob(uint32_t row_addr, void *buf)
{
	uint32_t col_addr = spinand_info.page_size;
	uint32_t blk_id = row_addr >> spinand_info.pages_per_block_shift;

	spi_nand_ctrl_ecc(0);

	spi_nand_read_page_by_row_addr(row_addr, buf, spinand_info.spare_size);

	if ((spinand_info.flags & FLAGS_SET_PLANE_BIT) && (blk_id % 2 == 1))
		col_addr |= SPI_NAND_PLANE_BIT_OFFSET;

	flush_dcache_range((uintptr_t)buf, spinand_info.spare_size);
	spi_nand_read_from_cache(0, 0, col_addr, spinand_info.spare_size, buf);
	spi_nand_ctrl_ecc(1);

	return 0;
}

static int nand_block_isbad(uint32_t offset)
{
	uint32_t read_page = offset / spinand_info.page_size;
        __aligned(64) uint8_t oob_buff[256];
        memset(oob_buff, 0xff, sizeof(oob_buff));


	switch (spinand_info.badblock_pos) {
	case BBP_LAST_PAGE:
		cv_spi_nand_read_oob(read_page + (spinand_info.pages_per_block - 1), oob_buff);
		// Check bad block if 1st byte of last page is non-0xFF
		if (*(uint8_t *)oob_buff != 0xFF) {
			INFO("Found bad block %x with spare data 0x%x\n",
				offset, *(uint8_t *)oob_buff);
			return -1;
		}
		break;
	case BBP_FIRST_2_PAGE:
		cv_spi_nand_read_oob(read_page + (spinand_info.pages_per_block - 1), oob_buff);
		// Check bad block if 1st byte of 1st page is non-0xFF
		if (*(uint8_t *)oob_buff != 0xFF) {
			INFO("Found bad block %x with spare data 0x%x\n",
				offset, *(uint8_t *)oob_buff);
			return -1;
		}

		cv_spi_nand_read_oob(read_page + 1, oob_buff);
		// Check bad block if 1st byte of 2nd page is non-0xFF
		if (*(uint8_t *)oob_buff != 0xFF) {
			INFO("Found bad block %x with spare data 0x%x\n",
				offset, *(uint8_t *)oob_buff);
			return -1;
		}
		break;
	case BBP_FIRST_PAGE:
	default:
		cv_spi_nand_read_oob(read_page, oob_buff);
		// Check bad block if 1st byte of first page is non-0xFF
		if (*(uint8_t *)oob_buff != 0xFF) {
			INFO("Found bad block %x with spare data 0x%x\n",
				offset, *(uint8_t *)oob_buff);
			return -1;
		}
		break;
	}
	return 0;
}

int cv_spi_nand_read_data(void **buffer, uint32_t offset, uint32_t size)
{
	uint32_t i, length = size;
	offset = offset / spinand_info.page_size;
	int ret = 0;

	if (size & (spinand_info.page_size - 1)) {
		size = (size + spinand_info.page_size - 1) &~(spinand_info.page_size - 1);
	}

	//Note: writing to the buffer here will go out of bounds.
	for (i = 0; i < size / spinand_info.page_size; i++) {
		ret = (int)cv_spi_nand_read_page_by_row_addr(offset, *buffer, spinand_info.page_size);
		if (ret) {
			ERROR("read page %u failed\n", offset);
			return ret;
		}
		offset++;
		if (length < spinand_info.page_size)
			*buffer += length;
		else
			*buffer += spinand_info.page_size;
		length -= spinand_info.page_size;
	}
	return ret;
}

int cv_spi_nand_read_skip_bad(void *buffer, uint32_t offset, uint32_t size)
{
	int ret;
	uint32_t left_to_read = size;
	void *read_buf = buffer;

	while (left_to_read > 0) {
		size_t block_offset = offset & (spinand_info.block_size - 1);
		size_t read_length;

		if (nand_block_isbad(offset & ~(spinand_info.block_size - 1))) {
			NOTICE("Skipping bad block 0x%x\n",
				offset & ~(spinand_info.block_size - 1));
			offset += spinand_info.block_size - block_offset;
			continue;
		}

		if (left_to_read < (spinand_info.block_size - block_offset))
			read_length = left_to_read;
		else
			read_length = spinand_info.block_size - block_offset;

		ret = cv_spi_nand_read_data(&read_buf, offset, read_length);

		if (ret) {
			printf("NAND read from offset %x failed %d\n", offset, ret);
			return ret;
		}

		left_to_read -= read_length;
		offset       += read_length;
	}

	return ret;
}

void cv_spi_nand_set_freq(uint8_t sck_l, uint8_t sck_h, uint16_t prd)
{
	VERBOSE("Before:NAND:CTRL:0x%x/CTRL1:0x%x\n",
			mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_BOOT_CTRL),
			mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL1));

	uint32_t val;

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_BOOT_CTRL, 0x100);
	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_BOOT_CTRL,
			    ((mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_BOOT_CTRL) & 0xFFFFFFFE) | prd));

	val = mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL1) & ~SPI_NAND_SCK_MASK;

	val |= SPI_NAND_SET_SCK_L(sck_l) | SPI_NAND_SET_SCK_H(sck_h);

	mmio_write_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL1, val);

	VERBOSE("NAND:CTRL:0x%x/CTRL1:0x%x\n",
			mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_BOOT_CTRL),
			mmio_read_32(spi_nand_ctrl_base + REG_SPI_NAND_TRX_CTRL1));
}
