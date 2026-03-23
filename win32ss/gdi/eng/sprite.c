/*
 * PROJECT:     ReactOS win32k
 * PURPOSE:     Longhorn-style sprite engine (SPRITESTATE, pSpCreateSprite / pSpGetSprite / GdiDeleteSprite)
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 */

#include <win32k.h>

#include "user/ntuser/dwm.h"
#include "user/ntuser/dwmvisual.h"

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(EngPDev);

static VOID
EngpSpriteRenumberZOrder(_In_ PSPRITESTATE pss)
{
    PSPRITE v1;
    ULONG z = 0;

    for (v1 = pss->pspriteZHead; v1; v1 = v1->pspriteNextZ)
        v1->zOrder = z++;
}

static VOID
EngpSpriteUnlinkY(_In_ PSPRITESTATE pss, _In_ PSPRITE ps)
{
    PSPRITE nextY, prevY;

    if (ps->fl & SPRITE_FL_SKIP_YORDER)
        return;

    nextY = ps->pspriteNextY;
    prevY = ps->pspritePrevY;

    /* Not linked: avoid clearing Y-head when prev/next are both NULL. */
    if (!nextY && !prevY)
    {
        if (pss->pspriteYHead != ps)
            return;
        pss->pspriteYHead = NULL;
        return;
    }

    if (nextY)
        nextY->pspritePrevY = prevY;
    if (prevY)
        prevY->pspriteNextY = nextY;
    else
        pss->pspriteYHead = nextY;
}

static LONG
EngpSpriteYKey(_In_ PSPRITE ps)
{
    if (ps->y != (LONG)0x80000000)
        return ps->y;
    return ps->lUnk44;
}

static VOID
EngpSpriteSyncYKey(_Inout_ PSPRITE ps)
{
    if (ps->y != (LONG)0x80000000)
        ps->lUnk44 = ps->y;
}

static VOID
EngpSpriteUnlinkZ(_In_ PSPRITESTATE pss, _In_ PSPRITE ps)
{
    if (pss->pspriteZHead == ps)
    {
        pss->pspriteZHead = ps->pspriteNextZ;
    }
    else
    {
        PSPRITE v3 = pss->pspriteZHead;

        while (v3 && v3->pspriteNextZ != ps)
            v3 = v3->pspriteNextZ;
        if (v3)
            v3->pspriteNextZ = ps->pspriteNextZ;
    }
}

static VOID
EngpSpriteOrderInY(_In_ PSPRITE ps)
{
    PSPRITESTATE pss;
    PSPRITE cur, prev;
    LONG key;

    if (!ps || (ps->fl & SPRITE_FL_SKIP_YORDER))
        return;

    pss = ps->pSpriteState;
    if (!pss)
        return;

    EngpSpriteSyncYKey(ps);
    key = EngpSpriteYKey(ps);

    EngpSpriteUnlinkY(pss, ps);

    prev = NULL;
    for (cur = pss->pspriteYHead; cur; cur = cur->pspriteNextY)
    {
        if (EngpSpriteYKey(cur) > key)
            break;
        prev = cur;
    }

    ps->pspriteNextY = cur;
    ps->pspritePrevY = prev;
    if (cur)
        cur->pspritePrevY = ps;
    if (prev)
        prev->pspriteNextY = ps;
    else
        pss->pspriteYHead = ps;
}

