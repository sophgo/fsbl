#include <cpu.h>
#include <mmio.h>
#include <debug.h>
#include <assert.h>
#include <errno.h>
#include <bl_common.h>
#include <platform.h>
#include <delay_timer.h>
#include <console.h>
#include <string.h>
#include <rom_api.h>

#include "ddr_pkg_info.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define TPU_BONDING_LMT0 0x1
#define TPU_BONDING_LMT1 0x2

extern struct rom_api p_rom_api;

void plat_panic_handler(void)
{
	void *ra;

	ATF_ERR = ATF_ERR_PLAT_PANIC;

	ra = __builtin_return_address(0);
	mmio_write_32(ATF_ERR_INFO0, ((uint64_t)ra) & 0xFFFFFFFFUL);

	ERROR("ra=0x%lx\n", (uint64_t)ra);

	__system_reset("panic", -1);
	__builtin_unreachable();
}

void __system_reset(const char *file, unsigned int line)
{
	ATF_ERR = ATF_ERR_PLAT_SYSTEM_RESET;
	ERROR("RESET:%s:%d\n", file, line);

	console_flush();

	ATF_STATE = ATF_STATE_RESET_WAIT;
	mdelay(5000);

	// enable rtc wdt reset
	mmio_write_32(0x050260E0, 0x0001); //enable rtc_core wathdog reset enable
	mmio_write_32(0x050260C8, 0x0001); //enable rtc_core power cycle   enable

	// sw delay 100us
	ATF_STATE = ATF_STATE_RESET_RTC_WAIT;
	udelay(100);

	// mmio_write_32(0x05025018,0x00FFFFFF); //Mercury rtcsys_rstn_src_sel
	mmio_write_32(0x050250AC, 0x00000000); //cv181x rtcsys_rstn_src_sel
	mmio_write_32(0x05025004, 0x0000AB18);
	mmio_write_32(0x05025008, 0x00400040); //enable rtc_ctrl wathdog reset enable

	// printf("Enable TOP_WDT\n");
	// mmio_write_32(0x03010004,0x00000000); //config watch dog 2.6ms
	// mmio_write_32(0x03010004,0x00000022); //config watch dog 166ms
	mmio_write_32(0x03010004, 0x00000066); //config watch dog 166ms
	mmio_write_32(0x0301001c, 0x00000020);
	mmio_write_32(0x0301000c, 0x00000076);
	mmio_write_32(0x03010000, 0x00000011);

	// ROM_PWR_CYC
	if (get_sw_info()->reset_type == 1) {
		ATF_ERR = ATF_ERR_PLAT_SYSTEM_PWR_CYC;
		// printf("Issue RTCSYS_PWR_CYC\n");

		// wait pmu state to ON
		while (mmio_read_32(0x050260D4) != 0x00000003) {
			;
		}

		mmio_write_32(0x05025008, 0x00080008);
	}

	while (1)
		;

	__builtin_unreachable();
}

void reset_c906l(uintptr_t reset_address)
{
	NOTICE("RSC.\n");

	mmio_clrbits_32(0x3003024, 1 << 6);

	mmio_setbits_32(SEC_SYS_BASE + 0x04, 1 << 13);
	mmio_write_32(SEC_SYS_BASE + 0x20, reset_address);
	mmio_write_32(SEC_SYS_BASE + 0x24, reset_address >> 32);

	mmio_setbits_32(0x3003024, 1 << 6);
}

void setup_dl_flag(void)
{
	uint32_t v = p_rom_api.get_boot_src();

	NOTICE("boot src: 0x%x \n", v);
	switch (v) {
	case BOOT_SRC_SD:
		mmio_write_32(BOOT_SOURCE_FLAG_ADDR, MAGIC_NUM_SD_DL);
		break;
	case BOOT_SRC_USB:
		mmio_write_32(BOOT_SOURCE_FLAG_ADDR, MAGIC_NUM_USB_DL);
		break;
	case BOOT_SRC_UART:
		mmio_write_32(BOOT_SOURCE_FLAG_ADDR, MAGIC_NUM_UART_DL);
		break;
	default:
		mmio_write_32(BOOT_SOURCE_FLAG_ADDR, v);
		break;
	}
}

void config_core_power(uint32_t low_period)
{
	/*
	 * low_period = 0x30; // 0.85V
	 * low_period = 0x24; // 0.90V
	 * low_period = 0x9; // 1.02V
	 */
	mmio_write_32(0x030002d0, mmio_read_32(0x030002d0) | 0x200); //enable pwm
	mmio_write_32(PINMUX_BASE + 0xec, 0x0); //pinmux for pwm0
	mmio_write_32(PWM0_BASE + 0x4, 0x64); //period0:100
	mmio_write_32(PWM0_BASE + 0x174, 0x0); //start_point0:0
	mmio_write_32(PWM0_BASE + 0x178, low_period); //end_point:low_period
	mmio_write_32(PWM0_BASE + 0x120, 0x1); //oe0:out
	mmio_write_32(PWM0_BASE + 0x6c, 0x1); //update
	mmio_write_32(PWM0_BASE + 0x64, 0x1); //start0
	mdelay(10);
}

