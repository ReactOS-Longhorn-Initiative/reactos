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
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#pragma once

MtExtern(MediaInstance);

class MediaInstance : public CMILCOMBase
{
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(MediaInstance));

    //
    // CMILCOMBase
    //
    DECLARE_COM_BASE;

    static
    HRESULT
    Create(
        _In_            CEventProxy         *pCEventProxy,
        __deref_out     MediaInstance       **ppMediaInstance
        );

    HRESULT
    Init(
        void
        );

    UINT
    GetID(
        void
        ) const;

    inline
    CMediaEventProxy&
    GetMediaEventProxy(
        void
        );

    inline
    CompositionNotifier&
    GetCompositionNotifier(
        void
        );

protected:

    //
    // CMILCOMBase
    //
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

private:

    MediaInstance(
        _In_        UINT        uiID,
        _In_        CEventProxy *pCEventProxy
        );

    ~MediaInstance(
        void
        );

    //
    // Cannot copy or assign a MediaInstance
    //
    MediaInstance(
        _In_    const MediaInstance &
        );

    MediaInstance &
    operator=(
        _In_    const MediaInstance &
        );

    UINT                    m_uiID;
    CompositionNotifier     m_compositionNotifier;
    CMediaEventProxy        m_mediaEventProxy;

    static LONG             ms_id;
};

#include "MediaInstance.inl"

