/*
 * PROJECT:     ReactOS Win32k subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     GDI side of DWM window content redirection
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * A REDIRECTION DC is a window DC whose pixels land in a per-window bitmap
 * instead of on the primary surface. Applications are not aware of it: they
 * keep calling GetDC/BeginPaint and drawing in window coordinates, and win32k
 * swaps the surface and the origin underneath them. DWM then composites those
 * bitmaps, which is what lets it draw windows that are occluded, animated or
 * scaled without asking anybody to repaint.
 *
 * Vista's win32k carries this as a distinct DC type reached through three
 * entry points, named here as they are named there (win32k.sys symbols):
 *
 *     _GreConvertMemToRedirectionDC@8
 *     _GreConvertRedirectionToMemDC@8
 *     _GreSelectRedirectionBitmap@8
 *
 * The separate type is not decoration. NtGdiSelectBitmap refuses anything that
 * is not DCTYPE_MEMORY, and a window DC is DCTYPE_DIRECT; without a third type
 * the choice would be to weaken that check for every caller or to lie about a
 * window DC being a memory DC, and a memory DC is wrong here -- it has no PDEV
 * relationship, and the DC has to keep behaving like a display DC for
 * everything except where its bits go.
 */

#include <win32k.h>

DBG_DEFAULT_CHANNEL(GdiBlt);

/*
 * The exclusivity rule that NtGdiSelectBitmap enforces through SURFACE::hdc --
 * one bitmap, one DC -- is deliberately NOT enforced here.
 *
 * A redirection bitmap belongs to the WINDOW, not to a DC. A window can have
 * several live DCEs at once (a cache DC from GetDC while a class DC or an
 * owned DC exists), and Vista retargets all of them together --
 * _UpdateRedirectedDCs@0 walks the DC list rather than reselecting one at a
 * time. Taking SURFACE::hdc here would make the second DCE for the same window
 * fail to select and silently fall back to drawing on the primary, which
 * presents as one window painting straight through the composition.
 */
HBITMAP
NTAPI
GreSelectRedirectionBitmap(
    _In_ HDC hdc,
    _In_ HBITMAP hbm)
{
    PDC pdc;
    PSURFACE psurfNew, psurfOld;
    HBITMAP hbmOld = NULL;

    if (hdc == NULL)
        return NULL;

    pdc = DC_LockDc(hdc);
    if (!pdc)
        return NULL;

    /* Only a converted DC may take one. A DIRECT DC that reached here has not
     * been through GreConvertMemToRedirectionDC and would keep its PDEV
     * surface semantics while pointing at a bitmap. */
    if (pdc->dctype != DCTYPE_REDIRECTION)
    {
        DC_UnlockDc(pdc);
        return NULL;
    }

    psurfOld = pdc->dclevel.pSurface;
    if (psurfOld)
        hbmOld = (HBITMAP)psurfOld->BaseObject.hHmgr;

    if (hbm == NULL)
    {
        /* Detach: back to no surface. The caller is expected to convert the
         * DC back to DIRECT straight after. */
        DC_vSelectSurface(pdc, NULL);
        pdc->dclevel.sizl.cx = 0;
        pdc->dclevel.sizl.cy = 0;
        DC_UnlockDc(pdc);
        return hbmOld;
    }

    psurfNew = SURFACE_ShareLockSurface(hbm);
    if (!psurfNew)
    {
        DC_UnlockDc(pdc);
        return NULL;
    }

    /* Must be an API bitmap; a device surface here would mean the redirection
     * bitmap was never created by IntDwmCreateRedirectionBitmap. */
    if (!(psurfNew->flags & API_BITMAP))
    {
        SURFACE_ShareUnlockSurface(psurfNew);
        DC_UnlockDc(pdc);
        return NULL;
    }

    DC_vSelectSurface(pdc, psurfNew);
    pdc->dclevel.sizl = psurfNew->SurfObj.sizlBitmap;

    /* DC_vSelectSurface took its own reference. */
    SURFACE_ShareUnlockSurface(psurfNew);

    DC_UnlockDc(pdc);
    return hbmOld;
}

/*
 * Vista _GreConvertMemToRedirectionDC@8. Puts a window DC into the redirected
 * state so GreSelectRedirectionBitmap will accept a bitmap for it.
 *
 * The name is Vista's and reads backwards: the second argument is the
 * direction, and the "Mem" in it is about the DC acquiring bitmap-backed
 * (memory-like) behaviour, not about DCTYPE_MEMORY.
 */