static VOID
EngpSpriteRelinkZOrder(
    _In_ PSPRITESTATE pss,
    _In_ PSPRITE ps,
    _In_ HWND hwndInsertAfter)
{
    PSPRITE sent = pss->pspriteSentinel;
    PSPRITE pred;

    EngpSpriteUnlinkZ(pss, ps);

    if (hwndInsertAfter == HWND_TOP || hwndInsertAfter == HWND_TOPMOST)
    {
        if (pss->pspriteZHead == sent)
        {
            ps->pspriteNextZ = sent;
            pss->pspriteZHead = ps;
        }
        else
        {
            for (pred = pss->pspriteZHead; pred->pspriteNextZ != sent; pred = pred->pspriteNextZ)
                ;
            ps->pspriteNextZ = sent;
            pred->pspriteNextZ = ps;
        }
    }
    else if (hwndInsertAfter == HWND_BOTTOM || hwndInsertAfter == HWND_NOTOPMOST)
    {
        ps->pspriteNextZ = pss->pspriteZHead;
        pss->pspriteZHead = ps;
    }
    else
    {
        PSPRITE after = EngpSpriteGet(pss, hwndInsertAfter, NULL);

        if (after && !(after->fl & SPRITE_FL_SENTINEL))
        {
            ps->pspriteNextZ = after->pspriteNextZ;
            after->pspriteNextZ = ps;
        }
        else
        {
            if (pss->pspriteZHead == sent)
            {
                ps->pspriteNextZ = sent;
                pss->pspriteZHead = ps;
            }
            else
            {
                for (pred = pss->pspriteZHead; pred->pspriteNextZ != sent; pred = pred->pspriteNextZ)
                    ;
                ps->pspriteNextZ = sent;
                pred->pspriteNextZ = ps;
            }
        }
    }

    EngpSpriteRenumberZOrder(pss);
    if ((ps->fl & SPRITE_FL_SKIP_YORDER) == 0)
        EngpSpriteOrderInY(ps);
}

static VOID
EngpSpriteFillVisualFromResolvedSurface(
    _Inout_ PROS_DWM_VISUAL vis,
    _In_ PSURFACE psurfSrc,
    _In_ ULONG cx,
    _In_ ULONG cy)
{
    SIZEL sz;
    HBITMAP hbm;
    PSURFACE psurfDib;
    SURFOBJ *psoSrc, *psoDst;
    RECTL rcl;
    POINTL ptSrc = {0, 0};
    LONG ldSrc;
    ULONG rowB;

    if (!vis || !psurfSrc || cx == 0 || cy == 0)
        return;

    ldSrc = psurfSrc->SurfObj.lDelta;
    rowB = (ULONG)(ldSrc >= 0 ? ldSrc : -ldSrc);
    if (rowB == 0)
        return;

    sz.cx = (LONG)cx;
    sz.cy = (LONG)cy;
    hbm = EngCreateBitmap(sz,
                          (LONG)rowB,
                          psurfSrc->SurfObj.iBitmapFormat,
                          BMF_TOPDOWN,
                          NULL);
    if (!hbm)
        return;

    psurfDib = SURFACE_ShareLockSurface((HSURF)hbm);
    if (!psurfDib)
    {
        GreDeleteObject(hbm);
        return;
    }

    psoSrc = &psurfSrc->SurfObj;
    psoDst = &psurfDib->SurfObj;
    rcl.left = 0;
    rcl.top = 0;
    rcl.right = (LONG)cx;
    rcl.bottom = (LONG)cy;

    if (!EngCopyBits(psoDst, psoSrc, NULL, NULL, &rcl, &ptSrc))
    {
        SURFACE_ShareUnlockSurface(psurfDib);
        GreDeleteObject(hbm);
        return;
    }

    SURFACE_ShareUnlockSurface(psurfDib);
    vis->pso = EngLockSurface((HSURF)hbm);
    if (vis->pso)
        vis->hsurfEngAlloc = (HSURF)hbm;
    else
        GreDeleteObject(hbm);
}

static VOID
EngpSpriteZap(_In_ PSPRITESTATE pss, _In_ PSPRITE ps)
{
    if (!ps || (ps->fl & SPRITE_FL_SENTINEL))
        return;

    EngpSpriteUnlinkZ(pss, ps);
    EngpSpriteUnlinkY(pss, ps);

    if (ps->hsurfShape)
        ps->hsurfShape = 0;
    ps->fl &= ~SPRITE_FL_SHAPE_D21_IS_THUNK;
    RtlZeroMemory(&ps->shapeD21, sizeof(ps->shapeD21));

    ExFreePoolWithTag(ps, GDITAG_SPRITE);
    EngpSpriteRenumberZOrder(pss);
}

