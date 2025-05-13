// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_media
//      $Keywords:
//
//  $Description:
//      This provides the implementation that handles a hardware media buffer.
//      (I.e. a buffer that is only decoded to by hardware video processing).
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#pragma once

MtExtern(CHWMFMediaBuffer);

class CHWMFMediaBuffer : public CMFMediaBuffer
{
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CHWMFMediaBuffer));

    CHWMFMediaBuffer(
        _In_    UINT             uiID,
        _In_    LONG             continuity,
        _In_    UINT             uiWidth,
        _In_    UINT             uiHeight,
        _In_    D3DFORMAT        format,
        _In_    CD3DDeviceLevel1 *pRenderDevice,
        _In_    CD3DDeviceLevel1 *pMixerDevice
        );

    /* override */
    ~CHWMFMediaBuffer(
        void
        );

    /* override */
    HRESULT
    GetBitmapSource(
        _In_            bool                syncChannel,
        __in_opt        CD3DDeviceLevel1    *pDisplayDevice,
        __deref_out     IWGXBitmapSource    **ppIBitmapSource
        );

    /* override */
    HRESULT
    DoneWithBitmap(
        void
        );

protected:

    /* override */
    HRESULT
    Init(
        void
        );

    virtual
    HRESULT
    CreateMixerTexture(
        void
        );

    HRESULT
    GetSurfaceDescription(
        _In_    D3DPOOL             d3dPool,
        _Out_   D3DSURFACE_DESC     *pD3DSurfaceDesc
        );

    CD3DDeviceLevel1        *m_pMixerDevice;
    IDirect3DTexture9       *m_pIMixerTexture;

private:

    //
    // Cannot copy or assign a Hardware media buffer
    //
    CHWMFMediaBuffer(
        _In_    const CHWMFMediaBuffer &
        );

    CHWMFMediaBuffer &
    operator=(
        _In_    const CHWMFMediaBuffer &
        );

    HRESULT
    CacheTextureOnBitmap(
        void
        );

    HRESULT
    CopyBitmap(
        _In_    bool                fetchData = true
        );

    //
    // Cross thread members- used by both the media thread and the composition
    // thread.
    //
    IDirect3DSurface9       *m_pIMixerSurface;

    //
    // Composition thread members- used only by the composition thread.
    //
    CClientMemoryBitmap     *m_pBitmap;
    IDirect3DSurface9       *m_pIBitmapSurface;
    bool                    m_fTextureCachedOnBitmap;
    bool                    m_surfaceLocked;
};

