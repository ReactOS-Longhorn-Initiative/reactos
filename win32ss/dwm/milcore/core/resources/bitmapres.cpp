// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    bitmapres.cpp

Abstract:

    Bitmap resource. This file contains the implementation for all the
    bitmap resource functionality.

Environment:

    User mode only.


--*/

#include "precomp.hpp"

MtDefine(CMilSlaveBitmap, MILRender, "CMilSlaveBitmap");

//
// The 52-byte header is wire format: Vista's dispatch hard-codes both the
// 0x34 size floor and the +52 payload offset. The two trailing doubles sit at
// 36 and 44, which only holds while wgx_core_types.h's #pragma pack(1) region
// covers the struct -- natural alignment silently grows it to 56 and every
// upload would then be rejected as malformed.
//
C_ASSERT(sizeof(MILCMD_BITMAP_PIXELS) == 0x34);
C_ASSERT(FIELD_OFFSET(MILCMD_BITMAP_PIXELS, DpiX) == 36);


/* override */ HRESULT
CMilSlaveBitmap::Draw(
    __in_ecount(1) CDrawingContext *pDC,
    MilBitmapWrapMode::Enum wrapMode
    )
{
    RRETURN(pDC->DrawBitmap(
        this,
        wrapMode
        ));
}

/*++

Routine Description:

Initialize a bitmap from IWGXBitmap.

Arguments:

MILCMD_BITMAP_SOURCE - Packed data structure that contains basic
information about the bitmap.
Return Value:

HRESULT

--*/

HRESULT CMilSlaveBitmap::ProcessSource(
    __in_ecount(1) CMilSlaveHandleTable*,
    __in_ecount(1) const MILCMD_BITMAP_SOURCE* pBmp
    )
{
    HRESULT hr = S_OK;
    IWGXBitmap *pCWICWrapperBitmap = static_cast<IWGXBitmap *>(static_cast<CWICWrapperBitmap *>(pBmp->pIBitmap));

    IFCNULL(pCWICWrapperBitmap);

    ReplaceInterface(m_pIBitmap, pCWICWrapperBitmap);

Cleanup:
    NotifyOnChanged(this);

    //
    // Do not increase the reference count for pCWICWrapperBitmap -- it has
    // already been increased by the transport
    //
    ReleaseInterface(pCWICWrapperBitmap);

    RRETURN(hr);
}

/*++

Routine Description:

    Initialize a bitmap from raw pixels carried inline in the command batch.

    1:1 of Vista milcore's CMilSlaveBitmap::ProcessPixels (milcore.dll.c:204244).
    This is the only bitmap-upload path that survives a channel crossing:
    MilCmdBitmapSource marshals an IWICBitmapSource *pointer*, so it is
    in-process only, and it is what Vista's uDWM uses for its own decoded
    art. uDWM's placeholder path (uDWM.dll.c:4554) sends this command instead,
    and ours sends it for every bitmap -- the theme atlas, the rasterized
    caption text and the window icons all arrive here.

    Payload layout, validated exactly as Vista validates it:

        [0                     .. Stride*Height)          pixels
        [Stride*Height         .. + 4*cPaletteEntries)    palette
        rounded up to a DWORD == cbPayload

Arguments:

    pCmd      - the 52-byte MILCMD_BITMAP_PIXELS header
    pvPayload - the bits, i.e. the batch command + 52
    cbPayload - command size - 52

Return Value:

    HRESULT

--*/

