/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <endian.h>
#include <mmio.h>
#include <debug.h>
#include <assert.h>
#include <errno.h>
#include <bl_common.h>
#include <platform.h>
#include <platform_def.h>
#include <platform_common_def.h>
#include <delay_timer.h>
#include <cv_private.h>
#include <xlat_tables.h>
//#include <cv_sd.h>
#include <cli.h>
#include <console.h>
#include <string.h>
#include <cv_efuse.h>
#include "cv_usb.h"
#include "cryptodma.h"
#include "cv_private.h"
#include "cv_security.h"
#include "mbedtls/sha256.h"

#define u32 uint32_t
#define u64 uint64_t
#define u8 uint8_t

#define MAP_DEVICE                                                             \
	MAP_REGION_FLAT(0, 0x100000000, MT_DEVICE | MT_RW | MT_SECURE)
#define MAP_DRAM                                                               \
	MAP_REGION_FLAT(0x100000000, 0x80000000, MT_MEMORY | MT_RW | MT_SECURE)
#define MAP_IO_DATA                                                            \
	MAP_REGION_FLAT(BM_IO_BUF_BASE, BM_IO_BUF_SIZE,                        \
			MT_DEVICE | MT_RW | MT_SECURE)

/*
 * Table of regions for various BL stages to map using the MMU.
 * This doesn't include TZRAM as the 'mem_layout' argument passed to
 * arm_configure_mmu_elx() will give the available subset of that,
 */
#ifdef IMAGE_BL1
static const mmap_region_t plat_bm_mmap[] = {
	MAP_DEVICE,
	MAP_DRAM,
	MAP_IO_DATA,
	{ 0 },
};
#else
static const mmap_region_t plat_bm_mmap[] = {
	MAP_DEVICE,
	MAP_DRAM,
	{ 0 },
};
#endif
/*******************************************************************************
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 ******************************************************************************/
// Writable area for customer
static struct efuse_area efuse_writable_areas[] = {
	// Customer: fastboot
	{ 0x04, 4 },
	// ROM-patch data
	{ 0x2c, 96 },
	// Device ID
	{ 0x8c, 8 },
	// Secure configuration
	{ 0xA0, 8 },
	// Hash0 of Root public
	{ 0xA8, 32 },
	// REE Debug port password
	{ 0xC8, 8 },
	// Loader encryption key
	{ 0xD8, 16 },
	// Device unique key
	{ 0xE8, 16 },
	// secure program lock field
	{ 0xF8, 4 }
};
#define DEFINE_CONFIGURE_MMU_EL(_el)                                           \
	void bm_configure_mmu_el##_el(                                         \
		unsigned long total_base, unsigned long total_size,            \
		unsigned long ro_start, unsigned long ro_limit,                \
		unsigned long coh_start, unsigned long coh_limit)              \
	{                                                                      \
		mmap_add_region(total_base, total_base, total_size,            \
				MT_MEMORY | MT_RW | MT_SECURE);                \
		mmap_add_region(ro_start, ro_start, ro_limit - ro_start,       \
				MT_MEMORY | MT_RO | MT_SECURE);                \
		mmap_add_region(coh_start, coh_start, coh_limit - coh_start,   \
				MT_DEVICE | MT_RW | MT_SECURE);                \
		mmap_add(plat_bm_mmap);                                        \
		init_xlat_tables();                                            \
		enable_mmu_el##_el(0);                                         \
	}

/* Define EL1 and EL3 variants of the function initialising the MMU */
DEFINE_CONFIGURE_MMU_EL(1)
DEFINE_CONFIGURE_MMU_EL(3)

int keep_mmu_disabled;

uint32_t system_counter_freq = SYS_COUNTER_FREQ_IN_TICKS;

int plat_cryptodma_des(uintptr_t src, uintptr_t des, uint64_t len,
		       unsigned char *key, unsigned char *iv, int algo)
{
	uint32_t data;
	uint32_t ts;

	__aligned(32) uint32_t dma_descriptor[22];

	INFO("plat_cryptodma_des, src=%p, des=%p, image_size=0x%x\n",
	     (void *)src, (void *)des, (int)len);

	// Prepare descriptor
	memset(dma_descriptor, 0, sizeof(dma_descriptor));

	dma_descriptor[CRYPTODMA_CTRL] =
		DES_USE_DESCRIPTOR_IV | DES_USE_DESCRIPTOR_KEY | algo | 0xF;
	dma_descriptor[CRYPTODMA_CIPHER] =
		AES_KEY_MODE << 3 | CBC_ENABLE << 1 | DECRYPT_ENABLE;

	dma_descriptor[CRYPTODMA_SRC_ADDR_L] =
		(uint32_t)((uint64_t)src & 0xFFFFFFFF);
	dma_descriptor[CRYPTODMA_SRC_ADDR_H] = (uint32_t)((uint64_t)src >> 32);

	dma_descriptor[CRYPTODMA_DST_ADDR_L] =
		(uint32_t)((uint64_t)des & 0xFFFFFFFF);
	dma_descriptor[CRYPTODMA_DST_ADDR_H] = (uint32_t)((uint64_t)des >> 32);

	dma_descriptor[CRYPTODMA_DATA_AMOUNT_L] =
		(uint32_t)((uint64_t)len & 0xFFFFFFFF);
	dma_descriptor[CRYPTODMA_DATA_AMOUNT_H] =
		(uint32_t)((uint64_t)len >> 32);

	if (algo == DES_USE_BYPASS) {
		memset(&dma_descriptor[CRYPTODMA_KEY], 0, 16);
		memset(&dma_descriptor[CRYPTODMA_IV], 0, 16);
	} else {
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 16);
		memcpy(&dma_descriptor[CRYPTODMA_IV], iv, 16);
	}

	// Set cryptodma control
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_INT_MASK, 0x3);
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DES_BASE_L,
		      (uint32_t)((uint64_t)dma_descriptor & 0xFFFFFFFF));
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DES_BASE_H,
		      (uint32_t)((uint64_t)dma_descriptor >> 32));

	data = mmio_read_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DMA_CTRL);

	flush_dcache_range((unsigned long)dma_descriptor,
			   sizeof(dma_descriptor));

	// Clear interrupt
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_WR_INT, 0x1);
	// Trigger cryptodma engine
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DMA_CTRL,
		      DMA_WRITE_MAX_BURST << 24 | DMA_READ_MAX_BURST << 16 |
			      DMA_DESCRIPTOR_MODE << 1 | DMA_ENABLE);

	ts = get_timer(0);
	do {
		data = mmio_read_32(SEC_CRYPTODMA_BASE + CRYPTODMA_WR_INT);
		INFO("INT status 0x%x\n", data);
		if (get_timer(ts) >= 1000) {
			ERROR("Decryption timeout\n");
			break;
		}
	} while (data == 0);

	return 0;
}

