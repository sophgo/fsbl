#include <debug.h>
#include <console.h>
#include <platform.h>
#include <rom_api.h>
#include <bl2.h>
#include <cli.h>
#include <string.h>
#include <decompress.h>
#include <delay_timer.h>
#include <security/security.h>
#include <tempsen.h>
#include <spinor/cvi_spinor.h>
#include <spinand/cvi_spinand.h>
#include <emmc/emmc.h>
#include <cvipart.h>
#if defined(CONFIG_BOARD_palladium)
#include <ddr_sys_bring_up_pld.h>
#elif defined(CONFIG_BOARD_fpga)
#else
	#include <ddr.h>
#endif

struct rom_api p_rom_api = {
	.get_boot_src = (void *)0x0000000004400020,
	.set_boot_src = (void *)0x0000000004400040,
	.load_image = (void *)0x0000000004400060,
	.flash_init = (void *)0x0000000004400080,
	.image_crc = (void *)0x00000000044000a0,
	.get_number_of_retries = (void *)0x00000000044000c0,
	.verify_rsa = (void *)0x00000000044000e0,
	.cryptodma_aes_decrypt = (void *)0x0000000004400100
};

//#define BL2_USE_CLI
//#define BL2_ACCESS_BT256MB //bl2 cpu access bigthan 256M byte addrspace
struct _time_records *time_records = (void *)TIME_RECORDS_ADDR;
struct fip_param1 *fip_param1 = (void *)PARAM1_BASE;
static struct fip_param2 fip_param2 __aligned(BLOCK_SIZE);

static union {
	#if !defined(CONFIG_BOARD_palladium) && !defined(CONFIG_BOARD_fpga)
	struct ddr_param ddr_param;
	#endif
	struct loader_2nd_header loader_2nd_header;
	uint8_t buf[BLOCK_SIZE];
} sram_union_buf __aligned(BLOCK_SIZE);

#define BL2_LOAD_IMAGE_SPLIT	0

void print_sram_log(void)
{
	uint32_t *const log_size = (void *)BOOT_LOG_LEN_ADDR;
	uint8_t *const log_buf = (void *)phys_to_dma(BOOT_LOG_BUF_BASE);
	uint32_t i;

	const char m1[] = "\nSRAM Log: ========================================\n";
	const char m2[] = "\nSRAM Log end: ====================================\n";

	for (i = 0; m1[i]; i++)
		console_putc(m1[i]);

	for (i = 0; i < *log_size; i++)
		console_putc(log_buf[i]);

	for (i = 0; m2[i]; i++)
		console_putc(m2[i]);
}

void rom_api_redirect(void)
{
	if (!(mmio_read_32(REG_TOP_CONF_INFO) & 0x00100000))
		return;

	NOTICE("%s, %d. Redirect the rom api address.\n", __FUNCTION__, __LINE__);
	p_rom_api.get_boot_src = (void *)0x0000000005400020;
	p_rom_api.set_boot_src = (void *)0x0000000005400040;
	p_rom_api.load_image = (void *)0x0000000005400060;
	p_rom_api.flash_init = (void *)0x0000000005400080;
	p_rom_api.image_crc = (void *)0x00000000054000a0;
	p_rom_api.get_number_of_retries = (void *)0x00000000054000c0;
	p_rom_api.verify_rsa = (void *)0x00000000054000e0;
	p_rom_api.cryptodma_aes_decrypt = (void *)0x0000000005400100;
}

#if BL2_LOAD_IMAGE_SPLIT
#define ALIGNMENT_THRESHOLD (512 * 1024) // 512KB threshold for alignment
int bl2_load_image_split(void *buf, uint32_t offset, size_t image_size, int retry_num)
{
	unsigned int ret;
	unsigned char *split_start, *split_end;
	unsigned char *start_addr = buf;
	unsigned char *end_addr = buf + image_size;
	unsigned char *aligned_start = (unsigned char *)((size_t)start_addr & ~(ALIGNMENT_THRESHOLD - 1));
	unsigned char *aligned_end = (unsigned char *)(((size_t)end_addr + ALIGNMENT_THRESHOLD - 1)
				& ~(ALIGNMENT_THRESHOLD - 1));

	INFO("Aligned memory segments: start: %p, end: %p.\n", aligned_start, aligned_end);
	for (unsigned char *addr = aligned_start; addr < aligned_end; addr += ALIGNMENT_THRESHOLD) {
		if (addr < start_addr)
			split_start = start_addr;
		else
			split_start = addr;

		if ((addr + ALIGNMENT_THRESHOLD) > end_addr)
			split_end = end_addr;
		else
			split_end = addr + ALIGNMENT_THRESHOLD;

		INFO("Start: %p, End: %p\n", split_start, split_end);
		ret = load_data_from_storage(split_start, offset,
					     split_end - split_start, retry_num, 1);
		if (ret < 0) {
			ERROR("fail to load image split! loadaddr:0x%x, size:%d, retry:%d\n",
			      offset, split_end - split_start, retry_num);
			return ret;
		}
		offset += split_end - split_start;
	}

	return 0;
}
#endif

