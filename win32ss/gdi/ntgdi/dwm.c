/*
 * PROJECT:         ReactOS Win32k GDI
 * LICENSE:         GPL-2.0-or-later
 * PURPOSE:         Longhorn DWM GDI path (GreDwm*; NtGdiDwmGetSurfaceData is extra syscall — LH5048 uses NtUserDwmGetSurfaceData).
 *
 * GreDwmGetSurfaceData follows build ~5048 layout (7×ULONG_PTR user block + optional section; qmemcpy 0x1C on x86).
 */

#include <win32k.h>

#include "user/ntuser/dwm.h"
#include "user/ntuser/dce.h"
#include "user/ntuser/dwmnotify.h"
#include "user/ntuser/dwmvisual.h"
#include "dwmkm.h"

DBG_DEFAULT_CHANNEL(UserMisc);

#include <debug.h>

#define TAG_GRE_DWM 'wmRG'

typedef struct _GRE_DWM_STATE
{
    HRGN hrgnScratch;
    HDEV hdev;
} GRE_DWM_STATE;

static GRE_DWM_STATE *gGreDwmState;

NTSTATUS
APIENTRY
IntGreDwmResolveSurface(
    _In_ HDEV hdev,
    _In_opt_ HWND hwnd,
    _Out_ PWND *ppWnd,
    _Out_ PSURFACE *ppsurf)
{
    PWND pwnd = NULL;
    PDCE dce;
    PDC pdc;
    PSURFACE psurf = NULL;
    PPDEVOBJ ppdev;

    *ppWnd = NULL;
    *ppsurf = NULL;

    if (hwnd)
    {
        pwnd = UserGetWindowObject(hwnd);
        if (!pwnd)
        {
            DPRINT1("[DWM] IntGreDwmResolveSurface: invalid hwnd=%p\n", hwnd);
            return STATUS_INVALID_PARAMETER;
        }
        *ppWnd = pwnd;

        if (pwnd->hbmDwmRedirect && GreIsHandleValid(pwnd->hbmDwmRedirect))
        {
            psurf = SURFACE_ShareLockSurface(pwnd->hbmDwmRedirect);
            if (psurf)
            {
                *ppsurf = psurf;
                return STATUS_SUCCESS;
            }
        }

        dce = DceFindDceForDwmSurfaceResolve(pwnd);
        if (dce && GreIsHandleValid(dce->hDC))
        {
            pdc = DC_LockDc(dce->hDC);
            if (pdc)
            {
                psurf = pdc->dclevel.pSurface;
                if (psurf)
                    SURFACE_ShareLockByPointer(psurf);
                DC_UnlockDc(pdc);
                if (psurf)
                {
                    *ppsurf = psurf;
                    return STATUS_SUCCESS;
                }
            }
        }

        if (UserIsDesktopWindow(pwnd))
        {
            ppdev = (PPDEVOBJ)hdev;
            psurf = PDEVOBJ_pSurface(ppdev);
            if (psurf)
            {
                SURFACE_ShareLockByPointer(psurf);
                *ppsurf = psurf;
                return STATUS_SUCCESS;
            }
        }

        DPRINT1("[DWM] IntGreDwmResolveSurface: NOT_FOUND hwnd=%p\n", hwnd);
        return STATUS_NOT_FOUND;
    }

    ppdev = (PPDEVOBJ)hdev;
    psurf = PDEVOBJ_pSurface(ppdev);
    if (!psurf)
    {
        DPRINT1("[DWM] IntGreDwmResolveSurface: no primary pdev=%p\n", ppdev);
        return STATUS_NOT_FOUND;
    }
    SURFACE_ShareLockByPointer(psurf);
    *ppsurf = psurf;
    return STATUS_SUCCESS;
}

