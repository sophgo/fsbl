#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>
#include "spi-mem.h"
#include "mmio.h"

/* note: need to adjust */
#define SPI_BASE   0x10000000UL
#define SPI_NOR_BASE SPI_BASE
/* Register offsets */
#define DW_SPI_CTRLR0                   0x00
#define DW_SPI_CTRLR1                   0x04
#define DW_SPI_SSIENR                   0x08
#define DW_SPI_MWCR                     0x0c
#define DW_SPI_SER                      0x10
#define DW_SPI_BAUDR                    0x14
#define DW_SPI_TXFTLR                   0x18
#define DW_SPI_RXFTLR                   0x1c
#define DW_SPI_TXFLR                    0x20
#define DW_SPI_RXFLR                    0x24
#define DW_SPI_SR                       0x28
#define DW_SPI_IMR                      0x2c
#define DW_SPI_ISR                      0x30
#define DW_SPI_RISR                     0x34
#define DW_SPI_TXOICR                   0x38
#define DW_SPI_RXOICR                   0x3c
#define DW_SPI_RXUICR                   0x40
#define DW_SPI_MSTICR                   0x44
#define DW_SPI_ICR                      0x48
#define DW_SPI_DMACR                    0x4c
#define DW_SPI_DMATDLR                  0x50
#define DW_SPI_DMARDLR                  0x54
#define DW_SPI_IDR                      0x58
#define DW_SPI_VERSION                  0x5c
#define DW_SPI_DR                       0x60
#define DW_SPI_RX_SAMPLE_DLY            0xf0
#define DW_SPI_CTRLR0_EXT               0xf4


/* Bit fields in CTRLR0 */
#define SPI_DFS_OFFSET                  0
#define SPI_SECONV_OFFSET               25
#define SPI_SSTE_OFFSET                 24
#define SPI_DATA_FRF_OFFSET             21
#define SPI_DATA_SIZE_OFFSET            16

#define SPI_FRF_OFFSET                  4
#define SPI_FRF_SPI                     0x0
#define SPI_FRF_SSP                     0x1
#define SPI_FRF_MICROWIRE               0x2
#define SPI_FRF_RESV                    0x3

#define SPI_MODE_OFFSET                 6
#define SPI_SCPH_OFFSET                 6
#define SPI_SCOL_OFFSET                 7

#define SPI_TMOD_OFFSET                 8
#define SPI_TMOD_MASK                   (0x3 << SPI_TMOD_OFFSET)
#define SPI_TMOD_TR                     0x0             /* xmit & recv */
#define SPI_TMOD_TO                     0x1             /* xmit only */
#define SPI_TMOD_RO                     0x2             /* recv only */
#define SPI_TMOD_EPROMREAD              0x3             /* eeprom read mode */

#define SPI_SLVOE_OFFSET                10
#define SPI_SRL_OFFSET                  11
#define SPI_CFS_OFFSET                  12

#define DWC_SSI_CTRLR0_FRF_OFFSET       4
#define DWC_SSI_CTRLR0_DFS_OFFSET       0

/*
 * For Keem Bay, CTRLR0[31] is used to select controller m
 * 0: SSI is slave
 * 1: SSI is master
 */
#define DWC_SSI_CTRLR0_KEEMBAY_MST      BIT(31)

/* Bit fields in CTRLR1 */
#define SPI_NDF_MASK                    GENMASK(15, 0)

/* Bit fields in SR, 7 bits */
#define SR_MASK                         0x7f
#define SR_BUSY                         (1 << 0)
#define SR_TF_NOT_FULL                  (1 << 1)
#define SR_TF_EMPT                      (1 << 2)
#define SR_RF_NOT_EMPT                  (1 << 3)
#define SR_RF_FULL                      (1 << 4)
#define SR_TX_ERR                       (1 << 5)
#define SR_DCOL                         (1 << 6)

/* Bit fields in ISR, IMR, RISR, 7 bits */
#define SPI_INT_TXEI                    (1 << 0)
#define SPI_INT_TXOI                    (1 << 1)
#define SPI_INT_RXUI                    (1 << 2)
#define SPI_INT_RXOI                    (1 << 3)
#define SPI_INT_RXFI                    (1 << 4)
#define SPI_INT_MSTI                    (1 << 5)

