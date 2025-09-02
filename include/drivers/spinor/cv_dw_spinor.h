/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cv_spinor.h
 *
 * Description: Cvitek SPI NOR flash driver header
 */

#ifndef __CV_DW_SPINOR_H__
#define __CV_DW_SPINOR_H__

#define SPINOR_CMD_REN              0x66
#define SPINOR_CMD_RESET            0x99
#define SPINOR_CMD_WREN             0x06
#define SPINOR_CMD_WRDI             0x04
#define SPINOR_CMD_RDID             0x9F
#define SPINOR_CMD_RDSR             0x05
#define SPINOR_CMD_WRSR             0x01

#define SPINOR_CMD_READ             0x03
#define SPINOR_CMD_FAST_READ        0x0B
#define SPINOR_CMD_READ_4B          0x13
#define SPINOR_CMD_READ_FRD4B       0x0C
#define SPINOR_CMD_READ_FRQIO       0xEB

#define SPINOR_CMD_SE               0x20
#define SPINOR_CMD_SE_4B            0x21
#define SPINOR_CMD_BE_32K           0x52
#define SPINOR_CMD_BE_64K           0xD8
#define SPINOR_CMD_BE_64K_4B        0xDC

#define SPINOR_CMD_PP               0x02
#define SPINOR_CMD_PP_4B            0x12
#define SPINOR_CMD_PP_4B_QIO        0x34
#define SPINOR_CMD_PP_4B_QIEF       0x3E
#define SPINOR_CMD_SFDP             0x5A
#define SPINOR_CMD_EN4B             0xB7
#define SPINOR_CMD_EX4B             0xE9
#define SPINOR_CMD_ENQD             0x35
#define SPINOR_CMD_EXQD             0xF5

#define SPINOR_STATUS_WIP          (0x01 << 0)
#define SPINOR_STATUS_WEL          (0x01 << 1)
#define SPINOR_STATUS_BP0          (0x01 << 2)
#define SPINOR_STATUS_BP1          (0x01 << 3)
#define SPINOR_STATUS_BP2          (0x01 << 4)
#define SPINOR_STATUS_SRWD         (0x01 << 7)

#define SPINOR_FLASH_BLOCK_SIZE             256
#define ADDR_3B             3
#define ADDR_4B             4
#define  _16M          (0x1000000UL)

#define SPINOR_OP_READ          0x03    /* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST     0x0b    /* Read data bytes (high frequency) */
#define SPINOR_OP_READ_1_1_2    0x3b    /* Read data bytes (Dual Output SPI) */
#define SPINOR_OP_READ_1_2_2    0xbb    /* Read data bytes (Dual I/O SPI) */
#define SPINOR_OP_READ_1_1_4    0x6b    /* Read data bytes (Quad Output SPI) */
#define SPINOR_OP_READ_1_4_4    0xeb    /* Read data bytes (Quad I/O SPI) */
#define SPINOR_OP_READ_1_1_8    0x8b    /* Read data bytes (Octal Output SPI) */
#define SPINOR_OP_READ_1_8_8    0xcb    /* Read data bytes (Octal I/O SPI) */
#define SPINOR_OP_PP            0x02    /* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_1_1_4      0x32    /* Quad page program */
#define SPINOR_OP_PP_1_4_4      0x38    /* Quad page program */
#define SPINOR_OP_PP_1_1_8      0x82    /* Octal page program */
#define SPINOR_OP_PP_1_8_8      0xc2    /* Octal page program */
#define SPINOR_OP_BE_4K         0x20    /* Erase 4KiB block */
#define SPINOR_OP_BE_4K_PMC     0xd7    /* Erase 4KiB block on PMC chips */
#define SPINOR_OP_BE_32K        0x52    /* Erase 32KiB block */
#define SPINOR_OP_CHIP_ERASE    0xc7    /* Erase whole flash chip */
#define SPINOR_OP_SE            0xd8    /* Sector erase (usually 64KiB) */
#define SPINOR_OP_RDID          0x9f    /* Read JEDEC ID */
#define SPINOR_OP_RDSFDP        0x5a    /* Read SFDP */
#define SPINOR_OP_RDCR          0x35    /* Read configuration register */
#define SPINOR_OP_RDFSR         0x70    /* Read flag status register */
#define SPINOR_OP_CLFSR         0x50    /* Clear flag status register */
#define SPINOR_OP_RDEAR         0xc8    /* Read Extended Address Register */
#define SPINOR_OP_WREAR         0xc5    /* Write Extended Address Register */