int plat_cryptodma_exec(uintptr_t src, uintptr_t dst, uint64_t len,
			spacc_exec_config *config)
{
	return 0;
}

static inline void setup_src(u32 *dma_descriptor, uintptr_t src, u32 len)
{
	flush_dcache_range((unsigned long)src, len);

	dma_descriptor[CRYPTODMA_SRC_ADDR_L] =
		(uint32_t)((uint64_t)src & 0xFFFFFFFF);
	dma_descriptor[CRYPTODMA_SRC_ADDR_H] = (uint32_t)((uint64_t)src >> 32);

	dma_descriptor[CRYPTODMA_SRC_LEN] = len;
}

// some algo like sha256/sha1/sm3 needn't dst
#define setup_dst(dst)                                                         \
	do {                                                                   \
		dma_descriptor[CRYPTODMA_DST_ADDR_L] =                         \
			(uint32_t)((uint64_t)(dst) & 0xFFFFFFFF);              \
		dma_descriptor[CRYPTODMA_DST_ADDR_H] =                         \
			(uint32_t)((uint64_t)(dst) >> 32);                     \
	} while (0)
static inline void setup_key_size(u32 *dma_descriptor,
				  CRYPTODMA_KEY_SIZE_E size, unsigned char *key)
{
	switch (size) {
	case CRYPTODMA_KEY_SIZE_64BITS:
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 8);
		break;
	case CRYPTODMA_KEY_SIZE_128BITS:
		dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 5);
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 16);
		break;
	case CRYPTODMA_KEY_SIZE_192BITS:
		dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 4);
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 24);
		break;
	case CRYPTODMA_KEY_SIZE_256BITS:
		dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 3);
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, 32);
		break;
	default:
		break;
	}
}
static inline void setup_mode(u32 *dma_descriptor, CRYPTODMA_ALGO_MODE_E mode,
			      unsigned char *iv)
{
	switch (mode) {
	case CRYPTODMA_ALGO_MODE_CBC:
		dma_descriptor[CRYPTODMA_CTRL] |= DES_USE_DESCRIPTOR_IV;
		dma_descriptor[CRYPTODMA_CIPHER] = CBC_ENABLE << 1;
		memcpy(&dma_descriptor[CRYPTODMA_IV], iv, 16);
		break;
	case CRYPTODMA_ALGO_MODE_CTR:
		dma_descriptor[CRYPTODMA_CTRL] |= DES_USE_DESCRIPTOR_IV;
		dma_descriptor[CRYPTODMA_CIPHER] = 0x1 << 2;
		memcpy(&dma_descriptor[CRYPTODMA_IV], iv, 16);
		break;
	case CRYPTODMA_ALGO_MODE_ECB:
	default:
		break;
	}
}
static inline void setup_action(u32 *dma_descriptor, CRYPTODMA_ACTION_E action)
{
	if (action == CRYPTODMA_ACTION_ENCRYPTION)
		dma_descriptor[CRYPTODMA_CIPHER] |= 0x1;
}

int trigger_cryptodma_work_and_wait_finish(u32 *dma_descriptor, int irq_enable)
{
	u32 data;
	u32 ts;
	flush_dcache_range((unsigned long)dma_descriptor,
			   32 * sizeof(uint32_t));
	// set exec dma descriptor addr
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DES_BASE_L,
		      (uint32_t)((uint64_t)dma_descriptor & 0xFFFFFFFF));
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DES_BASE_H,
		      (uint32_t)((uint64_t)dma_descriptor >> 32));

	// Set cryptodma dma control: poll mode
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_INT_MASK, 0x0);
	// irq_mode
	// mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_INT_MASK, 0x7);

	// Clear interrupt
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_WR_INT, 0x7);

	// Trigger cryptodma engine
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DMA_CTRL,
		      DMA_WRITE_MAX_BURST << 24 | DMA_READ_MAX_BURST << 16 |
			      DMA_DESCRIPTOR_MODE << 1 | DMA_ENABLE);
