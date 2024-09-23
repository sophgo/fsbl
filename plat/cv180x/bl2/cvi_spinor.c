/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cv_spinor.c
 *
 * Description: Cvitek SPI NOR flash driver
 */

#include <cpu.h>
#include <assert.h>
#include <delay_timer.h>
#include <errno.h>
#include <mmio.h>
#include <platform.h>
#include <string.h>
#include <platform.h>
#include <cvi_spinor.h>
#include <security/security.h>




static inline enum boot_freq get_boot_freq(void)
{
	/*
	 * Bonding0[10] Bonding0[11]
	 * PV_CPU_boot_freq	"00 = all xtal freq
	 * 01 = CPU 450M, Apfab 300M, TPU 350M
	 * 10 = CPU 900M, Apfab 600M, TPU 700M
	 * 11 = all xtal freq"
	 */
	
	NOTICE("TCS\n");
	mmio_write_32(0x03002040, 0x00010009); // set cpu clock to PLL(MPLL) div by 1 = 1GHz
	mmio_write_32(0x03002048, 0x00020009); // set ap_fab clock to PLL(MIPIPLL) div 2 = 600MHz
	mmio_write_32(0x03002030, 0x00000000); // switch clock to PLL from xtal
	mmio_write_32(0x03002034, 0x00000000); // switch clock to PLL from xtal
	// mmio_write_32(0x04060024, 0x40001000); // spi nand parameter, response data latched at SCK posedge
	// mmio_write_32(0x04060004, 0x00110900); // spi nand clock setting => 187.5/4 = 46.875MHz
	// mmio_write_32(0x1000000C, 0x00000000); // spinor: disable DMMR
	// mmio_write_32(0x10000000, 0x0008C003); // spinor: set clock => 300/2/(3+1) = 37.5MHz
	// mmio_write_32(0x1000000C, 0x00000001); // spinor: enable DMMR
	NOTICE("TCE\n");

	uint32_t boot_freq = (mmio_read_32(EFUSE_BONDING0) >> 10) & 0x3;

	WARN("boot_freq :%d\n", boot_freq);
	if (boot_freq == 3)
		return BOOT_FREQ_XTAL;

	return boot_freq;
}
//
// Function
//     spi_non_data_tran
// Parameters
//     cmd_buf: pointer to command buffer.
// Description
//     Issue transation w/o address and data.
// Return
//     None.
//
static void spi_non_data_tran(uint8_t *cmd_buf)
{
	uint32_t tran_csr = 0;
	uint32_t *p_data = (uint32_t *)cmd_buf;

	// disable DMMR
	mmio_write_32(REG_SPI_DMMR, BIT_DMMR_DIS);

	// flush FIFO before filling fifo
	mmio_write_8(REG_SPI_FIFO_PT, 0);

	// write cmd to tx fifo in double-words
	mmio_write_32(REG_SPI_FIFO_PORT, p_data[0]);

	// clear all interrupts
	mmio_write_8(REG_SPI_INT_STS, 0);

	// init tran_csr
	tran_csr = BIT_TRAN_CSR_BUSWIDTH_1_BIT |
		BIT_TRAN_CSR_WITH_CMD |
		BIT_TRAN_CSR_FIFO_TRGLVL_1_BYTE |
		BIT_TRAN_CSR_GO_BUSY;
	mmio_write_32(REG_SPI_TRAN_CSR, tran_csr);

	// wait rx tran done
	while ((mmio_read_32(REG_SPI_INT_STS) & BIT_INT_TRAN_DONE) == 0)
	;

	// flush FIFO after tran
	mmio_write_8(REG_SPI_FIFO_PT, 0);
}

