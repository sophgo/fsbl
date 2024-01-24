#ifndef __TPU_DRIVER_H__
#define __TPU_DRIVER_H__

#include <debug.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>


typedef unsigned char	uint8_t;
typedef unsigned char	u8;
typedef unsigned int	u32;
typedef unsigned long	u64;

#define ASSERT(x) assert(x)
#define TDMA_DESCRIPTOR_ALIGNED_BIT		6
#define BDC_ENGINE_CMD_ALIGNED_BIT		8

typedef struct __dma_hdr_t {
	uint16_t dmabuf_magic_m;
	uint16_t dmabuf_magic_s;
	uint32_t dmabuf_size;
	uint32_t cpu_desc_count;
	uint32_t bd_desc_count; //16bytes
	uint32_t tdma_desc_count;
	uint32_t tpu_clk_rate;
	uint32_t pmubuf_size;
	uint32_t pmubuf_offset; //32bytes
	uint32_t arraybase_0_L;
	uint32_t arraybase_0_H;
	uint32_t arraybase_1_L;
	uint32_t arraybase_1_H; //48bytes
	uint32_t arraybase_2_L;
	uint32_t arraybase_2_H;
	uint32_t arraybase_3_L;
	uint32_t arraybase_3_H; //64bytes

	uint32_t arraybase_4_L;
	uint32_t arraybase_4_H;
	uint32_t arraybase_5_L;
	uint32_t arraybase_5_H;
	uint32_t arraybase_6_L;
	uint32_t arraybase_6_H;
	uint32_t arraybase_7_L;
	uint32_t arraybase_7_H;
	uint32_t reserve[8];   //128bytes, 128bytes align
} dma_hdr_t;
#define TPU_DMABUF_HEADER_M		0xB5B5

// CPU_OP_SYNC structure
#define CPU_ENGINE_DESCRIPTOR_NUM 56
#define CPU_ENGINE_DESCRIPTOR_DMA_NUM CPU_ENGINE_DESCRIPTOR_NUM
#define CPU_ENGINE_BYTES (CPU_ENGINE_DESCRIPTOR_NUM*sizeof(u32))

typedef struct __bmk_cpu_sync_desc_t {
	u32 op_type; // CPU_CMD_ACCPI0
	u32 num_bd; // CPU_CMD_ACCPI1
	u32 num_gdma; // CPU_CMD_ACCPI2
	u32 offset_bd; // CPU_CMD_ACCPI3
	u32 offset_gdma; // CPU_CMD_ACCPI4
	u32 reserved[2]; // CPU_CMD_ACCPI5-CPU_CMD_ACCPI6
	char str[(CPU_ENGINE_DESCRIPTOR_NUM - 7) * sizeof(u32)];
} bmk_cpu_sync_desc_t;

enum TPU_SEC_SMCCALL {
	TPU_SEC_SMC_LOADCMD = 0x1001,
	TPU_SEC_SMC_RUN,
	TPU_SEC_SMC_WAIT,
};

typedef struct __TPU_SECURE_FUN {
	u64 param0;
	u64 param1;
	u64 param2;
	u64 param3;
	u64 param4;
	u64 param5;
	u64 param6;
	u64 param7;
} TPU_SECURE_FUN;

typedef struct __TPU_BUF_INFO {
	u64 dmabuf_addr_sec;

	u64 cmdbuf_addr_normal;
	u32 cmdbuf_len_normal;
	u64 weight_addr_normal;
	u32 weight_len_normal;
	u64 neuron_addr_normal;
} TPU_BUF_INFO;

#endif /* __TPU_DRIVER_H__ */