#ifdef DEBUG
	for (int i = 0; i < 22; i++) {
		NOTICE("dma_descriptor[%d]:%x\n", i, dma_descriptor[i]);
	}

#ifdef DEBUG
	NOTICE("SEC_CRYPTODMA_BASE + CRYPTODMA_DMA_CTRL = 0x%x\n",
	       mmio_read_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DMA_CTRL));
	NOTICE("SEC_CRYPTODMA_BASE + 0x4 = 0x%x\n",
	       mmio_read_32(SEC_CRYPTODMA_BASE + 0x4));
	NOTICE("SEC_CRYPTODMA_BASE + 0x8 = 0x%x\n",
	       mmio_read_32(SEC_CRYPTODMA_BASE + 0x8));
	NOTICE("SEC_CRYPTODMA_BASE + 0x10 = 0x%x\n",
	       mmio_read_32(SEC_CRYPTODMA_BASE + CRYPTODMA_WR_INT));
	NOTICE("SEC_CRYPTODMA_BASE + 0x14 = 0x%x\n",
	       mmio_read_32(SEC_CRYPTODMA_BASE + 0x14));
	NOTICE("SEC_CRYPTODMA_BASE + 0x18 = 0x%x\n",
	       mmio_read_32(SEC_CRYPTODMA_BASE + 0x18));
#endif
#endif
	ts = get_timer(0);
	do {
		data = mmio_read_32(SEC_CRYPTODMA_BASE + CRYPTODMA_WR_INT);
		NOTICE("INT status 0x%x\n", data);
		if (get_timer(ts) >= 1000) {
			ERROR("exec timeout\n");
			return -1;
		}
	} while (data == 0);

	return 0;
}


