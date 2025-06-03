$(call print_var,DDR_CFG)

# DDR_CFG = ddr3_2133_x16
# DDR_CFG = ddr3_1866_x16
# DDR_CFG = ddr2_1333_x16
# DDR_CDF = ddr_auto_x16

ifeq (${DDR_CFG},none)
DDR_CFG =
endif

ifeq ($(DDR_CFG), )
BL2_INCLUDES += \
	-Iinclude/drivers/ddr

BL2_SOURCES += \
	drivers/ddr/ddr.c

$(eval $(call add_define,NO_DDR_CFG))
else
BL2_INCLUDES += \
	-Iinclude/drivers/ddr \
	-Iinclude/drivers/ddr/ddr_config/${DDR_CFG}

BL2_SOURCES += \
	drivers/ddr/ddr.c \
	drivers/ddr/ddr_pkg_info.c \
	drivers/ddr/ddr_sys_bring_up.c \
	drivers/ddr/ddr_sys.c \
	drivers/ddr/phy_pll_init.c \
	drivers/ddr/cvx16_pinmux.c \
	drivers/ddr/cvx16_dram_cap_check.c \
	drivers/ddr/ddr_config/${DDR_CFG}/ddrc_init.c \
	drivers/ddr/ddr_config/${DDR_CFG}/phy_init.c \
	drivers/ddr/ddr_config/${DDR_CFG}/ddr_patch_regs.c

ifneq ($(findstring ddr3, ${DDR_CFG}),)
    $(eval $(call add_define,DDR3))
else ifneq ($(findstring ddr2, ${DDR_CFG}),)
    $(eval $(call add_define,DDR2))
else ifneq ($(findstring ddr_auto, ${DDR_CFG}),)
    $(eval $(call add_define,DDR2_3))
endif

ifneq ($(findstring 2133, ${DDR_CFG}),)
    $(eval $(call add_define,_mem_freq_2133))
else ifneq ($(findstring 1866, ${DDR_CFG}),)
    $(eval $(call add_define,_mem_freq_1866))
else ifneq ($(findstring 1333, ${DDR_CFG}),)
    $(eval $(call add_define,_mem_freq_1333))
endif

$(eval $(call add_define,REAL_DDRPHY))
# $(eval $(call add_define,SSC_EN))
$(eval $(call add_define,REAL_LOCK))
$(eval $(call add_define,X16_MODE))

# pinmux
# ifneq ($(findstring ddr3, ${DDR_CFG}),)
#     # $(eval $(call add_define,DDR3_4G))
#     $(eval $(call add_define,DDR3_1G))
# else ifneq ($(findstring ddr2, ${DDR_CFG}),)
#     $(eval $(call add_define,N25_DDR2_512))
# endif

# full mem bist
# $(eval $(call add_define,DBG_SHMOO))
# $(eval $(call add_define,DBG_SHMOO_CA))
# $(eval $(call add_define,DBG_SHMOO_CS))
# $(eval $(call add_define,FULL_MEM_BIST))
# $(eval $(call add_define,FULL_MEM_BIST_FOREVER))

# overdrive clock setting
ifeq ($(OD_CLK_SEL),y)
$(eval $(call add_define,OD_CLK_SEL))
else ifeq ($(VC_CLK_OVERDRIVE),y)
$(eval $(call add_define,VC_CLK_OVERDRIVE))
endif
ifeq ($(TPU_PERF_MODE),y)
$(eval $(call add_define,TPU_PERF_MODE))
endif

endif
