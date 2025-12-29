// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      Contains CDesktopRenderTarget which implements IRenderTargetInternal and
//      IMILRenderTargetHWND.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


//+-----------------------------------------------------------------------------
//
//  Class:
//      CDesktopRenderTarget
//
//  Synopsis:
//      This is a multiple or meta Render Target for rendering on multiple
//      desktop devices.  It handles enumerating the devices and managing an
//      array of sub-targets.
//
//      If necessary it is able to hardware accelerate and fall back to software
//      RTs as appropriate.
//
//------------------------------------------------------------------------------

enum State {
    // Invalid state. Set only temporarily during initialization
    Invalid = 0,

    // Normal operating state.
    Ready = 1,

    // A successful update to the current window position via SetPosition
    // is needed.
    FlagNeedSetPosition = 0x80000000,
    NeedSetPosition = 2 | FlagNeedSetPosition,

    // A special case of NeedSetPosition set when a desktop RT no longer
    // matches target layered window size and desktop RT needs resized via
    // a successful call to SetPosition.
    NeedResize = 3 | FlagNeedSetPosition,

    // A RT has been lost and entire desktop RT needs recreated.
    NeedRecreate = 4,
};

ExternTag(tagMILTraceDesktopState);

class CDesktopRenderTarget:
    public CMILCOMBase,
    public CMetaRenderTarget,
    public IMILRenderTargetHWND
{
public:

    // Dimension (width/height) range we allow for render targets:
    //  1. Stable converting to from single precision: <= MAX_INT_TO_FLOAT
    //  2. Within INT range: <= INT_MAX
    //  3. Non-negative: >= 0
    static UINT const kMaxDimension = MAX_INT_TO_FLOAT;
    static UINT const kMinDimension = 0;

    // internal methods.

protected:

    CDesktopRenderTarget(
        __out_ecount(cMaxRTs) MetaData *pMetaData,
        UINT cMaxRTs,
        __inout_ecount(1) CDisplaySet const *pDisplaySet
        );

protected:

    virtual ~CDesktopRenderTarget();


    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv) /* override */;

    virtual HRESULT EditMetaData() = 0;


    void SetSingleSubRT();

    HRESULT Init(
        _In_ HWND hwnd,
        MilWindowLayerType::Enum eWindowLayerType,
        MilRTInitialization::Flags dwFlags
        );


    //+-----------------------------------------------------------------------------
    //
    //  Member:
    //      CDesktopRenderTarget::TransitionToState
    //
    //  Synopsis:
    //      Set new state (Validate in debug)
    //
    //------------------------------------------------------------------------------
    MIL_FORCEINLINE void TransitionToState(
        State eNewState
#if DBG
        , const char *pszMethod = NULL
#endif
        )
    {
#if DBG
        Assert(DbgIsValidTransition(eNewState));
#endif

        if (IsTagEnabled(tagMILTraceDesktopState))
        {
            static const char * const rgStateName[] = {
                "Invalid",
                "Ready",
                "NeedSetPosition",
                "NeedResize",
                "NeedRecreate",
            };

            static_assert(ARRAY_SIZE(rgStateName) == NeedRecreate + 1, "ARRAY_SIZE(rgStateName) == NeedRecreate + 1");

#if DBG
            TraceTag((tagMILTraceDesktopState,
                      "0x%p Desktop::%s: %s to %s",
                      this,
                      pszMethod,
                      rgStateName[m_eState],
                      rgStateName[eNewState]
                      ));
#endif
        }

        m_eState = eNewState;

        return;
    }

    // Note: This call is dangerous since the caller must handle or propagate the mode change
    //       if this returns true, otherwise the DisplaySet can change in the middle of processing
    //       a frame which is unexpected.
    bool DangerousHasDisplayChanged() const
    {
        return DisplaySet()->DangerousHasDisplayStateChanged();
    }