NTSTATUS
NTAPI
EngpSpriteStateCreate(_In_ PPDEVOBJ ppdev)
{
    PSPRITESTATE_BLOCK block;
    PSPRITESTATE pss;

    if (!ppdev)
        return STATUS_INVALID_PARAMETER;

    if (ppdev->pSpriteState)
        return STATUS_SUCCESS;

    if (!(ppdev->flFlags & PDEV_DISPLAY) || (ppdev->flFlags & PDEV_META_DEVICE))
        return STATUS_SUCCESS;

    block = ExAllocatePoolWithTag(PagedPool, sizeof(*block), GDITAG_SPRITESTATE);
    if (!block)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(block, sizeof(*block));
    pss = &block->st;
    block->OwningPdev = ppdev;

    pss->pspriteSentinel = &block->Sentinel;
    block->Sentinel.pSpriteState = pss;
    block->Sentinel.fl = SPRITE_FL_SENTINEL;
    block->Sentinel.pspriteNextZ = NULL;
    pss->pspriteZHead = pss->pspriteSentinel;
    pss->pspriteYHead = NULL;

    ppdev->pSpriteState = pss;
    return STATUS_SUCCESS;
}

VOID
NTAPI
EngpSpriteStateDestroy(_In_ PPDEVOBJ ppdev)
{
    PSPRITESTATE_BLOCK block;
    PSPRITESTATE pss;
    PSPRITE ps, next;

    if (!ppdev || !ppdev->pSpriteState)
        return;

    pss = ppdev->pSpriteState;
    block = CONTAINING_RECORD(pss, SPRITESTATE_BLOCK, st);

    EngAcquireSemaphore(ppdev->hsemDevLock);

    for (ps = pss->pspriteZHead; ps && ps != pss->pspriteSentinel; ps = next)
    {
        next = ps->pspriteNextZ;
        EngpSpriteZap(pss, ps);
    }

    EngReleaseSemaphore(ppdev->hsemDevLock);

    ppdev->pSpriteState = NULL;
    RtlZeroMemory(block, sizeof(*block));
    ExFreePoolWithTag(block, GDITAG_SPRITESTATE);
}

PSPRITE
NTAPI
EngpSpriteGet(
    _In_ PSPRITESTATE pss,
    _In_opt_ HWND hwnd,
    _In_opt_ PVOID pShapeHint)
{
    PSPRITE result = (PSPRITE)pShapeHint;

    if (!pShapeHint && hwnd)
    {
        for (result = pss->pspriteZHead;
             result && result->hwnd != hwnd;
             result = result->pspriteNextZ)
        {
            /* LH5048 pSpGetSprite */
        }
    }
    return result;
}

PSPRITE
NTAPI
EngpSpriteCreate(
    _In_ HDEV hdev,
    _In_opt_ PRECTL prcl,
    _In_ HWND hwnd,
    _In_opt_ PPOINTL pptlOffset)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)hdev;
    PSPRITESTATE pss;
    PSPRITE ps, sent, v12;

    if (!ppdev || !hwnd)
        return NULL;

    if (!(ppdev->flFlags & PDEV_DISPLAY) || (ppdev->flFlags & PDEV_META_DEVICE))
        return NULL;

    pss = ppdev->pSpriteState;
    if (!pss)
        return NULL;

    EngAcquireSemaphore(ppdev->hsemDevLock);

    ps = EngpSpriteGet(pss, hwnd, NULL);
    if (ps)
    {
        EngReleaseSemaphore(ppdev->hsemDevLock);
        return ps;
    }

    ps = ExAllocatePoolWithTag(PagedPool, sizeof(SPRITE), GDITAG_SPRITE);
    if (!ps)
    {
        EngReleaseSemaphore(ppdev->hsemDevLock);
        return NULL;
    }

    RtlZeroMemory(ps, sizeof(*ps));

    if (prcl)
    {
        ps->cx = prcl->right - prcl->left;
        ps->cy = prcl->bottom - prcl->top;
        if (pptlOffset)
        {
            ps->x = pptlOffset->x;
            ps->y = pptlOffset->y;
        }
        else
        {
            ps->x = prcl->left;
            ps->y = prcl->top;
        }
    }
    else
    {
        ps->cx = 0;
        ps->cy = 0;
        ps->x = 0x80000000;
        ps->y = 0x80000000;
    }

    ps->ulSpFlags04 = 4;
    ps->lUnk40 = 0x80000000;
    ps->lUnk44 = 0x80000000;
    ps->lUnk48 = 0x80000000;
    ps->lUnk52 = 0x80000000;
    ps->pSpriteState = pss;
    ps->hwnd = hwnd;

    sent = pss->pspriteSentinel;
    v12 = pss->pspriteZHead;
    if (v12 == sent)
    {
        ps->pspriteNextZ = sent;
        pss->pspriteZHead = ps;
    }
    else
    {
        while (v12->pspriteNextZ != sent)
            v12 = v12->pspriteNextZ;
        ps->pspriteNextZ = sent;
        v12->pspriteNextZ = ps;
    }

    EngpSpriteRenumberZOrder(pss);

    if ((ps->fl & SPRITE_FL_SKIP_YORDER) == 0)
        EngpSpriteOrderInY(ps);

    EngReleaseSemaphore(ppdev->hsemDevLock);
    return ps;
}

