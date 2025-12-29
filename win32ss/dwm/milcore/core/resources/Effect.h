// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Effect resource header.
//
//-----------------------------------------------------------------------------

MtExtern(CMilEffectDuce);

// Class: CMilEffectDuce
class CMilEffectDuce : public CMilSlaveResource
{
    friend class CResourceFactory;
    friend class CMilPixelShaderDuce;

private:
    CComposition* m_pCompositionNoRef;


protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilEffectDuce));

    CMilEffectDuce(__in_ecount(1) CComposition* pComposition)
    {
        m_pCompositionNoRef = pComposition;
    }
    
    CMilEffectDuce() 
    {
    }

public:

    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_EFFECT;
    }

    virtual HRESULT ApplyEffect(
        _In_ CContextState *pContextState,
        _In_ CHwSurfaceRenderTarget *pDestRT,
        __in_ecount(1) CMILMatrix *pScaleTransform,
        _In_ CD3DDeviceLevel1 *pDevice, 
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt CHwTextureRenderTarget *pImplicitInputNoRef
        ) = 0;
    
    virtual HRESULT TransformBoundsForInflation(__inout CMilRectF *bounds) = 0;
    
    virtual HRESULT GetLocalSpaceClipBounds(
        _In_ CRectF<CoordinateSpace::LocalRendering> unclippedBoundsLocalSpace,
        _In_ CRectF<CoordinateSpace::PageInPixels> clip,
        _In_ const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
        _Out_ CRectF<CoordinateSpace::LocalRendering> *pClippedBoundsLocalSpace
        );

    virtual ShaderEffectShaderRenderMode::Enum GetShaderRenderMode();

    virtual HRESULT ApplyEffectSw(
        _In_ CContextState *pContextState,
        _In_ CSwRenderTargetSurface *pDestRT,
        _In_ CMILMatrix *pScaleTransform,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IWGXBitmap *pImplicitInput
        ) = 0;

    virtual HRESULT PrepareSoftwarePass(
        _In_ const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,        
        __inout CPixelShaderState *pPixelShaderState, 
        __deref_out CPixelShaderCompiler **ppPixelShaderCompiler
        ) = 0;

    virtual bool UsesImplicitInput() { return true; }

    virtual byte GetShaderMajorVersion()
    {
        //
        // Used when checking for ps_3_0 support when running a ps_3_0 pixel shader.
        // By default, a shader is not a ps_3_0 pixel shader.
        //
        return 2;
    }

protected:

    _Out_ CComposition *GetCompositionDeviceNoRef() 
    {
        Assert(m_pCompositionNoRef != NULL);
        return m_pCompositionNoRef;
    }


    static HRESULT CreateIntermediateRT(
        _In_ CD3DDeviceLevel1 *pD3DDevice, 
        _In_ UINT uWidth, 
        _In_ UINT uHeight, 
        _In_ D3DFORMAT d3dfmtTarget,
        _Out_ CD3DVidMemOnlyTexture **ppVidMemOnlyTexture
        );

    static HRESULT SetupVertexTransform(    
        _In_ const CContextState *pContextState, 
        _In_ CD3DDeviceLevel1 *pDevice, 
        float destinationWidth, 
        float destinationHeight,
        bool passToFinalDestination
        );

    static HRESULT SetSamplerState(
        _In_ CD3DDeviceLevel1 *pDevice,
        UINT uSamplerRegister,
        bool setAddressMode,
        bool useBilinear
        );

    //
    // Hw Pixel Shader Cache
    //
    HRESULT GetHwPixelShaderEffectFromCache(
        _In_ CD3DDeviceLevel1 *pDevice,   
        _In_ UINT cacheIndex,
        _In_ bool forceRecreation,
        __in_bcount(sizeInBytes) BYTE *pPixelShaderByteCode,
        _In_ UINT sizeInBytes,        
        __out_opt CHwPixelShaderEffect **ppPixelShaderEffect);

    void ReleasePixelShaderEffectFromCache(_In_ UINT cacheIndex);
   
    HRESULT SetPixelShaderCacheCapacity(_In_ UINT cacheSize);
    

    static HRESULT LockResource(
        _In_ UINT resourceId, 
        __deref_out_bcount(*pSizeInBytes) BYTE **ppResource,
        _Out_ UINT *pSizeInBytes
        );

};


