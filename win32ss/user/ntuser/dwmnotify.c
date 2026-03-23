/*
 * Longhorn ~5048 DWM LPC notifications to milcore + per-window redirect bitmaps.
 *
 * Opcodes: 5–8 (top-level create/destroy/update/zorder), 9 display (OR DWM_DISPLAY_HINT_*),
 * 10 owner, 0x11 / 18–20 child create/destroy/zorder/update, 13 input (active root + focus top).
 */

#include <win32k.h>

#include "dce.h"
#include "dwm.h"
#include "dwmnotify.h"

DBG_DEFAULT_CHANNEL(UserMisc);

#include <debug.h>

extern BOOL NTAPI DC_bIsBitmapCompatible(_In_ PDC pdc, _In_ PSURFACE psurf);

/* Opcodes observed in Vista build 5048 win32k (Dwm* helpers). */
#define DWM_LPC_OP_TOPLEVEL_CREATE 5u
#define DWM_LPC_OP_TOPLEVEL_DESTROY 6u
#define DWM_LPC_OP_TOPLEVEL_UPDATE 7u
#define DWM_LPC_OP_TOPLEVEL_ZORDER 8u
#define DWM_LPC_OP_CHILD_CREATE 0x11u
#define DWM_LPC_OP_CHILD_DESTROY 18u
#define DWM_LPC_OP_CHILD_ZORDER 19u
/* Child layout after create (same payload shape as CHILD_CREATE; milcore may treat as update). */
#define DWM_LPC_OP_CHILD_UPDATE 20u
/* Applied display mode (kernel extension; notify milcore to rebuild targets). */
#define DWM_LPC_OP_DISPLAY_CHANGED 9u
#define DWM_LPC_OP_TOPLEVEL_OWNER 10u
/* Foreground queue snapshot: root(active), non-child-ancestor(focus). */
#define DWM_LPC_OP_INPUT_FOCUS 13u

/* TopLevelUpdate UpdateArg: pass2 startup uses 1; runtime layout/show/hide use 2-4. */
#define DWM_TOP_UPDATE_LAYOUT 2u
#define DWM_TOP_UPDATE_SHOW   3u
#define DWM_TOP_UPDATE_HIDE   4u
#define DWM_TOP_UPDATE_SHAPE  5u
#define DWM_TOP_UPDATE_LAYERED 6u
#define DWM_TOP_UPDATE_LAYERED_EXSTYLE 7u
#define DWM_TOP_UPDATE_CAPTION 10u

static VOID
IntDwmSendChildUpdate(_In_ PWND Wnd);

static VOID
DwmMsgInitHeader(_Out_writes_bytes_(sizeof(PORT_MESSAGE) + cbPayload) PPORT_MESSAGE H, USHORT cbPayload)
{
    USHORT hdr = (USHORT)sizeof(PORT_MESSAGE);

    RtlZeroMemory(H, hdr + cbPayload);
    H->u1.s1.DataLength = (CSHORT)cbPayload;
    H->u1.s1.TotalLength = (CSHORT)(hdr + cbPayload);
    H->u2.s2.Type = LPC_DATAGRAM;
    H->u2.s2.DataInfoOffset = 0;
}

static PUCHAR
DwmMsgPayload(_In_ PPORT_MESSAGE H)
{
    return (PUCHAR)H + sizeof(PORT_MESSAGE);
}

