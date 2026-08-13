/*
 * PROJECT:     ReactOS Win32k subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USER side of DWM window content redirection
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * Phase 2 of the DWM bring-up: WINDOW CONTENT.
 *
 * Phase 1 (dwm.c) tells DWM where every window is. This tells DWM what is IN
 * them. The mechanism is not "win32k stops drawing" -- applications keep
 * painting exactly as they always did, through GetDC and BeginPaint, in window
 * coordinates. What changes is where those pixels land: each redirected
 * top-level window gets a bitmap of its own, and win32k swaps that bitmap in
 * as the DC's surface with an origin that maps window coordinates onto it.
 *
 * That distinction is the whole design and it is worth stating plainly,
 * because "turn off rendering when redirection is on" is the intuitive reading
 * and it is wrong -- it produces a black screen, not a composed one.
 *
 * Vista's win32k keeps this as a small set of functions whose names are still
 * in the symbol table; they are reproduced here under the same names:
 *
 *     _SetRedirectedWindow@8        _UnsetRedirectedWindow@8
 *     _ResetRedirectedWindows@4     _GetRedirectionFlags@4
 *     _CreateRedirectionBitmap@4    _RecreateRedirectionBitmap@16
 *     _DeleteRedirectionBitmap@4    _RemoveRedirectionBitmap@4
 *     _GetRedirectionBitmap@4       _SetRedirectionBitmap@8
 *     _GetOldRedirectionBitmap@4    _SetOldRedirectionBitmap@8
 *     _UserGetRedirectedWindowOrigin@8
 *     _UpdateRedirectedDC@4         _UpdateRedirectedDCs@0
 *     _gfStructuralRedirection
 *
 * WHAT IS NOT HERE YET. The bitmaps are allocated, kept in step with window
 * size and selected into the window's DCs, so application output goes into
 * them. Handing them to the compositor is the next piece: Vista carries the
 * surface to dwmredir through the sprite (CMilWindowContext::GetGDISurface and
 * the GdiSpriteBitmap commands), and our CREATESPRITE still carries no surface
 * handle. Until that lands, content redirection produces correct bitmaps that
 * nothing reads, which is why gfStructuralRedirection defaults to structural
 * mode and dwm.exe asks for it.
 */

#include <win32k.h>
#include "dwm.h"

DBG_DEFAULT_CHANNEL(UserDwm);

/*
 * Vista _gfStructuralRedirection. TRUE means geometry only: sprites carry
 * position and z-order, no window content is captured, and no redirection
 * bitmap is ever allocated. dwm.exe picks the mode --
 * DwmStartRedirection(!fStructuralMode) -- and passes FALSE today.
 */
BOOL gfStructuralRedirection = TRUE;

/* ------------------------------------------------------------------------- */
/*  Per-window accessors                                                      */
/* ------------------------------------------------------------------------- */

HBITMAP
FASTCALL
UserGetRedirectionBitmap(PWND Wnd)
{
    return Wnd ? (HBITMAP)Wnd->DwmRedirBitmap : NULL;
}

VOID
FASTCALL
UserSetRedirectionBitmap(PWND Wnd, HBITMAP hbm)
{
    if (Wnd)
        Wnd->DwmRedirBitmap = (HANDLE)hbm;
}

HBITMAP
FASTCALL
UserGetOldRedirectionBitmap(PWND Wnd)
{
    return Wnd ? (HBITMAP)Wnd->DwmRedirOldBitmap : NULL;
}

VOID
FASTCALL
UserSetOldRedirectionBitmap(PWND Wnd, HBITMAP hbm)
{
    if (Wnd)
        Wnd->DwmRedirOldBitmap = (HANDLE)hbm;
}

UINT32
FASTCALL
UserGetRedirectionFlags(PWND Wnd)
{
    return Wnd ? Wnd->DwmRedirFlags : 0;
}

/*
 * TRUE only when this window's output should go to a bitmap right now: content
 * redirection is on, the window was marked redirected, and it actually has a
 * bitmap. All three are checked because they come apart -- a window marked
 * redirected whose bitmap allocation failed must keep drawing to the primary
 * rather than lose its output entirely.
 */
BOOL
FASTCALL
UserIsWindowRedirected(PWND Wnd)
{
    if (Wnd == NULL || gfStructuralRedirection)
        return FALSE;

    if (!(Wnd->DwmRedirFlags & DWM_REDIRF_REDIRECTED))
        return FALSE;

    return (Wnd->DwmRedirBitmap != NULL);
}