/* 4-byte address opcodes - used on Spansion and some Macronix flashes. */
#define SPINOR_OP_READ_4B       0x13    /* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST_4B  0x0c    /* Read data bytes (high frequency) */
#define SPINOR_OP_READ_1_1_2_4B 0x3c    /* Read data bytes (Dual Output SPI) */
#define SPINOR_OP_READ_1_2_2_4B 0xbc    /* Read data bytes (Dual I/O SPI) */
#define SPINOR_OP_READ_1_1_4_4B 0x6c    /* Read data bytes (Quad Output SPI) */
#define SPINOR_OP_READ_1_4_4_4B 0xec    /* Read data bytes (Quad I/O SPI) */
#define SPINOR_OP_READ_1_1_8_4B 0x7c    /* Read data bytes (Octal Output SPI) */
#define SPINOR_OP_READ_1_8_8_4B 0xcc    /* Read data bytes (Octal I/O SPI) */
#define SPINOR_OP_PP_4B         0x12    /* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_1_1_4_4B   0x34    /* Quad page program */
#define SPINOR_OP_PP_1_4_4_4B   0x3e    /* Quad page program */
#define SPINOR_OP_PP_1_1_8_4B   0x84    /* Octal page program */
#define SPINOR_OP_PP_1_8_8_4B   0x8e    /* Octal page program */
#define SPINOR_OP_BE_4K_4B      0x21    /* Erase 4KiB block */
#define SPINOR_OP_BE_32K_4B     0x5c    /* Erase 32KiB block */
#define SPINOR_OP_SE_4B         0xdc    /* Sector erase (usually 64KiB) */

/* Double Transfer Rate opcodes - defined in JEDEC JESD216B. */
#define SPINOR_OP_READ_1_1_1_DTR        0x0d
#define SPINOR_OP_READ_1_2_2_DTR        0xbd
#define SPINOR_OP_READ_1_4_4_DTR        0xed

#define SPINOR_OP_READ_1_1_1_DTR_4B     0x0e
#define SPINOR_OP_READ_1_2_2_DTR_4B     0xbe
#define SPINOR_OP_READ_1_4_4_DTR_4B     0xee

/* spi-nor status register */
#define SPI_STATUS_WIP          (1 << 0)
#define SPI_STATUS_WEL          (1 << 1)
#define SPI_STATUS_BP0          (1 << 2)
#define SPI_STATUS_BP1          (1 << 3)
#define SPI_STATUS_BP2          (1 << 4)
#define SPI_STATUS_BP3          (1 << 5)
#define SPI_STATUS_TP           (1 << 6)
#define SPI_STATUS_SRWD         (1 << 7)

/* general definition */
#define SPI_FLASH_BLOCK_SIZE             256
#define SPI_TRAN_CSR_ADDR_BYTES_SHIFT    8
#define SPI_MAX_FIFO_DEPTH               8

#define SIZE_SECTOR_4K          0x1000
#define SIZE_SECTOR_64K         0x10000
#define SIZE_256B               256


/* functions */
void cv_dw_spinor_init(void);
int cv_dw_spinor_read(void *buff, uint32_t offset, size_t size);
int cv_dw_spinor_program(uint32_t addr, uintptr_t buf, size_t len);
int cv_dw_spinor_erase(uint32_t addr, size_t len);
void spi_init(void);

#endif	/* __CV_SPINOR_H__ */