VOID
NTAPI
EngpSpriteUpdateLayout(
    _In_ PPDEVOBJ ppdev,
    _In_ HWND hwnd,
    _In_ PRECTL prcl)
{
    PSPRITESTATE pss;
    PSPRITE ps;

    if (!ppdev || !hwnd || !prcl)
        return;

    pss = ppdev->pSpriteState;
    if (!pss)
        return;

    EngAcquireSemaphore(ppdev->hsemDevLock);
    ps = EngpSpriteGet(pss, hwnd, NULL);
    if (ps && !(ps->fl & SPRITE_FL_SENTINEL))
    {
        ps->x = prcl->left;
        ps->y = prcl->top;
        ps->cx = prcl->right - prcl->left;
        ps->cy = prcl->bottom - prcl->top;
        EngpSpriteSyncYKey(ps);
        EngpSpriteOrderInY(ps);
    }
    EngReleaseSemaphore(ppdev->hsemDevLock);
}

NTSTATUS
NTAPI
EngpTransferSpriteStateToVisualState(_In_ PPDEVOBJ ppdev)
{
    PSPRITESTATE pss;
    PSPRITE ps, sent, next;
    PROS_DWM_VISUAL buildHead = NULL, buildTail = NULL, vis, vwalk;
    NTSTATUS stat = STATUS_SUCCESS;

    if (!ppdev)
        return STATUS_INVALID_PARAMETER;

    pss = ppdev->pSpriteState;
    if (!pss)
    {
        IntRosDwmReplaceVisualListHead(NULL);
        return STATUS_SUCCESS;
    }

    EngAcquireSemaphore(ppdev->hsemDevLock);
    sent = pss->pspriteSentinel;

    for (ps = pss->pspriteZHead; ps && ps != sent; ps = ps->pspriteNextZ)
    {
        vis = ExAllocatePoolWithTag(PagedPool, sizeof(*vis), TAG_ROS_DWM_VISUAL);
        if (!vis)
        {
            stat = STATUS_INSUFFICIENT_RESOURCES;
            goto rollback_newonly;
        }

        RtlZeroMemory(vis, sizeof(*vis));
        vis->hwnd = ps->hwnd;
        vis->rcScreen.left = ps->x;
        vis->rcScreen.top = ps->y;
        vis->rcScreen.right = ps->x + (LONG)ps->cx;
        vis->rcScreen.bottom = ps->y + (LONG)ps->cy;
        vis->Flags |= ROS_DWM_VISUAL_FLAG_VALID;
        vis->ulSpriteAttr10 = ps->ulSpriteAttr39;
        vis->ulSpriteAttr8 = ps->ulSpriteAttr41;
        vis->Blend = ps->BlendSprite;

        if (ps->hsurfShape && GreIsHandleValid(ps->hsurfShape))
            vis->pso = EngLockSurface((HSURF)ps->hsurfShape);
        else if ((ps->fl & SPRITE_FL_SHAPE_D21_IS_THUNK) && ps->shapeD21.pvMinus16ToSurf)
        {
            PSURFACE psM = (PSURFACE)((PUCHAR)ps->shapeD21.pvMinus16ToSurf - 16);

            if (psM && psM->SurfObj.hsurf && GreIsHandleValid((HSURF)psM->SurfObj.hsurf))
                vis->pso = EngLockSurface((HSURF)psM->SurfObj.hsurf);
        }
        else if (!(ps->fl & SPRITE_FL_SHAPE_D21_IS_THUNK) &&
                 ps->shapeD21.hsurfAux && GreIsHandleValid(ps->shapeD21.hsurfAux))
        {
            vis->pso = EngLockSurface(ps->shapeD21.hsurfAux);
        }

        if (!buildHead)
            buildHead = vis;
        else
            buildTail->pNext = vis;
        buildTail = vis;
    }

    EngReleaseSemaphore(ppdev->hsemDevLock);

    /*
     * LH5048: without shape bits, SURFMEM::bCreateDIB + EngCopyBits from window surface.
     * Done without pdev sprite lock held (resolves DCE / redirect).
     */
    for (vis = buildHead; vis; vis = vis->pNext)
    {
        PWND pwndR = NULL;
        PSURFACE psurfSrc = NULL;

        ULONG w, h;

        if (vis->pso)
            continue;

        if (vis->rcScreen.right <= vis->rcScreen.left || vis->rcScreen.bottom <= vis->rcScreen.top)
            continue;

        w = (ULONG)(vis->rcScreen.right - vis->rcScreen.left);
        h = (ULONG)(vis->rcScreen.bottom - vis->rcScreen.top);
        if (w == 0 || h == 0)
            continue;

        if (!NT_SUCCESS(IntGreDwmResolveSurface((HDEV)ppdev, vis->hwnd, &pwndR, &psurfSrc)) ||
            !psurfSrc)
        {
            continue;
        }

        EngpSpriteFillVisualFromResolvedSurface(vis, psurfSrc, w, h);
        SURFACE_ShareUnlockSurface(psurfSrc);
    }

    EngAcquireSemaphore(ppdev->hsemDevLock);
    sent = pss->pspriteSentinel;
    for (ps = pss->pspriteZHead; ps && ps != sent; ps = next)
    {
        next = ps->pspriteNextZ;
        EngpSpriteZap(pss, ps);
    }
    EngReleaseSemaphore(ppdev->hsemDevLock);

    IntRosDwmReplaceVisualListHead(buildHead);
    return STATUS_SUCCESS;

rollback_newonly:
    EngReleaseSemaphore(ppdev->hsemDevLock);
    for (vwalk = buildHead; vwalk; )
    {
        PROS_DWM_VISUAL nx = vwalk->pNext;

        if (vwalk->pso)
            EngUnlockSurface(vwalk->pso);
        if (vwalk->hsurfEngAlloc)
            GreDeleteObject((HBITMAP)vwalk->hsurfEngAlloc);
        ExFreePoolWithTag(vwalk, TAG_ROS_DWM_VISUAL);
        vwalk = nx;
    }
    return stat;
}

