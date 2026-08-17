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
 * handle.
 *
 * So content redirection is now REQUESTED (dwm.exe passes TRUE) while that
 * handoff is still missing, and the consequence is known rather than
 * discovered: window content is captured correctly into bitmaps that nothing
 * reads, so window interiors do not reach the screen. Frames and glass still
 * do -- uDWM builds those itself from the theme atlas, and they never went
 * through a redirection bitmap.
 *
 * That is a deliberate intermediate state, not a working one. It is on so that
 * the redirection layer actually executes -- every gate below had never run a
 * single time while structural mode was selected -- and so the compositor owns
 * the primary alone instead of fighting win32k for it. GetGDISurface is what
 * finishes it.
 */

#include <win32k.h>
#include "dwm.h"

DBG_DEFAULT_CHANNEL(UserDwm);

/*
 * Vista _gfStructuralRedirection. TRUE means geometry only: sprites carry
 * position and z-order, no window content is captured, and no redirection
 * bitmap is ever allocated. dwm.exe picks the mode --
 * DwmStartRedirection(!fStructuralMode) -- and passes TRUE for content today,
 * which lands here as FALSE.
 *
 * The initialiser stays TRUE: before dwm.exe has said anything, no redirection
 * is active at all (gbDwmRedirectionActive is FALSE and every emitter gates on
 * it), and structural is the mode that allocates nothing. A boot-time default
 * that captured content would be allocating bitmaps for a compositor that does
 * not exist yet.
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
/*
 * The window whose redirection bitmap Wnd's output belongs in: itself if it is
 * redirected, otherwise its nearest redirected ancestor, otherwise NULL.
 *
 * CHILDREN DO NOT OWN BITMAPS AND MUST NOT. IntDwmSetRedirectedWindow marks
 * only top-level windows, which is correct -- a child composes into the same
 * surface as the window it is part of, and giving it a surface of its own
 * would separate its output from its frame. But refusing it a bitmap is only
 * half the model: the other half is pointing it at the ancestor's, and that
 * half was missing. Without it UserIsWindowRedirected answered FALSE for every
 * child, DceSetDrawable converted the DC back to a plain display DC, and every
 * toolbar, list view, scrollbar and button painted straight to the primary --
 * which is nearly all of the visible desktop, so redirection looked inert even
 * though the top-level frames really were being captured.
 *
 * The walk is bounded by the window tree, which is already bounded elsewhere
 * in win32k, and stops at the desktop.
 */
PWND
FASTCALL
UserGetRedirectionTarget(PWND Wnd)
{
    PWND Desktop;

    if (Wnd == NULL || gfStructuralRedirection)
        return NULL;

    Desktop = UserGetDesktopWindow();

    while (Wnd != NULL)
    {
        if (UserIsWindowRedirected(Wnd))
            return Wnd;

        /* The desktop is the top of the walk; it has no parent to climb to. */
        if (Wnd == Desktop)
            break;

        Wnd = Wnd->spwndParent;
    }

    return NULL;
}

/*
 * Vista _UserGetRedirectedWindowOrigin@8, corrected to answer for the surface
 * the window actually draws into rather than for the window itself.
 *
 * For a top-level window the two are the same and the result is unchanged. For
 * a child it is the ANCESTOR's rcWindow top-left, so that DceSetDrawable's
 * `rect - ptOrg` places the child at its true offset inside the ancestor's
 * surface instead of at (0,0) of a bitmap it does not have.
 */
