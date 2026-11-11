/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __CV_PRIVATE_H
#define __CV_PRIVATE_H

#include <sys/types.h>
#include "cryptodma.h"
#define GET_USB_BOOT_CFG(usb_boot_conf) (usb_boot_conf & 0x0000000F)
#define GET_USB_VID_VAL(usb_boot_conf) ((usb_boot_conf >> 4) & 0x0000FFFF)

typedef struct plat_conf_info_0_s {
	uint32_t dis_dbg_inject : 1; // bit[0]
	uint32_t dis_sd_upgrade : 1; // bit[1]
	uint32_t dis_usb_upgrade : 1; // bit[2]
	uint32_t dis_efuse_patch : 1; // bit[3]
	uint32_t usb_dl_t1 : 2; // bit[5:4]
	uint32_t usb_dl_t2 : 2; // bit[7:6]
	uint32_t usb_vid : 16; // bit[23:8]
	uint32_t dbg_i2c_addr : 2; // bit[25:24]
	uint32_t dis_hw_dec : 1; // bit[26]
	uint32_t dis_usb_rxf : 1; // bit[27]
	uint32_t dis_uart_log : 1; // bit[28]
	uint32_t force_device_memory : 1; // bit[29]
	uint32_t reserved : 2; // bit[31:30]
} plat_conf_info_0_t;

enum {
	FIP_SRC_SPI_NAND = 0x0,
	FIP_SRC_MEMMAP = 0x1,
	FIP_SRC_SPI_NOR = 0x2,
	FIP_SRC_EMMC = 0x3,
};

enum { // patch table index in SPI flash
	PT_USER_CONF = 0,
	PT_EMMC_PHY = 1,
	PT_SDIO_PHY = 2,
	PT_RESERVED_3,
	PT_RESERVED_4,
};

typedef struct _plat_cryptodma_sha256 {
	size_t total;
	uint32_t state[8];
} plat_cryptodma_sha256_t;

extern uint32_t system_counter_freq;
extern uint32_t gpio_value_override;
extern int keep_mmu_disabled;

void bm_configure_mmu_el1(unsigned long total_base, unsigned long total_size,
			  unsigned long ro_start, unsigned long ro_limit,
			  unsigned long coh_start, unsigned long coh_limit);
void bm_configure_mmu_el3(unsigned long total_base, unsigned long total_size,
			  unsigned long ro_start, unsigned long ro_limit,
			  unsigned long coh_start, unsigned long coh_limit);

void plat_bm_io_setup(void);
unsigned int plat_bm_calc_core_pos(u_register_t mpidr);
int64_t plat_efuse_read_safe(uint32_t addr);
int plat_efuse_write_safe(uint32_t addr, uint32_t value);
uint32_t plat_bm_efuse_read(uint32_t address);
uint32_t plat_bm_gpio_read(uint32_t mask);
void plat_cv_set_pinmux(int io_type);
void plat_init_user_conf(void);
void plat_init_efuse_patch(void);
int plat_bm_clock_init(void);
int plat_bm_sd_get_clk(void);
int plat_bm_emmc_get_clk(void);
uint32_t plat_bm_get_uart_clock(void);
void plat_bm_init_i2c(void);
void plat_bm_deinit_i2c(void);
void plat_usb_phy_setting(void);
void plat_bm_usb_polling(uint32_t fip_src);
void plat_bm_io_apply_patch(void);
void plat_bm_io_load_bld(void *buf);
int plat_bm_io_load_fip_from_sd(void *buf, uint32_t max_size);
int plat_cryptodma_des(uintptr_t src, uintptr_t des, uint64_t len,
		       unsigned char *key, unsigned char *iv, int algo);
int plat_cryptodma_sha256(const void *msg, size_t len, uint8_t digest[32]);
int plat_cryptodma_hmac_sha256(const void *msg, size_t len, const uint8_t *key,
			       size_t keylen, uint8_t mac[32]);
void bm_plat_reboot(void);
int read_fip_direct(uint32_t fip_idx, uint32_t offset, uint32_t size,
		    uintptr_t buf);
void bm_sdio_pad_setting(void);

void do_wdt_reset(const char *file, unsigned int line, const char *assertion);
void do_wdt_reset_direct(const char *file, unsigned int line,
			 const char *assertion);
#define rst_assert(e) ((e) ? (void)0 : do_wdt_reset(__FILE__, __LINE__, #e))

int plat_ddr_firewall_set(int region_idx, uintptr_t start, uintptr_t end);
void plat_wait_debug_port(void);

uint32_t get_mfr_secret_id(void);

enum algo {
	ALGO_BYPASS = 8,
	ALGO_AES = 9,
	ALGO_DES = 10,
	ALGO_SM4 = 11,
	ALGO_BASE64 = 13
};