static NTSTATUS
IntGreDwmCopySurfaceToSection(
    _In_ PSURFACE psurf,
    _Out_ HANDLE *pSectionOut)
{
    PVOID pvSrcBase;
    LONG lDelta;
    ULONG rowBytes;
    ULONG cy;
    SIZE_T imageCb;
    SIZE_T sectionCb;
    LARGE_INTEGER liMax;
    HANDLE hSection = NULL;
    NTSTATUS Status;
    PVOID pvView = NULL;
    SIZE_T viewSize = 0;
    LARGE_INTEGER liZero = {{0}};
    ULONG y;
    PUCHAR pd, ps;

    *pSectionOut = NULL;

    pvSrcBase = psurf->SurfObj.pvScan0 ? psurf->SurfObj.pvScan0 : psurf->SurfObj.pvBits;
    if (!pvSrcBase || (psurf->flags & UNREADABLE_SURFACE))
        return STATUS_SUCCESS;

    lDelta = psurf->SurfObj.lDelta;
    rowBytes = (ULONG)(lDelta >= 0 ? lDelta : -lDelta);
    cy = psurf->SurfObj.sizlBitmap.cy;
    if (rowBytes == 0 || cy == 0 || cy > 0x100000ul)
        return STATUS_INVALID_BUFFER_SIZE;

    imageCb = (SIZE_T)rowBytes * cy;
    sectionCb = (imageCb + PAGE_SIZE - 1) & ~(SIZE_T)(PAGE_SIZE - 1);
    liMax.QuadPart = (LONGLONG)sectionCb;

    Status = ZwCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &liMax,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ZwMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                &pvView,
                                0,
                                0,
                                &liZero,
                                &viewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(hSection);
        return Status;
    }

    ps = (PUCHAR)pvSrcBase;
    if (lDelta < 0)
        ps += (cy - 1) * rowBytes;

    pd = (PUCHAR)pvView;
    for (y = 0; y < cy; y++)
    {
        RtlCopyMemory(pd, ps, rowBytes);
        pd += rowBytes;
        if (lDelta >= 0)
            ps += rowBytes;
        else
            ps -= rowBytes;
    }

    ZwUnmapViewOfSection(NtCurrentProcess(), pvView);
    *pSectionOut = hSection;
    return STATUS_SUCCESS;
}

