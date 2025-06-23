/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <bitwise_ops.h>
#include <console.h>
#include <platform.h>
#include <rom_api.h>
#include <bl2.h>
#include <ddr.h>
#include <string.h>
#include <decompress.h>
#include <cvi_spinor.h>
#include <delay_timer.h>
#include <security/security.h>
#include <cv_spi_nand.h>
#include <emmc/emmc.h>
#include "bl2_2nd_parse.h"

#include "cvi_board_memmap.h"

#define TRUE 1
#define FALSE 0

static enum boot_src cur_boot_src;

struct _time_records *time_records = (void *)TIME_RECORDS_ADDR;
struct fip_param1 *fip_param1 = (void *)PARAM1_BASE;
struct fip_param2 fip_param2 __aligned(BLOCK_SIZE);
extern struct _macros_misc macros_misc;

union SRAMBuffer sram_union_buf;
int init_comm_info(int ret) __attribute__((weak));
int init_comm_info(int ret)
{
	return ret;
}

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

	if (macros_misc.storage_type == BOOT_SRC_SPI_NAND) {
		static int count = 0;

		if (!count)
			get_nand_info();

		count++;
		ret = cv_spi_nand_read_data(buffer, offset, size);
	} else if (macros_misc.storage_type == BOOT_SRC_SPI_NOR) {
		ret = p_rom_api_load_image(buffer, offset, size, retry);
	} else if (macros_misc.storage_type == BOOT_SRC_EMMC) {
		if (is_eMMC_boot_partition) {
			uint32_t retry_offset =
				retry * FIP_RETRY_OFFSET + offset;

			ret = emmc_read_fip_bl2(retry_offset, size,
						(uintptr_t)buffer);
		} else {
			ret = emmc_read_blocks(offset / 512, (uintptr_t)buffer,
					       size);
		}
	}

	return ret;
}

__attribute__((weak)) int bl2_decompress(void *dst, size_t *dst_size,
					 const void *src, size_t src_size,
					 enum COMPRESS_TYPE type)
{
	int ret;
	ret = decompress(dst, dst_size, src, src_size, type);

	if (ret < 0) {
		return -1;
	}

	return ret;
}