BOOL
NTAPI
GreConvertMemToRedirectionDC(
    _In_ HDC hdc,
    _In_ BOOL bRedirect)
{
    PDC pdc;
    BOOL bRet = FALSE;

    if (hdc == NULL)
        return FALSE;

    pdc = DC_LockDc(hdc);
    if (!pdc)
        return FALSE;

    if (bRedirect)
    {
        if (pdc->dctype == DCTYPE_DIRECT)
        {
            pdc->dctype = DCTYPE_REDIRECTION;
            bRet = TRUE;
        }
        else
        {
            /* Already redirected is success; anything else is not a window
             * DC and must not be touched. */
            bRet = (pdc->dctype == DCTYPE_REDIRECTION);
        }
    }
    else
    {
        if (pdc->dctype == DCTYPE_REDIRECTION)
        {
            PSURFACE psurfPdev;

            /*
             * PUT THE DC BACK ON THE PRIMARY SURFACE, not merely back to type.
             *
             * The caller reaches here through GreConvertRedirectionToMemDC,
             * which has already detached the bitmap -- so pSurface is NULL at
             * this instant. Flipping the type alone produces a DCTYPE_DIRECT
             * DC with a NULL surface, and that combination does not otherwise
             * exist in GDI: every DIRECT DC is created holding the PDEV
             * surface. Nothing downstream guards against it.
             *
             * DC_vPrepareDCsForBlit is where it detonates. It tests
             *
             *     dctype == DCTYPE_DIRECT && ppdev->pSurface != dclevel.pSurface
             *
             * which a NULL surface satisfies, and calls DC_vUpdateDC -- whose
             * first act is SURFACE_ShareUnlockSurface(pSurface). That macro is
             * a bare GDIOBJ_vDereferenceObject with no NULL check, so the next
             * blit into such a DC faults in the kernel. It came back as
             * taskmgr drawing a tab through uxtheme's AlphaBlend.
             *
             * PDEVOBJ_pSurface returns the surface with a reference already
             * taken, and DC_vSelectSurface takes its own, so the extra one is
             * dropped here -- the same acquire/select/release shape
             * GreSelectRedirectionBitmap uses for the bitmap.
             */
            psurfPdev = PDEVOBJ_pSurface(pdc->ppdev);
            if (psurfPdev != NULL)
            {
                DC_vSelectSurface(pdc, psurfPdev);
                SURFACE_ShareUnlockSurface(psurfPdev);

                /* And the extent, which the detach zeroed. A DIRECT DC left
                 * 0x0 clips every subsequent draw away silently. */
                PDEVOBJ_sizl(pdc->ppdev, &pdc->dclevel.sizl);

                pdc->dctype = DCTYPE_DIRECT;
                bRet = TRUE;
            }
            else
            {
                /* No primary surface to go back to. Leaving the DC typed
                 * REDIRECTION keeps it out of the DIRECT blit path above,
                 * which is the safe side of this failure. */
                ERR("GreConvertMemToRedirectionDC: no PDEV surface\n");
                bRet = FALSE;
            }
        }
        else
        {
            bRet = (pdc->dctype == DCTYPE_DIRECT);
        }
    }

    DC_UnlockDc(pdc);
    return bRet;
}

/*
 * Vista _GreConvertRedirectionToMemDC@8. The counterpart: drop the bitmap and
 * put the DC back on the primary surface.
 *
 * Order matters. The surface has to go before the type changes, because
 * GreSelectRedirectionBitmap refuses a DC that is no longer
 * DCTYPE_REDIRECTION, and a DIRECT DC still holding a bitmap surface would
 * blt window content into a bitmap nobody composites.
 */
BOOL
NTAPI
GreConvertRedirectionToMemDC(
    _In_ HDC hdc,
    _In_ BOOL bUnredirect)
{
    if (hdc == NULL)
        return FALSE;

    if (bUnredirect)
    {
        (VOID)GreSelectRedirectionBitmap(hdc, NULL);
        return GreConvertMemToRedirectionDC(hdc, FALSE);
    }

    return GreConvertMemToRedirectionDC(hdc, TRUE);
}

/*
 * Vista DxgkEngIsRedirectionDC. Used by the display path to tell a redirected
 * window DC from a real device DC -- accelerated blits and DirectX presents
 * must not take the device path when the target is a redirection bitmap.
 */
BOOL
NTAPI
GreIsRedirectionDC(
    _In_ HDC hdc)
{
    PDC pdc;
    BOOL bRet;

    if (hdc == NULL)
        return FALSE;

    pdc = DC_LockDc(hdc);
    if (!pdc)
        return FALSE;

    bRet = (pdc->dctype == DCTYPE_REDIRECTION);

    DC_UnlockDc(pdc);
    return bRet;
}

/*
 * The bitmap's PIXEL size.
 *
 * Not GreGetBitmapDimension: that returns the logical dimension
 * SetBitmapDimensionEx stores, in 0.1mm units, which is (0,0) on every bitmap
 * nobody has called that on -- so a size comparison against it would report
 * every redirection bitmap as 0x0 and reallocate on every single resize
 * notification, including pure moves.
 */
BOOL
NTAPI
GreGetBitmapPixelSize(
    _In_ HBITMAP hbm,
    _Out_ PSIZEL psizl)
{
    PSURFACE psurf;

    if (hbm == NULL || psizl == NULL)
        return FALSE;

    psurf = SURFACE_ShareLockSurface(hbm);
    if (psurf == NULL)
        return FALSE;

    *psizl = psurf->SurfObj.sizlBitmap;

    SURFACE_ShareUnlockSurface(psurf);
    return TRUE;
}

/* EOF */