NTSTATUS
FASTCALL
IntDwmPrepareRedirectSurface(_In_ HDEV hdev, _In_ PWND Wnd)
{
    LONG w, h;
    PPDEVOBJ ppdev;
    PSURFACE psurfPrim;
    PSURFACE psNew;
    ULONG fmt;
    HBITMAP hbmp;

    if (!Wnd || !hdev)
    {
        DPRINT1("[DWM] PrepareRedirect: bad param Wnd=%p hdev=%p\n", Wnd, hdev);
        return STATUS_SUCCESS;
    }
    if (!gfbDwmCompositing)
        return STATUS_SUCCESS;

    /*
     * After a PDEV / display mode change the old bitmap may already be deleted while
     * the window still holds hbmpDwmRedirect — do not skip allocation in that case or
     * BindRedirect sees "no redirect hbmp after prepare".
     */
    if (Wnd->hbmDwmRedirect)
    {
        if (GreIsHandleValid(Wnd->hbmDwmRedirect))
            return STATUS_SUCCESS;
        DPRINT1("[DWM] PrepareRedirect: stale hbmp=%p Wnd=%p, recreating\n",
                Wnd->hbmDwmRedirect, Wnd);
        Wnd->hbmDwmRedirect = NULL;
    }

    w = Wnd->rcClient.right - Wnd->rcClient.left;
    h = Wnd->rcClient.bottom - Wnd->rcClient.top;
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384)
    {
        DPRINT1("[DWM] PrepareRedirect: bad client size Wnd=%p w=%ld h=%ld\n", Wnd, w, h);
        return STATUS_SUCCESS;
    }

    ppdev = (PPDEVOBJ)hdev;
    psurfPrim = PDEVOBJ_pSurface(ppdev);
    if (!psurfPrim)
    {
        DPRINT1("[DWM] PrepareRedirect: no primary surface ppdev=%p\n", ppdev);
        return STATUS_DEVICE_NOT_READY;
    }

    fmt = psurfPrim->SurfObj.iBitmapFormat;

    psNew = SURFACE_AllocSurface(STYPE_BITMAP,
                                 (ULONG)w,
                                 (ULONG)h,
                                 fmt,
                                 BMF_TOPDOWN,
                                 0,
                                 0,
                                 NULL);
    SURFACE_ShareUnlockSurface(psurfPrim);
    if (!psNew)
    {
        DPRINT1("[DWM] PrepareRedirect: SURFACE_AllocSurface failed Wnd=%p %ldx%ld fmt=%lu\n",
                Wnd, w, h, fmt);
        return STATUS_NO_MEMORY;
    }

    psNew->flags |= API_BITMAP;
    hbmp = (HBITMAP)psNew->SurfObj.hsurf;
    /*
     * AllocSurface leaves the bitmap exclusively locked. EngAssociateSurface uses
     * SURFACE_ShareLockSurface -> GDIOBJ_ReferenceObjectByHandle, which fails if
     * cExclusiveLock != 0 ("Cannot reference object ... with exclusive lock").
     */
    SURFACE_UnlockSurface(psNew);

    if (!EngAssociateSurface((HSURF)hbmp, (HDEV)ppdev, 0))
    {
        DPRINT1("[DWM] PrepareRedirect: EngAssociateSurface failed hbmp=%p ppdev=%p\n", hbmp, ppdev);
        GreDeleteObject(hbmp);
        return STATUS_UNSUCCESSFUL;
    }

    Wnd->hbmDwmRedirect = hbmp;
    GreSetBitmapOwner(Wnd->hbmDwmRedirect, GDI_OBJ_HMGR_POWNED);
    DPRINT1("[DWM] PrepareRedirect: OK Wnd=%p hbmp=%p %ldx%ld fmt=%lu\n", Wnd, hbmp, w, h, fmt);
    return STATUS_SUCCESS;
}

static VOID FASTCALL
DwmUnbindRedirectEnum(_In_ PDCE dce, _In_opt_ PVOID Context)
{
    PWND pwnd = (PWND)Context;

    if (!dce || !pwnd)
        return;
    if (dce->hwndCurrent != UserHMGetHandle(pwnd))
        return;
    if (!dce->fDwmRedirectBound)
        return;
    IntDwmUnbindRedirectDc(dce);
}

VOID
FASTCALL
IntDwmUnbindRedirectForWindow(_In_ PWND Wnd)
{
    if (!Wnd)
        return;
    DPRINT1("[DWM] UnbindRedirectForWindow Wnd=%p hwnd=%p\n", Wnd, UserHMGetHandle(Wnd));
    DceEnumerateAll(DwmUnbindRedirectEnum, Wnd);
}

static VOID FASTCALL
DwmUnbindAllRedirectEnum(_In_ PDCE dce, _In_opt_ PVOID Context)
{
    (void)Context;
    if (dce && dce->fDwmRedirectBound)
        IntDwmUnbindRedirectDc(dce);
}

VOID
FASTCALL
IntDwmUnbindAllActiveRedirectDcs(VOID)
{
    DPRINT1("[DWM] UnbindAllActiveRedirectDcs\n");
    DceEnumerateAll(DwmUnbindAllRedirectEnum, NULL);
}

VOID
FASTCALL
IntDwmUnbindRedirectDcLocked(_Inout_ PDC pdc, _Inout_ PDCE dce)
{
    PSURFACE psurfRedirect;

    if (!pdc || !dce || !dce->fDwmRedirectBound)
        return;

    /* hwndCurrent can be NULL if the DCE was detached before unbind (still must restore the DC). */
    DPRINT1("[DWM] UnbindRedirectDcLocked dce=%p hDC=%p hwndCurrent=%p\n",
            dce, dce->hDC, dce->hwndCurrent);

    psurfRedirect = pdc->dclevel.pSurface;
    if (psurfRedirect)
    {
        psurfRedirect->hdc = NULL;
        SURFACE_ShareUnlockSurface(psurfRedirect);
    }

    pdc->dclevel.pSurface = PDEVOBJ_pSurface(pdc->ppdev);
    if (pdc->dclevel.pSurface)
        PDEVOBJ_sizl(pdc->ppdev, &pdc->dclevel.sizl);

    pdc->ptlDCOrig = dce->DwmPtlDcOrigSave;
    pdc->erclWindow = dce->DwmErclWindowSave;
    pdc->ptlFillOrigin.x = pdc->dclevel.ptlBrushOrigin.x + pdc->ptlDCOrig.x;
    pdc->ptlFillOrigin.y = pdc->dclevel.ptlBrushOrigin.y + pdc->ptlDCOrig.y;

    pdc->fs &= ~DC_REDIRECTION;
    pdc->fs |= DC_DIRTY_RAO;
    dce->fDwmRedirectBound = FALSE;
}

