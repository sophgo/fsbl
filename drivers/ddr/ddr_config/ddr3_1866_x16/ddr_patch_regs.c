#include <stddef.h>
#include <utils_def.h>
#include <mmio.h>
#include <reg_soc.h>
#include <ddr_sys.h>
#include <ddr_pkg_info.h>
#include "regconfig.h"

//regpatch_ddr3_x16_bga.c for 1866

/* SIP (KGD) tuning: the upstream 1866-x16 defaults. Board-tuned external-DDR3
 * settings live in the ext table below, so a SIP board and an external-DDR3
 * board sharing one config (1901C QFN88 SIP vs 1904G BGA9 external, both
 * ddr3_1866_x16) do not have to share one tuning. The active table is picked
 * at runtime by ddr_patch_regs_init() from get_ddr_vendor(). */
static struct regpatch _ddr_patch_regs_sip[] = {
	// // tune damp //////
	// {0x08000150, 0xFFFFFFFF, 0x00000005},

	// // CSB & CA driving
	// {0x0800097c, 0xFFFFFFFF, 0x08080404},

	// // CLK driving
	// {0x08000980, 0xFFFFFFFF, 0x08080808},

	// // DQ driving // BYTE0
	// {0x08000a38, 0xFFFFFFFF, 0x00000606},
	// // DQS driving // BYTE0
	// {0x08000a3c, 0xFFFFFFFF, 0x06060606},
	// // DQ driving // BYTE1
	// {0x08000a78, 0xFFFFFFFF, 0x00000606},
	// // DQS driving // BYTE1
	// {0x08000a7c, 0xFFFFFFFF, 0x06060606},

	// //trigger level //////
	// // BYTE0
	// {0x08000b24, 0xFFFFFFFF, 0x00100010},
	// // BYTE1
	// {0x08000b54, 0xFFFFFFFF, 0x00100010},

	// //APHY TX VREFDQ rangex2 [1]
	// //VREF DQ   //
	// {0x08000410, 0xFFFFFFFF, 0x00120002},
	// //APHY TX VREFCA rangex2 [1]
	// //VREF CA  //
	// {0x08000414, 0xFFFFFFFF, 0x00100002},

	// // tx dline code
	// //  BYTE0 DQ
	// {0x08000a00, 0xFFFFFFFF, 0x06430643},
	// {0x08000a04, 0xFFFFFFFF, 0x06430643},
	// {0x08000a08, 0xFFFFFFFF, 0x06430643},
	// {0x08000a0c, 0xFFFFFFFF, 0x06430643},
	// {0x08000a10, 0xFFFFFFFF, 0x00000643},
	// {0x08000a14, 0xFFFFFFFF, 0x0a7e007e},
	// //  BYTE1 DQ
	// {0x08000a40, 0xFFFFFFFF, 0x06480648},
	// {0x08000a44, 0xFFFFFFFF, 0x06480648},
	// {0x08000a48, 0xFFFFFFFF, 0x06480648},
	// {0x08000a4c, 0xFFFFFFFF, 0x06480648},
	// {0x08000a50, 0xFFFFFFFF, 0x00000648},
	// {0x08000a54, 0xFFFFFFFF, 0x0a7e007e},

	// //APHY RX TRIG rangex2[18] & disable lsmode[0]
	// //f0_param_phya_reg_rx_byte0_en_lsmode[0]
	// //f0_param_phya_reg_byte0_en_rec_vol_mode[12]
	// //f0_param_phya_reg_rx_byte0_force_en_lvstl_odt[16]
	// //f0_param_phya_reg_rx_byte0_sel_dqs_rec_vref_mode[8]
	// //param_phya_reg_rx_byte0_en_trig_lvl_rangex2[18]
	// // BYTE0 [0]
	// {0x08000500, 0xFFFFFFFF, 0x00041001},
	// //f0_param_phya_reg_rx_byte1_en_lsmode[0]
	// //f0_param_phya_reg_byte1_en_rec_vol_mode[12]
	// //f0_param_phya_reg_rx_byte0_force_en_lvstl_odt[16]
	// //f0_param_phya_reg_rx_byte0_sel_dqs_rec_vref_mode[8]
	// //param_phya_reg_rx_byte0_en_trig_lvl_rangex2[18]
	// // BYTE1 [0]
	// {0x08000540, 0xFFFFFFFF, 0x00041001},

	// ////////  FOR U02 ///////
	// /////////// U02 enable DQS voltage mode receiver
	// // f0_param_phya_reg_tx_byte0_en_tx_de_dqs[20]
	// {0x08000504, 0xFFFFFFFF, 0x00100000},
	// // f0_param_phya_reg_tx_byte1_en_tx_de_dqs[20]
	// {0x08000544, 0xFFFFFFFF, 0x00100000},
	// /////////// U02 enable MASK voltage mode receiver
	// // param_phya_reg_rx_sel_dqs_wo_pream_mode[2]
	// {0x08000138, 0xFFFFFFFF, 0x00000014},

	// BYTE0 RX DQ deskew
	{0x08000b00, 0xFFFFFFFF, 0x05030703},
	{0x08000b04, 0xFFFFFFFF, 0x03030403},
	// BYTE0  DQ8 deskew [6:0] neg DQS  [15:8]  ;  pos DQS  [23:16]
	{0x08000b08, 0xFFFFFFFF, 0x00323603},