int plat_cryptodma_base64(uintptr_t src, uint64_t len, uintptr_t dst,
			  uint32_t customer_code, uint32_t action)
{
	__aligned(64) u32 dma_descriptor[32] = { 0 };
	int result_len;

	INFO("%s: src=0x%lx, len=0x%lx dst=0x%lx\n", __func__, src, len, dst);

	dma_descriptor[CRYPTODMA_CTRL] = DES_USE_BASE64 | 0xF;
	if (action == 1) {
		dma_descriptor[CRYPTODMA_CIPHER] = 0x1;
		result_len = (len + (3 - 1)) / 3 * 4;
	} else {
		result_len = (len / 4) * 3;
	}
	dma_descriptor[CRYPTODMA_DST_LEN] = result_len;
	printf("result_len = %d\n", result_len);
	setup_src(dma_descriptor, src, len);
	setup_dst(dst);

	for (int i = 0; i < 22; i++) {
		NOTICE("dma_descriptor[%d] = %x\n", i, dma_descriptor[i]);
	}
	INFO("dma_descriptor address: %p\n", dma_descriptor);
	if (customer_code)
		mmio_write_32(SEC_CRYPTODMA_BASE +
				      CRYPTODMA_BASE64_CUSTOMIZE_CODE,
			      customer_code);

	NOTICE("CRYPTODMA_BASE64_CUSTOMIZE_CODE = 0x%x\n",
	       mmio_read_32(SEC_CRYPTODMA_BASE +
			    CRYPTODMA_BASE64_CUSTOMIZE_CODE));

	if (trigger_cryptodma_work_and_wait_finish(dma_descriptor, 0))
		return -1;

	return result_len;
}
int plat_cryptodma_do(int isEncrypt, uintptr_t in, uintptr_t out, uint64_t len,
              unsigned char *key, E_KEY_MODE keyMode, unsigned char *iv,
              E_ALGO a, E_MODE b, uint32_t *state,E_OTP_KEY otp)
{
    uint32_t data;
    int i;
    uint32_t des_ctrl = 0;
    __attribute__((aligned(64))) uint32_t dma_descriptor[32] = { 0 };
    uint64_t src = (uintptr_t)in;
    uint64_t des = (uintptr_t)out;
	NOTICE("%s(): src=%p, len=%lu, out=%p, key=%p, iv=%p E_ALGO=%d, E_MODE=%d,KEY_MODE=%d,isEncrypt=%d\n",
	       __func__, (void *)src, len, (void *)des, (void *)key, (void *)iv, a, b, keyMode, isEncrypt);		   
    // 参数检查
    if (!in || !out || !len) {
		NOTICE("in or out or len is null\n");
        return -1;
    }

    // 对于需要密钥的算法检查密钥
    if ((a == AES || a == SM4 || a == DES || a == TDES) && (!key || !iv)) {
		NOTICE("key or iv is null\n");
        return -1;
    }

    // 基础控制位设置
    des_ctrl = 0xF;  // 基础控制位

    switch (a) {
    case AES:
        des_ctrl |= DES_USE_DESCRIPTOR_IV | DES_USE_AES |IV_OUT0_SELECT;
		if(otp==USE_OTP_KEY){
			des_ctrl |= OTP_KEY_SEL;
		}else{
			des_ctrl |= DES_USE_DESCRIPTOR_KEY;
		}
        dma_descriptor[CRYPTODMA_CTRL] = des_ctrl;
        switch (b) {
        case ECB:
            break;
        case CBC:
            dma_descriptor[CRYPTODMA_CIPHER] = CBC_ENABLE << 1;
            break;
        case CTR:
            dma_descriptor[CRYPTODMA_CIPHER] = 0x1 << 2;
            break;
        default:
            return -1;
        }

        switch (keyMode) {
        case KEY_128BITS:
            dma_descriptor[CRYPTODMA_CIPHER] |= (0x4 << 3);
            break;
        case KEY_192BITS:
            dma_descriptor[CRYPTODMA_CIPHER] |= (0x2 << 3);
            break;
        case KEY_256BITS:
            dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 3);
            break;
        default:
           break;
        }
        break;

    case SM3:
        des_ctrl |= DES_USE_SM3 | DES_USE_DESCRIPTOR_KEY|DES_USE_DESCRIPTOR_IV;
        dma_descriptor[CRYPTODMA_CTRL] = des_ctrl;
        dma_descriptor[CRYPTODMA_CIPHER] = 0x1;
        
        break;

    case SM4:
        des_ctrl |= DES_USE_DESCRIPTOR_IV | DES_USE_SM4 | IV_OUT0_SELECT;
		if(otp==USE_OTP_KEY){
			des_ctrl |= OTP_KEY_SEL;
		}else{
			des_ctrl |= DES_USE_DESCRIPTOR_KEY;
		}
        dma_descriptor[CRYPTODMA_CTRL] = des_ctrl;
        switch (b) {
        case ECB:
            break;
        case CBC:
            dma_descriptor[CRYPTODMA_CIPHER] = CBC_ENABLE << 1;
            break;
        case CTR:
            dma_descriptor[CRYPTODMA_CIPHER] = 0x1 << 2;
            break;
        case OFB:
            dma_descriptor[CRYPTODMA_CIPHER] = 0x1 << 3;
            break;
        default:
            return -1;
        }
        break;
	case BYPASS: {
		dma_descriptor[CRYPTODMA_CTRL] = DES_USE_BYPASS | 0xF;
		break;
	}
    case SHA256:
		dma_descriptor[CRYPTODMA_CIPHER] = (0x1 << 1);
    case SHA1:
        dma_descriptor[CRYPTODMA_CTRL] = DES_USE_DESCRIPTOR_KEY  |
						 DES_USE_SHA | 0xF;
		dma_descriptor[CRYPTODMA_CIPHER] |= 0x1;
        break;
	case BASE64_CUSTOMER:
	case BASE64: {
		dma_descriptor[CRYPTODMA_CTRL] = DES_USE_BASE64 | 0xF;
		if (isEncrypt) {
			dma_descriptor[BASE64_SIZE] = (len + (3 - 1)) / 3 * 4;
		} else {
			dma_descriptor[BASE64_SIZE] = (len / 4) * 3;
		}
		break;
	}
    case TDES:
		dma_descriptor[CRYPTODMA_CIPHER] = (0x1 << 3);
    case DES:
        des_ctrl |= DES_USE_DESCRIPTOR_IV | DES_USE_DES | DES_USE_DESCRIPTOR_KEY;
        dma_descriptor[CRYPTODMA_CTRL] = des_ctrl;
        switch (b) {
        case ECB:
            break;
        case CBC:
            dma_descriptor[CRYPTODMA_CIPHER] |= (CBC_ENABLE << 1);
            break;
        case CTR:
            dma_descriptor[CRYPTODMA_CIPHER] |= (0x1 << 2);
            break;
        default:
            return -1;
        }
        break;
    default:
        return -1;
    }

    if (isEncrypt) {
        dma_descriptor[CRYPTODMA_CIPHER] |= 0x1;
    }

    dma_descriptor[CRYPTODMA_SRC_ADDR_L] = (uint32_t)(src & 0xFFFFFFFF);
    dma_descriptor[CRYPTODMA_SRC_ADDR_H] = (uint32_t)(src >> 32);
    dma_descriptor[CRYPTODMA_DATA_AMOUNT_L] = (uint32_t)(len & 0xFFFFFFFF);
    dma_descriptor[CRYPTODMA_DATA_AMOUNT_H] = (uint32_t)(len >> 32);

    if (a != SHA256 && a != SHA1 && a != SM3) {
        dma_descriptor[CRYPTODMA_DST_ADDR_L] = (uint32_t)(des & 0xFFFFFFFF);
        dma_descriptor[CRYPTODMA_DST_ADDR_H] = (uint32_t)(des >> 32);
    }

    if (a == AES || a == SM4 || a == DES || a == TDES) {
        uint32_t key_size = 0;
        switch (keyMode) {
        case KEY_128BITS:
            key_size = 16;
            break;
        case KEY_192BITS:
            key_size = 24;
            break;
        case KEY_256BITS:
            key_size = 32;
            break;
        }
#ifdef DEBUG
		for (i = 0; i < 16; i++) {
			NOTICE("key[%d] %x\n", i, key[i]);
			NOTICE("iv[%d] %x\n", i, iv[i]);
		}
#endif
		memcpy(&dma_descriptor[CRYPTODMA_KEY], key, key_size);
		memcpy(&dma_descriptor[CRYPTODMA_IV], iv, 16);
    }else if (a == SHA256) {
		// Clear SHA output first
		for (i = 0; i < 8; i++)
			mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_SHA_PARA + i * 4,
				      0x12345678);

		for (i = 0; i < 8; i++)
			dma_descriptor[CRYPTODMA_KEY + i] = state[i];
	} else if (a == SM3) {
		for (i = 0; i < 8; i++)
			mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_SM3_PARA + i * 4,
				      0x12345678);

		for (i = 0; i < 8; i++)
			dma_descriptor[CRYPTODMA_KEY + i] = state[i];
	} else if (a == SHA1) {
		// Clear SHA output first
		for (i = 0; i < 5; i++)
			mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_SHA_PARA + i * 4,
				      0x12345678);

		for (i = 0; i < 5; i++)
			dma_descriptor[CRYPTODMA_KEY + i] = state[i];
	} 

	for (int i = 0; i < 22; i++) {
		NOTICE("dma_descriptor[%d] = %x\n", i, dma_descriptor[i]);
	}

	flush_dcache_range((unsigned long)dma_descriptor, sizeof(dma_descriptor));
    flush_dcache_range((unsigned long)src, len);
    mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DES_BASE_L,
                  (uint32_t)((uint64_t)dma_descriptor & 0xFFFFFFFF));
    mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DES_BASE_H,
                  (uint32_t)((uint64_t)dma_descriptor >> 32));
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_INT_MASK, 0x0);
	// Clear interrupt
	mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_WR_INT, 0x7);

	if (a == BASE64_CUSTOMER) {
		uint32_t value = 0;
		// '/'->'!';'+'->'#'.'+'=0x2b '/'=0x2f
		value = ('!' << 8) | '#';
		mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_BASE64_CUSTOMIZE_CODE,
			      value);
	}

	NOTICE("cryptodma start !\n");
	
    mmio_write_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DMA_CTRL,
                  DMA_WRITE_MAX_BURST << 24 | DMA_READ_MAX_BURST << 16 |
                  DMA_DESCRIPTOR_MODE << 1 | DMA_ENABLE);
	NOTICE("cryptodma dma_ctrl = %x\n", mmio_read_32(SEC_CRYPTODMA_BASE + CRYPTODMA_DMA_CTRL));
    do {
        data = mmio_read_32(SEC_CRYPTODMA_BASE + CRYPTODMA_WR_INT);
    } while (data == 0);
	NOTICE("cryptodma done\n");
    if (a == SHA256) {
		for (i = 0; i < 8; i++) {
			uintptr_t a = SEC_CRYPTODMA_BASE + CRYPTODMA_SHA_PARA + i * 4;
			state[i] = mmio_read_32(a);
			NOTICE("state[%d] = %x\n", i, state[i]);
		}
		len = 32;
	} else if (a == SHA1) {
		for (i = 0; i < 5; i++) {
			uintptr_t a = SEC_CRYPTODMA_BASE + CRYPTODMA_SHA_PARA + i * 4;
			state[i] = mmio_read_32(a);
			NOTICE("state[%d] = %x\n", i, state[i]);
		}
		len = 20;
	} else if (a == SM3) {
		for (i = 0; i < 8; i++) {
			uintptr_t a = SEC_CRYPTODMA_BASE + CRYPTODMA_SM3_PARA + i * 4;
			state[i] = mmio_read_32(a);
			NOTICE("state[%d] = %x\n", i, state[i]);
		}
		len = 32;
	} else {
		inv_dcache_range(des, len);
	}
    return len;
}
int plat_cryptodma_sha256_start(plat_cryptodma_sha256_t *ctx)
{
	INFO("%s:\n", __func__);

	memset(ctx, 0, sizeof(plat_cryptodma_sha256_t));

	ctx->state[0] = htobe32(0x6A09E667);
	ctx->state[1] = htobe32(0xBB67AE85);
	ctx->state[2] = htobe32(0x3C6EF372);
	ctx->state[3] = htobe32(0xA54FF53A);
	ctx->state[4] = htobe32(0x510E527F);
	ctx->state[5] = htobe32(0x9B05688C);
	ctx->state[6] = htobe32(0x1F83D9AB);
	ctx->state[7] = htobe32(0x5BE0CD19);

	return 0;
}