VOID
FASTCALL
IntDwmUnbindRedirectDc(_Inout_ PDCE dce)
{
    PDC pdc;

    if (!dce || !dce->hDC)
        return;

    pdc = DC_LockDc(dce->hDC);
    if (!pdc)
    {
        DPRINT1("[DWM] UnbindRedirectDc: DC_LockDc failed hDC=%p (clearing flag)\n", dce->hDC);
        dce->fDwmRedirectBound = FALSE;
        return;
    }
    IntDwmUnbindRedirectDcLocked(pdc, dce);
    DC_UnlockDc(pdc);
}

VOID
FASTCALL
IntDwmFreeRedirectSurface(_In_ PWND Wnd)
{
    if (!Wnd || !Wnd->hbmDwmRedirect)
        return;

    DPRINT1("[DWM] FreeRedirectSurface Wnd=%p hbmp=%p\n", Wnd, Wnd->hbmDwmRedirect);
    IntDwmUnbindRedirectForWindow(Wnd);

    if (GreIsHandleValid(Wnd->hbmDwmRedirect))
        GreDeleteObject(Wnd->hbmDwmRedirect);
    Wnd->hbmDwmRedirect = NULL;
}

VOID
FASTCALL
IntDwmBindRedirectDcLocked(_Inout_ PDC pdc, _Inout_ PDCE dce, _In_opt_ PWND Wnd, _In_ ULONG DcxFlags)
{
    PSURFACE psurfRedirect;
    LONG w, h;
    RECTL ercl;
    HDEV hdev;

    if (!pdc || !dce || !Wnd)
    {
        DPRINT1("[DWM] BindRedirect: null pdc=%p dce=%p Wnd=%p\n", pdc, dce, Wnd);
        return;
    }
    if (!gfbDwmCompositing)
        return;
    if (DcxFlags & DCX_WINDOW)
    {
        DPRINT1("[DWM] BindRedirect: skip DCX_WINDOW hwnd=%p\n", UserHMGetHandle(Wnd));
        return;
    }
    if (DcxFlags & DCX_PARENTCLIP)
    {
        DPRINT1("[DWM] BindRedirect: skip DCX_PARENTCLIP hwnd=%p\n", UserHMGetHandle(Wnd));
        return;
    }
    if (dce->fDwmRedirectBound)
        return;

    w = Wnd->rcClient.right - Wnd->rcClient.left;
    h = Wnd->rcClient.bottom - Wnd->rcClient.top;
    if (w <= 0 || h <= 0)
    {
        DPRINT1("[DWM] BindRedirect: zero client hwnd=%p w=%ld h=%ld\n", UserHMGetHandle(Wnd), w, h);
        return;
    }

    if (!gpmdev || !gpmdev->ppdevGlobal)
    {
        DPRINT1("[DWM] BindRedirect: no gpmdev/ppdevGlobal hwnd=%p\n", UserHMGetHandle(Wnd));
        return;
    }
    hdev = (HDEV)gpmdev->ppdevGlobal;

    IntDwmPrepareRedirectSurface(hdev, Wnd);
    if (!Wnd->hbmDwmRedirect || !GreIsHandleValid(Wnd->hbmDwmRedirect))
    {
        DPRINT1("[DWM] BindRedirect: no redirect hbmp after prepare hwnd=%p\n", UserHMGetHandle(Wnd));
        return;
    }

    psurfRedirect = SURFACE_ShareLockSurface(Wnd->hbmDwmRedirect);
    if (!psurfRedirect)
    {
        DPRINT1("[DWM] BindRedirect: ShareLockSurface failed hbmp=%p hwnd=%p\n",
                Wnd->hbmDwmRedirect, UserHMGetHandle(Wnd));
        return;
    }

    if (psurfRedirect->SurfObj.sizlBitmap.cx != (ULONG)w ||
        psurfRedirect->SurfObj.sizlBitmap.cy != (ULONG)h)
    {
        DPRINT1("[DWM] BindRedirect: size mismatch hwnd=%p client=%ldx%ld surf=%lux%lu -> recreate\n",
                UserHMGetHandle(Wnd), w, h,
                psurfRedirect->SurfObj.sizlBitmap.cx, psurfRedirect->SurfObj.sizlBitmap.cy);
        SURFACE_ShareUnlockSurface(psurfRedirect);
        IntDwmUnbindRedirectForWindow(Wnd);
        IntDwmFreeRedirectSurface(Wnd);
        IntDwmPrepareRedirectSurface(hdev, Wnd);
        if (!Wnd->hbmDwmRedirect || !GreIsHandleValid(Wnd->hbmDwmRedirect))
            return;
        psurfRedirect = SURFACE_ShareLockSurface(Wnd->hbmDwmRedirect);
        if (!psurfRedirect)
        {
            DPRINT1("[DWM] BindRedirect: ShareLock after recreate failed hwnd=%p\n", UserHMGetHandle(Wnd));
            return;
        }
    }

    if (!DC_bIsBitmapCompatible(pdc, psurfRedirect))
    {
        DPRINT1("[DWM] BindRedirect: !DC_bIsBitmapCompatible hwnd=%p surfFmt=%lu pdevBpp=%u\n",
                UserHMGetHandle(Wnd),
                psurfRedirect->SurfObj.iBitmapFormat,
                pdc->ppdev ? pdc->ppdev->gdiinfo.cBitsPixel : 0);
        SURFACE_ShareUnlockSurface(psurfRedirect);
        return;
    }

    dce->DwmPtlDcOrigSave = pdc->ptlDCOrig;
    dce->DwmErclWindowSave = pdc->erclWindow;

    DC_vSelectSurface(pdc, psurfRedirect);
    SURFACE_ShareUnlockSurface(psurfRedirect);

    ercl.left = 0;
    ercl.top = 0;
    ercl.right = w;
    ercl.bottom = h;
    pdc->erclWindow = ercl;
    pdc->ptlDCOrig.x = 0;
    pdc->ptlDCOrig.y = 0;
    pdc->dclevel.sizl.cx = w;
    pdc->dclevel.sizl.cy = h;
    pdc->ptlFillOrigin.x = pdc->dclevel.ptlBrushOrigin.x;
    pdc->ptlFillOrigin.y = pdc->dclevel.ptlBrushOrigin.y;

    pdc->fs |= DC_REDIRECTION | DC_DIRTY_RAO;
    dce->fDwmRedirectBound = TRUE;
    DPRINT1("[DWM] BindRedirect: OK hwnd=%p hDC=%p dce=%p %ldx%ld DCX=%#lx\n",
            UserHMGetHandle(Wnd), dce->hDC, dce, w, h, DcxFlags);
}