void config_rgmii_power(void)
{
	mmio_write_32(0x281051e0, mmio_read_32(0x281051e0) | 0x300); //RGMII 1.8V
	mmio_write_32(0x281001cc, mmio_read_32(0x281001cc) | 0x101); //rx delay 2ns
}

void sys_pll_init_od(void)
{
	NOTICE("ODS.\n");
	uint32_t val = 0, div_sel_val = 0;
	//0.set VDDC
	config_core_power(0x9);
	//1.switch pll to xtal(bypass mode)
	mmio_write_32(REG_CLK_BYP_H104, 0xFFFFFFFF);
	mmio_write_32(REG_CLK_BYP_H108, 0xFFFFFFFF);
	mmio_write_32(REG_CLK_BYP_H10C, 0xFFFFFFFF);

	//2.set tpll/appll/rvpll *_pll_csr to OD mode
	//set div_sel bit17-23
	div_sel_val = 60;	//1500MHz
	val = mmio_read_32(REG_TPLL_CSR);
	val = (val & ~G6_DIV_SEL_MASK) | ((div_sel_val << G6_DIV_SEL_SHIFT) & G6_DIV_SEL_MASK);
	mmio_write_32(REG_TPLL_CSR, val);

	div_sel_val = 44;	//1100MHz
	val = mmio_read_32(REG_APPLL_CSR);
	val = (val & ~G6_DIV_SEL_MASK) | ((div_sel_val << G6_DIV_SEL_SHIFT) & G6_DIV_SEL_MASK);
	mmio_write_32(REG_APPLL_CSR, val);

	div_sel_val = 64;	//1600MHz
	val = mmio_read_32(REG_RVPLL_CSR);
	val = (val & ~G6_DIV_SEL_MASK) | ((div_sel_val << G6_DIV_SEL_SHIFT) & G6_DIV_SEL_MASK);
	mmio_write_32(REG_RVPLL_CSR, val);

	//3.clear *_pll_pwd reg
	val = mmio_read_32(REG_PLL_G6_CTRL);
	val = val & (~0x00011111);
	mmio_write_32(REG_PLL_G6_CTRL, val); //clear all pll PD

	//set clk_tpu_gdma src from tpll to cam0pll
	//1. set reg_tpu_clk_gdma_sel bit2 0
	mmio_clrbits_32(REG_CLK_SRC_SEL_H000, 1 << 2);
	//2. set div_tpu_clk_gdma_1_reg to 0x20009(D2)
	mmio_write_32(REG_DIV_TPU_CLK_GDMA_1, 0x20009);

	//set clk_tpu from 500MHz to 700MHz
	//1. set div_tpu_clk_0 to 0x20009(D2)
	mmio_write_32(REG_DIV_TPU_CLK_TPU_0, 0x20009);

	//set vc_pr_warp clk src to div1
	mmio_clrbits_32(REG_CLK_SRC_SEL_H000, 0x7 << 4);
	//set clk_video_axi from 500MHz to 600MHz
	//set clk_vc_src0 from 600MHz to 650MHz
	//set clk_vc_src1 from 400MHz to 500MHz
	NOTICE("ODE.\n");
}

void sys_pll_init(void)
{
	NOTICE("PLLS.\n");
#ifdef OD_CLK_SEL
	sys_pll_init_od();
#endif
	//fix cam*pll_ssc_syn_set reg to be empty
	uint32_t cam0pll_ssc_set = 0x90000000;	//cam0pll synthesizer set 36
	uint32_t cam1pll_ssc_set = 0x90000000;	//cam1pll synthesizer set 36

	//trigger pll G2
	//set cam*pll_ssc_syn_set reg for initial value and don't trigger
	mmio_write_32(REG_CAM0PLL_SSC_SYN_SET, cam0pll_ssc_set);
	mmio_write_32(REG_CAM1PLL_SSC_SYN_SET, cam1pll_ssc_set);

	//a0pll sw update
	mmio_write_32(REG_APLL_SSC_SYN_SET, 0x1D4C0000);	//set a0pll to 491.52MHz
	mmio_write_32(REG_APLL_SSC_SYN_CTRL, TOGGLE_SSC_SYN_SW_UP);
	//disppll sw update
	mmio_write_32(REG_DISPPLL_SSC_SYN_CTRL, TOGGLE_SSC_SYN_SW_UP);

	udelay(10);

	// bypass
	mmio_write_32(REG_CLK_BYP_H104, 0);
	mmio_clrbits_32(REG_CLK_BYP_H108,
		~(0x1 << 19 | 0x1 << 3 | 0x1 << 4)); //bit 19(reg_hsperi_clk_uart0_byp & sd0_byp)
	mmio_write_32(REG_CLK_BYP_H10C, 0);

	//reinit console
	//console_init(0, PLAT_UART_CLK_PLL_HZ, PLAT_CONSOLE_BAUDRATE);
	NOTICE("PLLE.\n");
}