	// BYTE1 RX DQ deskew
	{0x08000b30, 0xFFFFFFFF, 0x07030204},
	{0x08000b34, 0xFFFFFFFF, 0x03030403},
	// BYTE1  DQ8 deskew [6:0] neg DQS  [15:8]  ;  pos DQS  [23:16]
	{0x08000b38, 0xFFFFFFFF, 0x00323604},

	// //Read gate TX dline + shift
	// // BYTE0
	// {0x08000b0c, 0xFFFFFFFF, 0x00000a14},
	// // BYTE1
	// {0x08000b3c, 0xFFFFFFFF, 0x00000a14},

	// // CKE dline + shift CKE0 [6:0]+[13:8] ; CKE1 [22:16]+[29:24]
	// {0x08000930, 0xFFFFFFFF, 0x04000400},
	// // CSB dline + shift CSB0 [6:0]+[13:8] ; CSB1 [22:16]+[29:24]
	// {0x08000934, 0xFFFFFFFF, 0x04000400},
};

/* External DDR3 (1904G BGA9): the same structure as the SIP table with the
 * board-tuned values from the 1904G ext-DDR3 first-board bringup. */
static struct regpatch _ddr_patch_regs_ext[] = {
	// TX dline seeds: the 1904G ext-DDR3 1600 BIST-PASS run (log
	// 1904G-OCHP-1600-0930-1). TX DQ = wdqlvl converged (piwdqlvl
	// shift/delay readback of 0xa00+0x40*i); TX DQS = wrlvl converged
	// (0xa14/0xa54, [6:0]=dqsn [14:8]=dqsp [30:24]=raw).
	//  BYTE0 DQ
	{0x08000a00, 0xFFFFFFFF, 0x062e063e},
	{0x08000a04, 0xFFFFFFFF, 0x062f063d},
	{0x08000a08, 0xFFFFFFFF, 0x06320636},
	{0x08000a0c, 0xFFFFFFFF, 0x06310636},
	{0x08000a10, 0xFFFFFFFF, 0x00000634},
	{0x08000a14, 0xFFFFFFFF, 0x0a740074},
	//  BYTE1 DQ
	{0x08000a40, 0xFFFFFFFF, 0x0635062c},
	{0x08000a44, 0xFFFFFFFF, 0x06390629},
	{0x08000a48, 0xFFFFFFFF, 0x06370627},
	{0x08000a4c, 0xFFFFFFFF, 0x06340625},
	{0x08000a50, 0xFFFFFFFF, 0x0000062f},
	{0x08000a54, 0xFFFFFFFF, 0x0a6c006c},

	// BYTE0: f0 value 0x00040001 | en_rec_vol_mode[12] (needed by pirdlvl
	// vref training); bit0 en_lsmode / bit18 trig_lvl_rangex2 kept as-is.
	{0x08000500, 0xFFFFFFFF, 0x00041001},
	// BYTE1: same as byte0 [0]
	{0x08000540, 0xFFFFFFFF, 0x00041001},

	// RX ODT 120ohm. This is a chip-level value, not an external-DDR3
	// override: ddr_init.h now defaults DDR_PHY_REG_7_F0_DATA to 0x4242
	// (120ohm) for every config, so this entry is redundant with the
	// default and only kept so the value is visible in the ext table.
	{0x0800041c, 0xFFFFFFFF, 0x00004242},

	// RX DQ deskew seeds: the 1904G ext-DDR3 1600 BIST-PASS run's
	// after-rdlvl converged values (log 1904G-OCHP-1600-0930-1).
	// BYTE0 RX DQ deskew dq0[6:0] dq1[14:8] dq2[22:16] dq3[30:24]
	{0x08000b00, 0xFFFFFFFF, 0x050e040e},
	// BYTE0 RX DQ deskew dq4[6:0] dq5[14:8] dq6[22:16] dq7[30:24]
	{0x08000b04, 0xFFFFFFFF, 0x000f0516},
	// BYTE0  DQ8 deskew [6:0] neg DQS  [15:8]  ;  pos DQS  [23:16]
	{0x08000b08, 0xFFFFFFFF, 0x005d620e},

	// BYTE1 RX DQ deskew dq0[6:0] dq1[14:8] dq2[22:16] dq3[30:24]
	{0x08000b30, 0xFFFFFFFF, 0x0e00100a},
	// BYTE1 RX DQ deskew dq4[6:0] dq5[14:8] dq6[22:16] dq7[30:24]
	{0x08000b34, 0xFFFFFFFF, 0x0f011100},
	// BYTE1  DQ8 deskew [6:0] neg DQS  [15:8]  ;  pos DQS  [23:16]
	{0x08000b38, 0xFFFFFFFF, 0x005d670a},
};

struct regpatch *ddr_patch_regs;
uint32_t ddr_patch_regs_count;

void ddr_patch_regs_init(void)
{
	if (get_ddr_vendor() == DDR_VENDOR_EXT_DDR3 ||
	    get_ddr_vendor() == DDR_VENDOR_EXT_DDR3_1905J) {
		ddr_patch_regs = _ddr_patch_regs_ext;
		ddr_patch_regs_count = ARRAY_SIZE(_ddr_patch_regs_ext);
	} else {
		ddr_patch_regs = _ddr_patch_regs_sip;
		ddr_patch_regs_count = ARRAY_SIZE(_ddr_patch_regs_sip);
	}
}
