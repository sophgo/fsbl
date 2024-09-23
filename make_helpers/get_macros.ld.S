#include <platform.h>
#include <cdefs.h>

#define MAKE_MACRO_TO_SYMBOL(macro) DEF_##macro = macro;

SECTIONS
{
	.text : {
		LONG(0xDEADBEEF);
		*(*)
	}

	MAKE_MACRO_TO_SYMBOL(DRAM_BASE)
	MAKE_MACRO_TO_SYMBOL(MONITOR_RUNADDR)
#ifdef ADD_BL32
	MAKE_MACRO_TO_SYMBOL(BL32_RUNADDR)
#endif
	MAKE_MACRO_TO_SYMBOL(BLCP_2ND_RUNADDR)
}