BOOL
APIENTRY
GreDwmStartup(_In_ HDEV hdev)
{
    RECTL rcEmpty = {0, 0, 0, 0};
    BOOL ret;
    PPDEVOBJ ppdevFromHdev = (PPDEVOBJ)hdev;

    if (!hdev)
    {
        DPRINT1("[DWM] GreDwmStartup: [0] FAIL null hdev\n");
        return FALSE;
    }

    DPRINT1("[DWM] GreDwmStartup: [0] enter hdev=%p ppdev=%p gGreDwmState=%p gfbDwmCompositing=%u gpepDwm=%p\n",
            hdev, ppdevFromHdev, gGreDwmState, (unsigned)gfbDwmCompositing, gpepDwm);

    DPRINT1("[DWM] GreDwmStartup: [1] DxEngLockShareSem()\n");
    DxEngLockShareSem();
    DPRINT1("[DWM] GreDwmStartup: [2] DxEngLockHdev(%p)\n", hdev);
    DxEngLockHdev(hdev);
    DPRINT1("[DWM] GreDwmStartup: [3] locks held (share + hdev)\n");

    if (gGreDwmState)
    {
        DPRINT1("[DWM] GreDwmStartup: [4] idempotent hit state=%p saved_hdev=%p hrgnScratch=%p — bump uniq, unlock, TRUE\n",
                gGreDwmState, gGreDwmState->hdev, gGreDwmState->hrgnScratch);
        DxEngIncDispUniq();
        DPRINT1("[DWM] GreDwmStartup: [5] DxEngIncDispUniq (idempotent path)\n");
        DPRINT1("[DWM] GreDwmStartup: [6] DxEngUnlockHdev(%p)\n", hdev);
        DxEngUnlockHdev(hdev);
        DPRINT1("[DWM] GreDwmStartup: [7] DxEngUnlockShareSem()\n");
        DxEngUnlockShareSem();
        DPRINT1("[DWM] GreDwmStartup: [8] leave TRUE (idempotent)\n");
        return TRUE;
    }

    DPRINT1("[DWM] GreDwmStartup: [4] ExAllocatePoolWithTag(PagedPool, %lu, 'wmRG')\n",
            (ULONG)sizeof(*gGreDwmState));
    gGreDwmState = ExAllocatePoolWithTag(PagedPool, sizeof(*gGreDwmState), TAG_GRE_DWM);
    if (!gGreDwmState)
    {
        DPRINT1("[DWM] GreDwmStartup: [4] FAIL pool alloc — IncDispUniq, unlock, FALSE\n");
        goto cleanup_incomplete;
    }

    DPRINT1("[DWM] GreDwmStartup: [4] pool OK state=%p\n", gGreDwmState);
    RtlZeroMemory(gGreDwmState, sizeof(*gGreDwmState));
    gGreDwmState->hdev = hdev;

    DPRINT1("[DWM] GreDwmStartup: [5] GreCreateRectRgnIndirect(empty) for scratch region\n");
    gGreDwmState->hrgnScratch = GreCreateRectRgnIndirect(&rcEmpty);
    if (!gGreDwmState->hrgnScratch)
    {
        DPRINT1("[DWM] GreDwmStartup: [5] FAIL GreCreateRectRgnIndirect — free state, IncDispUniq, unlock, FALSE\n");
        ExFreePoolWithTag(gGreDwmState, TAG_GRE_DWM);
        gGreDwmState = NULL;
        goto cleanup_incomplete;
    }

    DPRINT1("[DWM] GreDwmStartup: [5] scratch HRGN=%p hdev stored=%p\n",
            gGreDwmState->hrgnScratch, gGreDwmState->hdev);

    /*
     * Longhorn 5048: TransferSpriteStateToVisualState(hsurf, &P) before DwmTopLevelCreate walk on gDceState.
     */
    if (!NT_SUCCESS(EngpTransferSpriteStateToVisualState(ppdevFromHdev)))
    {
        DPRINT1("[DWM] GreDwmStartup: EngpTransferSpriteStateToVisualState failed (continuing with DCE walk)\n");
    }

    DPRINT1("[DWM] GreDwmStartup: [6] IntDwmGreStartupWalkDceList(%p) START (DCE pass1+pass2 LPC)\n", hdev);
    IntDwmGreStartupWalkDceList(hdev);
    DPRINT1("[DWM] GreDwmStartup: [6] IntDwmGreStartupWalkDceList END\n");

    /*
     * Longhorn: TransferSpriteStateToVisualState; we mirror the gDceState two-pass LPC notify above.
     */
    DPRINT1("[DWM] GreDwmStartup: [7] DxEngIncDispUniq() (post-walk refresh)\n");
    DxEngIncDispUniq();

    DPRINT1("[DWM] GreDwmStartup: [8] DxEngUnlockHdev(%p)\n", hdev);
    DxEngUnlockHdev(hdev);
    DPRINT1("[DWM] GreDwmStartup: [9] DxEngUnlockShareSem()\n");
    DxEngUnlockShareSem();

    ret = (gGreDwmState != NULL);
    DPRINT1("[DWM] GreDwmStartup: [10] leave ret=%u state=%p hrgn=%p (success path)\n",
            ret, gGreDwmState, gGreDwmState ? gGreDwmState->hrgnScratch : NULL);
    return ret;

cleanup_incomplete:
    DPRINT1("[DWM] GreDwmStartup: [cleanup] DxEngIncDispUniq() (failed init path)\n");
    DxEngIncDispUniq();
    DPRINT1("[DWM] GreDwmStartup: [cleanup] DxEngUnlockHdev(%p)\n", hdev);
    DxEngUnlockHdev(hdev);
    DPRINT1("[DWM] GreDwmStartup: [cleanup] DxEngUnlockShareSem()\n");
    DxEngUnlockShareSem();
    DPRINT1("[DWM] GreDwmStartup: [cleanup] leave FALSE gGreDwmState=%p\n", gGreDwmState);
    return FALSE;
}

BOOL
APIENTRY
GreDwmShutdown(_In_ HDEV hdev)
{
    DPRINT1("[DWM] GreDwmShutdown: enter hdev=%p state=%p\n", hdev, gGreDwmState);

    IntRosDwmFreeAllVisuals();

    DxEngLockShareSem();
    DxEngLockHdev(hdev);

    if (gGreDwmState)
    {
        if (gGreDwmState->hrgnScratch)
        {
            GreDeleteObject(gGreDwmState->hrgnScratch);
            gGreDwmState->hrgnScratch = NULL;
        }
        ExFreePoolWithTag(gGreDwmState, TAG_GRE_DWM);
        gGreDwmState = NULL;
    }

    DxEngIncDispUniq();
    DxEngUnlockHdev(hdev);
    DxEngUnlockShareSem();

    DPRINT1("[DWM] GreDwmShutdown: done\n");
    return TRUE;
}

