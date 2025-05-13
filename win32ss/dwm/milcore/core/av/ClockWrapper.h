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

class EvrPresenter;

interface RenderClock
{
    virtual
    HRESULT
    GetRenderTime(
        _Out_       LONGLONG    *pCurrentTime,
        __out_opt   DWORD       *pContinuityKey = NULL
        ) = 0;
};

class TimerWrapper : public RenderClock
{
public:
    typedef
    HRESULT
    (EvrPresenter::*InvokeMethod)(
        _In_    IMFAsyncResult      *pIResult
        );

    TimerWrapper(
        void
        );

    /* override */
    ~TimerWrapper(
        void
        );

    HRESULT
    Init(
        _In_    UINT                uiID,
        _In_    EvrPresenter        *pEvrPresenter,
        _In_    InvokeMethod        method
        );

    /* override */
    void
    Shutdown(
        void
        );

    void
    SetUnderlyingClock(
        _In_    IMFClock    *pIMFClock
        );

    IMFClock*
    GetUnderlyingClockNoAddRef(
        void
        );

    void
    SetUnderlyingTimer(
        _In_    IMFTimer    *pIMFTimer
        );

    IMFTimer*
    GetUnderlyingTimerNoAddRef(
        void
        );

    void
    ClockStarted(
        void
        );

    void
    ClockPaused(
        void
        );

    void
    ClockStopped(
        void
        );

    HRESULT
    GetMixTime(
        _Out_       LONGLONG    *pCurrentTime,
        __out_opt   DWORD       *pContinuityKey = NULL
        );

    HRESULT
    GetRenderTime(
        _Out_       LONGLONG    *pCurrentTime,
        __out_opt   DWORD       *pContinuityKey = NULL
        );

    HRESULT SetTimer(
        _In_        DWORD               continuityKey,
        _In_        LONGLONG            clockTime
        );

private:
    class PresenterInvoker : public IMFAsyncCallback,
                             public CStateThreadItem
    {
    public:

        PresenterInvoker(
            void
            );

        ~PresenterInvoker(
            void
            );

        HRESULT
        Init(
            _In_    UINT                uiID,
            _In_    EvrPresenter        *pPresenter,
            _In_    TimerWrapper        *pTimerWrapper,
            _In_    InvokeMethod        method
            );

        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(
            _In_        REFIID      riid,
            __deref_out void        **ppvObject
            );

        STDMETHOD_(ULONG, AddRef)(
            void
            );

        STDMETHOD_(ULONG, Release)(
            void
            );

        //
        // IMFAsyncCallback
        //
        STDMETHOD(GetParameters)(
            __out_ecount(1)       DWORD* pdwFlags,
            __out_ecount(1)       DWORD* pdwQueue
            );

        STDMETHOD(Invoke)(
            _In_        IMFAsyncResult* pResult
            );

    protected:

        /* override */
        void
        Run(
            void
            );

        /* override */
        bool
        IsAnOwner(
            _In_    IUnknown    *pIUnknown
            );

    private:

        //
        // Cannot copy or assign a PresenterInvoker
        //
        PresenterInvoker(
            _In_    const PresenterInvoker &
            );

        PresenterInvoker &
        operator=(
            _In_    const PresenterInvoker &
            );

        UINT                m_uiID;
        EvrPresenter        *m_pEvrPresenter;
        TimerWrapper        *m_pTimerWrapper;
        InvokeMethod        m_method;
    };

    //
    // Cannot copy or assign a TimerWrapper
    //
    TimerWrapper(
        _In_ const TimerWrapper        &
        );

    TimerWrapper &
    operator=(
        _In_ const TimerWrapper &
        );

    HRESULT
    GetTime(
        _In_        LONGLONG    defaultTime,
        _Out_       LONGLONG    *pCurrentTime,
        __out_opt   DWORD       *pContinuityKey = NULL
       );

    void
    CallbackOccurred(
        void
        );

    void
    CancelAndReleaseTimer(
        void
        );

    HRESULT
    DoCallbackThroughStateThread(
        void
        );

    CCriticalSection    m_lock;
    IMFClock            *m_pIMFClock;
    bool                m_isStarted;
    UINT                m_uiID;

    CStateThread        *m_pCStateThread;
    IMFTimer            *m_pIMFTimer;
    IUnknown            *m_pITimerKey;
    EvrPresenter        *m_pEvrPresenter;
    PresenterInvoker    m_presenterInvoker;
    LONGLONG            m_setTimerTime;
    bool                m_timerBeingSet;

    static const LONGLONG msc_timerAccuracy = 10000;
};