PSURFACE
NTAPI
EngpSpGetShapeSurface(_In_ PPDEVOBJ ppdev, _In_ HWND hwnd)
{
    PSPRITESTATE pss;
    PSPRITE ps;
    PSURFACE psurf = NULL;

    if (!ppdev || !hwnd)
        return NULL;

    pss = ppdev->pSpriteState;
    if (!pss)
        return NULL;

    EngAcquireSemaphore(ppdev->hsemDevLock);
    ps = EngpSpriteGet(pss, hwnd, NULL);
    if (ps && !(ps->fl & SPRITE_FL_SENTINEL))
    {
        if (ps->hsurfShape && GreIsHandleValid(ps->hsurfShape))
            psurf = SURFACE_ShareLockSurface((HSURF)ps->hsurfShape);
        else if ((ps->fl & SPRITE_FL_SHAPE_D21_IS_THUNK) && ps->shapeD21.pvMinus16ToSurf)
        {
            PSURFACE psM = (PSURFACE)((PUCHAR)ps->shapeD21.pvMinus16ToSurf - 16);

            if (psM)
            {
                SURFACE_ShareLockByPointer(psM);
                psurf = psM;
            }
        }
        else if (!(ps->fl & SPRITE_FL_SHAPE_D21_IS_THUNK) &&
                 ps->shapeD21.hsurfAux && GreIsHandleValid(ps->shapeD21.hsurfAux))
            psurf = SURFACE_ShareLockSurface((HSURF)ps->shapeD21.hsurfAux);
    }
    EngReleaseSemaphore(ppdev->hsemDevLock);
    return psurf;
}