NTSTATUS
APIENTRY
GreDwmGetSurfaceData(
    _In_ HDEV hdev,
    _In_opt_ HWND hwnd,
    _Out_writes_bytes_(sizeof(DWM_SURFACE_KERNEL_OUT)) PVOID pUserOutput)
{
    DWM_SURFACE_KERNEL_OUT kOut;
    NTSTATUS Status;
    NTSTATUS CopyStatus;
    PWND pwnd = NULL;
    PSURFACE psurf = NULL;
    HANDLE hSection = NULL;
    LONG ld;
    ULONG rowB;

    if (!hdev || !pUserOutput)
    {
        DPRINT1("[DWM] GreDwmGetSurfaceData: bad param hdev=%p out=%p\n", hdev, pUserOutput);
        return STATUS_INVALID_PARAMETER;
    }

    DPRINT1("[DWM] GreDwmGetSurfaceData: enter hwnd=%p compositing=%u\n", hwnd, (unsigned)gfbDwmCompositing);
    RtlZeroMemory(&kOut, sizeof(kOut));
    Status = STATUS_UNSUCCESSFUL;

    if (!hwnd)
    {
        DPRINT1("[DWM] GreDwmGetSurfaceData: null hwnd\n");
        return STATUS_INVALID_PARAMETER;
    }

    DxEngLockShareSem();
    DxEngLockHdev(hdev);

    /*
     * Compositing: prefer LH5048-style pFindVisual (ROS_DWM_VISUAL). Unlike real win32k, our visual
     * list is not always complete for every HWND milcore asks for; refresh redirect + upsert first,
     * then if still no visual fall back to IntGreDwmResolveSurface (DWM syscall entry points already
     * restrict callers to the DWM process).
     */
    if (gfbDwmCompositing)
    {
        PROS_DWM_VISUAL vis;
        PWND pwndPrep;

        pwndPrep = UserGetWindowObject(hwnd);
        if (pwndPrep)
        {
            IntDwmPrepareRedirectSurface(hdev, pwndPrep);
            IntRosDwmUpsertForPwnd(pwndPrep);
        }

        vis = IntRosDwmFindVisual(hwnd);

        if (vis && (vis->Flags & ROS_DWM_VISUAL_FLAG_VALID))
        {
            LONG vrw = vis->rcScreen.right - vis->rcScreen.left;
            LONG vrh = vis->rcScreen.bottom - vis->rcScreen.top;

            if (vis->pso)
            {
                psurf = CONTAINING_RECORD(vis->pso, SURFACE, SurfObj);
                ld = psurf->SurfObj.lDelta;
                rowB = (ULONG)(ld >= 0 ? ld : -ld);

                if (psurf->SurfObj.sizlBitmap.cx > 0 && psurf->SurfObj.sizlBitmap.cy > 0 &&
                    rowB > 0)
                {
                    kOut.Width = psurf->SurfObj.sizlBitmap.cx;
                    kOut.Height = psurf->SurfObj.sizlBitmap.cy;
                    kOut.PixelFormat = psurf->SurfObj.iBitmapFormat;
                    kOut.SurfaceFlags = psurf->flags;
                    kOut.StrideBytes = rowB;
                    kOut.BlendState = EngpDwmBlendStateFromSpriteAttrs(&vis->Blend,
                                                                       vis->ulSpriteAttr10,
                                                                       vis->ulSpriteAttr8);

                    CopyStatus = IntGreDwmCopySurfaceToSection(psurf, &hSection);
                    if (!NT_SUCCESS(CopyStatus))
                    {
                        DPRINT1("[DWM] GreDwmGetSurfaceData: visual CopySurfaceToSection failed %08lX hwnd=%p\n",
                                CopyStatus, hwnd);
                        DxEngUnlockHdev(hdev);
                        DxEngUnlockShareSem();
                        return CopyStatus;
                    }
                    kOut.hSection = hSection;
                    Status = STATUS_SUCCESS;
                    DPRINT1("[DWM] GreDwmGetSurfaceData: visual+pso hwnd=%p section=%p\n", hwnd, kOut.hSection);
                    goto GreDwmSurfaceDone;
                }

                DPRINT1("[DWM] GreDwmGetSurfaceData: visual pso degenerate hwnd=%p surf=%lux%lu -> dims-only\n",
                        hwnd,
                        psurf->SurfObj.sizlBitmap.cx,
                        psurf->SurfObj.sizlBitmap.cy);
            }

            kOut.Width = (ULONG_PTR)vrw;
            kOut.Height = (ULONG_PTR)vrh;
            kOut.BlendState = EngpDwmBlendStateFromSpriteAttrs(&vis->Blend,
                                                               vis->ulSpriteAttr10,
                                                               vis->ulSpriteAttr8);
            Status = STATUS_SUCCESS;
            DPRINT1("[DWM] GreDwmGetSurfaceData: visual dims-only hwnd=%p %lux%lu\n",
                    hwnd, kOut.Width, kOut.Height);
            goto GreDwmSurfaceDone;
        }

        Status = IntGreDwmResolveSurface(hdev, hwnd, &pwnd, &psurf);
        if (NT_SUCCESS(Status) && psurf)
        {
            ld = psurf->SurfObj.lDelta;
            rowB = (ULONG)(ld >= 0 ? ld : -ld);

            kOut.Width = psurf->SurfObj.sizlBitmap.cx;
            kOut.Height = psurf->SurfObj.sizlBitmap.cy;
            kOut.PixelFormat = psurf->SurfObj.iBitmapFormat;
            kOut.SurfaceFlags = psurf->flags;
            kOut.StrideBytes = rowB;
            if (pwnd && (pwnd->ExStyle & WS_EX_LAYERED))
                kOut.BlendState = 1;

            CopyStatus = IntGreDwmCopySurfaceToSection(psurf, &hSection);
            if (!NT_SUCCESS(CopyStatus))
            {
                DPRINT1("[DWM] GreDwmGetSurfaceData: resolve CopySurfaceToSection failed %08lX hwnd=%p\n",
                        CopyStatus, hwnd);
                SURFACE_ShareUnlockSurface(psurf);
                psurf = NULL;
                DxEngUnlockHdev(hdev);
                DxEngUnlockShareSem();
                return CopyStatus;
            }
            kOut.hSection = hSection;
            Status = STATUS_SUCCESS;
            DPRINT1("[DWM] GreDwmGetSurfaceData: compositing resolve fallback hwnd=%p section=%p\n",
                    hwnd, kOut.hSection);
            SURFACE_ShareUnlockSurface(psurf);
            psurf = NULL;
            goto GreDwmSurfaceDone;
        }

        if (psurf)
        {
            SURFACE_ShareUnlockSurface(psurf);
            psurf = NULL;
        }

        DxEngUnlockHdev(hdev);
        DxEngUnlockShareSem();
        DPRINT1("[DWM] GreDwmGetSurfaceData: compositing, no visual and no resolve hwnd=%p\n", hwnd);
        return STATUS_NOT_FOUND;
    }

    {
        PPDEVOBJ ppdevSp = (PPDEVOBJ)hdev;
        ULONG spCx, spCy;
        ULONG attr4 = 0, attr6 = 0;
        BLENDFUNCTION blendSp;

        psurf = EngpSpGetShapeSurface(ppdevSp, hwnd);
        if (psurf)
        {
            if (EngpGdiGetSpriteAttributes(ppdevSp, hwnd, NULL, &attr4, &blendSp, &attr6) &&
                ppdevSp->pSpriteState)
            {
                (VOID)EngpSpriteGet(ppdevSp->pSpriteState, hwnd, NULL);
            }

            ld = psurf->SurfObj.lDelta;
            rowB = (ULONG)(ld >= 0 ? ld : -ld);
            kOut.Width = psurf->SurfObj.sizlBitmap.cx;
            kOut.Height = psurf->SurfObj.sizlBitmap.cy;
            kOut.PixelFormat = psurf->SurfObj.iBitmapFormat;
            kOut.SurfaceFlags = psurf->flags;
            kOut.StrideBytes = rowB;
            kOut.BlendState = EngpDwmBlendStateFromSpriteAttrs(&blendSp, attr6, attr4);

            CopyStatus = IntGreDwmCopySurfaceToSection(psurf, &hSection);
            SURFACE_ShareUnlockSurface(psurf);
            psurf = NULL;
            if (!NT_SUCCESS(CopyStatus))
            {
                DPRINT1("[DWM] GreDwmGetSurfaceData: sprite CopySurfaceToSection failed %08lX hwnd=%p\n",
                        CopyStatus, hwnd);
                DxEngUnlockHdev(hdev);
                DxEngUnlockShareSem();
                return CopyStatus;
            }
            kOut.hSection = hSection;
            Status = STATUS_SUCCESS;
            DPRINT1("[DWM] GreDwmGetSurfaceData: sprite+shape hwnd=%p section=%p\n", hwnd, kOut.hSection);
            goto GreDwmSurfaceDone;
        }

        if (EngpSpriteTryGetExtents(ppdevSp, hwnd, &spCx, &spCy))
        {
            if (EngpGdiGetSpriteAttributes(ppdevSp, hwnd, NULL, &attr4, &blendSp, &attr6) &&
                ppdevSp->pSpriteState)
            {
                (VOID)EngpSpriteGet(ppdevSp->pSpriteState, hwnd, NULL);
            }
            kOut.Width = spCx;
            kOut.Height = spCy;
            kOut.BlendState = EngpDwmBlendStateFromSpriteAttrs(&blendSp, attr6, attr4);
            Status = STATUS_SUCCESS;
            DPRINT1("[DWM] GreDwmGetSurfaceData: sprite dims-only hwnd=%p %lux%lu\n",
                    hwnd, kOut.Width, kOut.Height);
            goto GreDwmSurfaceDone;
        }
    }

GreDwmSurfaceDone:
    DxEngUnlockHdev(hdev);
    DxEngUnlockShareSem();

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("[DWM] GreDwmGetSurfaceData: ResolveSurface failed %08lX hwnd=%p\n", Status, hwnd);
        return Status;
    }

    _SEH2_TRY
    {
        ProbeForWrite(pUserOutput, sizeof(DWM_SURFACE_KERNEL_OUT), sizeof(ULONG_PTR));
        RtlCopyMemory(pUserOutput, &kOut, sizeof(kOut));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("[DWM] GreDwmGetSurfaceData: probe/copy user AV hwnd=%p\n", hwnd);
        if (kOut.hSection)
            ZwClose(kOut.hSection);
        return STATUS_ACCESS_VIOLATION;
    }
    _SEH2_END;

    DPRINT1("[DWM] GreDwmGetSurfaceData: success hwnd=%p\n", hwnd);
    return STATUS_SUCCESS;
}

