.PHONY: FORCE
# Default goal is build all images
.DEFAULT_GOAL := all

# Avoid any implicit propagation of command line variable definitions to
# sub-Makefiles, like CFLAGS that we reserved for the firmware images'
# usage. Other command line options like "-s" are still propagated as usual.
MAKEOVERRIDES =

RELEASE_VER := 1

include $(BUILD_PATH)/.config

S := $(shell printf '\033[1;34;40m  MAKECMDGOALS %s \033[0m\n' '${MAKECMDGOALS}')
$(info ${S})

ARCH ?=

ifneq ($(origin CROSS_COMPILE),command line)
ifeq ($(ARCH),riscv)
CROSS_COMPILE := ${CROSS_COMPILE_GLIBC_RISCV64}
BOOT_CPU ?= riscv
else
CROSS_COMPILE := ${CROSS_COMPILE_64}
BOOT_CPU := aarch64
ARCH := aarch64
endif
endif

ifeq (${CHIP_ARCH},)
$(error CHIP_ARCH is undefined)
endif

ifeq (${CROSS_COMPILE},)
$(error CROSS_COMPILE is undefined)
endif


DEFINES += -DCHIP=$(CHIP)
################################################################################
# Default values for build configurations, and their dependencies
################################################################################
MAKE_HELPERS_DIRECTORY := make_helpers/

V ?= 1
DEBUG := 0
LOG_LEVEL := 2
ENABLE_ASSERTIONS := 1
PRINTF_TIMESTAMP := 0

NANDBOOT_V2 := 1

# Verbose flag
ifeq (${V},0)
        Q:=@
else
        Q:=
endif
export Q

PRINTABLE	:= abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
PRINTABLE_TR	:= v1JfTUIeqdsE7P0oiczuVbW9aLnOFySG5YtDQ3lHN4rRpZABwmkCg2XhjM6x8K
CHIP_ARCH	:= $(shell echo '${CHIP_ARCH}' | tr A-Z a-z)
CHIP_ARCH_ALT	:= $(shell echo '${CHIP_ARCH}' | tr '${PRINTABLE}' '${PRINTABLE_TR}')
BUILD_BASE	:= ./build

O ?= ${BUILD_BASE}/${CHIP_ARCH}
BUILD_PLAT	= ${O}

BUILD_STRING := g$(shell git rev-parse --short HEAD 2> /dev/null)
BUILD_STRING := ${BUILD_STRING}$(shell if git diff-index --name-only HEAD | grep -q "."; then echo -dirty; fi)
VERSION_STRING := ${CHIP_ARCH_ALT}:${BUILD_STRING}

ifeq ($(CHIP_ARCH),$(filter $(CHIP_ARCH),cv183x ))
DEFINES += -D__CVITEK__
# TPU model decryption
ENABLE_TPU_SECURITY:=1
BOOTFLOW_TYPE:=0
else ifeq ($(CHIP_ARCH),$(filter $(CHIP_ARCH),cv182x  cv181x athena2))
DEFINES += -D__CVITEK__
ENABLE_TPU_SECURITY:=0
BOOTFLOW_TYPE:=1
endif

ifeq (${CRC}, 1)
	DEFINES += -DADD_CRC
endif

# Add BLD to fip.bin
IMG_BLD?=0
ifeq (${IMG_BLD}, 1)
	ADD_BLD := yes
	DEFINES += -DLOAD_BLD
endif



################################################################################
# Toolchain
################################################################################
HOSTCC			:=	gcc
export HOSTCC

CC			:=	${CROSS_COMPILE}gcc
CPP			:=	${CROSS_COMPILE}cpp
AS			:=	${CROSS_COMPILE}gcc
AR			:=	${CROSS_COMPILE}ar
LD			:=	${CROSS_COMPILE}ld
OC			:=	${CROSS_COMPILE}objcopy
OD			:=	${CROSS_COMPILE}objdump
NM			:=	${CROSS_COMPILE}nm
PP			:=	${CROSS_COMPILE}gcc -E
READELF			:=	${CROSS_COMPILE}readelf
GDB			:=	${CROSS_COMPILE}gdb

################################################################################
# Generic definitions
################################################################################
include ${MAKE_HELPERS_DIRECTORY}unix.mk
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk
include ${MAKE_HELPERS_DIRECTORY}defaults.mk
################################################################################
# Common sources and include directories
################################################################################
include lib/stdlib/stdlib.mk