void switch_rtc_mode_1st_stage(void)
{
	uint32_t read_data;
	uint32_t write_data;
	uint32_t rtc_mode;

#ifdef CV181X_SUPPORT_SUSPEND_RESUME
	void (*warmboot_entry)(void) = get_warmboot_entry();

	if (warmboot_entry == (void *)BL31_WARMBOOT_ENTRY)
		return;
#endif

	// reg_rtc_mode = rtc_ctrl0[10]
	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
	rtc_mode = (read_data >> 10) & 0x1;
	if (rtc_mode == 0x1) {
		NOTICE("By pass rtc mode switch\n");
		return;
	}

	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0_UNLOCKKEY, 0xAB18);
	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);

	// reg_clk32k_cg_en = rtc_ctrl0[11] -> 0
	write_data = 0x08000000 | (read_data & 0xfffff7ff);
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, write_data);

	//cg_en_out_clk_32k = rtc_ctrl_status0[25]
	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0_STATUS0);
	while ((read_data & 0x02000000) != 0x00)
		read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0_STATUS0);

	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
	//r eg_rtc_mode = rtc_ctrl0[10];
	write_data = 0x04000000 | (read_data & 0xfffffbff) | (0x1 << 10);
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, write_data);

	// DA_SOC_READY = 1
	mmio_write_32(RTC_MACRO_BASE + 0x8C, 0x01);
	// DA_SOC_READY = 0
	mmio_write_32(RTC_MACRO_BASE + 0x8C, 0x0);

	udelay(200); // delay ~200us

	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
	// reg_clk32k_cg_en = rtc_ctrl0[11] -> 1
	write_data = 0x0C000000 | (read_data & 0xffffffff) | (0x1 << 11);
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, write_data); //rtc_ctrl0
}


void switch_rtc_mode_2nd_stage(void)
{
	uint32_t read_data;
	uint32_t write_data;

	mdelay(50);
	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0_STATUS0);

	if (get_pkg() == PKG_QFN || (read_data & 0x02000000) == 0x00) {

		read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
		// reg_rtc_mode = rtc_ctrl0[10]
		write_data = 0x0C000000 | (read_data & 0xfffffbff);
		mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, write_data);
		//DA_SOC_READY = 1
		mmio_write_32(RTC_MACRO_BASE + 0x8C, 0x01);
		//DA_SOC_READY = 0
		mmio_write_32(RTC_MACRO_BASE + 0x8C, 0x00);
		NOTICE("Use internal 32k\n");
	} else
		NOTICE("Switch RTC mode to xtal32k\n");
}


void set_rtc_en_registers(void)
{
	uint32_t write_data;
	uint32_t read_data;

	read_data = mmio_read_32(REG_RTC_BASE + RTC_ST_ON_REASON);
	NOTICE("st_on_reason=%x\n", read_data);
	read_data = mmio_read_32(REG_RTC_BASE + RTC_ST_OFF_REASON);
	NOTICE("st_off_reason=%x\n", read_data);

	mmio_write_32(REG_RTC_BASE + RTC_EN_SHDN_REQ, 0x01);
	while (mmio_read_32(REG_RTC_BASE + RTC_EN_SHDN_REQ) != 0x01)
		;

	mmio_write_32(REG_RTC_BASE + RTC_EN_WARM_RST_REQ, 0x01);
	while (mmio_read_32(REG_RTC_BASE + RTC_EN_WARM_RST_REQ) != 0x01)
		;

	mmio_write_32(REG_RTC_BASE + RTC_EN_PWR_CYC_REQ, 0x01);
	while (mmio_read_32(REG_RTC_BASE + RTC_EN_PWR_CYC_REQ) != 0x01)
		;

	mmio_write_32(REG_RTC_BASE + RTC_EN_WDT_RST_REQ, 0x01);
	while (mmio_read_32(REG_RTC_BASE + RTC_EN_WDT_RST_REQ) != 0x01)
		;

	// Set rtcsys_rst_ctrl[24] = 1; bit 24 is reg_rtcsys_reset_en
	mmio_setbits_32(REG_RTC_CTRL_BASE + RTC_POR_RST_CTRL, 0X1);

	// rtc_ctrl0_unlockkey
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0_UNLOCKKEY, 0xAB18);

	// Enable hw_wdg_rst_en
	write_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
	write_data = 0xffff0000 | write_data | (0x1 << 11) | (0x01 << 6);
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, write_data);

	// Avoid power up again after poweroff
	mmio_clrbits_32(REG_RTC_BASE + RTC_EN_PWR_VBAT_DET, BIT(2));
}

