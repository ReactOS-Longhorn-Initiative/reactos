// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      MILCore.dll entry point
//
//

#include <initguid.h>
#include "precomp.hpp"
#include <MemUtils.h>
#include "av/avloader.h" // todo remove

EXTERN_C HRESULT AvCreateProcessHeap(VOID);
extern "C" void InitDebugLib(
    __in_ecount_opt(1) HANDLE, 
    __in_ecount_opt(1) BOOL (WINAPI *)(HANDLE, DWORD, LPVOID), 
    BOOL fExe
    );
extern "C"
BOOL
__stdcall
DllMain(
    HINSTANCE   dllHandle,
    ULONG       reason,
    __in_ecount(1) CONTEXT* /* context */
    )
{
    AvCreateProcessHeap();
    InitDebugLib(dllHandle, NULL, TRUE);
    return MILCoreDllMain(
        dllHandle,
        reason
        );
}

BOOL g_fNoMeterChecks;

/* Stubs */
bool WPFUtils::OSVersionHelper::IsWindows8OrGreater()
{
    return false;
}
bool WPFUtils::OSVersionHelper::IsWindowsVistaOrGreater()
{
    return true;
}
bool WPFUtils::OSVersionHelper::IsWindows7OrGreater()
{
    return false;
}

/* TODO: does AV lib depend on WMP headers? */
HRESULT
AvDllInitialize(
    void
    )
{
    OutputDebugStringW(L"WARNING: dllentry.cpp attempted to initialize WMP (AvDllInitialize)\n");
    return S_OK;
}
void
AvDllShutdown(void)
{
    OutputDebugStringW(L"WARNING: stub AvDllShutdown called\n");
}

HRESULT CAVLoader::Startup()
{
    OutputDebugStringW(L"WARNING: stub CAVLoader::Startup called\n");
    return S_OK;
}

void CAVLoader::Shutdown()
{
    OutputDebugStringW(L"WARNING: stub CAVLoader::Shutdown called\n");
}

HRESULT
CMILAV::
CreateMedia(
    _In_        CEventProxy *pEventProxy,
    _In_        bool        canOpenAnyMedia,
    __deref_out IMILMedia   **ppMedia
    )
{
    *ppMedia = nullptr;
    OutputDebugStringW(L"WARNING: stub CMILAV::CreateMedia called\n");
    return E_NOTIMPL;
}