int plat_cryptodma_sha256_blocks(plat_cryptodma_sha256_t *ctx, const u8 *src,
				 size_t len)
{
	__aligned(64) u32 dma_descriptor[32] = { 0 };
	u32 i;
	u32 result_size = 8;
	size_t alen;

	alen = len / 64 * 64;

	INFO("%s: S2 msg=%p, len=0x%lx alen=0x%lx\n", __func__, src, len, alen);
	if (alen != len) {
		ERROR("%s: len (%lu) must be multiple of 64 bytes\n", __func__,
		      len);
		return -1;
	}

	ctx->total += len;

	dma_descriptor[CRYPTODMA_CTRL] =
		DES_USE_DESCRIPTOR_KEY | DES_USE_SHA | 0xF;
	dma_descriptor[CRYPTODMA_CIPHER] = (0x1 << 1);
	dma_descriptor[CRYPTODMA_CIPHER] |= 0x1;

	for (i = 0; i < result_size; i++)
		dma_descriptor[CRYPTODMA_KEY + i] = ctx->state[i];

	setup_src(dma_descriptor, (uintptr_t)src, len);

	if (trigger_cryptodma_work_and_wait_finish(dma_descriptor, 0))
		return -1;

	for (i = 0; i < result_size; i++)
		ctx->state[i] = mmio_read_32(SEC_CRYPTODMA_BASE +
					     CRYPTODMA_SHA_PARA + i * 4);

	return 0;
}

