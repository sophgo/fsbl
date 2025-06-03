#include <stdio.h>
#include "ddr_sys_bring_up_pld.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define GENMASK(msb, lsb) ((2U << (msb)) - (1U << (lsb)))

uint32_t modified_bits_by_value(uint32_t orig, uint32_t value, uint32_t msb,
				uint32_t lsb)
{
	uint32_t bitmask = GENMASK(msb, lsb);

	orig &= ~bitmask;
	return (orig | ((value << lsb) & bitmask));
}

uint32_t get_bits_from_value(uint32_t value, uint32_t msb, uint32_t lsb)
{
	// if (msb < lsb)
	//     uartlog("%s: msb %u < lsb %u\n", __func__, msb, lsb);
	return ((value & GENMASK(msb, lsb)) >> lsb);
}

void cvx16_ctrl_init(uint32_t ddr_type, uint32_t ddr_datarate)
{
	if (ddr_type == 3) {
		if (ddr_datarate == 2133) {
			mmio_wr32(DDRC_BASE + 0xc, 0x63746371);
			// PATCH0.use_blk_extend:0:2:=0x1
			// PATCH0.dis_auto_ref_cnt_fix:2:1:=0x0
			// PATCH0.dis_auto_ref_algn_to_8:3:1:=0x0
			// PATCH0.starve_stall_at_dfi_ctrlupd:4:1:=0x1
			// PATCH0.starve_stall_at_abr:5:1:=0x1
			// PATCH0.dis_rdwr_switch_at_abr:6:1:=0x1
			// PATCH0.dfi_wdata_same_to_axi:7:1:=0x0
			// PATCH0.pagematch_limit_threshold:8:3=0x3
			// PATCH0.qos_sel:12:2:=0x2
			// PATCH0.timeout_wait_wr_mode:14:1:=0x1
			// PATCH0.burst_rdwr_xpi:16:4:=0x4
			// PATCH0.always_critical_when_urgent_hpr:20:1:=0x1
			// PATCH0.always_critical_when_urgent_lpr:21:1:=0x1
			// PATCH0.always_critical_when_urgent_wr:22:1:=0x1
			// PATCH0.disable_hif_rcmd_stall_path:24:1:=0x1
			// PATCH0.disable_hif_wcmd_stall_path:25:1:=0x1
			// PATCH0.derate_sys_en:29:1:=0x1
			// PATCH0.ref_4x_sys_high_temp:30:1:=0x1
			mmio_wr32(DDRC_BASE + 0x44, 0x14000000);
			// PATCH1.ref_adv_stop_threshold:0:7:=0x0
			// PATCH1.ref_adv_dec_threshold:8:7:=0x0
			// PATCH1.ref_adv_max:16:7:=0x0
			// PATCH1.burst_rdwr_wr_xpi:24:4:=0x4
			// PATCH1.use_blk_extend:28:2:=0x1
			mmio_wr32(DDRC_BASE + 0x148, 0x999F0000);
			// PATCH4.t_phyd_rden:16:6=0x0
			// PATCH4.phyd_rd_clk_stop:23:1=0x0
			// PATCH4.t_phyd_wren:24:6=0x0
			// PATCH4.phyd_wr_clk_stop:31:1=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x0, 0x81041401);
			mmio_wr32(DDRC_BASE + 0x30, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x34, 0x00A80001);
			mmio_wr32(DDRC_BASE + 0x38, 0x00020000);
			mmio_wr32(DDRC_BASE + 0x50, 0x00201070);
			mmio_wr32(DDRC_BASE + 0x60, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x64, 0x008200BB);
			mmio_wr32(DDRC_BASE + 0xc0, 0x00000000);
			mmio_wr32(DDRC_BASE + 0xc4, 0x00000000);
#ifdef DDR_INIT_SPEED_UP //
			mmio_wr32(DDRC_BASE + 0xd0, 0x00010002);
			mmio_wr32(DDRC_BASE + 0xd4, 0x00030000);
#else
			mmio_wr32(DDRC_BASE + 0xd0, 0x00010106);
			mmio_wr32(DDRC_BASE + 0xd4, 0x00790000);
#endif
			mmio_wr32(DDRC_BASE + 0xdc, 0x11240040);
#ifdef DDR_DODT
			mmio_wr32(DDRC_BASE + 0xe0, 0x04680000);
#else
			mmio_wr32(DDRC_BASE + 0xe0, 0x00680000);
#endif
			mmio_wr32(DDRC_BASE + 0xe4, 0x000C03BF);
			mmio_wr32(DDRC_BASE + 0x100, 0x0F132412);
			mmio_wr32(DDRC_BASE + 0x104, 0x00040419);
			mmio_wr32(DDRC_BASE + 0x108, 0x0507060B);
			mmio_wr32(DDRC_BASE + 0x10c, 0x00002008);
			mmio_wr32(DDRC_BASE + 0x110, 0x07020408);
			mmio_wr32(DDRC_BASE + 0x114, 0x06060403);
			mmio_wr32(DDRC_BASE + 0x120, 0x00000907);
			mmio_wr32(DDRC_BASE + 0x13c, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x180, 0xC0AB002B);
			mmio_wr32(DDRC_BASE + 0x184, 0x00000001);

			// phyd related
			mmio_wr32(DDRC_BASE + 0x190, 0x048a8306);
			// DFITMG0.dfi_t_ctrl_delay:24:5:=0x4
			// DFITMG0.dfi_rddata_use_dfi_phy_clk:23:1:=0x1
			// DFITMG0.dfi_t_rddata_en:16:7:=0xa
			// DFITMG0.dfi_wrdata_use_dfi_phy_clk:15:1:=0x1
			// DFITMG0.dfi_tphy_wrdata:8:6:=0x3
			// DFITMG0.dfi_tphy_wrlat:0:6:=0x6
			mmio_wr32(DDRC_BASE + 0x194, 0x00070202);
			// DFITMG1.dfi_t_cmd_lat:28:4:=0x0
			// DFITMG1.dfi_t_parin_lat:24:2:=0x0
			// DFITMG1.dfi_t_wrdata_delay:16:5:=0x7
			// DFITMG1.dfi_t_dram_clk_disable:8:5:=0x2
			// DFITMG1.dfi_t_dram_clk_enable:0:5:=0x2
			mmio_wr32(DDRC_BASE + 0x198, 0x07c13121);
			// DFILPCFG0.dfi_tlp_resp:24:5:=0x7
			// DFILPCFG0.dfi_lp_wakeup_dpd:20:4:=0xc
			// DFILPCFG0.dfi_lp_en_dpd:16:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_sr:12:4:=0x3
			// DFILPCFG0.dfi_lp_en_sr:8:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_pd:4:4:=0x2
			// DFILPCFG0.dfi_lp_en_pd:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x19c, 0x00000021);
			// DFILPCFG1.dfi_lp_wakeup_mpsm:4:4:=0x2
			// DFILPCFG1.dfi_lp_en_mpsm:0:1:=0x1

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x1a0, 0xC0400018);
			mmio_wr32(DDRC_BASE + 0x1a4, 0x00FE00FF);
			mmio_wr32(DDRC_BASE + 0x1a8, 0x80000000);
			mmio_wr32(DDRC_BASE + 0x1b0, 0x000002C1);
			mmio_wr32(DDRC_BASE + 0x1c0, 0x00000001);
			mmio_wr32(DDRC_BASE + 0x1c4, 0x00000001);

			// address map, auto gen.
			//cs bit1 & bit0
			mmio_wr32(DDRC_BASE + 0x200, 0x00001F1F);
			//bank bit2 & bit1 & bit0(+4, +3, +2)
			mmio_wr32(DDRC_BASE + 0x204, 0x00070707);
			//col bit 5 4 3 2
			mmio_wr32(DDRC_BASE + 0x208, 0x00000000);
			//col bit 9 8 7 6
			mmio_wr32(DDRC_BASE + 0x20c, 0x1F000000);
			//col bit x x 11 10
			mmio_wr32(DDRC_BASE + 0x210, 0x00001F1F);
			//row bit 11 jump x x
			mmio_wr32(DDRC_BASE + 0x214, 0x060F0606);
			//row bit 15 14 13 12(+21 +20 +19 +18)
			mmio_wr32(DDRC_BASE + 0x218, 0x06060606);
			//for ddr4
			mmio_wr32(DDRC_BASE + 0x21c, 0x00000606);
			//bg1 bg0 for ddr4
			mmio_wr32(DDRC_BASE + 0x220, 0x00003F3F);
			//row bit 5 4 3 2(+11 +10 +9 +8)
			mmio_wr32(DDRC_BASE + 0x224, 0x06060606);
			//row bit 9 8 7 6(+15 +14 +13 +12)
			mmio_wr32(DDRC_BASE + 0x228, 0x06060606);
			//row bit 10(+16)
			mmio_wr32(DDRC_BASE + 0x22c, 0x001F1F06);

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x240, 0x08000610);
#ifdef DDR_DODT
			mmio_wr32(DDRC_BASE + 0x244, 0x00000001);
#else
			mmio_wr32(DDRC_BASE + 0x244, 0x00000000);
#endif
			mmio_wr32(DDRC_BASE + 0x250, 0x00003F85);
			// SCHED.opt_vprw_sch:31:1:=0x0
			// SCHED.rdwr_idle_gap:24:7:=0x0
			// SCHED.go2critical_hysteresis:16:8:=0x0
			// SCHED.lpddr4_opt_act_timing:15:1:=0x0
			// SCHED.lpr_num_entries:8:7:=0x1f
			// SCHED.autopre_rmw:7:1:=0x1
			// SCHED.dis_opt_ntt_by_pre:6:1:=0x0
			// SCHED.dis_opt_ntt_by_act:5:1:=0x0
			// SCHED.opt_wrcam_fill_level:4:1:=0x0
			// SCHED.rdwr_switch_policy_sel:3:1:=0x0
			// SCHED.pageclose:2:1:=0x1
			// SCHED.prefer_write:1:1:=0x0
			// SCHED.dis_opt_wrecc_collision_flush:0:1:=0x1

			mmio_wr32(DDRC_BASE + 0x254, 0x00000000);
			// SCHED1.page_hit_limit_rd:28:3:=0x0
			// SCHED1.page_hit_limit_wr:24:3:=0x0
			// SCHED1.visible_window_limit_rd:20:3:=0x0
			// SCHED1.visible_window_limit_wr:16:3:=0x0
			// SCHED1.delay_switch_write:12:4:=0x0
			// SCHED1.pageclose_timer:0:8:=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x25c, 0x10000110);
			// PERFHPR1.hpr_xact_run_length:24:8:=0x20
			// PERFHPR1.hpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x264, 0x10000110);
			// PERFLPR1.lpr_xact_run_length:24:8:=0x20
			// PERFLPR1.lpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x26c, 0x10000110);
			// PERFWR1.w_xact_run_length:24:8:=0x20
			// PERFWR1.w_max_starve:0:16:=0x1a8

			mmio_wr32(DDRC_BASE + 0x300, 0x00000000);
			// DBG0.dis_max_rank_wr_opt:7:1:=0x0
			// DBG0.dis_max_rank_rd_opt:6:1:=0x0
			// DBG0.dis_collision_page_opt:4:1:=0x0
			// DBG0.dis_act_bypass:2:1:=0x0
			// DBG0.dis_rd_bypass:1:1:=0x0
			// DBG0.dis_wc:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x304, 0x00000000);
			// DBG1.dis_hif:1:1:=0x0
			// DBG1.dis_dq:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x30c, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x320, 0x00000001);
			// SWCTL.sw_done:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x36c, 0x00000000);
			// POISONCFG.rd_poison_intr_clr:24:1:=0x0
			// POISONCFG.rd_poison_intr_en:20:1:=0x0
			// POISONCFG.rd_poison_slverr_en:16:1:=0x0
			// POISONCFG.wr_poison_intr_clr:8:1:=0x0
			// POISONCFG.wr_poison_intr_en:4:1:=0x0
			// POISONCFG.wr_poison_slverr_en:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x400, 0x00000011);
			// PCCFG.dch_density_ratio:12:2:=0x0
			// PCCFG.bl_exp_mode:8:1:=0x0
			// PCCFG.pagematch_limit:4:1:=0x1
			// PCCFG.go2critical_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x404, 0x00006000);
			// PCFGR_0.rdwr_ordered_en:16:1:=0x0
			// PCFGR_0.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_0.rd_port_urgent_en:13:1:=0x1
			// PCFGR_0.rd_port_aging_en:12:1:=0x0
			// PCFGR_0.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_0.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x408, 0x00006000);
			// PCFGW_0.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_0.wr_port_urgent_en:13:1:=0x1
			// PCFGW_0.wr_port_aging_en:12:1:=0x0
			// PCFGW_0.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x490, 0x00000001);
			// PCTRL_0.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x494, 0x00000007);
			// PCFGQOS0_0.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_0.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_0.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_0.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_0.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x498, 0x0000006a);
			// PCFGQOS1_0.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_0.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x49c, 0x00000e07);
			// PCFGWQOS0_0.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_0.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_0.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_0.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_0.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x4a0, 0x01a801a8);
			// PCFGWQOS1_0.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_0.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x4b4, 0x00006000);
			// PCFGR_1.rdwr_ordered_en:16:1:=0x0
			// PCFGR_1.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_1.rd_port_urgent_en:13:1:=0x1
			// PCFGR_1.rd_port_aging_en:12:1:=0x0
			// PCFGR_1.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_1.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x4b8, 0x00006000);
			// PCFGW_1.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_1.wr_port_urgent_en:13:1:=0x1
			// PCFGW_1.wr_port_aging_en:12:1:=0x0
			// PCFGW_1.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x540, 0x00000001);
			// PCTRL_1.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x544, 0x00000007);
			// PCFGQOS0_1.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_1.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_1.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_1.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_1.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x548, 0x0000006a);
			// PCFGQOS1_1.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_1.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x54c, 0x00000e07);
			// PCFGWQOS0_1.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_1.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_1.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_1.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_1.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x550, 0x01a801a8);
			// PCFGWQOS1_1.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_1.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x564, 0x00006000);
			// PCFGR_2.rdwr_ordered_en:16:1:=0x0
			// PCFGR_2.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_2.rd_port_urgent_en:13:1:=0x1
			// PCFGR_2.rd_port_aging_en:12:1:=0x0
			// PCFGR_2.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_2.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x568, 0x00006000);
			// PCFGW_2.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_2.wr_port_urgent_en:13:1:=0x1
			// PCFGW_2.wr_port_aging_en:12:1:=0x0
			// PCFGW_2.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x5f0, 0x00000001);
			// PCTRL_2.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x5f4, 0x00000007);
			// PCFGQOS0_2.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_2.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_2.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_2.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_2.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x5f8, 0x0000006a);
			// PCFGQOS1_2.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_2.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x5fc, 0x00000e07);
			// PCFGWQOS0_2.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_2.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_2.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_2.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_2.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x600, 0x01a801a8);
			// PCFGWQOS1_2.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_2.wqos_map_timeout1:0:16:=0x1a8
		} else if (ddr_datarate == 1866) {
			///task ctrl_init;
			mmio_wr32(DDRC_BASE + 0xc, 0x63746371);
			// PATCH0.use_blk_extend:0:2:=0x1
			// PATCH0.dis_auto_ref_cnt_fix:2:1:=0x0
			// PATCH0.dis_auto_ref_algn_to_8:3:1:=0x0
			// PATCH0.starve_stall_at_dfi_ctrlupd:4:1:=0x1
			// PATCH0.starve_stall_at_abr:5:1:=0x1
			// PATCH0.dis_rdwr_switch_at_abr:6:1:=0x1
			// PATCH0.dfi_wdata_same_to_axi:7:1:=0x0
			// PATCH0.pagematch_limit_threshold:8:3=0x3
			// PATCH0.qos_sel:12:2:=0x2
			// PATCH0.timeout_wait_wr_mode:14:1:=0x1
			// PATCH0.burst_rdwr_xpi:16:4:=0x4
			// PATCH0.always_critical_when_urgent_hpr:20:1:=0x1
			// PATCH0.always_critical_when_urgent_lpr:21:1:=0x1
			// PATCH0.always_critical_when_urgent_wr:22:1:=0x1
			// PATCH0.disable_hif_rcmd_stall_path:24:1:=0x1
			// PATCH0.disable_hif_wcmd_stall_path:25:1:=0x1
			// PATCH0.derate_sys_en:29:1:=0x1
			// PATCH0.ref_4x_sys_high_temp:30:1:=0x1
			mmio_wr32(DDRC_BASE + 0x44, 0x14000000);
			// PATCH1.ref_adv_stop_threshold:0:7:=0x0
			// PATCH1.ref_adv_dec_threshold:8:7:=0x0
			// PATCH1.ref_adv_max:16:7:=0x0
			// PATCH1.burst_rdwr_wr_xpi:24:4:=0x4
			// PATCH1.use_blk_extend:28:2:=0x1
			mmio_wr32(DDRC_BASE + 0x148, 0x999F0000);
			// PATCH4.t_phyd_rden:16:6=0x0
			// PATCH4.phyd_rd_clk_stop:23:1=0x0
			// PATCH4.t_phyd_wren:24:6=0x0
			// PATCH4.phyd_wr_clk_stop:31:1=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x0, 0x81041401);
			mmio_wr32(DDRC_BASE + 0x30, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x34, 0x00930001);
			mmio_wr32(DDRC_BASE + 0x38, 0x00020000);
			mmio_wr32(DDRC_BASE + 0x50, 0x00201070);
			mmio_wr32(DDRC_BASE + 0x60, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x64, 0x007100A4);
			mmio_wr32(DDRC_BASE + 0xc0, 0x00000000);
			mmio_wr32(DDRC_BASE + 0xc4, 0x00000000);
#ifdef DDR_INIT_SPEED_UP
			mmio_wr32(DDRC_BASE + 0xd0, 0x00010002);
			mmio_wr32(DDRC_BASE + 0xd4, 0x00020000);
#else
			mmio_wr32(DDRC_BASE + 0xd0, 0x000100E5);
			mmio_wr32(DDRC_BASE + 0xd4, 0x006A0000);
#endif
			mmio_wr32(DDRC_BASE + 0xdc, 0x1F140040);
#ifdef DDR_DODT
			mmio_wr32(DDRC_BASE + 0xe0, 0x04600000);
#else
			mmio_wr32(DDRC_BASE + 0xe0, 0x00600000);
#endif
			mmio_wr32(DDRC_BASE + 0xe4, 0x000B03BF);
			mmio_wr32(DDRC_BASE + 0x100, 0x0E111F10);
			mmio_wr32(DDRC_BASE + 0x104, 0x00030417);
			mmio_wr32(DDRC_BASE + 0x108, 0x0507060A);
			mmio_wr32(DDRC_BASE + 0x10c, 0x00002007);
			mmio_wr32(DDRC_BASE + 0x110, 0x07020307);
			mmio_wr32(DDRC_BASE + 0x114, 0x05050303);
			mmio_wr32(DDRC_BASE + 0x120, 0x00000907);
			mmio_wr32(DDRC_BASE + 0x13c, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x180, 0xC0960026);
			mmio_wr32(DDRC_BASE + 0x184, 0x00000001);

			// phyd related
			mmio_wr32(DDRC_BASE + 0x190, 0x048a8305);
			// DFITMG0.dfi_t_ctrl_delay:24:5:=0x4
			// DFITMG0.dfi_rddata_use_dfi_phy_clk:23:1:=0x1
			// DFITMG0.dfi_t_rddata_en:16:7:=0xa
			// DFITMG0.dfi_wrdata_use_dfi_phy_clk:15:1:=0x1
			// DFITMG0.dfi_tphy_wrdata:8:6:=0x3
			// DFITMG0.dfi_tphy_wrlat:0:6:=0x5
			mmio_wr32(DDRC_BASE + 0x194, 0x00070202);
			// DFITMG1.dfi_t_cmd_lat:28:4:=0x0
			// DFITMG1.dfi_t_parin_lat:24:2:=0x0
			// DFITMG1.dfi_t_wrdata_delay:16:5:=0x7
			// DFITMG1.dfi_t_dram_clk_disable:8:5:=0x2
			// DFITMG1.dfi_t_dram_clk_enable:0:5:=0x2
			mmio_wr32(DDRC_BASE + 0x198, 0x07c13121);
			// DFILPCFG0.dfi_tlp_resp:24:5:=0x7
			// DFILPCFG0.dfi_lp_wakeup_dpd:20:4:=0xc
			// DFILPCFG0.dfi_lp_en_dpd:16:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_sr:12:4:=0x3
			// DFILPCFG0.dfi_lp_en_sr:8:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_pd:4:4:=0x2
			// DFILPCFG0.dfi_lp_en_pd:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x19c, 0x00000021);
			// DFILPCFG1.dfi_lp_wakeup_mpsm:4:4:=0x2
			// DFILPCFG1.dfi_lp_en_mpsm:0:1:=0x1

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x1a0, 0xC0400018);
			mmio_wr32(DDRC_BASE + 0x1a4, 0x00FE00FF);
			mmio_wr32(DDRC_BASE + 0x1a8, 0x80000000);
			mmio_wr32(DDRC_BASE + 0x1b0, 0x000002C1);
			mmio_wr32(DDRC_BASE + 0x1c0, 0x00000001);
			mmio_wr32(DDRC_BASE + 0x1c4, 0x00000001);

			// address map, auto gen.
			mmio_wr32(DDRC_BASE + 0x200, 0x00001F1F);
			mmio_wr32(DDRC_BASE + 0x204, 0x00070707);
			mmio_wr32(DDRC_BASE + 0x208, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x20c, 0x1F000000);
			mmio_wr32(DDRC_BASE + 0x210, 0x00001F1F);
			mmio_wr32(DDRC_BASE + 0x214, 0x060F0606);
			mmio_wr32(DDRC_BASE + 0x218, 0x06060606);
			mmio_wr32(DDRC_BASE + 0x21c, 0x00000606);
			mmio_wr32(DDRC_BASE + 0x220, 0x00003F3F);
			mmio_wr32(DDRC_BASE + 0x224, 0x06060606);
			mmio_wr32(DDRC_BASE + 0x228, 0x06060606);
			mmio_wr32(DDRC_BASE + 0x22c, 0x001F1F06);

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x240, 0x08000610);
#ifdef DDR_DODT
			mmio_wr32(DDRC_BASE + 0x244, 0x00000001);