/* 5048 payload: 10 ULONG window snapshot (window rect, client rect, style, exstyle). */
static VOID
IntDwmFillMiniWinInfo(_In_ PWND Wnd, _Out_writes_(10) PULONG Mini)
{
    Mini[0] = (ULONG)(LONG)Wnd->rcWindow.left;
    Mini[1] = (ULONG)(LONG)Wnd->rcWindow.top;
    Mini[2] = (ULONG)(LONG)Wnd->rcWindow.right;
    Mini[3] = (ULONG)(LONG)Wnd->rcWindow.bottom;
    Mini[4] = (ULONG)(LONG)Wnd->rcClient.left;
    Mini[5] = (ULONG)(LONG)Wnd->rcClient.top;
    Mini[6] = (ULONG)(LONG)Wnd->rcClient.right;
    Mini[7] = (ULONG)(LONG)Wnd->rcClient.bottom;
    Mini[8] = Wnd->style;
    Mini[9] = Wnd->ExStyle;
}

VOID
FASTCALL
IntDwmTopLevelCreate(_In_ PWND Wnd, _In_opt_ PRECTL prcIn, _In_ ULONG HintFlagsAnd1)
{
    UCHAR Buf[256];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND) + sizeof(RECTL) + sizeof(ULONG) + (10 * sizeof(ULONG));
    RECTL rcFallback;

    if (!Wnd)
        return;

    if (!prcIn)
    {
        rcFallback = Wnd->rcWindow;
        prcIn = &rcFallback;
    }

    if (sizeof(PORT_MESSAGE) + cbPayload > sizeof(Buf))
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_TOPLEVEL_CREATE;
    pl += sizeof(ULONG);
    {
        HWND h = UserHMGetHandle(Wnd);
        RtlCopyMemory(pl, &h, sizeof(HWND));
        pl += sizeof(HWND);
    }
    RtlCopyMemory(pl, prcIn, sizeof(RECTL));
    pl += sizeof(RECTL);
    *(PULONG)pl = HintFlagsAnd1 & 1u;
    pl += sizeof(ULONG);
    {
        ULONG mini[10];
        IntDwmFillMiniWinInfo(Wnd, mini);
        RtlCopyMemory(pl, mini, sizeof(mini));
    }

    {
        HWND hLog = UserHMGetHandle(Wnd);
        DPRINT1("[DWM] LPC TopLevelCreate hwnd=%p hintBit=%lu\n", hLog, HintFlagsAnd1 & 1u);
    }
    IntDwmSendLpcDatagram(H);
}

