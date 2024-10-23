#include "cdd.h"
#include <debug.h>



BOOL
APIENTRY
DrvBitBlt(_Inout_ SURFOBJ  *psoTrg,
          _In_opt_ SURFOBJ  *psoSrc,
          _In_opt_ SURFOBJ  *psoMask,
          _In_ CLIPOBJ  *pco,
          _In_opt_ XLATEOBJ *pxlo,
          _In_ RECTL    *prclTrg,
          _In_opt_ POINTL   *pptlSrc,
          _In_opt_ POINTL   *pptlMask,
          _In_opt_ BRUSHOBJ *pbo,
          _In_opt_ POINTL   *pptlBrush,
          _In_ ROP4      rop4)
{
    return EngBitBlt(psoTrg,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     prclTrg,
                     pptlSrc,
                     pptlMask,
                     pbo,
                     pptlBrush,
                     rop4);
}


VOID APIENTRY DrvSynchronizeSurface(
    SURFOBJ *pso,
    RECTL   *prcl,
    FLONG    fl
)
{
    UNIMPLEMENTED;
    __debugbreak();
}

BOOL APIENTRY DrvStrokePath(
    _Inout_ SURFOBJ   *pso,
    _In_ PATHOBJ   *ppo,
    _In_ CLIPOBJ   *pco,
    _In_opt_ XFORMOBJ  *pxo,
    _In_ BRUSHOBJ  *pbo,
    _In_ POINTL    *pptlBrushOrg,
    _In_ LINEATTRS *plineattrs,
    _In_ MIX        mix
    )
{

    return EngStrokePath(pso, ppo, pco,
                    pxo, pbo, pptlBrushOrg, plineattrs, mix);
}

BOOL APIENTRY DrvTransparentBlt(
    _Inout_ SURFOBJ    *psoDst,
    _In_ SURFOBJ    *psoSrc,
    _In_ CLIPOBJ    *pco,
    _In_opt_ XLATEOBJ   *pxlo,
    _In_ RECTL      *prclDst,
    _In_ RECTL      *prclSrc,
    _In_ ULONG      iTransColor,
    _In_ ULONG      ulReserved)
{
    return EngTransparentBlt(psoDst, psoSrc,
                             pco, pxlo, prclDst,
                             prclSrc, iTransColor,
                              ulReserved);
}

BOOL
APIENTRY
DrvCopyBits(_Out_ SURFOBJ*  DestObj,
            _In_  SURFOBJ*  SourceObj,
            _In_  CLIPOBJ*  ClipObj,
            _In_  XLATEOBJ* XLateObj,
            _In_  RECTL*    DestRectL,
            _In_  POINTL*   SrcPointL)
{
    return  DrvBitBlt(DestObj, SourceObj, 0,
                        ClipObj, XLateObj, DestRectL,
                        SrcPointL, 0, 0, 0, FALSE);
}


BOOL APIENTRY DrvTextOut(
    _In_ SURFOBJ  *pso,
    _In_ STROBJ   *pstro,
    _In_ FONTOBJ  *pfo,
    _In_ CLIPOBJ  *pco,
    _In_ RECTL    *prclExtra,        // Obsolete, always NULL
    _In_ RECTL    *prclOpaque,
    _In_ BRUSHOBJ *pboFore,
    _In_ BRUSHOBJ *pboOpaque,
    _In_ POINTL   *pptlOrg,
    _In_ MIX       mix
    )
{
    return EngTextOut(pso, pstro, pfo, pco,
                      prclExtra, prclOpaque, pboFore,
                       pboOpaque, pptlOrg, mix);
}

BOOL APIENTRY
DrvLineTo(
    _In_ SURFOBJ *DestObj,
    _In_ CLIPOBJ *Clip,
    _In_ BRUSHOBJ *Brush,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_ RECTL *RectBounds,
    _In_ MIX mix)
{
    return EngLineTo(DestObj, Clip, Brush,
                    x1,  y1, x2, y2, RectBounds, mix);
}

BOOL APIENTRY DrvFillPath(
    _Inout_ SURFOBJ  *pso,
    _In_ PATHOBJ  *ppo,
    _In_ CLIPOBJ  *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL   *pptlBrushOrg,
    _In_ MIX       mix,
    _In_ FLONG     flOptions
    )
{
    return EngFillPath(pso, ppo, pco,
                      pbo, pptlBrushOrg,
                      mix, flOptions);
}

BOOL APIENTRY DrvStrokeAndFillPath(
    _Inout_ SURFOBJ   *pso,
    _Inout_ PATHOBJ   *ppo,
    _In_ CLIPOBJ   *pco,
    _In_opt_ XFORMOBJ  *pxo,
    _In_ BRUSHOBJ  *pboStroke,
    _In_ LINEATTRS *plineattrs,
    _In_ BRUSHOBJ  *pboFill,
    _In_ POINTL    *pptlBrushOrg,
    _In_ MIX        mixFill,
    _In_ FLONG      flOptions
    )
{
    return  EngStrokeAndFillPath(pso, ppo, pco,
                               pxo, pboStroke,
                               plineattrs, pboFill,
                              pptlBrushOrg,  mixFill, flOptions);
}

BOOL APIENTRY DrvStretchBltROP(
    _Inout_ SURFOBJ         *psoDest,
    _Inout_ SURFOBJ         *psoSrc,
    _In_opt_ SURFOBJ         *psoMask,
    _In_ CLIPOBJ         *pco,
    _In_opt_ XLATEOBJ        *pxlo,
    _In_opt_ COLORADJUSTMENT *pca,
    _In_ POINTL          *pptlHTOrg,
    _In_ RECTL           *prclDest,
    _In_ RECTL           *prclSrc,
    _In_opt_ POINTL          *pptlMask,
    _In_ ULONG            iMode,
    _In_ BRUSHOBJ        *pbo,
    _In_ DWORD            rop4
    )
{
    return EngStretchBltROP(psoDest, psoSrc, psoMask,
                            pco, pxlo, pca, pptlHTOrg,
                            prclDest, prclSrc, pptlMask,
                           iMode, pbo,  rop4);
}


BOOL APIENTRY DrvPlgBlt(
    _Inout_ SURFOBJ         *psoTrg,
    _Inout_ SURFOBJ         *psoSrc,
    _In_opt_ SURFOBJ         *psoMsk,
    _In_ CLIPOBJ         *pco,
    _In_opt_ XLATEOBJ        *pxlo,
    _In_opt_ COLORADJUSTMENT *pca,
    _In_opt_ POINTL          *pptlBrushOrg,
    _In_ POINTFIX        *pptfx,
    _In_ RECTL           *prcl,
    _In_opt_ POINTL          *pptl,
    _In_ ULONG            iMode
    )
{
    return EngPlgBlt(psoTrg, psoSrc, psoMsk,
                      pco, pxlo, pca, pptlBrushOrg,
                      pptfx, prcl, pptl, iMode);
}