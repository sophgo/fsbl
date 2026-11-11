#include <platform_def.h>
#include <ddr_pkg_info.h>
#include <mmio.h>
#include <bitwise_ops.h>
#include <debug.h>

uint8_t ddr_vendor;
uint8_t ddr_capacity;
uint8_t pkg;
uint8_t ddr_type;

void read_ddr_pkg_info(void)
{
	uint32_t efuse_leakage = mmio_read_32(0x03050108);
	uint32_t chip_id;

#if defined(DBG_SHMOO) || defined(DBG_SHMOO_CA) || defined(DBG_SHMOO_CS)
	uint32_t conf_info = mmio_read_32(0x03000004);
	NOTICE("conf_info(0x03000004)=0x%x\n", conf_info);
	NOTICE("efuse_leakage(0x03050108)=0x%08x\n", efuse_leakage);
	NOTICE("FTSN3=0x%08x\n", mmio_read_32(0x0305010C));
	NOTICE("FTSN4=0x%08x\n", mmio_read_32(0x03050110));
#endif // DBG_SHMOO || DBG_SHMOO_CA || DBG_SHMOO_CS

	NOTICE("efuse_leakage(0x03050108)=0x%08x\n", efuse_leakage);

	/* conf_info[30:28] pkg_type is bonding IO trap, read-only and NOT
	 * efuse-programmable. On ES samples the bonding doesn't reflect the
	 * actual SIP DDR config, so read ddr vendor/capacity/pkg directly
	 * from efuse_leakage (row 2, 0x03050108) per Mars_Efuse_v0.12:
	 *   bit[25:21] = ddr_vendor   (DDR_VENDOR_*)
	 *   bit[28:26] = ddr_capacity (DDR_CAPACITY_*)
	 *   bit[31:29] = pkg          (PKG_*)
	 * Burn efuse row 2 with the right combination to override.
	 * External DDR3 (no SIP): pkg==7 (BGA169, CV1904G) -> DDR_VENDOR_EXT_DDR3.
	 * capacity is not consulted, same as cv181x CV1815J/K (pkg==5/6).
	 */
	ddr_vendor = FIELD_GET(efuse_leakage, 25, 21);
	ddr_capacity = FIELD_GET(efuse_leakage, 28, 26);
	pkg = FIELD_GET(efuse_leakage, 31, 29);
	if (pkg == PKG_BGA169) {
		NOTICE("External DDR detected\n");
		ddr_vendor = DDR_VENDOR_EXT_DDR3;
		ddr_capacity = DDR_CAPACITY_UNKNOWN;
		/* 1904G external DDR3: the board's native rate is 1600, while the
		 * shared ddr3_1866_x16 config defaults ddr_data_rate to 1866.
		 * Package-gated so the SIP boards sharing that config (1901C) keep
		 * 1866. The register set is unchanged; ddr_data_rate drives both the
		 * DDRPLL target (phy_pll_init.c) and the DDRPLL divider mode select
		 * (ddr_sys.c cvx16_pll_init). */
		ddr_data_rate = 1600;
	}
	NOTICE("vendor=%d cap=%d pkg=%d\n", ddr_vendor, ddr_capacity, pkg);

	NOTICE("D%x_%x_%x\n", pkg, ddr_capacity, ddr_vendor);

	// assign ddr_type / datarate (onebin support: rate decided by vendor)
	switch (ddr_vendor) {
	case DDR_VENDOR_ESMT_512M_DDR2:
	case DDR_VENDOR_ETRON_512M_DDR2:
	case DDR_VENDOR_UNIIC_N25_512M_DDR2:
		NOTICE("DDR2");
		ddr_type = DDR_TYPE_DDR2;
		ddr_data_rate = 1333;
		break;
	case DDR_VENDOR_NY_4G:
	case DDR_VENDOR_NY_2G:
	case DDR_VENDOR_ESMT_1G:
	case DDR_VENDOR_ETRON_1G:
	case DDR_VENDOR_ETRON_2G:
	case DDR_VENDOR_ESMT_2G:
	case DDR_VENDOR_PM_2G:
	case DDR_VENDOR_PM_1G:
	case DDR_VENDOR_ESMT_N25_1G:
	case DDR_VENDOR_NY_N20_1G:
	case DDR_VENDOR_UNIIC_N25_1G:
	case DDR_VENDOR_UNIIC_N21_2G:
	case DDR_VENDOR_ESMT_N21_2G:
	case DDR_VENDOR_ESMT_N19_4G:
	case DDR_VENDOR_EXT_DDR3:
	case DDR_VENDOR_EXT_DDR3_1905J:
		NOTICE("DDR3");
		ddr_type = DDR_TYPE_DDR3;
		// ddr_data_rate = 1866;	/* onebin: rate decided by defconfig DDR_CFG */
		break;
	default:
		NOTICE("unknown vendor=%d", ddr_vendor);
		ddr_type = DDR_TYPE_UNKNOWN;
	}

	switch (ddr_capacity) {
	case DDR_CAPACITY_512M:
		NOTICE("-512M");
		chip_id = (pkg == PKG_QFN88 ? 0x1900c : 0x1900b);   //cv190x lineage
		break;
	case DDR_CAPACITY_1G:
		NOTICE("-1G");
		chip_id = (pkg == PKG_QFN88 ? 0x1901c : 0x1901b);   //cv190x lineage
		break;
	case DDR_CAPACITY_2G:
		NOTICE("-2G");
		chip_id = 0x0;//cv190x lineage: unknown chip id
		break;
	case DDR_CAPACITY_4G:
		NOTICE("-4G");
		chip_id = 0x0;//cv190x lineage: unknown chip id
		break;
	default:
		if (ddr_vendor == DDR_VENDOR_EXT_DDR3) {
			/* external DDR, no SIP DDR: identify as CV1904G */
			chip_id = 0x1904b;
		} else {
			NOTICE("-unknown capacity=%d", ddr_capacity);
			chip_id = 0x0;
		}
	}
	mmio_write_32(REG_GP_REG3, chip_id);

	switch (pkg) {
	case PKG_QFN88:
		NOTICE("-QFN88\n");
		break;
	case PKG_QFN68:
		NOTICE("-QFN68\n");
		break;
	case PKG_BGA:
		NOTICE("-BGA\n");
		break;
	default:
		NOTICE("-unknown pkg=%d", pkg);
	}
}

uint8_t get_ddr_vendor(void)
{
	return ddr_vendor;
}

uint8_t get_ddr_capacity(void)
{
	return ddr_capacity;
}

uint8_t get_pkg(void)
{
	return pkg;
}

uint8_t get_ddr_type(void)
{
	return ddr_type;
}
