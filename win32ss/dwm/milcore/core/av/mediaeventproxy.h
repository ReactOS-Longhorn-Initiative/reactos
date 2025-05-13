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
//      Provides support media specific support for proxying events up to
//      managed code.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#pragma once

MtExtern(EventItem);

class CMediaEventProxy
{
public:

    CMediaEventProxy(
        _In_        UINT                    uiID,
        __in_opt        CEventProxy             *pCEventProxy
        );

    virtual
    ~CMediaEventProxy(
        );

    HRESULT
    Init(
        void
        );

    HRESULT
    RaiseEvent(
        _In_ AVEvent    eventType,
        _In_ HRESULT    failureHr = S_OK
        );

    HRESULT
    RaiseEvent(
        _In_     AVEvent    eventType,
        __in_opt PCWSTR     type,
        __in_opt PCWSTR     param,
        _In_     HRESULT    failureHr = S_OK
        );

    void
    Shutdown(
        void
        );

private:

    class EventItem : public CStateThreadItem,
                             CMILCOMBase
    {
    public:
        DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(EventItem));

        // Declares IUnknown functions
        DECLARE_COM_BASE;

        static
        HRESULT
        Create(
            _In_        UINT                    id,
            _In_        CEventProxy             *pCEventProxy,
            _In_        AVEvent                 eventType,
            __in_opt    PCWSTR                  type,
            __in_opt    PCWSTR                  param,
            _In_        HRESULT                 failureHr,
            __deref_out EventItem               **ppEventItem
            );

    protected:
        //
        // CMILCOMBase
        //
        STDMETHOD(HrFindInterface)(
            __in_ecount(1) REFIID riid,
            __deref_out void **ppv
            );

        //
        // CStateThreadItem
        //
        /* override */
        void
        Run(
            void
            );

    private:
        EventItem(
            _In_        UINT                    uiID,
            _In_        CEventProxy             *pCEventProxy,
            _In_        AVEvent                 eventType,
            _In_        HRESULT                 failureHr
            );

        virtual
        ~EventItem(
            );

        HRESULT
        Init(
            __in_opt    PCWSTR                  type,
            __in_opt    PCWSTR                  param
            );

        //
        // Cannot copy or assign a CMediaEventProxy
        // 
        EventItem(
            _In_ const EventItem &
            );

        EventItem &
        operator=(
            _In_ const EventItem &
            );

        UINT                    m_uiID;
        CEventProxy             *m_pCEventProxy;
        AVEvent                 m_eventType;
        PWSTR                   m_type;
        PWSTR                   m_param;
        HRESULT                 m_failureHr;
    };

    enum
    {
        maximumEventPacketSize = 4096
    };

    //
    // Cannot copy or assign a CMediaEventProxy
    // 
    CMediaEventProxy(
        _In_ const CMediaEventProxy &
        );

    CMediaEventProxy &
    operator=(
        _In_ const CMediaEventProxy &
        );

    UINT                    m_uiID;
    CEventProxy             *m_pCEventProxy;
    CStateThread            *m_pEventThread;
    CCriticalSection        m_stateLock;

    typedef CGuard<CCriticalSection>    CriticalSectionGuard_t;
};

