// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Pixel shader compiler
//

#pragma once

// Custom HRESULT included here for WPF/SL interop.  SL uses XRESULT elsewhere
// and WPF uses HRESULT.
typedef long PS_HRESULT;

struct CTextureVariables;
struct CInstructionVariables;
struct PSTRINST_BASE_PARAMS;
class CPixelShaderRegisters;
class RDPSTrans;
class P_u8;
class C_u32;
class C_f32x4;

//-------------------------------------------------------------------------
//
//  Class:   CPixelShaderCompiler
//
//  Synopsis:
//     Pixel shader compiler
//
//-------------------------------------------------------------------------
class CPixelShaderCompiler
{
public:
    static PS_HRESULT Create(
        _In_ void* pCode,
        _In_ unsigned uByteCodeSize,
        _Out_ CPixelShaderCompiler **ppPixelShaderCompiler
        );

    unsigned AddRef();
    unsigned Release();

    GenerateColorsEffect *GetGenerateColorsFunction()
    {
        return m_pfn;
    }

private:
    CPixelShaderCompiler();
    ~CPixelShaderCompiler();

    PS_HRESULT Init(
        _In_ void *pCode,
        _In_ unsigned uByteCodeSize
        );

    PS_HRESULT Compile(
        _Out_ GenerateColorsEffect **ppfn
        );

    PS_HRESULT LoadTextureVariables(_In_ P_u8 *pPixelShaderState);

    PS_HRESULT LoadShaderConstants(_In_ int nChannel, __inout CPixelShaderRegisters *pShaderRegisters);

    PS_HRESULT ComputeEval(
        _In_  const P_u8 *pPixelShaderState,
        _In_  const C_u32 *puX,
        _In_  const C_u32 *puY,
        _Out_ C_f32x4  *pEvalRight,
        _Out_ C_f32x4  *pEvalDeltaRight,
        _Out_ C_f32x4  *pEvalDown,
        _Out_ C_f32x4  *pEvalDeltaDown
        );

    PS_HRESULT CompileInstruction(
        _In_ int i,                                         // channel
        _In_ PSTRINST_BASE_PARAMS* pBaseInstr,              // instruction
        __inout CInstructionVariables *pInstructionVars     // variables used by instruction compiler
        );

    PS_HRESULT CompileDependentInstruction(
        _In_ PSTRINST_BASE_PARAMS* pBaseInstr,              // instruction
        __inout CInstructionVariables *pInstructionVars     // variables used by instruction compiler
        );

    PS_HRESULT PreloadConstant(
        _In_ int i,                                         // channel
        _In_ PSTRINST_BASE_PARAMS* pBaseInstr,              // instruction
        __inout CInstructionVariables *pInstructionVars     // variables used by instruction compiler
        );

private:
    unsigned              m_cRefs;
    RDPSTrans            *m_pTranslated;
    CTextureVariables    *m_pTextureVariables;
    GenerateColorsEffect *m_pfn;
};




