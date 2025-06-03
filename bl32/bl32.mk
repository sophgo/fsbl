
# BL32
BL32_ROOT		:=	bl32/cvsp
include ${BL32_ROOT}/cvsp.mk
BMSP_NS_INTR_ASYNC_PREEMPT	:=	0
BL32_SOURCES		+=	plat/cvitek/${CHIP_ARCH}/common/cv_bl32_setup.c
