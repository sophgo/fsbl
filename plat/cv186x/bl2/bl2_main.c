/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <debug.h>
#include <console.h>
#include <platform.h>
#include <bl2.h>
#include <string.h>
#include <delay_timer.h>

void bl2_main(void)
{
	ATF_STATE = ATF_STATE_BL2_MAIN;

	// set uart0, emmc, sd0 bypass
	mmio_setbits_32(0x281021a4, 1 << 25 | 1 << 8 | 1<< 7 | 1 << 2 | 1 << 1);

	config_rgmii_power();
	time_records->fsbl_start = read_time_ms();
	NOTICE("\nFSBL %s:%s\n", version_string, build_message);
	INFO("sw_info=0x%x\n", get_sw_info()->value);

	INFO("fip_param1: param_cksum=0x%x param2_loadaddr=0x%x\n", fip_param1->param_cksum,
	     fip_param1->param2_loadaddr);

	INFO("CP_STATE_REG=0x%x\n", mmio_read_32(0x25047018));

	print_sram_log();

	rom_api_redirect();

	setup_dl_flag();

	//switch_rtc_mode_1st_stage();

	set_rtc_en_registers();
	bm_storage_boot_loader_version(BL2_VERSION_BASE);

	enable_top_wdt();

	load_ddr();

	load_rest();

	NOTICE("BL2 end.\n");

	while (1)
		;
}
