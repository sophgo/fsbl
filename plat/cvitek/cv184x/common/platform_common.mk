#
# Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ENABLE_COMPRESSION:=1
NANDBOOT_V2:=1
BL1_SEPARATE_CODE_AND_RODATA:=1
ARM_CRYPTOCELL_INTEG:=0
MBEDTLS_DIR:=third_party/mbedtls-mbedtls-2.12.0
CRYPTO_LIB_MK := drivers/auth/mbedtls/mbedtls_crypto.mk
$(info Including ${CRYPTO_LIB_MK})
include ${CRYPTO_LIB_MK}

TEST_FROM_SPINOR1 := 0

DEFINES += -DLZ4_USER_MEMORY_FUNCTIONS=1

ifeq ($(FSBL_SECURE_BOOT_SUPPORT),1)
DEFINES += \
	-DNO_ALLOCS \
	-DARGTYPE=3 \
	-DLTC_NO_FILE \
	-DLTM_NO_FILE \
	-DLTM_DESC \
	-DLTC_SOURCE

CRYPT_INCLUDES := \
	-Ilib/libtommath \
	-Ilib/libtomcrypt/src/headers \
	-Ilib/BigDigits

CRYPT_SOURCES := \
	lib/BigDigits/bigdigits.c \
	lib/libtomcrypt/src/hashes/sha2/sha256.c
endif

include lib/stdlib_v2/stdlib_v2.mk
include bl2/bl2.mk

PLAT_INCLUDES		:=	-Iinclude/plat/arm/common/		\
				-Ibuild \
				-Iinclude/plat/arm/common/aarch64/	\
				-Iinclude/drivers/emmc		\
				-Iinclude/drivers/usb		\
				-Iinclude/drivers/spinor	\
				-Iinclude/drivers/i2c		\
				-Iinclude/drivers/ddr		\
				-Iinclude/drivers/spinand		\
				-Iinclude/common/tbbr			\
				-Ilib/crc				\
				-Iplat/cvitek/${CHIP_ARCH}/common/include	\
				-Iplat/cvitek/${CHIP_ARCH}/${SUBTYPE}/include

PLAT_BL_COMMON_SOURCES	:=	\
				plat/cvitek/${CHIP_ARCH}/common/board_cv_trusted_boot.c	\
				drivers/uart/uart_dw.c		\
				lib/xlat_tables/xlat_tables_common.c		\
				lib/xlat_tables/aarch64/xlat_tables.c

CV_LF_EK            = plat/cvitek/$(CHIP_ARCH)/common/keys/cv_license_file_key.bin
    $(eval $(call add_define_val,CV_LF_EK,'"$(CV_LF_EK)"'))

# Disable the PSCI platform compatibility layer
ENABLE_PLAT_COMPAT	:= 	0

# Enable workarounds for selected Cortex-A53 errata
ERRATA_A53_835769	:=	1
ERRATA_A53_843419	:=	1
ERRATA_A53_855873	:=	1
