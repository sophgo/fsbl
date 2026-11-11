#include <cpu.h>
#include <mmio.h>
#include <debug.h>
#include <assert.h>
#include <errno.h>
#include <bl_common.h>
#include <bl2.h>
#include <platform.h>
#include <delay_timer.h>
#include <console.h>
#include <string.h>
#include <rom_api.h>
#include <platform_common_def.h>
#include <ddr_sys.h>

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
	mmio_setbits_32(REG_RTC_BASE + RTC_EN_WDT_RST_REQ, 0x0001);
	mmio_setbits_32(REG_RTC_BASE + RTC_EN_PWR_CYC_REQ, 0x0001);

	// sw delay 100us
	ATF_STATE = ATF_STATE_RESET_RTC_WAIT;
	udelay(100);

	// mmio_write_32(0x05025018,0x00FFFFFF); //Mercury rtcsys_rstn_src_sel
	mmio_setbits_32(REG_RTC_CTRL_BASE + RTC_POR_RST_CTRL, 0x00000000);
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0_UNLOCKKEY, 0x0000AB18);
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, 0x00400040); //enable rtc_ctrl watchdog reset enable

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
		while (mmio_read_32(REG_RTC_BASE + RTC_FSM_STATE) != 0x00000003)
			;

		mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, 0x00080008);
	}

	while (1)
		;

	__builtin_unreachable();
}

void reset_c906l(uintptr_t reset_address)
{
#if (defined(CONFIG_SUSPEND) && !defined(CONFIG_DUAL_OS))
	/*
	* In the scenario of a single-system suspend/resume operation,
	* disable the C906L clock to reduce power consumption.
	* clk_en_4 bit14 = clk_c906_1_en (C906L clock enable).
	*/
	NOTICE("Disable C906L CLK.\n");
	mmio_clrbits_32(REG_CLK_EN_4, BIT(CLK_C906_1_EN_BIT));
#else
	NOTICE("RSC.\n");
	mmio_clrbits_32(0x3003024, 1 << 6);

	mmio_setbits_32(SEC_SYS_BASE + 0x04, 1 << 13);
	mmio_write_32(SEC_SYS_BASE + 0x20, reset_address);
	mmio_write_32(SEC_SYS_BASE + 0x24, reset_address >> 32);

	mmio_setbits_32(0x3003024, 1 << 6);
#endif
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
	//0.set VDDC
	config_core_power(0x9);
	//1.switch pll to xtal(bypass mode)
	udelay(100);
	NOTICE("ODE.\n");
}

static void sys_pll_nd(int vc_overdrive)
{
	// ND clk setting
	uint32_t value;

	uint32_t pll_syn_set[] = {
		580296498, // set apll synthesizer  98.304 M
		576187216, // set disp synthesizer  99 M
	};

	uint32_t pll_csr[] = {
		0x00128201, // set apll *9/2 (442.368 MHz)
		0x00188101, // set disp *12/1 (1188 MHz)
	};

	// switch clock to xtal
	mmio_write_32(REG_CLK_BYP_H030, 0xffffffff);
	mmio_write_32(REG_CLK_BYP_H034, 0x0000003f);

	// set synthersizer clock
	mmio_write_32(REG_PLL_G2_SSC_SYN_CTRL, 0x3F); // enable synthesizer clock enable,
		// [0]: 1: MIPIMPLL(900)/1=900MHz,
		//      0: MIPIMPLL(900)/2=450MHz

	for (uint32_t i = 0; i < 2; i++) {
		mmio_write_32(REG_APLL_SSC_SYN_SET + 0x10 * i, pll_syn_set[i]); // set pll_syn_set

		value = mmio_read_32(REG_APLL_SSC_SYN_CTRL + 0x10 * i);
		value |= 1; // [0]: sw update (w1t: write one toggle)
		value &= ~(1 << 4); // [4]: bypass = 0
		mmio_write_32(REG_APLL_SSC_SYN_CTRL + 0x10 * i, value);

		mmio_write_32(REG_APLL0_CSR + 4 * i, pll_csr[i]); // set pll_csr
	}

	value = mmio_read_32(REG_PLL_G2_CTRL);
	value = value & (~0x00011111);
	mmio_write_32(REG_PLL_G2_CTRL, value); //clear all pll PD

	//wait for pll stable
	udelay(200);
	// 配置 c906_1 备用分频(div_clk_c906_1_1): fpll / 2
	mmio_write_32(REG_DIV_AP_CLK_RV1_1, 0x20009);
	mmio_clrbits_32(REG_CLK_SRC_SEL_H020, BIT(24));

	mmio_write_32(REG_CLK_BYP_H030, 0x0); // REG_CLK_BYPASS_SEL0_REG
	mmio_write_32(REG_CLK_BYP_H034, 0x0); // REG_CLK_BYPASS_SEL1_REG
}

/* Bump clk_axi4 (SPI NOR source) from reset 100MHz (FPLL/12) to 300MHz
 * (FPLL/4). Called after ROM API image loads complete, before jumping
 * to next stage, so uboot/kernel can run spinor at 75MHz sck. */