static int emmc_read_fip_bl2(uint32_t offset, uint32_t size, uintptr_t buf)
{
	int lba = 0;

	if (((offset & EMMC_BLOCK_MASK) != 0) ||
	    ((buf & EMMC_BLOCK_MASK) != 0) || ((size & EMMC_BLOCK_MASK) != 0))
		return -1;

	lba = offset / EMMC_BLOCK_SIZE;

	INFO("%s offset %x,lba %d, size %d, dst buf 0x%lx\n", __func__, offset,
	     lba, size, buf);

	if (size !=
	    emmc_partition_read_blocks(EMMC_PARTITION_BOOT1, lba, buf, size))
		return -1;

	return 0;
}

int load_data_from_storage(void *buffer, uint32_t offset, uint32_t size,
			   int retry, u_int8_t is_eMMC_boot_partition)
{
	int ret = 0;

	if (p_rom_api.get_boot_src() == BOOT_SRC_EMMC) {
		if (is_eMMC_boot_partition) {
			uint32_t retry_offset =
				retry * FIP_RETRY_OFFSET + offset;

			ret = emmc_read_fip_bl2(retry_offset, size,
						(uintptr_t)buffer);
		} else {
			ret = emmc_read_blocks(offset / 512, (uintptr_t)buffer, size);
		}
	} else {
		ret = p_rom_api.load_image(buffer, offset, size, retry);
	}

	// uint64_t data = *(uint64_t *)(uintptr_t)buffer;

	NOTICE("\nthe first 8 bytes data is(ret:%d):0x%lx\n", ret, *(uint64_t *)(uintptr_t)buffer);
	return ret;
}

int load_param2(int retry)
{
	uint32_t crc;
	int ret = -1;

	NOTICE("P2S/0x%lx/%p.\n", sizeof(fip_param2), &fip_param2);
#if BL2_LOAD_IMAGE_SPLIT
	ret = bl2_load_image_split(&fip_param2, fip_param1->param2_loadaddr, PARAM2_SIZE, retry);
#else
	ret = p_rom_api.load_image(&fip_param2, fip_param1->param2_loadaddr,
											PARAM2_SIZE, retry);
#endif
	if (ret < 0) {
		ERROR("load data failed! loadaddr:0x%x, size:%d, retry:%d\n",
		      fip_param1->param2_loadaddr, PARAM2_SIZE, retry);
		return ret;
	}

	if (fip_param2.magic1 != FIP_PARAM2_MAGIC1) {
		WARN("LP2_NOMAGIC\n");
		return -1;
	}

	crc = p_rom_api.image_crc(&fip_param2.reserved1, sizeof(fip_param2) - 12);
	if (crc != fip_param2.param2_cksum) {
		ERROR("param2_cksum (0x%x/0x%x)\n", crc, fip_param2.param2_cksum);
		return -1;
	}

	NOTICE("P2E.\n");

	return 0;
}

int load_ddr_param(int retry)
{
	return 0;
}

int load_ddr(void)
{
	int retry = 0;

retry_from_flash:
	for (retry = 0; retry < p_rom_api.get_number_of_retries(); retry++) {
		if (load_param2(retry) < 0)
			continue;

		if (load_ddr_param(retry) < 0)
			continue;

		break;
	}

	if (retry >= p_rom_api.get_number_of_retries()) {
		switch (p_rom_api.get_boot_src()) {
		case BOOT_SRC_UART:
		case BOOT_SRC_SD:
		case BOOT_SRC_USB:
			WARN("DL cancelled. Load flash. (%d).\n", retry);
			// Continue to boot from flash if boot from external source
			p_rom_api.flash_init();
			goto retry_from_flash;
		default:
			ERROR("Failed to load DDR param (%d).\n", retry);
			plat_panic_handler();
		}
	}

	time_records->ddr_init_start = read_time_ms();

#if defined(CONFIG_BOARD_palladium)
	ddr_sys_bring_up();
#elif defined(CONFIG_BOARD_fpga)
#else
	//TODO: tmp modifications for fpga testing
	ddr_init(&sram_union_buf.ddr_param);
#endif

#ifdef  BL2_USE_CLI
	tempsen_init();
	cli_simple_loop(1);
#endif
	time_records->ddr_init_end = read_time_ms();
	return 0;
}