/*
 * Vista _UserGetRedirectedWindowOrigin@8.
 *
 * The redirection bitmap covers rcWindow and its top-left IS bitmap (0,0), so
 * a point given in screen coordinates maps into the bitmap by subtracting
 * rcWindow's top-left. This is what a redirected DC's origin becomes: a client
 * DC, whose logical origin is rcClient's top-left on screen, ends up at
 * (rcClient.left - rcWindow.left, rcClient.top - rcWindow.top) in the bitmap,
 * i.e. exactly the non-client border thickness -- which is where the client
 * area belongs inside a window-sized surface.
 */
VOID
FASTCALL
UserGetRedirectedWindowOrigin(PWND Wnd, PPOINT ppt)
{
    if (ppt == NULL)
        return;

    if (Wnd == NULL)
    {
        ppt->x = 0;
        ppt->y = 0;
        return;
    }

    ppt->x = Wnd->rcWindow.left;
    ppt->y = Wnd->rcWindow.top;
}

/* ------------------------------------------------------------------------- */
/*  Bitmap lifetime                                                           */
/* ------------------------------------------------------------------------- */

/*
 * Vista _CreateRedirectionBitmap@4.
 *
 * 32bpp BGRA, window-sized, top-down. The format is not a choice: DWM samples
 * these as premultiplied BGRA textures, and every other depth would need a
 * conversion on the composition path that Vista does not have.
 *
 * A zero-area window gets no bitmap and no failure. Windows legitimately exist
 * at 0x0 -- message-only windows, windows mid-creation before their first
 * WM_SIZE -- and treating that as an error would fail their creation.
 */
/*
 * The stride the compositor will be told about, and the one GDI must lay the
 * surface out with. DWORD-aligned 32bpp, which is what GreCreateBitmapEx
 * computes for itself when given 0 -- it is spelled out here because the same
 * number goes on the wire in DWMSURFACEDATA::dwStride, and the two have to be
 * the same number or dwmredir maps the section and reads every row at the
 * wrong offset.
 */
FORCEINLINE
UINT32
IntDwmRedirStride(LONG cx)
{
    return (UINT32)(((cx * 4) + 3) & ~3);
}

static
HBITMAP
FASTCALL
IntDwmCreateRedirectionBitmapForSize(
    LONG cx,
    LONG cy,
    PVOID *ppSectionObject,
    PVOID *ppView,
    UINT32 *pcjView)
{
    HBITMAP hbm;
    NTSTATUS Status;
    PVOID pSectionObject = NULL;
    PVOID pvView = NULL;
    SIZE_T cjView = 0;
    LARGE_INTEGER liSize;
    UINT32 cjStride;

    *ppSectionObject = NULL;
    *ppView = NULL;
    *pcjView = 0;

    if (cx <= 0 || cy <= 0)
        return NULL;

    cjStride = IntDwmRedirStride(cx);
    liSize.QuadPart = (LONGLONG)cjStride * (LONGLONG)cy;

    /*
     * PAGE_READWRITE + SEC_COMMIT. The compositor maps it read-only on its
     * side; the pages have to be committed up front because GDI will draw into
     * them from arbitrary contexts and a fault-in on a paged section during a
     * blt is not something the graphics path is prepared for.
     */
    Status = MmCreateSection(&pSectionObject,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &liSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Redirection section %ldx%ld failed 0x%08lx\n", cx, cy, Status);
        return NULL;
    }

    /*
     * SYSTEM space. See the note on WND::DwmRedirSectionView -- a view in the
     * creating process would be invalid the moment another process painted
     * this window.
     */
    Status = MmMapViewInSystemSpace(pSectionObject, &pvView, &cjView);
    if (!NT_SUCCESS(Status))
    {
        ERR("Redirection view %ldx%ld failed 0x%08lx\n", cx, cy, Status);
        ObDereferenceObject(pSectionObject);
        return NULL;
    }

    /*
     * Blank, not garbage. MmCreateSection commits zeroed pages, so this costs
     * nothing today -- it is here because the surface must be blank and that
     * must not silently depend on MM's zeroing policy.
     */
    RtlZeroMemory(pvView, cjView);

    /*
     * BMF_TOPDOWN, and the bits are the section view rather than NULL: passing
     * NULL would have GDI allocate pool, which is exactly the storage the
     * compositor cannot reach.
     */
    hbm = GreCreateBitmapEx(cx,
                            cy,
                            cjStride,
                            BMF_32BPP,
                            BMF_TOPDOWN,
                            (ULONG)(cjStride * cy),
                            pvView,
                            DDB_SURFACE);
    if (hbm == NULL)
    {
        ERR("Redirection bitmap %ldx%ld: allocation failed\n", cx, cy);
        MmUnmapViewInSystemSpace(pvView);
        ObDereferenceObject(pSectionObject);
        return NULL;
    }

    /*
     * Kernel ownership. These are win32k's, not the creating process's -- a
     * process exiting must not take the surface DWM is compositing with it.
     */
    GreSetBitmapOwner(hbm, GDI_OBJ_HMGR_PUBLIC);

    *ppSectionObject = pSectionObject;
    *ppView = pvView;
    *pcjView = (UINT32)cjView;

    return hbm;
}

