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
#include <debug.h>   // [RWM] DPRINT1 handshake tracing

EXTERN_C HRESULT AvCreateProcessHeap(VOID);
EXTERN_C HRESULT AvDestroyProcessHeap(VOID);
extern "C" void InitDebugLib(
    __in_ecount_opt(1) HANDLE,
    __in_ecount_opt(1) BOOL (WINAPI *)(HANDLE, DWORD, LPVOID),
    BOOL fExe
    );
extern "C" void TermDebugLib(__in_ecount(1) HANDLE, BOOL);

extern "C"
BOOL
__stdcall
DllMain(
    HINSTANCE   dllHandle,
    ULONG       reason,
    __in_ecount(1) CONTEXT* /* context */
    )
{
    //
    // [RWM] The stock body ran AvCreateProcessHeap()/InitDebugLib() on EVERY
    // reason, including THREAD_ATTACH, and never tore either down. Gate both on
    // PROCESS_ATTACH and pair them with the PROCESS_DETACH teardown, 1:1 with
    // what shared/util/DllUtil/dllmainimpl.cxx already does for the other DLLs.
    // AvCreateProcessHeap() is idempotent (see UtilLib/MemUtils.cxx).
    //
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (FAILED(AvCreateProcessHeap()))
            return FALSE;
        InitDebugLib(dllHandle, NULL, TRUE);
        DPRINT1("[RWM] milcore.dll DLL_PROCESS_ATTACH (RWM-built milcore loaded, DPRINT1 channel live)\n");
    }

    BOOL const ok = MILCoreDllMain(dllHandle, reason);

    if (reason == DLL_PROCESS_DETACH)
    {
        TermDebugLib(dllHandle, TRUE);
        AvDestroyProcessHeap();
    }

    return ok;
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
//
// [RWM] AV (WMP/EVR media playback) is disabled in this tree -- core/av is
// stripped, engine.cpp does not call CAVLoader, and api_factory.cpp returns
// E_NOTIMPL from CreateMediaPlayer. DWM needs none of it. These two remain
// because MILCoreDllMain still references them; routed to DPRINT1 so they land
// in the same log as the rest of the [RWM] trace instead of OutputDebugString.
//
HRESULT
AvDllInitialize(
    void
    )
{
    DPRINT1("[RWM] STUB AvDllInitialize (WMP init skipped) -> S_OK\n");
    return S_OK;
}
void
AvDllShutdown(void)
{
    DPRINT1("[RWM] STUB AvDllShutdown called\n");
}
