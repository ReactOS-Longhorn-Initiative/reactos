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

MtExtern(CEvrFilterWrapper);

class CEvrFilterWrapper : IMediaSeeking
{
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CEvrFilterWrapper));

    static
    HRESULT
    Create(
        _In_        UINT                id,
        __deref_out CEvrFilterWrapper   **ppCEvrFilterWrapper
        );

    void
    SwitchToInnerIMediaSeeking(
        void
        );

    //
    // IUnknown
    //
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    STDMETHOD(QueryInterface)(_In_ REFIID riid, __deref_out void **ppvObject);

    //
    // IMediaSeeking
    //
    STDMETHOD(GetCapabilities)(
        _Out_       DWORD       *pCapabilities
        );

    STDMETHOD(CheckCapabilities)(
        _In_        DWORD       *pCapabilities
        );

    STDMETHOD(SetTimeFormat)(
        _In_        const GUID  *pFormat
        );

    STDMETHOD(GetTimeFormat)(
        _Out_       GUID        *pFormat
        );

    STDMETHOD(IsUsingTimeFormat)(
        _In_        const GUID  *pFormat
        );

    STDMETHOD(IsFormatSupported)(
        _In_        const GUID  *pFormat
        );

    STDMETHOD(QueryPreferredFormat)(
        _Out_       GUID        *pFormat
        );

    STDMETHOD(ConvertTimeFormat)(
        _Out_       LONGLONG    *pTarget,
        __in_opt    const GUID  *pTargetFormat,
        _In_        LONGLONG    Source,
        __in_opt    const GUID  *pSourceFormat
        );

    STDMETHOD(SetPositions)(
        __in_opt    LONGLONG    *pCurrent,
        _In_        DWORD       CurrentFlags,
        __in_opt    LONGLONG    *pStop,
        _In_        DWORD       StopFlags
        );

    STDMETHOD(GetPositions)(
        __out_opt   LONGLONG    *pCurrent,
        __out_opt   LONGLONG    *pStop
        );

    STDMETHOD(GetCurrentPosition)(
        _Out_       LONGLONG    *pCurrent
        );

    STDMETHOD(GetStopPosition)(
        _Out_       LONGLONG    *pStop
        );

    STDMETHOD(SetRate)(
        _In_        double      dRate
        );

    STDMETHOD(GetRate)(
        _Out_       double      *pdRate
        );

    STDMETHOD(GetDuration)(
        _Out_       LONGLONG    *pDuration
        );

    STDMETHOD(GetAvailable)(
        __out_opt   LONGLONG    *pEarliest,
        __out_opt   LONGLONG    *pLatest
        );

    STDMETHOD(GetPreroll)(
        _Out_       LONGLONG    *pllPreroll
        );

private:

    CEvrFilterWrapper(
        _In_        UINT                    uiID
        );

    virtual
    ~CEvrFilterWrapper(
        );

    HRESULT
    Init(
        );

    CCriticalSection        m_stateLock;
    UINT                    m_uiID;
    LONG                    m_cRef;
    IUnknown                *m_pINonDelegatingUnknown;
    IMediaSeeking           *m_pIMediaSeeking;
    bool                    m_useInnerIMediaSeeking;

    typedef CGuard<CCriticalSection>    CriticalSectionGuard_t;
};

