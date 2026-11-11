#
# Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Include the CPU specific operations makefile
include lib/stdlib_v2/stdlib_v2.mk

BLDS_INCLUDES += \
		 -Ibuild/ \
		 ${STDLIB_INCLUDES_V2} \
		 -Iinclude/ \
		 -Iinclude/arch \
		 -Iinclude/arch/${ARCH} \
		 -Iinclude/lib \
		 -Iinclude/drivers \
		 -Iinclude/drivers/console \
		 -Iinclude/drivers/ddr \
		 -Iinclude/drivers/ddr/ddr_config/${DDR_CFG} \
		 -Iinclude/common \
		 -Iinclude/bl2 \
		 -Iblds/include/ \
		 -Iplat/cvitek/${CHIP_ARCH}/${SUBTYPE}/include \
		 -Iplat/cvitek/${CHIP_ARCH}/common/include

BLDS_SOURCES = \
	       drivers/delay_timer/${ARCH}/delay_timer.c \
	       drivers/ddr/ddr_suspend.c \
	       drivers/uart/uart_dw.c \
	       blds/blds_main.c

BLDS_LINKERFILE := blds/blds.ld.S

################################################################################
# BLDS Compilation Options
################################################################################
BLDS_CPPFLAGS += \
	${DEFINES} ${BLDS_INCLUDES} \
	-nostdinc \
	-Wmissing-include-dirs -Werror

BLds_CFLAGS += -ggdb3 -gdwarf-2 -fno-stack-protector -Os \
	       ${BLDS_CPPFLAGS} -DIMAGE_BL2

BLDS_ASFLAGS += -g -Wa,--gdwarf-2