static int check_blcp_2nd_header(struct fip_param2 _param2)
{
	if (!_param2.blcp_2nd_runaddr) {
		NOTICE("No C906L image.\n");
		return 1;
	}

	if (!IN_RANGE(_param2.blcp_2nd_runaddr, DRAM_BASE, DRAM_SIZE)) {
		ERROR("blcp_2nd_runaddr (0x%x) is not in DRAM.\n", fip_param2.blcp_2nd_runaddr);
		plat_panic_handler();
	}

	if (!IN_RANGE(_param2.blcp_2nd_runaddr + _param2.blcp_2nd_size, DRAM_BASE, DRAM_SIZE)) {
		ERROR("blcp_2nd_size (0x%x) is not in DRAM.\n", _param2.blcp_2nd_size);
		plat_panic_handler();
	}

	switch (fip_param2.blcp_2nd_comp_type) {
	case LOADER_2ND_MAGIC_LZMA:
		fip_param2.blcp_2nd_comp_type = COMP_LZMA;
		break;
	case LOADER_2ND_MAGIC_LZ4:
		fip_param2.blcp_2nd_comp_type = COMP_LZ4;
		break;
	default:
		fip_param2.blcp_2nd_comp_type = COMP_NONE;
		break;
	}

	if (fip_param2.blcp_2nd_comp_type <= COMP_NONE && fip_param2.blcp_2nd_comp_type >= COMP_MAX) {
		if (!IN_RANGE(_param2.blcp_2nd_comp_addr, DRAM_BASE, DRAM_SIZE)) {
			ERROR("blcp_2nd_comp_addr (0x%x) is not in DRAM.\n", fip_param2.blcp_2nd_comp_addr);
			plat_panic_handler();
		}

		if (!IN_RANGE(_param2.blcp_2nd_comp_addr + _param2.blcp_2nd_comp_size, DRAM_BASE, DRAM_SIZE)) {
			ERROR("blcp_2nd_comp_size (0x%x) is not in DRAM.\n", _param2.blcp_2nd_comp_size);
			plat_panic_handler();
		}
	}

	if ((p_rom_api.get_boot_src() == BOOT_SRC_SD
		|| p_rom_api.get_boot_src() == BOOT_SRC_USB
		|| p_rom_api.get_boot_src() == BOOT_SRC_UART)
		&& _param2.blcp_2nd_loadaddr == 0) {
		NOTICE("Wouldn't boot up RTOS, cause image didn't build in to fip.bin when boot src is SD.\n");
		return 1;
	}

	return 0;
}

