// StyleMapCompat.h - constants the Windows SDK vsstyle.h/vssym32.h omit but the
// ported ReactOS stylemap.c tables reference (GLOBALS parts/states, etc.).
// Values taken from ReactOS sdk/include/psdk/tmschema.h.
#pragma once

#ifndef TMT_STOCKIMAGEFILE
#define TMT_STOCKIMAGEFILE 3007
#endif

// GLOBALS class parts
#ifndef GP_BORDER
#define GP_BORDER 1
#define GP_LINEHORZ 2
#define GP_LINEVERT 3
#endif

// GLOBALS BORDER states
#ifndef BSS_FLAT
#define BSS_FLAT 1
#define BSS_RAISED 2
#define BSS_SUNKEN 3
#endif

// GLOBALS LINEHORZ states
#ifndef LHS_FLAT
#define LHS_FLAT 1
#define LHS_RAISED 2
#define LHS_SUNKEN 3
#endif

// GLOBALS LINEVERT states
#ifndef LVS_FLAT
#define LVS_FLAT 1
#define LVS_RAISED 2
#define LVS_SUNKEN 3
#endif