#else
			mmio_wr32(DDRC_BASE + 0x244, 0x00000000);
#endif
			mmio_wr32(DDRC_BASE + 0x250, 0x00003F85);
			// SCHED.opt_vprw_sch:31:1:=0x0
			// SCHED.rdwr_idle_gap:24:7:=0x0
			// SCHED.go2critical_hysteresis:16:8:=0x0
			// SCHED.lpddr4_opt_act_timing:15:1:=0x0
			// SCHED.lpr_num_entries:8:7:=0x1f
			// SCHED.autopre_rmw:7:1:=0x1
			// SCHED.dis_opt_ntt_by_pre:6:1:=0x0
			// SCHED.dis_opt_ntt_by_act:5:1:=0x0
			// SCHED.opt_wrcam_fill_level:4:1:=0x0
			// SCHED.rdwr_switch_policy_sel:3:1:=0x0
			// SCHED.pageclose:2:1:=0x1
			// SCHED.prefer_write:1:1:=0x0
			// SCHED.dis_opt_wrecc_collision_flush:0:1:=0x1

			mmio_wr32(DDRC_BASE + 0x254, 0x00000000);
			// SCHED1.page_hit_limit_rd:28:3:=0x0
			// SCHED1.page_hit_limit_wr:24:3:=0x0
			// SCHED1.visible_window_limit_rd:20:3:=0x0
			// SCHED1.visible_window_limit_wr:16:3:=0x0
			// SCHED1.delay_switch_write:12:4:=0x0
			// SCHED1.pageclose_timer:0:8:=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x25c, 0x100000F0);
			// PERFHPR1.hpr_xact_run_length:24:8:=0x20
			// PERFHPR1.hpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x264, 0x100000F0);
			// PERFLPR1.lpr_xact_run_length:24:8:=0x20
			// PERFLPR1.lpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x26c, 0x100000F0);
			// PERFWR1.w_xact_run_length:24:8:=0x20
			// PERFWR1.w_max_starve:0:16:=0x1a8

			mmio_wr32(DDRC_BASE + 0x300, 0x00000000);
			// DBG0.dis_max_rank_wr_opt:7:1:=0x0
			// DBG0.dis_max_rank_rd_opt:6:1:=0x0
			// DBG0.dis_collision_page_opt:4:1:=0x0
			// DBG0.dis_act_bypass:2:1:=0x0
			// DBG0.dis_rd_bypass:1:1:=0x0
			// DBG0.dis_wc:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x304, 0x00000000);
			// DBG1.dis_hif:1:1:=0x0
			// DBG1.dis_dq:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x30c, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x320, 0x00000001);
			// SWCTL.sw_done:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x36c, 0x00000000);
			// POISONCFG.rd_poison_intr_clr:24:1:=0x0
			// POISONCFG.rd_poison_intr_en:20:1:=0x0
			// POISONCFG.rd_poison_slverr_en:16:1:=0x0
			// POISONCFG.wr_poison_intr_clr:8:1:=0x0
			// POISONCFG.wr_poison_intr_en:4:1:=0x0
			// POISONCFG.wr_poison_slverr_en:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x400, 0x00000011);
			// PCCFG.dch_density_ratio:12:2:=0x0
			// PCCFG.bl_exp_mode:8:1:=0x0
			// PCCFG.pagematch_limit:4:1:=0x1
			// PCCFG.go2critical_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x404, 0x00006000);
			// PCFGR_0.rdwr_ordered_en:16:1:=0x0
			// PCFGR_0.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_0.rd_port_urgent_en:13:1:=0x1
			// PCFGR_0.rd_port_aging_en:12:1:=0x0
			// PCFGR_0.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_0.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x408, 0x00006000);
			// PCFGW_0.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_0.wr_port_urgent_en:13:1:=0x1
			// PCFGW_0.wr_port_aging_en:12:1:=0x0
			// PCFGW_0.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x490, 0x00000001);
			// PCTRL_0.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x494, 0x00000007);
			// PCFGQOS0_0.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_0.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_0.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_0.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_0.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x498, 0x0000006a);
			// PCFGQOS1_0.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_0.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x49c, 0x00000e07);
			// PCFGWQOS0_0.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_0.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_0.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_0.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_0.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x4a0, 0x01a801a8);
			// PCFGWQOS1_0.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_0.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x4b4, 0x00006000);
			// PCFGR_1.rdwr_ordered_en:16:1:=0x0
			// PCFGR_1.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_1.rd_port_urgent_en:13:1:=0x1
			// PCFGR_1.rd_port_aging_en:12:1:=0x0
			// PCFGR_1.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_1.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x4b8, 0x00006000);
			// PCFGW_1.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_1.wr_port_urgent_en:13:1:=0x1
			// PCFGW_1.wr_port_aging_en:12:1:=0x0
			// PCFGW_1.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x540, 0x00000001);
			// PCTRL_1.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x544, 0x00000007);
			// PCFGQOS0_1.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_1.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_1.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_1.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_1.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x548, 0x0000006a);
			// PCFGQOS1_1.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_1.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x54c, 0x00000e07);
			// PCFGWQOS0_1.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_1.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_1.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_1.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_1.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x550, 0x01a801a8);
			// PCFGWQOS1_1.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_1.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x564, 0x00006000);
			// PCFGR_2.rdwr_ordered_en:16:1:=0x0
			// PCFGR_2.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_2.rd_port_urgent_en:13:1:=0x1
			// PCFGR_2.rd_port_aging_en:12:1:=0x0
			// PCFGR_2.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_2.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x568, 0x00006000);
			// PCFGW_2.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_2.wr_port_urgent_en:13:1:=0x1
			// PCFGW_2.wr_port_aging_en:12:1:=0x0
			// PCFGW_2.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x5f0, 0x00000001);
			// PCTRL_2.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x5f4, 0x00000007);
			// PCFGQOS0_2.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_2.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_2.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_2.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_2.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x5f8, 0x0000006a);
			// PCFGQOS1_2.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_2.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x5fc, 0x00000e07);
			// PCFGWQOS0_2.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_2.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_2.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_2.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_2.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x600, 0x01a801a8);
			// PCFGWQOS1_2.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_2.wqos_map_timeout1:0:16:=0x1a8
		} else if (ddr_datarate == 1600) {
			mmio_wr32(DDRC_BASE + 0xc, 0x63746371);
			// PATCH0.use_blk_extend:0:2:=0x1
			// PATCH0.dis_auto_ref_cnt_fix:2:1:=0x0
			// PATCH0.dis_auto_ref_algn_to_8:3:1:=0x0
			// PATCH0.starve_stall_at_dfi_ctrlupd:4:1:=0x1
			// PATCH0.starve_stall_at_abr:5:1:=0x1
			// PATCH0.dis_rdwr_switch_at_abr:6:1:=0x1
			// PATCH0.dfi_wdata_same_to_axi:7:1:=0x0
			// PATCH0.pagematch_limit_threshold:8:3=0x3
			// PATCH0.qos_sel:12:2:=0x2
			// PATCH0.timeout_wait_wr_mode:14:1:=0x1
			// PATCH0.burst_rdwr_xpi:16:4:=0x4
			// PATCH0.always_critical_when_urgent_hpr:20:1:=0x1
			// PATCH0.always_critical_when_urgent_lpr:21:1:=0x1
			// PATCH0.always_critical_when_urgent_wr:22:1:=0x1
			// PATCH0.disable_hif_rcmd_stall_path:24:1:=0x1
			// PATCH0.disable_hif_wcmd_stall_path:25:1:=0x1
			// PATCH0.derate_sys_en:29:1:=0x1
			// PATCH0.ref_4x_sys_high_temp:30:1:=0x1
			mmio_wr32(DDRC_BASE + 0x44, 0x14000000);
			// PATCH1.ref_adv_stop_threshold:0:7:=0x0
			// PATCH1.ref_adv_dec_threshold:8:7:=0x0
			// PATCH1.ref_adv_max:16:7:=0x0
			// PATCH1.burst_rdwr_wr_xpi:24:4:=0x4
			// PATCH1.use_blk_extend:28:2:=0x1
			mmio_wr32(DDRC_BASE + 0x148, 0x989E0000);
			// PATCH4.t_phyd_rden:16:6=0x0
			// PATCH4.phyd_rd_clk_stop:23:1=0x0
			// PATCH4.t_phyd_wren:24:6=0x0
			// PATCH4.phyd_wr_clk_stop:31:1=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x0, 0x81041401);
			mmio_wr32(DDRC_BASE + 0x30, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x34, 0x007E0001);
			mmio_wr32(DDRC_BASE + 0x38, 0x00020000);
			mmio_wr32(DDRC_BASE + 0x50, 0x00201070);
			mmio_wr32(DDRC_BASE + 0x60, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x64, 0x0061008C);
			mmio_wr32(DDRC_BASE + 0xc0, 0x00000000);
			mmio_wr32(DDRC_BASE + 0xc4, 0x00000000);
#ifdef DDR_INIT_SPEED_UP
			mmio_wr32(DDRC_BASE + 0xd0, 0x00010002);
			mmio_wr32(DDRC_BASE + 0xd4, 0x00020000);
#else
			mmio_wr32(DDRC_BASE + 0xd0, 0x000100C5);
			mmio_wr32(DDRC_BASE + 0xd4, 0x005B0000);
#endif
			mmio_wr32(DDRC_BASE + 0xdc, 0x1D700040);
#ifdef DDR_DODT
			mmio_wr32(DDRC_BASE + 0xe0, 0x04580000);
#else
			mmio_wr32(DDRC_BASE + 0xe0, 0x00580000);
#endif
			mmio_wr32(DDRC_BASE + 0xe4, 0x000903BF);
			mmio_wr32(DDRC_BASE + 0x100, 0x0C101A0E);
			mmio_wr32(DDRC_BASE + 0x104, 0x00030314);
			mmio_wr32(DDRC_BASE + 0x108, 0x04060509);
			mmio_wr32(DDRC_BASE + 0x10c, 0x00002006);
			mmio_wr32(DDRC_BASE + 0x110, 0x06020306);
			mmio_wr32(DDRC_BASE + 0x114, 0x04040302);
			mmio_wr32(DDRC_BASE + 0x120, 0x00000906);
			mmio_wr32(DDRC_BASE + 0x13c, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x180, 0xC0800020);
			mmio_wr32(DDRC_BASE + 0x184, 0x00000001);

			// phyd related
			mmio_wr32(DDRC_BASE + 0x190, 0x04888304);
			// DFITMG0.dfi_t_ctrl_delay:24:5:=0x4
			// DFITMG0.dfi_rddata_use_dfi_phy_clk:23:1:=0x1
			// DFITMG0.dfi_t_rddata_en:16:7:=0x8
			// DFITMG0.dfi_wrdata_use_dfi_phy_clk:15:1:=0x1
			// DFITMG0.dfi_tphy_wrdata:8:6:=0x3
			// DFITMG0.dfi_tphy_wrlat:0:6:=0x4
			mmio_wr32(DDRC_BASE + 0x194, 0x00070202);
			// DFITMG1.dfi_t_cmd_lat:28:4:=0x0
			// DFITMG1.dfi_t_parin_lat:24:2:=0x0
			// DFITMG1.dfi_t_wrdata_delay:16:5:=0x7
			// DFITMG1.dfi_t_dram_clk_disable:8:5:=0x2
			// DFITMG1.dfi_t_dram_clk_enable:0:5:=0x2
			mmio_wr32(DDRC_BASE + 0x198, 0x07c13121);
			// DFILPCFG0.dfi_tlp_resp:24:5:=0x7
			// DFILPCFG0.dfi_lp_wakeup_dpd:20:4:=0xc
			// DFILPCFG0.dfi_lp_en_dpd:16:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_sr:12:4:=0x3
			// DFILPCFG0.dfi_lp_en_sr:8:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_pd:4:4:=0x2
			// DFILPCFG0.dfi_lp_en_pd:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x19c, 0x00000021);
			// DFILPCFG1.dfi_lp_wakeup_mpsm:4:4:=0x2
			// DFILPCFG1.dfi_lp_en_mpsm:0:1:=0x1

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x1a0, 0xC0400018);
			mmio_wr32(DDRC_BASE + 0x1a4, 0x00FE00FF);
			mmio_wr32(DDRC_BASE + 0x1a8, 0x80000000);
			mmio_wr32(DDRC_BASE + 0x1b0, 0x000002C1);
			mmio_wr32(DDRC_BASE + 0x1c0, 0x00000001);
			mmio_wr32(DDRC_BASE + 0x1c4, 0x00000001);

			// address map, auto gen.
			mmio_wr32(DDRC_BASE + 0x200, 0x00001F1F);
			mmio_wr32(DDRC_BASE + 0x204, 0x00070707);
			mmio_wr32(DDRC_BASE + 0x208, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x20c, 0x1F000000);
			mmio_wr32(DDRC_BASE + 0x210, 0x00001F1F);
			mmio_wr32(DDRC_BASE + 0x214, 0x060F0606);
			mmio_wr32(DDRC_BASE + 0x218, 0x06060606);
			mmio_wr32(DDRC_BASE + 0x21c, 0x00000606);
			mmio_wr32(DDRC_BASE + 0x220, 0x00003F3F);
			mmio_wr32(DDRC_BASE + 0x224, 0x06060606);
			mmio_wr32(DDRC_BASE + 0x228, 0x06060606);
			mmio_wr32(DDRC_BASE + 0x22c, 0x001F1F06);

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x240, 0x0800060C);
#ifdef DDR_DODT
			mmio_wr32(DDRC_BASE + 0x244, 0x00000001);
