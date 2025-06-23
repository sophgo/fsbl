/* SPDX-License-Identifier: BSD-3-Clause */


void ddr_suspend_entry(void);
void susp_udelay(uint32_t delay);
void ddr_sys_suspend_sus_res(void);
void cvx16_ddr_phya_pd_sus_res(void);
void cvx16_ddrc_suspend_sus_res(void);
void cvx16_ddr_phyd_save_sus_res(uint32_t);

struct reg {
	uint32_t addr;
	uint32_t val;
};