HBITMAP
FASTCALL
IntDwmCreateRedirectionBitmap(PWND Wnd)
{
    HBITMAP hbm;
    LONG cx, cy;
    PVOID pSectionObject = NULL;
    PVOID pvView = NULL;
    UINT32 cjView = 0;

    if (Wnd == NULL || gfStructuralRedirection)
        return NULL;

    /* Already has one. */
    if (Wnd->DwmRedirBitmap != NULL)
        return (HBITMAP)Wnd->DwmRedirBitmap;

    cx = Wnd->rcWindow.right - Wnd->rcWindow.left;
    cy = Wnd->rcWindow.bottom - Wnd->rcWindow.top;

    hbm = IntDwmCreateRedirectionBitmapForSize(cx, cy,
                                               &pSectionObject,
                                               &pvView,
                                               &cjView);
    if (hbm == NULL)
        return NULL;

    UserSetRedirectionBitmap(Wnd, hbm);
    Wnd->DwmRedirSectionObject = pSectionObject;
    Wnd->DwmRedirSectionView   = pvView;
    Wnd->DwmRedirSectionSize   = cjView;

    TRACE("Redirection bitmap %p created for %p (%ldx%ld)\n",
          hbm, Wnd->head.h, cx, cy);

    return hbm;
}

/*
 * Vista _RemoveRedirectionBitmap@4 / _DeleteRedirectionBitmap@4.
 *
 * Remove detaches, delete frees. They are separate in Vista because the old
 * bitmap is detached from the window well before it can be freed -- a DC may
 * still hold it selected.
 */
VOID
FASTCALL
IntDwmRemoveRedirectionBitmap(PWND Wnd)
{
    HBITMAP hbm, hbmOld;
    PVOID pSectionObject, pvView;

    if (Wnd == NULL)
        return;

    hbm = UserGetRedirectionBitmap(Wnd);
    hbmOld = UserGetOldRedirectionBitmap(Wnd);
    pSectionObject = Wnd->DwmRedirSectionObject;
    pvView = Wnd->DwmRedirSectionView;

    UserSetRedirectionBitmap(Wnd, NULL);
    UserSetOldRedirectionBitmap(Wnd, NULL);
    Wnd->DwmRedirSectionObject = NULL;
    Wnd->DwmRedirSectionView   = NULL;
    Wnd->DwmRedirSectionSize   = 0;

    /*
     * The bitmap goes FIRST. Its SURFOBJ points straight at the section view,
     * so unmapping the view before the surface is gone leaves GDI holding a
     * pointer into unmapped system space -- and unlike a use-after-free in
     * pool, that one bugchecks on the next draw rather than corrupting quietly.
     */
    if (hbm != NULL)
        GreDeleteObject(hbm);

    /* Kept for symmetry with Vista's slot; see the note in Recreate on why
     * nothing ever puts a bitmap here today. */
    if (hbmOld != NULL)
        GreDeleteObject(hbmOld);

    if (pvView != NULL)
        MmUnmapViewInSystemSpace(pvView);

    if (pSectionObject != NULL)
        ObDereferenceObject(pSectionObject);
}

/*
 * Vista _RecreateRedirectionBitmap@16. Called when the window changes size.
 *
 * The previous bitmap becomes the OLD one rather than being freed straight
 * away. A window that has been resized has not repainted yet -- the WM_PAINT
 * for the new size is still queued -- so the compositor would otherwise have
 * one frame with nothing to show. Vista keeps the stale pixels until the new
 * surface has been painted at least once.
 *
 * The bitmap is not resized in place because GDI has no such operation: the
 * surface's dimensions and stride are fixed at creation, and every DC holding
 * it caches dclevel.sizl from it.
 */