#else
			mmio_wr32(DDRC_BASE + 0x244, 0x00000000);
#endif
			mmio_wr32(DDRC_BASE + 0x250, 0x00003F85);
			// SCHED.opt_vprw_sch:31:1:=0x0
			// SCHED.rdwr_idle_gap:24:7:=0x0
			// SCHED.go2critical_hysteresis:16:8:=0x0
			// SCHED.lpddr4_opt_act_timing:15:1:=0x0
			// SCHED.lpr_num_entries:8:7:=0x1f
			// SCHED.autopre_rmw:7:1:=0x1
			// SCHED.dis_opt_ntt_by_pre:6:1:=0x0
			// SCHED.dis_opt_ntt_by_act:5:1:=0x0
			// SCHED.opt_wrcam_fill_level:4:1:=0x0
			// SCHED.rdwr_switch_policy_sel:3:1:=0x0
			// SCHED.pageclose:2:1:=0x1
			// SCHED.prefer_write:1:1:=0x0
			// SCHED.dis_opt_wrecc_collision_flush:0:1:=0x1

			mmio_wr32(DDRC_BASE + 0x254, 0x00000000);
			// SCHED1.page_hit_limit_rd:28:3:=0x0
			// SCHED1.page_hit_limit_wr:24:3:=0x0
			// SCHED1.visible_window_limit_rd:20:3:=0x0
			// SCHED1.visible_window_limit_wr:16:3:=0x0
			// SCHED1.delay_switch_write:12:4:=0x0
			// SCHED1.pageclose_timer:0:8:=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x25c, 0x100000C8);
			// PERFHPR1.hpr_xact_run_length:24:8:=0x20
			// PERFHPR1.hpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x264, 0x100000C8);
			// PERFLPR1.lpr_xact_run_length:24:8:=0x20
			// PERFLPR1.lpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x26c, 0x100000C8);
			// PERFWR1.w_xact_run_length:24:8:=0x20
			// PERFWR1.w_max_starve:0:16:=0x1a8

			mmio_wr32(DDRC_BASE + 0x300, 0x00000000);
			// DBG0.dis_max_rank_wr_opt:7:1:=0x0
			// DBG0.dis_max_rank_rd_opt:6:1:=0x0
			// DBG0.dis_collision_page_opt:4:1:=0x0
			// DBG0.dis_act_bypass:2:1:=0x0
			// DBG0.dis_rd_bypass:1:1:=0x0
			// DBG0.dis_wc:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x304, 0x00000000);
			// DBG1.dis_hif:1:1:=0x0
			// DBG1.dis_dq:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x30c, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x320, 0x00000001);
			// SWCTL.sw_done:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x36c, 0x00000000);
			// POISONCFG.rd_poison_intr_clr:24:1:=0x0
			// POISONCFG.rd_poison_intr_en:20:1:=0x0
			// POISONCFG.rd_poison_slverr_en:16:1:=0x0
			// POISONCFG.wr_poison_intr_clr:8:1:=0x0
			// POISONCFG.wr_poison_intr_en:4:1:=0x0
			// POISONCFG.wr_poison_slverr_en:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x400, 0x00000011);
			// PCCFG.dch_density_ratio:12:2:=0x0
			// PCCFG.bl_exp_mode:8:1:=0x0
			// PCCFG.pagematch_limit:4:1:=0x1
			// PCCFG.go2critical_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x404, 0x00006000);
			// PCFGR_0.rdwr_ordered_en:16:1:=0x0
			// PCFGR_0.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_0.rd_port_urgent_en:13:1:=0x1
			// PCFGR_0.rd_port_aging_en:12:1:=0x0
			// PCFGR_0.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_0.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x408, 0x00006000);
			// PCFGW_0.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_0.wr_port_urgent_en:13:1:=0x1
			// PCFGW_0.wr_port_aging_en:12:1:=0x0
			// PCFGW_0.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x490, 0x00000001);
			// PCTRL_0.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x494, 0x00000007);
			// PCFGQOS0_0.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_0.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_0.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_0.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_0.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x498, 0x0000006a);
			// PCFGQOS1_0.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_0.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x49c, 0x00000e07);
			// PCFGWQOS0_0.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_0.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_0.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_0.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_0.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x4a0, 0x01a801a8);
			// PCFGWQOS1_0.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_0.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x4b4, 0x00006000);
			// PCFGR_1.rdwr_ordered_en:16:1:=0x0
			// PCFGR_1.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_1.rd_port_urgent_en:13:1:=0x1
			// PCFGR_1.rd_port_aging_en:12:1:=0x0
			// PCFGR_1.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_1.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x4b8, 0x00006000);
			// PCFGW_1.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_1.wr_port_urgent_en:13:1:=0x1
			// PCFGW_1.wr_port_aging_en:12:1:=0x0
			// PCFGW_1.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x540, 0x00000001);
			// PCTRL_1.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x544, 0x00000007);
			// PCFGQOS0_1.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_1.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_1.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_1.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_1.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x548, 0x0000006a);
			// PCFGQOS1_1.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_1.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x54c, 0x00000e07);
			// PCFGWQOS0_1.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_1.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_1.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_1.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_1.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x550, 0x01a801a8);
			// PCFGWQOS1_1.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_1.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x564, 0x00006000);
			// PCFGR_2.rdwr_ordered_en:16:1:=0x0
			// PCFGR_2.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_2.rd_port_urgent_en:13:1:=0x1
			// PCFGR_2.rd_port_aging_en:12:1:=0x0
			// PCFGR_2.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_2.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x568, 0x00006000);
			// PCFGW_2.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_2.wr_port_urgent_en:13:1:=0x1
			// PCFGW_2.wr_port_aging_en:12:1:=0x0
			// PCFGW_2.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x5f0, 0x00000001);
			// PCTRL_2.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x5f4, 0x00000007);
			// PCFGQOS0_2.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_2.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_2.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_2.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_2.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x5f8, 0x0000006a);
			// PCFGQOS1_2.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_2.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x5fc, 0x00000e07);
			// PCFGWQOS0_2.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_2.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_2.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_2.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_2.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x600, 0x01a801a8);
			// PCFGWQOS1_2.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_2.wqos_map_timeout1:0:16:=0x1a8
		}
	} else {
		if (ddr_datarate == 1333) {
			mmio_wr32(DDRC_BASE + 0xc, 0x63746371);
			// PATCH0.use_blk_extend:0:2:=0x1
			// PATCH0.dis_auto_ref_cnt_fix:2:1:=0x0
			// PATCH0.dis_auto_ref_algn_to_8:3:1:=0x0
			// PATCH0.starve_stall_at_dfi_ctrlupd:4:1:=0x1
			// PATCH0.starve_stall_at_abr:5:1:=0x1
			// PATCH0.dis_rdwr_switch_at_abr:6:1:=0x1
			// PATCH0.dfi_wdata_same_to_axi:7:1:=0x0
			// PATCH0.pagematch_limit_threshold:8:3=0x3
			// PATCH0.qos_sel:12:2:=0x2
			// PATCH0.timeout_wait_wr_mode:14:1:=0x1
			// PATCH0.burst_rdwr_xpi:16:4:=0x4
			// PATCH0.always_critical_when_urgent_hpr:20:1:=0x1
			// PATCH0.always_critical_when_urgent_lpr:21:1:=0x1
			// PATCH0.always_critical_when_urgent_wr:22:1:=0x1
			// PATCH0.disable_hif_rcmd_stall_path:24:1:=0x1
			// PATCH0.disable_hif_wcmd_stall_path:25:1:=0x1
			// PATCH0.derate_sys_en:29:1:=0x1
			// PATCH0.ref_4x_sys_high_temp:30:1:=0x1
			mmio_wr32(DDRC_BASE + 0x44, 0x14000000);
			// PATCH1.ref_adv_stop_threshold:0:7:=0x0
			// PATCH1.ref_adv_dec_threshold:8:7:=0x0
			// PATCH1.ref_adv_max:16:7:=0x0
			// PATCH1.burst_rdwr_wr_xpi:24:4:=0x4
			// PATCH1.use_blk_extend:28:2:=0x1
			mmio_wr32(DDRC_BASE + 0x148, 0x989D0000);
			// PATCH4.t_phyd_rden:16:6=0x0
			// PATCH4.phyd_rd_clk_stop:23:1=0x0
			// PATCH4.t_phyd_wren:24:6=0x0
			// PATCH4.phyd_wr_clk_stop:31:1=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x0, 0x81041400);
			mmio_wr32(DDRC_BASE + 0x30, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x34, 0x006A0001);
			mmio_wr32(DDRC_BASE + 0x38, 0x00020000);
			mmio_wr32(DDRC_BASE + 0x50, 0x00201070);
			mmio_wr32(DDRC_BASE + 0x60, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x64, 0x0051006E);
#ifdef DDR_INIT_SPEED_UP
			mmio_wr32(DDRC_BASE + 0xd0, 0x00010002);
			mmio_wr32(DDRC_BASE + 0xd4, 0x00000000);
#else
			mmio_wr32(DDRC_BASE + 0xd0, 0x00010043);
			mmio_wr32(DDRC_BASE + 0xd4, 0x00000000);
#endif
			mmio_wr32(DDRC_BASE + 0xdc, 0x03030040);
			mmio_wr32(DDRC_BASE + 0xe0, 0x00800000);
			mmio_wr32(DDRC_BASE + 0x100, 0x0B0F1610);
			mmio_wr32(DDRC_BASE + 0x104, 0x00030414);
			mmio_wr32(DDRC_BASE + 0x108, 0x04050409);
			mmio_wr32(DDRC_BASE + 0x10c, 0x00003004);
			mmio_wr32(DDRC_BASE + 0x110, 0x05020406);
			mmio_wr32(DDRC_BASE + 0x114, 0x01010303);
			mmio_wr32(DDRC_BASE + 0x120, 0x00000505);

			// phyd related
			mmio_wr32(DDRC_BASE + 0x190, 0x04878304);
			// DFITMG0.dfi_t_ctrl_delay:24:5:=0x4
			// DFITMG0.dfi_rddata_use_dfi_phy_clk:23:1:=0x1
			// DFITMG0.dfi_t_rddata_en:16:7:=0x6
			// DFITMG0.dfi_wrdata_use_dfi_phy_clk:15:1:=0x1
			// DFITMG0.dfi_tphy_wrdata:8:6:=0x3
			// DFITMG0.dfi_tphy_wrlat:0:6:=0x4
			mmio_wr32(DDRC_BASE + 0x194, 0x00070102);
			// DFITMG1.dfi_t_cmd_lat:28:4:=0x0
			// DFITMG1.dfi_t_parin_lat:24:2:=0x0
			// DFITMG1.dfi_t_wrdata_delay:16:5:=0x7
			// DFITMG1.dfi_t_dram_clk_disable:8:5:=0x2
			// DFITMG1.dfi_t_dram_clk_enable:0:5:=0x2
			mmio_wr32(DDRC_BASE + 0x198, 0x07c13121);
			// DFILPCFG0.dfi_tlp_resp:24:5:=0x7
			// DFILPCFG0.dfi_lp_wakeup_dpd:20:4:=0xc
			// DFILPCFG0.dfi_lp_en_dpd:16:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_sr:12:4:=0x3
			// DFILPCFG0.dfi_lp_en_sr:8:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_pd:4:4:=0x2
			// DFILPCFG0.dfi_lp_en_pd:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x19c, 0x00000021);
			// DFILPCFG1.dfi_lp_wakeup_mpsm:4:4:=0x2
			// DFILPCFG1.dfi_lp_en_mpsm:0:1:=0x1

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x1a0, 0xC0400018);
			mmio_wr32(DDRC_BASE + 0x1a4, 0x00FE00FF);
			mmio_wr32(DDRC_BASE + 0x1a8, 0x80000000);
			mmio_wr32(DDRC_BASE + 0x1b0, 0x000002C1);
			mmio_wr32(DDRC_BASE + 0x1c0, 0x00000001);
			mmio_wr32(DDRC_BASE + 0x1c4, 0x00000001);

			// address map, auto gen.
			// support from 0.5Gb to 4Gb
			// R[17:13]B[2]R[12:0]B[1:0]C[9:0]
			mmio_wr32(DDRC_BASE + 0x200, 0x00001F1F);
			mmio_wr32(DDRC_BASE + 0x204, 0x00140707);
			mmio_wr32(DDRC_BASE + 0x208, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x20c, 0x1F000000);
			mmio_wr32(DDRC_BASE + 0x210, 0x00001F1F);
			mmio_wr32(DDRC_BASE + 0x214, 0x050F0505);
			mmio_wr32(DDRC_BASE + 0x218, 0x06060605);
			mmio_wr32(DDRC_BASE + 0x21c, 0x00000606);
			mmio_wr32(DDRC_BASE + 0x220, 0x00003F3F);
			mmio_wr32(DDRC_BASE + 0x224, 0x05050505);
			mmio_wr32(DDRC_BASE + 0x228, 0x05050505);
			mmio_wr32(DDRC_BASE + 0x22c, 0x001F1F05);

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x240, 0x07030710); //TBD
			mmio_wr32(DDRC_BASE + 0x244, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x250, 0x00003F85);
			// SCHED.opt_vprw_sch:31:1:=0x0
			// SCHED.rdwr_idle_gap:24:7:=0x0
			// SCHED.go2critical_hysteresis:16:8:=0x0
			// SCHED.lpddr4_opt_act_timing:15:1:=0x0
			// SCHED.lpr_num_entries:8:7:=0x1f
			// SCHED.autopre_rmw:7:1:=0x1
			// SCHED.dis_opt_ntt_by_pre:6:1:=0x0
			// SCHED.dis_opt_ntt_by_act:5:1:=0x0
			// SCHED.opt_wrcam_fill_level:4:1:=0x0
			// SCHED.rdwr_switch_policy_sel:3:1:=0x0
			// SCHED.pageclose:2:1:=0x1
			// SCHED.prefer_write:1:1:=0x0
			// SCHED.dis_opt_wrecc_collision_flush:0:1:=0x1

			mmio_wr32(DDRC_BASE + 0x254, 0x00000020);
			// SCHED1.page_hit_limit_rd:28:3:=0x0
			// SCHED1.page_hit_limit_wr:24:3:=0x0
			// SCHED1.visible_window_limit_rd:20:3:=0x0
			// SCHED1.visible_window_limit_wr:16:3:=0x0
			// SCHED1.delay_switch_write:12:4:=0x0
			// SCHED1.pageclose_timer:0:8:=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x25c, 0x100000A8);
			// PERFHPR1.hpr_xact_run_length:24:8:=0x20
			// PERFHPR1.hpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x264, 0x100000A8);
			// PERFLPR1.lpr_xact_run_length:24:8:=0x20
			// PERFLPR1.lpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x26c, 0x100000A8);
			// PERFWR1.w_xact_run_length:24:8:=0x20
			// PERFWR1.w_max_starve:0:16:=0x1a8

			mmio_wr32(DDRC_BASE + 0x300, 0x00000000);
			// DBG0.dis_max_rank_wr_opt:7:1:=0x0
			// DBG0.dis_max_rank_rd_opt:6:1:=0x0
			// DBG0.dis_collision_page_opt:4:1:=0x0
			// DBG0.dis_act_bypass:2:1:=0x0
			// DBG0.dis_rd_bypass:1:1:=0x0
			// DBG0.dis_wc:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x304, 0x00000000);
			// DBG1.dis_hif:1:1:=0x0
			// DBG1.dis_dq:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x30c, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x320, 0x00000001);
			// SWCTL.sw_done:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x36c, 0x00000000);
			// POISONCFG.rd_poison_intr_clr:24:1:=0x0
			// POISONCFG.rd_poison_intr_en:20:1:=0x0
			// POISONCFG.rd_poison_slverr_en:16:1:=0x0
			// POISONCFG.wr_poison_intr_clr:8:1:=0x0
			// POISONCFG.wr_poison_intr_en:4:1:=0x0
			// POISONCFG.wr_poison_slverr_en:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x400, 0x00000011);
			// PCCFG.dch_density_ratio:12:2:=0x0
			// PCCFG.bl_exp_mode:8:1:=0x0
			// PCCFG.pagematch_limit:4:1:=0x1
			// PCCFG.go2critical_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x404, 0x00006000);
			// PCFGR_0.rdwr_ordered_en:16:1:=0x0
			// PCFGR_0.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_0.rd_port_urgent_en:13:1:=0x1
			// PCFGR_0.rd_port_aging_en:12:1:=0x0
			// PCFGR_0.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_0.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x408, 0x00006000);
			// PCFGW_0.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_0.wr_port_urgent_en:13:1:=0x1
			// PCFGW_0.wr_port_aging_en:12:1:=0x0
			// PCFGW_0.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x490, 0x00000001);
			// PCTRL_0.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x494, 0x00000007);
			// PCFGQOS0_0.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_0.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_0.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_0.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_0.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x498, 0x0000006a);
			// PCFGQOS1_0.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_0.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x49c, 0x00000e07);
			// PCFGWQOS0_0.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_0.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_0.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_0.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_0.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x4a0, 0x01a801a8);
			// PCFGWQOS1_0.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_0.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x4b4, 0x00006000);
			// PCFGR_1.rdwr_ordered_en:16:1:=0x0
			// PCFGR_1.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_1.rd_port_urgent_en:13:1:=0x1
			// PCFGR_1.rd_port_aging_en:12:1:=0x0
			// PCFGR_1.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_1.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x4b8, 0x00006000);
			// PCFGW_1.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_1.wr_port_urgent_en:13:1:=0x1
			// PCFGW_1.wr_port_aging_en:12:1:=0x0
			// PCFGW_1.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x540, 0x00000001);
			// PCTRL_1.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x544, 0x00000007);
			// PCFGQOS0_1.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_1.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_1.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_1.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_1.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x548, 0x0000006a);
			// PCFGQOS1_1.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_1.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x54c, 0x00000e07);
			// PCFGWQOS0_1.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_1.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_1.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_1.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_1.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x550, 0x01a801a8);
			// PCFGWQOS1_1.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_1.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x564, 0x00006000);
			// PCFGR_2.rdwr_ordered_en:16:1:=0x0
			// PCFGR_2.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_2.rd_port_urgent_en:13:1:=0x1
			// PCFGR_2.rd_port_aging_en:12:1:=0x0
			// PCFGR_2.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_2.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x568, 0x00006000);
			// PCFGW_2.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_2.wr_port_urgent_en:13:1:=0x1
			// PCFGW_2.wr_port_aging_en:12:1:=0x0
			// PCFGW_2.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x5f0, 0x00000001);
			// PCTRL_2.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x5f4, 0x00000007);
			// PCFGQOS0_2.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_2.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_2.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_2.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_2.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x5f8, 0x0000006a);
			// PCFGQOS1_2.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_2.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x5fc, 0x00000e07);
			// PCFGWQOS0_2.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_2.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_2.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_2.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_2.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x600, 0x01a801a8);
			// PCFGWQOS1_2.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_2.wqos_map_timeout1:0:16:=0x1a8
		} else if (ddr_datarate == 1066) {
			mmio_wr32(DDRC_BASE + 0xc, 0x63746371);
			// PATCH0.use_blk_extend:0:2:=0x1
			// PATCH0.dis_auto_ref_cnt_fix:2:1:=0x0
			// PATCH0.dis_auto_ref_algn_to_8:3:1:=0x0
			// PATCH0.starve_stall_at_dfi_ctrlupd:4:1:=0x1
			// PATCH0.starve_stall_at_abr:5:1:=0x1
			// PATCH0.dis_rdwr_switch_at_abr:6:1:=0x1
			// PATCH0.dfi_wdata_same_to_axi:7:1:=0x0
			// PATCH0.pagematch_limit_threshold:8:3=0x3
			// PATCH0.qos_sel:12:2:=0x2
			// PATCH0.timeout_wait_wr_mode:14:1:=0x1
			// PATCH0.burst_rdwr_xpi:16:4:=0x4
			// PATCH0.always_critical_when_urgent_hpr:20:1:=0x1
			// PATCH0.always_critical_when_urgent_lpr:21:1:=0x1
			// PATCH0.always_critical_when_urgent_wr:22:1:=0x1
			// PATCH0.disable_hif_rcmd_stall_path:24:1:=0x1
			// PATCH0.disable_hif_wcmd_stall_path:25:1:=0x1
			// PATCH0.derate_sys_en:29:1:=0x1
			// PATCH0.ref_4x_sys_high_temp:30:1:=0x1
			mmio_wr32(DDRC_BASE + 0x44, 0x14000000);
			// PATCH1.ref_adv_stop_threshold:0:7:=0x0
			// PATCH1.ref_adv_dec_threshold:8:7:=0x0
			// PATCH1.ref_adv_max:16:7:=0x0
			// PATCH1.burst_rdwr_wr_xpi:24:4:=0x4
			// PATCH1.use_blk_extend:28:2:=0x1
			mmio_wr32(DDRC_BASE + 0x148, 0x979C0000);
			// PATCH4.t_phyd_rden:16:6=0x0
			// PATCH4.phyd_rd_clk_stop:23:1=0x0
			// PATCH4.t_phyd_wren:24:6=0x0
			// PATCH4.phyd_wr_clk_stop:31:1=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x0, 0x81041400);
			mmio_wr32(DDRC_BASE + 0x30, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x34, 0x00550001);
			mmio_wr32(DDRC_BASE + 0x38, 0x00020000);
			mmio_wr32(DDRC_BASE + 0x50, 0x00201070);
			mmio_wr32(DDRC_BASE + 0x60, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x64, 0x00410058);
#ifdef DDR_INIT_SPEED_UP
			mmio_wr32(DDRC_BASE + 0xd0, 0x00010002);
			mmio_wr32(DDRC_BASE + 0xd4, 0x00000000);
#else
			mmio_wr32(DDRC_BASE + 0xd0, 0x00010036);
			mmio_wr32(DDRC_BASE + 0xd4, 0x00000000);
#endif
			mmio_wr32(DDRC_BASE + 0xdc, 0x0F730040);
			mmio_wr32(DDRC_BASE + 0xe0, 0x00800000);
			mmio_wr32(DDRC_BASE + 0x100, 0x090C110C);
			mmio_wr32(DDRC_BASE + 0x104, 0x00020310);
			mmio_wr32(DDRC_BASE + 0x108, 0x03040407);
			mmio_wr32(DDRC_BASE + 0x10c, 0x00001004);
			mmio_wr32(DDRC_BASE + 0x110, 0x04020304);
			mmio_wr32(DDRC_BASE + 0x114, 0x01010202);
			mmio_wr32(DDRC_BASE + 0x120, 0x00000504);

			// phyd related
			mmio_wr32(DDRC_BASE + 0x190, 0x04858302);
			// DFITMG0.dfi_t_ctrl_delay:24:5:=0x4
			// DFITMG0.dfi_rddata_use_dfi_phy_clk:23:1:=0x1
			// DFITMG0.dfi_t_rddata_en:16:7:=0xa
			// DFITMG0.dfi_wrdata_use_dfi_phy_clk:15:1:=0x1
			// DFITMG0.dfi_tphy_wrdata:8:6:=0x3
			// DFITMG0.dfi_tphy_wrlat:0:6:=0x5
			mmio_wr32(DDRC_BASE + 0x194, 0x00070202);
			// DFITMG1.dfi_t_cmd_lat:28:4:=0x0
			// DFITMG1.dfi_t_parin_lat:24:2:=0x0
			// DFITMG1.dfi_t_wrdata_delay:16:5:=0x7
			// DFITMG1.dfi_t_dram_clk_disable:8:5:=0x2
			// DFITMG1.dfi_t_dram_clk_enable:0:5:=0x2
			mmio_wr32(DDRC_BASE + 0x198, 0x07c13121);
			// DFILPCFG0.dfi_tlp_resp:24:5:=0x7
			// DFILPCFG0.dfi_lp_wakeup_dpd:20:4:=0xc
			// DFILPCFG0.dfi_lp_en_dpd:16:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_sr:12:4:=0x3
			// DFILPCFG0.dfi_lp_en_sr:8:1:=0x1
			// DFILPCFG0.dfi_lp_wakeup_pd:4:4:=0x2
			// DFILPCFG0.dfi_lp_en_pd:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x19c, 0x00000021);
			// DFILPCFG1.dfi_lp_wakeup_mpsm:4:4:=0x2
			// DFILPCFG1.dfi_lp_en_mpsm:0:1:=0x1

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x1a0, 0xC0400018);
			mmio_wr32(DDRC_BASE + 0x1a4, 0x00FE00FF);
			mmio_wr32(DDRC_BASE + 0x1a8, 0x80000000);
			mmio_wr32(DDRC_BASE + 0x1b0, 0x000002C1);
			mmio_wr32(DDRC_BASE + 0x1c0, 0x00000001);
			mmio_wr32(DDRC_BASE + 0x1c4, 0x00000001);

			// address map, auto gen.
			// support from 0.5Gb to 4Gb
			// R[17:13]B[2]R[12:0]B[1:0]C[9:0]
			mmio_wr32(DDRC_BASE + 0x200, 0x00001F1F);
			mmio_wr32(DDRC_BASE + 0x204, 0x00140707);
			mmio_wr32(DDRC_BASE + 0x208, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x20c, 0x1F000000);
			mmio_wr32(DDRC_BASE + 0x210, 0x00001F1F);
			mmio_wr32(DDRC_BASE + 0x214, 0x050F0505);
			mmio_wr32(DDRC_BASE + 0x218, 0x06060605);
			mmio_wr32(DDRC_BASE + 0x21c, 0x00000606);
			mmio_wr32(DDRC_BASE + 0x220, 0x00003F3F);
			mmio_wr32(DDRC_BASE + 0x224, 0x05050505);
			mmio_wr32(DDRC_BASE + 0x228, 0x05050505);
			mmio_wr32(DDRC_BASE + 0x22c, 0x001F1F05);

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x240, 0x07010708);
			mmio_wr32(DDRC_BASE + 0x244, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x250, 0x00003f85);
			// SCHED.opt_vprw_sch:31:1:=0x0
			// SCHED.rdwr_idle_gap:24:7:=0x0
			// SCHED.go2critical_hysteresis:16:8:=0x0
			// SCHED.lpddr4_opt_act_timing:15:1:=0x0
			// SCHED.lpr_num_entries:8:7:=0x1f
			// SCHED.autopre_rmw:7:1:=0x1
			// SCHED.dis_opt_ntt_by_pre:6:1:=0x0
			// SCHED.dis_opt_ntt_by_act:5:1:=0x0
			// SCHED.opt_wrcam_fill_level:4:1:=0x0
			// SCHED.rdwr_switch_policy_sel:3:1:=0x0
			// SCHED.pageclose:2:1:=0x1
			// SCHED.prefer_write:1:1:=0x0
			// SCHED.dis_opt_wrecc_collision_flush:0:1:=0x1

			mmio_wr32(DDRC_BASE + 0x254, 0x00000000);
			// SCHED1.page_hit_limit_rd:28:3:=0x0
			// SCHED1.page_hit_limit_wr:24:3:=0x0
			// SCHED1.visible_window_limit_rd:20:3:=0x0
			// SCHED1.visible_window_limit_wr:16:3:=0x0
			// SCHED1.delay_switch_write:12:4:=0x0
			// SCHED1.pageclose_timer:0:8:=0x0

			// auto gen.
			mmio_wr32(DDRC_BASE + 0x25c, 0x10000088);
			// PERFHPR1.hpr_xact_run_length:24:8:=0x20
			// PERFHPR1.hpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x264, 0x10000088);
			// PERFLPR1.lpr_xact_run_length:24:8:=0x20
			// PERFLPR1.lpr_max_starve:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x26c, 0x10000088);
			// PERFWR1.w_xact_run_length:24:8:=0x20
			// PERFWR1.w_max_starve:0:16:=0x1a8

			mmio_wr32(DDRC_BASE + 0x300, 0x00000000);
			// DBG0.dis_max_rank_wr_opt:7:1:=0x0
			// DBG0.dis_max_rank_rd_opt:6:1:=0x0
			// DBG0.dis_collision_page_opt:4:1:=0x0
			// DBG0.dis_act_bypass:2:1:=0x0
			// DBG0.dis_rd_bypass:1:1:=0x0
			// DBG0.dis_wc:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x304, 0x00000000);
			// DBG1.dis_hif:1:1:=0x0
			// DBG1.dis_dq:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x30c, 0x00000000);
			mmio_wr32(DDRC_BASE + 0x320, 0x00000001);
			// SWCTL.sw_done:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x36c, 0x00000000);
			// POISONCFG.rd_poison_intr_clr:24:1:=0x0
			// POISONCFG.rd_poison_intr_en:20:1:=0x0
			// POISONCFG.rd_poison_slverr_en:16:1:=0x0
			// POISONCFG.wr_poison_intr_clr:8:1:=0x0
			// POISONCFG.wr_poison_intr_en:4:1:=0x0
			// POISONCFG.wr_poison_slverr_en:0:1:=0x0
			mmio_wr32(DDRC_BASE + 0x400, 0x00000011);
			// PCCFG.dch_density_ratio:12:2:=0x0
			// PCCFG.bl_exp_mode:8:1:=0x0
			// PCCFG.pagematch_limit:4:1:=0x1
			// PCCFG.go2critical_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x404, 0x00006000);
			// PCFGR_0.rdwr_ordered_en:16:1:=0x0
			// PCFGR_0.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_0.rd_port_urgent_en:13:1:=0x1
			// PCFGR_0.rd_port_aging_en:12:1:=0x0
			// PCFGR_0.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_0.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x408, 0x00006000);
			// PCFGW_0.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_0.wr_port_urgent_en:13:1:=0x1
			// PCFGW_0.wr_port_aging_en:12:1:=0x0
			// PCFGW_0.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x490, 0x00000001);
			// PCTRL_0.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x494, 0x00000007);
			// PCFGQOS0_0.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_0.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_0.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_0.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_0.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x498, 0x0000006a);
			// PCFGQOS1_0.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_0.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x49c, 0x00000e07);
			// PCFGWQOS0_0.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_0.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_0.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_0.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_0.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x4a0, 0x01a801a8);
			// PCFGWQOS1_0.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_0.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x4b4, 0x00006000);
			// PCFGR_1.rdwr_ordered_en:16:1:=0x0
			// PCFGR_1.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_1.rd_port_urgent_en:13:1:=0x1
			// PCFGR_1.rd_port_aging_en:12:1:=0x0
			// PCFGR_1.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_1.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x4b8, 0x00006000);
			// PCFGW_1.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_1.wr_port_urgent_en:13:1:=0x1
			// PCFGW_1.wr_port_aging_en:12:1:=0x0
			// PCFGW_1.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x540, 0x00000001);
			// PCTRL_1.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x544, 0x00000007);
			// PCFGQOS0_1.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_1.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_1.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_1.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_1.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x548, 0x0000006a);
			// PCFGQOS1_1.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_1.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x54c, 0x00000e07);
			// PCFGWQOS0_1.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_1.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_1.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_1.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_1.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x550, 0x01a801a8);
			// PCFGWQOS1_1.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_1.wqos_map_timeout1:0:16:=0x1a8
			mmio_wr32(DDRC_BASE + 0x564, 0x00006000);
			// PCFGR_2.rdwr_ordered_en:16:1:=0x0
			// PCFGR_2.rd_port_pagematch_en:14:1:=0x1
			// PCFGR_2.rd_port_urgent_en:13:1:=0x1
			// PCFGR_2.rd_port_aging_en:12:1:=0x0
			// PCFGR_2.read_reorder_bypass_en:11:1:=0x0
			// PCFGR_2.rd_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x568, 0x00006000);
			// PCFGW_2.wr_port_pagematch_en:14:1:=0x1
			// PCFGW_2.wr_port_urgent_en:13:1:=0x1
			// PCFGW_2.wr_port_aging_en:12:1:=0x0
			// PCFGW_2.wr_port_priority:0:10:=0x0
			mmio_wr32(DDRC_BASE + 0x5f0, 0x00000001);
			// PCTRL_2.port_en:0:1:=0x1
			mmio_wr32(DDRC_BASE + 0x5f4, 0x00000007);
			// PCFGQOS0_2.rqos_map_region2:24:8:=0x0
			// PCFGQOS0_2.rqos_map_region1:20:4:=0x0
			// PCFGQOS0_2.rqos_map_region0:16:4:=0x0
			// PCFGQOS0_2.rqos_map_level2:8:8:=0x0
			// PCFGQOS0_2.rqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x5f8, 0x0000006a);
			// PCFGQOS1_2.rqos_map_timeoutr:16:16:=0x0
			// PCFGQOS1_2.rqos_map_timeoutb:0:16:=0x6a
			mmio_wr32(DDRC_BASE + 0x5fc, 0x00000e07);
			// PCFGWQOS0_2.wqos_map_region2:24:8:=0x0
			// PCFGWQOS0_2.wqos_map_region1:20:4:=0x0
			// PCFGWQOS0_2.wqos_map_region0:16:4:=0x0
			// PCFGWQOS0_2.wqos_map_level2:8:8:=0xe
			// PCFGWQOS0_2.wqos_map_level1:0:8:=0x7
			mmio_wr32(DDRC_BASE + 0x600, 0x01a801a8);
			// PCFGWQOS1_2.wqos_map_timeout2:16:16:=0x1a8
			// PCFGWQOS1_2.wqos_map_timeout1:0:16:=0x1a8
		}
	}
}

