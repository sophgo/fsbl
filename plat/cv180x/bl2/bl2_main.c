#include <debug.h>
#include <console.h>
#include <platform.h>
#include <bl2.h>
#include <string.h>
#include <delay_timer.h>
#include <decompress.h>
#include <rom_api.h>
#include <rtc.h>
#include "cvi_spinor.h"
#include <usb/cv_usb.h>

struct _macros_misc macros_misc;
enum CHIP_CLK_MODE chip_clk_mode = CLK_ND;
#ifdef DOUBLESDK
char boot_flag_A;
#endif
#ifdef DOUBLESDK
int init_comm_info(int ret)
{
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

	return ret;
}
#endif

#if !FSBL_SECURE_BOOT_SUPPORT
int dec_verify_image(const void *image, size_t size, size_t dec_skip, struct fip_param1 *fip_param1)
{
	return 0;
}
#endif

#ifndef COMPRESS_RTOS_BIN
int bl2_decompress(void *dst, size_t *dst_size, const void *src, size_t src_size, enum COMPRESS_TYPE type)
{
	src = &src_size;
	return 0;
}
#endif

#ifndef CONFIG_SKIP_UBOOT
int  load_kernel(int retry)
{
	NOTICE("not SKIP_UBOOT \n");
	return 1;
}
#else
int load_loader_2nd(int retry, uint64_t *loader_2nd_entry){
	return 0;
}
#endif

void init_macros(void)
{
	macros_misc.dram_base = DRAM_BASE;
	macros_misc.dram_size = DRAM_SIZE;
	macros_misc.blcp_2nd_loadaddr  = BLCP_2ND_LOADADDR;
	macros_misc.jump_loadaddr      = JUMP_LOADADDR;
	macros_misc.jump_loadaddr_size = JUMP_LOADADDR_SIZE;
	macros_misc.decomp_dts_size    =  DECOMP_DST_SIZE;
	macros_misc.blcp_2nd_comp_bin_addr = BLCP_2ND_COMP_BIN_ADDR;
	macros_misc.bl_param_runaddr = BL_PARAM_RUNADDR;
	macros_misc.bl_param_loadaddr = BL_PARAM_LOADADDR;
	macros_misc.bl_param_size = BL_PARAM_SIZE;
	macros_misc.bl_param_bak_runaddr = BL_PARAM_BAK_RUNADDR;
	macros_misc.bl_param_bak_loadaddr = BL_PARAM_BAK_LOADADDR;
	macros_misc.bl_logo_runaddr = BL_LOGO_RUNADDR;
	macros_misc.bl_logo_loadaddr = BL_LOGO_LOADADDR;
	macros_misc.bl_logo_size= BL_LOGO_SIZE;
	macros_misc.bl_pq_runaddr = BL_PQ_RUNADDR;
	macros_misc.bl_pq_loadaddr = BL_PQ_LOADADDR;
	macros_misc.bl_pq_size = BL_PQ_SIZE;
	macros_misc.bl_decomp_buf_addr = DECOMP_BUF_ADDR; 
#ifdef	BOOT_FROM_NAND
	macros_misc.storage_type = BOOT_SRC_SPI_NAND;
#endif

#ifdef  BOOT_FROM_NOR
	macros_misc.storage_type = BOOT_SRC_SPI_NOR;
#endif
}

