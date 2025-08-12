


#define PM_SRAM_SAVED_REGS 0x640
#define PM_SRAM_SAVED_REG1 (0x05207280)
#define PM_SRAM_SAVED_REG2 (0x052078c0)

extern const unsigned short preserverd_phy_regs[];
extern const uint32_t REGS_LEN;
extern uint32_t ddr_ctrl_base;
extern uint32_t ddr_phya_base;
extern uint32_t ddr_top_base;
extern uint32_t ddr_phyd_base;
extern uint32_t ddr_preserved_data;

void cvx32_ddr_sys1_info_init(void);
void cvx32_ddr_sys2_info_init(void);