int load_blcp_2nd(int retry)
{
	int ret = -1;
	// uint32_t crc = 0;
	uint32_t rtos_base = 0;
	void *image_buf = NULL;
	uint32_t image_size = 0;
	u_int8_t is_emmc_boot_partition = 1;

	// if no blcp_2nd, set release_blcp_2nd previou and change it end of function.
	time_records->release_blcp_2nd = read_time_ms();

	NOTICE("C2S/0x%x/0x%x/0x%x, 0x%x/0x%x/0x%x.\n",
	       fip_param2.blcp_2nd_comp_type, fip_param2.blcp_2nd_comp_size, fip_param2.blcp_2nd_comp_addr,
	       fip_param2.blcp_2nd_loadaddr, fip_param2.blcp_2nd_runaddr, fip_param2.blcp_2nd_size);

	if (check_blcp_2nd_header(fip_param2))
		return 0;

	if (fip_param2.blcp_2nd_comp_type > COMP_NONE && fip_param2.blcp_2nd_comp_type < COMP_MAX) {
		image_buf = (void *)(uintptr_t)fip_param2.blcp_2nd_comp_addr;
		image_size = fip_param2.blcp_2nd_comp_size;
	} else {
		image_buf = (void *)(uintptr_t)fip_param2.blcp_2nd_runaddr;
		image_size = fip_param2.blcp_2nd_size;
	}

	// fip_param2.blcp_2nd_loadaddr == 0, means didn't build in fip.bin.
	if (!fip_param2.blcp_2nd_loadaddr) {
		#ifdef SECOND_OFFSET
		//reset the loadaddr to SECOND_OFFSET from xmp partitions.
		fip_param2.blcp_2nd_loadaddr = SECOND_OFFSET;
		#endif
		if (p_rom_api.get_boot_src() == BOOT_SRC_EMMC)
			is_emmc_boot_partition = 0;
	}

	// load blcp 2nd image from flash to run comp address, and speed up freqency
	ret = load_data_from_storage(image_buf,
				     fip_param2.blcp_2nd_loadaddr,
				     image_size, retry, is_emmc_boot_partition);
	if (ret < 0) {
		ERROR("load data failed! loadaddr:0x%x, size:%d, retry:%d\n",
		      fip_param2.blcp_2nd_loadaddr, image_size, retry);
		return ret;
	}

	// crc = p_rom_api.image_crc(image_buf, image_size);
	NOTICE("blcp_2nd_cksum (0x%x/0x%x)\n", p_rom_api.image_crc(image_buf, image_size), fip_param2.blcp_2nd_cksum);
	// if (crc != fip_param2.blcp_2nd_cksum) {
	// 	ERROR("blcp_2nd_cksum (0x%x/0x%x)\n", crc, fip_param2.blcp_2nd_cksum);
	// 	return -1;
	// }

	// ret = dec_verify_image(image_buf, image_size, 0, fip_param1);
	// if (ret < 0) {
	// 	ERROR("verify blcp 2nd (%d)\n", ret);
	// 	return ret;
	// }
	time_records->load_loader_2nd_end = read_time_ms();

    // if the blcp 2nd image has been compressed, decompressing it.
	time_records->fsbl_decomp_start = read_time_ms();
	if (fip_param2.blcp_2nd_comp_type > COMP_NONE &&
		fip_param2.blcp_2nd_comp_type < COMP_MAX) {
		size_t dst_size = BLCP_2ND_DECOMP_DST_SIZE;

		ret = decompress((void *)(uintptr_t)fip_param2.blcp_2nd_runaddr,
						&dst_size,
						(void *)(uintptr_t)fip_param2.blcp_2nd_comp_addr,
						fip_param2.blcp_2nd_comp_size,
						fip_param2.blcp_2nd_comp_type);
		if (ret < 0) {
			ERROR("Failed to decompress blcp_2nd (%d/%u)\n", ret,
				fip_param2.blcp_2nd_size);
			return -1;
		}
		fip_param2.blcp_2nd_size = dst_size;
	}

	flush_dcache_range(fip_param2.blcp_2nd_runaddr, fip_param2.blcp_2nd_size);

	rtos_base = mmio_read_32(AXI_SRAM_RTOS_BASE);
	init_comm_info();

	switch (p_rom_api.get_boot_src()) {
	case BOOT_SRC_UART:
	// case BOOT_SRC_SD:
	// case BOOT_SRC_USB:
		break;

	default:
		time_records->release_blcp_2nd = read_time_ms();
		if (rtos_base == CVI_RTOS_MAGIC_CODE)
			mmio_write_32(AXI_SRAM_RTOS_BASE, fip_param2.blcp_2nd_runaddr);
		else
			reset_c906l(fip_param2.blcp_2nd_runaddr);
	}

	NOTICE("C2E.\n");
	return 0;
}

