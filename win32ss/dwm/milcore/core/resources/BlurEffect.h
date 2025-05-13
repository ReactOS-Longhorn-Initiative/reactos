// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Blur bitmap effect resource header.
//
//-----------------------------------------------------------------------------

MtExtern(CMilBlurEffectDuce);

class P_u32;
class C_u32;
class C_u32x4;
class P_f32x1;
class P_f32x4;
class P_u32x4;
struct u32x4;


//
// GenerateColors function prototype
//
struct GenerateColorsBlurParams
{
    unsigned *pargbSource;
    unsigned *pargbDestination;
    UINT sourceWidth;
    UINT radius;
    UINT nOutputPixelsPerLine;
    UINT nOutputLines;
    void *pBoxBlurLineBuffer;
    UINT boxBlurLineBufferLength;
    float *pGaussianWeights;
    UINT vertical;
};

typedef void (__stdcall *GenerateColorsBlur)(
    _In_ GenerateColorsBlurParams *pParams
    );

// Class: CMilBlurEffectDuce
class CMilBlurEffectDuce : public CMilEffectDuce
{
    friend class CResourceFactory;

public:
    /* override */ virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_BLUREFFECT || CMilEffectDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_BLUREFFECT* pCmd
        );

    /* override */ HRESULT ApplyEffect(
        _In_ CContextState *pContextState, 
        _In_ CHwSurfaceRenderTarget *pDestRT,
        _In_ CMILMatrix *pScaleTransform,
        _In_ CD3DDeviceLevel1 *pDevice, 
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt CHwTextureRenderTarget *pImplicitInput
        );

    /* override */ HRESULT ApplyEffectSw(
        _In_ CContextState *pContextState,
        _In_ CSwRenderTargetSurface *pDestRT,
        _In_ CMILMatrix *pScaleTransform,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IWGXBitmap *pImplicitInput
        );

    /* override */ HRESULT PrepareSoftwarePass(
        _In_ const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
        __inout CPixelShaderState *pPixelShaderState, 
        __deref_out CPixelShaderCompiler **ppPixelShaderCompiler
        )
    {
        RRETURN(E_UNEXPECTED);
    }
    
    /* override */ HRESULT TransformBoundsForInflation(__inout CMilRectF *bounds);

    /* override */ HRESULT GetLocalSpaceClipBounds(
        _In_ CRectF<CoordinateSpace::LocalRendering> unclippedBoundsLocalSpace,
        _In_ CRectF<CoordinateSpace::PageInPixels> clip,
        _In_ const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
        _Out_ CRectF<CoordinateSpace::LocalRendering> *pClippedBoundsLocalSpace
        );

    HRESULT ApplyEffectInPipeline(
        _In_ const CContextState *pContextState, 
        _In_ const CMILMatrix *pScaleTransform,
        _In_ CD3DDeviceLevel1 *pDevice,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        _In_ CHwTextureRenderTarget *pSourceRT, 
        _In_ CD3DVidMemOnlyTexture *pDestRT
        );

    static HRESULT Create(
        DOUBLE radius, 
        MilKernelType::Enum kernelType,
        MilEffectRenderingBias::Enum renderingBias,
        __deref_out CMilBlurEffectDuce **ppBlur
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    /* override */ void UnRegisterNotifiers();

    static HRESULT CalculateGaussianSamplingWeightsFullKernel(
        _In_ UINT radius,
        __deref_out_xcount(2*radius+1) float **ppSamplingWeights
        );

    static HRESULT InitializeBlurFunction(bool fGaussian, bool fColor, GenerateColorsBlur *pProgram);

    // These should probably both go in a utility class sometime.
    static HRESULT ClearMarginPixels(
         __inout_ecount(width*height) UINT *pStart, 
         UINT width, 
         UINT height,
         UINT leftMargin,
         UINT topMargin,
         UINT rightMargin,
         UINT bottomMargin
         );

    static void ApplyRadiusScaling(
        _In_ const CMILMatrix *pScaleTransform,
        _In_ UINT localSpaceRadius,
        _Out_ UINT *scaledRadius
        );
    
protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilBlurEffectDuce));

    CMilBlurEffectDuce(__inout_ecount(1) CComposition* pComposition) : CMilEffectDuce(pComposition)
    {
        m_pComposition = pComposition;
    }

    CMilBlurEffectDuce(DOUBLE radius, MilKernelType::Enum kernelType, MilEffectRenderingBias::Enum renderingBias);

    ~CMilBlurEffectDuce();

    /* override */ HRESULT Initialize();    

