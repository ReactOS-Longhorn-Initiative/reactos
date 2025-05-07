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

MtExtern(EvrPresenter);

class CMFMediaBuffer;
class CDXVAManagerWrapper;
class CWmpStateEngine;

//
// RenderState
//
namespace RenderState
{
    enum Enum
    {
        Started,
        Stopped,
        Paused,
        RatePaused,
        Shutdown
    };
};

class EvrPresenter;

typedef RealComObject<EvrPresenter, NoDllRefCount> EvrPresenterObj;

//
// EvrPresenter
//
class EvrPresenter :
    public IMFVideoPresenter,
    public IMFVideoDeviceID,
    //public IMFClockRateSink,
    public IMFRateSupport,
    public IMFGetService,
    public IMFTopologyServiceLookupClient,
    public IMFVideoDisplayControl
{
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(EvrPresenter));

    static
    HRESULT
    Create(
        _In_    MediaInstance           *pMediaInstance,
        _In_    UINT                    resetToken,
        _In_    CWmpStateEngine         *pWmpStateEngine,
        _In_    CDXVAManagerWrapper     *pDXVAManagerWrapper,
        __deref_out EvrPresenterObj     **ppEvrPresenter
        );

    HRESULT
    Init(
        void
        );

    //
    // IMFVideoPresenter
    //
    STDMETHOD(ProcessMessage)(
        MFVP_MESSAGE_TYPE eMessage,
        ULONG_PTR ulParam
        );

    STDMETHOD(GetCurrentMediaType)(
        __deref_out     IMFVideoMediaType   **ppIMediaType
        );

    //
    // IMFVideoDeviceID
    //
    STDMETHOD(GetDeviceID)(
        __out_ecount(1)           IID                 *pDeviceID
        );

    //
    // IMFTopologyServiceLookupClient
    //
    STDMETHOD(InitServicePointers)(
        _In_        IMFTopologyServiceLookup        *pILookup
        );

    STDMETHOD(ReleaseServicePointers)(
        void
        );

    //
    // IMFClockStateSink methods
    //
    STDMETHOD(OnClockStart)(
        MFTIME SystemTime,
        MFTIME StartOffset
        );

    STDMETHOD(OnClockStop)(
        MFTIME SystemTime
        );

    STDMETHOD(OnClockPause)(
        MFTIME SystemTime
        );

    STDMETHOD(OnClockRestart)(
        MFTIME SystemTime
        );

    STDMETHOD(OnClockSetRate)(
        MFTIME SystemTime,
        float flRate
        );

    //
    // IMFRateSupport
    //
    STDMETHOD(GetSlowestRate)(
        MFRATE_DIRECTION    direction,
        BOOL                fAllowThinning,
        _Out_               float *pflRate
        );

    STDMETHOD(GetFastestRate)(
        MFRATE_DIRECTION    direction,
        BOOL                fAllowThinning,
        _Out_               float *pflRate
        );

    STDMETHOD(IsRateSupported)(
        BOOL    fAllowThinning,
        float   flRate,
        __out_opt float   *pflNearestRate
        );

    //
    // IMFGetService
    //
    STDMETHOD(GetService)(
        _In_        REFGUID guidService,
        _In_        REFIID  riid,
        __deref_out LPVOID  *ppvObject
        );

    //
    // IMFVideoDisplayControl
    //
    STDMETHOD(GetNativeVideoSize)(
        __inout_opt SIZE    *pszVideo,
        __inout_opt SIZE    *pszARVideo
        );

    STDMETHOD(GetIdealVideoSize)(
        __inout_opt SIZE    *pszMin,
        __inout_opt SIZE    *pszMax
        ) NOTIMPL_METHOD;

    STDMETHOD(SetVideoPosition)(
        __inout_opt const MFVideoNormalizedRect *pnrcSource,
        __inout_opt const LPRECT                prcDest
        );

    STDMETHOD(GetVideoPosition)(
        __inout_opt MFVideoNormalizedRect *pnrcSource,
        _Out_       LPRECT               prcDest
        );

    STDMETHOD(SetAspectRatioMode)(
        _In_        DWORD                dwAspectRatioMode
        );

    STDMETHOD(GetAspectRatioMode)(
        _Out_       DWORD                *pdwAspectRatioMode
        );

    STDMETHOD(SetVideoWindow)(
        _In_        HWND                 hwndVideo
        );

    STDMETHOD(GetVideoWindow)(
        __deref_out HWND                 *phwndVideo
        );

    STDMETHOD(RepaintVideo)(
        ) NOTIMPL_METHOD;

    STDMETHOD(GetCurrentImage)(
        __inout     BITMAPINFOHEADER    *pBih,
        __deref_out BYTE                **pDib,
        _Out_       DWORD               *pcbDib,
        __inout_opt LONGLONG            *pTimeStamp
        ) NOTIMPL_METHOD;

    STDMETHOD(SetBorderColor)(
        _In_        COLORREF            Clr
        ) NOTIMPL_METHOD;

    STDMETHOD(GetBorderColor)(
        _Out_       COLORREF            *pClr
        ) NOTIMPL_METHOD;

    STDMETHOD(SetRenderingPrefs)(
        _In_        DWORD               dwRenderFlags
        );

    STDMETHOD(GetRenderingPrefs)(
        _Out_       DWORD               *pdwRenderFlags
        );

    STDMETHOD(SetFullscreen)(
        _In_        BOOL                fFullscreen
        );

    STDMETHOD(GetFullscreen)(
        _Out_       BOOL                *pfFullscreen
        );

    //
    // Normal public methods
    //
    HRESULT
    GetSurfaceRenderer(
        __deref_out IAVSurfaceRenderer      **ppIAVSurfaceRenderer
        );

    DWORD
    DisplayWidth(
        void
        );

    DWORD
    DisplayHeight(
        void
        );

    void
    AvalonShutdown(
        void
        );

    HRESULT
    SignalMixer(
        _In_    DWORD                   continuityKey,
        _In_    LONGLONG                timeToSignal
        );

    HRESULT
    CancelTimer(
        void
        );

    HRESULT
    NewMixerDevice(
        _In_    CD3DDeviceLevel1        *pRenderDevice,
        _In_    CD3DDeviceLevel1        *pMixerDevice,
        _In_    D3DDEVTYPE              devType
        );

    HRESULT
    TimeCallback(
        _In_    IMFAsyncResult          *pIAsyncResult
        );

    HRESULT
    FlushSamples(
        void
        );

    static inline
    bool
    IsSoftwareFallbackError(
        _In_    HRESULT                     hr
        );

    static inline
    HRESULT
    TreatNonSoftwareFallbackErrorAsUnknownHardwareError(
        _In_    HRESULT                     hr
        );

    static inline
    bool
    IsMandatorySoftwareFallbackError(
        _In_    HRESULT                     hr
        );

    inline
    SampleScheduler &
    GetSampleScheduler(
        void
        );