int load_monitor(int retry, uint64_t *monitor_entry)
{
	uint32_t crc;
	int ret = -1;

	NOTICE("MS/0x%lx/0x%lx/0x%x.\n", fip_param2.monitor_loadaddr, fip_param2.monitor_runaddr,
	       fip_param2.monitor_size);

	if (!fip_param2.monitor_runaddr) {
		NOTICE("No monitor.\n");
		return 0;
	}

	if (!IN_RANGE(fip_param2.monitor_runaddr, DRAM_BASE, DRAM_SIZE)) {
		ERROR("monitor_runaddr (0x%lx) is not in DRAM.\n", fip_param2.monitor_runaddr);
		plat_panic_handler();
	}

	if (!IN_RANGE(fip_param2.monitor_runaddr + fip_param2.monitor_size, DRAM_BASE, DRAM_SIZE)) {
		ERROR("monitor_size (0x%x) is not in DRAM.\n", fip_param2.monitor_size);
		plat_panic_handler();
	}

// #ifdef USB_DL_BY_FSBL
// 	if (p_rom_api.get_boot_src() == BOOT_SRC_USB)
// 		ret = load_image_by_usb((void *)(uintptr_t)fip_param2.monitor_runaddr, fip_param2.monitor_loadaddr,
// 					fip_param2.monitor_size, retry);
// 	else
// #endif
#if BL2_LOAD_IMAGE_SPLIT
	ret = bl2_load_image_split((void *)(uintptr_t)fip_param2.monitor_runaddr, fip_param2.monitor_loadaddr,
				   fip_param2.monitor_size, retry);
#else
	ret = load_data_from_storage((void *)(uintptr_t)fip_param2.monitor_runaddr,
				     fip_param2.monitor_loadaddr,
				     fip_param2.monitor_size, retry, 1);
#endif
	if (ret < 0) {
		ERROR("load data failed! loadaddr:0x%lx, size:%d, retry:%d\n",
		      fip_param2.monitor_loadaddr, fip_param2.monitor_size, retry);
		return ret;
	}

	crc = p_rom_api.image_crc((void *)(uintptr_t)fip_param2.monitor_runaddr, fip_param2.monitor_size);
	if (crc != fip_param2.monitor_cksum) {
		ERROR("monitor_cksum (0x%x/0x%x)\n", crc, fip_param2.monitor_cksum);
		return -1;
	}

	ret = dec_verify_image((void *)(uintptr_t)fip_param2.monitor_runaddr, fip_param2.monitor_size, 0, fip_param1);
	if (ret < 0) {
		ERROR("verify monitor (%d)\n", ret);
		return ret;
	}

	flush_dcache_range(fip_param2.monitor_runaddr, fip_param2.monitor_size);
	NOTICE("ME.\n");

	*monitor_entry = fip_param2.monitor_runaddr;

	return 0;
}

int load_bl32(int retry)
{
	uint32_t crc;
	int ret = -1;
	NOTICE("BL32/0x%lx/0x%lx/0x%x.\n", fip_param2.bl32_loadaddr,
	       fip_param2.bl32_runaddr, fip_param2.bl32_size);

	if (!fip_param2.bl32_runaddr) {
		NOTICE("No monitor.\n");
		return 0;
	}

	if (!IN_RANGE(fip_param2.bl32_runaddr, DRAM_BASE, DRAM_SIZE)) {
		ERROR("bl32_runaddr (0x%lx) is not in DRAM.\n", fip_param2.bl32_runaddr);
		plat_panic_handler();
	}

	if (!IN_RANGE(fip_param2.bl32_runaddr + fip_param2.bl32_size, DRAM_BASE, DRAM_SIZE)) {
		ERROR("bl32_size (0x%x) is not in DRAM.\n", fip_param2.bl32_size);
		plat_panic_handler();
	}

// #ifdef USB_DL_BY_FSBL
// 	if (p_rom_api.get_boot_src() == BOOT_SRC_USB)
// 		ret = load_image_by_usb((void *)(uintptr_t)fip_param2.bl32_runaddr, fip_param2.monitor_loadaddr,
// 					fip_param2.bl32_size, retry);
// 	else
// #endif
#if BL2_LOAD_IMAGE_SPLIT
	ret = bl2_load_image_split((void *)(uintptr_t)fip_param2.bl32_runaddr, fip_param2.bl32_loadaddr,
				   fip_param2.bl32_size, retry);
#else
	ret = load_data_from_storage((void *)(uintptr_t)fip_param2.bl32_runaddr,
				     fip_param2.bl32_loadaddr,
				     fip_param2.bl32_size, retry, 1);
#endif
	if (ret < 0) {
		ERROR("load data failed! loadaddr:0x%lx, size:%d, retry:%d\n",
		      fip_param2.monitor_loadaddr, fip_param2.monitor_size, retry);
		return ret;
	}

	crc = p_rom_api.image_crc((void *)(uintptr_t)fip_param2.bl32_runaddr, fip_param2.bl32_size);
	if (crc != fip_param2.bl32_cksum) {
		ERROR("monitor_cksum (0x%x/0x%x)\n", crc, fip_param2.bl32_cksum);
		return -1;
	}

	ret = dec_verify_image((void *)(uintptr_t)fip_param2.bl32_runaddr, fip_param2.bl32_size, 0, fip_param1);
	if (ret < 0) {
		ERROR("verify monitor (%d)\n", ret);
		return ret;
	}

	flush_dcache_range(fip_param2.bl32_runaddr, fip_param2.bl32_size);
	NOTICE("BL32.E.\n");
	return 0;
}