VOID
FASTCALL
IntDwmTopLevelUpdate(_In_ PWND Wnd, _In_ ULONG UpdateArg, _In_opt_ PRECTL prcOptional)
{
    UCHAR Buf[256];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND) + sizeof(ULONG) + sizeof(ULONG) + sizeof(RECTL) + (10 * sizeof(ULONG));
    ULONG mini[10];

    if (!Wnd)
        return;

    if (sizeof(PORT_MESSAGE) + cbPayload > sizeof(Buf))
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_TOPLEVEL_UPDATE;
    pl += sizeof(ULONG);
    {
        HWND h = UserHMGetHandle(Wnd);
        RtlCopyMemory(pl, &h, sizeof(HWND));
        pl += sizeof(HWND);
    }
    *(PULONG)pl = UpdateArg;
    pl += sizeof(ULONG);
    if (prcOptional)
    {
        *(PULONG)pl = 1u;
        pl += sizeof(ULONG);
        RtlCopyMemory(pl, prcOptional, sizeof(RECTL));
    }
    else
    {
        *(PULONG)pl = 0u;
        pl += sizeof(ULONG);
        RtlZeroMemory(pl, sizeof(RECTL));
    }
    pl += sizeof(RECTL);
    IntDwmFillMiniWinInfo(Wnd, mini);
    RtlCopyMemory(pl, mini, sizeof(mini));

    DPRINT1("[DWM] LPC TopLevelUpdate hwnd=%p UpdateArg=%lu hasRect=%u\n",
            UserHMGetHandle(Wnd), UpdateArg, prcOptional ? 1u : 0u);
    IntDwmSendLpcDatagram(H);
}

static VOID FASTCALL
DwmGreStartupEnumeratePass1(_In_ PDCE dce, _In_opt_ PVOID Context)
{
    PWND pwnd;
    PRECTL prc;
    ULONG hint;

    (void)Context;

    if (!dce || (dce->DCXFlags & DCX_DCEEMPTY))
        return;
    if (dce->AllocType == DCE_CACHE_DC)
        return;

    pwnd = dce->pwndOrg;
    if (!pwnd && dce->hwndCurrent)
        pwnd = UserGetWindowObject(dce->hwndCurrent);
    if (!pwnd || UserIsDesktopWindow(pwnd))
        return;
    if (pwnd->style & WS_CHILD)
        return;

    prc = (dce->DCXFlags & DCX_WINDOW) ? &pwnd->rcWindow : &pwnd->rcClient;
    hint = (dce->DCXFlags & DCX_WINDOW) ? 1u : 0u;
    DPRINT1("[DWM] GreStartup pass1 DCE=%p hwnd=%p AllocType=%d DCX=%#lx\n",
            dce, dce->hwndCurrent, (int)dce->AllocType, dce->DCXFlags);
    IntDwmTopLevelCreate(pwnd, prc, hint);
}

static VOID FASTCALL
DwmGreStartupEnumeratePass2(_In_ PDCE dce, _In_opt_ PVOID Context)
{
    PWND pwnd;

    (void)Context;

    if (!dce || dce->AllocType != DCE_CACHE_DC)
        return;
    if (dce->DCXFlags & DCX_DCEEMPTY)
        return;

    pwnd = dce->pwndOrg;
    if (!pwnd && dce->hwndCurrent)
        pwnd = UserGetWindowObject(dce->hwndCurrent);
    if (!pwnd || UserIsDesktopWindow(pwnd))
        return;
    if (pwnd->style & WS_CHILD)
        return;

    DPRINT1("[DWM] GreStartup pass2 DCE=%p hwnd=%p\n", dce, dce->hwndCurrent);
    IntDwmTopLevelUpdate(pwnd, 1, NULL);
}

VOID
FASTCALL
IntDwmGreStartupWalkDceList(_In_ HDEV hdev)
{
    DPRINT1("[DWM] GreStartupWalkDceList hdev=%p\n", hdev);
    DceEnumerateAll(DwmGreStartupEnumeratePass1, NULL);
    DceEnumerateAll(DwmGreStartupEnumeratePass2, NULL);
    DPRINT1("[DWM] GreStartupWalkDceList done\n");
}

static VOID FASTCALL
DwmFreeRedirectSurfacesInSubtree(_In_ PWND Wnd)
{
    PWND Child;

    if (!Wnd)
        return;

    for (Child = Wnd->spwndChild; Child; Child = Child->spwndNext)
    {
        if (Child->hbmDwmRedirect)
            IntDwmFreeRedirectSurface(Child);
        DwmFreeRedirectSurfacesInSubtree(Child);
    }
}

VOID
FASTCALL
IntDwmFreeAllRedirectBitmaps(VOID)
{
    PWND Desktop = UserGetDesktopWindow();

    if (Desktop)
        DwmFreeRedirectSurfacesInSubtree(Desktop);
}

VOID
FASTCALL
IntDwmOnWindowCaptionChanged(_In_ PWND Wnd)
{
    if (!gfbDwmCompositing || !Wnd || UserIsDesktopWindow(Wnd))
        return;

    if (Wnd->style & WS_CHILD)
        IntDwmSendChildUpdate(Wnd);
    else
        IntDwmTopLevelUpdate(Wnd, DWM_TOP_UPDATE_CAPTION, &Wnd->rcWindow);
}

