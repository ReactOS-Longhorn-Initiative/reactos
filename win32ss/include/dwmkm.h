/*
 * Kernel / user shared layout for Longhorn-style DwmGetSurfaceData (build ~5048).
 * Milcore passes a pointer to this block; win32k fills it (7 pointer-sized fields).
 */
#pragma once

#include <windef.h>

typedef struct _DWM_SURFACE_KERNEL_OUT
{
    HANDLE hSection;
    ULONG_PTR Width;
    ULONG_PTR Height;
    ULONG_PTR PixelFormat;   /* SURFOBJ.iBitmapFormat */
    ULONG_PTR SurfaceFlags; /* SURFACE.flags subset / hints */
    ULONG_PTR StrideBytes;  /* abs(SURFOBJ.lDelta) */
    ULONG_PTR BlendState;   /* layered / MIL hints (0 if unknown) */
} DWM_SURFACE_KERNEL_OUT;

/* x86: 7 * 4 = 0x1C (matches LH win32k GreDwmGetSurfaceData copy size). */
