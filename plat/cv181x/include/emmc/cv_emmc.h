/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __CV_EMMC_H__
#define __CV_EMMC_H__

#include <emmc.h>

#define SDHCI_DMA_ADDRESS               0x00
#define SDHCI_BLOCK_SIZE                0x04
#define SDHCI_MAKE_BLKSZ(dma, blksz)    ((((dma) & 0x7) << 12) | ((blksz) & 0xFFF))
#define SDHCI_BLOCK_COUNT               0x06
#define SDHCI_ARGUMENT                  0x08
#define SDHCI_TRANSFER_MODE             0x0C
#define SDHCI_TRNS_DMA                  BIT(0)
#define SDHCI_TRNS_BLK_CNT_EN           BIT(1)
#define SDHCI_TRNS_ACMD12               BIT(2)
#define SDHCI_TRNS_READ                 BIT(4)
#define SDHCI_TRNS_MULTI                BIT(5)
#define SDHCI_TRNS_RESP_INT             BIT(8)
#define SDHCI_COMMAND                   0x0E
#define SDHCI_CMD_RESP_MASK             0x03
#define SDHCI_CMD_CRC                   0x08
#define SDHCI_CMD_INDEX                 0x10
#define SDHCI_CMD_DATA                  0x20
#define SDHCI_CMD_ABORTCMD              0xC0
#define SDHCI_CMD_RESP_NONE             0x00
#define SDHCI_CMD_RESP_LONG             0x01
#define SDHCI_CMD_RESP_SHORT            0x02
#define SDHCI_CMD_RESP_SHORT_BUSY       0x03
#define SDHCI_MAKE_CMD(c, f)            ((((c) & 0xff) << 8) | ((f) & 0xff))
#define SDHCI_RESPONSE_01               0x10
#define SDHCI_RESPONSE_23               0x14
#define SDHCI_RESPONSE_45               0x18
#define SDHCI_RESPONSE_67               0x1C
#define SDHCI_PSTATE                    0x24
#define SDHCI_CMD_INHIBIT               BIT(0)
#define SDHCI_CMD_INHIBIT_DAT           BIT(1)
#define SDHCI_HOST_CONTROL              0x28
#define SDHCI_DAT_XFER_WIDTH            BIT(1)
#define SDHCI_EXT_DAT_XFER              BIT(5)
#define SDHCI_CTRL_DMA_MASK             0x18
#define SDHCI_CTRL_SDMA                 0x00
#define SDHCI_PWR_CONTROL               0x29
#define SDHCI_BUS_VOL_VDD1_1_8V         0xC
#define SDHCI_BUS_VOL_VDD1_3_0V         0xE
#define SDHCI_BUF_DATA_R                0x20
#define SDHCI_BLOCK_GAP_CONTROL         0x2A
#define SDHCI_CLK_CTRL                  0x2C
#define SDHCI_TOUT_CTRL                 0x2E
#define SDHCI_SOFTWARE_RESET            0x2F
#define SDHCI_RESET_CMD                 0x02
#define SDHCI_RESET_DATA                0x04
#define SDHCI_INT_STATUS                0x30
#define SDHCI_ERR_INT_STATUS            0x32
#define SDHCI_INT_CMD_COMPLETE          BIT(0)
#define SDHCI_INT_XFER_COMPLETE         BIT(1)
#define SDHCI_INT_DMA_END               BIT(3)
#define SDHCI_INT_BUF_RD_READY          BIT(5)
#define SDHCI_INT_ERROR                 BIT(15)
#define SDHCI_INT_STATUS_EN             0x34
#define SDHCI_ERR_INT_STATUS_EN         0x36
#define SDHCI_INT_CMD_COMPLETE_EN       BIT(0)
#define SDHCI_INT_XFER_COMPLETE_EN      BIT(1)
#define SDHCI_INT_DMA_END_EN            BIT(3)
#define SDHCI_INT_ERROR_EN              BIT(15)
#define SDHCI_SIGNAL_ENABLE             0x38
#define SDHCI_HOST_CONTROL2             0x3E
#define SDHCI_HOST_VER4_ENABLE          BIT(12)
#define SDHCI_CAPABILITIES1             0x40
#define SDHCI_CAPABILITIES2             0x44
#define SDHCI_ADMA_SA_LOW               0x58
#define SDHCI_ADMA_SA_HIGH              0x5C

