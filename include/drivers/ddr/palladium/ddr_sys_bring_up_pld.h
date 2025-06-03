#ifndef __DDR_SYS_BRING_UP_H__
#define __DDR_SYS_BRING_UP_H__

#include <mmio.h>
#include <stdint.h>

#define DDR3_2133
// #define DDR3_1866
// #define DDR2_1333
// #define DDR2_1066

#define DDRC_BASE 0x08004000
#define PHYD_BASE 0x08000000
#define CV_DDR_PHYD_APB 0x08006000
#define DDR_BIST_BASE 0x08010000
#define DDR_TOP_BASE 0x0800a000

#define mmio_wr32	mmio_write_32
#define mmio_rd32	mmio_read_32

uint32_t modified_bits_by_value(uint32_t orig, uint32_t value, uint32_t msb, uint32_t lsb);
uint32_t get_bits_from_value(uint32_t value, uint32_t msb, uint32_t lsb);
void cvx16_setting_check(void);
void cvx16_set_dfi_init_start(void);
void cvx16_wait_for_dfi_init_complete(void);
void cvx16_polling_synp_normal_mode(void);
void ctrl_init_high_patch(void);
void ddr_init(uint32_t ddr_type, uint32_t ddr_freq_mode);
void ddr_sys_bring_up(void);

#endif