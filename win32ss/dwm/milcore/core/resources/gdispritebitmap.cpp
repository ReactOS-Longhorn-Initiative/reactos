// [RWM] CMilGdiSpriteBitmap -- the compositor's view of a window's redirection
// bitmap. Declared in VistaDwmResources.h; wire protocol and the recovered
// field map are in NOTES-chrome.md, "GdiSpriteBitmap: the surface handoff".
//
// This is the receiving half of the content handoff: win32k allocates a
// section-backed bitmap per redirected window, dwmredir sends the section
// across as cmd 17, and mapping it here is what lets the compositor read live
// application output. Everything else in the chrome pipeline draws art we
// generate ourselves.

#include "precomp.hpp"
#include <debug.h>   // [RWM] DPRINT1; not pulled in by this precomp

MtDefine(CMilGdiSpriteBitmap, MILRender, "GdiSpriteBitmap Resource");

//
// Vista's HandleSectionChange accepts exactly two pixel formats, as the
// literals 14 and 16. Assert the mapping rather than transcribing the numbers:
// if these ever stop matching, a wrong-format section would be mapped and
// misinterpreted silently, and a build break is much cheaper than that.
//
C_ASSERT(MilPixelFormat::BGR32bpp   == 0x0E);
C_ASSERT(MilPixelFormat::PBGRA32bpp == 0x10);

C_ASSERT(sizeof(MILCMD_GDISPRITEBITMAP)               == 0x10);
C_ASSERT(sizeof(MILCMD_GDISPRITEBITMAP_UPDATEMARGINS) == 0x18);
C_ASSERT(sizeof(MILCMD_BITMAP_SECTION)                == 0x38);
C_ASSERT(FIELD_OFFSET(MILCMD_BITMAP_SECTION, hSection) == 32);


CMilGdiSpriteBitmap::CMilGdiSpriteBitmap(__in_ecount(1) CComposition *pComposition)
    : CMilSlaveBitmap(pComposition)
{
    m_pCompositionNoRef  = pComposition;

    m_nVisibleWidth      = 0;
    m_nVisibleHeight     = 0;
    m_nStride            = 0;
    m_cbFirstPixelOffset = 0;
    m_fmt                = MilPixelFormat::BGR32bpp;

    m_hSection           = NULL;
    m_pvMappedBase       = NULL;

    m_nFullWidth         = 0;
    m_nFullHeight        = 0;

    m_cxLeft             = 0;
    m_cxRight            = 0;
    m_cyTop              = 0;
    m_cyBottom           = 0;

    m_hSprite            = 0;
    m_dwReserved0        = 0;
}

CMilGdiSpriteBitmap::~CMilGdiSpriteBitmap()
{
    UnmapAndDispose();
}

//
// Drop the bitmap, then the mapping, then the section.
//
// The order is load-bearing. RecreateBitmap WRAPS the mapped memory without
// copying it, so the IWGXBitmap holds a raw pointer into the view. Unmapping
// before releasing the bitmap would leave that pointer dangling and the fault
// would land at the next draw, somewhere in the render walk, rather than here.
//
void
CMilGdiSpriteBitmap::UnmapAndDispose()
{
    ReleaseInterface(m_pIBitmap);

    if (m_pvMappedBase != NULL)
    {
        UnmapViewOfFile(m_pvMappedBase);
        m_pvMappedBase = NULL;
    }

    if (m_hSection != NULL)
    {
        CloseHandle(m_hSection);
        m_hSection = NULL;
    }
}

//
// cmd 90. Vista's ProcessUpdate (milcore.dll.c:91403) is two stores and a
// return -- it deliberately does not touch the bitmap, because the surface
// arrives separately as cmd 17 and may arrive before or after this.
//
HRESULT
CMilGdiSpriteBitmap::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable *,
    __in_ecount(1) const MILCMD_GDISPRITEBITMAP *pCmd
    )
{
    m_hSprite     = pCmd->hSprite;
    m_dwReserved0 = pCmd->Reserved0;

    return S_OK;
}