HRESULT CMilSlaveBitmap::ProcessPixels(
    __in_ecount(1) CMilSlaveHandleTable*,
    __in_ecount(1) const MILCMD_BITMAP_PIXELS* pCmd,
    __in_bcount(cbPayload) const void* pvPayload,
    UINT cbPayload
    )
{
    HRESULT hr = S_OK;

    CClientMemoryBitmap *pWrapper = NULL;
    CSystemMemoryBitmap *pCopy = NULL;
    IWICImagingFactory *pIWICFactory = NULL;
    IWICPalette *pIPalette = NULL;

    UINT cbPixels = 0;
    UINT cbPalette = 0;
    UINT cbExpected = 0;
    UINT cbAligned = 0;

    //
    // Vista calls Dispose() before it looks at anything: the resource drops
    // its old content whether or not new content arrives.
    //
    ReleaseInterface(m_pIBitmap);

    IFCNULL(pCmd);
    IFCNULL(pvPayload);

    if (!IsValidPixelFormat(pCmd->PixelFormat))
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    //
    // The payload must account for exactly the pixels plus the palette, with
    // no slack beyond DWORD alignment. Each step is overflow-checked because
    // every one of Width/Height/Stride/cPaletteEntries is attacker-controlled
    // wire data.
    //
    IFC(UIntMult(pCmd->Stride, pCmd->Height, &cbPixels));
    IFC(UIntMult(sizeof(WICColor), pCmd->cPaletteEntries, &cbPalette));
    IFC(UIntAdd(cbPixels, cbPalette, &cbExpected));
    IFC(UIntAdd(cbExpected, 3, &cbAligned));

    cbAligned &= ~3u;

    if (cbAligned != cbPayload)
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    //
    // Wrap the batch memory without copying so the widths, stride and format
    // it declares get validated (HrInit runs HrCheckBufferSize), then hand it
    // to a CSystemMemoryBitmap, which owns its bits.
    //
    // DEVIATION, forced: Vista builds the source with
    // MILCreateBitmapFromMemory and then re-wraps it with
    // HrCreateBitmapFromSource; neither helper exists in this milcore, whose
    // equivalent pairing is CClientMemoryBitmap + CSystemMemoryBitmap::Init.
    // The result is the same object graph and the same single copy. The copy
    // is not optional either way: the command batch buffer is recycled as
    // soon as ProcessCommandBatch returns.
    //
    pWrapper = new CClientMemoryBitmap;
    IFCOOM(pWrapper);
    pWrapper->AddRef();

    IFC(pWrapper->HrInit(
        pCmd->Width,
        pCmd->Height,
        pCmd->PixelFormat,
        cbPixels,
        const_cast<void*>(pvPayload),
        pCmd->Stride
        ));

    //
    // Vista sets the resolution on the source before copying. uDWM zeroes
    // these fields, and 0 is CWGXBitmap's "resolution not set" sentinel, so
    // passing them through verbatim is the correct behaviour rather than a
    // gap -- do not substitute 96.
    //
    IFC(pWrapper->SetResolution(pCmd->DpiX, pCmd->DpiY));

    if (pCmd->cPaletteEntries > 0)
    {
        IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
        IFC(pIWICFactory->CreatePalette(&pIPalette));

        IFC(pIPalette->InitializeCustom(
            const_cast<WICColor*>(reinterpret_cast<const WICColor*>(
                static_cast<const BYTE*>(pvPayload) + cbPixels)),
            pCmd->cPaletteEntries
            ));

        IFC(pWrapper->SetPalette(pIPalette));
    }

    pCopy = new CSystemMemoryBitmap();
    IFCOOM(pCopy);
    pCopy->AddRef();

    // Carries over size, format, resolution and (for indexed formats) palette.
    IFC(pCopy->Init(static_cast<IWGXBitmapSource*>(pWrapper)));

    m_pIBitmap = static_cast<IWGXBitmap*>(pCopy);   // take over the reference
    pCopy = NULL;

Cleanup:
    ReleaseInterface(pIPalette);
    ReleaseInterface(pIWICFactory);
    ReleaseInterface(pWrapper);
    ReleaseInterface(pCopy);

    NotifyOnChanged(this);

    RRETURN(hr);
}

HRESULT CMilSlaveBitmap::ProcessInvalidate(
    __in_ecount(1) CMilSlaveHandleTable*,
    __in_ecount(1) const MILCMD_BITMAP_INVALIDATE* pData
    )
{
    HRESULT hr = S_OK;
    
    if (m_pIBitmap)
    {
        const RECT * pDirtyRect = NULL;

        // Use the dirty rect specified in the payload only if told to.
        if (pData->UseDirtyRect)
        {
            pDirtyRect = &pData->DirtyRect;
        }
        
        IFC(m_pIBitmap->AddDirtyRect(pDirtyRect));
    }

Cleanup:
    NotifyOnChanged(this);
    
    RRETURN(hr);
}

HRESULT CMilSlaveBitmap::GetBounds(
    __in_ecount_opt(1) CContentBounder *pBounder,
    __out_ecount(1) CMilRectF *prcBounds
    )
{
    HRESULT hr = S_OK;
    if (m_pIBitmap)
    {
        Assert(prcBounds);

        IFC(GetBitmapSourceBounds(m_pIBitmap, prcBounds));
    }
    else
    {
        IFC(WGXERR_NOTINITIALIZED);
    }

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

Constructor - initialize the bitmap resource to an empty bitmap for the
given device object.

Return Value:

NONE

--*/

CMilSlaveBitmap::CMilSlaveBitmap(__in_ecount(1) CComposition*)
{
    m_pIBitmap = NULL;
}

/*++

Routine Description:

  Destructor

--*/

CMilSlaveBitmap::~CMilSlaveBitmap()
{
    ReleaseInterface(m_pIBitmap);
}