public:

    // This method supports the factory. It creates an initialized and
    // referenced RT instance.

    static HRESULT Create(
        __in_opt HWND hwnd,
        __in_ecount(1) CDisplaySet const *pDisplaySet,            // Display Set
        MilWindowLayerType::Enum eWindowLayerType,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetHWND **ppIRenderTarget
        );

    // IUnknown.

    DECLARE_COM_BASE;

    // IMILRenderTarget.

    STDMETHOD(Clear)(
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        ) /* override */;

    STDMETHODIMP Begin3D(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        ) /* override */;

    STDMETHODIMP End3D(
        ) /* override */;

    // IMILRenderTargetHWND.

    STDMETHOD(Present)(
        ) /* override */;

    STDMETHOD(ScrollBlt) (
        __in_ecount(1) const RECT *prcSource,
        __in_ecount(1) const RECT *prcDest
        ) /* override */;

    STDMETHOD(Invalidate)(
        __in_ecount_opt(1) MilRectF const *prc
        ) /* override */;

    STDMETHOD_(VOID, GetBounds)(
        __out_ecount(1) MilRectF * const pBounds
        ) /* override */;


    STDMETHOD(WaitForVBlank)(
        ) /* override */;

    STDMETHOD_(VOID, AdvanceFrame)(
        UINT uFrameNumber
        ) /* override */;

    STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        ) /* override */;

    STDMETHOD(CanAccelerateScroll)(
        __out_ecount(1) bool *pfCanAccelerateScroll
        ) /* override */;

    // IRenderTargetInternal.

    STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        ) /* override */;

#if DBG
    //
    // These are only defined in checked builds to assert state.  Retail builds
    // could try to optimize away these calls, but statck capture infrastruture
    // could prevent it; so, just be explicit about not wanting them in retail.
    //

    // IRenderTargetInternal.

    STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) /* override */;

    STDMETHOD(DrawMesh3D)(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __inout_ecount_opt(1) CMILShader *pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) /* override */;

    STDMETHOD(DrawPath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) IShapeData *pShape,
        __inout_ecount_opt(1) CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        ) /* override */;

    STDMETHOD(DrawGlyphs)(
        __inout_ecount(1) DrawGlyphsParameters &pars
        ) /* override */;

    STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) /* override */;
#endif /* DBG */

protected:

    // Common hwnd for all the devices.
    HWND m_hwnd;

    // Bounds of the backbuffer
    CMILSurfaceRect m_rcSurfaceBounds;

    // Remember flags in case SW RT's need to be created after init
    MilRTInitialization::Flags m_dwRTInitFlags;

    //
    // State of desktop RT
    //
    // Expected state transitions:
    //
    //  Ready to:
    //    Ready: not expected
    //    NeedSetPosition: SetPosition failed for a reason other than WGXERR_DISPLAYSTATEINVALID
    //    NeedResize: RT Present returned ERROR_INCORRECT_SIZE (Present)
    //    NeedRecreate: RT Present returned WGXERR_DISPLAYSTATEINVALID (Present)
    //      or RT Resize/Create returned WGXERR_DISPLAYSTATEINVALID (SetPosition)
    //
    //  NeedSetPosition to:
    //    Ready: SetPosition has been successful
    //    NeedSetPosition: Initial or multiple SetPositions failed (SetPosition)
    //    NeedResize: not expected
    //    NeedRecreate: RT Resize/Create returned WGXERR_DISPLAYSTATEINVALID (SetPosition)
    //
    //  NeedResize to:
    //    Ready: SetPosition has been successful
    //    NeedSetPosition: SetPosition is called, but fails
    //    NeedResize: not expected
    //    NeedRecreate: RT Resize/Create returned WGXERR_DISPLAYSTATEINVALID (SetPosition)
    //
    //  NeedRecreate to:
    //    Ready: not expected
    //    NeedSetPosition: not expected
    //    NeedResize: not expected
    //    NeedRecreate: RT Present returned WGXERR_DISPLAYSTATEINVALID and
    //      SetPosition was called to resize RTs to 0 x 0
    //

    State m_eState;

    // Location of client region OR fullscreen RTs in virtual desktop space.
    // For HWND targets this is the last position successfuly reported to
    // SetPosition.
    CMILSurfaceRect m_rcCurrentPosition;

#if DBG
private:
    virtual bool DbgIsValidTransition(enum State eNewState) = 0;
#endif
};