// init CVIMMAP_SHARE_MEM_ADDR,CVIMMAP_FREERTOS_ADDR for C906B/C906L
void init_ipcm_memory(void)
{
	mmio_write_32(AXI_SRAM_BASE + 0x2c, CVIMMAP_FREERTOS_ADDR);
	// 0x30 for usb dl
	mmio_write_32(AXI_SRAM_BASE + 0x34, CVIMMAP_FREERTOS_SIZE);
	mmio_write_32(AXI_SRAM_BASE + 0x38, CVIMMAP_SHARE_MEM_ADDR);
	mmio_write_32(AXI_SRAM_BASE + 0x3c, CVIMMAP_SHARE_MEM_SIZE);
	// NOTICE("0x30 = 0x%x. 0x%x , 0x%x , 0x%x\n",
	// 		mmio_read_32(AXI_SRAM_BASE + 0x30),
	// 		mmio_read_32(AXI_SRAM_BASE + 0x34),
	// 		mmio_read_32(AXI_SRAM_BASE + 0x38),
	// 		mmio_read_32(AXI_SRAM_BASE + 0x3c));
}
void init_param_memory(void)
{
	mmio_write_32(CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 4*4 , CVIMMAP_SHARE_PARAM_ADDR);
	mmio_write_32(CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 4*3 , CVIMMAP_SHARE_PARAM_ADDR_BAK);
	mmio_write_32(CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 4*2 , CVIMMAP_PQBIN_ADDR);
	// NOTICE("0x%x(0x%x), 0x%x(0x%x), 0x%x(0x%x)\n",
	// 		CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 4*4 , CVIMMAP_SHARE_PARAM_ADDR,
	// 		CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 4*3 , CVIMMAP_SHARE_PARAM_ADDR_BAK,
	// 		CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 4*2 , CVIMMAP_PQBIN_ADDR);

	// store alios log addr and size
	mmio_write_32(CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 4*5 , CVIMMAP_ALIOS_LOG_ADDR);
	mmio_write_32(CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 4*6 , CVIMMAP_ALIOS_LOG_SIZE);
	flush_dcache_range(CVIMMAP_SHARE_MEM_ADDR + CVIMMAP_SHARE_MEM_SIZE - 64, 64);

	// zero alios log write/read flag memory
	mmio_write_32(CVIMMAP_ALIOS_LOG_ADDR, 0);
	mmio_write_32(CVIMMAP_ALIOS_LOG_ADDR + 0x4, 0);
	flush_dcache_range(CVIMMAP_ALIOS_LOG_ADDR, 64);
	mmio_write_32(CVIMMAP_ALIOS_LOG_ADDR + CVIMMAP_ALIOS_LOG_SIZE - 0x4, 0);
	mmio_write_32(CVIMMAP_ALIOS_LOG_ADDR + CVIMMAP_ALIOS_LOG_SIZE - 0x8, 0);
	flush_dcache_range(CVIMMAP_ALIOS_LOG_ADDR + CVIMMAP_ALIOS_LOG_SIZE - 64, 64);
}

void check_spi_nor(void)
{
	//check magic1
	int ret = -1;
	uint64_t magic1_value = 0;
	uint64_t magic1_load_addr = 0;
	cv_spinor_init(SPI_CLK_75M);
#ifdef USB_DL_BY_FSBL
	if (p_rom_api_get_boot_src() == BOOT_SRC_USB)
		ret = load_image_by_usb(&magic1_value, magic1_load_addr, sizeof(magic1_value), 0);
	else
#endif
		ret = p_rom_api_load_image(&magic1_value,magic1_load_addr,sizeof(magic1_value),0);
	if(ret <0){
			ERROR("Fail use four lines mode addr:%lx  value:%lx \r\n",magic1_load_addr,magic1_value);
	}
	if(magic1_value != fip_param1->magic1){
			cv_spinor_init(SPI_CLK_15M);
			NOTICE("\r retry one line mode \r\n");
	}
}


void bl2_main(void)
{
	ATF_STATE = ATF_STATE_BL2_MAIN;
	time_records->fsbl_start = read_time_ms();

	NOTICE("\nFSBL %s:%s\n", version_string, build_message);

	INFO("sw_info=0x%x\n", get_sw_info()->value);
	INFO("fip_param1: param_cksum=0x%x param2_loadaddr=0x%x\n", fip_param1->param_cksum,
	     fip_param1->param2_loadaddr);

	INFO("CP_STATE_REG=0x%x\n", mmio_read_32(0x0E000018));

#ifdef DOUBLESDK
	boot_flag_A = console_getc();
	if (boot_flag_A == 'A')
		boot_flag_A = 1;
	else
		boot_flag_A = 0;
#endif
	// print_sram_log();
#ifdef  BOOT_FROM_NOR
	check_spi_nor();
#endif
	init_macros();
	init_ipcm_memory();

#ifdef CONFIG_SKIP_UBOOT
	cv180x_ephy_id_init();
	cv180x_ephy_led_pinmux();
#endif

	setup_dl_flag();

	switch_rtc_mode_1st_stage();

	set_rtc_en_registers();

	/* force boot from flash */
#ifdef FORCE_BOOT_FROM_FLASH
	if (((mmio_read_32(REG_RTC_ST_ON_REASON) & 0xF) != COLD_BOOT) &&
		(mmio_read_32(RTC_INFO0) == UPDATED)) {
		p_rom_api_set_boot_src(macros_misc.storage_type);
		if (macros_misc.storage_type == BOOT_SRC_SPI_NAND ||
			macros_misc.storage_type == BOOT_SRC_EMMC) {
			p_rom_api_flash_init();
		}
		mmio_write_32(RTC_INFO0, NO_UPDATE);
		NOTICE("boot from flash\n");
	}
#endif

#ifdef OD_CLK_SEL
	chip_clk_mode = CLK_OD;
#else
#ifdef VC_CLK_OVERDRIVE
	chip_clk_mode = CLK_VC_OD;
#endif
#endif
	load_ddr();
	init_param_memory();

#ifdef DOUBLESDK
	load_rest_doublesdk();
#else
	load_rest();
#endif

	NOTICE("BL2 end.\n");

	while (1)
		;
}
