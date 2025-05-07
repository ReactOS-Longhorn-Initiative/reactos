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
//      Header for the CWmpPlayer class, which adapts the WMP OCX interface to
//      match the interface we'd like to have. For example, we convert volume
//      from a double between 0 and 1 to an integer between 0 and 100.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once

MtExtern(CWmpPlayer);

class UpdateState;

class CWmpPlayer : public IMILMedia,
                   public IMILSurfaceRendererProvider,
                   public CMILCOMBase
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CWmpPlayer));

    static
    STDMETHODIMP
    Create(
        _In_            MediaInstance       *pMediaInstance,
        _In_            bool                canOpenAnyMedia,
        __deref_out     CWmpPlayer          **ppPlayer
        );

    // Declares IUnknown functions
    DECLARE_COM_BASE;

    //
    // IMILMedia
    //
    STDMETHOD(Open)(
        _In_ LPCWSTR pwszURL
        );

    STDMETHOD(Stop)();

    STDMETHOD(Close)();

    STDMETHOD(GetPosition)(
        _Out_    LONGLONG    *pllTime
        );

    STDMETHOD(SetPosition)(
        _In_    LONGLONG    llTime
        );

    STDMETHOD(SetRate)(
        _In_    double      dblRate
        );

    STDMETHOD(SetVolume)(
        _In_    double      dblVolume
        );

    STDMETHOD(SetBalance)(
        _In_    double      dblBalance
        );

    STDMETHOD(SetIsScrubbingEnabled)(
        _In_    bool        isScrubbingEnabled
        );

    /* Return whether or not we're currently buffering */
    STDMETHOD(IsBuffering)(
        _Out_   bool        *pIsBuffering
        );

    /* Return whether or not we can pause */
    STDMETHOD(CanPause)(
        _Out_   bool        *pCanPause
        );

    /* Get the download progress */
    STDMETHOD(GetDownloadProgress)(
        _Out_   double      *pProgress
        );

    /* Get the buffering progress */
    STDMETHOD(GetBufferingProgress)(
        _Out_   double      *pProgress
        );

    STDMETHOD(HasVideo)(
        _Out_   bool        *pfHasVideo
        );

    STDMETHOD(HasAudio)(
        _Out_   bool        *pfHasAudio
        );

    STDMETHOD(GetNaturalHeight)(
        _Out_   UINT        *puiHeight
        );

    STDMETHOD(GetNaturalWidth)(
        _Out_   UINT        *puiWidth
        );

    // Get the duration of the clip in 100 nanosecond ticks
    STDMETHOD(GetMediaLength)(
        _Out_   LONGLONG    *pllLength
        );

    //
    // IMILSurfaceRendererProvider
    //
    STDMETHOD(GetSurfaceRenderer)(
        __deref_out IAVSurfaceRenderer **ppSurfaceRenderer
        );

    STDMETHOD(RegisterResource)(
        _In_    CMilSlaveVideo *pSlaveVideo
        );

    STDMETHOD(UnregisterResource)(
        _In_    CMilSlaveVideo *pSlaveVideo
        );

    STDMETHOD(NeedUIFrameUpdate)(
        );

    STDMETHOD(Shutdown)();

    STDMETHOD(ProcessExitHandler)();

protected:
    //
    // CMILCOMBase
    //
    STDMETHOD(HrFindInterface)(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppvObject
        );

private:
    CWmpPlayer(
        _In_    MediaInstance       *pMediaInstance
        );

    virtual ~CWmpPlayer();

    HRESULT
    Init(
        _In_            bool                canOpenAnyMedia
        );

    SharedState         m_sharedState;
    UpdateState         *m_pUpdateState;
    CWmpStateEngine     *m_pCWmpStateEngine;
    MediaInstance       *m_pMediaInstance;
    bool                m_fShutdown;
    UINT                m_uiID;
    PWSTR               m_currentUrl;
};

