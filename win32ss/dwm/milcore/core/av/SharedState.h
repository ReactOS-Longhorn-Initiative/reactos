// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

//
// SharedState keeps track of the state that must be shared across the apartment
// thread and the UI thread. This object is aggregated by CWmpPlayer
//
class SharedState
{
public:

    SharedState(
        void
        );

    HRESULT
    Init(
        void
        );

    UINT
    GetNaturalWidth(
        void
        );

    void
    SetNaturalWidth(
        _In_    UINT        width
        );

    UINT
    GetNaturalHeight(
        void
        );

    void
    SetNaturalHeight(
        _In_    UINT        height
        );

    bool
    GetIsBuffering(
        void
        );

    void
    SetIsBuffering(
        _In_    bool        isBuffering
        );

    bool
    GetCanPause(
        void
        );

    void
    SetCanPause(
        _In_    bool        canPause
        );

    bool
    GetHasVideo(
        void
        );

    void
    SetHasVideo(
        _In_    bool        hasVideo
        );

    bool
    GetHasAudio(
        void
        );

    void
    SetHasAudio(
        _In_    bool        hasAudio
        );

    LONGLONG
    GetLength(
        void
        );

    void
    SetLength(
        _In_    LONGLONG    length
        );

    double
    GetDownloadProgress(
        void
        );

    void
    SetDownloadProgress(
        _In_    double      downloadProgress
        );

    double
    GetBufferingProgress(
        void
        );

    void
    SetBufferingProgress(
        _In_    double      bufferingProgress
        );

    LONGLONG
    GetPosition(
        void
        );

    void
    SetPosition(
        _In_    LONGLONG    position
        );

    LONGLONG
    GetTimedOutPosition(
        void
        );

    void
    SetTimedOutPosition(
        _In_    LONGLONG    position
        );

    double
    GetTimedOutDownloadProgress(
        void
        );

    void
    SetTimedOutDownloadProgress(
        _In_    double      downloadProgress
        );

    double
    GetTimedOutBufferingProgress(
        void
        );

    void
    SetTimedOutBufferingProgress(
        _In_    double      bufferingProgress
        );

private:
    CCriticalSection    m_lock;
    UINT                m_uiID;

    LONGLONG            m_length;

    UINT                m_width;
    UINT                m_height;

    bool                m_isBuffering;
    bool                m_canPause;
    bool                m_hasVideo;
    bool                m_hasAudio;

    double              m_downloadProgress;
    double              m_bufferingProgress;
    LONGLONG            m_position;

    Optional<LONGLONG>  m_timedOutPosition;
    Optional<double>    m_timedOutDownloadProgress;
    Optional<double>    m_timedOutBufferingProgress;
};