int load_blmcu(int retry)
{
	uint32_t crc;
	int ret = -1;

	NOTICE("BLMCU/0x%lx/0x%lx/0x%x.\n", fip_param2.blmcu_loadaddr, fip_param2.blmcu_runaddr,
	       fip_param2.blmcu_size);

	if (!fip_param2.blmcu_runaddr) {
		NOTICE("No blmcu.\n");
		return 0;
	}
	
#if BL2_LOAD_IMAGE_SPLIT
	ret = bl2_load_image_split((void *)(uintptr_t)fip_param2.blmcu_runaddr, fip_param2.blmcu_loadaddr,
				   fip_param2.blmcu_size, retry);
#else
	ret = load_data_from_storage((void *)(uintptr_t)fip_param2.blmcu_runaddr,
				     fip_param2.blmcu_loadaddr,
				     fip_param2.blmcu_size, retry, 1);
#endif
	if (ret < 0) {
		ERROR("load data failed! loadaddr:0x%lx, size:%d, retry:%d\n",
		      fip_param2.monitor_loadaddr, fip_param2.monitor_size, retry);
		return ret;
	}
	crc = p_rom_api.image_crc((void *)(uintptr_t)fip_param2.blmcu_runaddr, fip_param2.blmcu_size);
	if (crc != fip_param2.blmcu_cksum) {
		ERROR("blmcu_cksum (0x%x/0x%x)\n", crc, fip_param2.blmcu_cksum);
		return -1;
	}

	ret = dec_verify_image((void *)(uintptr_t)fip_param2.blmcu_runaddr, fip_param2.blmcu_size, 0, fip_param1);
	if (ret < 0) {
		ERROR("verify blmcu (%d)\n", ret);
		return ret;
	}

	mmio_write_32(0x28100248, 0x1);
	mmio_write_32(0x05025020, 0x5200080);
	mmio_write_32(0x05025024, 0x5200000);
	mmio_write_32(0x05025018, 0x1fffff);

	NOTICE("BLMCU.E.\n");

	return 0;
}

int load_loader_2nd(int retry, uint64_t *loader_2nd_entry)
{
	struct loader_2nd_header *loader_2nd_header = &sram_union_buf.loader_2nd_header;
	uint32_t crc;
	int ret = -1;
	const int cksum_offset =
		offsetof(struct loader_2nd_header, cksum) + sizeof(((struct loader_2nd_header *)0)->cksum);

	enum COMPRESS_TYPE comp_type = COMP_NONE;
	int reading_size;
	void *image_buf;

	NOTICE("L2/0x%lx.\n", fip_param2.loader_2nd_loadaddr);

// #ifdef USB_DL_BY_FSBL
// 	if (p_rom_api.get_boot_src() == BOOT_SRC_USB)
// 		ret = load_image_by_usb(loader_2nd_header, fip_param2.loader_2nd_loadaddr, BLOCK_SIZE, retry);
// 	else
// #endif
#if BL2_LOAD_IMAGE_SPLIT
	ret = bl2_load_image_split(loader_2nd_header, fip_param2.loader_2nd_loadaddr, BLOCK_SIZE, retry);
#else
	ret = load_data_from_storage(loader_2nd_header,
				     fip_param2.loader_2nd_loadaddr,
				     BLOCK_SIZE, retry, 1);
#endif
	if (ret < 0) {
		ERROR("load data failed! loadaddr:0x%lx, size:%d, retry:%d\n",
		      fip_param2.loader_2nd_loadaddr, BLOCK_SIZE, retry);
		return ret;
	}
	reading_size = ROUND_UP(loader_2nd_header->size, BLOCK_SIZE);

	NOTICE("L2/0x%x/0x%x/0x%lx/0x%x/0x%x\n", loader_2nd_header->magic, loader_2nd_header->cksum,
	       loader_2nd_header->runaddr, loader_2nd_header->size, reading_size);

	switch (loader_2nd_header->magic) {
	case LOADER_2ND_MAGIC_LZMA:
		comp_type = COMP_LZMA;
		break;
	case LOADER_2ND_MAGIC_LZ4:
		comp_type = COMP_LZ4;
		break;
	default:
		comp_type = COMP_NONE;
		break;
	}
	if (comp_type) {
		NOTICE("COMP/%d.\n", comp_type);
		image_buf = (void *)DECOMP_BUF_ADDR;
	} else {
		image_buf = (void *)loader_2nd_header->runaddr;
	}

// #ifdef USB_DL_BY_FSBL
// 	if (p_rom_api.get_boot_src() == BOOT_SRC_USB)
// 		ret = load_image_by_usb(image_buf, fip_param2.loader_2nd_loadaddr, reading_size, retry);
// 	else
// #endif
#if BL2_LOAD_IMAGE_SPLIT
	ret = bl2_load_image_split(image_buf, fip_param2.loader_2nd_loadaddr, reading_size, retry);
#else
	ret = load_data_from_storage(image_buf,
				     fip_param2.loader_2nd_loadaddr,
				     reading_size, retry, 1);
#endif
	if (ret < 0) {
		ERROR("load data failed! loadaddr:0x%lx, size:%d, retry:%d\n",
		      fip_param2.loader_2nd_loadaddr, reading_size, retry);
		return ret;
	}
	crc = p_rom_api.image_crc(image_buf + cksum_offset, loader_2nd_header->size - cksum_offset);
	if (crc != loader_2nd_header->cksum) {
		ERROR("loader_2nd_cksum (0x%x/0x%x)\n", crc, loader_2nd_header->cksum);
		return -1;
	}

	ret = dec_verify_image(image_buf + cksum_offset, loader_2nd_header->size - cksum_offset,
			       sizeof(struct loader_2nd_header) - cksum_offset, fip_param1);
	if (ret < 0) {
		ERROR("verify loader 2nd (%d)\n", ret);
		return ret;
	}

	time_records->load_loader_2nd_end = read_time_ms();

	time_records->fsbl_decomp_start = read_time_ms();
	if (comp_type) {
		size_t dst_size = DECOMP_DST_SIZE;

		// header is not compressed.
		void *dst = (void *)loader_2nd_header->runaddr;

		memcpy(dst, image_buf, sizeof(struct loader_2nd_header));
		image_buf += sizeof(struct loader_2nd_header);

		ret = decompress(dst + sizeof(struct loader_2nd_header), &dst_size, \
			image_buf, loader_2nd_header->size, comp_type);
		if (ret < 0) {
			ERROR("Failed to decompress loader_2nd (%d/%lu)\n", ret, dst_size);
			return -1;
		}

		reading_size = dst_size;
	}

	flush_dcache_range(loader_2nd_header->runaddr, reading_size);
	time_records->fsbl_decomp_end = read_time_ms();
	NOTICE("Loader_2nd loaded.\n");

	*loader_2nd_entry = loader_2nd_header->runaddr + sizeof(struct loader_2nd_header);

	return 0;
}