BOOL
NTAPI
EngpGdiGetSpriteAttributes(
    _In_ PPDEVOBJ ppdev,
    _In_ HWND hwnd,
    _In_opt_ PVOID pShapeHint,
    _Out_opt_ PULONG pulAttrA4,
    _Out_opt_ PBLENDFUNCTION pBlend,
    _Out_opt_ PULONG pulAttrA6)
{
    PSPRITESTATE pss;
    PSPRITE ps;
    BOOL ok = FALSE;

    if (!ppdev || !hwnd)
        return FALSE;

    pss = ppdev->pSpriteState;
    if (!pss)
        return FALSE;

    EngAcquireSemaphore(ppdev->hsemDevLock);
    ps = EngpSpriteGet(pss, hwnd, pShapeHint);
    if (ps && !(ps->fl & SPRITE_FL_SENTINEL))
    {
        if (pulAttrA4)
            *pulAttrA4 = ps->ulSpriteAttr41;
        if (pBlend)
            *pBlend = ps->BlendSprite;
        if (pulAttrA6)
            *pulAttrA6 = ps->ulSpriteAttr39;
        ok = TRUE;
    }
    EngReleaseSemaphore(ppdev->hsemDevLock);
    return ok;
}

BOOL
NTAPI
EngpSpriteTryGetExtents(
    _In_ PPDEVOBJ ppdev,
    _In_ HWND hwnd,
    _Out_ PULONG pcx,
    _Out_ PULONG pcy)
{
    PSPRITESTATE pss;
    PSPRITE ps;
    BOOL ok = FALSE;

    if (!ppdev || !hwnd || !pcx || !pcy)
        return FALSE;

    *pcx = *pcy = 0;
    pss = ppdev->pSpriteState;
    if (!pss)
        return FALSE;

    EngAcquireSemaphore(ppdev->hsemDevLock);
    ps = EngpSpriteGet(pss, hwnd, NULL);
    if (ps && !(ps->fl & SPRITE_FL_SENTINEL))
    {
        if (ps->x == (LONG)0x80000000 || ps->y == (LONG)0x80000000)
        {
            EngReleaseSemaphore(ppdev->hsemDevLock);
            return FALSE;
        }
        *pcx = ps->cx;
        *pcy = ps->cy;
        ok = TRUE;
    }
    EngReleaseSemaphore(ppdev->hsemDevLock);
    return ok;
}

ULONG_PTR
NTAPI
EngpDwmBlendStateFromSpriteAttrs(
    _In_opt_ const BLENDFUNCTION *bf,
    _In_ ULONG ulAttr39,
    _In_ ULONG ulAttr41)
{
    ULONG_PTR s = (ULONG_PTR)ulAttr39;

    if (bf)
    {
        if (bf->AlphaFormat & AC_SRC_ALPHA)
            s |= (ULONG_PTR)0x100;
        if (bf->SourceConstantAlpha != 255)
            s |= ((ULONG_PTR)(bf->SourceConstantAlpha & 0xFFu)) << 16;
        if (bf->BlendOp != AC_SRC_OVER || bf->BlendFlags != 0)
            s |= (ULONG_PTR)0x200;
    }
    if (ulAttr41)
        s |= ((ULONG_PTR)(ulAttr41 & 0xFFu)) << 24;
    return s;
}