//
// cmd 91. Reject negative margins, then rebuild ONLY on an actual change.
//
// The change test is not an optimisation to skip. UpdateMargins is sent on
// every create and again on every move, so rebuilding unconditionally would
// tear down and re-wrap the mapping on every frame of a window drag.
//
HRESULT
CMilGdiSpriteBitmap::ProcessUpdateMargins(
    __in_ecount(1) CMilSlaveHandleTable *,
    __in_ecount(1) const MILCMD_GDISPRITEBITMAP_UPDATEMARGINS *pCmd
    )
{
    HRESULT hr = S_OK;

    if (pCmd->cxLeftWidth < 0 || pCmd->cxRightWidth   < 0 ||
        pCmd->cyTopHeight < 0 || pCmd->cyBottomHeight < 0)
    {
        IFC(E_INVALIDARG);
    }

    if (pCmd->cxLeftWidth    != m_cxLeft ||
        pCmd->cxRightWidth   != m_cxRight ||
        pCmd->cyTopHeight    != m_cyTop ||
        pCmd->cyBottomHeight != m_cyBottom)
    {
        m_cxLeft   = pCmd->cxLeftWidth;
        m_cxRight  = pCmd->cxRightWidth;
        m_cyTop    = pCmd->cyTopHeight;
        m_cyBottom = pCmd->cyBottomHeight;

        IFC(RecreateBitmap());
    }

Cleanup:
    RRETURN(hr);
}

//
// cmd 17 (MilCmdBitmapSection) -- the local surface handoff.
//
// The section handle rides the wire as a 64-bit value at +32 so the payload
// has one layout on both architectures; milcore reads it back through
// UnwrapHandleFromUInt64. The Terminal Services path sends cmd 92 instead and
// carries no section at all, which is why a local desktop must never be
// classified as cross-machine.
//
HRESULT
CMilGdiSpriteBitmap::ProcessSection(
    __in_ecount(1) CMilSlaveHandleTable *,
    __in_ecount(1) const MILCMD_BITMAP_SECTION *pCmd
    )
{
    HRESULT hr = S_OK;
    HANDLE hSection;

    m_nFullWidth         = pCmd->Width;
    m_nFullHeight        = pCmd->Height;
    m_nStride            = pCmd->Stride;
    m_cbFirstPixelOffset = pCmd->Offset;

    hSection = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(pCmd->hSection));

    IFC(HandleSectionChange(hSection, pCmd->PixelFormat));

Cleanup:
    RRETURN(hr);
}

//
// cmd 93.
//
HRESULT
CMilGdiSpriteBitmap::ProcessUnmapSection(
    __in_ecount(1) CMilSlaveHandleTable *,
    __in_ecount(1) const MILCMD_GDISPRITEBITMAP_UNMAPSECTION *
    )
{
    UnmapAndDispose();

    NotifyOnChanged(this);

    return S_OK;
}

//
// Take ownership of a section and map it.
//
// THE HANDLE IS CONSUMED on every path: kept and closed later on success,
// closed here on failure. Vista does the same (it zeroes its local after the
// store and closes at the shared exit). Getting this wrong leaks a section
// per window resize, which on a dragged window is a leak per frame.
//
// DEVIATION, stated: Vista calls NtMapViewOfSection directly with
// PAGE_READONLY. MapViewOfFile is the documented Win32 wrapper over exactly
// that and is what this milcore already links against; the mapping produced is
// the same. FILE_MAP_READ is deliberate -- the compositor only ever reads
// window content, and a read-only view means a bug here cannot corrupt an
// application's own backing store.
//
HRESULT
CMilGdiSpriteBitmap::HandleSectionChange(
    HANDLE hSection,
    MilPixelFormat::Enum fmt
    )
{
    HRESULT hr = S_OK;

    if (fmt != MilPixelFormat::BGR32bpp && fmt != MilPixelFormat::PBGRA32bpp)
    {
        //
        // A redirection bitmap is always one of these two because win32k
        // creates it. A third value means the descriptor was mangled between
        // win32k and the wire, so failing is better than mapping a section we
        // would then misread.
        //
        DPRINT1("[RWM] GdiSpriteBitmap: refusing pixel format %d\n", (int)fmt);
        IFC(E_INVALIDARG);
    }

    /* Whatever was being shown is gone regardless of what happens next. */
    UnmapAndDispose();

    if (hSection == NULL)
    {
        /* Not an error -- an unmap arrives as a NULL section. */
        goto Cleanup;
    }

    m_pvMappedBase = MapViewOfFile(hSection, FILE_MAP_READ, 0, 0, 0);

    if (m_pvMappedBase == NULL)
    {
        DPRINT1("[RWM] GdiSpriteBitmap: MapViewOfFile failed, gle=%lu\n",
                GetLastError());
        IFC(HRESULT_FROM_WIN32(GetLastError()));
    }

    m_hSection = hSection;
    hSection   = NULL;              /* consumed */
    m_fmt      = fmt;

    IFC(RecreateBitmap());

Cleanup:
    if (hSection != NULL)
    {
        CloseHandle(hSection);
    }

    RRETURN(hr);
}