#define P_VENDOR_SPECIFIC_AREA          0xE8
#define P_VENDOR2_SPECIFIC_AREA         0xEA
#define VENDOR_EMMC_CTRL                0x0
#define SDHCI_RESET_N                   BIT(8)

#define SDHCI_PHY_R_OFFSET         0x300
#define SDHCI_P_PHY_CNFG             (SDHCI_PHY_R_OFFSET + 0x00)
#define SDHCI_P_CMDPAD_CNFG      (SDHCI_PHY_R_OFFSET + 0x04)
#define SDHCI_P_DATPAD_CNFG      (SDHCI_PHY_R_OFFSET + 0x06)
#define SDHCI_P_CLKPAD_CNFG       (SDHCI_PHY_R_OFFSET + 0x08)
#define SDHCI_P_STBPAD_CNFG       (SDHCI_PHY_R_OFFSET + 0x0A)
#define SDHCI_P_RSTNPAD_CNFG     (SDHCI_PHY_R_OFFSET + 0x0C)
#define SDHCI_P_PADTEST_CNFG      (SDHCI_PHY_R_OFFSET + 0x0E)
#define SDHCI_P_PADTEST_OUT        (SDHCI_PHY_R_OFFSET + 0x10)
#define SDHCI_P_PADTEST_IN          (SDHCI_PHY_R_OFFSET + 0x12)
#define SDHCI_P_COMMDL_CNFG       (SDHCI_PHY_R_OFFSET + 0x1C)
#define SDHCI_P_SDCLKDL_CNFG      (SDHCI_PHY_R_OFFSET + 0x1D)
#define SDHCI_P_SDCLKDL_DC          (SDHCI_PHY_R_OFFSET + 0x1E)
#define SDHCI_P_SMPLDL_CNFG       (SDHCI_PHY_R_OFFSET + 0x20)
#define SDHCI_P_ATDL_CNFG            (SDHCI_PHY_R_OFFSET + 0x21)
#define SDHCI_P_DLL_CTRL              (SDHCI_PHY_R_OFFSET + 0x24)
#define SDHCI_P_DLL_CNFG1            (SDHCI_PHY_R_OFFSET + 0x25)
#define SDHCI_P_DLL_CNFG2            (SDHCI_PHY_R_OFFSET + 0x26)
#define SDHCI_P_DLLDL_CNFG         (SDHCI_PHY_R_OFFSET + 0x28)
#define SDHCI_P_DLL_OFFST            (SDHCI_PHY_R_OFFSET + 0x29)
#define SDHCI_P_DLLMST_TSTDC     (SDHCI_PHY_R_OFFSET + 0x2A)
#define SDHCI_P_DLLLBT_CNFG        (SDHCI_PHY_R_OFFSET + 0x2C)
#define SDHCI_P_DLL_STATUS          (SDHCI_PHY_R_OFFSET + 0x2E)
#define SDHCI_P_DLLDBG_MLKDC     (SDHCI_PHY_R_OFFSET + 0x30)
#define SDHCI_P_DLLDBG_SLKDC      (SDHCI_PHY_R_OFFSET + 0x32)

#define SDHCI_RX_DELAY_LINE		(0x200 + 0x40) // P_VERDOR_SPECIFIC_AREA + 0x40(0x240) PHY_TX_RX_DLY
#define REG_RX_SRC_SEL_MASK     (0xFCFFFFFF) //	0x240 PHY_RX_SRC [25:24]
#define REG_RX_SRC_SEL_CLR_MASK     (0x3000000) //	0x240 PHY_RX_SRC [25:24]
#define REG_RX_SRC_SEL_CLK_TX			(0) // clk_tx
#define REG_RX_SRC_SEL_CLK_TX_INV		(1) // clk_tx_inv
#define REG_RX_SRC_SEL_RES			(2) // resvd
#define REG_RX_SRC_SEL_PAD_CLK			(1) // pad_clk
#define REG_RX_SRC_SEL_SHIFT		(24) // 0x240 PHY_RX_SRC [25:24]