#INCLUDES		+= -Iinclude/bl31				\
				-Iinclude/common			\
				-Iinclude/common/${ARCH}		\
				-Iinclude/drivers			\
				-Iinclude/drivers/arm			\
				-Iinclude/drivers/auth			\
				-Iinclude/drivers/io			\
				-Iinclude/drivers/ti/uart		\
				-Iinclude/lib				\
				-Iinclude/lib/${ARCH}			\
				-Iinclude/lib/cpus			\
				-Iinclude/lib/cpus/${ARCH}		\
				-Iinclude/lib/el3_runtime		\
				-Iinclude/lib/el3_runtime/${ARCH}	\
				-Iinclude/lib/pmf			\
				-Iinclude/lib/psci			\
				-Iinclude/lib/xlat_tables		\
				-Iinclude/plat/common			\
				-Iinclude/services			\
				-Ilib/lzma                  \
				-Ilib/lz4                   \
				${PLAT_INCLUDES}			\
				${SPD_INCLUDES}				\
				-Iinclude/tools_share       

################################################################################
# Generic definitions
################################################################################

include ${MAKE_HELPERS_DIRECTORY}plat_helpers.mk

PLAT                    :=      ${CHIP}_${SUBTYPE}

SPDS			:=	$(sort $(filter-out none, $(patsubst services/spd/%,%,$(wildcard services/spd/*))))

# Platforms providing their own TBB makefile may override this value
INCLUDE_TBBR_MK		:=	1               


# Platform compatibility is not supported in AArch32
ifeq (${BOOT_CPU},aarch64)
# If the platform has not defined ENABLE_PLAT_COMPAT, then enable it by default
#ifndef ENABLE_PLAT_COMPAT
#ENABLE_PLAT_COMPAT := 1
#endif
#ifeq (${RELEASE_VER},0)
#include plat/cvitek/$(CHIP_ARCH)/asic/platform.mk
#endif
# Include the platform compatibility helpers for PSCI
#ifneq (${ENABLE_PLAT_COMPAT}, 0)
#include plat/compat/plat_compat.mk
#endif
# Enable workarounds for selected Cortex-A53 errata
ERRATA_A53_835769   :=  1
ERRATA_A53_843419   :=  1
ERRATA_A53_855873   :=  1
endif




################################################################################
# Include libraries' Makefile that are used in all BL
################################################################################

include lib/stack_protector/stack_protector.mk


# Include the CPU specific operations makefile, which provides default
# values for all CPU errata workarounds and CPU specific optimisations.
# This can be overridden by the platform.

include lib/cpus/cpu-ops.mk
#ifeq (${RELEASE_VER},0)
#ifeq (${ARCH},aarch32)
#NEED_BL32 := yes
################################################################################
# Build `AARCH32_SP` as BL32 image for AArch32
################################################################################
#ifneq (${AARCH32_SP},none)
# We expect to locate an sp.mk under the specified AARCH32_SP directory
#AARCH32_SP_MAKE	:=	$(wildcard bl32/${AARCH32_SP}/${AARCH32_SP}.mk)

#ifeq (${AARCH32_SP_MAKE},)
#  $(error Error: No bl32/${AARCH32_SP}/${AARCH32_SP}.mk located)
#endif

#$(info Including ${AARCH32_SP_MAKE})
#include ${AARCH32_SP_MAKE}
#endif

#endif
#endif


# Process TBB related flags
ifneq (${GENERATE_COT},0)
        FIP_DEPS += certificates

        # Common cert_create options
        ifneq (${CREATE_KEYS},0)
                $(eval CRT_ARGS += -n)
                $(eval FWU_CRT_ARGS += -n)
                ifneq (${SAVE_KEYS},0)
                        $(eval CRT_ARGS += -k)
                        $(eval FWU_CRT_ARGS += -k)
                endif
        endif
        # Include TBBR makefile (unless the platform indicates otherwise)
        ifeq (${INCLUDE_TBBR_MK},1)
                include make_helpers/tbbr/tbbr_tools.mk
        endif
endif

ifneq (${FIP_ALIGN},0)
FIP_ARGS += --align ${FIP_ALIGN}
endif

################################################################################
# Auxiliary tools (fiptool, cert_create, etc)
################################################################################

# Variables for use with Certificate Generation Tool
CRTTOOLPATH		?=	tools/cert_create
CRTTOOL			?=	${CRTTOOLPATH}/cert_create${BIN_EXT}


################################################################################
# Convert building option
################################################################################
FSBL_SECURE_BOOT_SUPPORT := $(call yn10,${FSBL_SECURE_BOOT_SUPPORT})

################################################################################
# CPU and platform
################################################################################
$(call print_var,CHIP_ARCH)
$(call print_var,BOOT_CPU)

include ${PLAT_MAKEFILE_FULL}

# Check CPU define
$(eval $(call add_define,$(shell echo '${BOOT_CPU}' | tr a-z A-Z)))
ifeq (${BOOT_CPU},aarch64)

else ifeq (${BOOT_CPU},riscv)
else
$(error "BOOT_CPU=${BOOT_CPU} is not supported")
endif

################################################################################
# BL2 Compilation Options
################################################################################
BL2_CPPFLAGS += \
	${DEFINES} ${BL2_INCLUDES} \
	-nostdinc \
	-Wmissing-include-dirs -Werror

################################################################################
# BL31 Compilation Options
################################################################################

BL2_CFLAGS += -ggdb3 -gdwarf-2
BL2_ASFLAGS += -g -Wa,--gdwarf-2

BL31_ASFLAGS += -g -Wa,--gdwarf-2

ifeq ($(notdir $(CC)),armclang)
BL31_CFLAGS_aarch32	=	-target arm-arm-none-eabi -march=armv8-a
BL31_CFLAGS_aarch64	=	-target aarch64-arm-none-eabi -march=armv8-a
else ifneq ($(findstring clang,$(notdir $(CC))),)
BL31_CFLAGS_aarch32	=	-target armv8a-none-eabi
BL31_CFLAGS_aarch64	=	-target aarch64-elf
else
BL31_CFLAGS_aarch32	=	-march=armv8-a
BL31_CFLAGS_aarch64	=	-march=armv8-a
endif

BL31_CFLAGS_aarch64	+=	-mstrict-align

ASFLAGS_aarch32		=	-march=armv8-a
ASFLAGS_aarch64		=	-march=armv8-a

BL31_CPPFLAGS += \
				${DEFINES} ${INCLUDES} \
				-nostdinc \
				-Wmissing-include-dirs -Werror

BL31_ASFLAGS	+=	$(BL31_CPPFLAGS) $(ASFLAGS_$(ARCH))			\
				-D__ASSEMBLY__ -ffreestanding 			\
				-Wa,--fatal-warnings

BL31_CFLAGS		+=	$(BL31_CPPFLAGS) $(BL31_CFLAGS_$(ARCH))		\
				-ffreestanding -fno-builtin -Wall -std=gnu99	\
				-Os -ffunction-sections -fdata-sections		\
				-fno-delete-null-pointer-checks


BL32_CPPFLAGS += \
				${DEFINES} ${INCLUDES} \
				-nostdinc \
				-Wmissing-include-dirs -Werror

BL32_ASFLAGS  += $(BL32_CPPFLAGS) $(ASFLAGS_$(ARCH))			\
				-D__ASSEMBLY__ -ffreestanding 			\
				-Wa,--fatal-warnings

BL32_CFLAGS  +=	$(BL32_CPPFLAGS) $(BL31_CFLAGS_$(ARCH))		\
				-ffreestanding -fno-builtin -Wall -std=gnu99	\
				-Os -ffunction-sections -fdata-sections		\
				-fno-delete-null-pointer-checks

ifeq (${ERROR_DEPRECATED},0)
    BL31_CFLAGS		+= 	-Wno-error=deprecated-declarations
endif

# Include the CPU specific operations makefile
include lib/cpu/${BOOT_CPU}/cpu.mk

BL2_INCLUDES += -Ibuild
BL2_INCLUDES += -Iinclude/cpu

################################################################################
# Build options checks
################################################################################
ifeq ($(NEED_BL31),yes)
$(eval $(call assert_boolean,COLD_BOOT_SINGLE_CPU))
$(eval $(call assert_boolean,CREATE_KEYS))
$(eval $(call assert_boolean,CTX_INCLUDE_AARCH32_REGS))
$(eval $(call assert_boolean,CTX_INCLUDE_FPREGS))
$(eval $(call assert_boolean,DEBUG))
$(eval $(call assert_boolean,DISABLE_PEDANTIC))
$(eval $(call assert_boolean,ENABLE_ASSERTIONS))
$(eval $(call assert_boolean,ENABLE_PLAT_COMPAT))
$(eval $(call assert_boolean,ENABLE_PMF))
$(eval $(call assert_boolean,ENABLE_PSCI_STAT))
$(eval $(call assert_boolean,ENABLE_RUNTIME_INSTRUMENTATION))
$(eval $(call assert_boolean,ERROR_DEPRECATED))
$(eval $(call assert_boolean,GENERATE_COT))
$(eval $(call assert_boolean,HW_ASSISTED_COHERENCY))
$(eval $(call assert_boolean,LOAD_IMAGE_V2))
$(eval $(call assert_boolean,NS_TIMER_SWITCH))
$(eval $(call assert_boolean,PL011_GENERIC_UART))
$(eval $(call assert_boolean,PROGRAMMABLE_RESET_ADDRESS))
$(eval $(call assert_boolean,PSCI_EXTENDED_STATE_ID))
$(eval $(call assert_boolean,RESET_TO_BL31))
$(eval $(call assert_boolean,SAVE_KEYS))
$(eval $(call assert_boolean,SEPARATE_CODE_AND_RODATA))
$(eval $(call assert_boolean,BL1_SEPARATE_CODE_AND_RODATA))
$(eval $(call assert_boolean,SPIN_ON_BL1_EXIT))
$(eval $(call assert_boolean,TRUSTED_BOARD_BOOT))
$(eval $(call assert_boolean,USE_COHERENT_MEM))
$(eval $(call assert_boolean,USE_TBBR_DEFS))
$(eval $(call assert_boolean,WARMBOOT_ENABLE_DCACHE_EARLY))
$(eval $(call assert_boolean,ENABLE_SPE_FOR_LOWER_ELS))
$(eval $(call assert_boolean,ENABLE_COMPRESSION))
$(eval $(call assert_boolean,ENABLE_TPU_SECURITY))
$(eval $(call assert_numeric,ARM_ARCH_MAJOR))
$(eval $(call assert_numeric,ARM_ARCH_MINOR))
endif
################################################################################
# Add definitions to the cpp preprocessor based on the current build options.
# This is done after including the platform specific makefile to allow the
# platform to overwrite the default options
################################################################################
ifeq ($(NEED_BL31),yes)
$(eval $(call add_define,ARM_CCI_PRODUCT_ID))
$(eval $(call add_define,ARM_ARCH_MAJOR))
$(eval $(call add_define,ARM_ARCH_MINOR))
$(eval $(call add_define,ARM_GIC_ARCH))
$(eval $(call add_define,COLD_BOOT_SINGLE_CPU))
$(eval $(call add_define,CTX_INCLUDE_AARCH32_REGS))
$(eval $(call add_define,CTX_INCLUDE_FPREGS))
$(eval $(call add_define,ENABLE_ASSERTIONS))
$(eval $(call add_define,ENABLE_PLAT_COMPAT))
$(eval $(call add_define,ENABLE_PMF))
$(eval $(call add_define,ENABLE_PSCI_STAT))
$(eval $(call add_define,ENABLE_RUNTIME_INSTRUMENTATION))
$(eval $(call add_define,ERROR_DEPRECATED))
$(eval $(call add_define,HW_ASSISTED_COHERENCY))
$(eval $(call add_define,LOAD_IMAGE_V2))
$(eval $(call add_define,LOG_LEVEL))
$(eval $(call add_define,NS_TIMER_SWITCH))
$(eval $(call add_define,PL011_GENERIC_UART))
$(eval $(call add_define,PLAT_${PLAT}))
$(eval $(call add_define,PROGRAMMABLE_RESET_ADDRESS))
$(eval $(call add_define,PSCI_EXTENDED_STATE_ID))
$(eval $(call add_define,RESET_TO_BL31))
$(eval $(call add_define,SEPARATE_CODE_AND_RODATA))
$(eval $(call add_define,BL1_SEPARATE_CODE_AND_RODATA))
$(eval $(call add_define,SPD_${SPD}))
$(eval $(call add_define,SPIN_ON_BL1_EXIT))
$(eval $(call add_define,TRUSTED_BOARD_BOOT))
$(eval $(call add_define,USE_COHERENT_MEM))
$(eval $(call add_define,USE_TBBR_DEFS))
$(eval $(call add_define,WARMBOOT_ENABLE_DCACHE_EARLY))
$(eval $(call add_define,ENABLE_SPE_FOR_LOWER_ELS))
$(eval $(call add_define,ENABLE_COMPRESSION))
$(eval $(call add_define,ENABLE_TPU_SECURITY))
$(eval $(call add_define,NANDBOOT_V2))
$(eval $(call add_define,BOOTFLOW_TYPE))
endif

ifeq ($(CONFIG_BOARD),"fpga")
$(eval $(call add_define,CONFIG_BOARD_fpga))
endif

################################################################################
# Build options checks
################################################################################
$(eval $(call assert_boolean,DEBUG))
$(eval $(call assert_boolean,ENABLE_ASSERTIONS))
$(eval $(call assert_boolean,NANDBOOT_V2))
$(eval $(call assert_boolean,NANDBOOT_V2))
$(eval $(call assert_boolean,PAGE_SIZE_64KB))
$(eval $(call assert_boolean,TEST_FROM_SPINOR1))
$(eval $(call assert_boolean,PRINTF_TIMESTAMP))

################################################################################
# Add definitions to the cpp preprocessor based on the current build options.
# This is done after including the platform specific makefile to allow the
# platform to overwrite the default options
################################################################################
$(call print_var,TEST_FROM_SPINOR1)
$(call print_var,PAGE_SIZE_64KB)

$(eval $(call add_define,TEST_FROM_SPINOR1))
$(eval $(call add_define,PAGE_SIZE_64KB))
$(eval $(call add_define,PRINTF_TIMESTAMP))

$(eval $(call add_define,ENABLE_ASSERTIONS))
$(eval $(call add_define,LOG_LEVEL))
$(eval $(call add_define,__CVITEK__))
$(eval $(call add_define,NANDBOOT_V2))


ifeq (${BOOT_CPU},riscv)
$(eval $(call add_define_val,TOC_HEADER_NAME,0xC906B001))
else
$(eval $(call add_define_val,TOC_HEADER_NAME,0xAA640001))
endif

ifeq (${RTOS_ENABLE_FREERTOS},y)
$(eval $(call add_define,RTOS_ENABLE_FREERTOS))
$(eval $(call add_define_val,RTOS_DUMP_PRINT_SZ_IDX,${RTOS_DUMP_PRINT_SZ_IDX}))
$(eval $(call add_define_val,RTOS_FAST_IMAGE_TYPE,${RTOS_FAST_IMAGE_TYPE}))
$(eval $(call add_define_val,RTOS_DUMP_PRINT_ENABLE,$(call yn10,${RTOS_DUMP_PRINT_ENABLE})))
endif

$(eval $(call add_define,FSBL_SECURE_BOOT_SUPPORT))
$(eval $(call add_define, USB_DL_BY_FSBL))

#ifeq (${NEED_BL31},yes)
# Add RTC_CORE_SRAM_BIN_PATH into cv_pm.c
#$(info RTC_CORE_SRAM_BIN_PATH is '${RTC_CORE_SRAM_BIN_PATH}')
#ifeq ($(filter clean %clean clean%,$(MAKECMDGOALS)),)
#ifeq (,$(wildcard ${RTC_CORE_SRAM_BIN_PATH}))
#$(error RTC_CORE_SRAM_BIN_PATH is not existed)
#else
#$(shell touch -c plat/cvitek/${CHIP_ARCH}/common/cv_pm.c > /dev/null)
#endif
#endif
#$(eval $(call add_define_val,RTC_CORE_SRAM_BIN_PATH,'"${RTC_CORE_SRAM_BIN_PATH}"'))
#endif

################################################################################
# Build targets
################################################################################
.PHONY: all fip clean bl-check bl-build fake-blcp
.SUFFIXES:

export BUILD_PLAT NM


all: bl2 fip blmacros

include ${MAKE_HELPERS_DIRECTORY}fip.mk
$(eval $(call MAKE_BL,2))
# Convert '#define ...' to ELF symbols
BLMACROS_LINKERFILE := make_helpers/get_macros.ld.S
BLMACROS_ELF := ${BUILD_PLAT}/blmacros/blmacros.elf

BLmacros_CPPFLAGS := $(BL2_CPPFLAGS)
BLmacros_ASFLAGS := $(BL2_ASFLAGS)
BLmacros_CFLAGS := $(BL2_CFLAGS)

$(eval $(call MAKE_BL,macros))

clean:
	$(print_target)
	$(call SHELL_REMOVE_DIR,${BUILD_PLAT})
