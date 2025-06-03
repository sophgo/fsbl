#!/bin/bash
set -eo

if [ "$1" = "" ];then
    echo "output path error"
    exit 1
fi

FSBL_PATH="$TOP_DIR"/fsbl

function generate_output()
{
    local OUTPUT_PATH=$1

    if [ ! -d ${OUTPUT_PATH}/plat/cv184x ]; then
        mkdir -p ${OUTPUT_PATH}/plat/cv184x
        mkdir -p ${OUTPUT_PATH}/plat/cv184x/bl2
        mkdir -p ${OUTPUT_PATH}/plat/cv184x/bl2_objs
    fi

    cp ${FSBL_PATH}/build/${PROJECT_FULLNAME}/bl31.bin ${OUTPUT_PATH}/plat/cv184x/
    cp ${FSBL_PATH}/build/${PROJECT_FULLNAME}/bl32.bin ${OUTPUT_PATH}/plat/cv184x/

    pushd ${FSBL_PATH}/build
        #copy cv184x objs
        local bl2_objs_dir=`find . -name "bl2"`
        pushd ${OUTPUT_PATH}/plat/cv184x/bl2_objs
        for dirName in ${bl2_objs_dir}
        do
            mkdir -p ${dirName}
            cp ${FSBL_PATH}/build/${dirName}/*.o ${dirName}
            rm -rf ${dirName}/{bl2.*,bl2_main.*,bl2_entrypoint*,build_message*,bl2_exceptions*}
        done
        popd

    popd

    cp -rf  ${FSBL_PATH}/include       ${OUTPUT_PATH}/
    cp -rf  ${FSBL_PATH}/lib            ${OUTPUT_PATH}/
    cp -rf  ${FSBL_PATH}/make_helpers   ${OUTPUT_PATH}/
    cp -rf  ${FSBL_PATH}/test/           ${OUTPUT_PATH}/
    cp -rf  ${FSBL_PATH}/Makefile_rel        ${OUTPUT_PATH}/Makefile
    cp -rf  ${FSBL_PATH}/README.rst     ${OUTPUT_PATH}/

    for dirName in {cv184x,}
    do
        cp -rf  ${FSBL_PATH}/bl2/{bl2.ld.S,bl2_main.c}             ${OUTPUT_PATH}/plat/${dirName}/bl2
        cp -rf  ${FSBL_PATH}/plat/cvitek/${dirName}/common/include                               ${OUTPUT_PATH}/plat/${dirName}
        cp -rf  ${FSBL_PATH}/plat/cvitek/${dirName}/common/prebuilt                              ${OUTPUT_PATH}/plat/${dirName}
        cp -rf  ${FSBL_PATH}/plat/cvitek/${dirName}/common/chip_conf.py                          ${OUTPUT_PATH}/plat/${dirName}
        cp -rf  ${FSBL_PATH}/plat/cvitek/${dirName}/common/fipsign.py                            ${OUTPUT_PATH}/plat/${dirName}
        cp -rf  ${FSBL_PATH}/plat/cvitek/${dirName}/common/fiptool.py                            ${OUTPUT_PATH}/plat/${dirName}
        cp -rf  ${FSBL_PATH}/plat/cvitek/${dirName}/common/platform_common.mk                           ${OUTPUT_PATH}/plat/${dirName}

        pushd ${OUTPUT_PATH}/plat/${dirName}
        sed -i "s,BL_COMMON_SOURCES = ,#BL_COMMON_SOURCES = ," platform_common.mk
        sed -i "s,DECOMPRESSION_SOURCES = ,#DECOMPRESSION_SOURCES = ," platform_common.mk
        sed -i "s,BL2_SRCS = ,#BL2_SRCS = ," platform_common.mk
        sed -i "s,include plat/,#include plat/," platform_common.mk
        sed -i "s,#BL2_RLS_OBJS,BL2_RLS_OBJS," platform_common.mk
        popd
    done

    # pushd ${OUTPUT_PATH}/make_helpers

    # sed -i "s!\$(eval OBJS       := \$(addprefix \$(BUILD_DIR)/,\$(call SOURCES_TO_OBJS,\
    # \$(SOURCES))))!\$(eval BULID_OUT  := \$(BUILD_DIR)/build_message.o) \n\t\t\
    # \$(eval OBJS_SOURCE := \$(wildcard \$(BUILD_DIR)/*.o))\n\t\t\$(eval OBJS       \
    # := \$(filter-out \$(BULID_OUT),\$(OBJS_SOURCE)))!" build_macros.mk

    # # remove BL2 ELF LD action
    # sed -i "/\$\$(LD) -o \$\$@ \$\$(TF_LDFLAGS)/d" build_macros.mk
    # sed -i "/--script \$(LINKERFILE)/d" build_macros.mk

    # popd
}

NORMAL_OUTPUT_PATH="$1"/fsbl

generate_output ${NORMAL_OUTPUT_PATH}
