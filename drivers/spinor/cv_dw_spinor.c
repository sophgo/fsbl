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
#include <spinor/cv_dw_spinor.h>
#include <security/security.h>
#include "spi/spi-mem.h"
#include "debug.h"

#define CONFIG_NOR_PLL_SWITCHED

static uint8_t fast_mode;
int spi_data_read_qread(uint8_t *dst_buf, uint32_t addr, uint32_t size);
int spi_data_read(void *dst_buf, uint32_t addr, uint32_t size);

struct spi_clk spi_clk = {
	.in = 25000000,
	.out = 12500000
};

void bm_spi_set_clk(uint32_t clk_in, uint32_t clk_out)
{
	struct spi_clk *clk = &spi_clk;
	if (clk_in < clk_out)
		clk_out = clk_in / 8;

	clk->in = clk_in;
	clk->out = clk_out;
}

//
// Function
//     dw_spi_read_status
// Parameters
//     None.
// Description
//     Read SPI-NOR device status.
// Return
//     status reg return from device.
//
int dw_spi_read_status(uint8_t *val)
{
	int ret = 0;

	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(SPINOR_CMD_RDSR, 1),
				SPI_MEM_OP_NO_ADDR,
				SPI_MEM_OP_NO_DUMMY,
				SPI_MEM_OP_DATA_IN(1, val, 1));

	ret = spi_exec_mem_op(&spi_clk, &op);
	if (ret)
		ERROR("read SR0 failed!\n");

	return ret;
}

int dw_spi_write_en(void)
{
	int ret = 0;
	uint8_t status = 0;
	int wait = 0;
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(SPINOR_CMD_WREN, 1),
				SPI_MEM_OP_NO_ADDR,
				SPI_MEM_OP_NO_DUMMY,
				SPI_MEM_OP_NO_DATA);

	ret = spi_exec_mem_op(&spi_clk, &op);
	if (ret) {
		NOTICE("tx write enable command failed!\n");
		return ret;
	}

	do {
		udelay(100);
		ret = dw_spi_read_status(&status);	//05h
		if (!ret && ((status & SPINOR_STATUS_WEL)))
			return 0;
		wait++;
	} while (wait < 1000);
	return -1;
}

int try_read_mode(void)
{
	int ret = 0;
	uint8_t tmp[64] = {0};

	memset(tmp + 32, 0xff, 32);
	ret = spi_data_read(tmp, 0, 32);
	if (ret)
		ERROR("T1F\n");

	ret = spi_data_read_qread(tmp + 32, 0, 32);
	if (ret) {
		ERROR("T4F\n");
		goto normal;
	}

	if (!memcmp(tmp, tmp + 32, 32)) {
		fast_mode = 1;
		return 1;
	}

normal:
	fast_mode = 0;
	return 0;
}

uint32_t spinor_get_source_clk(void)
{
	uint32_t clk_in;

#ifdef CONFIG_NOR_PLL_SWITCHED
	clk_in = 300000000;
#else
	clk_in = 25000000;
#endif

	return clk_in;
}

uint32_t spinor_get_work_clk(uint8_t fast)
{
	uint32_t clk_out;

#ifdef CONFIG_NOR_PLL_SWITCHED
	if (fast) {
		clk_out = 75000000;
	} else {
		clk_out = 30000000;
	}
#else
	if (fast) {
		clk_out = 4200000;
	} else {
		clk_out = 4200000;
	}
#endif

	return clk_out;
}

void cv_dw_spinor_init(void)
{
	uint32_t delay_cnt = 10000;	// 1000ms
	uint8_t status = 0;

	bm_spi_set_clk(spinor_get_source_clk(), spinor_get_work_clk(0));

	// wait until SPI NOR flash is not WIP, max waiting time is 1s
	while (delay_cnt--) {
		dw_spi_read_status(&status);
		if ((status & SPI_STATUS_WIP) == 0)	// device is idle
			break;
		udelay(100);
	}

	try_read_mode();
}

static uint8_t spi_nor_convert_opcode(uint8_t opcode, const uint8_t table[][2], size_t size)
{
	size_t i;

	for (i = 0; i < size; i++)
		if (table[i][0] == opcode)
			return table[i][1];

	/* No conversion found, keep input op code. */
	return opcode;
}

