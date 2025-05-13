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
//      This provides the implementation that handles a buffer that we use for
//      decoding in software.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#pragma once

MtExtern(CSWMFMediaBuffer);

class CSWMFMediaBuffer : public CMFMediaBuffer
{
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CSWMFMediaBuffer));

    CSWMFMediaBuffer(
        _In_    UINT             uiID,
        _In_    LONG             continuity,
        _In_    UINT             uiWidth,
        _In_    UINT             uiHeight,
        _In_    D3DFORMAT        format,
        _In_    CD3DDeviceLevel1 *pRenderDevice
        );

    /* override */
    ~CSWMFMediaBuffer(
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

private:

    //
    // Cannot copy or assign a Hardware media buffer
    //
    CSWMFMediaBuffer(
        _In_    const CSWMFMediaBuffer &
        );

    CSWMFMediaBuffer &
    operator=(
        _In_    const CSWMFMediaBuffer &
        );

    HRESULT
    CreateCompositionObjects(
        void
        );

    HRESULT
    AliasBitmap(
        _In_    CClientMemoryBitmap         *pBitmap,
        _In_    bool                        initializing
        );

    IDirect3DSurface9       *m_pIBitmapSurface;
    CClientMemoryBitmap     *m_pBitmap;
};