private:
    double GetRadius();

    void GetScaledRadius(
        _In_ const CMILMatrix *pScaleTransform,
        _Out_ UINT* pRadius
        );

    static void CalculateSamplingWeights(
        _In_ UINT radius,
        __deref_out_xcount(radius+1) float **ppSamplingWeights,
        _In_ MilKernelType::Enum kernelType
        );

    static HRESULT SetupShader(
         _In_ CD3DDeviceLevel1 *pDevice, 
        bool isHorizontalPass,
        bool isMultiInputPass,
        float destinationSize,
        UINT cSamples,
        int sampleIndex,
        _In_ float *arrSamplingWeights
        );

    HRESULT ApplyEffectImpl(
        _In_ const CContextState *pContextState,
        _In_ const CMILMatrix *pScaleTransform,
        _In_ CD3DDeviceLevel1 *pDevice,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        _In_ CHwTextureRenderTarget *pSourceRT, 
        __in_opt CHwSurfaceRenderTarget *pFinalDestRT,
        __in_opt CD3DVidMemOnlyTexture *pPipelineDestRT
        );

    HRESULT ExecutePasses(
        _In_ CD3DDeviceLevel1 *pDevice, 
        _In_ bool isHorizontal,
        _In_ bool isQuality,
        _In_ UINT radius,
        _In_ float destinationSize,
        _In_ float* pSamplingWeights,
        _In_ CD3DVidMemOnlyTexture* pTextureNoRef_A,
        _In_ CD3DVidMemOnlyTexture* pTexture_B,
        _In_ CD3DSurface* pSurface_B,
        __in_opt CD3DVidMemOnlyTexture* pTexture_C,
        __in_opt CD3DSurface* pSurface_C
        );

    HRESULT ApplyGaussianBlurSw(__in_ecount(sourceWidth * sourceHeight * 4) BYTE * pInputOutputBuffer,
                                __in_ecount(sourceWidth * sourceHeight * 4) BYTE * pIntermediateBuffer,
                                UINT sourceWidth,
                                UINT sourceHeight,
                                UINT radius
                                );

    HRESULT ApplyBoxBlurSw(__in_ecount(sourceWidth * sourceHeight * 4) BYTE * pInputBuffer,
                           __in_ecount(sourceWidth * sourceHeight * 4) BYTE * pOutputBuffer,
                           UINT sourceWidth,
                           UINT sourceHeight,
                           UINT radius);                                   
    
    static C_u32x4 SetupBox(P_u32 pSource, 
                             C_u32 sourcePositionDelta,
                             C_u32 sampleLength,
                             C_u32 sourceWidth, 
                             P_u32x4 pLineSumBuffer, 
                             C_u32 lineSumBufferCount
                             );

    static C_u32x4 MoveBoxToNextLine(P_u32 pSource, 
                             C_u32 sampleLength,
                             C_u32 sourceWidth, 
                             P_u32x4 pLineSumBuffer, 
                             C_u32 lineSumBufferCount
                             );

    static void SampleBox(C_u32 sampleLength,
                          P_u32 pDst, 
                          P_u32x4 pLineSumBuffer,
                          C_u32x4 *pPreviousSumValue,
                          C_f32x4 sampleLengthSquareReplicate
                          );

    static void SampleGaussian(P_u32 pSource, 
                               C_u32 sourcePositionDelta,
                               C_u32 sampleLength,
                               P_u32 pDst, 
                               P_f32x1 pGaussianWeights
                               );

    static void TakeNSamples(C_u32 sampleCount, 
                               P_u32 pSource, 
                               C_u32 sourcePositionDelta,
                               C_u32x4 *result,
                               P_f32x1 pGaussianWeights,
                               bool fGaussian
                               );
    
    static C_u32 DivideAndPackResult(C_u32x4 uResult, C_f32x4 divisor);

    static C_u32 PackResult(C_u32x4 uResult);
    
    static C_u32x4 Sample(P_u32 pSampleSource);
    
    CMilBlurEffectDuce_Data m_data;
    
    // The maximum number of blur samples we can take in one hardware shader pass.
    // We can run multiple passes to accumulate enough samples for large radius blurs.
    static const UINT MAX_SAMPLES_PER_PASS = 15;

    // The maximum supported radius for a blur effect.
    static const UINT MAX_RADIUS = 100;
    
    // Holds the pixel shader resources (a pair of horizontal and vertical, one
    // each for single-texture input and for multi-texture input).
    static CMilPixelShaderDuce* s_pBlurPixelShaders[4];

    // Holds the compiled SIMD code for the software blur and Gaussian functions
    static GenerateColorsBlur s_pfnBlurFunctionBox;
    static GenerateColorsBlur s_pfnBlurFunctionGaussian;

    // Column buffer required for box blur
    BYTE *m_pBoxBlurLineBuffer;
    UINT m_boxBlurLineBufferSize;

    CComposition* m_pComposition;
};