int plat_cryptodma_sha256_finish(plat_cryptodma_sha256_t *ctx, const void *msg,
				 u64 rlen, u8 digest[32])
{
	mbedtls_sha256_context mt_ctx;

	u32 pad_len;
	u64 bits_len_be;
	u8 block[128] = { 0 };
	u32 out[8] = { 0 };
	u8 *p;
	int i;

	INFO("%s: msg=%p, rlen=0x%lx digest=%p\n", __func__, msg, rlen, digest);

	if (rlen >= 64) {
		ERROR("%s: rlen (%lu) must be lesser than 64 bytes\n", __func__,
		      rlen);
		return -1;
	}

	if (!msg || !rlen) {
		msg = "";
		rlen = 0;
	}

	ctx->total += rlen;
	bits_len_be = htobe64(ctx->total * 8);

	if (rlen < 56)
		pad_len = 55 - rlen;
	else
		pad_len = 119 - rlen;

	INFO("%s: pad_len=%u total=0x%lx\n", __func__, pad_len, ctx->total);

	p = block;

	// copy rest data
	memcpy(p, msg, rlen);
	p += rlen;
	*p++ = 0x80; // pad bit 1
	p += pad_len; // pad '\0' * pad_len
	memcpy(p, &bits_len_be,
	       sizeof(uint64_t)); // append the length of bits in big-endian

	// SW impl. is faster for one block
	mbedtls_sha256_init(&mt_ctx);
	mbedtls_sha256_starts_ret(&mt_ctx, 0);

	for (i = 0; i < 8; i++)
		mt_ctx.state[i] = be32toh(ctx->state[i]);

	mbedtls_sha256_update_ret(&mt_ctx, block, (rlen < 56) ? 64 : 128);

	for (i = 0; i < 8; i++)
		out[i] = be32toh(mt_ctx.state[i]);

	memcpy(digest, out, 32);

	return 0;
}

int plat_soft_sha256(const void *msg, uint64_t len, uint8_t digest[32])
{
	plat_cryptodma_sha256_t ctx;
	u64 alen = 0;

	plat_cryptodma_sha256_start(&ctx);

	if (len > 64) {
		alen = len / 64 * 64;
		plat_cryptodma_sha256_blocks(&ctx, msg, alen);
		plat_cryptodma_sha256_finish(&ctx, msg + alen, len - alen,
					     digest);
	} else {
		plat_cryptodma_sha256_finish(&ctx, msg, len, digest);
	}

	return 32;
}

static struct {
	uint32_t start;
	uint32_t size;
	uint32_t wlock_shift;
	uint32_t rlock_shift;
} efuse_lock_map[] = { [EFUSE_LOCK_KPUB0] = { 0xA8, 32, 0, 8 },
		       [EFUSE_LOCK_DEBUG_PW] = { 0xC8, 16, 2, 10 },
		       [EFUSE_LOCK_LOADER_EK] = { 0xD8, 16, 4, 12 },
		       [EFUSE_LOCK_DEVICE_EK] = { 0xE8, 16, 6, 14 } };

static int plat_efuse_is_not_secret(uint32_t addr)
{
	uint32_t lock_value =
		mmio_read_32(SEC_EFUSE_SHADOW_REG + EFUSE_LOCK_ADDR);
	int i;

	if (addr == EFUSE_LOCK_ADDR)
		return 1;

	for (i = 0; i < ARRAY_SIZE(efuse_lock_map); i++) {
		uint32_t start = efuse_lock_map[i].start;
		uint32_t end = efuse_lock_map[i].start + efuse_lock_map[i].size;
		uint32_t lock =
			(lock_value >> efuse_lock_map[i].rlock_shift) & 0x3;

		if (addr >= start && addr < end && !lock)
			return 1;
	}

	if (addr < EFUSE_SECRET_START_OFFSET)
		return 1;

	if (addr >= EFUSE_SECRET_END_OFFSET)
		return 1;

	return 0;
}

int64_t plat_efuse_read_safe(uint32_t addr)
{
	if (addr >= EFUSE_SIZE)
		return -EFAULT;

	if (addr % 4 != 0)
		return -EFAULT;

	if (plat_efuse_is_not_secret(addr))
		return mmio_read_32(SEC_EFUSE_SHADOW_REG + addr);

	return -EFAULT;
}
#ifdef BL1_INIT_EFUSE
static void efuse_wait_for_ready(void)
{
	while (mmio_read_32(EFUSE_STATUS) & BIT_BUSY)
		;
}

static void efuse_power_on(uint32_t on)
{
	if (on)
		mmio_setbits_32(EFUSE_MODE, BIT_CMD);
	else
		mmio_setbits_32(EFUSE_MODE, BIT_PWR_DN | BIT_CMD);
}

static void efuse_refresh(void)
{
	mmio_write_32(EFUSE_MODE, CMD_REFRESH);
}

static void efuse_prog_bit(uint32_t word_addr, uint32_t bit_addr,
			   uint32_t high_row)
{
	uint32_t phy_addr;

	// word_addr: virtual addr, take "lower 6-bits" from 7-bits (0-127)
	// bit_addr: virtual addr, 5-bits (0-31)

	// composite physical addr[11:0] = [11:7]bit_addr + [6:0]word_addr
	phy_addr =
		((bit_addr & 0x1F) << 7) | ((word_addr & 0x3F) << 1) | high_row;

	efuse_wait_for_ready();

	// send efuse program cmd
	mmio_write_32(EFUSE_ADR, phy_addr);
	mmio_write_32(EFUSE_MODE, BIT_PRG | BIT_CMD);
}