//
// Function
//     spi_in_out_tran
// Parameters
//     dst_buf: pointer to dest buffer.
//     src_buf: pointer to src buffer.
//     get_bytes: number of received bytes.
// Description
//     Issue transation w/o address but with read data.
// Return
//     None.
//
static void spi_in_out_tran(uint8_t *dst_buf, uint8_t *src_buf, uint32_t get_bytes)
{
	uint32_t *p_data = (uint32_t *)src_buf;
	uint32_t i, tran_csr = 0;

	// disable DMMR
	mmio_write_32(REG_SPI_DMMR, BIT_DMMR_DIS);

	// flush FIFO before filling fifo
	mmio_write_32(REG_SPI_FIFO_PT, 0);

	// write cmd to tx fifo in double-words
	mmio_write_32(REG_SPI_FIFO_PORT, p_data[0]);

	// clear all interrupts
	mmio_write_32(REG_SPI_INT_STS, 0);

	// set number of bytes to receive
	mmio_write_32(REG_SPI_TRAN_NUM, get_bytes);

	// init tran_csr
	tran_csr = BIT_TRAN_CSR_BUSWIDTH_1_BIT |
		BIT_TRAN_CSR_WITH_CMD |
		BIT_TRAN_CSR_FIFO_TRGLVL_1_BYTE |
		BIT_TRAN_CSR_TRAN_MODE_RX |
		BIT_TRAN_CSR_GO_BUSY;
	mmio_write_32(REG_SPI_TRAN_CSR, tran_csr);

	// wait rx tran done
	while ((mmio_read_32(REG_SPI_INT_STS) & BIT_INT_TRAN_DONE) == 0)
	;

	p_data = (uint32_t *)dst_buf;
	for (i = 0; i < ((get_bytes - 1) / 4 + 1); i++)
		p_data[i] = mmio_read_32(REG_SPI_FIFO_PORT);

	// flush FIFO after tran
	mmio_write_8(REG_SPI_FIFO_PT, 0);
}

//
// Function
//     spi_reset_device
// Parameters
//     None.
// Description
//     Reset SPI-NOR device.
// Return
//     None.
//
static void spi_reset_device(void)
{
	uint8_t cmd_buf[4];

	memset(cmd_buf, 0, sizeof(cmd_buf));

	// send reset (66h/99h) cmd in SPI mode
	cmd_buf[0] = CMD_RSTEN;
	spi_non_data_tran(cmd_buf);
	cmd_buf[0] = CMD_RST;
	spi_non_data_tran(cmd_buf);

	// delay 100us for Trst, no cmd will be accepted during the reset period
	udelay(100);
}

//
// Function
//     spi_read_status
// Parameters
//     None.
// Description
//     Read SPI-NOR device status.
// Return
//     status reg return from device.
//
static uint8_t spi_read_status(void)
{
	uint8_t cmd_buf[4];
	uint8_t rx_buf[4];

	memset(cmd_buf, 0, sizeof(cmd_buf));
	memset(rx_buf, 0, sizeof(rx_buf));

	cmd_buf[0] = CMD_RDSR;
	spi_in_out_tran(rx_buf, cmd_buf, 1);

	return rx_buf[0];
}