uint8_t spi_nor_convert_3to4_read(uint8_t opcode)
{
	static const uint8_t spi_nor_3to4_read[][2] = {
		{ SPINOR_OP_READ,       SPINOR_OP_READ_4B },
		{ SPINOR_OP_READ_FAST,  SPINOR_OP_READ_FAST_4B },
		{ SPINOR_OP_READ_1_1_2, SPINOR_OP_READ_1_1_2_4B },
		{ SPINOR_OP_READ_1_2_2, SPINOR_OP_READ_1_2_2_4B },
		{ SPINOR_OP_READ_1_1_4, SPINOR_OP_READ_1_1_4_4B },
		{ SPINOR_OP_READ_1_4_4, SPINOR_OP_READ_1_4_4_4B },
		{ SPINOR_OP_READ_1_1_8, SPINOR_OP_READ_1_1_8_4B },
		{ SPINOR_OP_READ_1_8_8, SPINOR_OP_READ_1_8_8_4B },

		{ SPINOR_OP_READ_1_1_1_DTR,     SPINOR_OP_READ_1_1_1_DTR_4B },
		{ SPINOR_OP_READ_1_2_2_DTR,     SPINOR_OP_READ_1_2_2_DTR_4B },
		{ SPINOR_OP_READ_1_4_4_DTR,     SPINOR_OP_READ_1_4_4_DTR_4B },
	};

	return spi_nor_convert_opcode(opcode, spi_nor_3to4_read,
			ARRAY_SIZE(spi_nor_3to4_read));
}

int spi_nor_read_reg(uint8_t cmd, void *buf, uint8_t len)
{
	int ret = 0;
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(cmd, 1),
				SPI_MEM_OP_NO_ADDR,
				SPI_MEM_OP_NO_DUMMY,
				SPI_MEM_OP_DATA_IN(len, buf, 1));

	ret = spi_exec_mem_op(&spi_clk, &op);
	if (ret)
		ERROR("opcode:0x%x read reg failed!\n", cmd);

	return ret;
}

int spi_nor_write_reg(uint8_t cmd, void *buf, uint8_t len)
{
	int ret;
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(cmd, 1),
				SPI_MEM_OP_NO_ADDR,
				SPI_MEM_OP_NO_DUMMY,
				SPI_MEM_OP_DATA_OUT(len, buf, 1));

	ret = spi_exec_mem_op(&spi_clk, &op);
	if (ret)
		ERROR("opcode:0x%x write reg failed!\n", cmd);

	return ret;
}
int spi_nor_wait_for_ready(void)
{
	uint32_t wait = 0;
	uint8_t status = 0xff;

	do {
		udelay(100);
		dw_spi_read_status(&status);	//05h
		if ((status & SPINOR_STATUS_WIP) == 0) { // 3s, spec 0.15~1s
			return 0;
		}
		wait++;
	} while (wait < 30000);

	return -1;
}

int spi_data_read_qread(uint8_t *dst_buf, uint32_t addr, uint32_t size)
{
	uint8_t addr_len;
	int ret = 0;
	addr_len = (addr > _16M) ? ADDR_4B:ADDR_3B;

	bm_spi_set_clk(spinor_get_source_clk(), spinor_get_work_clk(1));
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(SPINOR_OP_READ_1_1_4, 1),
				SPI_MEM_OP_ADDR(addr_len, addr, 1),
				SPI_MEM_OP_DUMMY(1, 1),
				SPI_MEM_OP_DATA_IN(size, dst_buf, 4));

	if (addr_len == ADDR_4B)
		op.cmd.opcode = spi_nor_convert_3to4_read(op.cmd.opcode);

	ret = spi_exec_mem_op(&spi_clk, &op);
	if (ret)
		ERROR("read data failed!\n");

	return ret;
}

int spi_data_read(void *dst_buf, uint32_t addr, uint32_t size)
{
	uint8_t addr_len;
	int ret = 0;
	addr_len = (addr > _16M) ? ADDR_4B:ADDR_3B;

	bm_spi_set_clk(spinor_get_source_clk(), spinor_get_work_clk(0));
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(SPINOR_OP_READ, 1),
				SPI_MEM_OP_ADDR(addr_len, addr, 1),
				SPI_MEM_OP_DUMMY(0, 0),
				SPI_MEM_OP_DATA_IN(size, dst_buf, 1));

	if (addr_len == ADDR_4B)
		op.cmd.opcode = spi_nor_convert_3to4_read(SPINOR_OP_READ);

	ret = spi_exec_mem_op(&spi_clk, &op);
	if (ret)
		ERROR("read data failed!\n");

	return ret;
}

#define MAX_READ_LEN  65536
int cv_dw_spinor_read(void *buff, uint32_t offset, size_t size)
{
	int ret = 0;
	uint32_t len;

	while (size) {
		len = (size > MAX_READ_LEN) ? MAX_READ_LEN : size;

		if (fast_mode)
			ret = spi_data_read_qread(buff, offset, len);
		else
			ret = spi_data_read(buff, offset, len);

		if (ret) {
			ERROR("read data failed at 0x%x\n", offset);
			return ret;
		}
		size -= len;
		buff += len;
		offset += len;
	}

	return ret;
}