VOID
FASTCALL
IntDwmNotifyDisplaySettingsChanged(_In_ DWORD CdsFlags, _In_ ULONG OldBitCount)
{
    UCHAR Buf[128];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + 4 * sizeof(ULONG);
    PWND Desktop;

    if (!gfbDwmCompositing)
        return;

    IntDwmUnbindAllActiveRedirectDcs();

    if (OldBitCount != gpsi->BitCount)
    {
        Desktop = UserGetDesktopWindow();
        if (Desktop)
        {
            DPRINT1("[DWM] Display BitCount %lu -> %lu: freeing redirect surfaces\n",
                    OldBitCount, (ULONG)gpsi->BitCount);
            DwmFreeRedirectSurfacesInSubtree(Desktop);
        }
    }

    if (sizeof(PORT_MESSAGE) + cbPayload > sizeof(Buf))
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_DISPLAY_CHANGED;
    pl += sizeof(ULONG);
    *(PULONG)pl = CdsFlags;
    pl += sizeof(ULONG);
    *(PULONG)pl = gpsi->aiSysMet[SM_CXSCREEN];
    pl += sizeof(ULONG);
    *(PULONG)pl = gpsi->aiSysMet[SM_CYSCREEN];
    pl += sizeof(ULONG);
    *(PULONG)pl = gpsi->BitCount;

    DPRINT1("[DWM] LPC DisplayChanged flags=%#lx screen=%lux%lu BitCount=%lu\n",
            CdsFlags,
            (ULONG)gpsi->aiSysMet[SM_CXSCREEN],
            (ULONG)gpsi->aiSysMet[SM_CYSCREEN],
            (ULONG)gpsi->BitCount);
    IntDwmSendLpcDatagram(H);
}

VOID
FASTCALL
IntDwmSendForegroundInputLpc(VOID)
{
    UCHAR Buf[128];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND) + sizeof(HWND);
    PUSER_MESSAGE_QUEUE fq = gpqForeground;
    HWND hActiveRoot = NULL;
    HWND hFocusTop = NULL;
    PWND pwnd;

    if (!gfbDwmCompositing)
        return;

    if (fq && fq->spwndActive)
    {
        pwnd = UserGetAncestor(fq->spwndActive, GA_ROOT);
        if (pwnd)
            hActiveRoot = UserHMGetHandle(pwnd);
    }
    if (fq && fq->spwndFocus)
    {
        pwnd = IntGetNonChildAncestor(fq->spwndFocus);
        if (pwnd)
            hFocusTop = UserHMGetHandle(pwnd);
    }

    if (sizeof(PORT_MESSAGE) + cbPayload > sizeof(Buf))
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_INPUT_FOCUS;
    pl += sizeof(ULONG);
    RtlCopyMemory(pl, &hActiveRoot, sizeof(HWND));
    pl += sizeof(HWND);
    RtlCopyMemory(pl, &hFocusTop, sizeof(HWND));

    DPRINT1("[DWM] LPC InputFocus activeRoot=%p focusTop=%p\n", hActiveRoot, hFocusTop);
    IntDwmSendLpcDatagram(H);
}

VOID
FASTCALL
IntDwmOnNonClientStateHintChanged(_In_ PWND Wnd)
{
    if (!gfbDwmCompositing || !Wnd || UserIsDesktopWindow(Wnd))
        return;

    if (Wnd->style & WS_CHILD)
        IntDwmSendChildUpdate(Wnd);
    else
        IntDwmTopLevelUpdate(Wnd, DWM_TOP_UPDATE_LAYOUT, &Wnd->rcWindow);
}

VOID
FASTCALL
IntDwmOnWindowShapeChanged(_In_ PWND Wnd)
{
    if (!gfbDwmCompositing || !Wnd || UserIsDesktopWindow(Wnd))
        return;

    if (Wnd->style & WS_CHILD)
        IntDwmSendChildUpdate(Wnd);
    else
        IntDwmTopLevelUpdate(Wnd, DWM_TOP_UPDATE_SHAPE, &Wnd->rcWindow);
}

VOID
FASTCALL
IntDwmOnLayeredPresentationChanged(_In_ PWND Wnd)
{
    if (!gfbDwmCompositing || !Wnd || UserIsDesktopWindow(Wnd))
        return;

    if (Wnd->style & WS_CHILD)
        IntDwmSendChildUpdate(Wnd);
    else
        IntDwmTopLevelUpdate(Wnd, DWM_TOP_UPDATE_LAYERED, &Wnd->rcWindow);
}

VOID
FASTCALL
IntDwmOnExStyleLayeredToggle(_In_ PWND Wnd)
{
    if (!gfbDwmCompositing || !Wnd || UserIsDesktopWindow(Wnd))
        return;

    if (Wnd->style & WS_CHILD)
        IntDwmSendChildUpdate(Wnd);
    else
        IntDwmTopLevelUpdate(Wnd, DWM_TOP_UPDATE_LAYERED_EXSTYLE, &Wnd->rcWindow);
}

VOID
FASTCALL
IntDwmOnVisibleStyleChanged(_In_ PWND Wnd, _In_ BOOL NowVisible)
{
    ULONG updateArg;

    if (!gfbDwmCompositing || !Wnd || UserIsDesktopWindow(Wnd))
        return;

    updateArg = NowVisible ? DWM_TOP_UPDATE_SHOW : DWM_TOP_UPDATE_HIDE;

    if (Wnd->style & WS_CHILD)
        IntDwmSendChildUpdate(Wnd);
    else
        IntDwmTopLevelUpdate(Wnd, updateArg, &Wnd->rcWindow);
}