void cvx16_setting_check(void)
{
	uint32_t dfi_tphy_wrlat;
	uint32_t dfi_tphy_wrddata;
	uint32_t dfi_t_rddata_en;
	uint32_t dfi_t_ctrl_delay;
	uint32_t dfi_t_wrddata_delay;
	uint32_t rddata;

	//mmio_rd32(0x3000 + PHYD_BASE,  phy_reg_version);

	rddata = mmio_rd32(DDRC_BASE + 0x190);
	dfi_tphy_wrlat =
		get_bits_from_value(rddata, 5, 0); //DFITMG0.dfi_tphy_wrlat
	dfi_tphy_wrddata =
		get_bits_from_value(rddata, 13, 8); //DFITMG0.dfi_tphy_wrddata
	dfi_t_rddata_en =
		get_bits_from_value(rddata, 22, 16); //DFITMG0.dfi_t_rddata_en
	dfi_t_ctrl_delay =
		get_bits_from_value(rddata, 29, 24); //DFITMG0.dfi_t_ctrl_delay
	rddata = mmio_rd32(DDRC_BASE + 0x194);
	dfi_t_wrddata_delay = get_bits_from_value(
		rddata, 20, 16); //DFITMG1.dfi_t_wrddata_delay

	// if (dfi_t_ctrl_delay != 0x4)
	// 	printf("ERR !!! dfi_t_ctrl_delay not 0x4");
}