//
// Function
//     cv_spinor_init
// Parameters
//     clk: SPI clock frequency.
// Description
//     Init SPINOR flash controller settings.
// Return
//     None.
//
void cv_spinor_init(uint32_t clk)
{
	uint32_t delay_cnt = 1000;	// 1000ms

	// disable DMMR
	mmio_write_32(REG_SPI_DMMR, BIT_DMMR_DIS);

	// wait until SPI NOR flash is not WIP, max waiting time is 1s
	while (delay_cnt--) {
		if ((spi_read_status() & SPI_STATUS_WIP) == 0)	// device is idle
			break;
		mdelay(1);
	}

	// reset SPI NOR flash
	spi_reset_device();

	// config SPI clock
#ifdef test
	if (get_boot_freq() == BOOT_FREQ_XTAL) {
		mmio_clrsetbits_32(REG_SPI_CTRL, 0x7FF, 1);	// sck = 25MHz/(2*(1+1)) = 6.25MHz
		// set positive edge sampling
		mmio_write_32(REG_SPI_DLY_CTRL, BIT_DLY_CTRL_CET);
	} else {
		mmio_clrsetbits_32(REG_SPI_CTRL, 0x7FF, clk);
		if (clk == SPI_CLK_75M || clk == SPI_CLK_50M || clk == SPI_CLK_37M) {
			// set negative edge sampling
			mmio_write_32(REG_SPI_DLY_CTRL, BIT_DLY_CTRL_CET|BIT_DLY_CTRL_NEG_SAMPLE);
		} else {
			// set positive edge sampling
			mmio_write_32(REG_SPI_DLY_CTRL, BIT_DLY_CTRL_CET);
		}
	}
#endif
	mmio_write_32(0x03002040, 0x00010009); // set cpu clock to PLL(MPLL) div by 1 = 1GHz
	mmio_write_32(0x03002048, 0x00020009); // set ap_fab clock to PLL(MIPIPLL) div 2 = 600MHz
	mmio_write_32(0x03002030, 0x00000000); // switch clock to PLL from xtal
	mmio_write_32(0x03002034, 0x00000000); // switch clock to PLL from xtal
	mmio_write_32(0x04060024, 0x40001000); // spi nand parameter, response data latched at SCK posedge
	mmio_write_32(0x04060004, 0x00110900); // spi nand clock setting => 187.5/4 = 46.875MHz

	if(clk == SPI_CLK_75M){
		mmio_write_32(0x1000000C, 0x00000000); // spinor: disable DMMR
		mmio_write_32(0x10000000, 0x0008C001); // spinor: set clock => 300/2/(1+1) = 75MHz
		mmio_write_32(0x1000000C, 0x00000001); // spinor: enable DMMR
		mmio_write_32(REG_SPI_DLY_CTRL, BIT_DLY_CTRL_CET|BIT_DLY_CTRL_NEG_SAMPLE);
		// disable CE manaual mode
		mmio_write_32(REG_SPI_CE_CTRL, 0);
		// config dmmr with default SPI mode
		mmio_write_32(REG_SPI_TRAN_CSR, CSR_DMMR_READ_EBh);
	}else if(clk == SPI_CLK_15M){
		mmio_write_32(0x1000000C, 0x00000000); // spinor: disable DMMR
		mmio_write_32(0x10000000, 0x0008C009); // spinor: set clock => 300/2/(1+1) = 75MHz
		mmio_write_32(0x1000000C, 0x00000001); // spinor: enable DMMR
		mmio_write_32(REG_SPI_DLY_CTRL, BIT_DLY_CTRL_CET);
		mmio_write_32(REG_SPI_CE_CTRL, 0);
		// config dmmr with default SPI mode
		mmio_write_32(REG_SPI_TRAN_CSR, CSR_DMMR_READ_03h);
	}
	// enable DMMR
	mmio_write_32(REG_SPI_DMMR, BIT_DMMR_EN);
}

//
// Function
//     cv_spinor_config_param
// Parameters
//     info: reg settings from flash.
// Description
//     Init SPINOR flash controller settings.
// Return
//     0: success.
//     -1: fail.
//
int cv_spinor_config_param(struct spinor_info_t info)
{
	uint32_t ctrl = info.ctrl;
	uint32_t dly_ctrl = info.dly_ctrl;
	uint32_t tran_csr = info.tran_csr;
	uint32_t opt = info.opt;
	int ret = 0;

	if ((ctrl == 0xFFFFFFFF) || (dly_ctrl == 0xFFFFFFFF) ||
		(tran_csr == 0xFFFFFFFF) || (opt == 0xFFFFFFFF)) {
		ret = -EINVAL;	/* Invalid argument */
	} else {
		mmio_write_32(REG_SPI_CTRL, ctrl);
		mmio_write_32(REG_SPI_DLY_CTRL, dly_ctrl);
		mmio_write_32(REG_SPI_TRAN_CSR, tran_csr);
		mmio_write_32(REG_SPI_OPT, opt);

		ATF_STATE = ATF_STATE_BL1_SPINOR_INFO;
	}
	return ret;
}
