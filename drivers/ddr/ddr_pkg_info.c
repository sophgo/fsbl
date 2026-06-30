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
	uint32_t conf_info = mmio_read_32(0x03000004);
	uint32_t efuse_leakage = mmio_read_32(0x03050108);
	uint32_t pkg_type;
	uint32_t chip_id;

#if defined(DBG_SHMOO) || defined(DBG_SHMOO_CA) || defined(DBG_SHMOO_CS)
	NOTICE("conf_info(0x03000004)=0x%x\n", conf_info);
	NOTICE("efuse_leakage(0x03050108)=0x%08x\n", efuse_leakage);
	NOTICE("FTSN3=0x%08x\n", mmio_read_32(0x0305010C));
	NOTICE("FTSN4=0x%08x\n", mmio_read_32(0x03050110));
#endif // DBG_SHMOO || DBG_SHMOO_CA || DBG_SHMOO_CS

	NOTICE("conf_info(0x03000004)=0x%x\n", conf_info);
	pkg_type = (FIELD_GET(conf_info, 23, 23) << 3) | FIELD_GET(conf_info, 30, 28);
	//pkg_type = 2;
	NOTICE("pkg_type=%x\n", pkg_type);

	switch (pkg_type) {
	case 0x0: //2nd src need to read from efuse
		NOTICE("2nd\n");
		ddr_vendor = FIELD_GET(efuse_leakage, 25, 21);
		ddr_capacity = FIELD_GET(efuse_leakage, 28, 26);
		pkg = FIELD_GET(efuse_leakage, 31, 29);
		break;
	case 0x1: //BGA 10x10, SIP 4Gb DDR3
		ddr_vendor = DDR_VENDOR_NY_4G;
		ddr_capacity = DDR_CAPACITY_4G;
		//pkg = PKG_BGA;
		ddr_type = DDR_TYPE_DDR3;
		break;
	case 0x2: //BGA 10x10, SIP 2Gb DDR3
		ddr_vendor = DDR_VENDOR_NY_2G;
		ddr_capacity = DDR_CAPACITY_2G;
		//pkg = PKG_BGA;
		ddr_type = DDR_TYPE_DDR3;
		break;
	case 0x3: //BGA 10x10, SIP 1Gb DDR3
		ddr_vendor = DDR_VENDOR_ESMT_1G;
		ddr_capacity = DDR_CAPACITY_1G;
		//pkg = PKG_QFN;
		ddr_type = DDR_TYPE_DDR3;
		break;
	case 0x4: //SIP 512Mb DDR2
		ddr_vendor = DDR_VENDOR_ESMT_512M_DDR2;
		ddr_capacity = DDR_CAPACITY_512M;
		//pkg = PKG_BGA;
		ddr_type = DDR_TYPE_DDR2;
		break;
	case 0x5: //SIP 2Gb DDR3
		ddr_vendor = DDR_VENDOR_ETRON_1G;
		ddr_capacity = DDR_CAPACITY_1G;
		//pkg = PKG_QFN;
		ddr_type = DDR_TYPE_DDR3;
		break;
	case 0x6: //SIP 1Gb DDR3
		ddr_vendor = DDR_VENDOR_ESMT_2G;
		ddr_capacity = DDR_CAPACITY_2G;
		//pkg = PKG_BGA;
		ddr_type = DDR_TYPE_DDR3;
		break;
	case 0x9: //ETRON 512Mb DDR2
		ddr_vendor = DDR_VENDOR_ETRON_512M_DDR2;
		ddr_capacity = DDR_CAPACITY_512M;
		//pkg = PKG_BGA;
		ddr_type = DDR_TYPE_DDR2;
		break;
	case 0xa: //ESMT 1Gb DDR3 25nm
		ddr_vendor = DDR_VENDOR_ESMT_N25_1G;
		ddr_capacity = DDR_CAPACITY_1G;
		//pkg = PKG_QFN;
		ddr_type = DDR_TYPE_DDR3;
		break;
	case 0xb: //NY 1Gb DDR3
		ddr_vendor = DDR_VENDOR_NY_N20_1G;
		ddr_capacity = DDR_CAPACITY_1G;
		//pkg = PKG_QFN;
		ddr_type = DDR_TYPE_DDR3;
		break;
	default:
		NOTICE("unknown pkg_type=0x%x\n", pkg_type);
	}

	NOTICE("D%x_%x_%x\n", pkg, ddr_capacity, ddr_vendor);

	// 2nd source: ddr_type determined by vendor read from efuse
	if (pkg_type == 0x0) {
		switch (ddr_vendor) {
		case DDR_VENDOR_ESMT_512M_DDR2:
		case DDR_VENDOR_ETRON_512M_DDR2:
		case DDR_VENDOR_UNILC_N25_512M_DDR2:
			ddr_type = DDR_TYPE_DDR2;
			break;
		default:
			ddr_type = DDR_TYPE_DDR3;
			break;
		}
	}

	//if onebin, need to set datarate here

	pkg = FIELD_GET(efuse_leakage, 31, 29);
	//pkg = PKG_QFN;

	switch (ddr_capacity) {
	case DDR_CAPACITY_512M:
		NOTICE("-512M");
		chip_id = (pkg == PKG_BGA ? 0x1840 : 0x0);
		break;
	case DDR_CAPACITY_1G:
		NOTICE("-1G");
		chip_id = (pkg == PKG_QFN ? 0x1841c : 0x0);
		break;
	case DDR_CAPACITY_2G:
		NOTICE("-2G");
		chip_id = (pkg == PKG_QFN ? 0x1842c : 0x1842);
		break;
	case DDR_CAPACITY_4G:
		NOTICE("-4G");
		chip_id = (pkg == PKG_BGA ? 0x1843 : 0x0);
		break;
	default:
		NOTICE("-unknown capacity=%d", ddr_capacity);
		chip_id = 0x0;
	}
	mmio_write_32(REG_GP_REG3, chip_id);

	if (pkg == 0b010)
		NOTICE("-BGA\n");
	else
		NOTICE("-QFN\n");
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
