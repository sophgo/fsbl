BLCP_PATH = ../fast_image_mcu/riscv/output/fast_image_mcu.bin
ifeq ("$(wildcard $(BLCP_PATH))","")
BLCP_PATH = test/empty.bin
endif
FIP_COMPRESS ?= lz4

CHIP_CONF_PATH = ${BUILD_PLAT}/chip_conf.bin

ifeq (${BOOT_CPU},aarch64)
MONITOR_PATH = plat/${CHIP_ARCH}/prebuilt/bl31.bin
BL32_PATH = plat/${CHIP_ARCH}/prebuilt/bl32.bin
else ifeq (${BOOT_CPU},riscv)
ifeq (${CONFIG_SKIP_UBOOT},y)
MONITOR_PATH = ${OUTPUT_DIR}/fw_jump.bin
else
MONITOR_PATH = ../opensbi/build/platform/generic/firmware/fw_dynamic.bin
endif
endif

ifeq (${DOUBLESDK},y)
FREERTOS_A_PATH = ./cvirtos.bin
MONITOR_A_PATH = ./fw_dynamic.bin
LOADER_2ND_A_PATH = ./u-boot-raw.bin
endif

fip%: export BLCP_IMG_RUNADDR=0x05200200
fip%: export BLCP_PARAM_LOADADDR=0
fip%: export NAND_INFO=00000000
ifeq ($(IMPROVE_BL_NOR_SPEED),y)
fip%: export NOR_INFO=01C0080000430000A93B060000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
else
fip%: export NOR_INFO=$(shell printf '%72s' | tr ' ' 'FF')
endif
fip%: export DDR_PARAM_TEST_PATH = test/cv181x/ddr_param.bin
ifeq (${CONFIG_ENABLE_EMMC_HW_RESET_QFN},y)
$(eval $(call add_define,CONFIG_ENABLE_EMMC_HW_RESET_QFN))
endif

${BUILD_PLAT}:
	@mkdir -p '${BUILD_PLAT}'

gen-chip-conf:
	$(print_target)
	${Q}mkdir -p '${BUILD_PLAT}'
	${Q}./plat/${CHIP_ARCH}/chip_conf.py ${CHIP_CONF_PATH}

macro_to_env = ${NM} '${BLMACROS_ELF}' | awk '/DEF_${1}/ { rc = 1; print "${1}=0x" $$1 } END { exit !rc }' >> ${BUILD_PLAT}/blmacros.env

ifeq (${BOOT_CPU},aarch64)
blmacros-env: blmacros
	$(print_target)
	${Q}> ${BUILD_PLAT}/blmacros.env  # clear .env first
	${Q}$(call macro_to_env,MONITOR_RUNADDR)
	${Q}$(call macro_to_env,BLCP_2ND_RUNADDR)
	${Q}$(call macro_to_env,BL32_RUNADDR)
else
blmacros-env: blmacros
	$(print_target)
	${Q}> ${BUILD_PLAT}/blmacros.env  # clear .env first
	${Q}$(call macro_to_env,MONITOR_RUNADDR)
	${Q}$(call macro_to_env,BLCP_2ND_RUNADDR)
endif



fip: fip-all

fip-dep: bl2 blmacros-env gen-chip-conf

fip-simple: fip-dep
	$(print_target)
	${Q}echo "  [GEN] fip.bin"
	${Q}${FIPTOOL} -v genfip \
		'${BUILD_PLAT}/fip.bin' \
		--CHIP_CONF='${CHIP_CONF_PATH}' \
		--NOR_INFO='${NOR_INFO}' \
		--NAND_INFO='${NAND_INFO}'\
		--BL2='${BUILD_PLAT}/bl2.bin'
	${Q}echo "  [LS] " $$(ls -l '${BUILD_PLAT}/fip.bin')

ifeq (${BOOT_CPU},aarch64)
fip-all: fip-dep
	$(print_target)
	${Q}echo "  [GEN] fip.bin"
	${Q}. ${BUILD_PLAT}/blmacros.env && \
	${FIPTOOL} -v genfip \
		'${BUILD_PLAT}/fip.bin' \
		--MONITOR_RUNADDR="$${MONITOR_RUNADDR}" \
		--BLCP_2ND_RUNADDR="$${BLCP_2ND_RUNADDR}" \
		--BL32_RUNADDR="$${BL32_RUNADDR}" \
		--CHIP_CONF='${CHIP_CONF_PATH}' \
		--NOR_INFO='${NOR_INFO}' \
		--NAND_INFO='${NAND_INFO}'\
		--BL2='${BUILD_PLAT}/bl2.bin' \
		--BLCP_IMG_RUNADDR=${BLCP_IMG_RUNADDR} \
		--BLCP_PARAM_LOADADDR=${BLCP_PARAM_LOADADDR} \
		--BLCP=${BLCP_PATH} \
		--BLCP_2ND='${BLCP_2ND_PATH}' \
		--MONITOR='${MONITOR_PATH}' \
		--BL32='${BL32_PATH}' \
		--LOADER_2ND='${LOADER_2ND_PATH}' \
		--compress='${FIP_COMPRESS}'\
		--doublesdk='${DOUBLESDK}'\
		--FREERTOS_A='${FREERTOS_A_PATH}' \
		--MONITOR_A='${MONITOR_A_PATH}'\
		--LOADER_2ND_A='${LOADER_2ND_A_PATH}'
	${Q}echo "  [LS] " $$(ls -l '${BUILD_PLAT}/fip.bin')
else
fip-all: fip-dep
	$(print_target)
	${Q}echo "  [GEN] fip.bin"
	${Q}. ${BUILD_PLAT}/blmacros.env && \
	${FIPTOOL} -v genfip \
		'${BUILD_PLAT}/fip.bin' \
		--MONITOR_RUNADDR="$${MONITOR_RUNADDR}" \
		--BLCP_2ND_RUNADDR="$${BLCP_2ND_RUNADDR}" \
		--CHIP_CONF='${CHIP_CONF_PATH}' \
		--NOR_INFO='${NOR_INFO}' \
		--NAND_INFO='${NAND_INFO}'\
		--BL2='${BUILD_PLAT}/bl2.bin' \
		--BLCP_IMG_RUNADDR=${BLCP_IMG_RUNADDR} \
		--BLCP_PARAM_LOADADDR=${BLCP_PARAM_LOADADDR} \
		--BLCP=${BLCP_PATH} \
		--BLCP_2ND='${BLCP_2ND_PATH}' \
		--MONITOR='${MONITOR_PATH}' \
		--LOADER_2ND='${LOADER_2ND_PATH}' \
		--compress='${FIP_COMPRESS}'\
		--doublesdk='${DOUBLESDK}'\
		--FREERTOS_A='${FREERTOS_A_PATH}' \
		--MONITOR_A='${MONITOR_A_PATH}'\
		--LOADER_2ND_A='${LOADER_2ND_A_PATH}'
	${Q}echo "  [LS] " $$(ls -l '${BUILD_PLAT}/fip.bin')
endif