HBITMAP
FASTCALL
IntDwmRecreateRedirectionBitmap(PWND Wnd)
{
    HBITMAP hbmNew, hbmPrev;
    LONG cx, cy;
    PVOID pSectionObject = NULL;
    PVOID pvView = NULL;
    UINT32 cjView = 0;
    PVOID pOldSectionObject, pvOldView;

    if (Wnd == NULL || gfStructuralRedirection)
        return NULL;

    if (!(Wnd->DwmRedirFlags & DWM_REDIRF_REDIRECTED))
        return NULL;

    cx = Wnd->rcWindow.right - Wnd->rcWindow.left;
    cy = Wnd->rcWindow.bottom - Wnd->rcWindow.top;

    hbmPrev = UserGetRedirectionBitmap(Wnd);

    /* Same size: nothing to do. Resize notifications arrive for pure moves
     * too, and reallocating on those would throw away good pixels every time
     * a window is dragged. */
    if (hbmPrev != NULL)
    {
        SIZEL sizl;
        if (GreGetBitmapPixelSize(hbmPrev, &sizl) &&
            sizl.cx == cx && sizl.cy == cy)
        {
            return hbmPrev;
        }
    }

    hbmNew = IntDwmCreateRedirectionBitmapForSize(cx, cy,
                                                  &pSectionObject,
                                                  &pvView,
                                                  &cjView);
    if (hbmNew == NULL && cx > 0 && cy > 0)
    {
        /* Keep what we have rather than leaving the window with nothing. */
        return hbmPrev;
    }

    /*
     * DEVIATION, deliberate: the previous surface is FREED here, not retired
     * into the old slot.
     *
     * Vista keeps one generation so a window that has been resized but not yet
     * repainted still has its old pixels to show. That only works if the old
     * surface's section stays mapped for as long as its bitmap lives, which
     * means tracking two sections per window -- and nothing in this tree reads
     * the old slot yet, so the second generation would be pure lifetime risk
     * for no visible benefit. Keeping the slot but leaving its section untracked
     * would be worse still: GreDeleteObject would free a SURFOBJ pointing into
     * a view that was already unmapped.
     *
     * The slot stays because Vista has it and because the stale-pixel behaviour
     * is worth adding once something consumes it; it is simply never filled.
     */
    pOldSectionObject = Wnd->DwmRedirSectionObject;
    pvOldView         = Wnd->DwmRedirSectionView;

    UserSetRedirectionBitmap(Wnd, hbmNew);
    Wnd->DwmRedirSectionObject = pSectionObject;
    Wnd->DwmRedirSectionView   = pvView;
    Wnd->DwmRedirSectionSize   = cjView;

    /* Surface before storage, same ordering rule as IntDwmRemoveRedirectionBitmap. */
    if (hbmPrev != NULL)
        GreDeleteObject(hbmPrev);

    if (pvOldView != NULL)
        MmUnmapViewInSystemSpace(pvOldView);

    if (pOldSectionObject != NULL)
        ObDereferenceObject(pOldSectionObject);

    TRACE("Redirection bitmap for %p resized to %ldx%ld (%p -> %p)\n",
          Wnd->head.h, cx, cy, hbmPrev, hbmNew);

    return hbmNew;
}

/* ------------------------------------------------------------------------- */
/*  Marking windows redirected                                                */
/* ------------------------------------------------------------------------- */

/*
 * Vista _SetRedirectedWindow@8.
 *
 * Only TOP-LEVEL windows are redirected. A child draws into its parent's
 * surface -- that is what makes a child a child -- and giving it one of its
 * own would separate it from the window it is part of, so its output would
 * vanish from the composed frame. Vista's sprite model has the same shape:
 * NotifyChildCreate registers every window, CreateSprite covers top-level
 * ones.
 */