static uint32_t efuse_read_word(uint32_t phy_word_addr,
				enum EFUSE_READ_TYPE type)
{
	// power on efuse macro
	efuse_power_on(1);

	efuse_wait_for_ready();

	mmio_write_32(EFUSE_ADR, phy_word_addr);

	if (type == AREAD) // array read
		mmio_write_32(EFUSE_MODE, BIT_AREAD | BIT_CMD);
	else if (type == MREAD) // margin read
		mmio_write_32(EFUSE_MODE, BIT_MREAD | BIT_CMD);
	else
		NOTICE("EFUSE: Unsupported read type!");

	efuse_wait_for_ready();
	return mmio_read_32(EFUSE_RD_DATA);
}

uint32_t plat_bm_efuse_read(uint32_t vir_word_addr)
{
	uint32_t j;

	for (j = 0; j < 2; j++) {
		// check by margin read
		// val = efuse_read_word((vir_word_addr << 1) | j, MREAD);
		NOTICE("EFUSE MREAD: Program bits %d check 0x%x\n",
			j, efuse_read_word((vir_word_addr << 1) | j, MREAD));
		// val = efuse_read_word((vir_word_addr << 1) | j, AREAD);
		NOTICE("EFUSE AREAD: Program bits %d check 0x%x\n",
			j, efuse_read_word((vir_word_addr << 1) | j, AREAD));
	}
	return 0;
}

int plat_bm_efuse_write(uint32_t vir_word_addr, uint32_t val)
{
	uint32_t i, j, row_val, zero_bit;
	uint32_t new_value;
	int err_cnt = 0;

	for (j = 0; j < 2; j++) {
		VERBOSE("EFUSE: Program physical word addr #%d\n",
			(vir_word_addr << 1) | j);

		// array read by word address
		row_val = efuse_read_word((vir_word_addr << 1) | j,
					  AREAD); // read low word of word_addr
		zero_bit = val & (~row_val); // only program zero bit

		// program row which bit is zero
		for (i = 0; i < 32; i++) {
			if ((zero_bit >> i) & 1)
				efuse_prog_bit(vir_word_addr, i, j);
		}

		// check by margin read
		new_value = efuse_read_word((vir_word_addr << 1) | j, MREAD);
		VERBOSE("%s(): val=0x%x new_value=0x%x\n", __func__, val,
			new_value);
		if ((val & new_value) != val) {
			err_cnt += 1;
			ERROR("EFUSE: Program bits check failed (%d)!\n",
			      err_cnt);
		}
	}

	efuse_refresh();

	return err_cnt >= 2 ? -EIO : 0;
}

void plat_bm_efuse_init(void)
{
	// power on efuse macro
	efuse_power_on(1);

	// send refresh cmd to reload all eFuse values to shadow registers
	efuse_refresh();

	// efuse macro will be auto powered off after refresh cmd, so don't
	// need to turn it off manually
}

/**
  * plat_efuse_write_safe() - Program eFuse only for the non-secret area
  * @addr:	eFuse address in byte addressing. Must be 4 bytes alignment
  * @value:	32 bits value
  *
  * Program eFuse only for the non-secret area
  *
  * @returns < 0 on failure
  */
int plat_efuse_write_safe(uint32_t addr, uint32_t value)
{
	int ret;
	int i;
	static struct efuse_area *a;

	VERBOSE("%s(): 0x%x = 0x%x\n", __func__, addr, value);

	if (addr >= EFUSE_SIZE)
		return -EFAULT;

	if (addr % 4 != 0)
		return -EFAULT;

	for (i = 0; i < ARRAY_SIZE(efuse_writable_areas); i++) {
		a = efuse_writable_areas + i;
		if (addr >= a->start && addr < (a->start + a->size)) {
#if 0
			// For the test purpose
			NOTICE("%s(): 0x%x = 0x%x\n", __func__, addr, value);
			ret = 0;
#else
			ret = plat_bm_efuse_write(addr / 4, value);
			VERBOSE("%s(): ret=%d\n", __func__, ret);
			plat_bm_efuse_init();
			efuse_wait_for_ready();
#endif
			return ret;
		}
	}

	return -EFAULT;
}