BOOL
NTAPI
EngpGdiDeleteSprite(
    _In_ PPDEVOBJ ppdev,
    _In_opt_ HWND hwnd,
    _In_opt_ PVOID pShapeHint)
{
    PSPRITESTATE pss;
    PSPRITE ps;
    BOOL ok = FALSE;

    if (!ppdev || !hwnd)
        return FALSE;

    pss = ppdev->pSpriteState;
    if (!pss)
        return FALSE;

    EngAcquireSemaphore(ppdev->hsemDevLock);
    ps = EngpSpriteGet(pss, hwnd, pShapeHint);
    if (ps && !(ps->fl & SPRITE_FL_SENTINEL))
    {
        EngpSpriteZap(pss, ps);
        ok = TRUE;
    }
    EngReleaseSemaphore(ppdev->hsemDevLock);
    return ok;
}

VOID
FASTCALL
IntEngSpriteOnWindowCreated(_In_ PWND Wnd)
{
    PPDEVOBJ ppdev;
    RECTL rc;

    if (!Wnd || UserIsDesktopWindow(Wnd))
        return;
    if (Wnd->style & WS_CHILD)
        return;
    if (!gpmdev || !gpmdev->ppdevGlobal)
        return;

    ppdev = gpmdev->ppdevGlobal;
    if (!ppdev->pSpriteState)
        return;

    rc.left = Wnd->rcWindow.left;
    rc.top = Wnd->rcWindow.top;
    rc.right = Wnd->rcWindow.right;
    rc.bottom = Wnd->rcWindow.bottom;

    (VOID)EngpSpriteCreate((HDEV)ppdev, &rc, UserHMGetHandle(Wnd), NULL);
}

VOID
FASTCALL
IntEngSpriteOnWindowDestroyed(_In_ PWND Wnd)
{
    PPDEVOBJ ppdev;

    if (!Wnd || UserIsDesktopWindow(Wnd))
        return;
    if (Wnd->style & WS_CHILD)
        return;
    if (!gpmdev || !gpmdev->ppdevGlobal)
        return;

    ppdev = gpmdev->ppdevGlobal;
    (VOID)EngpGdiDeleteSprite(ppdev, UserHMGetHandle(Wnd), NULL);
}

VOID
FASTCALL
IntEngSpriteOnWindowPosChanged(_In_ PWND Wnd)
{
    PPDEVOBJ ppdev;
    RECTL rc;

    if (!Wnd || UserIsDesktopWindow(Wnd))
        return;
    if (Wnd->style & WS_CHILD)
        return;
    if (!gpmdev || !gpmdev->ppdevGlobal)
        return;

    ppdev = gpmdev->ppdevGlobal;
    if (!ppdev->pSpriteState)
        return;

    rc.left = Wnd->rcWindow.left;
    rc.top = Wnd->rcWindow.top;
    rc.right = Wnd->rcWindow.right;
    rc.bottom = Wnd->rcWindow.bottom;
    EngpSpriteUpdateLayout(ppdev, UserHMGetHandle(Wnd), &rc);
}

VOID
FASTCALL
IntEngSpriteOnZorderChanged(_In_ PWND Wnd, _In_ HWND hwndInsertAfter)
{
    PPDEVOBJ ppdev;
    PSPRITESTATE pss;
    PSPRITE ps;

    if (!Wnd || UserIsDesktopWindow(Wnd))
        return;
    if (Wnd->style & WS_CHILD)
        return;
    if (!gpmdev || !gpmdev->ppdevGlobal)
        return;

    ppdev = gpmdev->ppdevGlobal;
    pss = ppdev->pSpriteState;
    if (!pss)
        return;

    EngAcquireSemaphore(ppdev->hsemDevLock);
    ps = EngpSpriteGet(pss, UserHMGetHandle(Wnd), NULL);
    if (ps && !(ps->fl & SPRITE_FL_SENTINEL))
        EngpSpriteRelinkZOrder(pss, ps, hwndInsertAfter);
    EngReleaseSemaphore(ppdev->hsemDevLock);
}
