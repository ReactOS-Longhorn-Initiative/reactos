#include "cdd.h"
#include <debug.h>

static DRVFN DrvFunctionTable[] =
{
   {INDEX_DrvEnablePDEV, (PFN)DrvEnablePDEV},
   {INDEX_DrvCompletePDEV, (PFN)DrvCompletePDEV},
   {INDEX_DrvDisablePDEV, (PFN)DrvDisablePDEV},
   {INDEX_DrvEnableSurface, (PFN)DrvEnableSurface},
   {INDEX_DrvDisableSurface, (PFN)DrvDisableSurface},
   {INDEX_DrvAssertMode, (PFN)DrvAssertMode},
   {INDEX_DrvNotify, (PFN)DrvNotify},
   {INDEX_DrvDisableDriver,(PFN) DrvDisableDriver},
   {INDEX_DrvGetModes, (PFN)DrvGetModes},
   {INDEX_DrvSetPalette, (PFN)DrvSetPalette},
   {INDEX_DrvSetPointerShape, (PFN)DrvSetPointerShape},
   {INDEX_DrvMovePointer, (PFN)DrvMovePointer},
   {INDEX_DrvIcmSetDeviceGammaRamp, (PFN)DrvIcmSetDeviceGammaRamp},
   {INDEX_DrvSynchronizeSurface, (PFN)DrvSynchronizeSurface},
   {INDEX_DrvStrokePath, (PFN)DrvStrokePath},
   {INDEX_DrvTransparentBlt, (PFN)DrvTransparentBlt},
   {INDEX_DrvBitBlt, (PFN)DrvBitBlt},
   {INDEX_DrvCopyBits, (PFN)DrvCopyBits},
   {INDEX_DrvTextOut, (PFN)DrvTextOut},
   {INDEX_DrvLineTo, (PFN)DrvLineTo},
   {INDEX_DrvFillPath, (PFN)DrvFillPath},
   {INDEX_DrvStrokeAndFillPath, (PFN)DrvStrokeAndFillPath},
   {INDEX_DrvStretchBltROP, (PFN)DrvStretchBltROP},
   {INDEX_DrvPlgBlt, (PFN)DrvPlgBlt},
};

/*
 * Canonical Display Driver is a XDDM Framebuf like driver to handle
 * the WDDM Monitor resolution changes and Pointer accleration
 *
 * Even in windows 11 this driver handles these operations.
 * Eventually more stuff got passed to cdd.dll handling more and more so we will
 * eventually build on this but for now.
 *
 * This is the first real rendering requests for WDDM
 */
BOOL
APIENTRY
DrvEnableDriver(ULONG iEngineVersion,
                ULONG cj,
                PDRVENABLEDATA pded)
{
    DPRINT1("---ReactOS CDD - ReactOS Display Driver Model---\n");
    __debugbreak();
    if (cj >= sizeof(DRVENABLEDATA))
    {
       pded->c = sizeof(DrvFunctionTable) / sizeof(DRVFN);
       pded->pdrvfn = DrvFunctionTable;
       pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;
       return TRUE;
    }
    else
    {
       return FALSE;
    }
}

VOID
APIENTRY
DrvDisableDriver()
{
    /* Nothing to do here as CDD doesn't handle adapter*/
}

VOID
APIENTRY
DrvNotify(_In_ SURFOBJ *pso,
          _In_ ULONG    iType,
          _In_ PVOID    pvData)
{
    UNIMPLEMENTED;
  // .. __debugbreak();
}

BOOL
APIENTRY
DrvAssertMode(_In_ DHPDEV dhpdev,
              _In_ BOOL bEnable)
{
    UNIMPLEMENTED;
 //   __debugbreak();
    return 0;
}