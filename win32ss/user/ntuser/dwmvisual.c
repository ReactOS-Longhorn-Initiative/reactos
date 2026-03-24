/*
 * PROJECT:         ReactOS Win32k
 * LICENSE:         GPL-2.0-or-later
 * PURPOSE:         Longhorn 5112-style DWM visual list (pFindVisual / gDceState analogue).
 *
 * Visuals are built from USER DCEs + hbmDwmRedirect and/or from EngpTransferSpriteStateToVisualState
 * (sprite z-list → ROS_DWM_VISUAL) so GreDwmGetSurfaceData matches 5112 compositing / sprite paths.
 */

#include <win32k.h>

#include "dce.h"
#include "dwmvisual.h"

#define TAG_DWM_VISUAL TAG_ROS_DWM_VISUAL

DBG_DEFAULT_CHANNEL(UserMisc);

#include <debug.h>

static PROS_DWM_VISUAL gpRosDwmVisualList;

static VOID FASTCALL
RosDwmVisualUpsertPwnd(_In_ PWND pwnd)
{
    PROS_DWM_VISUAL vis;
    HWND hwnd;
    SURFOBJ *psoNew;

    if (!pwnd)
        return;

    hwnd = UserHMGetHandle(pwnd);

    for (vis = gpRosDwmVisualList; vis; vis = vis->pNext)
    {
        if (vis->hwnd == hwnd)
            break;
    }

    if (!vis)
    {
        vis = ExAllocatePoolWithTag(PagedPool, sizeof(*vis), TAG_DWM_VISUAL);
        if (!vis)
            return;
        RtlZeroMemory(vis, sizeof(*vis));
        vis->pNext = gpRosDwmVisualList;
        gpRosDwmVisualList = vis;
        vis->hwnd = hwnd;
    }

    vis->rcScreen.left = pwnd->rcWindow.left;
    vis->rcScreen.top = pwnd->rcWindow.top;
    vis->rcScreen.right = pwnd->rcWindow.right;
    vis->rcScreen.bottom = pwnd->rcWindow.bottom;
    vis->Flags |= ROS_DWM_VISUAL_FLAG_VALID;

    if (pwnd->ExStyle & WS_EX_LAYERED)
        vis->ulSpriteAttr10 = 1;
    else
        vis->ulSpriteAttr10 = 0;

    psoNew = NULL;
    if (pwnd->hbmDwmRedirect && GreIsHandleValid(pwnd->hbmDwmRedirect))
        psoNew = EngLockSurface((HSURF)pwnd->hbmDwmRedirect);

    if (psoNew)
    {
        if (vis->hsurfEngAlloc)
        {
            if (vis->pso)
                EngUnlockSurface(vis->pso);
            GreDeleteObject((HBITMAP)vis->hsurfEngAlloc);
            vis->hsurfEngAlloc = NULL;
            vis->pso = NULL;
        }
        else if (vis->pso)
        {
            EngUnlockSurface(vis->pso);
        }
        vis->pso = psoNew;
    }
    else if (vis->hsurfEngAlloc)
    {
        /* Keep transfer-time DIB until a redirect bitmap appears. */
    }
    else if (vis->pso)
    {
        EngUnlockSurface(vis->pso);
        vis->pso = NULL;
    }
}

static VOID FASTCALL
RosDwmVisualUpsertFromDce(_In_ PDCE dce)
{
    PWND pwnd;

    if (!dce || (dce->DCXFlags & DCX_DCEEMPTY))
        return;

    /* Match DwmGreStartupEnumerate*: pwndOrg may be NULL while hwndCurrent is valid. */
    pwnd = dce->pwndOrg;
    if (!pwnd && dce->hwndCurrent)
        pwnd = UserGetWindowObject(dce->hwndCurrent);
    if (!pwnd || UserIsDesktopWindow(pwnd))
        return;

    RosDwmVisualUpsertPwnd(pwnd);
}

static VOID FASTCALL
RosDwmVisualEnumerateDce(_In_ PDCE dce, _In_opt_ PVOID Context)
{
    (void)Context;
    RosDwmVisualUpsertFromDce(dce);
}

VOID
FASTCALL
IntRosDwmFreeAllVisuals(VOID)
{
    PROS_DWM_VISUAL v, next;

    for (v = gpRosDwmVisualList; v; v = next)
    {
        next = v->pNext;
        if (v->pso)
        {
            EngUnlockSurface(v->pso);
            v->pso = NULL;
        }
        if (v->hsurfEngAlloc)
        {
            GreDeleteObject((HBITMAP)v->hsurfEngAlloc);
            v->hsurfEngAlloc = NULL;
        }
        ExFreePoolWithTag(v, TAG_DWM_VISUAL);
    }
    gpRosDwmVisualList = NULL;
}

VOID
FASTCALL
IntRosDwmRebuildVisualList(_In_ HDEV hdev)
{
    (void)hdev;

    /*
     * GreDwmStartup calls EngpTransferSpriteStateToVisualState first, which builds a visual list
     * from the sprite z-order. A full free here discarded that list so only DCE-org windows
     * reappeared — milcore then hit NOT_FOUND on AttachToHwnd and AV'd. Merge DCE state in.
     */
    DceEnumerateAll(RosDwmVisualEnumerateDce, NULL);
}

PROS_DWM_VISUAL
FASTCALL
IntRosDwmFindVisual(_In_opt_ HWND hwnd)
{
    PROS_DWM_VISUAL v;

    if (!hwnd)
        return NULL;

    for (v = gpRosDwmVisualList; v; v = v->pNext)
    {
        if (v->hwnd == hwnd)
            return v;
    }
    return NULL;
}

VOID
FASTCALL
IntRosDwmUpsertForPwnd(_In_ PWND pwnd)
{
    RosDwmVisualUpsertPwnd(pwnd);
}

VOID
FASTCALL
IntRosDwmReplaceVisualListHead(_In_opt_ PROS_DWM_VISUAL NewHead)
{
    IntRosDwmFreeAllVisuals();
    gpRosDwmVisualList = NewHead;
}

VOID
FASTCALL
IntRosDwmRemoveVisualForPwnd(_In_ PWND pwnd)
{
    PROS_DWM_VISUAL *pp, cur;
    HWND hwnd;

    if (!pwnd)
        return;

    hwnd = UserHMGetHandle(pwnd);
    for (pp = &gpRosDwmVisualList; *pp; pp = &(*pp)->pNext)
    {
        if ((*pp)->hwnd == hwnd)
        {
            cur = *pp;
            *pp = cur->pNext;
            if (cur->pso)
                EngUnlockSurface(cur->pso);
            if (cur->hsurfEngAlloc)
                GreDeleteObject((HBITMAP)cur->hsurfEngAlloc);
            ExFreePoolWithTag(cur, TAG_DWM_VISUAL);
            return;
        }
    }
}