__attribute__((weak)) int load_kernel(int retry)
{
	int ret = -1;
	void *image_buf = (void *)(uintptr_t)0x80200000;
	NOTICE("load_kernel jump_LOADADDR2:%x \n", macros_misc.jump_loadaddr);
	ret = p_rom_api_load_image(image_buf, macros_misc.jump_loadaddr,
				   macros_misc.jump_loadaddr_size, retry);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

void print_sram_log(void)
{
	uint32_t *const log_size = (void *)BOOT_LOG_LEN_ADDR;
	uint8_t *const log_buf = (void *)phys_to_dma(BOOT_LOG_BUF_BASE);
	uint32_t i;

	const char m1[] =
		"\nSRAM Log: ========================================\n";
	const char m2[] =
		"\nSRAM Log end: ====================================\n";

	for (i = 0; m1[i]; i++)
		console_putc(m1[i]);

	for (i = 0; i < *log_size; i++)
		console_putc(log_buf[i]);

	for (i = 0; m2[i]; i++)
		console_putc(m2[i]);
}

int load_param2(int retry)
{
	uint32_t crc;
	int ret = -1;

	NOTICE("P2S/0x%lx/%p.\n", sizeof(fip_param2), &fip_param2);

	if (cur_boot_src == BOOT_SRC_EMMC) {
		ret = load_data_from_storage(&fip_param2,
					     fip_param1->param2_loadaddr,
					     PARAM2_SIZE, retry, 1);
		if (ret < 0)
			return ret;
	} else {
		ret = p_rom_api_load_image(&fip_param2,
					   fip_param1->param2_loadaddr,
					   PARAM2_SIZE, retry);
		if (ret < 0)
			return ret;
	}

	if (fip_param2.magic1 != FIP_PARAM2_MAGIC1) {
		WARN("LP2_NOMAGIC\n");
		return -1;
	}

	crc = p_rom_api_image_crc(&fip_param2.reserved1,
				  sizeof(fip_param2) - 12);
	if (crc != fip_param2.param2_cksum) {
		ERROR("param2_cksum (0x%x/0x%x)\n", crc,
		      fip_param2.param2_cksum);
		return -1;
	}

	NOTICE("P2E.\n");

	return 0;
}

int load_ddr(void)
{
	int retry = 0;

retry_from_flash:
	if (retry >= p_rom_api_get_number_of_retries()) {
		switch (p_rom_api_get_boot_src()) {
		case BOOT_SRC_UART:
		case BOOT_SRC_SD:
		case BOOT_SRC_USB:
			WARN("DL cancelled. Load flash. (%d).\n", retry);
			// Continue to boot from flash if boot from external source
			p_rom_api_flash_init();
			goto retry_from_flash;
		default:
			ERROR("Failed to load DDR param (%d).\n", retry);
			panic_handler();
		}
	}

	time_records->ddr_init_start = read_time_ms();
	ddr_init(&sram_union_buf.ddr_param);
	time_records->ddr_init_end = read_time_ms();

	return 0;
}

#ifdef CONFIG_MMC_SKIP_TUNING
static void bl2_read_tuning_tap(void)
{
	int ret;
	uint32_t data;
	uint32_t bl2_tuning_read_addr =
		4 * 1024 * 1024 -
		512; //tuning tap save to eMMC boot1 last block
	uint32_t bl2_tuning_load_addr = CVIMMAP_CVI_MMC_SKIP_TUNING_ADDR;

	if (p_rom_api_get_boot_src() != BOOT_SRC_EMMC)
		return;

	NOTICE("BL2_RTL:0x%x\n", bl2_tuning_load_addr);
	ret = emmc_boot1_read_blocks(bl2_tuning_read_addr / 512,
				     (uintptr_t)(long)bl2_tuning_load_addr,
				     512);
	if (ret < 0)
		ERROR("fail to read tuning tap.\n");
	else {
		data = *(uint32_t *)(uintptr_t)bl2_tuning_load_addr;
		NOTICE("the first 4 bytes of tuning tap is:0x%x\n", data);
		mmio_write_32(0x4300240, (data & 0x7F) << 16);
	}
}
#endif

int load_blcp_2nd(int retry)
{
	uint32_t rtos_base;
	int ret = -1;
	size_t dst_size = macros_misc.decomp_dts_size;
	uint32_t load_pos_com;
	uint32_t header_size_2nd = 0;
	uint32_t part_offset_2nd;

	part_offset_2nd = macros_misc.blcp_2nd_loadaddr;

	if (macros_misc.blcp_2nd_comp_bin_addr > 0) {
		load_pos_com = macros_misc.blcp_2nd_comp_bin_addr;
	} else {
		load_pos_com = fip_param2.blcp_2nd_runaddr;
	}

	NOTICE("\t load_pos_com:%x, macros_misc.blcp_2nd_loadaddr:%x, size:%x \n",
	       load_pos_com, macros_misc.blcp_2nd_loadaddr,
	       fip_param2.blcp_2nd_size);
	// if no blcp_2nd, release_blcp_2nd should be ddr_init_end
	time_records->release_blcp_2nd = time_records->ddr_init_end;
	NOTICE("blcp_2nd_runaddr (0x%x) is \n", fip_param2.blcp_2nd_runaddr);

	if (!fip_param2.blcp_2nd_runaddr) {
		NOTICE("No C906L image.\n");
		return 0;
	}

	if (!IN_RANGE(fip_param2.blcp_2nd_runaddr, macros_misc.dram_base,
		      macros_misc.dram_size)) {
		ERROR("blcp_2nd_runaddr (0x%x) is not in DRAM.\n",
		      fip_param2.blcp_2nd_runaddr);
		panic_handler();
	}

	if (!IN_RANGE(fip_param2.blcp_2nd_runaddr + fip_param2.blcp_2nd_size,
		      macros_misc.dram_base, macros_misc.dram_size)) {
		ERROR("blcp_2nd_size (0x%x) is not in DRAM.\n",
		      fip_param2.blcp_2nd_size);
		panic_handler();
	}

	struct head *header_2nd_ptr = get_head_2nd();
	if (header_2nd_ptr != NULL) {
		// get 2nd header
		header_size_2nd = sizeof(struct head);
		ret = load_data_from_storage((void *)header_2nd_ptr,
					     part_offset_2nd, header_size_2nd,
					     retry, 0);
		if (ret < 0) {
			ERROR("error: load header, ret=0x%x\n", ret);
			goto exit;
		}
		if (header_verify_2nd()) {
			goto exit;
		}
		fip_param2.blcp_2nd_size = header_2nd_ptr->file_sz;
	} else {
		fip_param2.blcp_2nd_size = SECEND_SIZE;
	}

	NOTICE("\t load_pos:%x, part_offset_2nd:%x, size:%x\n", load_pos_com,
	       part_offset_2nd, fip_param2.blcp_2nd_size);

	ret = load_data_from_storage((void *)(uintptr_t)load_pos_com,
				     (part_offset_2nd + header_size_2nd),
				     fip_param2.blcp_2nd_size, retry, 0);

	if (ret < 0) {
		goto exit;
	}
	if (header_2nd_ptr != NULL) {
		if (verify_2nd((uint8_t *)(uintptr_t)(load_pos_com))) {
			goto exit;
		}
	}
	uint64_t data = *(uint64_t *)(uintptr_t)load_pos_com;
	NOTICE("\nthe first 8 bytes data of yoc is:0x%lx\n", data);

	ret = bl2_decompress((void *)(uintptr_t)fip_param2.blcp_2nd_runaddr,
			     &dst_size, (void *)(uintptr_t)load_pos_com,
			     (size_t)fip_param2.blcp_2nd_size, COMP_LZMA);

	/*
	crc = p_rom_api_image_crc((void *)(uintptr_t)fip_param2.blcp_2nd_runaddr, fip_param2.blcp_2nd_size);
	if (crc != fip_param2.blcp_2nd_cksum) {
		ERROR("blcp_2nd_cksum (0x%x/0x%x)\n", crc, fip_param2.blcp_2nd_cksum);
		return -1;
	}

	ret = dec_verify_image((void *)(uintptr_t)fip_param2.blcp_2nd_runaddr, fip_param2.blcp_2nd_size, 0, fip_param1);
	if (ret < 0) {
		ERROR("verify blcp 2nd (%d)\n", ret);
		return ret;
	}
*/

	flush_dcache_range(fip_param2.blcp_2nd_runaddr, dst_size);

	rtos_base = mmio_read_32(AXI_SRAM_RTOS_BASE);
	init_comm_info(0);

	switch (p_rom_api_get_boot_src()) {
		case BOOT_SRC_UART:
		// case BOOT_SRC_SD:
		// case BOOT_SRC_USB:
			break;

		default:
			time_records->release_blcp_2nd = read_time_ms();
			if (rtos_base == CVI_RTOS_MAGIC_CODE) {
				mmio_write_32(AXI_SRAM_RTOS_BASE, fip_param2.blcp_2nd_runaddr);
			} else {
				reset_c906l(fip_param2.blcp_2nd_runaddr);
			}
	}

	NOTICE("C2E.\n");
	set_boot_flag_2nd(0);
	return 0;
exit:
	set_boot_flag_2nd(1);
	ERROR("The mirror is bad\n");
	return -1;
}

int load_user_param_and_logo(int retry)
{
	int ret = -1;

	NOTICE("UPS/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/.\n",
	       macros_misc.bl_param_runaddr, macros_misc.bl_param_loadaddr,
	       macros_misc.bl_param_bak_runaddr,
	       macros_misc.bl_param_bak_loadaddr, macros_misc.bl_param_size,
	       macros_misc.bl_logo_runaddr, macros_misc.bl_logo_loadaddr,
	       macros_misc.bl_logo_size);

	if (macros_misc.bl_param_size) {
		ret = load_data_from_storage(
			(void *)(long)macros_misc.bl_param_runaddr,
			macros_misc.bl_param_loadaddr,
			macros_misc.bl_param_size, retry, 0);
		if (ret < 0) {
			return ret;
		}
		flush_dcache_range(macros_misc.bl_param_runaddr,
				   macros_misc.bl_param_size);
	}

	if (macros_misc.bl_param_bak_runaddr) {
		ret = load_data_from_storage(
			(void *)(long)macros_misc.bl_param_bak_runaddr,
			macros_misc.bl_param_bak_loadaddr,
			macros_misc.bl_param_size, retry, 0);
		if (ret < 0) {
			return ret;
		}
		flush_dcache_range(macros_misc.bl_param_bak_runaddr,
				   macros_misc.bl_param_size);
	}

	if (macros_misc.bl_logo_size) {
		ret = load_data_from_storage(
			(void *)(long)macros_misc.bl_logo_runaddr,
			macros_misc.bl_logo_loadaddr, macros_misc.bl_logo_size,
			retry, 0);
		if (ret < 0) {
			return ret;
		}
		flush_dcache_range(macros_misc.bl_logo_runaddr,
				   macros_misc.bl_logo_size);
	}

	if (macros_misc.bl_pq_size) {
		ret = load_data_from_storage(
			(void *)(long)macros_misc.bl_pq_runaddr,
			macros_misc.bl_pq_loadaddr, macros_misc.bl_pq_size,
			retry, 0);
		if (ret < 0) {
			return ret;
		}
		flush_dcache_range(macros_misc.bl_pq_runaddr,
				   macros_misc.bl_pq_size);
	}

	NOTICE("UPE.\n");

	return 0;
}

int load_monitor(int retry, uint64_t *monitor_entry)
{
	int ret = -1;
	uint32_t crc;
	NOTICE("MS/0x%x/0x%x/0x%x.\n", fip_param2.monitor_loadaddr,
	       fip_param2.monitor_runaddr, fip_param2.monitor_size);
	if (!fip_param2.monitor_runaddr) {
		NOTICE("No monitor.\n");
		return 0;
	}

	if (!IN_RANGE(fip_param2.monitor_runaddr, macros_misc.dram_base,
		      macros_misc.dram_size)) {
		ERROR("monitor_runaddr (0x%x) is not in DRAM.\n",
		      fip_param2.monitor_runaddr);
		panic_handler();
	}

	if (!IN_RANGE(fip_param2.monitor_runaddr + fip_param2.monitor_size,
		      macros_misc.dram_base, macros_misc.dram_size)) {
		ERROR("monitor_size (0x%x) is not in DRAM.\n",
		      fip_param2.monitor_size);
		panic_handler();
	}

	if (cur_boot_src == BOOT_SRC_EMMC) {
		ret = load_data_from_storage(
			(void *)(uintptr_t)fip_param2.monitor_runaddr,
			fip_param2.monitor_loadaddr, fip_param2.monitor_size,
			retry, 1);
		if (ret < 0)
			return ret;
	} else {
		ret = p_rom_api_load_image(
			(void *)(uintptr_t)fip_param2.monitor_runaddr,
			fip_param2.monitor_loadaddr, fip_param2.monitor_size,
			retry);
		if (ret < 0)
			return ret;
	}

	crc = p_rom_api_image_crc((void *)(uintptr_t)fip_param2.monitor_runaddr, fip_param2.monitor_size);
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

__attribute__((weak)) int load_bl32(int retry)
{
	uint32_t crc;
	int ret = -1;

	NOTICE("BL32/0x%x/0x%x/0x%x.\n", fip_param2.bl32_loadaddr,
	       fip_param2.bl32_runaddr, fip_param2.bl32_size);

	if (!fip_param2.bl32_runaddr) {
		NOTICE("No monitor.\n");
		return 0;
	}

	if (!IN_RANGE(fip_param2.bl32_runaddr, DRAM_BASE, DRAM_SIZE)) {
		ERROR("bl32_runaddr (0x%x) is not in DRAM.\n",
		      fip_param2.bl32_runaddr);
		panic_handler();
	}

	if (!IN_RANGE(fip_param2.bl32_runaddr + fip_param2.bl32_size, DRAM_BASE,
		      DRAM_SIZE)) {
		ERROR("bl32_size (0x%x) is not in DRAM.\n",
		      fip_param2.bl32_size);
		panic_handler();
	}

	if (cur_boot_src == BOOT_SRC_EMMC) {
		ret = load_data_from_storage(
			(void *)(uintptr_t)fip_param2.bl32_runaddr,
			fip_param2.bl32_loadaddr, fip_param2.bl32_size, retry,
			1);
		if (ret < 0)
			return ret;
	} else {
		ret = p_rom_api_load_image(
			(void *)(uintptr_t)fip_param2.bl32_runaddr,
			fip_param2.bl32_loadaddr, fip_param2.bl32_size, retry);
		if (ret < 0)
			return ret;
	}

	crc = p_rom_api_image_crc((void *)(uintptr_t)fip_param2.bl32_runaddr,
				  fip_param2.bl32_size);
	if (crc != fip_param2.bl32_cksum) {
		ERROR("monitor_cksum (0x%x/0x%x)\n", crc,
		      fip_param2.bl32_cksum);
		return -1;
	}

	ret = dec_verify_image((void *)(uintptr_t)fip_param2.bl32_runaddr,
			       fip_param2.bl32_size, 0, fip_param1);
	if (ret < 0) {
		ERROR("verify monitor (%d)\n", ret);
		return ret;
	}

	flush_dcache_range(fip_param2.bl32_runaddr, fip_param2.bl32_size);
	NOTICE("ME.\n");

	return 0;
}

__attribute__((weak)) int load_loader_2nd(int retry, uint64_t *loader_2nd_entry)
{
	struct loader_2nd_header *loader_2nd_header =
		&sram_union_buf.loader_2nd_header;
	uint32_t crc;
	int ret = -1;
	const int cksum_offset = offsetof(struct loader_2nd_header, cksum) +
				 sizeof(((struct loader_2nd_header *)0)->cksum);

	enum COMPRESS_TYPE comp_type = COMP_NONE;
	int reading_size;
	void *image_buf;

	NOTICE("L2/0x%x.\n", fip_param2.loader_2nd_loadaddr);

	if (cur_boot_src == BOOT_SRC_EMMC) {
		ret = load_data_from_storage(loader_2nd_header,
					     fip_param2.loader_2nd_loadaddr,
					     BLOCK_SIZE, retry, 1);
		if (ret < 0)
			return -1;
	} else {
		ret = p_rom_api_load_image(loader_2nd_header,
					   fip_param2.loader_2nd_loadaddr,
					   BLOCK_SIZE, retry);
		if (ret < 0)
			return -1;
	}

	reading_size = ROUND_UP(loader_2nd_header->size, BLOCK_SIZE);

	NOTICE("L2/0x%x/0x%x/0x%lx/0x%x/0x%x\n", loader_2nd_header->magic,
	       loader_2nd_header->cksum, loader_2nd_header->runaddr,
	       loader_2nd_header->size, reading_size);

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
		image_buf = (void *)(uintptr_t)macros_misc.bl_decomp_buf_addr;
	} else {
		image_buf = (void *)loader_2nd_header->runaddr;
	}

	if (cur_boot_src == BOOT_SRC_EMMC) {
		ret = load_data_from_storage(image_buf,
					     fip_param2.loader_2nd_loadaddr,
					     reading_size, retry, 1);
		if (ret < 0)
			return ret;
	} else {
		ret = p_rom_api_load_image(image_buf,
					   fip_param2.loader_2nd_loadaddr,
					   reading_size, retry);
		if (ret < 0)
			return -1;
	}

	crc = p_rom_api_image_crc(image_buf + cksum_offset,
				  loader_2nd_header->size - cksum_offset);
	if (crc != loader_2nd_header->cksum) {
		ERROR("loader_2nd_cksum (0x%x/0x%x)\n", crc,
		      loader_2nd_header->cksum);
		return -1;
	}

	ret = dec_verify_image(image_buf + cksum_offset,
			       loader_2nd_header->size - cksum_offset,
			       sizeof(struct loader_2nd_header) - cksum_offset,
			       fip_param1);
	if (ret < 0) {
		ERROR("verify loader 2nd (%d)\n", ret);
		return ret;
	}

	time_records->load_loader_2nd_end = read_time_ms();

	sys_switch_all_to_pll();

	time_records->fsbl_decomp_start = read_time_ms();
	if (comp_type) {
		size_t dst_size = DECOMP_DST_SIZE;

		// header is not compressed.
		void *dst = (void *)loader_2nd_header->runaddr;

		memcpy(dst, image_buf, sizeof(struct loader_2nd_header));
		image_buf += sizeof(struct loader_2nd_header);

		ret = decompress(dst + sizeof(struct loader_2nd_header),
				 &dst_size, image_buf, loader_2nd_header->size,
				 comp_type);
		if (ret < 0) {
			ERROR("Failed to decompress loader_2nd (%d/%lu)\n", ret,
			      dst_size);
			return -1;
		}

		reading_size = dst_size;
	}

	flush_dcache_range(loader_2nd_header->runaddr, reading_size);
	time_records->fsbl_decomp_end = read_time_ms();
	NOTICE("Loader_2nd loaded.\n");

	*loader_2nd_entry =
		loader_2nd_header->runaddr + sizeof(struct loader_2nd_header);

	return 0;
}

int load_rest(enum CHIP_CLK_MODE mode)
{
	int retry = 0;
	uint64_t monitor_entry = 0;
	uint64_t loader_2nd_entry = 0;

	// Init sys PLL and switch clocks to PLL
	sys_pll_init(mode);
#ifdef IMPROVE_AXI_CLK
	// set hsperi clock to PLL (FPLL) div by 3  = 500MHz
	mmio_write_32(0x030020B8, 0x00030009); //--> CLK_AXI4
#endif

	//get current boot source
	cur_boot_src = p_rom_api_get_boot_src();
	if (cur_boot_src == BOOT_SRC_EMMC)
		bm_emmc_init(); //reinit eMMC

retry_from_flash:
	for (retry = 0; retry < p_rom_api_get_number_of_retries(); retry++) {
		if (load_param2(retry) < 0)
			continue;

		break;
	}

	// Small hart boot failure does not affect large hart boot
	for (retry = 0; retry < p_rom_api_get_number_of_retries(); retry++) {
		if (cur_boot_src != BOOT_SRC_USB && cur_boot_src != BOOT_SRC_UART) {
			if (load_user_param_and_logo(retry) < 0)
				continue;

			if (load_blcp_2nd(retry) < 0)
				continue;
		}

		break;
	}
	for (retry = 0; retry < p_rom_api_get_number_of_retries(); retry++) {
		if (load_monitor(retry, &monitor_entry) < 0)
			continue;

		if (load_bl32(retry) < 0)
			continue;

		if (load_kernel(retry) == 1) {
			if (load_loader_2nd(retry, &loader_2nd_entry) < 0)
				continue;
		}

		break;
	}

	if (retry >= p_rom_api_get_number_of_retries()) {
		switch (p_rom_api_get_boot_src()) {
		case BOOT_SRC_UART:
		case BOOT_SRC_SD:
		case BOOT_SRC_USB:
			WARN("DL cancelled. Load flash. (%d).\n", retry);
			// Continue to boot from flash if boot from external source
			p_rom_api_flash_init();
			goto retry_from_flash;
		default:
			ERROR("Failed to load rest (%d).\n", retry);
			panic_handler();
		}
	}

#ifdef CONFIG_MMC_SKIP_TUNING
	/*
	This function is used to skiping mmc tuning process by writing
	tuning tap, which is already kept in flash, to mmc tuning register.
	Therefor, bl33/kernel can directly use this tuning tap as its default
	value to skip tuning process when switching to UHS mode.
*/
	bl2_read_tuning_tap();
#endif
#ifdef IMPROVE_AXI_CLK
	// set hsperi clock to PLL (FPLL) div by 5  = 300MHz
	mmio_write_32(0x030020B8, 0x00050009); //--> CLK_AXI4
#endif
	sync_cache();
	console_flush();

	switch_rtc_mode_2nd_stage();

	if (monitor_entry) {
		NOTICE("Jump to monitor at 0x%lx.\n", monitor_entry);
		jump_to_monitor(monitor_entry, loader_2nd_entry);
	} else {
		NOTICE("Jump to loader_2nd at 0x%lx.\n", loader_2nd_entry);
		jump_to_loader_2nd(loader_2nd_entry);
	}

	return 0;
}