BOOL
APIENTRY
NtGdiDwmGetSurfaceData(_In_ HWND hwnd, _In_opt_ PVOID pSurfaceDataOut)
{
    NTSTATUS Status;
    HDEV hdev;

    if (!pSurfaceDataOut)
    {
        DPRINT1("[DWM] NtGdiDwmGetSurfaceData: null out hwnd=%p\n", hwnd);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!DwmIsDwmClientProcess())
    {
        DPRINT1("[DWM] NtGdiDwmGetSurfaceData: access denied hwnd=%p\n", hwnd);
        EngSetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (!gpmdev || !gpmdev->ppdevGlobal)
    {
        DPRINT1("[DWM] NtGdiDwmGetSurfaceData: no pdev hwnd=%p\n", hwnd);
        EngSetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    hdev = (HDEV)gpmdev->ppdevGlobal;

    UserEnterExclusive();
    Status = GreDwmGetSurfaceData(hdev, hwnd, pSurfaceDataOut);
    UserLeave();

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("[DWM] NtGdiDwmGetSurfaceData: Gre failed %08lX hwnd=%p\n", Status, hwnd);
        EngSetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    DPRINT1("[DWM] NtGdiDwmGetSurfaceData: OK hwnd=%p\n", hwnd);
    return TRUE;
}
