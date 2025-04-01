/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifdef DOUBLESDK
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

extern struct _macros_misc macros_misc;
extern union SRAMBuffer sram_union_buf;

int init_comm_info(int ret) __attribute__((weak));
int init_comm_info(int ret)
{
	return ret;
}
int load_user_param_and_logo(int retry);
int load_data_from_storage(void *buffer, uint32_t offset, uint32_t size,
			   int retry, u_int8_t is_eMMC_boot_partition);
int bl2_decompress(void *dst, size_t *dst_size, const void *src,
		   size_t src_size, enum COMPRESS_TYPE type);
int load_kernel(int retry);
void print_sram_log(void);
int load_param2(int retry);
int load_ddr(void);
int load_bl32(int retry);

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

int load_blcp_2nd_doublesdk(int retry)
{
	uint32_t rtos_base;
	int ret = -1;
	size_t dst_size = macros_misc.decomp_dts_size;
	uint32_t part_offset_2nd;
	uint32_t header_size_2nd = 0;
	uint32_t crc = 0;
	(void)crc;
	if (!boot_flag_A) {
		uint32_t load_pos_com;
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
		NOTICE("blcp_2nd_runaddr (0x%x) is \n",
		       fip_param2.blcp_2nd_runaddr);

		if (!fip_param2.blcp_2nd_runaddr) {
			NOTICE("No C906L image.\n");
			return 0;
		}

		if (!IN_RANGE(fip_param2.blcp_2nd_runaddr,
			      macros_misc.dram_base, macros_misc.dram_size)) {
			ERROR("blcp_2nd_runaddr (0x%x) is not in DRAM.\n",
			      fip_param2.blcp_2nd_runaddr);
			panic_handler();
		}

		if (!IN_RANGE(fip_param2.blcp_2nd_runaddr +
				      fip_param2.blcp_2nd_size,
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
						     part_offset_2nd,
						     header_size_2nd, retry, 0);
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

		NOTICE("\t load_pos:%x, part_offset_2nd:%x, size:%x\n",
		       load_pos_com, part_offset_2nd, fip_param2.blcp_2nd_size);

		ret = load_data_from_storage(
			(void *)(uintptr_t)load_pos_com,
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
		ret = bl2_decompress(
			(void *)(uintptr_t)fip_param2.blcp_2nd_runaddr,
			&dst_size, (void *)(uintptr_t)load_pos_com,
			(size_t)fip_param2.blcp_2nd_size, COMP_LZMA);

		flush_dcache_range(fip_param2.blcp_2nd_runaddr, dst_size);
	} else {
		part_offset_2nd = RTOS_PART_LOADADDR;
		// if no blcp_2nd, release_blcp_2nd should be ddr_init_end
		time_records->release_blcp_2nd = time_records->ddr_init_end;

		NOTICE("C2S/0x%x/0x%x/0x%x.\n", part_offset_2nd,
		       fip_param2.blcp_2nd_runaddr, fip_param2.freertos_a_size);

		if (!fip_param2.blcp_2nd_runaddr) {
			NOTICE("No C906L image.\n");
			return 0;
		}

		if (!IN_RANGE(fip_param2.blcp_2nd_runaddr, DRAM_BASE,
			      DRAM_SIZE)) {
			ERROR("blcp_2nd_runaddr (0x%x) is not in DRAM.\n",
			      fip_param2.blcp_2nd_runaddr);
			panic_handler();
		}

		if (!IN_RANGE(fip_param2.blcp_2nd_runaddr +
				      fip_param2.freertos_a_size,
			      DRAM_BASE, DRAM_SIZE)) {
			ERROR("blcp_2nd_size (0x%x) is not in DRAM.\n",
			      fip_param2.freertos_a_size);
			panic_handler();
		}

		struct head *header_2nd_ptr = get_head_2nd();

		if (header_2nd_ptr != NULL) {
			// get 2nd header
			header_size_2nd = sizeof(struct head);
			if (cur_boot_src == BOOT_SRC_EMMC) {
				ret = load_data_from_storage(
					(void *)header_2nd_ptr, part_offset_2nd,
					header_size_2nd, retry, 1);
			} else {
				ret = p_rom_api_load_image(
					(void *)header_2nd_ptr, part_offset_2nd,
					header_size_2nd, retry);
			}

			if (ret < 0) {
				ERROR("error: load header, ret=0x%x\n", ret);
				return -1;
			}
			if (header_verify_2nd())
				return -1;
			fip_param2.freertos_a_size = header_2nd_ptr->file_sz;
		}
		if (cur_boot_src == BOOT_SRC_EMMC) {
			ret = load_data_from_storage(
				(void *)(uintptr_t)fip_param2.blcp_2nd_runaddr,
				part_offset_2nd + header_size_2nd,
				fip_param2.freertos_a_size, retry, 1);
			if (ret < 0)
				return ret;
		} else {
			ret = p_rom_api_load_image(
				(void *)(uintptr_t)fip_param2.blcp_2nd_runaddr,
				part_offset_2nd + header_size_2nd,
				fip_param2.freertos_a_size, retry);
			if (ret < 0)
				return ret;
		}
		flush_dcache_range(fip_param2.blcp_2nd_runaddr,
				   fip_param2.freertos_a_size);
	}

	rtos_base = mmio_read_32(AXI_SRAM_RTOS_BASE);
	init_comm_info(0);

	time_records->release_blcp_2nd = read_time_ms();
	if (rtos_base == CVI_RTOS_MAGIC_CODE) {
		mmio_write_32(AXI_SRAM_RTOS_BASE, fip_param2.blcp_2nd_runaddr);
	} else {
		reset_c906l(fip_param2.blcp_2nd_runaddr);
	}

	NOTICE("C2E.\n");
	set_boot_flag_2nd(0);
	return 0;
exit:
	set_boot_flag_2nd(1);
	ERROR("The mirror is bad\n");
	return -1;
}

int load_monitor_doublesdk(int retry, uint64_t *monitor_entry)
{
	int ret = -1;
	uint32_t monitor_loadaddr = 0;
	uint32_t monitor_runaddr = 0;
	uint32_t monitor_size = 0;
	if (!boot_flag_A) {
		monitor_loadaddr = fip_param2.monitor_loadaddr;
		monitor_runaddr = fip_param2.monitor_runaddr;
		monitor_size = fip_param2.monitor_size;
	} else {
		monitor_loadaddr = fip_param2.monitor_a_loadaddr;
		monitor_runaddr = fip_param2.monitor_a_runaddr;
		monitor_size = fip_param2.monitor_a_size;
	}
	NOTICE("MS/0x%x/0x%x/0x%x.\n", monitor_loadaddr, monitor_runaddr,
	       monitor_size);

	if (!monitor_runaddr) {
		NOTICE("No monitor.\n");
		return 0;
	}

	if (!IN_RANGE(monitor_runaddr, macros_misc.dram_base,
		      macros_misc.dram_size)) {
		ERROR("monitor_runaddr (0x%x) is not in DRAM.\n",
		      monitor_runaddr);
		panic_handler();
	}

	if (!IN_RANGE(monitor_runaddr + monitor_size, macros_misc.dram_base,
		      macros_misc.dram_size)) {
		ERROR("monitor_size (0x%x) is not in DRAM.\n", monitor_size);
		panic_handler();
	}

	if (cur_boot_src == BOOT_SRC_EMMC) {
		ret = load_data_from_storage((void *)(uintptr_t)monitor_runaddr,
					     monitor_loadaddr, monitor_size,
					     retry, 1);
		if (ret < 0)
			return ret;
	} else {
		ret = p_rom_api_load_image((void *)(uintptr_t)monitor_runaddr,
					   monitor_loadaddr, monitor_size,
					   retry);
		if (ret < 0)
			return ret;
	}
	flush_dcache_range(monitor_runaddr, monitor_size);
	NOTICE("ME.\n");

	*monitor_entry = monitor_runaddr;

	return 0;
}

__attribute__((weak)) int load_loader_2nd_doublesdk(int retry,
						    uint64_t *loader_2nd_entry)
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

	if (boot_flag_A) {
		NOTICE("L2/0x%x.\n", fip_param2.loader_2nd_a_loadaddr);
		if (cur_boot_src == BOOT_SRC_EMMC) {
			ret = load_data_from_storage(
				loader_2nd_header,
				fip_param2.loader_2nd_a_loadaddr, BLOCK_SIZE,
				retry, 1);
		} else {
			ret = p_rom_api_load_image(
				loader_2nd_header,
				fip_param2.loader_2nd_a_loadaddr, BLOCK_SIZE,
				retry);
		}
		if (ret < 0)
			return -1;
	} else {
		NOTICE("L2/0x%x.\n", fip_param2.loader_2nd_loadaddr);
		if (cur_boot_src == BOOT_SRC_EMMC) {
			ret = load_data_from_storage(
				loader_2nd_header,
				fip_param2.loader_2nd_loadaddr, BLOCK_SIZE,
				retry, 1);
		} else {
			ret = p_rom_api_load_image(
				loader_2nd_header,
				fip_param2.loader_2nd_loadaddr, BLOCK_SIZE,
				retry);
		}
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

	if (boot_flag_A) {
		if (cur_boot_src == BOOT_SRC_EMMC) {
			ret = load_data_from_storage(
				image_buf, fip_param2.loader_2nd_a_loadaddr,
				reading_size, retry, 1);
		} else {
			ret = p_rom_api_load_image(
				image_buf, fip_param2.loader_2nd_a_loadaddr,
				reading_size, retry);
		}
		if (ret < 0)
			return -1;
	} else {
		if (cur_boot_src == BOOT_SRC_EMMC) {
			ret = load_data_from_storage(
				image_buf, fip_param2.loader_2nd_loadaddr,
				reading_size, retry, 1);
		} else {
			ret = p_rom_api_load_image(
				image_buf, fip_param2.loader_2nd_loadaddr,
				reading_size, retry);
		}
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

int load_rest_doublesdk(enum CHIP_CLK_MODE mode)
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
		if (!boot_flag_A) {
			if (p_rom_api_get_boot_src() != BOOT_SRC_USB && p_rom_api_get_boot_src() != BOOT_SRC_UART) {
				if (load_user_param_and_logo(retry) < 0)
					continue;
				if (load_blcp_2nd_doublesdk(retry) < 0)
					continue;
			}
			break;
		} else {
			if (load_blcp_2nd_doublesdk(retry) < 0)
				continue;
			break;
		}
	}
	for (retry = 0; retry < p_rom_api_get_number_of_retries(); retry++) {
		if (!boot_flag_A) {
			if (load_monitor_doublesdk(retry, &monitor_entry) < 0)
				continue;

			if (load_bl32(retry) < 0)
				continue;

			if (load_kernel(retry) == 1) {
				if (load_loader_2nd_doublesdk(
					    retry, &loader_2nd_entry) < 0)
					continue;
			}
		} else {
			if (load_monitor_doublesdk(retry, &monitor_entry) < 0)
				continue;
			if (load_loader_2nd_doublesdk(retry,
						      &loader_2nd_entry) < 0)
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
#endif