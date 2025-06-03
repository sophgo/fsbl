#include <arch_helpers.h>
#include <bl_common.h>
#include <cvsp.h>
#include <debug.h>
#include <delay_timer.h>
#include <generic_delay_timer.h>
#include <optee_msg.h>
#include <optee_smc.h>
#include <opteed_private.h>
#include <platform.h>
#include <platform_cvsp.h>
#include <platform_def.h>
#include <spinlock.h>
#include <string.h>
#include <teesmc_opteed.h>
#include <teesmc_opteed_macros.h>
#include <mmio.h>
#include <errno.h>

#include "cv_private.h"
#include "cv_security.h"
#include "cv_efuse.h"
#include "cvsp_private.h"
#include "tpu_security.h"
#include "license_file.h"
#include "scheme-private.h"

#include "mbedtls/mbedtls_common.h"
#include "mbedtls/platform.h"
#include "mbedtls/aes.h"
#include "mbedtls/sha256.h"
#include <endian.h>

#define u64 uint64_t

static uint32_t global_time0;

bmsp_args_t *cvsp_privte_efuse_read_handler(uint64_t func, uint64_t arg1,
					    uint64_t arg2, uint64_t arg3,
					    uint64_t arg4, uint64_t arg5,
					    uint64_t arg6, uint64_t arg7)
{
	uint32_t addr = arg1;
	uint32_t size = arg2;
	void *buf = (void *)arg3;

	int64_t ret = -1;
	int i;

	VERBOSE("%s(): 0x%x(%u) to %p\n", __func__, addr, size, buf);

	if (size > EFUSE_SIZE)
		size = EFUSE_SIZE;

	inv_dcache_range((uintptr_t)buf, size);

	for (i = 0; i < size; i += 4) {
		ret = plat_efuse_read_safe(addr + i);
		VERBOSE("%s(): i=%x ret=%lx\n", __func__, i, ret);
		*(uint32_t *)(buf + i) = (ret >= 0) ? ret : 0;
	}

	flush_dcache_range((uintptr_t)buf, size);

	return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, size, 0, 0, 0,
				 0, 0, 0);
}

bmsp_args_t *cvsp_privte_efuse_write_handler(uint64_t func, uint64_t arg1,
					     uint64_t arg2, uint64_t arg3,
					     uint64_t arg4, uint64_t arg5,
					     uint64_t arg6, uint64_t arg7)
{
	uint32_t ret = -1;
	uint32_t addr = arg1;
	uint32_t value = arg2;

	ret = plat_efuse_write_safe(addr, value);

	return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, ret, 0, 0, 0,
				 0, 0, 0);
}



bmsp_args_t *cvsp_privte_trng_init_handler(uint64_t func, uint64_t arg1,
					   uint64_t arg2, uint64_t arg3,
					   uint64_t arg4, uint64_t arg5,
					   uint64_t arg6, uint64_t arg7)
{
	uint32_t data;

	// wait idle
	do {
		data = mmio_read_32(0x0207000C);
	} while ((data & 0x80000000) == 0x80000000);

	// start seed generation
	mmio_write_32(0x02070000, 0x1);

	do {
		data = mmio_read_32(0x02070014);
	} while (!(data & 0x10));

	// ack and clear the done bit flag
	mmio_write_32(0x02070014, 0x10);

	// create state;
	mmio_write_32(0x02070000, 0x3);

	do {
		data = mmio_read_32(0x02070014);
	} while (!(data & 0x10));

	// ack and clear the done bit flag
	mmio_write_32(0x02070014, 0x10);

	return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, 0, 0, 0, 0, 0,
				 0, 0);
}