void cvx16_set_dfi_init_start(void)
{
	uint32_t rddata;

	// synp setting
	// phy is ready for initial dfi_init_start request
	// set umctl2 to tigger dfi_init_start
	mmio_wr32(DDRC_BASE + 0x00000320, 0x00000000);
	rddata =
		mmio_rd32(DDRC_BASE + 0x000001b0); //dfi_init_start @ rddata[5];
	//rddata={rddata[31:6], 1, rddata[4:0]};//
	rddata = modified_bits_by_value(rddata, 1, 5, 5);
	mmio_wr32(DDRC_BASE + 0x000001b0, rddata);
	mmio_wr32(DDRC_BASE + 0x00000320, 1);
	//`uvm_info("set_dfi_init_start", " dfi_init_start finish", UVM_LOW);
	// NOTICE("set_dfi_init_start!\n");
}
#if 0
void cvx16_polling_dfi_init_start(void)
{
	uint32_t rddata;

	while(1) {
		// param_phyd_to_reg_phy_int
		rddata = mmio_rd32(0x3028 + PHYD_BASE);
		//(~param_phyd_phy_int_mask[08] & int_ps_dfi_init_start_phy_0to1)
		if (get_bits_from_value(rddata, 8, 8) == 1)
			break;
	}
}
#endif