BOOL
FASTCALL
IntDwmSetRedirectedWindow(PWND Wnd)
{
    if (Wnd == NULL || gfStructuralRedirection)
        return FALSE;

    /* Desktop and top-level only. A window whose parent is the desktop is
     * top-level; anything else is a child. */
    if (Wnd->spwndParent != NULL &&
        Wnd->spwndParent != UserGetDesktopWindow())
    {
        return FALSE;
    }

    if (Wnd->DwmRedirFlags & DWM_REDIRF_REDIRECTED)
        return TRUE;

    Wnd->DwmRedirFlags |= DWM_REDIRF_REDIRECTED;

    if (IntDwmCreateRedirectionBitmap(Wnd) == NULL)
    {
        /* No surface, no redirection -- UserIsWindowRedirected checks for the
         * bitmap as well as the flag, so this window simply keeps drawing to
         * the primary until it is resized to something allocatable. */
        TRACE("Redirection for %p deferred: no bitmap yet\n", Wnd->head.h);
    }

    return TRUE;
}

/* Vista _UnsetRedirectedWindow@8. */
VOID
FASTCALL
IntDwmUnsetRedirectedWindow(PWND Wnd)
{
    if (Wnd == NULL)
        return;

    Wnd->DwmRedirFlags &= ~DWM_REDIRF_REDIRECTED;

    IntDwmRemoveRedirectionBitmap(Wnd);
}

/*
 * Vista _ResetRedirectedWindows@4. The bulk pass at start and stop.
 *
 * Redirection is switched on long after the desktop exists, so the windows
 * that need surfaces are almost all of them -- the same reason
 * IntDwmStartRedirection has to enumerate for sprites.
 */
static
VOID
IntDwmResetOne(PWND Wnd)
{
    if (gfStructuralRedirection)
        IntDwmUnsetRedirectedWindow(Wnd);
    else
        IntDwmSetRedirectedWindow(Wnd);
}

VOID
FASTCALL
IntDwmResetRedirectedWindows(VOID)
{
    IntDwmForEachOnDesktop(IntDwmResetOne);

    /*
     * Existing DCs still point at whatever they pointed at before. Vista
     * follows the same bulk pass with _UpdateRedirectedDCs@0 for exactly this
     * reason: a DC handed out before the switch would keep drawing to the
     * primary through a surface pointer it already holds.
     */
    IntDwmUpdateRedirectedDCs();
}

/* ------------------------------------------------------------------------- */
/*  Window lifecycle hooks                                                    */
/* ------------------------------------------------------------------------- */

VOID
FASTCALL
IntDwmRedirOnWindowCreated(PWND Wnd)
{
    if (gfStructuralRedirection)
        return;

    IntDwmSetRedirectedWindow(Wnd);
}

VOID
FASTCALL
IntDwmRedirOnWindowDestroyed(PWND Wnd)
{
    /*
     * NOT gated on the mode. A window created while content redirection was on
     * still owns a bitmap after the mode is switched off, and skipping the
     * teardown here would leak it for the lifetime of the session.
     */
    IntDwmUnsetRedirectedWindow(Wnd);
}

VOID
FASTCALL
IntDwmRedirOnWindowSized(PWND Wnd)
{
    if (gfStructuralRedirection)
        return;

    if (Wnd == NULL || !(Wnd->DwmRedirFlags & DWM_REDIRF_REDIRECTED))
        return;

    /* A window that had no bitmap because it was 0x0 gets one here. */
    if (Wnd->DwmRedirBitmap == NULL)
        (VOID)IntDwmCreateRedirectionBitmap(Wnd);
    else
        (VOID)IntDwmRecreateRedirectionBitmap(Wnd);
}

/* ------------------------------------------------------------------------- */
/*  Handing the surface to the compositor                                     */
/* ------------------------------------------------------------------------- */

/*
 * Vista _NtGdiDwmGetSurfaceData@8, reached from dwmredir through
 * gdi32!DwmGetSurfaceData and consumed by
 * CMilWindowContext::GetNewSurfaceData (dwmredir.dll.c:10564).
 *
 * KEYED ON THE SPRITE, not the window. That is dwmredir's identifier for a
 * composed window -- GetNewSurfaceData has m_hsprite and nothing else at that
 * point -- so the lookup walks the desktop for the window carrying it.
 *
 * The section handle handed back is a NEW handle in the CALLER's process. This
 * runs in dwm.exe's context on its own thread, so ObOpenObjectByPointer with
 * UserMode access mode puts the handle straight in its table; dwmredir closes
 * it after NtMapViewOfSection, and the mapping keeps the section alive from
 * there.
 *
 * The struct is the contract: seven DWORDs, and dwmredir zeroes exactly those
 * seven when the query fails. Layout in dwmredir/RedirCommands.hpp.
 */