#ifdef NO_USE
//#ifdef	BOOT_FROM_EMMC
#define OEM_INFO_MAX_BYTE_SIZE	256
int load_oem_info(void)
{
	static int is_emmc_init = 0;
	int ret;
	char oem_info[EMMC_BLOCK_SIZE] __attribute__((aligned(EMMC_BLOCK_SIZE)));
	uint8_t dram_size_GB = 0;

	if (is_emmc_init == 0) {
		bm_emmc_init();
		is_emmc_init = 1;
	}

	/* OEM information is save in eMMC BOOT2 partition
	 * @para buf: Must be aligned EMMC_BLOCK_SIZE
	 * @para size: Must be greater than 512
	 */
	ret = emmc_boot2_read_blocks(0, (uintptr_t)oem_info, EMMC_BLOCK_SIZE);
	if (ret < 0)
		return -1;

	/* Write ddr size into OEM */
	dram_size_GB = (uint8_t)((usys0_cap_in_mbyte + usys1_cap_in_mbyte) / 1024);

	if (dram_size_GB != ((uint8_t)oem_info[0xF0])) {
		oem_info[0xF0] = (dram_size_GB & 0xFF);
		NOTICE("Rewrite ddr size into OEM, ddr_size = %d GB\n", oem_info[0xF0]);
		ret = emmc_boot2_write_blocks(0, (uintptr_t)oem_info, EMMC_BLOCK_SIZE);
		if (ret < 0)
			return -1;
	}

	/* Write OEM information to the last OEM_INFO_MAX_BYTE_SIZE bytes of RTC SRAM */
	for (int i = 0; i < OEM_INFO_MAX_BYTE_SIZE; i ++)
		mmio_write_8(OEM_INFO_RTC_SRAM_ADDR+i, oem_info[i]);

	NOTICE("Load OEM info end.\n");

// #define FSBL_PRINT_OEM_INFORMATION
#ifdef FSBL_PRINT_OEM_INFORMATION
	NOTICE("OEM info: \n");
	NOTICE("  SN0	: %s.\n", &oem_info[0x00]);	//SN0
	NOTICE("  SN1	: %s.\n", &oem_info[0x20]);	//SN1
	NOTICE("  MAC0	: %02x:%02x:%02x:%02x:%02x:%02x\n", 
			oem_info[0x40],oem_info[0x41],oem_info[0x42],oem_info[0x43],oem_info[0x44],oem_info[0x45]);	//MAC0
	NOTICE("  MAC1	: %02x:%02x:%02x:%02x:%02x:%02x\n", 
			oem_info[0x50],oem_info[0x51],oem_info[0x52],oem_info[0x53],oem_info[0x54],oem_info[0x55]);	//MAC1
	NOTICE("  product type	: %s.\n", &oem_info[0x60]);	//product type
	NOTICE("  module type	: %s.\n", &oem_info[0x70]);	//module type
	NOTICE("  inter flag	: %d.\n", oem_info[0x80]);	//interface flag
	NOTICE("  aging flag	: %d.\n", oem_info[0x81]);	//aging flag
	NOTICE("  vendor	: %s.\n", &oem_info[0x90]);		//vendor
	NOTICE("  dts type	: %s.\n", &oem_info[0xA0]);		//dts type
	NOTICE("  hw version	: %s.\n", &oem_info[0xC0]);	//hw version
	NOTICE("  produce	: %s.\n", &oem_info[0xD0]);		//product
	NOTICE("  chip		: %s.\n", &oem_info[0xE0]);		//chip
	NOTICE("  ddr size	: %s.\n", &oem_info[0xF0]);		//ddr size

	/* readback check*/
	for (int i = 0; i < OEM_INFO_MAX_BYTE_SIZE; i ++) {
		if (oem_info[i] != mmio_read_8(OEM_INFO_RTC_SRAM_ADDR+i)) {
			ERROR("cmp err %d. \n", i);
			break;
		}
	}
	NOTICE("OEM info readback check OK.\n");
#endif
	return 0;
}
#endif

