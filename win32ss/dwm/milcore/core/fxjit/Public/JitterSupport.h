// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Platform dependent support for run time SIMD code generator.
//
//-----------------------------------------------------------------------------
#pragma once

class CProgram;

//+------------------------------------------------------------------------------
//
//  Class:
//      CJitterSupport
//
//  Synopsis:
//      Definitions for platform dependent routines used in jitter.
//      These routines should be implemented by jitter client.
//
//-------------------------------------------------------------------------------
class CJitterSupport
{
public:
    static CProgram* __STDCALL GetCurrentProgram();
    static __checkReturn HRESULT __STDCALL CodeAllocate(_In_ UINT32 cbSize, _Out_ UINT8 **ppAddress);
    static void __STDCALL CodeFree(_In_ void *pAddress);
    static UINT8* __STDCALL MemoryAllocate(_In_ UINT32 cbSize, _Out_ UINT32 & cbActualSize);
    static void __STDCALL MemoryFree(_In_ void *pAddress);
};