VOID
FASTCALL
UserGetRedirectedWindowOrigin(PWND Wnd, PPOINT ppt)
{
    PWND Target;

    if (ppt == NULL)
        return;

    ppt->x = 0;
    ppt->y = 0;

    if (Wnd == NULL)
        return;

    Target = UserGetRedirectionTarget(Wnd);

    /* No redirection target: report the window's own origin, which is what
     * the unredirected path expects. */
    if (Target == NULL)
        Target = Wnd;

    ppt->x = Target->rcWindow.left;
    ppt->y = Target->rcWindow.top;
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

    /*
     * ANNOUNCE THE NEW SURFACE.
     *
     * The notification belongs to the surface lifecycle, not to any one call
     * site. It used to live only in winpos.c beside IntDwmRedirOnWindowSized,
     * which left every other path that allocates a bitmap silent -- including
     * the startup sweep, which allocates one for EVERY window that already
     * exists. Those windows had sprites, had real bitmaps, and the compositor
     * was never told, so it kept the 0x0 surface it had queried at
     * CreateSprite time and composed nothing for the entire session.
     *
     * Gated on the window already having a sprite: if it does not, the
     * CreateSprite that follows carries the geometry itself and DWM queries
     * the surface as part of handling it.
     */
    if (Wnd->DwmSprite != 0)
        IntDwmUpdateSprite(Wnd);

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

    /*
     * The section changed identity, so the compositor's mapped view now points
     * at a section that no longer backs this window. It must re-query, and the
     * same-size early return above means this only fires when the surface
     * really was replaced -- a drag does not reallocate and does not notify.
     */
    if (Wnd->DwmSprite != 0)
        IntDwmUpdateSprite(Wnd);

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
    HBITMAP hbm;

    if (Wnd == NULL || gfStructuralRedirection)
        return FALSE;

    /*
     * NEVER REDIRECT THE DESKTOP WINDOW ITSELF.
     *
     * It is the compositor's OUTPUT, not one of its inputs.
     * CMilDesktopRenderTargetDuce creates its render target on
     * GetDesktopWindow() (VistaDwmResources.cpp:396), so the software
     * presenter's final GDI blit goes to the desktop window's DC. Give that
     * window a redirection bitmap and DceSetDrawable points the DC at the
     * bitmap, so every composed frame is delivered into a surface nobody
     * displays -- Render S_OK, Present S_OK, black screen.
     *
     * This is the same defect as redirecting dwm.exe's own windows, one level
     * up, and it survived that fix because the desktop window belongs to
     * win32k, not to the compositor's process. The test below did not catch it
     * either: the desktop window has spwndParent == NULL, so "is my parent the
     * desktop?" admits it rather than rejecting it.
     */
    if (Wnd == UserGetDesktopWindow())
    {
        return FALSE;
    }

    /* Top-level only. A window whose parent is the desktop is top-level;
     * anything else is a child. */
    if (Wnd->spwndParent != NULL &&
        Wnd->spwndParent != UserGetDesktopWindow())
    {
        return FALSE;
    }

    /*
     * NEVER REDIRECT THE COMPOSITOR'S OWN WINDOWS.
     *
     * dwm.exe's composition window is top-level and parented to the desktop,
     * so every test above admits it -- and redirecting it is self-defeating in
     * a way that reports success at every layer:
     *
     *   1. the window gets a redirection bitmap like any other,
     *   2. DceSetDrawable retargets its DC at that bitmap,
     *   3. milcore finishes each frame in CSwPresenter32bppGDI::Present with a
     *      GDI blit to that DC (this is the SoftwareOnly path -- there is no
     *      D3D device here, see "Could not open device \Device\Video1"),
     *   4. so the composed desktop lands in DWM's own redirection surface.
     *
     * Render returns S_OK, Present returns S_OK, the sprite bitmaps are all
     * correct, and the screen never changes. Nothing in the pipeline can
     * report this, because nothing in it is failing.
     *
     * Vista has the same requirement -- the compositor's output surface cannot
     * itself be a redirected surface -- and this is where the exclusion has to
     * live, because this is the single function that decides what gets one.
     */
    if (gpepDwm != NULL &&
        Wnd->head.pti != NULL &&
        Wnd->head.pti->ppi != NULL &&
        Wnd->head.pti->ppi->peProcess == gpepDwm)
    {
        TRACE("DwmRedir: hwnd %p belongs to the compositor, not redirecting\n",
              Wnd->head.h);
        return FALSE;
    }

    if (Wnd->DwmRedirFlags & DWM_REDIRF_REDIRECTED)
        return TRUE;

    Wnd->DwmRedirFlags |= DWM_REDIRF_REDIRECTED;

    hbm = IntDwmCreateRedirectionBitmap(Wnd);

    /*
     * BOTH OUTCOMES GET LOGGED, because they are indistinguishable downstream.
     * A window whose bitmap allocation failed keeps its REDIRECTED flag and
     * quietly carries on drawing to the primary -- deliberately, so its output
     * is visibly wrong rather than invisibly lost -- and UserIsWindowRedirected
     * then answers FALSE for it forever. Nothing further in the session says
     * which of the two happened, so "redirection is on" and "redirection is on
     * and allocating nothing" read identically in a log.
     *
     * Bounded: this fires once per top-level window and the startup sweep
     * walks every window that already exists.
     */
    {
        static LONG s_cLogged = 0;
        if (InterlockedIncrement(&s_cLogged) <= 24)
        {
            ERR("DwmRedir: hwnd %p %dx%d -> bitmap %p\n",
                Wnd->head.h,
                Wnd->rcWindow.right - Wnd->rcWindow.left,
                Wnd->rcWindow.bottom - Wnd->rcWindow.top,
                hbm);
        }
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

/*
 * Allocate (or tear down) the surfaces only. Split out from
 * IntDwmResetRedirectedWindows so the startup path can run it BEFORE sprites
 * are announced while still refreshing DCs afterwards -- see
 * IntDwmStartRedirection for why both orderings have to hold at once.
 */
VOID
FASTCALL
IntDwmResetRedirectedWindowSurfaces(VOID)
{
    IntDwmForEachOnDesktop(IntDwmResetOne);
}

VOID
FASTCALL
IntDwmResetRedirectedWindows(VOID)
{
    IntDwmResetRedirectedWindowSurfaces();

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

    /*
     * [RWM] Every gate below used to bail silently, so a caller saw one
     * outcome -- an all-zero struct -- for five different causes. dwmredir
     * reports exactly that as "0x0 stride=0 fmt=0 section=0", which says
     * nothing about which link failed. Name them, then STOP.
     *
     * The break is one-shot and deliberately fires AFTER the message: the
     * reason is in the log before the machine halts, so the trap is readable
     * even if the debugger is not attached to catch it. Continuing past it
     * will not re-trap -- one failure repeats per window, and eight identical
     * breaks would be worse than none.
     *
     * Remove the break once this is diagnosed; the messages are worth keeping.
     */
    {
        static LONG s_fBroke = 0;

#define RWM_SURFDATA_BAIL(reason)                                              \
        do {                                                                   \
            ERR("DwmGetSurfaceData(sprite=%lu): %s\n",                         \
                (unsigned long)hSprite, (reason));                             \
            goto Cleanup;                                                      \
        } while (0)

    if (gfStructuralRedirection)
    {
        /* Structural mode has no surfaces at all. Reporting failure rather
         * than an empty struct is what makes dwmredir leave the window on the
         * geometry-only path instead of trying to compose from nothing. */
        RWM_SURFDATA_BAIL("structural mode");
    }

    Wnd = IntDwmFindWindowBySprite(hSprite);
    if (Wnd == NULL)
    {
        /*
         * Dump what the table DOES hold, once.
         *
         * "No window carries this sprite" has two very different causes and
         * the bare message cannot separate them: the sprite was cleared before
         * the query (the window was destroyed -- IntDwmDestroySprite zeroes
         * DwmSprite), or the id dwmredir asked about never matched what win32k
         * stores. Printing the live set answers both at once: if the wanted id
         * is absent but its neighbours are present, it is a lifetime race; if
         * the whole set is offset or empty, it is an identity problem.
         */
        static LONG s_fDumped = 0;

        if (InterlockedCompareExchange(&s_fDumped, 1, 0) == 0 &&
            gHandleTable != NULL)
        {
            int i, cShown = 0;

            ERR("DwmGetSurfaceData: sprite=%lu not found. Live sprites:\n",
                (unsigned long)hSprite);

            for (i = 0; i < gHandleTable->nb_handles && cShown < 24; i++)
            {
                PUSER_HANDLE_ENTRY e = &gHandleTable->handles[i];
                PWND w;

                if (e->type != TYPE_WINDOW || e->ptr == NULL)
                    continue;

                w = (PWND)e->ptr;
                if (w->DwmSprite == 0)
                    continue;

                ERR("    hwnd %p sprite=%lu flags=0x%lx bitmap=%p %dx%d\n",
                    w->head.h, (unsigned long)w->DwmSprite,
                    (unsigned long)w->DwmRedirFlags, (PVOID)w->DwmRedirBitmap,
                    w->rcWindow.right - w->rcWindow.left,
                    w->rcWindow.bottom - w->rcWindow.top);
                cShown++;
            }
        }

        RWM_SURFDATA_BAIL("no window carries this sprite");
    }

    if (!UserIsWindowRedirected(Wnd))
    {
        /*
         * THE ZERO-AREA CASE IS NOT A FAILURE, and must not trap.
         *
         * IntDwmSetRedirectedWindow sets DWM_REDIRF_REDIRECTED and then tries
         * to allocate; a 0x0 window legitimately gets the flag and no bitmap,
         * and IntDwmRedirOnWindowSized allocates one if it is ever given a
         * real size. Reporting no surface is the correct answer here.
         *
         * These windows are also the FIRST ones swept at startup -- IME and
         * helper windows -- so a break that fires on the first failure fires
         * on this every boot and never reaches a real one. Stay quiet.
         */
        if (Wnd->DwmRedirBitmap == NULL &&
            (Wnd->rcWindow.right  - Wnd->rcWindow.left) <= 0 &&
            (Wnd->rcWindow.bottom - Wnd->rcWindow.top)  <= 0)
        {
            /*
             * Bounded, not silent. Making this fully quiet was an
             * over-correction: if sprites are created while their windows are
             * still 0x0 -- which the startup sweep suggests is common -- then
             * EVERY window fails here, nothing is logged, and the absence of
             * output reads as "the query is fine" when nothing is succeeding.
             *
             * Note also that nothing re-queries once the window is sized:
             * BuildRedirectionSurface runs at sprite creation only, so a
             * window that was 0x0 at that instant never gets a surface even
             * after IntDwmRedirOnWindowSized allocates its bitmap. If this
             * count is high, that is the next thing to fix, not the lookup.
             */
            static LONG s_cZero = 0;
            LONG n = InterlockedIncrement(&s_cZero);

            if (n <= 4 || (n % 25) == 0)
            {
                ERR("DwmGetSurfaceData(sprite=%lu): hwnd %p is 0x0, no surface "
                    "yet (deferred #%ld)\n",
                    (unsigned long)hSprite, Wnd->head.h, n);
            }

            goto Cleanup;
        }

        /* Spelled out rather than folded into the macro: the flags and the
         * bitmap handle are what separate "never marked" from "marked but
         * allocation failed for a window that has real area", which is a bug. */
        ERR("DwmGetSurfaceData(sprite=%lu): hwnd %p not redirected "
            "(flags=0x%lx bitmap=%p %dx%d structural=%d)\n",
            (unsigned long)hSprite, Wnd->head.h,
            (unsigned long)Wnd->DwmRedirFlags,
            (PVOID)Wnd->DwmRedirBitmap,
            Wnd->rcWindow.right  - Wnd->rcWindow.left,
            Wnd->rcWindow.bottom - Wnd->rcWindow.top,
            (int)gfStructuralRedirection);

        /*
         * NO BREAKPOINT HERE.
         *
         * This used to __debugbreak() once, to bound the log while the sprite
         * lookup was being diagnosed. It fires during boot on a window that is
         * destroyed between the notification and the query -- a benign race --
         * and an int 3 in win32k halts the whole guest in the debugger, so
         * every session stopped dead at this line and looked like a hang.
         *
         * The message stays; it is cheap and this path is rare. A diagnostic
         * that stops the machine costs more than the information it returns.
         */
        goto Cleanup;
    }

    if (Wnd->DwmRedirSectionObject == NULL)
        RWM_SURFDATA_BAIL("redirection bitmap has no section object");

    if (!GreGetBitmapPixelSize((HBITMAP)Wnd->DwmRedirBitmap, &sizl))
        RWM_SURFDATA_BAIL("GreGetBitmapPixelSize failed");

    }   /* s_fBroke scope */

    /*
     * A handle in the compositor's process. SECTION_MAP_READ only: dwmredir
     * maps it to sample the pixels and has no business writing into a surface
     * that GDI owns.
     */
    /*
     * THE OBJECT TYPE IS MANDATORY HERE, not optional.
     *
     * This passed NULL, on the usual reading that NULL means "accept any
     * type". That is only true for KernelMode. ReactOS's
     * ObReferenceObjectByPointer, which ObOpenObjectByPointer calls first,
     * reads:
     *
     *     if ((Header->Type != ObjectType) &&
     *         ((AccessMode != KernelMode) || (ObjectType == ObpSymbolicLinkObjectType)))
     *         return STATUS_OBJECT_TYPE_MISMATCH;
     *
     * so with ObjectType == NULL and AccessMode == UserMode the first clause is
     * trivially true and the second is too -- it fails EVERY time, for every
     * window, and reported 0xC0000024 with the section, the bitmap and the
     * flags all perfectly valid.
     *
     * UserMode stays: the handle is destined for dwm.exe's table and must be
     * charged and validated against it. Supplying the type is what makes that
     * combination legal.
     */
    Status = ObOpenObjectByPointer(Wnd->DwmRedirSectionObject,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   SECTION_MAP_READ | SECTION_QUERY,
                                   MmSectionObjectType,
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
    /*
     * THE GDI BITMAP FORMAT, not a DIB compression constant.
     *
     * This was BI_RGB, which is 0 -- and dwmredir's TranslateGdiFormat
     * (dwmredir.dll.c:5793) reads slot 3 as a BMF_* code, whose arms are
     * 4=16bpp, 5=24bpp, 6=32bpp. Zero falls through to MILPIXFMT_UNDEFINED,
     * and GetNewSurfaceData's unusable-format path then CLOSES the section and
     * zeroes nWidth/nHeight before CreateSurface ever sees them. The surface
     * arrived intact and was discarded one function later, which in the log
     * looked like "the query returned nothing" -- the give-away was dwStride
     * surviving, because that path does not clear it.
     *
     * BMF_32BPP is what IntDwmCreateRedirectionBitmapForSize passes to
     * GreCreateBitmapEx, so this is the format of the bitmap we actually made,
     * not an assumption about it.
     *
     * The aux word only selects 555-vs-565 on the 16bpp arm; at 32bpp
     * TranslateGdiFormat ignores it and picks PBGRA32/BGR32 from the alpha
     * flag below. It stays as the bit depth for the 16bpp case to be correct
     * if this ever creates one.
     */
    Data.dwGdiFormat    = BMF_32BPP;
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

/*
 * Vista walks the USER HANDLE TABLE for this (win32k.sys.c:49816: every entry
 * of type window whose rpdesk is grpdeskDwm), and so do we now.
 *
 * THIS USED TO WALK THE DESKTOP TREE, and that was wrong for this particular
 * question. The deviation was written down when the tree walk went in --
 * "a handle-table walk also finds windows that exist but are not linked into
 * the tree, which this cannot see" -- with the note that such windows arrive
 * later through the per-window CreateWindow hook so the end state is the same.
 * That holds for REGISTRATION, which can be late. It does not hold here.
 *
 * A sprite id is minted in IntDwmCreateSprite and the notification is sent
 * immediately; dwmredir turns straight round and asks for that sprite's
 * surface. If the window is not yet linked into spwndChild/spwndNext at that
 * instant the tree walk cannot see it, the query answers "no window carries
 * this sprite", and the window silently never gets a surface -- there is no
 * retry, because nothing asks again. The answer is needed NOW, not eventually.
 *
 * The table is the same size the tree walk covered plus the unlinked windows,
 * so this is not a cost increase in any meaningful sense, and it runs once per
 * surface creation rather than per frame.
 *
 * Callers hold the USER lock, which is what makes touching gHandleTable here
 * safe.
 */
static
PWND
FASTCALL
IntDwmFindWindowBySprite(UINT32 hSprite)
{
    int i;

    if (hSprite == 0 || gHandleTable == NULL)
        return NULL;

    for (i = 0; i < gHandleTable->nb_handles; i++)
    {
        PUSER_HANDLE_ENTRY Entry = &gHandleTable->handles[i];
        PWND Wnd;

        if (Entry->type != TYPE_WINDOW || Entry->ptr == NULL)
            continue;

        Wnd = (PWND)Entry->ptr;

        if (Wnd->DwmSprite != 0 && Wnd->DwmSprite == hSprite)
            return Wnd;
    }

    return NULL;
}

/* EOF */