static void efuse_scan_valid_data(enum EFUSE_PATTERN idx)
{
	uint32_t i, val_a, val_d, pattern;
	int is_rom_patch_disabled = (mmio_read_32(EFUSE_SCS_CONFIG_REG) &
				     (0x3 << BIT_ROM_PATCH_DISABLE)) >>
				    BIT_ROM_PATCH_DISABLE;

	if (is_rom_patch_disabled) {
		if (idx == USERCONF) {
			ATF_STATE = ATF_STATE_BL1_APPLY_SW_CONF;
			INFO("Load userconf directly...\n");
			*(uint32_t *)PLAT_COMMON_INFO =
				mmio_read_32(EFUSE_PATCH_START);
			INFO("0x%x = 0x%x\n", PLAT_COMMON_INFO,
			     *(uint32_t *)PLAT_COMMON_INFO);
		}
		return;
	}

	if (idx == USERCONF) {
		ATF_STATE = ATF_STATE_BL1_APPLY_SW_CONF;
		INFO("Load userconf...\n");
		pattern = 0x2;
	} else {
		ATF_STATE = ATF_STATE_BL1_APPLY_EFUSE_PATCH;
		INFO("Load data...\n");
		pattern = 0x0;
	}

	for (i = 0; i < EFUSE_PATCH_SIZE; i += 8) {
		val_a = mmio_read_32(EFUSE_PATCH_START + i);
		INFO("addr 0x%x, val 0x%x\n", EFUSE_PATCH_START + i, val_a);

		// Get valid user conf
		if ((val_a != 0) && ((val_a & 0x1) == 0) &&
		    ((val_a & 0x2) == pattern)) {
			val_d = mmio_read_32(EFUSE_PATCH_START + i + 4);
			mmio_write_32(val_a & 0xFFFFFFFC, val_d);
			INFO("0x%x = 0x%x\n", val_a & 0xFFFFFFFC, val_d);
		} else {
			INFO("  -> skip addr 0x%x, val 0x%x\n",
			     EFUSE_PATCH_START + i, val_a);
		}
	}
}
#else
int plat_efuse_write_safe(uint32_t addr, uint32_t value)
{
	return -EFAULT;
}
#endif
void plat_init_user_conf(void)
{
#ifdef BL1_INIT_EFUSE
	efuse_scan_valid_data(USERCONF);
#endif
}

uint32_t plat_bm_get_uart_clock(void)
{
	return PLAT_UART_CLK_IN_HZ;
}

void plat_bm_init_i2c(void)
{
	printf("fix me: plat_bm_init_i2c\n");
}

void plat_bm_deinit_i2c(void)
{
	printf("fix me: plat_bm_deinit_i2c\n");
}

void do_wdt_reset(const char *file, unsigned int line, const char *assertion)
{
	mdelay(5000);

	ERROR("WDT RST ASSERT: %s, <%d>, %s\n", file, line, assertion);
	plat_wait_debug_port();
	console_flush();

	mmio_setbits_32(TOP_BASE + 0x8, 0x4); // wdt reset enable
	mmio_write_32(WATCHDOG_BASE + 0x4, 0x20); // Set timeout range reigster
	mmio_write_32(WATCHDOG_BASE + 0xc, 0x76); // Counter restart register
	mmio_write_32(WATCHDOG_BASE, 0x13); // Response mode and enable WDT

	while (1)
		;
}

void do_wdt_reset_direct(const char *file, unsigned int line,
			 const char *assertion)
{
	console_flush();

	mmio_setbits_32(TOP_BASE + 0x8, 0x4); // wdt reset enable
	mmio_write_32(WATCHDOG_BASE + 0x4, 0x20); // Set timeout range reigster
	mmio_write_32(WATCHDOG_BASE + 0xc, 0x76); // Counter restart register
	mmio_write_32(WATCHDOG_BASE, 0x13); // Response mode and enable WDT
}

int plat_ddr_firewall_set(int region_idx, uintptr_t start, uintptr_t end)
{
	uint32_t a;

	INFO("%s: region_idx=%d start=0x%lx end=0x%lx\n", __func__, region_idx,
	     start, end);

	mmio_write_32(SEC_DDR_FW_DIRTY_REGION,
		      SEC_DDR_FW_DIRTY_REGION_DEFAULT >> SEC_DDR_FW_ADDR_SHIFT);

	start &= 0x7fffffff;
	end &= 0x7fffffff;

	// Set secure dram region_idx
	mmio_write_32(SEC_DDR_FW_SECREGION_START_0 + 4 * region_idx,
		      (uint32_t)(start >> SEC_DDR_FW_ADDR_SHIFT));
	mmio_write_32(SEC_DDR_FW_SECREGION_END_0 + 4 * region_idx,
		      (uint32_t)(end >> SEC_DDR_FW_ADDR_SHIFT));

	// Allow all AID
	mmio_write_32(SEC_DDR_FW_AID_CTRL0, SEC_DDR_FW_AID_CTRL_EN_ALL);
	mmio_write_32(SEC_DDR_FW_AID_CTRL1, SEC_DDR_FW_AID_CTRL_EN_ALL);
	mmio_write_32(SEC_DDR_FW_AID_CTRL2, SEC_DDR_FW_AID_CTRL_EN_ALL);
	mmio_write_32(SEC_DDR_FW_AID_CTRL3, SEC_DDR_FW_AID_CTRL_EN_ALL);

	// Enable secure region
	a = mmio_read_32(SEC_DDR_FW_CTRL0);
	a |= SEC_DDR_FW_CTRL0_EN;
	a |= 1 << (SEC_DDR_FW_CTRL0_SECREGION_EN_SHIFT + region_idx);
	a |= SEC_DDR_FW_CTRL0_AID_CTRL_REGION_ALL
	     << SEC_DDR_FW_CTRL0_AID_CTRL_REGION_SHIFT;
	mmio_write_32(SEC_DDR_FW_CTRL0, a);

	return 0;
}

void bm_storage_boot_loader_version(uint32_t addr)
{
	int size_version, size_time;

	size_version = strlen(version_string);
	for (int i = 0; i < size_version; i++) {
		mmio_write_8(addr + i, version_string[i]);
	}

	mmio_write_8(addr + size_version, ' ');

	size_time = strlen(build_message);
	for (int i = 0; i < size_time; i++) {
		mmio_write_8(addr + size_version + 1 + i, build_message[i]);
	}

	mmio_write_8(addr + size_version + 1 + size_time, '\0');
}