void cvx16_wait_for_dfi_init_complete(void)
{
	uint32_t rddata;

	// synp setting
	// wait for dfi_init_complete to be 1
	while (1) {
		rddata = mmio_rd32(DDRC_BASE + 0x000001bc);
		if (get_bits_from_value(rddata, 0, 0) == 1)
			break;
	}
	// synp setting
	// deassert dfi_init_start, and enable the act on dfi_init_complete
	mmio_wr32(DDRC_BASE + 0x00000320, 0x00000000);
	rddata = mmio_rd32(DDRC_BASE + 0x000001b0);
	//rddata=modified_bits_by_value(rddata, 5, 5, 0 );//
	//rddata = {rddata[31:6], 0x5};
	rddata = modified_bits_by_value(rddata, 5, 5, 0);
	mmio_wr32(DDRC_BASE + 0x000001b0, rddata);
	mmio_wr32(DDRC_BASE + 0x00000320, 0x00000001);
	//`uvm_info("dfi_init_complete", "dfi_init_complete finish", UVM_LOW);
	// NOTICE("dfi_init_complete.\n");
}

void cvx16_polling_synp_normal_mode(void)
{
	//synp ctrl operating_mode
	uint32_t rddata;

	// NOTICE("polling_synp_normal_mode.\n");
	while (1) {
		rddata = mmio_rd32(DDRC_BASE + 0x00000004);
		if (get_bits_from_value(rddata, 2, 0) == 1)
			break;
	}
	//`uvm_info("polling_synp_normal_mode", "polling init done ", UVM_NONE);
}

