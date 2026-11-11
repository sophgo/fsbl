#include <platform.h>
#include <ddr.h>
#include <ddr_sys_bring_up.h>

#ifndef CONFIG_BOARD_palladium
#include <ddr_pkg_info.h>
#include <regconfig.h>
#endif


int ddr_init(const struct ddr_param *ddr_param)
{
	NOTICE("cv190x DDR init.\n");
	NOTICE("ddr_param[0]=0x%x.\n", ((const uint32_t *)ddr_param)[0]);

#ifndef NO_DDR_CFG
#ifndef CONFIG_BOARD_palladium
	read_ddr_pkg_info();
#ifndef DDR2_3
	ddr_patch_regs_init();
#endif
#endif
	ddr_sys_bring_up();
#endif //NO_DDR_CFG

	return 0;
}