void cvi_bump_clk_axi4_to_300m(void)
{
	mmio_write_32(REG_DIV_CLK_AXI4, 0x00040009);
}

void sys_pll_init(void)
{
	NOTICE("PLLS.\n");
#ifdef CONFIG_BOARD_palladium
	/* PLD 仿真: PLL 不实际工作, 只打 PLLS/PLLE 标记,
	 * 让 fsbl 调用 rom 加载 image 时钟保持一致(走 25M xtal bypass) */
	NOTICE("PLLE.\n");
	return;
#endif
#ifdef OD_CLK_SEL
	mmio_write_32(0x030002d0, mmio_read_32(0x030002d0) | 0x200); //enable pwm
	sys_pll_init_od();
#else
	sys_pll_nd(0);
#endif
	console_flush();
	NOTICE("PLLE.\n");
}

void sys_switch_all_to_pll(void)
{
#ifdef CONFIG_BOARD_palladium
	/* PLD 仿真: 不切 clock 到 PLL, 保持 xtal bypass */
	return;
#endif
	// Switch all clocks to PLL
	mmio_write_32(REG_CLK_BYP_H030, 0x0);
	mmio_write_32(REG_CLK_BYP_H034, 0x0);
	NOTICE("sys_switch_all_to_pll...\n");
}

#ifndef NO_DDR_CFG //for fpga
static void *get_warmboot_entry(void)
{
	/*
	 * "FSM state change to ST_ON from the state
	 * 4'h0 = state changed from ST_OFF to ST_ON
	 * 4'h3 = state changed to ST_PWR_CYC or ST_WARM_RESET then back to ST_ON
	 * 4'h9 = state changed from ST_SUSP to ST_ON
	 */
// the WARM_BOOT state can be used for WDG reset test
#define WANTED_STATE 0x9

	/* Check if RTC state changed from ST_SUSP */
	if ((mmio_read_32(REG_RTC_ST_ON_REASON) & 0xF) == WANTED_STATE)
		return (void *)(uintptr_t)mmio_read_32(RTC_SRAM_FLAG_ADDR);

	return 0;
}
#endif

#ifndef NO_DDR_CFG //for fpga

static void blcp_2nd_c906l_reset(struct fip_param2 *fip_param2)
{
	uint32_t rtos_base;

	if (!fip_param2->blcp_2nd_runaddr)
		return;

	rtos_base = mmio_read_32(AXI_SRAM_RTOS_BASE);
	if (rtos_base != CVI_RTOS_MAGIC_CODE) {
		INFO("WE_0x%lx\n", fip_param2->blcp_2nd_runaddr);
		reset_c906l(fip_param2->blcp_2nd_runaddr);
	}
}
void rtc_set_ddr_pwrok(void)
{
	mmio_setbits_32(REG_RTC_BASE + RTC_PG_REG, 0x00000001);
}

void rtc_set_rmio_pwrok(void)
{
	mmio_setbits_32(REG_RTC_BASE + RTC_PG_REG, 0x00000002);
}

static void ddr_resume(void)
{
	rtc_set_ddr_pwrok();
	rtc_set_rmio_pwrok();
	ddr_sys_resume();
}

void platform_warmentry(void)
{
	void (*warmboot_entry)() = get_warmboot_entry();
	struct fip_param1 *fip_param1 = (void *)PARAM1_BASE;
	struct fip_param2 fip_param2 __aligned(BLOCK_SIZE);

	// treat next reset as warm boot
	mmio_write_64(RTC_SRAM_FLAG_ADDR, 0);
	if (warmboot_entry) {
		INFO("WE=0x%lx\n", (uintptr_t)warmboot_entry);

		if (p_rom_api.load_image(&fip_param2,
			fip_param1->param2_loadaddr,
			PARAM2_SIZE - sizeof(fip_param2.reserved4), 0) < 0) {
			fip_param2.blcp_2nd_runaddr = 0;
			ERROR("param2 load fail\n");
		}
		NOTICE("ddr resume...\n");
		ddr_resume();
		NOTICE("ddr resume end\n");
		sys_pll_init();
		sys_switch_all_to_pll();
		blcp_2nd_c906l_reset(&fip_param2);
#ifdef AARCH64
		disable_mmu_icache_el3();
		__asm__ volatile("tlbi alle3\n");
#endif
		warmboot_entry();
	}
}
#endif

