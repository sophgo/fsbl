// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 */
#include <platform_def.h>
#include <mmio.h>
#include <string.h>
#include <delay_timer.h>
#include <blds.h>
#include <console.h>

void rtc_clr_ddr_pwrok(void)
{
	mmio_clrbits_32(REG_RTC_BASE + RTC_PG_REG, 0x00000001);
}

void rtc_clr_rmio_pwrok(void)
{
	mmio_clrbits_32(REG_RTC_BASE + RTC_PG_REG, 0x00000002);
}

#ifndef SUSPEND_USE_WDG_RST
void rtc_req_suspend(void)
{
        //info("Send suspend request\n");
        /* Enable power suspend wakeup source mask */
        mmio_write_32(REG_RTC_BASE + 0x3C, 0x1); // 1 = select prdata from 32K domain
        mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0_UNLOCKKEY, 0xAB18);
        if (mmio_read_32(REG_RTC_BASE + RTC_INFO0) != 0x8051)
                mmio_setbits_32(REG_RTC_BASE + RTC_EN_PWR_WAKEUP, 0x1 << 4);
        else
                mmio_write_32(REG_RTC_BASE + RTC_EN_PWR_WAKEUP, 0x0);

	//disable WDG_RTS_REQ when suspend
	mmio_write_32(REG_RTC_BASE + 0xE0, 0);

        mmio_write_32(REG_RTC_BASE + RTC_EN_SUSPEND_REQ, 0x01);
        while (mmio_read_32(REG_RTC_BASE + RTC_EN_SUSPEND_REQ) != 0x01)
                ;
        while (1) {
            /* Send suspend request to RTC */
            mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, 0x00800080);
		    udelay(10);
        }
}
#else
void rtc_req_wdg_rst(void)
{
        uint32_t write_data = 0;

        write_data = mmio_read_32(REG_RTC_CTRL_BASE + 0x18); //rtcsys_rst_ctrl
        write_data = write_data | (0x01 << 24); //reg_rtcsys_reset_en
        mmio_write_32(REG_RTC_CTRL_BASE + 0x18, write_data); //
        mmio_write_32(REG_RTC_BASE + 0xE8, 0x04); // RTC_DB_REQ_WARM_RST
        mmio_write_32(REG_RTC_BASE + 0xE0, 0x01); // RTC_EN_WDG_RST_REQ
        mmio_write_32(REG_RTC_CTRL_BASE + 0x60, 0xA5);    // write dummy register
        mmio_write_32(REG_RTC_CTRL_BASE + 0x04, 0xAB18);          // rtc_ctrl0_unlockkey
        write_data = mmio_read_32(REG_RTC_CTRL_BASE + 0x08); // rtc_ctrl0
        //req_shdn         = rtc_ctrl0[0];
        //req_sw_thm_shdn  = rtc_ctrl0[1];
        //hw_thm_shdn_en   = rtc_ctrl0[2];
        //req_pwr_cyc      = rtc_ctrl0[3];
        //req_warm_rst     = rtc_ctrl0[4];
        //req_sw_wdg_rst   = rtc_ctrl0[5];
        //hw_wdg_rst_en    = rtc_ctrl0[6];
        //req_suspend      = rtc_ctrl0[7];
        write_data = 0xffff0000 | write_data | (0x01 << 5);
        // printf("[RTC] ----> Set req_sw_wdg_rst to 1 by register setting\n");
        mmio_write_32(REG_RTC_CTRL_BASE + 0x08, write_data); //rtc_ctrl0
}
#endif

extern uint32_t __STACKS_END__;
#define __BLDS_STACKS_SWITCH__ \
{				\
	asm volatile("ldr x0, =__STACKS_END__");	\
	asm volatile("mov x1, sp");	\
	asm volatile("sub x0, x0, #16");	\
	asm volatile("str x1, [x0]");	\
	asm volatile("sub x0, x0, #16");	\
	asm volatile("mov sp, x0");	\
}

#define __BLDS_STACKS_RESTORE__ \
{				\
	asm volatile("ldr x0, =__STACKS_END__"); \
	asm volatile("sub x0, x0, #16");	\
	asm volatile("ldr x1, [x0]");	\
	asm volatile("mov sp, x1");	\
}

void __attribute__((section(".text.entry"))) blds_entry(void)
{
	__BLDS_STACKS_SWITCH__

	//ddr enter self-refresh
	ddr_suspend_entry();

    rtc_clr_ddr_pwrok();
	rtc_clr_rmio_pwrok();

#ifndef SUSPEND_USE_WDG_RST
	//rtc request suspend
	rtc_req_suspend();
#else
	rtc_req_wdg_rst();
#endif
	__BLDS_STACKS_RESTORE__
}
