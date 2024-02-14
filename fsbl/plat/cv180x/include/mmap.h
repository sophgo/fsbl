#ifndef __MMAP_H__
#define __MMAP_H__

#include "cvi_board_memmap.h"

/*
 * BL1 read-only specific defines.
 */
#define BL1_ROM_BASE ROM_BASE

#define BL1_ROM_SIZE ROM_SIZE

#define BL1_RO_BASE BL1_ROM_BASE
#define BL1_RO_LIMIT (BL1_ROM_BASE + BL1_ROM_SIZE)

#define BL1_VERSION_INFO_SIZE 64 // 32bytes date string + 32bytes of SHA256
#define BL1_DIGEST_ADDR (BL1_ROM_BASE + BL1_ROM_SIZE - 32) // Size of SHA256

#define BL1_ROM_MASK_SIZE 8192
#define RISCV_BL1_ROM_PSMSK_VALUE (1 << 23)
#define CA53_BL1_ROM_PSMSK_VALUE (1 << 11)

#define ROM_API_ALIGN 32

#define BL_RAM_BASE TPU_SRAM_BASE
#define BL_RAM_SIZE TPU_SRAM_SIZE

/*
 * IO buffer specific defines.
 * block IO buffer's start address and size must be block size aligned
 */
#define BOOT_LOG_BUF_BASE (BL_RAM_BASE)
#define BOOT_LOG_BUF_SIZE 0x2000

#define PARAM1_BASE (BOOT_LOG_BUF_BASE + BOOT_LOG_BUF_SIZE)
#define PARAM1_SIZE 0x1000 // same as typeof(struct fip_param1).
#define PARAM1_SIZE_WO_SIG 0x800

#define CV_IO_BUF_BASE (PARAM1_BASE + PARAM1_SIZE)
#define CV_IO_BUF_SIZE 0x2000

#define BL1_RW_BASE (CV_IO_BUF_BASE + CV_IO_BUF_SIZE)
#define BL1_RW_SIZE (0xB000)

/*
 * Put BL2 in VC SRAM
 */
#define BL2_BASE (VC_RAM_BASE)
#define BL2_SIZE (VC_SRAM_SIZE)
#define BL2_ENTRY_OFFSET 32

/*
 * FIP binary defines.
 */
#define FIP_PARAM1_MAGIC1 0x000A31304c425643ULL // "CVBL01\n\0"
#define FIP_PARAM2_MAGIC1 0x000A3230444c5643ULL // "CVLD02\n\0"

#define FLASH_NUMBER_OF_RETRIES 8
#define FIP_RETRY_OFFSET (256 * 1024)
#define FIP_MAX_SIZE (get_number_of_retries() * FIP_RETRY_OFFSET)

#define PLATFORM_STACK_SIZE 0x2000

/*
 * DRAM map
 */

#define DRAM_BASE CVIMMAP_DRAM_BASE
#define DRAM_SIZE CVIMMAP_DRAM_SIZE

#define MONITOR_RUNADDR CVIMMAP_MONITOR_ADDR
#define LICENSE_FILE_ADDR (DRAM_BASE + 0x20020) // in ATF

#define OPENSBI_FDT_ADDR CVIMMAP_OPENSBI_FDT_ADDR

#define DECOMP_ALLOC_SIZE (1 << 20)
#define DECOMP_BUF_SIZE (CVIMMAP_FSBL_UNZIP_SIZE - DECOMP_ALLOC_SIZE)

#define DECOMP_ALLOC_ADDR CVIMMAP_FSBL_UNZIP_ADDR
#define DECOMP_BUF_ADDR (DECOMP_ALLOC_ADDR + DECOMP_ALLOC_SIZE)

#if DECOMP_BUF_SIZE <= 0
#error "FSBL_UNZIP_SIZE is not enough"
#endif

#define DECOMP_DST_SIZE (16 << 20)

#define BLCP_2ND_RUNADDR CVIMMAP_FSBL_C906L_START_ADDR

#endif /* __MMAP_H__ */