static VOID
IntDwmSendOwnerChanged(_In_ PWND Wnd)
{
    UCHAR Buf[128];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND) * 2;

    if (!gfbDwmCompositing || !Wnd)
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_TOPLEVEL_OWNER;
    pl += sizeof(ULONG);
    {
        HWND h = UserHMGetHandle(Wnd);
        RtlCopyMemory(pl, &h, sizeof(HWND));
        pl += sizeof(HWND);
    }
    {
        HWND hOwner = Wnd->spwndOwner ? UserHMGetHandle(Wnd->spwndOwner) : NULL;
        RtlCopyMemory(pl, &hOwner, sizeof(HWND));
    }

    DPRINT1("[DWM] LPC TopLevelOwner hwnd=%p owner=%p\n",
            UserHMGetHandle(Wnd),
            Wnd->spwndOwner ? UserHMGetHandle(Wnd->spwndOwner) : NULL);
    IntDwmSendLpcDatagram(H);
}

VOID
FASTCALL
IntDwmOnOwnerChanged(_In_ PWND Wnd)
{
    if (!gfbDwmCompositing || !Wnd || UserIsDesktopWindow(Wnd))
        return;
    if (Wnd->style & WS_CHILD)
        return;

    IntDwmSendOwnerChanged(Wnd);
}

static VOID
IntDwmSendTopLevelDestroy(_In_ PWND Wnd)
{
    UCHAR Buf[128];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND);

    if (!gfbDwmCompositing || !Wnd)
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_TOPLEVEL_DESTROY;
    pl += sizeof(ULONG);
    {
        HWND h = UserHMGetHandle(Wnd);
        RtlCopyMemory(pl, &h, sizeof(HWND));
    }

    DPRINT1("[DWM] LPC TopLevelDestroy hwnd=%p\n", UserHMGetHandle(Wnd));
    IntDwmSendLpcDatagram(H);
}

static VOID
IntDwmSendChildCreate(_In_ PWND Wnd)
{
    UCHAR Buf[256];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND) * 2 + sizeof(ULONG) * 2 + sizeof(RECTL);

    if (!gfbDwmCompositing || !Wnd || !Wnd->spwndParent)
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_CHILD_CREATE;
    pl += sizeof(ULONG);
    {
        HWND h = UserHMGetHandle(Wnd);
        RtlCopyMemory(pl, &h, sizeof(HWND));
        pl += sizeof(HWND);
    }
    {
        HWND h = UserHMGetHandle(Wnd->spwndParent);
        RtlCopyMemory(pl, &h, sizeof(HWND));
        pl += sizeof(HWND);
    }
    *(PULONG)pl = Wnd->style;
    pl += sizeof(ULONG);
    *(PULONG)pl = Wnd->ExStyle;
    pl += sizeof(ULONG);
    RtlCopyMemory(pl, &Wnd->rcWindow, sizeof(RECTL));

    DPRINT1("[DWM] LPC ChildCreate hwnd=%p parent=%p\n",
            UserHMGetHandle(Wnd), UserHMGetHandle(Wnd->spwndParent));
    IntDwmSendLpcDatagram(H);
}

static VOID
IntDwmSendChildUpdate(_In_ PWND Wnd)
{
    UCHAR Buf[256];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND) * 2 + sizeof(ULONG) * 2 + sizeof(RECTL);

    if (!gfbDwmCompositing || !Wnd || !Wnd->spwndParent)
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_CHILD_UPDATE;
    pl += sizeof(ULONG);
    {
        HWND h = UserHMGetHandle(Wnd);
        RtlCopyMemory(pl, &h, sizeof(HWND));
        pl += sizeof(HWND);
    }
    {
        HWND h = UserHMGetHandle(Wnd->spwndParent);
        RtlCopyMemory(pl, &h, sizeof(HWND));
        pl += sizeof(HWND);
    }
    *(PULONG)pl = Wnd->style;
    pl += sizeof(ULONG);
    *(PULONG)pl = Wnd->ExStyle;
    pl += sizeof(ULONG);
    RtlCopyMemory(pl, &Wnd->rcWindow, sizeof(RECTL));

    DPRINT1("[DWM] LPC ChildUpdate hwnd=%p parent=%p\n",
            UserHMGetHandle(Wnd), UserHMGetHandle(Wnd->spwndParent));
    IntDwmSendLpcDatagram(H);
}

static VOID
IntDwmSendChildDestroy(_In_ PWND Wnd)
{
    UCHAR Buf[128];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND);

    if (!gfbDwmCompositing || !Wnd)
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = DWM_LPC_OP_CHILD_DESTROY;
    pl += sizeof(ULONG);
    {
        HWND h = UserHMGetHandle(Wnd);
        RtlCopyMemory(pl, &h, sizeof(HWND));
    }

    DPRINT1("[DWM] LPC ChildDestroy hwnd=%p\n", UserHMGetHandle(Wnd));
    IntDwmSendLpcDatagram(H);
}

