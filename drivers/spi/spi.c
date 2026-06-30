#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "stdlib.h"
#include "spi/spi.h"
#include "mmio.h"
#include "spi/spi-mem.h"
#include <delay_timer.h>
#include "debug.h"

int dw_spi_check_status(int raw)
{
	uint32_t irq_status;
	int ret = 0;

	if (raw)
		irq_status = dw_readl(DW_SPI_RISR);
	else
		irq_status = dw_readl(DW_SPI_ISR);

	if (irq_status & SPI_INT_RXOI) {
		ERROR("RX FIFO overflow detected\n");
		ret = -1;
	}

	if (irq_status & SPI_INT_RXUI) {
		ERROR("RX FIFO underflow detected\n");
		ret = -1;
	}

	if (irq_status & SPI_INT_TXOI) {
		ERROR("TX FIFO overflow detected\n");
		ret = -1;
	}

	/* Generically handle the erroneous situation */
	if (ret)
		spi_reset_chip();

	return ret;
}

void disable_xip_mode(uint8_t enable)
{
	mmio_write_32(0x0E000000 + 0, enable);
}

void spi_init(void)
{
	disable_xip_mode(0);
	spi_reset_chip();
}

uint8_t select_data_width(uint8_t width)
{
	enum DATA_WIDTH w = X1;

	switch (width) {
	case 1:
		w = X1;
		break;

	case 2:
		w = X2;
		break;

	case 4:
		w = X4;
		break;
	}
	return (uint8_t)w;
}

void spi_update_config(const struct spi_mem_op *op, struct spi_clk *clk)
{
	uint32_t ctrl0 = 0;
	uint32_t spi_ctrl0 = 0;
	uint32_t rx_sample = 0;
	struct spi_cfg cfg;

	memset(&cfg, 0, sizeof(struct spi_cfg));
	cfg.dfs = 8;
	if (op->data.buswidth > 1 && op->data.nbytes > 4) {
		cfg.dfs = 32;
		ctrl0 |= 1 << 25;
	} else {
		cfg.dfs = 8;
	}

	if (op->data.dir == SPI_MEM_DATA_IN) {
		cfg.tmode = SPI_TMOD_EPROMREAD;
		cfg.ndf = op->data.nbytes * 8 / cfg.dfs;
		if ((op->data.nbytes * 8 % cfg.dfs) != 0)
			cfg.ndf += 1;
	} else {
		cfg.tmode = SPI_TMOD_TO;
	}

	if (cfg.tmode == SPI_TMOD_EPROMREAD || cfg.tmode == SPI_TMOD_RO)
		dw_writel(DW_SPI_CTRLR1, cfg.ndf - 1);

	dw_spi_set_clock(clk);
	/*
	 * disable Chip Select Toggle Enable.
	 * spi0 mode
	 * Data Frame Size 8(bit 16:20)
	 * TMOD
	 * Motorola SPI Frame Format
	 */
	ctrl0 |= (((cfg.dfs - 1) & 0x1F)  << 16) | cfg.tmode << 8;

	if (op->data.nbytes) {
		ctrl0 &= ~(0x3 << 21);
		ctrl0 |= ((select_data_width(op->data.buswidth) & 0x3) << 21);
	}

	/* only have cmd */
	if (!op->addr.nbytes && !op->data.nbytes) {
		ctrl0 &= ~(0x3 << 21);
		ctrl0 |= ((select_data_width(op->cmd.buswidth) & 0x3) << 21);
	}
	dw_writel(DW_SPI_CTRLR0, ctrl0);

	/* Instruction Length: always 8bit inst */
	if (op->cmd.nbytes)
		spi_ctrl0 |= (2 << 8);

	/*  set Address Length */
	if (op->addr.nbytes)
		spi_ctrl0 |= ((op->addr.nbytes * 2) << 2);

	if (op->dummy.nbytes)
		spi_ctrl0 |= (op->dummy.nbytes * 8 / op->dummy.buswidth) << 11;

	if (op->cmd.buswidth && op->addr.buswidth) {
		/*
		 * Address and instruction transfer in Standard SPI Mode format
		 */
		if ((op->cmd.buswidth == 1) && (op->cmd.buswidth == op->addr.buswidth))
			spi_ctrl0 &= ~0x3;

		/*
		 * Address and instruction transfer format
		 * for 1-x-x, x != 1
		 */
		if (op->cmd.buswidth == 1 && op->cmd.buswidth != op->addr.buswidth)
			spi_ctrl0 |= 0x1;

		/*
		 *  QSPI mode or std mode
		 */
		if (op->cmd.buswidth != 1)
			spi_ctrl0 |= 0x2;
	}

	/* for only cmd */
	if (op->cmd.buswidth && !op->addr.buswidth)
		spi_ctrl0 |= 0x2;

	if (op->data.dtr == 1)
		spi_ctrl0 |= 1 << 16;

	dw_writel(DW_SPI_CTRLR0_EXT, spi_ctrl0);

	rx_sample = 1;
	dw_writel(DW_SPI_RX_SAMPLE_DLY, rx_sample);
}

void dw_spi_set_cs(int enable)
{
	dw_writel(DW_SPI_SER, enable);
}

void write_addr_data(const struct spi_mem_op *op)
{
	uint8_t addr[8] = {0};
	int j = 0;
	uint32_t addr_len = op->addr.nbytes;

	if (!op->addr.nbytes)
		return;

	for (j = 0; j < op->addr.nbytes; j++)
		addr[j] = SPI_GET_BYTE(op->addr.val, op->addr.nbytes - j - 1);

	switch (op->data.buswidth) {
	case 0:
	case 1:
		for (j = 0; j < addr_len; j++)
			dw_write_io_reg(REG_IO_WIDTH, DW_SPI_DR, addr[j]);
		break;

	case 2:
		dw_write_io_reg(REG_IO_WIDTH, DW_SPI_DR, (uint32_t)op->addr.val);
		break;

	case 4:
		dw_write_io_reg(REG_IO_WIDTH, DW_SPI_DR, (uint32_t)op->addr.val);
		break;

	default:
		ERROR("error data width\n");
	}
}