void fip_src_check(void)
{
	if (p_rom_api.get_boot_src() != BOOT_SRC_RTC_NOR)
		return;

	NOTICE("Waiting for boot image in position\n");
	while (1) {
		if(mmio_read_32(PCIE_BOOT_REG) & 0x10) {
			NOTICE("The image is in position\n");
			mmio_clrbits_32(PCIE_BOOT_REG, 0x10);
			break;
		}

		feed_dog();
		mdelay(100);
	}
}

int load_rest(void)
{
	int retry = 0;

	uint64_t monitor_entry = MONITOR_RUNADDR;
	// uint64_t monitor_entry = 0;
	uint64_t loader_2nd_entry = 0x80200000 + sizeof(struct loader_2nd_header);

	// Init sys PLL and switch clocks to PLL
	sys_pll_init();

	//get current boot source
	if (p_rom_api.get_boot_src() == BOOT_SRC_EMMC)
		bm_emmc_init(); //reinit eMMC

	// Todo: nand set clk to 75M first
	mmio_write_32(0x04060000 + REG_SPI_NAND_BOOT_CTRL, 0x100);
	cv_spi_nand_set_freq(1, 1, 0);

retry_from_flash:
	for (retry = 0; retry < p_rom_api.get_number_of_retries(); retry++) {
		if (load_blcp_2nd(retry) < 0)
			continue;

		if (load_monitor(retry, &monitor_entry) < 0)
			continue;

		if (load_bl32(retry) < 0)
			continue;

		if (load_loader_2nd(retry, &loader_2nd_entry) < 0)
			continue;

		if (load_blmcu(retry) < 0)
			continue;

		break;
	}

	if (retry >= p_rom_api.get_number_of_retries()) {
		switch (p_rom_api.get_boot_src()) {
		case BOOT_SRC_UART:
		case BOOT_SRC_SD:
		case BOOT_SRC_USB:
			WARN("DL cancelled. Load flash. (%d).\n", retry);
			// Continue to boot from flash if boot from external source
			p_rom_api.flash_init();
			goto retry_from_flash;
		default:
			ERROR("Failed to load rest (%d).\n", retry);
			plat_panic_handler();
		}
	}

	sync_cache();
	console_flush();

	switch_rtc_mode_2nd_stage();

	NOTICE("M/%lx/N/%lx/", monitor_entry, loader_2nd_entry);
	if (monitor_entry) {
		INFO("From BL2 jump to BL31 at 0x%lx.\n", monitor_entry);
		jump_to_monitor(monitor_entry, loader_2nd_entry);
	} else {
		INFO("Jump to loader_2nd at 0x%lx.\n", loader_2nd_entry);
		jump_to_loader_2nd(loader_2nd_entry);
	}

	return 0;
}