enum mode { AES_ECB = 0, AES_CBC = 1, AES_CTR = 2, DES_DES = 3, DES_TDES = 4 };

enum key_mode { AES_128BIT = 4, AES_192BIT = 2, AES_256BIT = 1 };

enum action { DECRYPT = 0, ENCRYPTION = 1 };

typedef enum CRYPTODMA_ALGO_MODE {
	CRYPTODMA_ALGO_MODE_ECB,
	CRYPTODMA_ALGO_MODE_CBC,
	CRYPTODMA_ALGO_MODE_CTR,
	CRYPTODMA_ALGO_MODE_OFB,
} CRYPTODMA_ALGO_MODE_E;

typedef enum CRYPTODMA_KEY_SIZE {
	CRYPTODMA_KEY_SIZE_64BITS,
	CRYPTODMA_KEY_SIZE_128BITS,
	CRYPTODMA_KEY_SIZE_192BITS,
	CRYPTODMA_KEY_SIZE_256BITS,
} CRYPTODMA_KEY_SIZE_E;

typedef enum CRYPTODMA_ACTION {
	CRYPTODMA_ACTION_ENCRYPTION,
	CRYPTODMA_ACTION_DECRYPT,
} CRYPTODMA_ACTION_E;

typedef enum CRYPTODMA_KEY_SOURCE {
	CRYPTODMA_KEY_SOURCE_DESCRIPTOR,
	CRYPTODMA_KEY_SOURCE_OTP, // key from otp or efuse
} CRYPTODMA_KEY_SOURCE_E;

typedef struct cryptoDMA_ctrl {
	CRYPTODMA_KEY_SOURCE_E key_src;
	int is_end;
	uint64_t next_descriptor;
} cryptoDMA_ctrl_s;

typedef struct cryptoDMA_base64_action {
	CRYPTODMA_ACTION_E action;
} cryptoDMA_base64_action_s;

typedef struct cryptoDMA_aes_config {
	uintptr_t key;
	uintptr_t iv;
	CRYPTODMA_ALGO_MODE_E mode;
	CRYPTODMA_KEY_SIZE_E key_mode;
	CRYPTODMA_ACTION_E action;
	CRYPTODMA_KEY_SOURCE_E otp;
} cryptoDMA_aes_config_s;

typedef struct cryptoDMA_des_config {
	uintptr_t key;
	uintptr_t iv;
	CRYPTODMA_ALGO_MODE_E mode;
	CRYPTODMA_KEY_SIZE_E action;
} cryptoDMA_des_config_s;

typedef enum CRYPTODMA_SHA {
	CRYPTODMA_SHA_SHA1,
	CRYPTODMA_SHA_SHA256,
} CRYPTODMA_SHA_E;

typedef struct cryptoDMA_sha_config {
	CRYPTODMA_SHA_E algo;
	uint32_t state[8];
	unsigned char result[32];
} cryptoDMA_sha_config_s;

typedef struct cryptoDMA_sm3_config {
	uint32_t state[8];
} cryptoDMA_sm3_config_s;

typedef cryptoDMA_aes_config_s cryptoDMA_sm4_config_s;
typedef cryptoDMA_des_config_s cryptoDMA_tdes_config_s;

typedef struct _spacc_exec_config {
	enum algo algo;
} spacc_exec_config;

int plat_cryptodma_sm4(uintptr_t src, uintptr_t dst, uint32_t len,
		       cryptoDMA_sm4_config_s *config);

int plat_cryptodma_aes(uintptr_t src, uintptr_t dst, uint64_t len,
		       cryptoDMA_aes_config_s *config);

int plat_cryptodma_sha(uintptr_t src, uintptr_t dst, uint32_t len,
		       cryptoDMA_sha_config_s *config);

int plat_cryptodma_tdes(uintptr_t src, uintptr_t dst, uint32_t len,
			cryptoDMA_des_config_s *config, int is_tdes);

int plat_cryptodma_exec(uintptr_t src, uintptr_t dst, uint64_t len,
			spacc_exec_config *config);

int plat_cryptodma_base64(uintptr_t src, uint64_t len, uintptr_t dst,
			  uint32_t customer_code, uint32_t action);

int plat_cryptodma_do(int isEncrypt, uintptr_t in, uintptr_t out, uint64_t len,
              unsigned char *key, E_KEY_MODE keyMode, unsigned char *iv,
              E_ALGO a, E_MODE b, uint32_t *state,E_OTP_KEY otp);

int plat_cryptodma_sha256(const void *msg, uint64_t len, uint8_t digest[32]);

void bm_storage_boot_loader_version(uint32_t addr);

#endif /*__CV_PRIVATE_H*/