bmsp_args_t *cvsp_privte_trng_read_handler(uint64_t func, uint64_t arg1,
					   uint64_t arg2, uint64_t arg3,
					   uint64_t arg4, uint64_t arg5,
					   uint64_t arg6, uint64_t arg7)
{
	uint32_t data;

	mmio_write_32(0x02070000, 0x6);

	do {
		data = mmio_read_32(0x02070014);
	} while (!(data & 0x10));

	// ack and clear the done bit flag
	mmio_write_32(0x02070014, 0x10);

	uint32_t value1 = mmio_read_32(0x02070024);
	uint32_t value2 = mmio_read_32(0x02070028);
	uint32_t value3 = mmio_read_32(0x0207002C);
	uint32_t value4 = mmio_read_32(0x02070030);

	return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, value1, value2,
				 value3, value4, 0, 0, 0);
}
static void hexdump(const char *label, const void *buf, size_t len)
{
#ifdef DEBUG
	size_t i;
	uint8_t *data = (uint8_t *)buf;
	NOTICE("%s (%zu bytes):", label, len);
	for (i = 0; i < len; i++) {
		// NOTICE(" %08X: ", (unsigned int)(i));
		NOTICE("%x", data[i]);
		if ((i % 16 == 15) || (i == len - 1))
			NOTICE("\n");
	}
#endif
}
bmsp_args_t *cvsp_privte_spacc_exec_handler(uint64_t func, uint64_t arg1,
					    uint64_t arg2, uint64_t arg3,
					    uint64_t arg4, uint64_t arg5,
					    uint64_t arg6, uint64_t arg7)
{
	int ret = 0;
	return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, 0, 0, 0, 0, 0,
				 0, ret);
}

// AES函数改造
bmsp_args_t *cvsp_privte_aes_handler(uint64_t func, uint64_t arg1,
                     uint64_t arg2, uint64_t arg3,
                     uint64_t arg4, uint64_t arg5,
                     uint64_t arg6, uint64_t arg7)
{
    void *src = (void *)arg1;
    void *dst = (void *)arg2;
    u64 len = arg3;
    void *key = (void *)arg4;
    void *iv = (void *)arg5;
    uint64_t key_len = arg6;
    uint32_t state[8] = {0};
    u64 ret;

    // 参数检查和缓存操作
    inv_dcache_range((uintptr_t)src, len);
    inv_dcache_range((uintptr_t)key, key_len);
    inv_dcache_range((uintptr_t)iv, 16);

    // 从arg7解析配置
    E_MODE mode = (arg7) & 0x3;
    E_KEY_MODE key_mode = (arg7 >> 2) & 0x3;
    int isEncrypt = ((arg7 >> 4) & 0x1)==0?1:0;

    // 调用新接口
    ret = plat_cryptodma_do(isEncrypt, (uintptr_t)src, (uintptr_t)dst, len,
                           key, key_mode, iv, AES, mode, state,USE_DES_KEY);

    return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, len, 0, 0, 0, 0, 0, ret);
}

// SM3接口修改
bmsp_args_t *cvsp_privte_sm3_handler(uint64_t func, uint64_t arg1,
                     uint64_t arg2, uint64_t arg3,
                     uint64_t arg4, uint64_t arg5,
                     uint64_t arg6, uint64_t arg7)
{
    void *src = (void *)arg1;
    u64 len = arg2;
    void *dst = (void *)arg3;
    uint32_t state[8] = {0};
    int ret;

    inv_dcache_range((uintptr_t)src, len);
	NOTICE("%s(): src=%p, len=%lu, dst=%p\n",
	       __func__, src, len, dst);
	state[0] = 0x7380166f;
	state[1] = 0x4914b2b9;
	state[2] = 0x172442d7;
	state[3] = 0xda8a0600;
	state[4] = 0xa96f30bc;
	state[5] = 0x163138aa;
	state[6] = 0xe38dee4d;
	state[7] = 0xb0fb0e4e;
    ret = plat_cryptodma_do(0, (uintptr_t)src, (uintptr_t)dst, len,
                           NULL, KEY_128BITS, NULL, SM3, ECB, state,USE_DES_KEY);


    memcpy(dst, state, 32);
    flush_dcache_range((uintptr_t)dst, 32);
    return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, ret, 0, 0, 0, 0, 0, 0);
}

