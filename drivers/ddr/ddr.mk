

${warning "THIS IS $(PWD)"}

ifeq (${CVIBOARD},"palladium")
include drivers/ddr/palladium/ddr.mk
else ifeq (${CVIBOARD},"fpga")
else
include drivers/ddr/ddr_normal.mk
endif