void switch_rtc_mode_1st_stage(void)
{
	uint32_t read_data;
	uint32_t write_data;
	uint32_t rtc_mode;

#ifdef AARCH64
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
	mmio_write_32(RTC_MACRO_BASE + RTC_MACRO_DA_SOC_READY, 0x01);
	// DA_SOC_READY = 0
	mmio_write_32(RTC_MACRO_BASE + RTC_MACRO_DA_SOC_READY, 0x0);

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

	// mdelay(50);
	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0_STATUS0);

	if (get_pkg() == PKG_QFN || (read_data & 0x02000000) == 0x00) {

		read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
		// reg_rtc_mode = rtc_ctrl0[10]
		write_data = 0x0C000000 | (read_data & 0xfffffbff);
		mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, write_data);
		//DA_SOC_READY = 1
		mmio_write_32(RTC_MACRO_BASE + RTC_MACRO_DA_SOC_READY, 0x01);
		//DA_SOC_READY = 0
		mmio_write_32(RTC_MACRO_BASE + RTC_MACRO_DA_SOC_READY, 0x00);
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
	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
	write_data = 0x00200000|(read_data & ~0x20);
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, write_data);
	read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
	read_data = read_data & 0x20;
	while (read_data == 0x20) {
		read_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
		NOTICE("RTC_CTRL0=%x\n", read_data);
		read_data = read_data & 0x20;
	}

	mmio_write_32(REG_RTC_BASE + RTC_EN_SHDN_REQ, 0x01);

	mmio_write_32(REG_RTC_BASE + RTC_EN_WARM_RST_REQ, 0x01);

	mmio_write_32(REG_RTC_BASE + RTC_EN_PWR_CYC_REQ, 0x01);

	mmio_setbits_32(REG_RTC_BASE + RTC_EN_WDT_RST_REQ, 0x01);
	while (!(mmio_read_32(REG_RTC_BASE + RTC_EN_WDT_RST_REQ) & 0x01))
		;

	// Set rtcsys_por_rst_ctrl[0] = 1; bit 0 is reg_rtcsys_reset_en
	mmio_setbits_32(REG_RTC_CTRL_BASE + RTC_POR_RST_CTRL, 0X1);

	// rtc_ctrl0_unlockkey
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0_UNLOCKKEY, 0xAB18);

	// Enable hw_wdg_rst_en
	write_data = mmio_read_32(REG_RTC_CTRL_BASE + RTC_CTRL0);
	write_data = 0xffff0000 | write_data | (0x1 << 11) | (0x01 << 6);
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0, write_data);

	// Disable RTC_EN_AUTO_POWER_UP (VBAT_DET[2]) to prevent unexpected power-up after poweroff
	mmio_clrbits_32(REG_RTC_BASE + RTC_EN_PWR_VBAT_DET, BIT(2));
}

#if defined(RTOS_ENABLE_FREERTOS) || defined(RTOS_ENABLE_RTT)
void memcpy_u32(void *dst, void *src, int size)
{
	int i = 0;

	for (i = 0; i < size / 4; i++) {
		*(unsigned int *)dst = *(unsigned int *)src;
		dst += 4;
		src += 4;
	}
}
#endif
void init_comm_info(void) __attribute((optimize("O0")));
void init_comm_info(void)
{
#if defined(RTOS_ENABLE_FREERTOS) || defined(RTOS_ENABLE_RTT)
	struct transfer_config_t *transfer_config = (struct transfer_config_t *)MAILBOX_FIELD;
	struct transfer_config_t transfer_config_s;
	unsigned char           *ptr      = (unsigned char *)&transfer_config_s;
	unsigned short           checksum = 0;
	/* mailbox field is 4 byte write access, and can not access byte by byte.
	 * so init parameters and copy all to mailbox field together.
	 */
	transfer_config_s.conf_magic      = RTOS_MAGIC_HEADER;
	transfer_config_s.conf_size       = ((uint64_t)&transfer_config_s.checksum -
								   (uint64_t)&transfer_config_s.conf_magic);
	transfer_config_s.isp_buffer_addr = CVIMMAP_ISP_MEM_BASE_ADDR;
	transfer_config_s.isp_buffer_size = CVIMMAP_ISP_MEM_BASE_SIZE;
	transfer_config_s.encode_img_addr = CVIMMAP_H26X_BITSTREAM_ADDR;
	transfer_config_s.encode_img_size = CVIMMAP_H26X_BITSTREAM_SIZE;
	transfer_config_s.encode_buf_addr = CVIMMAP_H26X_ENC_BUFF_ADDR;
	transfer_config_s.encode_buf_size = CVIMMAP_H26X_ENC_BUFF_SIZE;
	transfer_config_s.dump_print_enable   = RTOS_DUMP_PRINT_ENABLE;
	transfer_config_s.dump_print_size_idx = RTOS_DUMP_PRINT_SZ_IDX;
	transfer_config_s.image_type          = RTOS_FAST_IMAGE_TYPE;
	transfer_config_s.mcu_status          = MCU_STATUS_NONOS_DONE;
	for (int i = 0; i < transfer_config_s.conf_size; i++) {
		checksum += ptr[i];
	}

	transfer_config_s.checksum = checksum;
	memcpy_u32((void *)transfer_config, (void *)&transfer_config_s,
			   sizeof(struct transfer_config_t));
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
#ifndef CONFIG_AOV_REDUCE_TIME
	print_sram_log();
#endif

	rom_api_redirect();
	setup_dl_flag();

#ifdef SWITCH_32K_XTAL
	switch_rtc_mode_1st_stage();
#endif
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