// SM4接口修改
bmsp_args_t *cvsp_privte_sm4_handler(u64 func, u64 arg1, u64 arg2, u64 arg3,
                     u64 arg4, u64 arg5, u64 arg6, u64 arg7)
{
    void *src = (void *)arg1;
    void *dst = (void *)arg2;
    u64 len = arg3;
    void *key = (void *)arg4;
    void *iv = (void *)arg5;
    uint32_t state[8] = {0};
    
    inv_dcache_range((uintptr_t)src, len);
    inv_dcache_range((uintptr_t)key, 16);  // SM4固定使用128位密钥
    inv_dcache_range((uintptr_t)iv, 16);

    E_MODE mode = (arg7) & 0x3;
    int isEncrypt = ((arg7 >> 4) & 0x1)==0?1:0;
	NOTICE("%s(): src=%p, len=%lu, dst=%p, key=%p, iv=%p\n",
	       __func__, src, len, dst, key, iv);
    int ret = plat_cryptodma_do(isEncrypt, (uintptr_t)src, (uintptr_t)dst, len,
                           key, KEY_128BITS, iv, SM4, mode, state,USE_DES_KEY);

    if (ret == 0) {
        flush_dcache_range((uintptr_t)dst, len);
    }

    return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, ret, 0, 0, 0, 0, 0, 0);
}

// TDES函数改造
bmsp_args_t *cvsp_privte_tdes_handler(u64 func, u64 arg1, u64 arg2, u64 arg3,
                      u64 arg4, u64 arg5, u64 arg6, u64 arg7)
{
    void *src = (void *)arg1;
    u64 len = arg2;
    void *dst = (void *)arg3;
    void *key = (void *)arg4;
    void *iv = (void *)arg5;
    uint32_t state[8] = {0};
    int ret;
	
    inv_dcache_range((uintptr_t)key, 24);
    inv_dcache_range((uintptr_t)iv, 8);
	NOTICE("%s(): src=%p, len=%lu, dst=%p, key=%p, iv=%p\n",
	       __func__, src, len, dst, key, iv);
    // arg6为mode，arg7为action(encrypt/decrypt)
    E_MODE mode = arg6;
    int isEncrypt = arg7==0?1:0;
	
    ret = plat_cryptodma_do(isEncrypt, (uintptr_t)src, (uintptr_t)dst, len,
                           key, KEY_192BITS, iv, TDES, mode, state,USE_DES_KEY);

    flush_dcache_range((uintptr_t)dst, len);

    return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, ret, 0, 0, 0, 0, 0, 0);
}
bmsp_args_t *cvsp_privte_des_handler(u64 func, u64 arg1, u64 arg2, u64 arg3,
                      u64 arg4, u64 arg5, u64 arg6, u64 arg7)
{
    void *src = (void *)arg1;
    u64 len = arg2;
    void *dst = (void *)arg3;
    void *key = (void *)arg4;
    void *iv = (void *)arg5;
    uint32_t state[8] = {0};
    int ret;
	
    inv_dcache_range((uintptr_t)key, 24);
    inv_dcache_range((uintptr_t)iv, 8);
	NOTICE("%s(): src=%p, len=%lu, dst=%p, key=%p, iv=%p\n",
	       __func__, src, len, dst, key, iv);
    // arg6为mode，arg7为action(encrypt/decrypt)
    E_MODE mode = arg6;
    int isEncrypt = arg7==0?1:0;
	
    ret = plat_cryptodma_do(isEncrypt, (uintptr_t)src, (uintptr_t)dst, len,
                           key, KEY_128BITS, iv, DES, mode, state,USE_DES_KEY);

    flush_dcache_range((uintptr_t)dst, len);
    return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, ret, 0, 0, 0, 0, 0, 0);
}
// SHA256函数改造
bmsp_args_t *cvsp_privte_sha256_handler(u64 func, u64 arg1, u64 arg2, u64 arg3,
                    u64 arg4, u64 arg5, u64 arg6, u64 arg7)
{
    void *src = (void *)arg1;
    u64 len = arg2;
    void *dst = (void *)arg3;
    uint32_t state[8] = {0};
    int ret;
	state[0] = htobe32(0x6A09E667);
	state[1] = htobe32(0xBB67AE85);
	state[2] = htobe32(0x3C6EF372);
	state[3] = htobe32(0xA54FF53A);
	state[4] = htobe32(0x510E527F);
	state[5] = htobe32(0x9B05688C);
	state[6] = htobe32(0x1F83D9AB);
	state[7] = htobe32(0x5BE0CD19);
    inv_dcache_range((uintptr_t)src, len);
	NOTICE("SHA256: src=%p, len=%lu, dst=%p\n",
	       src, len, dst);
    ret = plat_cryptodma_do(0, (uintptr_t)src, (uintptr_t)dst, len,
                           NULL, KEY_128BITS, NULL, SHA256, ECB, state,USE_DES_KEY);

    // 复制结果到目标地址
    memcpy(dst, state, 32);
    flush_dcache_range((uintptr_t)dst, 32);
#ifdef DEBUG
	for(int i=0;i<32;i++){
		NOTICE("dst[%d] = %x\n", i, ((uint8_t *)dst)[i]);
	}
#endif
    return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, ret, 0, 0, 0, 0, 0, 0);
}
bmsp_args_t *cvsp_privte_base64_handler(u64 func, u64 arg1, u64 arg2, u64 arg3,
					u64 arg4, u64 arg5, u64 arg6, u64 arg7)
{
	void *src = (void *)arg1;
	u64 len = arg2;
	void *dst = (void *)arg3;
	u64 customer_code = arg4;
	u64 action = arg5;
	NOTICE("%s(): src=%p, len=%lu, dst=%p, customer_code=%lu, action=%lu\n",
	       __func__, src, len, dst, customer_code, action);
	inv_dcache_range((uintptr_t)src, len);
	inv_dcache_range((uintptr_t)dst, len);

	int ret = plat_cryptodma_base64((uintptr_t)src, len, (uintptr_t)dst,
					customer_code, action);

	hexdump("result", dst, ret);
	return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, ret, 0, 0, 0,
				 0, 0, 0);
}
static int cvsp_privte_smc_handler_init(void)
{
	static int ready;

	if (IS_IN_EL1()) {
		write_cpacr_el1(read_cpacr_el1() |
				CPACR_EL1_FPEN(CPACR_EL1_FP_TRAP_EL0));
	}

	if (!ready) {
		NOTICE("%s()\n", __func__);

		mbedtls_init();
		generic_delay_timer_init();

		ready = 1;
	}

	global_time0 = get_timer(0);
#if 0
	// Check eFuse clk
	if (!(mmio_read_32(REG_CLK_ENABLE_REG0) &
	      REG_CLK_ENABLE_REG0_CLK_EFUSE)) {
		ERROR("clk_eFuse is not enabled\n");
		return -1;
	}
#endif
	return 0;
}