int dw_spi_write_then_read(const struct spi_mem_op *op)
{
	uint32_t out_len, entries, room, in_len;
	uint32_t val = 0;
	void *in_buf;
	uint8_t *out_buf;
	uint8_t data_width = 1;
	uint32_t len = 0;

	in_len = 0;
	out_len = 0;
	in_buf = NULL;
	out_buf = NULL;

	if ((op->data.nbytes > REG_IO_WIDTH) && (op->data.buswidth > 1))
		data_width = REG_IO_WIDTH;

	if (op->data.dir == SPI_MEM_DATA_OUT) {
		out_buf = op->data.buf.out;
		out_len = op->data.nbytes;
	}

	/* write cmd */
	if (op->cmd.nbytes)
		dw_write_io_reg(REG_IO_WIDTH, DW_SPI_DR, op->cmd.opcode);

	/* write addr */
	write_addr_data(op);
	len = MIN((MAX_FIFO_LEN - dw_readl(DW_SPI_TXFLR)), out_len / data_width);
	while (len) {
		if (data_width == 1) {
			dw_write_io_reg(REG_IO_WIDTH, DW_SPI_DR, *(uint8_t *)out_buf);
		} else {
			dw_write_io_reg(REG_IO_WIDTH, DW_SPI_DR, *(uint32_t *)out_buf);
		}
		len -= 1;
		out_buf += data_width;
		out_len -= data_width;
	}

	dw_spi_set_cs(1);
	while (out_len) {
		entries = dw_readl(DW_SPI_TXFLR);
		if (!entries) {
			ERROR("CS de-assertion on Tx\n");
			dw_spi_check_status(1);
			return -1;
		}


		room = MIN(MAX_FIFO_LEN - entries, out_len / data_width);
		for (; room;) {
			if (data_width == 1) {
				dw_write_io_reg(REG_IO_WIDTH, DW_SPI_DR, *(uint8_t *)out_buf);
			} else {
				dw_write_io_reg(REG_IO_WIDTH, DW_SPI_DR, *(uint32_t *)out_buf);
			}
			room -= 1;
			out_len -= data_width;
			out_buf += data_width;
		}
	}

	if (op->data.dir == SPI_MEM_DATA_IN) {
		in_len = op->data.nbytes;
		in_buf = op->data.buf.in;
	}

	while (in_len) {
		entries = 0;
		entries = dw_readl(DW_SPI_RXFLR);
		if (!entries) {
			val = dw_readl(DW_SPI_RISR);
			if (val & SPI_INT_RXOI) {
				ERROR("FIFO overflow on Rx\n");
				return -1;
			}
			continue;
		}

		for (; entries;) {
			if (data_width == REG_IO_WIDTH) {
				if (in_len >= 4) {
					*(uint32_t *)in_buf = dw_read_io_reg(REG_IO_WIDTH, DW_SPI_DR);
					in_buf += data_width;
					in_len -=  data_width;
					entries -= 1;
				} else {
					val = dw_read_io_reg(REG_IO_WIDTH, DW_SPI_DR);
					memcpy(in_buf, &val, in_len);
					entries -= 1;
					in_len -=  in_len;
				}
			} else {
				*(uint8_t *)in_buf++ = dw_read_io_reg(REG_IO_WIDTH, DW_SPI_DR);
				in_len -= data_width;
				entries -= 1;
			}
		}
	}
	return 0;
}

static inline int dw_spi_ctlr_busy(void)
{
	return dw_readl(DW_SPI_SR) & SR_BUSY;
}

int dw_spi_wait_mem_op_done(uint32_t clk, uint32_t data_len)
{
	int retry = SPI_WAIT_RETRIES;
	unsigned long ns, delay;
	uint32_t nents;

	nents = dw_readl(DW_SPI_TXFLR);
	ns = NSEC_PER_SEC / clk * nents;
	ns *= data_len * BITS_PER_BYTE;
	/* unit: us */
	if (ns <= NSEC_PER_USEC)
		delay = 1;
	else
		delay = DIV_ROUND_UP(ns, NSEC_PER_USEC);

	while (dw_spi_ctlr_busy() && retry--)
		udelay(delay);

	if (retry < 0)
		return -1;

	return 0;
}

static void dw_spi_stop_mem_op(void)
{
	spi_enable_chip(0);
	dw_spi_set_cs(0);
	udelay(50);
	spi_enable_chip(1);
}

int spi_exec_mem_op(struct spi_clk *clk, const struct spi_mem_op *op)
{
	unsigned int out_len;
	int ret = 0;

	if (clk->out > clk->in)
		return -1;

	spi_enable_chip(0);
	spi_update_config(op, clk);
	spi_mask_intr(0xff);
	spi_enable_chip(1);

	if (op->data.dir != SPI_MEM_DATA_IN)
		out_len = op->data.nbytes + op->cmd.nbytes + op->addr.nbytes;
	else
		out_len = op->cmd.nbytes + op->addr.nbytes;

	ret = dw_spi_write_then_read(op);
	if (!ret) {
		ret = dw_spi_wait_mem_op_done(clk->out, out_len);
		if (!ret)
			ret = dw_spi_check_status(1);
	}
	dw_spi_stop_mem_op();
	return ret;
}




