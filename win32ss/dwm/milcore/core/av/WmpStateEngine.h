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
//      Provides support for the player state manager. This provides a separate
//      thread which starts up the Player OCX and also provides services for
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#pragma once

MtExtern(CWmpStateEngine);
MtExtern(SubArcMethodItem);

#ifdef DBG_CODE
#undef DBG_CODE
#endif

#if DBG

    #define DBG_CODE(x)     x

#else

    #define DBG_CODE(x)

#endif

class CMediaEventProxy;
class SharedState;

class CWmpStateEngine : public CStateThreadItem,
                        public CMILCOMBase
{
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CWmpStateEngine));

    // Declares IUnknown functions
    DECLARE_COM_BASE;

    static
    HRESULT
    Create(
        _In_        MediaInstance       *pMediaInstance,
        _In_        bool                canOpenAnyMedia,
        _In_        SharedState         *pSharedState,
        __deref_out CWmpStateEngine     **ppPlayerState
        );

    //
    // The only free-threaded member function. Use this to schedule work from
    // another thread.
    //
    inline
    HRESULT
    AddItem(
        _In_    CStateThreadItem    *pItem
        );

    //
    // Stuff called from CWmpClientSite
    //
    void
    SetHasAudio(
        _In_    bool            hasAudio
        );

    void
    SetHasVideo(
        _In_    bool            hasVideo
        );

    HRESULT
    NewPresenter(
        __deref_out    EvrPresenterObj    **ppNewPresenter
        );

    //
    // Stuff called from CWmpEventHandler
    //
    void
    PlayerReachedPosition(
        _In_    double                  newPosition
        );

    void
    PlayerReachedActionState(
        _In_    WMPPlayState            state
        );

    void
    PlayerReachedOpenState(
        _In_    WMPOpenState            state
        );

    HRESULT
    EvrReachedState(
        _In_    RenderState::Enum       renderState
        );

    HRESULT
    ScrubSampleComposited(
        _In_    int                     placeholder
        );

    //
    // Stuff called from CWmpPlayer because it implements IMILMedia
    //
    HRESULT
    Close(
        void
        );

    HRESULT
    SetTargetOcx(
        _In_    bool                isOcxCreated
        );

    HRESULT
    SetTargetUrl(
        _In_    LPCWSTR             url
        );

    HRESULT
    SetTargetActionState(
        _In_    ActionState::Enum   actionState
        );

    HRESULT
    SetTargetVolume(
        _In_    long                volume
        );

    HRESULT
    SetTargetBalance(
        _In_    long                balance
        );

    HRESULT
    SetTargetRate(
        _In_    double              rate
        );

    HRESULT
    SetTargetSeekTo(
        _In_    Optional<double>    seekTo
        );

    HRESULT
    SetTargetIsScrubbingEnabled(
        _In_    bool                isScrubbingEnabled
        );

    HRESULT
    InvalidateDidRaisePrerolled(
        void
        );

    HRESULT
    UpdatePosition(
        void
        );

    HRESULT
    UpdateNaturalHeight(
        void
        );

    HRESULT
    UpdateNaturalWidth(
        void
        );

    HRESULT
    UpdateDownloadProgress(
        void
        );

    HRESULT
    UpdateBufferingProgress(
        void
        );

    HRESULT
    Shutdown(
        _In_    int     placeholder
        );

    //
    // Called by CWmpPlayer because it implements IMILSurfaceRendererProvider
    //
    HRESULT
    GetSurfaceRenderer(
        __deref_out    IAVSurfaceRenderer  **ppISurfaceRenderer
        );

    void
    NeedUIFrameUpdate(
        void
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
    typedef
    HRESULT
    (CWmpStateEngine::*SubArcMethod)(
        void
        );

    CWmpStateEngine(
        _In_        MediaInstance           *pMediaInstance,
        _In_        bool                    canOpenAnyMedia,
        _In_        SharedState             *pSharedState
        );

    virtual
    ~CWmpStateEngine(
        );

    HRESULT
    Init(
        void
        );

    //
    // Cannot copy or assign a CWmpStateEngine
    //
    CWmpStateEngine(
        _In_ const CWmpStateEngine &
        );

    CWmpStateEngine &
    operator=(
        _In_ const CWmpStateEngine &
        );

    HRESULT
    InitializeOcx(
        void
        );

    HRESULT
    DiscardOcx(
        void
        );

    HRESULT
    RemoveOcx(
        void
        );

    void
    ErrorInTransition(
        _In_    HRESULT                 hr
        );

    HRESULT
    MediaFinished();

    HRESULT
    SetPauseTimeOnPresenter(
        void
        );

    HRESULT
    CheckPlayerVersion(
        _In_            IWMPPlayer          *pIWmpPlayer
        );

    void
    HandleStateChange(
        void
        );

    HRESULT
    DoPreemptiveTransitions(
        void
        );

    HRESULT
    BeginNewTransition(
        void
        );


    //
    // Single step arc methods
    //
    HRESULT
    DoVolumeArc(
        void
        );

    HRESULT
    DoBalanceArc(
        void
        );

    HRESULT
    DoRateArc(
        void
        );


    //
    // Multi step arc methods
    //
    HRESULT
    BeginActionStateArc(
        void
        );

    HRESULT
    BeginUrlArc(
        void
        );

    HRESULT
    ContinueUrlArc(
        void
        );

    HRESULT
    BeginSeekToArc(
        void
        );

    HRESULT
    PostSeek(
        void
        );

    HRESULT
    BeginSeekToAndScrubArc(
        void
        );

    HRESULT
    FinishSeekToArc(
        void
        );

    HRESULT
    ContinueSeekToAndScrubArc(
        void
        );

    //
    // Action state arc transition methods
    //
    HRESULT
    BeginStopToPauseArc(
        void
        );

    HRESULT
    BeginStopToPlayArc(
        void
        );

    HRESULT
    BeginPauseToStopArc(
        void
        );

    HRESULT
    BeginPauseToPlayArc(
        void
        );

    HRESULT
    BeginPlayToStopArc(
        void
        );

    HRESULT
    BeginPlayToPauseArc(
        void
        );

    HRESULT
    StopToPauseArc_Done(
        void
        );

    HRESULT
    StopToPlayArc_Done(
        void
        );


    HRESULT
    PlayToRealPauseArc_Pause(
        void
        );

    HRESULT
    PlayToRealPauseArc_Done(
        void
        );

    HRESULT
    PlayToFakePauseArc_Done(
        void
        );


    HRESULT
    RealPauseToPlayArc_Play(
        void
        );

    HRESULT
    FakePauseToPlayArc_Done(
        void
        );

    HRESULT
    ContinueStopToPauseArc(
        void
        );

    HRESULT
    BeginNoScrubStopToPauseArc(
        void
        );

    HRESULT
    NoScrubStopToPauseArc_Pause(
        void
        );

    HRESULT
    NoScrubStopToPauseArc_Seek(
        void
        );

    HRESULT
    BeginScrubArc(
        void
        );

    HRESULT
    ScrubArc_Pause(
        void
        );

    HRESULT
    ScrubArc_Seek(
        void
        );

    HRESULT
    Arc_ActionStateComplete(
        void
        );

    HRESULT
    Arc_WaitForActionState(
        void
        );


    //
    // Capability query methods
    //
    HRESULT
    UpdateCanPause(
        void
        );

    void
    SetCanPause(
        _In_    bool        canPause
        );

    void
    SetIsBuffering(
        _In_    bool        isBuffering
        );

    void
    SetDownloadProgress(
        _In_    double      downloadProgress
        );

    void
    SetBufferingProgress(
        _In_    double      bufferingProgress
        );

    HRESULT
    InternalCanSeek(
        void
        );

    HRESULT
    UpdateHasAudioForWmp11(
        void
        );

    HRESULT
    UpdateHasVideoForWmp11(
        void
        );

    HRESULT
    UpdateMediaLength(
        void
        );

    void
    SetMediaLength(
        _In_    double      mediaLength
        );

    HRESULT
    RaiseEvent(
        _In_    AVEvent     event,
        _In_    HRESULT     failureHr = S_OK
        );

    void
    PlayerReachedActionStatePlay(
        void
        );

    HRESULT
    SignalSelf(
        void
        );

    HRESULT
    SetSafeForScripting(
        _In_        IWMPPlayer      *pIWMPMedia
        );

    HRESULT
    RaisePrerolledIfNecessary(
        void
        );

    static inline
    bool
    EvrStateToIsEvrClockRunning(
        _In_      RenderState::Enum                    renderState
        );

    static inline
    bool
    IsStatePartOfSet(
        _In_                        ActionState::Enum           playerState,
        __in_ecount(cPlayerState)   const ActionState::Enum     *aPlayerState,
        _In_                        int                         cPlayerState
        );

    static inline
    bool
    IsStatePartOfSet(
        _In_                        WMPPlayState                playerState,
        __in_ecount(cPlayerState)   const WMPPlayState          *aPlayerState,
        _In_                        int                         cPlayerState
        );

    static
    Optional<ActionState::Enum>
    MapWmpStateEngine(
        _In_                        WMPPlayState                playerState
        );

    static inline
    LONGLONG
    SecondsToTicks(
        _In_                        double                      seconds
        );

    //
    // Immutable variables, never change so they don't have to be accessed
    // from the lock.
    //
    UINT                    m_uiID;
    UINT                    m_ResetToken;
    DummySurfaceRenderer    *m_pDummyRenderer;
    MediaInstance           *m_pMediaInstance;

    //
    // General variables
    //
    CStateThread            *m_pStateThread;
    DWORD                   m_uiThreadId;
    bool                    m_isShutdown;


    //
    // external Avalon stuff
    //
    CDXVAManagerWrapper     *m_pDXVAManagerWrapper;
    CWmpEventHandler        *m_pWmpEventHandler;

    //
    // We need to wrap the EvrPresenter to account
    // for EvrPresenter changes when graphs change.
    //
    PresenterWrapper        m_presenterWrapper;

    //
    // State that must be shared between the apartment thread and the UI thread
    //
    SharedState             *m_pSharedState;

    //
    // WMP stuff
    //
    IWMPPlayer              *m_pIWMPPlayer;
    IConnectionPoint        *m_pIConnectionPoint;
    DWORD                   m_connectionPointAdvise;

    //
    // Variables only accessed by the state thread
    //

    // The most recent state that WMP has reported to us
    PlayerState             m_actualState;

    // The most recent state the EVR has reported to us
    bool                    m_isEvrClockRunning;

    // Whether or not media has ended.
    bool                    m_isMediaEnded;

    // This will be set after calling BeginSeekToArc
    bool                    m_didSeek;

    // We may need to flush when doing a non-scrub preroll
    bool                    m_needFlushWhenEndingFreeze;

    //
    // The 3 internal states are the states CWmpStateEngine accesses from the
    // state thread.
    //
    // m_currentInternalState is the current state we are in. This may differ
    // from m_actualState when, for example, we are "pausing" non-pauseable
    // media. In this case m_actualState will be Play, but
    // m_currentInternalState will be Pause.
    //
    // m_pendingInternalState is the state to which we are in the middle of
    // transitioning.
    //
    // m_targetInternalState is the state that the caller most recently
    // requested. This is mostly the same as m_targetState, except that it is
    // only accessed from the state thread, and it may differ in volume (See
    // m_volumeMask). It is synchronized to m_targetState at the beginning of
    // HandleStateChange
    //
    PlayerState             m_currentInternalState;
    PlayerState             m_pendingInternalState;
    PlayerState             m_targetInternalState;

    //
    // Sometimes we have to mute the volume and later resume it (e.g. fake
    // pause, stop to pause transition). In this case we "mask" the volume to
    // 0. When m_targetInternalState is synchronized to m_targetState, the
    // volume will be taken from m_volumeMask if the mask is valid.
    //
    Optional<long>          m_volumeMask;


    //
    // Variables only accessed from the UI thread
    //

    //
    // We assume that WMP won't change its mind after it has reported a non-zero
    // length to us, so we cache it until the url changes
    //
    double                  m_mediaLength;

    //
    // The state the caller has most recently requested - used to update
    // m_targetInternalState
    //
    PlayerState             m_targetState;

    bool                    m_canSeek;
    bool                    m_isScrubbingEnabled;
    Optional<double>        m_cachedScrubPosition;

    //
    // When we're in the midst of scrubbing, this variable describes whether or
    // not we've received our scrub sample
    //
    bool                    m_didReceiveScrubSample;

    bool                    m_didPreroll;
    bool                    m_didRaisePrerolled;

    //
    // We need another variable outside of SharedState to keep track of
    // m_canPause across closing and reopening. We can't rely on the OCX to
    // keep track of this information since we can only get this information
    // when the play state is wmppsPlaying
    //
    bool                    m_canPause;

    //
    // typedef's
    //
    typedef CGuard<CCriticalSection>    CriticalSectionGuard_t;

    class SubArcMethodStack
    {
    private:
        struct SubArcMethodItem : ListNodeT<SubArcMethodItem>
        {
            DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(SubArcMethodItem));

            SubArcMethod m_method;

            static
            HRESULT
            Create(
                SubArcMethod method,
                SubArcMethodItem **ppItem
                );
        };

        List<SubArcMethodItem>  m_stack;
        CWmpStateEngine         *m_pCWmpStateEngine;
        UINT                    m_uiID;

    public:
        SubArcMethodStack(
            UINT            uiID
            );

        ~SubArcMethodStack(
            void
            );

        void
        SetStateEngine(
            CWmpStateEngine *pCWmpStateEngine
            );

        HRESULT
        Push(
            SubArcMethod    next
            );

        HRESULT
        PopAndCall(
            void
            );

        HRESULT
        Clear(
            void
            );

        bool
        IsEmpty(
            void
            ) const;
    };

    //
    // We emulate coroutines with a stack of methods
    //
    SubArcMethodStack       m_nextSubArcMethodStack;

    ActionState::Enum       m_waitForActionState;
    ActionState::Enum       m_lastActionState;
    ActionState::Enum       m_lastRenderState;

    bool                    m_useRenderConfig;
    bool                    m_canOpenAnyMedia;

    HANDLE                  m_isShutdownEvent;

#if DBG_ANALYSIS
    DWORD                   m_stateThreadId;
#endif
};

#include "WmpStateEngine.inl"

