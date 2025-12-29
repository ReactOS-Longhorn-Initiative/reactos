// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DSwapChainWithSwDC implementation
//


//------------------------------------------------------------------------------
//
//  Class: CD3DSwapChainWithSwDC
//
//  Description:
//      This class /* override */s the GetDC method of CD3DSwapChain to implement
//      GetDC using GetRenderTargetData. This approach acheived phenominal perf
//      wins in WDDM since there is no GDI hardware acceleration in WDDM.
//
//------------------------------------------------------------------------------

class CD3DSwapChainWithSwDC : public CD3DSwapChain
{
    friend HRESULT CD3DSwapChain::Create(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __inout_ecount(1) IDirect3DSwapChain9 *pID3DSwapChain9,
        UINT BackBufferCount,
        __in_ecount_opt(1) CMILDeviceContext const *pPresentContext,
        __deref_out_ecount(1) CD3DSwapChain **ppSwapChain
        );

public:
    HRESULT GetDC(
        /*__in_range(<, this->m_cBackBuffers)*/ UINT iBackBuffer,
        __in_ecount(1) const CMilRectU& rcDirty,
        __deref_out HDC *phdcBackBuffer
        ) const;

    HRESULT ReleaseDC(
        /*__in_range(<, this->m_cBackBuffers)*/ UINT iBackBuffer,
        _In_ HDC hdcBackBuffer
        ) const;

private:
    CD3DSwapChainWithSwDC(
        _In_ HDC hdcPresentVia,
        __in_range(>, 0) /*__out_range(==, this->m_cBackBuffers)*/ UINT cBackBuffers,
        __inout_ecount(1) IDirect3DSwapChain9 *pID3DSwapChain
        );

protected:
    virtual ~CD3DSwapChainWithSwDC();

protected:

    HRESULT Init(
        __inout_ecount(1) CD3DResourceManager *pResourceManager
        ) /* override */;

private:
    HDC m_hdcCopiedBackBuffer;
    HBITMAP m_hbmpCopiedBackBuffer;
    __field_bcount(m_cbBuffer) void *m_pBuffer;
    UINT m_cbBuffer;
    UINT m_stride;
};



