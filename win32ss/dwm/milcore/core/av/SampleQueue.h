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
//      Provide a structure that holds samples and provides a very light
//      (non-contending and lockless) way to handle and retrieve samples.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#pragma once

namespace SampleThreads
{
    enum Enum
    {
        MixerThread,
        CompositionThread,
        NumberOfThreads
    };
}

class SampleQueue
{
public:

    SampleQueue(
        _In_    UINT        uiID
        );

    ~SampleQueue(
        void
        );

    HRESULT
    Init(
        void
        );

    HRESULT
    ChangeMediaType(
        __in_opt    IMFVideoMediaType   *pIVideoMediaType
        );

    HRESULT
    InvalidateDevice(
        _In_        CD3DDeviceLevel1    *pRenderDevice,
        _In_        CD3DDeviceLevel1    *pMixerDevice,
        _In_        D3DDEVTYPE          deviceType
        );

    HRESULT
    GetMixSample(
        _In_        LONGLONG            currentTime,
        __deref_out IMFSample           **ppISample
        );

    HRESULT
    ReturnMixSample(
        _In_        LONGLONG            currentTime,
        _In_        IMFSample           *pISample
        );

    HRESULT
    GetNextSampleTime(
        _In_        LONGLONG            currentTime,
        _Out_       LONGLONG            *pPrevTime,
        _Out_       LONGLONG            *pNextTime
        );

    HRESULT
    GetSmallestSampleTime(
        _Out_       LONGLONG            *pSmallestTime
        );

    void
    SignalFlush(
        _In_        LONGLONG            currentTime
        );

    HRESULT
    GetCompositionSample(
        _In_            bool                rechooseSample,
        _In_            LONGLONG            currentTime,
        __deref_out     IMFSample           **ppISample
        );

    HRESULT
    RechooseCompositionSampleFromMixerThread(
        _In_            LONGLONG            currentTime
        );

    void
    PauseCompositionSample(
        _In_            LONGLONG            currentTime,
        _In_            bool                allowForwardSamples
        );

    void
    UnpauseCompositionSample(
        void
        );

    HRESULT
    ReturnCompositionSample(
        _Out_           bool                *pSignalMixer
        );

private:

    enum
    {
        kSamples    = 3,
        kViewFields = SampleThreads::NumberOfThreads + 2,
        kInvalidSample = static_cast<BYTE>(-1),
        kNoPauseSample = static_cast<BYTE>(-2),
        kInvalidView   = -1,
        kInvalidTime   = -1,
        kReservedForCompositionTime = -2
    };

    //
    // Cannot copy or assign a SampleQueue.
    //
    SampleQueue(
        _In_ const SampleQueue &
        );

    SampleQueue &
    operator=(
        _In_ const SampleQueue &
        );

    struct StateViewLogicalSample
    {
        BYTE        currentView;
        BYTE        inUseView[SampleThreads::NumberOfThreads];
        BYTE        continuityNumber;
    };

    struct StateView
    {
        LONGLONG    sampleTimes[kSamples];
        BYTE        compositionSample;
        BYTE        mixerSample;
    };

    HRESULT
    GetStateView(
        _In_        SampleThreads::Enum         thread,
        _Out_       StateViewLogicalSample      *pStateView
        );

    bool
    ApplyStateView(
        _In_        SampleThreads::Enum         thread,
        _In_        StateViewLogicalSample      basedOnStateView
        );

    HRESULT
    AllocateSample(
        _Out_       IMFSample           **ppISample
        );

    void
    CalculateNextTime(
        _In_        StateViewLogicalSample      sampleView,
        _In_        LONGLONG                    currentTime,
        _Out_       LONGLONG                    *pLastTime,
        _Out_       LONGLONG                    *pNextTime
        ) const;

    HRESULT
    ValidateAndGetMixSample(
        _In_        BYTE                        sampleToUse,
        _Out_       IMFSample                   **ppISample
        );

    static inline
    StateViewLogicalSample
    TranslateViewState(
        _In_    LONG                        viewState
        );

    static inline
    LONG
    TranslateViewState(
        _In_    StateViewLogicalSample      logicalSample
        );

    static inline
    BYTE
    NextView(
        _In_    BYTE                        view
        );

    static
    void
    ReleaseSample(
        __deref_inout   IMFSample           **ppISample
        );

    void
    DumpState(
        _In_    PCSTR               method,
        _In_    const StateView     &startStateView,
        _In_    const StateView     &endStateView,
        _In_    LONGLONG            currentTime
        );

    void
    DumpSamples(
        _In_    const StateView     &startStateView,
        _In_    const StateView     &endStateView
        );

    void
    DumpTime(
        _In_    PCSTR               method,
        _In_    LONGLONG            currentTime
        );

    BYTE
    ChooseCompositionSample(
        _In_    bool                rechooseSample,
        _In_    bool                allowForwardSamples,
        _In_    LONGLONG            currentTime,
        _In_    const StateView     &stateView
        );

    static inline
    bool
    IsPositiveSampleTime(
        _In_    LONGLONG            sampleTime
        );

    static inline
    bool
    IsExpectedSampleTime(
        _In_    LONGLONG            sampleTime
        );

    static inline
    bool
    IsValidSampleIndex(
        _In_    BYTE                sampleIndex
        );


    UINT                m_uiID;

    LONG                m_viewState;
    StateView           m_stateViews[SampleThreads::NumberOfThreads + 1];

    //
    // The following objects are protected by the media lock.
    // The actual pointer values for m_apISamples is immutable.
    //
    //
    CCriticalSection    m_mediaLock;
    CD3DDeviceLevel1    *m_pRenderDevice;
    CD3DDeviceLevel1    *m_pMixerDevice;
    D3DDEVTYPE          m_deviceType;
    IMFVideoMediaType   *m_pIVideoMediaType;
    LONG                m_continuityNumber;
    IMFSample           *m_apISamples[kSamples];

    static const LONG msc_bitsPerField = 32 / kViewFields;
    static const LONG msc_fieldMask    = (1 << msc_bitsPerField) - 1;
};


#include "SampleQueue.inl"