void init_comm_info(void)
{
#ifdef RTOS_ENABLE_FREERTOS
	struct transfer_config_t *transfer_config = (struct transfer_config_t *)MAILBOX_FIELD;
	struct transfer_config_t transfer_config_s;
	unsigned char *ptr = (unsigned char *)&transfer_config_s;
	unsigned short checksum = 0;
	/* mailbox field is 4 byte write access, and can not access byte by byte.
	 * so init parameters and copy all to mailbox field together.
	 */
	transfer_config_s.conf_magic = RTOS_MAGIC_HEADER;
	transfer_config_s.conf_size = ((uint64_t)&transfer_config_s.checksum - (uint64_t)&transfer_config_s.conf_magic);
	transfer_config_s.isp_buffer_addr = CVIMMAP_ISP_MEM_BASE_ADDR;
	transfer_config_s.isp_buffer_size = CVIMMAP_ISP_MEM_BASE_SIZE;
	transfer_config_s.encode_img_addr = CVIMMAP_H26X_BITSTREAM_ADDR;
	transfer_config_s.encode_img_size = CVIMMAP_H26X_BITSTREAM_SIZE;
	transfer_config_s.encode_buf_addr = CVIMMAP_H26X_ENC_BUFF_ADDR;
	transfer_config_s.encode_buf_size = CVIMMAP_H26X_ENC_BUFF_SIZE;
	transfer_config_s.dump_print_enable = RTOS_DUMP_PRINT_ENABLE;
	transfer_config_s.dump_print_size_idx = RTOS_DUMP_PRINT_SZ_IDX;
	transfer_config_s.image_type = RTOS_FAST_IMAGE_TYPE;
	transfer_config_s.mcu_status = MCU_STATUS_NONOS_DONE;
	for (int i = 0; i < transfer_config_s.conf_size; i++) {
		checksum += ptr[i];
	}

	transfer_config_s.checksum = checksum;
	memcpy(transfer_config, &transfer_config_s, sizeof(struct transfer_config_t));
#endif
}

void apply_analog_trimming_data(void)
{
}

void bm_storage_boot_loader_version(uint32_t addr)
{
	int size_version, size_time;

	size_version = strlen(version_string);
	for (int i = 0; i < size_version; i++) {
		mmio_write_8(addr + i, version_string[i]);
	}

	mmio_write_8(addr + size_version, ' ');

	size_time = strlen(build_message);
	for (int i = 0; i < size_time; i++) {
		mmio_write_8(addr + size_version + 1 + i, build_message[i]);
	}

	mmio_write_8(addr + size_version + 1 + size_time, '\0');
}

void enable_top_wdt(void)
{
	// config top wdt 30s
	mmio_write_32((PERI_WDT0_BASE + 0x4), 0xEE);
	mmio_write_32((PERI_WDT0_BASE + 0x1c), 0x5969);
	mmio_write_32(PERI_WDT0_BASE, 0xD1);
}

void platform_setup(void)
{
	/* config_rgmii_power(); */
	time_records->fsbl_start = read_time_ms();
	NOTICE("\nFSBL %s:%s\n", version_string, build_message);

	INFO("fip_param1: param_cksum=0x%x param2_loadaddr=0x%x\n",
	     fip_param1->param_cksum, fip_param1->param2_loadaddr);
	//print_sram_log();

	rom_api_redirect();
	setup_dl_flag();
	switch_rtc_mode_1st_stage();
	set_rtc_en_registers();
	bm_storage_boot_loader_version(BL2_VERSION_BASE);
}

#define C906L_RESET_REG 0x03003024
#define C906L_RVBA_L 0x20b0020
#define C906L_RVBA_H 0x20b0024
#define C906L_BOOT_SEL 0x20b0004

void bringup_c906l(unsigned long addr)
{
	//bit10 ' b0
	mmio_clrbits_32(C906L_RESET_REG, (1 << 6));

	//set c906b rvba
	mmio_setbits_32(C906L_BOOT_SEL, 1 << 13);
	mmio_write_32(C906L_RVBA_H, addr >> 32);
	mmio_write_32(C906L_RVBA_L, addr & 0xFFFFFFFF);
	udelay(3);

	//bit10 ' b1
	mmio_setbits_32(C906L_RESET_REG, (1 << 6));
}

#pragma GCC diagnostic pop