static VOID
IntDwmSendZorder(_In_ ULONG Opcode, _In_ PWND Wnd, _In_ HWND hwndInsertAfter)
{
    UCHAR Buf[128];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Buf;
    PUCHAR pl;
    const ULONG cbPayload = sizeof(ULONG) + sizeof(HWND) * 2;

    if (!gfbDwmCompositing || !Wnd)
        return;

    DwmMsgInitHeader(H, (USHORT)cbPayload);
    pl = DwmMsgPayload(H);
    *(PULONG)pl = Opcode;
    pl += sizeof(ULONG);
    {
        HWND h = UserHMGetHandle(Wnd);
        RtlCopyMemory(pl, &h, sizeof(HWND));
        pl += sizeof(HWND);
    }
    RtlCopyMemory(pl, &hwndInsertAfter, sizeof(HWND));

    DPRINT1("[DWM] LPC Zorder op=%lu hwnd=%p insertAfter=%p\n",
            Opcode, UserHMGetHandle(Wnd), hwndInsertAfter);
    IntDwmSendLpcDatagram(H);
}

VOID
FASTCALL
IntDwmOnWindowCreated(_In_ PWND Wnd)
{
    if (!gfbDwmCompositing || !Wnd)
        return;

    if (UserIsDesktopWindow(Wnd))
        return;

    DPRINT1("[DWM] OnWindowCreated hwnd=%p child=%u\n", UserHMGetHandle(Wnd), (Wnd->style & WS_CHILD) ? 1u : 0u);
    if (Wnd->style & WS_CHILD)
        IntDwmSendChildCreate(Wnd);
    else
        IntDwmTopLevelCreate(Wnd, &Wnd->rcWindow, (Wnd->ExStyle & WS_EX_TOPMOST) ? 1u : 0u);
}

VOID
FASTCALL
IntDwmOnWindowDestroyed(_In_ PWND Wnd)
{
    if (!gfbDwmCompositing || !Wnd)
        return;

    if (UserIsDesktopWindow(Wnd))
        return;

    DPRINT1("[DWM] OnWindowDestroyed hwnd=%p child=%u\n", UserHMGetHandle(Wnd), (Wnd->style & WS_CHILD) ? 1u : 0u);
    if (Wnd->style & WS_CHILD)
        IntDwmSendChildDestroy(Wnd);
    else
        IntDwmSendTopLevelDestroy(Wnd);
}

VOID
FASTCALL
IntDwmOnZorderChanged(_In_ PWND Wnd, _In_ HWND hwndInsertAfter)
{
    if (!gfbDwmCompositing || !Wnd)
        return;

    if (UserIsDesktopWindow(Wnd))
        return;

    DPRINT1("[DWM] OnZorderChanged hwnd=%p insertAfter=%p child=%u\n",
            UserHMGetHandle(Wnd), hwndInsertAfter, (Wnd->style & WS_CHILD) ? 1u : 0u);
    if (Wnd->style & WS_CHILD)
        IntDwmSendZorder(DWM_LPC_OP_CHILD_ZORDER, Wnd, hwndInsertAfter);
    else
        IntDwmSendZorder(DWM_LPC_OP_TOPLEVEL_ZORDER, Wnd, hwndInsertAfter);
}

VOID
FASTCALL
IntDwmOnWindowPosChanged(_In_ PWND Wnd, _In_ UINT SwpFlags,
                         _In_ PRECTL prcOldClient)
{
    ULONG updateArg;
    LONG oldCw, oldCh, newCw, newCh;
    BOOL clientSizeChanged;

    if (!gfbDwmCompositing || !Wnd || UserIsDesktopWindow(Wnd))
        return;
    if (!prcOldClient)
        return;

    oldCw = prcOldClient->right - prcOldClient->left;
    oldCh = prcOldClient->bottom - prcOldClient->top;
    newCw = Wnd->rcClient.right - Wnd->rcClient.left;
    newCh = Wnd->rcClient.bottom - Wnd->rcClient.top;
    clientSizeChanged = (oldCw != newCw || oldCh != newCh);

    if (SwpFlags & SWP_HIDEWINDOW)
        updateArg = DWM_TOP_UPDATE_HIDE;
    else if (SwpFlags & SWP_SHOWWINDOW)
        updateArg = DWM_TOP_UPDATE_SHOW;
    else
        updateArg = DWM_TOP_UPDATE_LAYOUT;

    if (Wnd->style & WS_CHILD)
        IntDwmSendChildUpdate(Wnd);
    else
        IntDwmTopLevelUpdate(Wnd, updateArg, &Wnd->rcWindow);

    if (Wnd->hbmDwmRedirect && clientSizeChanged)
    {
        DPRINT1("[DWM] OnWindowPosChanged: client resize -> drop redirect hwnd=%p %ldx%ld -> %ldx%ld\n",
                UserHMGetHandle(Wnd), oldCw, oldCh, newCw, newCh);
        IntDwmUnbindRedirectForWindow(Wnd);
        IntDwmFreeRedirectSurface(Wnd);
    }
}