#define SDHCI_TX_DELAY_LINE		(0x200 + 0x40) // P_VERDOR_SPECIFIC_AREA + 0x40(0x240) PHY_TX_RX_DLY
#define REG_TX_SRC_SEL_MASK		(0xFFFFFCFF) // 0x240  PHY_TX_SRC[9:8]
#define REG_TX_SRC_SEL_CLR_MASK		(0x300) // 0x240  PHY_TX_SRC[9:8]
#define REG_TX_SRC_SEL_CLK_TX			(0) // clk_tx
#define REG_TX_SRC_SEL_CLK_TX_INV		(1) // clk_tx_inv
#define REG_TX_SRC_SEL_RES			(2) // resvd
#define REG_TX_SRC_SEL_PAD_CLK			(1) // pad_clk
#define REG_TX_SRC_SEL_SHIFT		(8) // 0x240  PHY_TX_SRC[9:8]

#define SDHCI_PHY_CONFIG		(0x200 + 0x4C) // P_VERDOR_SPECIFIC_AREA + 0x4c(0x24c) PHY_TX_BPS
#define REG_TX_BPS_SEL_MASK		(0xFFFFFFFE) // 0x24c  PHY_TX_BPS[0:0]
#define REG_TX_BPS_SEL_CLR_MASK		(0x1) // 0x24c  PHY_TX_BPS[0:0]
#define REG_TX_BPS_SEL_SHIFT		(0) // 0x24c  PHY_TX_BPS[0:0]
#define REG_TX_BPS_SEL_BYPASS		(1) // 0x24c PHY_TX_BPS inv

#define PHY_CNFG_PHY_RSTN                             0
#define PHY_CNFG_PHY_PWRGOOD                    1
#define PHY_CNFG_PAD_SP                                 16
#define PHY_CNFG_PAD_SP_MSK                        0xf
#define PHY_CNFG_PAD_SN                                 20
#define PHY_CNFG_PAD_SN_MSK                        0xf

#define PAD_CNFG_RXSEL                                   0
#define PAD_CNFG_RXSEL_MSK                          0x7
#define PAD_CNFG_WEAKPULL_EN                     3
#define PAD_CNFG_WEAKPULL_EN_MSK            0x3
#define PAD_CNFG_TXSLEW_CTRL_P                  5
#define PAD_CNFG_TXSLEW_CTRL_P_MSK         0xf
#define PAD_CNFG_TXSLEW_CTRL_N                 9
#define PAD_CNFG_TXSLEW_CTRL_N_MSK        0xf

#define COMMDL_CNFG_DLSTEP_SEL                 0
#define COMMDL_CNFG_DLOUT_EN                    1

#define SDCLKDL_CNFG_EXTDLY_EN                  0
#define SDCLKDL_CNFG_BYPASS_EN                  1
#define SDCLKDL_CNFG_INPSEL_CNFG               2
#define SDCLKDL_CNFG_INPSEL_CNFG_MSK      0x3
#define SDCLKDL_CNFG_UPDATE_DC                  4

#define SMPLDL_CNFG_EXTDLY_EN                    0
#define SMPLDL_CNFG_BYPASS_EN                    1
#define SMPLDL_CNFG_INPSEL_CNFG                 2
#define SMPLDL_CNFG_INPSEL_CNFG_MSK        0x3
#define SMPLDL_CNFG_INPSEL_OVERRIDE        4

#define ATDL_CNFG_EXTDLY_EN                       0
#define ATDL_CNFG_BYPASS_EN                       1
#define ATDL_CNFG_INPSEL_CNFG                    2
#define ATDL_CNFG_INPSEL_CNFG_MSK           0x3

typedef struct bm_emmc_params {
        uintptr_t       reg_base;
        uintptr_t       vendor_base;
        uintptr_t       desc_base;
        size_t          desc_size;
        int             clk_rate;
        int             bus_width;
        unsigned int    flags;
} bm_emmc_params_t;

void bm_emmc_phy_init(void);
void bm_emmc_init(void);
int bm_emmc_set_clk(int clk);
void bm_emmc_read_data(unsigned int *buf);
int bm_emmc_send_cmd_with_data_cmd8(emmc_cmd_t *cmd);
#endif	/* __CV_EMMC_H__ */