//
// Rebuild the IWGXBitmap over the mapped section.
//
// WRAPS, DOES NOT COPY -- and this is the one place the sibling
// CMilSlaveBitmap::ProcessPixels pattern must NOT be followed. ProcessPixels
// copies into a CSystemMemoryBitmap because its source is the command batch,
// which is recycled the instant ProcessCommandBatch returns. Here the source
// is a live section that the owning application keeps painting into. A copy
// would capture one frame and then never change again: every window would
// render once and freeze, which looks like a compositing bug and is not one.
// Vista wraps too (MILCreateBitmapFromMemory straight onto the mapped pointer).
//
// The visible rect is the full surface minus the margins, and the first
// visible pixel sits at top*stride + left*4 from the base -- so the
// non-client inset is applied by moving the origin, not by clipping later.
//
HRESULT
CMilGdiSpriteBitmap::RecreateBitmap()
{
    HRESULT hr = S_OK;
    CClientMemoryBitmap *pWrapper = NULL;
    UINT cbVisible = 0;
    UINT cbOffset = 0;
    UINT cbLeft = 0;
    UINT cxInset = 0;
    UINT cyInset = 0;

    ReleaseInterface(m_pIBitmap);

    if (m_hSection == NULL || m_pvMappedBase == NULL)
    {
        /* Nothing mapped. Not an error: the resource simply has no content
         * until a section arrives, and HasContent() already reports that. */
        goto Cleanup;
    }

    /* offset = top*stride + left*4, exactly Vista's `v3 * a1[6] + 4 * v4`.
     * Overflow-checked because every input is wire data. */
    IFC(UIntMult(static_cast<UINT>(m_cyTop), m_nStride, &cbOffset));
    IFC(UIntMult(static_cast<UINT>(m_cxLeft), 4, &cbLeft));
    IFC(UIntAdd(cbOffset, cbLeft, &cbOffset));
    IFC(UIntAdd(cbOffset, m_cbFirstPixelOffset, &cbOffset));

    /* Clamp before subtracting -- Vista clamps the margin sums against the
     * full extent, so an over-wide inset yields an empty surface rather than
     * an enormous one via underflow. */
    IFC(UIntAdd(static_cast<UINT>(m_cxLeft), static_cast<UINT>(m_cxRight), &cxInset));
    IFC(UIntAdd(static_cast<UINT>(m_cyTop), static_cast<UINT>(m_cyBottom), &cyInset));

    m_nVisibleWidth  = (cxInset >= m_nFullWidth)  ? 0 : (m_nFullWidth  - cxInset);
    m_nVisibleHeight = (cyInset >= m_nFullHeight) ? 0 : (m_nFullHeight - cyInset);

    if (m_nVisibleWidth == 0 || m_nVisibleHeight == 0)
    {
        goto Cleanup;
    }

    IFC(UIntMult(m_nStride, m_nVisibleHeight, &cbVisible));

    pWrapper = new CClientMemoryBitmap;
    IFCOOM(pWrapper);
    pWrapper->AddRef();

    IFC(pWrapper->HrInit(
        m_nVisibleWidth,
        m_nVisibleHeight,
        m_fmt,
        cbVisible,
        static_cast<BYTE*>(m_pvMappedBase) + cbOffset,
        m_nStride
        ));

    m_pIBitmap = static_cast<IWGXBitmap*>(pWrapper);    /* takes the reference */
    pWrapper = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pWrapper);

    /* Notify on every path, including the empty ones: a window that just lost
     * its surface still has to re-render without it. */
    NotifyOnChanged(this);

    RRETURN(hr);
}