protected:

    EvrPresenter(
        _In_    MediaInstance           *pMediaInstance,
        _In_    UINT                    resetToken,
        _In_    CWmpStateEngine         *pWmpStateEngine,
        _In_    CDXVAManagerWrapper     *pDXVAManagerWrapper
        );

    virtual
    ~EvrPresenter();

    void *
    GetInterface(
        _In_    REFIID      riid
        );

private:

    //
    // Encapsulated class that provides only IAVSurface Renderer interface.
    // This class interacts with the Composition Engine and, as such, needs
    // to have a different set of locks and data to the ones used by the
    // EVR Presenter we supply to the EVR. To help enforce this separation,
    // the implementation is broken out into a separate class.
    //
    class AVSurfaceRenderer : public IAVSurfaceRenderer
    {
    public:

        AVSurfaceRenderer(
            _In_    UINT                uiID,
            _In_    CWmpStateEngine     *pWmpStateEngine
            );

        ~AVSurfaceRenderer(
            void
            );

        HRESULT
        Init(
            _In_    EvrPresenter        *pEvrPresenter,
            _In_    RenderClock         *pRenderClock
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
        // IAVSurfaceRenderer
        //
        STDMETHOD(BeginComposition)(
            _In_    CMilSlaveVideo  *pCaller,
            _In_    BOOL            displaySetChanged,
            _In_    BOOL            syncChannel,
            __inout LONGLONG        *pLastCompositionSampleTime,
            _Out_   BOOL            *pbFrameReady
            );

        STDMETHOD(BeginRender)(
            __in_ecount_opt(1) CD3DDeviceLevel1 *pDeviceLevel1,        // NULL OK (in SW)
            __deref_opt_out_ecount(1) IWGXBitmapSource **ppWGXBitmapSource
            );

        STDMETHOD(EndRender)(
            );

        STDMETHOD(EndComposition)(
            _In_    CMilSlaveVideo  *pCaller
            );

        STDMETHOD(GetContentRect)(
            __out_ecount(1) MilPointAndSizeL *prcContent
            );

        STDMETHOD(GetContentRectF)(
            __out_ecount(1) MilPointAndSizeF *prcContent
            );

        HRESULT
        ChangeMediaType(
            __in_opt    IMFVideoMediaType       *pIVideoMediaType
            );

        void
        Shutdown(
            void
            );

        void
        SignalFallbackFailure(
            _In_        HRESULT                 hr
            );

        inline
        CD3DDeviceLevel1 *
        CurrentRenderDevice(
            void
            );

    private:

        //
        // Cannot copy or assign an AVSurfaceRenderer
        //
        AVSurfaceRenderer(
            _In_ const AVSurfaceRenderer        &
            );

        AVSurfaceRenderer &
        operator=(
            _In_ const AVSurfaceRenderer &
            );

        HRESULT
        ChooseSample(
            _In_        LONGLONG    currentTime,
            _In_        bool        isPaused,
            _Out_       LONGLONG    *pThisSampleTime
            );

        HRESULT
        GetSWDevice(
            __deref_out CD3DDeviceLevel1        **ppD3DDevice
            );

        HRESULT
        GetHWDevice(
            _In_        UINT                        adapter,
            _In_        bool                        forceMultithreaded,
            __deref_out CD3DDeviceLevel1            **ppD3DDevice
            );

        HRESULT
        NewRenderDevice(
            _In_    CD3DDeviceLevel1            *pNewRenderDevice
            );

        inline
        HRESULT
        FallbackToSoftwareIfNecessary(
            _In_    HRESULT                     hr
            );

        HRESULT
        FallbackToSoftware(
            void
            );

        HRESULT
        SignalMixer(
            void
            );

        static inline
        bool
        IsTransientError(
            _In_    HRESULT         hr
            );

        HRESULT
        AddCompositingResource(
            _In_    CMilSlaveVideo  *pCMilSlaveVideo
            );

        void
        RemoveCompositingResource(
            _In_    CMilSlaveVideo  *pCMilSlaveVideo
            );

        void
        DumpResourceList(
            void
            );

        HRESULT
        PostCompositionPassCleanup(
            void
            );

        //
        // This data is only touched by the composition thread (or is immutable).
        //
        UINT                m_uiID;
        UINT                m_ResetToken;
        EvrPresenter        *m_pEvrPresenter;
        RenderClock         *m_pRenderClock;
        CD3DDeviceLevel1    *m_pCurrentRenderDevice;
        CD3DDeviceLevel1    *m_pSoftwareDevice;
        CMFMediaBuffer      *m_pRenderedBuffer;
        CD3DDeviceLevel1    *m_pCompositionRenderDevice;
        bool                m_haveMultipleCompositionDevices;
        LONGLONG            m_deviceContinuity;
        LONGLONG            m_lastHardwareDeviceContinuity;


        //
        // Composition Lock is used for state that is generally accessed by the
        // composition thread and sometimes by the media thread.
        //
        CCriticalSection    m_compositionLock;
        DWORD               m_dwWidth;
        DWORD               m_dwHeight;
        CWmpStateEngine     *m_pWmpStateEngine;
        bool                m_isPaused;
        LONGLONG            m_lastSampleTime;
        HRESULT             m_fallbackFailure;
        LONGLONG            m_lastBeginCompositionTime;
        CDummySource        *m_pDummySource;
        UniqueList<CMilSlaveVideo*>    m_compositingResources;

        bool                m_syncChannel;

        //
        // Media Lock is used for state that is generally accessed by the media
        // thread and sometimes by the composition lock.
        //
        CCriticalSection    m_mediaLock;

        static const UINT                       msc_defaultAdapter = 0;
    };

    struct ProcessSamplesData
    {
        ProcessSamplesData(
            void
            );

        LONGLONG    nextTime;
        DWORD       continuityKey;
        HRESULT     fallbackFailure;
        bool        mediaFinished;
    };

    //
    // Cannot copy or assign a EvrPresenter.
    //
    EvrPresenter(
        _In_    const EvrPresenter     &
        );

    EvrPresenter &
    operator=(
        _In_    const EvrPresenter     &
        );

    HRESULT
    ClockStarted(
        void
        );

    HRESULT
    Flush(
        void
        );

    HRESULT
    ProcessInvalidateMediaType(
        void
        );

    HRESULT
    GetBestMediaType(
        _Out_       IMFMediaType    **ppIBestMediaType
        );

    HRESULT
    SetMediaType(
        __in_opt    IMFMediaType    *pIMediaType
        );

    HRESULT
    ProcessInputNotify(
        void
        );

    HRESULT
    InvalidateMediaType(
        void
        );

    HRESULT
    ProcessOneSample(
        _In_    LONGLONG    currentTime
        );

    HRESULT
    ProcessSamples(
        __inout     ProcessSamplesData          *pProcessSamplesData,
        _In_        LONGLONG                    currentTime = gc_invalidTimerTime
        );

    void
    ProcessSampleDataOutsideOfLock(
        _In_        const ProcessSamplesData    &processSamplesData
        );

    HRESULT
    BeginStreaming(
        void
        );

    HRESULT
    EndStreaming(
        void
        );

    HRESULT
    EndOfStream(
        void
        );

    HRESULT
    Step(
        _In_        DWORD               stepCount
        );

    HRESULT
    CancelStep(
        void
        );

    HRESULT
    ValidateMixerHasCorrectType(
        _In_        IMFTransform        *pIMixer
        );

    void
    MediaFinished(
        void
        );

    HRESULT
    NotifyEvent(
        long EventCode,
        __in_opt LONG_PTR EventParam1,
        __in_opt LONG_PTR EventParam2
        );

    HRESULT
    NotifyStateEngineOfState(
        RenderState::Enum   state
        );

    void
    CancelAndReleaseTimer(
        void
        );

    static inline
    HRESULT
    CheckForShutdown(
        _In_    RenderState::Enum       renderState
        );

    UINT                    m_uiID;
    UINT m_ResetToken;
    CDXVAManagerWrapper     *m_pDXVAManagerWrapper;
    MediaInstance           *m_pMediaInstance;
    HWND                    m_videoWindow;
    MFVideoNormalizedRect   m_nrcSource;
    RECT                    m_rcDest;

    CCriticalSection        m_csEntry;
    IMediaEventSink         *m_pIMediaEventSink;
    CWmpStateEngine         *m_pWmpStateEngine;
    IMFTransform            *m_pIMixer;
    IMFVideoMediaType       *m_pIVideoMediaType;
    RenderState::Enum       m_renderState;
    bool                    m_endStreaming;
    bool                    m_notifiedOfSample;
    LONGLONG                m_prevMixSampleTime;
    LONGLONG                m_finalSampleTime;

    TimerWrapper            m_timerWrapper;

    DWORD                   m_aspectRatioMode;

    //
    // The following instance members are internally locking
    //
    SampleScheduler         m_sampleScheduler;
    AVSurfaceRenderer       m_surfaceRenderer;

    static const LONGLONG   msc_timerAccuracy = 10000;
    static const float      msc_defaultMaxRate;
    static const float      msc_maxThinningRate;

    static const D3DFORMAT  msc_d3dFormatOrder[];
};

#include "EvrPresenter.inl"