/* Bit fields in DMACR */
#define SPI_DMA_RDMAE                   (1 << 0)
#define SPI_DMA_TDMAE                   (1 << 1)

struct spi_cfg {
	uint8_t tmode;
	uint8_t dfs;
	uint32_t ndf;
	uint32_t freq;
	struct spi_clk *clk;
};

enum DATA_WIDTH {
	X1 = 0,
	X2 = 1,
	X4 = 2
};

#define SPI_WAIT_RETRIES                5
#define NSEC_PER_SEC    1000000000L
#define NSEC_PER_USEC   1000L
#define BITS_PER_BYTE   8
#define MAX_FIFO_LEN    128

#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))
#define SPI_BUF_SIZE \
	(sizeof_field(struct spi_mem_op, cmd.opcode) + \
	 sizeof_field(struct spi_mem_op, addr.val) + MAX_FIFO_LEN)
#define SPI_GET_BYTE(_val, _idx) \
	((_val) >> (BITS_PER_BYTE * (_idx)) & 0xff)

// #define MIN(_a, _b) ((_a) > (_b)?(_b):(_a))

typedef  unsigned long int  uintptr_t;

static inline uint32_t dw_readl(uint32_t offset)
{
	return mmio_read_32(SPI_NOR_BASE + offset);
}

static inline uint32_t dw_readw(uint32_t offset)
{
	return mmio_read_16(SPI_NOR_BASE + offset);
}

static inline void dw_writel(uint32_t offset, uint32_t val)
{
	mmio_write_32(SPI_NOR_BASE + offset, val);
}

static inline void dw_writew(uint32_t offset, uint16_t val)
{
	mmio_write_16(SPI_NOR_BASE + offset, val);
}

#define REG_IO_WIDTH          4

static inline void dw_write_io_reg(uint8_t reg_io_width, uint32_t offset, uint32_t val)
{
	switch (reg_io_width) {
	case 2:
		dw_writew(DW_SPI_DR, val);
		break;
	case 4:
	default:
		dw_writel(DW_SPI_DR, val);
		break;
	}
}

static inline uint32_t dw_read_io_reg(uint8_t reg_io_width, uint32_t offset)
{
	switch (reg_io_width) {
	case 2:
		return dw_readw(offset);
	case 4:
	default:
		return dw_readl(offset);
	}
}

static inline void spi_enable_chip(int enable)
{
	dw_writel(DW_SPI_SSIENR, (enable ? 1 : 0));
}

static inline void spi_set_clk(uint16_t div)
{
	dw_writel(DW_SPI_BAUDR, div);
}

/* Disable IRQ bits */
static inline void spi_mask_intr(uint32_t mask)
{
	uint32_t new_mask;

	new_mask = dw_readl(DW_SPI_IMR) & ~mask;
	dw_writel(DW_SPI_IMR, new_mask);
}

/* Enable IRQ bits */
static inline void spi_umask_intr(uint32_t mask)
{
	uint32_t new_mask;

	new_mask = dw_readl(DW_SPI_IMR) | mask;
	dw_writel(DW_SPI_IMR, new_mask);
}

/*
 * This disables the SPI controller, interrupts, clears the interrupts status
 * and CS, then re-enables the controller back. Transmit and receive FIFO
 * buffers are cleared when the device is disabled.
 */
static inline void spi_reset_chip(void)
{
	spi_enable_chip(0);
	spi_mask_intr(0xff);
	dw_readl(DW_SPI_ICR);
	dw_writel(DW_SPI_SER, 0);
	spi_enable_chip(1);
}

static inline void spi_shutdown_chip(void)
{
	spi_enable_chip(0);
	spi_set_clk(0);
}

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
void dw_spi_set_clock(struct spi_clk *clk)
{
	uint16_t div;

	div = (DIV_ROUND_UP(clk->in, clk->out) + 1) & 0xfffe;
	spi_set_clk(div);
}



#endif
