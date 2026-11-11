#ifndef __DDR_PKG_INFO_H__
#define __DDR_PKG_INFO_H__
#include <stdint.h>

#define DDR_VENDOR_UNKNOWN		0b00000
#define DDR_VENDOR_NY_4G		0b00001
#define DDR_VENDOR_NY_2G		0b00010
#define DDR_VENDOR_ESMT_1G		0b00011
#define DDR_VENDOR_ESMT_512M_DDR2	0b00100
#define DDR_VENDOR_ETRON_1G		0b00101
#define DDR_VENDOR_ESMT_2G		0b00110
#define DDR_VENDOR_ETRON_2G		0b01000	/* ETRON 2Gb D3 20nm (efuse authority Mars_Efuse_v0.12) */
#define DDR_VENDOR_PM_2G		0b10010	/* cv181x lineage, no pkg_type entry on cv190x */
#define DDR_VENDOR_PM_1G		0b10011	/* cv181x lineage, no pkg_type entry on cv190x */
#define DDR_VENDOR_ETRON_512M_DDR2	0b01001
#define DDR_VENDOR_ESMT_N25_1G		0b01010
#define DDR_VENDOR_NY_N20_1G		0b01011
#define DDR_VENDOR_ESMT_N21_2G		0b01100
#define DDR_VENDOR_ESMT_N19_4G		0b01101
#define DDR_VENDOR_UNIIC_N25_512M_DDR2	0b01110
#define DDR_VENDOR_UNIIC_N25_1G		0b01111
#define DDR_VENDOR_UNIIC_N21_2G		0b10000
/* 1904G production ext DDR3 (BGA9, dedicated pinmux; selected when
 * efuse pkg==7 (BGA169))
 */
#define DDR_VENDOR_EXT_DDR3		0b11111
/* 1905J experimental ext DDR3 (BGA-2, pinmux inherited from P2M;
 * rarely used going forward, kept for legacy 1905J rebuilds).
 * Code space: keep clear of the real efuse vendor codes (contiguous up to
 * 0b10000) and of the reserved ext-DDR2 slot 0b11110; 0b11101 is unused. */
#define DDR_VENDOR_EXT_DDR3_1905J	0b11101

#define DDR_CAPACITY_UNKNOWN		0b000
#define DDR_CAPACITY_512M		0b001
#define DDR_CAPACITY_1G			0b010
#define DDR_CAPACITY_2G			0b011
#define DDR_CAPACITY_4G			0b100

#define PKG_UNKNOWN			0b000
#define PKG_QFN88			0b001
#define PKG_BGA				0b010
#define PKG_QFN68			0b011
#define PKG_BGA169			0b111
/* cv190x shared code (cv_bl2_setup.c) 用 PKG_QFN, 跟 PKG_QFN88 同值 0b001 */
#define PKG_QFN				PKG_QFN88

#define DDR_TYPE_UNKNOWN		0
#define DDR_TYPE_DDR2			1
#define DDR_TYPE_DDR3			2

extern uint32_t ddr_data_rate;

void read_ddr_pkg_info(void);
uint8_t get_ddr_vendor(void);
uint8_t get_ddr_capacity(void);
uint8_t get_pkg(void);
uint8_t get_ddr_type(void);

#endif /* __DDR_PKG_INFO_H__ */