static void cvsp_private_smc_handler_cleanup_fn(int *p)
{
	p = p;

	// Disable SPACC/TRNG clock at Mars3 platform
	mmio_setbits_32(0x20b0000, 0x3);
}

bmsp_args_t *cvsp_private_smc_handler(uint64_t func, uint64_t arg1,
				      uint64_t arg2, uint64_t arg3,
				      uint64_t arg4, uint64_t arg5,
				      uint64_t arg6, uint64_t arg7)
{
	int cleanup
		__attribute__((cleanup(cvsp_private_smc_handler_cleanup_fn))) =
			0;

	// uint32_t linear_id = plat_my_core_pos();
	int ret = 0;

	NOTICE("%s(): linear_id=%u [0x%lx 0x%lx]\n", __func__, plat_my_core_pos(), func,
	       arg1);

	// Enable SPACC/TRNG clock at Mars3 platform
	mmio_clrbits_32(0x20b0000, 0x3);
	ret = cvsp_privte_smc_handler_init();
	if (ret < 0) {
		return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, ret, 0,
					 0, 0, 0, 0, 0);
	}

	switch (func) {
	case OPTEE_SMC_CALL_CV_DEBUG:
		return cvsp_smc_call_debug_handler(func, arg1, arg2, arg3, arg4,
						   arg5, arg6, arg7);

#if defined(ENABLE_TPU_SECURITY) && ENABLE_TPU_SECURITY
	case OPTEE_SMC_CALL_CV_TPU_SEND_KEYS:
		return cvsp_privte_tpu_send_keys_handler(
			func, arg1, arg2, arg3, arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_TPU_LOAD_MODEL:
		return cvsp_privte_tpu_load_model_handler(
			func, arg1, arg2, arg3, arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_TPU_RUN:
		return cvsp_private_tpu_run_handler(func, arg1, arg2, arg3,
						    arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_TPU_WAIT:
		return cvsp_privte_tpu_wait_handler(func, arg1, arg2, arg3,
						    arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_TPU_SET_PROT_RANGE:
		return cvsp_privte_tpu_set_prot_range_handler(
			func, arg1, arg2, arg3, arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_TPU_CLEAR_PROT_RANGE:
		return cvsp_privte_tpu_clear_prot_range_handler(
			func, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
#endif /* ENABLE_TPU_SECURITY */

	case OPTEE_SMC_CALL_CV_EFUSE_READ:
		return cvsp_privte_efuse_read_handler(func, arg1, arg2, arg3,
						      arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_EFUSE_WRITE:
		return cvsp_privte_efuse_write_handler(func, arg1, arg2, arg3,
						       arg4, arg5, arg6, arg7);
	case OPTEE_SMC_CALL_CV_SPACC_EXEC:
		return cvsp_privte_spacc_exec_handler(func, arg1, arg2, arg3,
						      arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_TRNG_INIT:
		return cvsp_privte_trng_init_handler(func, arg1, arg2, arg3,
						     arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_TRNG_READ:
		return cvsp_privte_trng_read_handler(func, arg1, arg2, arg3,
						     arg4, arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_BASE64:
		return cvsp_privte_base64_handler(func, arg1, arg2, arg3, arg4,
						  arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_SHA256:
		return cvsp_privte_sha256_handler(func, arg1, arg2, arg3, arg4,
						  arg5, arg6, arg7);

	case OPTEE_SMC_CALL_CV_AES:
		return cvsp_privte_aes_handler(func, arg1, arg2, arg3, arg4,
					       arg5, arg6, arg7);
	case OPTEE_SMC_CALL_CV_SM4:
		return cvsp_privte_sm4_handler(func, arg1, arg2, arg3, arg4,
					       arg5, arg6, arg7);
	case OPTEE_SMC_CALL_CV_TDES:
		return cvsp_privte_tdes_handler(func, arg1, arg2, arg3, arg4,
						arg5, arg6, arg7);
	case OPTEE_SMC_CALL_CV_DES:
		return cvsp_privte_des_handler(func, arg1, arg2, arg3, arg4,
						arg5, arg6, arg7);
	case OPTEE_SMC_CALL_CV_SM3:
		return cvsp_privte_sm3_handler(func, arg1, arg2, arg3, arg4,
						arg5, arg6, arg7);
	default:
		ERROR("Unknown func id (0x%lx)\n", func);
		break;
	}

	return bmsp_set_smc_args(TEESMC_OPTEED_RETURN_CALL_DONE, 0, 0, 0, 0, 0,
				 0, 0);
}

unsigned int plat_get_syscnt_freq2(void)
{
	return SYS_COUNTER_FREQ_IN_TICKS;
}

int __trace(const char *prefix, const char *path, const char *func, int lineno,
	    const char *fmt, ...)
{
	char stmp[256], *x = stmp;

	va_list ap;
	int ret = 0;

	x += snprintf(x, _SIZE_67A9DC, "[%s%s:%s:%d] ", prefix, path, func,
		      lineno);

	if (!fmt || fmt[0] == '\0') {
		x += snprintf(x, _SIZE_67A9DC, "\n");
	} else {
		va_start(ap, fmt);
		x += vsnprintf(x, _SIZE_67A9DC, fmt, ap);
		va_end(ap);
	}

	_PRINTF_38A01C("%s", stmp);

	return ret;
}