static
PWND
FASTCALL
IntDwmFindWindowBySprite(UINT32 hSprite);

BOOL
APIENTRY
NtGdiDwmGetSurfaceData(
    UINT32 hSprite,
    PDWM_SURFACE_DATA pUnsafeData)
{
    DWM_SURFACE_DATA Data;
    PWND Wnd;
    SIZEL sizl;
    HANDLE hSectionUser = NULL;
    NTSTATUS Status;
    BOOL bRet = FALSE;

    RtlZeroMemory(&Data, sizeof(Data));

    UserEnterExclusive();

    if (gfStructuralRedirection)
    {
        /* Structural mode has no surfaces at all. Reporting failure rather
         * than an empty struct is what makes dwmredir leave the window on the
         * geometry-only path instead of trying to compose from nothing. */
        goto Cleanup;
    }

    Wnd = IntDwmFindWindowBySprite(hSprite);
    if (Wnd == NULL || !UserIsWindowRedirected(Wnd))
        goto Cleanup;

    if (Wnd->DwmRedirSectionObject == NULL)
        goto Cleanup;

    if (!GreGetBitmapPixelSize((HBITMAP)Wnd->DwmRedirBitmap, &sizl))
        goto Cleanup;

    /*
     * A handle in the compositor's process. SECTION_MAP_READ only: dwmredir
     * maps it to sample the pixels and has no business writing into a surface
     * that GDI owns.
     */
    Status = ObOpenObjectByPointer(Wnd->DwmRedirSectionObject,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   SECTION_MAP_READ | SECTION_QUERY,
                                   NULL,
                                   UserMode,
                                   &hSectionUser);
    if (!NT_SUCCESS(Status))
    {
        ERR("DwmGetSurfaceData: ObOpenObjectByPointer 0x%08lx\n", Status);
        goto Cleanup;
    }

    Data.hSection       = hSectionUser;
    Data.nWidth         = (UINT32)sizl.cx;
    Data.nHeight        = (UINT32)sizl.cy;
    Data.dwGdiFormat    = BI_RGB;
    Data.dwGdiFormatAux = 32;                       /* bits per pixel */
    Data.dwStride       = IntDwmRedirStride(sizl.cx);
    /*
     * High byte of slot 6 is dwmredir's "the surface itself declares alpha"
     * flag, tested against 1 in GetNewSurfaceData. A 32bpp redirection surface
     * does carry alpha -- that is the point of compositing it.
     */
    Data.dwFlags        = 0x01000000;

    bRet = TRUE;

Cleanup:
    UserLeave();

    /* Copy out AFTER the lock is dropped -- the probe can fault, and faulting
     * with the USER lock held deadlocks anything else that needs it. */
    _SEH2_TRY
    {
        ProbeForWrite(pUnsafeData, sizeof(Data), sizeof(ULONG));
        RtlCopyMemory(pUnsafeData, &Data, sizeof(Data));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
    }
    _SEH2_END;

    if (!bRet && hSectionUser != NULL)
    {
        /* The caller never saw the handle, so nobody will close it. */
        ObCloseHandle(hSectionUser, UserMode);
    }

    return bRet;
}

/* ------------------------------------------------------------------------- */

static PWND g_pWndSpriteFound;
static UINT32 g_hSpriteWanted;

static
VOID
IntDwmMatchSprite(PWND Wnd)
{
    if (g_pWndSpriteFound == NULL &&
        Wnd != NULL &&
        Wnd->DwmSprite != 0 &&
        Wnd->DwmSprite == g_hSpriteWanted)
    {
        g_pWndSpriteFound = Wnd;
    }
}

/*
 * A linear walk, deliberately. The alternative is a sprite -> window map, and
 * this runs once per surface creation or resize -- not per frame -- against a
 * tree win32k already walks for every redirection pass. A map would be a
 * second thing to keep in step with window destruction for no measurable gain.
 *
 * Callers hold the USER lock, which is also what makes the two statics safe.
 */
static
PWND
FASTCALL
IntDwmFindWindowBySprite(UINT32 hSprite)
{
    if (hSprite == 0)
        return NULL;

    g_pWndSpriteFound = NULL;
    g_hSpriteWanted = hSprite;

    IntDwmForEachOnDesktop(IntDwmMatchSprite);

    return g_pWndSpriteFound;
}

/* EOF */