void ctrl_init_high_patch(void)
{
	// enable auto PD/SR
	mmio_wr32(DDRC_BASE + 0x30, 0x00000002);

	// enable auto ctrl_upd
	mmio_wr32(DDRC_BASE + 0x1a0, 0x00400018);

	// enable clock gating
	mmio_wr32(DDR_TOP_BASE + 0x14, 0x00000000);

	// change xpi to multi DDR burst(change bit[16-19] 4 -> 8 bit[0] 1 -> 0)???
	mmio_wr32(DDRC_BASE + 0xc, 0x63786370);
	mmio_wr32(DDRC_BASE + 0x44, 0x08000000);
}

void ddr_init(uint32_t ddr_type, uint32_t ddr_freq_mode)
{
	uint32_t rddata;
	// uint8_t   dram_cap_in_mbyte;
	// uint32_t  bist_result;
	// uint64_t  err_data_even;
	// uint64_t  err_data_odd;

	//cvx16_pll_init(CV_DDR_PHYD_APB, ddr_freq_mode);
	//`uvm_info(get_name(), "pll_init_h finish", UVM_NONE);
	cvx16_ctrl_init(ddr_type, ddr_freq_mode);
	printf("ddrc init finish\n");

	//`uvm_info(get_name(), "ctrl_init_h finish", UVM_NONE);

	//`uvm_info("release reset", "Release controler reset ", UVM_NONE);
	rddata = mmio_rd32(0x0800a020);
	rddata = modified_bits_by_value(rddata, 0, 0, 0);
	mmio_wr32(0x0800a020, rddata);

	//cvx16_phy_init(PHYD_BASE, ddr_type);
	//`uvm_info(get_name(), "svtb_phy_init_h finish", UVM_NONE);

	//setting_check
	cvx16_setting_check();
	printf("cvx16_setting_check finish\n");
	//`uvm_info(get_name(), "cvx16_setting_check finish", UVM_NONE);

	//cvx16_pinmux(PHYD_BASE);

	///cvx16_en_rec_vol_mode();

	cvx16_set_dfi_init_start();
	printf("cvx16_set_dfi_init_start\n");

	//cvx16_ddr_phy_power_on_seq1(CV_DDR_PHYD_APB) ;

	//first dfi_init_start
	//`uvm_info(get_name(), "first dfi_init_start", UVM_NONE);
	//cvx16_polling_dfi_init_start(PHYD_BASE);
	//cvx16_INT_ISR_08(PHYD_BASE, CV_DDR_PHYD_APB, ddr_freq_mode);

	//`uvm_info(get_name(), "KC isr_process_h finish", UVM_NONE);

	//cvx16_ddr_phy_power_on_seq3(PHYD_BASE);
	cvx16_wait_for_dfi_init_complete();
	printf("cvx16_wait_for_dfi_init_complete\n");
	cvx16_polling_synp_normal_mode();
	printf("cvx16_polling_synp_normal_mode\n");

	///ctrl_init_h.ctrl_high_patch=1;
	///`uvm_send(ctrl_init_h);
	ctrl_init_high_patch();
	printf("ctrl_init_high_patch finish\n");
#if 0
    ctrl_init_detect_dram_size(DDRC_BASE, DDR_BIST_BASE, ddr_type, dram_cap_in_mbyte);
    
    ///ctrl_init_h.ctrl_update_by_dram_size=1;
    ///ctrl_init_h.dram_cap_in_mbyte = dram_cap_in_mbyte;
    ///`uvm_send(ctrl_init_h);
    ctrl_init_update_by_dram_size(DDRC_BASE, dram_cap_in_mbyte);

    //`uvm_info(get_name(), $sformatf("dram_cap_in_mbyte = %h ",dram_cap_in_mbyte ), UVM_NONE);;
    cvx16_dram_cap_check(dram_cap_in_mbyte);    

    ///if ($test$plusargs("DPLL_DIV2")) begin
    ///  cvx16_ddr_freq_change_htol();
    ///end

    cvx16_clk_gating_enable(CV_DDR_PHYD_APB, DDRC_BASE, PHYD_BASE);

    if ($test$plusargs("ENABLE_BIST")) begin
      cvx16_bist_wr_sram_init(1, DDR_BIST_BASE, ddr_type, ddr_freq_mode);
      cvx16_bist_start_check(DDR_BIST_BASE, bist_result, err_data_odd, err_data_even, ddr_type);
      `uvm_info(get_name(), $sformatf("bist_result = %x, err_data_odd = %lx, err_data_even = %lx ", bist_result,err_data_odd,err_data_even), UVM_NONE);;
      if (bist_result==0) begin
        `uvm_info(get_name(), "ERROR bist_fail", UVM_NONE);
      end
      
      //#20us;
      
      cvx16_bist_wr_prbs_init(DDR_BIST_BASE,1, ddr_type);
      cvx16_bist_start_check(DDR_BIST_BASE, bist_result, err_data_odd, err_data_even, ddr_type);
      `uvm_info(get_name(), $sformatf("bist_result = %x, err_data_odd = %lx, err_data_even = %lx ", bist_result,err_data_odd,err_data_even), UVM_NONE);
      if (bist_result==0) begin
        `uvm_info(get_name(), "ERROR bist_fail", UVM_NONE);
      end
    end
#endif
}

void ddr_sys_bring_up(void)
{
	// #ifdef DDR3_2133
	printf("DDR3 2133 init start\n");
	ddr_init(3, 2133);
	printf("DDR3 2133 init finish\n");
	// #endif

	// #ifdef DDR3_1866
	// 	ddr_init(3, 1866);
	// #endif

	// #ifdef DDR2_1333
	// 	ddr_init(2, 1333);
	// #endif

	// #ifdef DDR2_1066
	// 	ddr_init(2, 1066);
	// #endif
}

#pragma GCC diagnostic pop
