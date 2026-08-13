// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//     MIL core flat API set.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
#include "osversionhelper.h"
#include "transport.h"

#include <vector>
#include <debug.h>   // [RWM] DPRINT1 handshake tracing

// [RWM] Unbounded entry/exit tracer. An RAII object logs ENTER on construction and
// EXIT (with the final hr, read by reference) on destruction -- so a single
// RWM_TRACE() at the top of a function brackets it completely, on EVERY return path.
// Deliberately unbounded: flooding the boot log is preferred over missing the last
// export call before an abort. Use RWM_TRACE_NOHR() for functions without an `hr`.
struct RwmScopeTrace
{
    const char    *m_name;
    const HRESULT *m_phr;
    RwmScopeTrace(const char *name, const HRESULT *phr) : m_name(name), m_phr(phr)
    {
        DPRINT1("[RWM] >>> %s ENTER\n", m_name);
    }
    ~RwmScopeTrace()
    {
        if (m_phr) DPRINT1("[RWM] <<< %s EXIT hr=0x%08lx\n", m_name, (unsigned long)*m_phr);
        else       DPRINT1("[RWM] <<< %s EXIT\n", m_name);
    }
};
#define RWM_TRACE()      RwmScopeTrace _rwmScopeTrace(__FUNCTION__, &hr)
#define RWM_TRACE_NOHR() RwmScopeTrace _rwmScopeTrace(__FUNCTION__, nullptr)

extern void DumpInstrumentationData();

ExternTag(tagMILConnection);
ExternTag(tagMILTransport);

UINT g_uMilPerfInstrumentationFlags = 0;

namespace
{
    using PFNDWMGETTRANSPORTATTRIBUTES = HRESULT (WINAPI *)(BOOL *, BOOL *, DWORD *);

    HRESULT GetTransportAttributesInternal(_Out_ BOOL *pfIsRemoting, _Out_ BOOL *pfIsConnected, _Out_ DWORD *pdwGeneration)
    {
        if (!pfIsRemoting || !pfIsConnected || !pdwGeneration)
        {
            TraceTag((tagMILTransport, "GetTransportAttributesInternal: invalid argument"));
            return E_INVALIDARG;
        }

        *pfIsRemoting = FALSE;
        *pfIsConnected = TRUE;
        *pdwGeneration = 0;

        TraceTag((tagMILTransport, "GetTransportAttributesInternal: querying DWM transport attributes"));

        if (!DWMAPI::CheckOS())
        {
            TraceTag((tagMILTransport, "GetTransportAttributesInternal: unsupported OS"));
            return E_INVALIDARG;
        }

        HRESULT hr = DWMAPI::Load();
        if (FAILED(hr))
        {
            TraceTag((tagMILTransport, "GetTransportAttributesInternal: failed to load dwmapi.dll hr=0x%08x", hr));
            return hr;
        }

        PFNDWMGETTRANSPORTATTRIBUTES pfnGetTransportAttributes =
            reinterpret_cast<PFNDWMGETTRANSPORTATTRIBUTES>(DWMAPI::GetProcAddress("DwmGetTransportAttributes"));

        if (!pfnGetTransportAttributes)
        {
            DWORD dwError = GetLastError();
            if (dwError != ERROR_SUCCESS)
            {
                TraceTag((tagMILTransport, "GetTransportAttributesInternal: DwmGetTransportAttributes unavailable error=%lu", dwError));
                return HRESULT_FROM_WIN32(dwError);
            }

            TraceTag((tagMILTransport, "GetTransportAttributesInternal: DwmGetTransportAttributes returned null without error"));
            return E_FAIL;
        }

        HRESULT hrAttributes = pfnGetTransportAttributes(pfIsRemoting, pfIsConnected, pdwGeneration);
        if (SUCCEEDED(hrAttributes))
        {
            TraceTag((tagMILTransport,
                      "GetTransportAttributesInternal: success remoting=%d connected=%d generation=%lu",
                      *pfIsRemoting,
                      *pfIsConnected,
                      *pdwGeneration));
        }
        else
        {
            TraceTag((tagMILTransport,
                      "GetTransportAttributesInternal: call failed hr=0x%08x",
                      hrAttributes));
        }

        return hrAttributes;
    }

    HRESULT EnumerateGraphicsStreamClients(_Out_ std::vector<GUID> &clients)
    {
        clients.clear();

        HRESULT hr = S_OK;
        GUID clientId = {};

        TraceTag((tagMILTransport, "EnumerateGraphicsStreamClients: begin"));

        for (UINT index = 0;; ++index)
        {
            hr = GetGraphicsStreamClient(index, &clientId);

            if (hr == E_INVALIDARG ||
                hr == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND) ||
                hr == HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED))
            {
                TraceTag((tagMILTransport,
                          "EnumerateGraphicsStreamClients: terminating enumeration hr=0x%08x",
                          hr));
                hr = S_OK;
                break;
            }

            if (FAILED(hr))
            {
                TraceTag((tagMILTransport,
                          "EnumerateGraphicsStreamClients: GetGraphicsStreamClient failed index=%u hr=0x%08x",
                          index,
                          hr));
                clients.clear();
                break;
            }

            WCHAR wszClientGuid[64] = {0};
            if (StringFromGUID2(clientId, wszClientGuid, _countof(wszClientGuid)) > 0)
            {
                TraceTag((tagMILTransport,
                          "EnumerateGraphicsStreamClients: found client index=%u guid=%S",
                          index,
                          wszClientGuid));
            }
            else
            {
                TraceTag((tagMILTransport,
                          "EnumerateGraphicsStreamClients: found client index=%u guid conversion failed",
                          index));
            }

            clients.push_back(clientId);
        }

        TraceTag((tagMILTransport,
                  "EnumerateGraphicsStreamClients: completed with %u clients hr=0x%08x",
                  static_cast<UINT>(clients.size()),
                  hr));

        return hr;
    }

    void ApplyDefaultTransportSettings(_Inout_ MIL_TRANSPORT_PARAMETERS *pParameters)
    {
        if (!pParameters)
        {
            TraceTag((tagMILTransport, "ApplyDefaultTransportSettings: null parameters"));
            return;
        }

        if (pParameters->TransportType == 0)
        {
            TraceTag((tagMILTransport, "ApplyDefaultTransportSettings: applying MIL_TRANSPORT_TYPE_DEFAULT"));
            pParameters->TransportType = MIL_TRANSPORT_TYPE_DEFAULT;
        }

        if (pParameters->TransportGuidCount != 0)
        {
            pParameters->TransportFlags |= MIL_TRANSPORT_FLAG_HAS_REMOTED_CLIENT;
            TraceTag((tagMILTransport,
                      "ApplyDefaultTransportSettings: remote clients detected count=%u flags=0x%08x",
                      pParameters->TransportGuidCount,
                      pParameters->TransportFlags));
        }
        else
        {
            TraceTag((tagMILTransport, "ApplyDefaultTransportSettings: no remote clients"));
        }
    }
}

extern "C"
{

//+-----------------------------------------------------------------------------
//
//    Function:
//        MilVersionCheck
//
//    Synopsis:
//        Provides means for the caller to verify that this binary has been
//        built with exactly the same SDK version the caller is using.
//
//    Returns:
//        S_OK if milcore.dll was built with the specified MIL SDK version.
//        WGXERR_UNSUPPORTEDVERSION otherwise.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilVersionCheck(
    UINT uiCallerMilSdkVersion
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();

    if (uiCallerMilSdkVersion != MIL_SDK_VERSION)
    {
        TraceTag((tagMILWarning,
                  "MilVersionCheck: binary version mismatch (caller: 0x%08x, callee: 0x%08x), abort operation.",
                  uiCallerMilSdkVersion,
                  MIL_SDK_VERSION
                  ));

        IFC(WGXERR_UNSUPPORTEDVERSION);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        MilCompositionEngine_InitializePartitionManager
//
//    Synopsis:
//        Creates and initializes a partition manager. This will result in
//        creation of infrastructure necessary to perform composition in the
//        current process. Among other, a scheduler and a set of worker threads
//        will be created.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilCompositionEngine_InitializePartitionManager(
    int nPartitionMode,
    int nScheduleType
    )
{
    DPRINT1("[RWM] MilCompositionEngine_InitializePartitionManager(mode=%d, schedule=%d) ENTER\n",
            nPartitionMode, nScheduleType);
    //
    // [RWM] Vista SP1 dwmredir calls this with TWO stdcall args:
    //   MilCompositionEngine_InitializePartitionManager(0, MIL_SCHEDULE_UNTHROTTLED)
    // i.e. (int, MIL_SCHEDULE_TYPE) — see dwmredir.dll.c:4263. The original WPF
    // export took a single arg; that arg-count mismatch leaves 4 bytes on the
    // stack on every dwmredir->milcore call (stdcall callee-cleanup), corrupting
    // the handshake before any batch can flow. Accept both args; the schedule
    // type is the scheduler-relevant one.
    //
    UNREFERENCED_PARAMETER(nPartitionMode);
    HRESULT hr = EnsurePartitionManager(nScheduleType);
    DPRINT1("[RWM] MilCompositionEngine_InitializePartitionManager EXIT hr=0x%08lx\n", hr);
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Function:
//        MilCompositionEngine_UpdateSchedulerSettings
//
//    Synopsis:
//        Asks the partition manager to change scheduler settings.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilCompositionEngine_UpdateSchedulerSettings(
    int nPriority,
    int nScheduleType
    )
{
    //
    // [RWM] Vista dwm.exe calls this with TWO stdcall args:
    //   MilCompositionEngine_UpdateSchedulerSettings(DwmGetThreadPriority(),
    //       dwCheck ? MIL_SCHEDULE_ADAPTIVE : MIL_SCHEDULE_THROTTLED)   (dwm.exe.c:7854)
    // i.e. (int priority, MIL_SCHEDULE_TYPE). The WPF export took a single arg; that
    // arg-count mismatch corrupts the stack on the dwm->milcore stdcall return
    // (4-byte imbalance), which surfaced as a c0000005/0x8000ffff in dwm.exe's very
    // next call (StartRedirection's HR instrumentation). Same class of bug as
    // MilCompositionEngine_InitializePartitionManager. arg1 (priority) drives the
    // worker thread; the schedule type is accepted (WPF scheduler is priority-based).
    //
    DPRINT1("[RWM] MilCompositionEngine_UpdateSchedulerSettings(priority=%d, schedule=%d)\n",
            nPriority, nScheduleType);
    UNREFERENCED_PARAMETER(nScheduleType);
    RRETURN(UpdateSchedulerSettings(nPriority));
}


//+-----------------------------------------------------------------------------
//
//    Function:
//        MilCompositionEngine_DeinitializePartitionManager
//
//    Synopsis:
//        Releases the partition manager and all the relevant infrastructure.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilCompositionEngine_DeinitializePartitionManager()
{
    RWM_TRACE_NOHR();
    ReleasePartitionManager();

    return S_OK;
}

//+-----------------------------------------------------------------------
//
//    Function:
//        MilTransport_AddRef
//
//    Synopsis:
//        Icrement reference to the connection.
//------------------------------------------------------------------------

EXTERN_C
ULONG
WINAPI
MilTransport_AddRef(
    _In_opt_ HMIL_CONNECTION hConnection
    )
{
    return AddRefConnectionHandle(hConnection);
}

EXTERN_C
ULONG
WINAPI
MilTransport_Release(
    _In_opt_ HMIL_CONNECTION hConnection
    )
{
    return ReleaseConnectionHandle(hConnection);
}

EXTERN_C
HRESULT
WINAPI
MilTransport_Create(CMilConnectionManager *pConnectionManager,
                    PVOID TransportParams,
                    UINT32 Boolean,
                    HMIL_CONNECTION *phConnection)
{
    HRESULT hr = S_OK;
    CMilConnection* pConnection = NULL;

    DPRINT1("[RWM] MilTransport_Create(connMgr=%p, params=%p, bool=%u) ENTER\n",
            pConnectionManager, TransportParams, Boolean);

    UNREFERENCED_PARAMETER(pConnectionManager);
    UNREFERENCED_PARAMETER(TransportParams);
    UNREFERENCED_PARAMETER(Boolean);

    CHECKPTRARG(phConnection);

    // [RWM] NOTE: forces SameThread marshaling. DWM may need CrossThread; revisit.
    IFC(CMilConnection::Create(
        MilMarshalType::SameThread ,
        OUT &pConnection));

    HMIL_CONNECTION hNewConnection = PointerToHandle(pConnection);
    IFCOOM(hNewConnection);

    *phConnection = hNewConnection;
    pConnection = NULL;

Cleanup:
    ReleaseInterface(pConnection);

    DPRINT1("[RWM] MilTransport_Create EXIT hr=0x%08lx hConnection=%p\n",
            hr, (phConnection && SUCCEEDED(hr)) ? (void*)*phConnection : NULL);
    RRETURN(hr);
}

HRESULT
WINAPI
MilTransport_CreateFromPacketTransport(CMilConnectionManager *pConnectionManager,
                                       PVOID TransportParams,
                                       HMIL_CONNECTION *phConnection)
{
    HRESULT hr = S_OK;
    CMilConnection* pConnection = NULL;

    UNREFERENCED_PARAMETER(pConnectionManager);
    UNREFERENCED_PARAMETER(TransportParams);

    CHECKPTRARG(phConnection);

    IFC(CMilConnection::Create(
        MilMarshalType::CrossThread,
        OUT &pConnection));

    HMIL_CONNECTION hNewConnection = PointerToHandle(pConnection);
    IFCOOM(hNewConnection);

    *phConnection = hNewConnection;
    pConnection = NULL;

Cleanup:
    ReleaseInterface(pConnection);
    
    RRETURN(hr);
}

HRESULT
WINAPI
MilTransport_CreateSurfaceManager(_Outptr_ IMilRedirectedGDISurfaceManager **ppSurfaceManager)
{
    DPRINT1("[RWM] MilTransport_CreateSurfaceManager ENTER\n");
    TraceTag((tagMILTransport, "MilTransport_CreateSurfaceManager: create request"));

    HRESULT hr = CMilSurfaceManager::Create(ppSurfaceManager);
    if (SUCCEEDED(hr))
    {
        TraceTag((tagMILTransport, "MilTransport_CreateSurfaceManager: success manager=%p", *ppSurfaceManager));
    }
    else
    {
        TraceTag((tagMILTransport, "MilTransport_CreateSurfaceManager: failed hr=0x%08x", hr));
    }

    DPRINT1("[RWM] MilTransport_CreateSurfaceManager EXIT hr=0x%08lx manager=%p\n",
            hr, SUCCEEDED(hr) ? (void*)*ppSurfaceManager : NULL);
    return hr;
}

HRESULT
WINAPI
MilTransport_CreateTransportParameters(
    BOOL fRequestSynchronousTransport,
    _Out_ INT *pDefaultTransport,
    _Out_ UINT *pTransportGeneration,
    _Outptr_result_bytebuffer_(*pcbParameters) MIL_TRANSPORT_PARAMETERS **ppParameters,
    _Out_ UINT *pcbParameters)
{
    HRESULT hr = S_OK;
    BOOL fIsRemoting = FALSE;
    BOOL fIsConnected = TRUE;
    DWORD dwGeneration = 0;
    std::vector<GUID> streamClients;
    MIL_TRANSPORT_PARAMETERS *pAllocatedParameters = NULL;
    size_t cbParameters = sizeof(MIL_TRANSPORT_PARAMETERS);

    TraceTag((tagMILTransport,
              "MilTransport_CreateTransportParameters: request synchronous=%d",
              fRequestSynchronousTransport));

    CHECKPTRARG(pDefaultTransport);
    CHECKPTRARG(pTransportGeneration);
    CHECKPTRARG(ppParameters);
    CHECKPTRARG(pcbParameters);

    *pDefaultTransport = 0;
    *pTransportGeneration = 0;
    *ppParameters = NULL;
    *pcbParameters = 0;

    hr = GetTransportAttributesInternal(&fIsRemoting, &fIsConnected, &dwGeneration);
    if (hr == E_INVALIDARG ||
        hr == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND) ||
        hr == HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED))
    {
        TraceTag((tagMILTransport,
                  "MilTransport_CreateTransportParameters: using default attributes hr=0x%08x",
                  hr));
        hr = S_OK;
        fIsRemoting = FALSE;
        fIsConnected = TRUE;
        dwGeneration = 0;
    }
    IFC(hr);

    if (!fRequestSynchronousTransport)
    {
        hr = EnumerateGraphicsStreamClients(streamClients);
        if (hr == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND) ||
            hr == HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED))
        {
            TraceTag((tagMILTransport,
                      "MilTransport_CreateTransportParameters: stream clients unavailable hr=0x%08x",
                      hr));
            hr = S_OK;
            streamClients.clear();
        }
        else if (hr == E_INVALIDARG)
        {
            TraceTag((tagMILTransport,
                      "MilTransport_CreateTransportParameters: stream client enumeration returned invalid arg"));
            hr = S_OK;
        }
        IFC(hr);
    }

    UINT guidCount = static_cast<UINT>(streamClients.size());
    if (guidCount > 0)
    {
        cbParameters += static_cast<size_t>(guidCount - 1) * sizeof(GUID);
    }

    IFC(HrAlloc(0, cbParameters, reinterpret_cast<void **>(&pAllocatedParameters)));

    RtlZeroMemory(pAllocatedParameters, cbParameters);

    pAllocatedParameters->TransportGeneration = dwGeneration;
    pAllocatedParameters->TransportIsConnected = fIsConnected ? 1u : 0u;
    pAllocatedParameters->TransportGuidCount = guidCount;

    if (!fRequestSynchronousTransport && fIsRemoting)
    {
        pAllocatedParameters->TransportType = MIL_TRANSPORT_TYPE_REMOTE;
    }

    ApplyDefaultTransportSettings(pAllocatedParameters);

    if (guidCount > 0)
    {
        for (UINT i = 0; i < guidCount; ++i)
        {
            pAllocatedParameters->TransportGuidList[i] = streamClients[i];
        }
    }

    *pDefaultTransport = static_cast<INT>(pAllocatedParameters->TransportIsConnected);
    *pTransportGeneration = pAllocatedParameters->TransportGeneration;
    *ppParameters = pAllocatedParameters;
    *pcbParameters = static_cast<UINT>(cbParameters);
    TraceTag((tagMILTransport,
              "MilTransport_CreateTransportParameters: success default=%d generation=%u type=%u flags=0x%08x guidCount=%u size=%u",
              *pDefaultTransport,
              *pTransportGeneration,
              (*ppParameters)->TransportType,
              (*ppParameters)->TransportFlags,
              (*ppParameters)->TransportGuidCount,
              *pcbParameters));
    pAllocatedParameters = NULL;

Cleanup:
    if (FAILED(hr))
    {
        if (pAllocatedParameters)
        {
            FreeHeap(pAllocatedParameters);
        }

        TraceTag((tagMILTransport,
                  "MilTransport_CreateTransportParameters: failure hr=0x%08x",
                  hr));
    }

    RRETURN(hr);
}

HRESULT
WINAPI
MilTransport_DisconnectTransport(HMIL_CONNECTION hConnection)
{
    HRESULT hr = S_OK;
    RWM_TRACE();

    CHECKPTRARG(hConnection);

    hr = ReleaseConnectionHandle(hConnection);
    if (hr != 0)
    {
        TraceTag((tagMILTransport,
                  "MilTransport_DisconnectTransport: failed hr=0x%08x",
                  hr));
    }

Cleanup:
    RRETURN(hr);
}

EXTERN_C
HRESULT
WINAPI
MilTransport_InitializeConnectionManager(
    _In_opt_ IMilRedirectedGDISurfaceManager *pSurfaceManager,
    _Outptr_ CMilConnectionManager **ppConnectionManager)
{
    DPRINT1("[RWM] MilTransport_InitializeConnectionManager(surfaceMgr=%p) ENTER\n",
            pSurfaceManager);
    TraceTag((tagMILTransport,
              "MilTransport_InitializeConnectionManager: surfaceManager=%p",
              pSurfaceManager));

    HRESULT hr = CMilConnectionManager::Create(pSurfaceManager, ppConnectionManager);
    if (SUCCEEDED(hr))
    {
        TraceTag((tagMILTransport,
                  "MilTransport_InitializeConnectionManager: success connectionManager=%p",
                  *ppConnectionManager));
    }
    else
    {
        TraceTag((tagMILTransport,
                  "MilTransport_InitializeConnectionManager: failed hr=0x%08x",
                  hr));
    }

    DPRINT1("[RWM] MilTransport_InitializeConnectionManager EXIT hr=0x%08lx connMgr=%p\n",
            hr, SUCCEEDED(hr) ? (void*)*ppConnectionManager : NULL);
    return hr;
}

HRESULT
WINAPI
MilTransport_ShutDownConnectionManager(CMilConnectionManager *pConnectionManager)
{
    TraceTag((tagMILTransport,
              "MilTransport_ShutDownConnectionManager: connectionManager=%p",
              pConnectionManager));

    if (pConnectionManager)
    {
        pConnectionManager->Release();
        TraceTag((tagMILTransport, "MilTransport_ShutDownConnectionManager: released"));
    }

    return S_OK;
}


//+-----------------------------------------------------------------------
//
//    Function:
//        WgxConnection_SameThreadPresent
//
//    Synopsis:
//        Presents on the same thread sync compositor.
//------------------------------------------------------------------------


HRESULT WINAPI WgxConnection_SameThreadPresent(
    _In_ HMIL_CONNECTION hConnection
    )
{
    HRESULT hr = S_OK;
    CMilConnection* pConnection = HandleToPointer(hConnection);
    CHECKPTRARG(pConnection);
    IFC(pConnection->PresentAllPartitions());

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//    Function:
//        WgxConnection_ShouldForceSoftwareForGraphicsStreamClient
//
//------------------------------------------------------------------------

BOOL WINAPI 
WgxConnection_ShouldForceSoftwareForGraphicsStreamClient()
{
    BOOL fForceSoftwareForGraphicsStreamClient = FALSE;
    
    //
    // Discover graphics stream clients, but only on Vista. On OS < Vista, graphics
    // stream clients were not available. On OS > Vista, we don't want to force sw if a graphics 
    // stream client is present since the magnifier on OS > Vista will support magnifying DX content.
    //

    if (WPFUtils::OSVersionHelper::IsWindowsVistaOrGreater() && 
        !WPFUtils::OSVersionHelper::IsWindows7OrGreater())
    {
        UUID uuid;
        HRESULT hrEnumerate = GetGraphicsStreamClient(0 /* Only looking if there are any at all */, &uuid);

        if (hrEnumerate == S_OK)
        {
            fForceSoftwareForGraphicsStreamClient = TRUE;
        }
    }

    return fForceSoftwareForGraphicsStreamClient;
}


//+-----------------------------------------------------------------------------
//
//    Function:
//        WgxConnection_Create
//
//    Synopsis:
//        Creates a client transport object.
//
//------------------------------------------------------------------------------

HRESULT WINAPI WgxConnection_Create(
    bool requestSynchronousTransport,
    HMIL_CONNECTION *phConnection
    )
{
    HRESULT hr = S_OK;
    CMilConnection *pConnection = NULL;
    CHECKPTRARG(phConnection);

    
    IFC(CMilConnection::Create(
        requestSynchronousTransport ? MilMarshalType::SameThread : MilMarshalType::CrossThread,
        OUT &pConnection));

    HMIL_CONNECTION hNewConnection = PointerToHandle(pConnection);
    IFCOOM(hNewConnection);

    *phConnection = hNewConnection;
    pConnection = NULL;

Cleanup:
    ReleaseInterface(pConnection);
    
    RRETURN(hr);
}




HRESULT WINAPI WgxConnection_Disconnect(
    HMIL_CONNECTION hConnection
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(hConnection);

    ReleaseConnectionHandle(hConnection);

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilConnection_CreateChannel(
    HMIL_CONNECTION hConnection,
    MIL_CHANNEL hSourceChannel,
    MIL_CHANNEL *phChannel
    )
{
    HRESULT hr = S_OK;
    HMIL_CHANNEL hPartSource = NULL;
    CMilConnection *pConnection = NULL;
    CMilChannel *pChannel = NULL;
    const CMilChannel *pSourceChannel = HandleToPointer(hSourceChannel);

    CHECKPTRARG(phChannel);
    CHECKPTRARG(hConnection);

    DPRINT1("[RWM] MilConnection_CreateChannel(hConn=%p, hSrc=%p) ENTER\n",
            (void*)hConnection, (void*)hSourceChannel);

    pConnection = HandleToPointer(hConnection);
    if (!pConnection)
    {
        IFC(E_HANDLE);
    }

    if (pSourceChannel)
    {
        hPartSource = pSourceChannel->GetChannel();
    }

    IFC(pConnection->CreateChannel(hPartSource, &pChannel));

    *phChannel = PointerToHandle(pChannel);

    EventWriteCreateChannel(pChannel, pChannel->GetChannel());

Cleanup:
    DPRINT1("[RWM] MilConnection_CreateChannel EXIT hr=0x%08lx hChannel=%p\n",
            hr, (phChannel && SUCCEEDED(hr)) ? (void*)*phChannel : NULL);
    RRETURN(hr);
}

HRESULT WINAPI MilConnection_DestroyChannel(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->Destroy());

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilChannel_CloseBatch(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->CloseBatch());

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilChannel_CommitChannel(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->Commit());

Cleanup:
    {
        static LONG s_cCommit = 0;
        if (s_cCommit < 64)
        {
            InterlockedIncrement(&s_cCommit);
            DPRINT1("[RWM] MilChannel_CommitChannel(hChannel=%p) hr=0x%08lx [#%ld]\n",
                    (void*)hChannel, hr, s_cCommit);
        }
    }
    RRETURN(hr);
}


 
HRESULT WINAPI MilComposition_SyncFlush(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->SyncFlush());

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilComposition_PeekNextMessage(
    _In_ MIL_CHANNEL hChannel,
    __out_bcount_part(cbSize, sizeof(MIL_MESSAGE)) MIL_MESSAGE *pmsg,
    _In_ size_t cbSize,
    __out_ecount(1) BOOL *pfMessageRetrieved
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(pmsg);
    CHECKPTRARG(pfMessageRetrieved);

    hr = pChannel->PeekNextMessage(pmsg, cbSize, pfMessageRetrieved);
    if (pfMessageRetrieved && *pfMessageRetrieved && pmsg)
    {
        const DWORD *d = reinterpret_cast<const DWORD*>(pmsg);
        DPRINT1("[RWM] PeekNextMessage GOT msg type=0x%X payload=%08lX %08lX %08lX %08lX %08lX (hr=0x%08lx)\n",
                (UINT)pmsg->type, d[0], d[2], d[3], d[4], d[5], hr);
    }
    else
    {
        DPRINT1("[RWM] PeekNextMessage NO message retrieved (hr=0x%08lx)\n", hr);
    }
    IFC(hr);

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilComposition_WaitForNextMessage(
    MIL_CHANNEL hChannel,
    DWORD nCount,
    const HANDLE *pHandles,
    BOOL bWaitAll,
    DWORD waitTimeout,
    DWORD *pWaitReturn
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    if (pWaitReturn == NULL)
    {
        IFC(E_INVALIDARG);
    }

    if (nCount > 0 && pHandles == NULL)
    {
        IFC(E_INVALIDARG);
    }

    if (nCount > (MAXIMUM_WAIT_OBJECTS - 1))
    {
        IFC(E_INVALIDARG);
    }

    hr = pChannel->WaitForNextMessage(
        nCount,
        pHandles,
        bWaitAll,
        waitTimeout,
        pWaitReturn
        );
    {
        static LONG c=0;
        if (c < 32) { InterlockedIncrement(&c);
            DPRINT1("[RWM] MilComposition_WaitForNextMessage(nCount=%lu timeout=%lu) hr=0x%08lx waitRet=%lu [#%ld]\n",
                    nCount, waitTimeout, hr, pWaitReturn ? *pWaitReturn : 0xFFFFFFFF, c); }
    }
    IFC(hr);

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_CreateOrAddRefOnChannel(
    MIL_CHANNEL hChannel,
    MIL_RESOURCE_TYPE type,
    __inout_ecount(1) HMIL_RESOURCE *ph
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(ph);

    IFC(pChannel->CreateOrAddRefOnChannel(type, ph));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilResource_DuplicateHandle(
    __in_ecount(1) MIL_CHANNEL hSourceChannel,
    HMIL_RESOURCE hOriginal,
    __in_ecount(1) MIL_CHANNEL hTargetChannel,
    __out_ecount(1) HMIL_RESOURCE* phDuplicate
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pSourceChannel = HandleToPointer(hSourceChannel);
    CMilChannel *pTargetChannel = HandleToPointer(hTargetChannel);

    CHECKPTRARG(pSourceChannel);
    CHECKPTRARG(pTargetChannel);
    CHECKPTRARG(phDuplicate);

    IFC(pSourceChannel->DuplicateHandle(hOriginal, pTargetChannel, phDuplicate));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_ReleaseOnChannel(
    MIL_CHANNEL hChannel,
    HMIL_RESOURCE h,
    __out_ecount_opt(1) BOOL *pfDeleted
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(h);

    IFC(pChannel->ReleaseOnChannel(h, pfDeleted));

Cleanup:
    RRETURN(hr);
}

EXTERN_C
HRESULT WINAPI
MilResource_GetRefCountOnChannel(
    MIL_CHANNEL hChannel,
    HMIL_RESOURCE h,
    __out_ecount_opt(1) UINT *pcRefs
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(h);

    IFC(pChannel->GetRefCount(h, pcRefs));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilChannel_SetReceiveBroadcastMessages(
    MIL_CHANNEL hChannel,
    bool fReceivesBroadcast
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    pChannel->SetReceiveBroadcastMessages(fReceivesBroadcast);

Cleanup:
    RRETURN(hr);
}


HRESULT WINAPI
MilChannel_GetMarshalType(
    MIL_CHANNEL hChannel,
    __out_ecount(1) MilMarshalType::Enum *pMarshalType
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    const CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(pMarshalType);

    *pMarshalType = pChannel->GetMarshalType();

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_SendCommand(
    __in_bcount(cbSize) VOID *pvCommandData,
    UINT32 cbSize,
   // bool sendInSeparateBatch, Not In Vista RTM?
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    {
        static LONG s_cSend = 0;
        if (s_cSend < 256)
        {
            InterlockedIncrement(&s_cSend);
            UINT32 cmdType = (pvCommandData && cbSize >= sizeof(UINT32))
                                 ? *reinterpret_cast<UINT32*>(pvCommandData) : 0xFFFFFFFF;
            DPRINT1("[RWM] MilResource_SendCommand(hChannel=%p) Type=%u (0x%02X) cbSize=%u [#%ld]\n",
                    (void*)hChannel, cmdType, cmdType, cbSize, s_cSend);
        }
    }

    if (!pvCommandData && cbSize > 0)
    {
        IFC(E_INVALIDARG);
    }

    CHECKPTRARG(pChannel);

    //
    // CURRENT BATCH, NOT A SEPARATE ONE.
    //
    // This used to pass TRUE, and the commented-out parameter above already
    // suspected why that was wrong: Vista's MilResource_SendCommand has no
    // batch argument at all, so it cannot be choosing a separate batch.
    //
    // The consequence was an ordering inversion. CreateOrAddRefOnChannel puts
    // its MilCmdChannelCreateResource in the OPEN batch
    // (CMilMasterHandleTable::CreateOrAddRefOnChannel -> SendCommand with the
    // default false), while a separate batch is CLOSED immediately and queued
    // on m_pClosedBatches -- which Commit drains first. So every
    // resource-update command overtook the creation of the resource it
    // addressed:
    //
    //     MilResource_CreateOrAddRefOnChannel   -> open batch  (handle 5)
    //     MilResource_SendCommand(151)          -> closed batch, jumps ahead
    //     Commit                                -> batch of 48 bytes, cmd 151
    //     "Invalid resource handle."            generated_process_message.inl
    //
    // uDWM's whole environment-map graph failed this way: eight creates and
    // eight sends produced a 48-byte batch holding one command.
    //
    IFC(pChannel->SendCommand(pvCommandData, cbSize, false));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilChannel_BeginCommand(
    MIL_CHANNEL hChannel,
    __in_bcount(cbCmd) VOID *pCmd,
    UINT32 cbCmd,
    UINT32 cbExtra
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    if (!pCmd || cbCmd < sizeof(MILCMD))
    {
        IFC(E_INVALIDARG);
    }

    IFC(pChannel->BeginCommand(pCmd, cbCmd, cbExtra));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilChannel_AppendCommandData(
    MIL_CHANNEL hChannel,
    __in_bcount(cbSize) VOID *pvData,
    UINT32 cbSize
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    if (!pvData && cbSize > 0)
    {
        IFC(E_INVALIDARG);
    }

    IFC(pChannel->AppendCommandData(pvData, cbSize));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilChannel_EndCommand(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->EndCommand());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Function: MilChannel_SendSyncCommand
//
//    [RWM] Vista milcore export that dwmredir imports (milcore.dll.c:168809).
//    dwmredir calls it ONLY on the synchronous capture-bits path
//    (DuceHelper::CompositionTarget_CaptureBits, dwmredir.dll.c:2898) — never
//    during the boot handshake — but milcore.dll must export it or dwmredir
//    fails to bind. Vista: validate args, SendCommand, SyncFlush, then copy back
//    a <=60-byte reply from a per-channel scratch. We wire the send+flush
//    faithfully; the reply round-trip is not implemented yet, so we return a
//    zeroed MIL_MESSAGE. Capture-bits will fail gracefully (dwmredir checks the
//    reply type) until the reply path lands.
//
//------------------------------------------------------------------------------
EXTERN_C HRESULT WINAPI
MilChannel_SendSyncCommand(
    __in_bcount(cbCommand) VOID *pvCommand,
    UINT32 cbCommand,
    MIL_CHANNEL hChannel,
    __out_bcount_opt(cbReply) MIL_MESSAGE *pReply,
    UINT32 cbReply
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    DPRINT1("[RWM] MilChannel_SendSyncCommand(hChannel=%p, cbCommand=%u, cbReply=%u) ENTER\n",
            (void*)hChannel, cbCommand, cbReply);

    CHECKPTRARG(pvCommand);
    CHECKPTRARG(pChannel);

    IFC(pChannel->SendCommand(pvCommand, cbCommand, TRUE));
    IFC(pChannel->SyncFlush());

    if (pReply)
    {
        UINT32 cb = (cbReply < sizeof(MIL_MESSAGE)) ? cbReply : (UINT32)sizeof(MIL_MESSAGE);
        ZeroMemory(pReply, cb);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Function: MilCoreClientIsDwm
//
//    [RWM] Vista milcore export queried to determine whether the milcore client
//    process is the Desktop Window Manager. dwmredir imports it as a normal
//    (non-delay) import, so milcore.dll fails to load without it. For the RWM
//    stack milcore IS hosting DWM, so this returns TRUE.
//    (Vista milcore.dll.c export at RVA 74208571.)
//
//------------------------------------------------------------------------------
EXTERN_C int WINAPI
MilCoreClientIsDwm(void)
{
    DPRINT1("[RWM] MilCoreClientIsDwm queried -> TRUE\n");
    return TRUE;
}

//+-----------------------------------------------------------------------------
//
//    Function: MilCompositionEngine_GetFeedbackReader
//
//    [RWM] Vista milcore export. Real signature (decoded from dwmredir's mangled
//    import) is int __stdcall(CMrowReader<_DWM_TIMING_INFO>** ppReader): fills an
//    out-param with a lock-free reader over the shared DWM timing-info ring.
//    dwmredir uses it only on the DwmGetCompositionTimingInfo path and CHECKS the
//    HRESULT before dereferencing the reader (windowmanager.cpp:1952), so
//    returning a failure here is safe — timing-info queries fail gracefully and
//    boot is unaffected. The real MROW reader (writer = the composition scheduler
//    publishing per-frame timing) is deferred.
//
//------------------------------------------------------------------------------
EXTERN_C HRESULT WINAPI
MilCompositionEngine_GetFeedbackReader(
    __deref_out_opt void **ppReader
    )
{
    DPRINT1("[RWM] MilCompositionEngine_GetFeedbackReader(ppReader=%p) -> E_NOTIMPL (deferred)\n",
            (void*)ppReader);
    if (ppReader)
    {
        *ppReader = NULL;
    }
    return E_NOTIMPL;
}

/*++

Routine Description:

    MilResource_SendCommandMedia -

        This method sends a command from the managed MediaPlayer object to the
        associated slave media resource refreshing its media content.

        handle - Handle to the slave media resource
        pIMedia - Interface for accessing media content
        pBatch - Records commands to the slave media resource

--*/
HRESULT WINAPI
MilResource_SendCommandMedia(
    HMIL_RESOURCE       handle,
    IMILMedia           *pIMedia,
    MIL_CHANNEL         hChannel,
    bool                notifyUceDirect
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);
    MILCMD_MEDIAPLAYER player = { MilCmdMediaPlayer };

    CHECKPTRARG(pChannel);

    player.Handle = handle;
    player.pMedia = (UINT64)(ULONG_PTR)pIMedia;
    player.notifyUceDirect = notifyUceDirect;

    IFC(pChannel->SendCommand(&player, sizeof(player)));

Cleanup:
    RRETURN(hr);
}
/*++

Routine Description:

    MilResource_SendCommandBitmapSource -

        This method sends a command from the managed ImageData object to the
        associated slave bitmap resource refreshing its bitmap content based on
        the IWGXBitmapSource.

        handle - Handle to the slave bitmap resource
        pIBitmapSource - Interface for accessing bitmap content
        shareBitmap - Share the bitmap bits if possible.
        shareMemoryBitmap - Share the raw IWGXBitmapSource pointer.  We should
          only do this if we know its (1) trusted and (2) uncompressed.
        pBatch - Records commands to the slave bitmap resource

    The logic is as follows:

        If it's not 32-bpp, instantiate a format converter to 32-bpp and use
        this as the input "IWGXBitmapSource".

        If we're sharing and not cross machine then
            - If system memory bitmap
                    send the IWGXBitmapSource
            - else
                    create a section object
                    send section handle
        else
            send copy of pixels

        In the sharing case, the slave bitmap resource acquires its own
        reference through the bitmap source or section object and must
        release it when it's done.

--*/
MtDefine(BitmapMemory, MILRender, "BitmapMemory");
MtDefine(PaletteMemory, MILRender, "PaletteMemory");

EXTERN_C HRESULT WINAPI
MilResource_CreateCWICWrapperBitmap(
    __in_ecount(1) IWICBitmapSource *pIBitmapSource,
    __out_ecount(1) IWICBitmapSource **ppCWICWrapperBitmap
    )
{
    HRESULT hr = S_OK;
    IWICImagingFactory *pIWICFactory = NULL;
    WICPixelFormatGUID fmtWIC;
    UINT width, height, stride;
    IWICBitmap *pIWICBitmap = NULL;
    IWGXBitmap *pCWICWrapperBitmap = NULL;

    CHECKPTRARG(pIBitmapSource);
    CHECKPTRARG(ppCWICWrapperBitmap);

    // We don't need to format convert pIBitmapSource, we're already in an acceptable format.

    // Sanity check the bitmap size
    IFC(pIBitmapSource->GetPixelFormat(&fmtWIC));
    IFC(pIBitmapSource->GetSize(&width, &height));
    IFC(HrCalcDWordAlignedScanlineStride(width, fmtWIC, OUT stride));
    if (height >= (INT_MAX / stride))
    {
        IFC(WGXERR_VALUEOVERFLOW);
    }

    if (FAILED(pIBitmapSource->QueryInterface(
        IID_IWICBitmap,
        reinterpret_cast<void **>(&pIWICBitmap)
        )))
    {
        IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
        IFC(pIWICFactory->CreateBitmapFromSource(
            pIBitmapSource,
            WICBitmapNoCache,
            &pIWICBitmap
            ));
    }

    IFC(CWICWrapperBitmap::Create(pIWICBitmap, &pCWICWrapperBitmap));

    *ppCWICWrapperBitmap = static_cast<IWICBitmapSource *>(static_cast<CWICWrapperBitmap *>(pCWICWrapperBitmap));
    pCWICWrapperBitmap = NULL;

Cleanup:
    // If succeeded, the wrapper now owns the reference to pIWICBitmap
    // If failed, clean up pIWICBitmap
    ReleaseInterface(pIWICBitmap);

    ReleaseInterface(pIWICFactory);
    ReleaseInterface(pCWICWrapperBitmap);
    
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_SendCommandBitmapSource(
    __in_ecount(1) HMIL_RESOURCE handle,
    __in_ecount(1) IWICBitmapSource *pIBitmapSource,
    __in_ecount(1) MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    IWICBitmapSource *pIBitmapSourceExtraAddRef = NULL;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pIBitmapSource);
    CHECKPTRARG(pChannel);

    pIBitmapSourceExtraAddRef = pIBitmapSource;
    pIBitmapSourceExtraAddRef->AddRef();
    //
    // At this point, pIBitmapSourceExtraAddRef has a ref count of +1.  This reference
    // keeps it alive during transport and will be passed onto the slave bitmap
    // resource, which will release it when it's finished.
    //

    MILCMD_BITMAP_SOURCE bmp;
    bmp.Type     = MilCmdBitmapSource;
    bmp.Handle   = handle;
    bmp.pIBitmap = pIBitmapSourceExtraAddRef;

    IFC(pChannel->SendCommand(&bmp, sizeof(bmp)));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(pIBitmapSourceExtraAddRef);
    }

    RRETURN(hr);
}


HRESULT WINAPI
MilChannel_SetNotificationWindow(
    _In_ MIL_CHANNEL hChannel,
    HWND hwnd,
    UINT message
    )
{
    HRESULT hr = S_OK;
    RWM_TRACE();
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->SetNotificationWindow(hwnd, message));

Cleanup:
    RRETURN(hr);
}

/*++

MilCompositionEngine_EnterCompositionEngineLock

Enters the composition engine lock.

--*/

VOID WINAPI
MilCompositionEngine_EnterCompositionEngineLock()
{
    g_csCompositionEngine.Enter();
}

/*++

MilCompositionEngine_ExitCompositionEngineLock

Enters the composition engine lock.

--*/

VOID WINAPI
MilCompositionEngine_ExitCompositionEngineLock()
{
    g_csCompositionEngine.Leave();
}

HRESULT WINAPI MilPlayer_Create(
    __deref_out_ecount(1) HMIL_PLAYER* phPlayer
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(phPlayer);

#if PRERELEASE
    CMilRecordPacketPlayer *pPlayer = NULL;

    IFC(CMilRecordPacketPlayer::CreateRecordPacketPlayer(&pPlayer));

    *phPlayer = reinterpret_cast<HMIL_PLAYER>(pPlayer);
#else
    DPRINT1("[RWM] STUB MilPlayer_Create -> E_NOTIMPL (record player is PRERELEASE-only)\n");
    IFC(E_NOTIMPL);
#endif

Cleanup:
    RRETURN(hr);
}

EXTERN_C HRESULT WINAPI MilPlayer_Process(
    __in_ecount(1) HMIL_PLAYER hPlayer,
    _In_reads_(sizeof(MIL_REC_PACKET_HEADER)) const BYTE* pbHeader,
    _In_opt_count_(sizeof(UCE_RDP_HEADER)) const BYTE* pbRdpHeader,
    __in_bcount_opt(cbData) const BYTE* pbData,
    UINT cbData
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(hPlayer);
    CHECKPTRARG(pbHeader);

#if PRERELEASE
    CMilRecordPacketPlayer *pPlayer =
        reinterpret_cast<CMilRecordPacketPlayer *>(hPlayer);

    IFC(pPlayer->ProcessFilePacketContents(
        reinterpret_cast<const MIL_REC_PACKET_HEADER*>(pbHeader),
        reinterpret_cast<const UCE_RDP_HEADER*>(pbRdpHeader),
        pbData,
        cbData
        ));
#else
    DPRINT1("[RWM] STUB MilPlayer_Process -> E_NOTIMPL (record player is PRERELEASE-only)\n");
    IFC(E_NOTIMPL);
#endif

Cleanup:
    RRETURN(hr);
}




//+-----------------------------------------------------------------------
//
//  Member: MilCompositionEngine_GetComposedEventId
//
//  Synopsis:  Gets the counter used to create the name of the compsed event
//
//------------------------------------------------------------------------
HRESULT WINAPI MilCompositionEngine_GetComposedEventId(
    __out_ecount(1) UINT *pcEventId
    )
{
    return GetCompositionEngineComposedEventId(pcEventId);
}

// Ignore deprecation of D3DMATRIX on method prototypes defined
// in windows/published, where CMILMatrix isn't defined.
#pragma warning (push)
#pragma warning (disable : 4995)

//+------------------------------------------------------------------------
//
//  Function:
//      MilUtility_GetTileBrushMapping
//
//  Synopsis:
//      MILCore export that exposes CTileBrushUtils::CalculateTileBrushMapping
//      to external callers (e.g., managed code).
//
//-------------------------------------------------------------------------
EXTERN_C VOID WINAPI
MilUtility_GetTileBrushMapping(
    __in_ecount_opt(1) const D3DMATRIX *pTransform,
        // Transform that is applied to the Viewport
    __in_ecount_opt(1) const D3DMATRIX *pRelativeTransform,
        // RelativeTransform that is applied to the Viewport
    MilStretch::Enum stretch,
        // Stretch mode to use in Viewbox->Viewport mapping
    MilHorizontalAlignment::Enum alignmentX,
        // X Alignment to use in Viewbox->Viewport mapping
    MilVerticalAlignment::Enum alignmentY,
        // Y Alignment to use in Viewbox->Viewport mapping
    MilBrushMappingMode::Enum viewportUnits,
        // Viewport mapping mode (relative or absolute)
    MilBrushMappingMode::Enum viewboxUnits,
        // Viewbox mapping mode (relative or absolute)
    __in_ecount(1) const MilPointAndSizeD *pShapeFillBounds,
        // Shape fill-bounds pViewport is relative to.
        // Only needed when viewportUnits == RelativeToBoundingBox.
    __in_ecount(1) const MilPointAndSizeD *pContentBounds,
        // Content bounds pViewbox is relative to.
        // Only needed when viewboxUnits == RelativeToBoundingBox.
    __inout_ecount(1) MilPointAndSizeD *pViewport,
        // IN: User-specified Viewport to map Viewbox to
        // OUT: Viewport in absolute units
    __inout_ecount(1) MilPointAndSizeD *pViewbox,
        // IN: User-specified Viewbox to map to Viewport
        // OUT: Viewbox in absolute units
    __out_ecount(1) D3DMATRIX *pContentToWorld,
        // Combined Content->World transform
    __out_ecount(1) BOOL *pfBrushIsEmpty
        // Whether or not this brush renders nothing because of an empty viewport/viewbox
    )
{
    CTileBrushUtils::CalculateTileBrushMapping(
        reinterpret_cast<const CMILMatrix*>(pTransform),
        reinterpret_cast<const CMILMatrix*>(pRelativeTransform),
        stretch,
        alignmentX,
        alignmentY,
        viewportUnits,
        viewboxUnits,
        reinterpret_cast<const MilPointAndSizeD*>(pShapeFillBounds),
        reinterpret_cast<const MilPointAndSizeD*>(pContentBounds),
        1.0f,   // Content scale is only used for ImageBrush's, which do not call this method
        1.0f,   // Content scale is only used for ImageBrush's, which do not call this method
        reinterpret_cast<MilPointAndSizeD*>(pViewport),
        reinterpret_cast<MilPointAndSizeD*>(pViewbox),
        NULL,   // Caller doesn't need the Content->Viewport transform seperated from the final transform
        NULL,   // Caller doesn't need the Viewport->World transform seperated from the final transform
        reinterpret_cast<CMILMatrix*>(pContentToWorld),
        pfBrushIsEmpty
        );
}

#pragma warning (pop)

/*++

Routine Description:

    Enable instrumentation for rendering performance measurement.
    See comments for "MilPerfInstrumentationFlags" in partition.h.
--*/

VOID WINAPI SetMilPerfInstrumentationFlags(UINT flags)
{
    g_uMilPerfInstrumentationFlags = flags;
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        MilGlyphRun_GetGlyphOutline
//
//    Synopsis:
//        Used to communicate with DWrite directly to get a glyph's serialized geometric
//        representation and return it to managed code.
//
//------------------------------------------------------------------------------

EXTERN_C HRESULT WINAPI
MilGlyphRun_GetGlyphOutline(
    _In_ IDWriteFontFace* pFontFace,
    USHORT glyphIndex, 
    bool sideways, 
    double renderingEmSize,
    __deref_out_ecount(*pSize) byte **ppFigureDataBytes,
    _Out_ UINT *pSize,
    _Out_ MilFillMode::Enum *pFillRule
    )
{
    HRESULT hr = S_OK;
    Assert(pFontFace);

    CGlyphRunGeometrySink *pGeometrySink = NULL;
    MilPathGeometry *pFigureData = NULL;
   
    IFC(CGlyphRunGeometrySink::Create(&pGeometrySink));
    
    IFC(pFontFace->GetGlyphRunOutline(
        static_cast<float>(renderingEmSize),
        &glyphIndex,
        NULL,
        NULL,
        1,
        sideways,
        false, // This is handled by GlyphRun::BuildGeometry in managed code.
        pGeometrySink
        ));

    // We now own the reference to pFigureData, the allocated memory block containing serialized 
    // geometry data structs.
    IFC(pGeometrySink->ProduceGeometryData(&pFigureData, pSize, pFillRule));

    *ppFigureDataBytes = reinterpret_cast<byte*>(pFigureData);
    pFigureData = NULL;

Cleanup:
    delete pFigureData;
    ReleaseInterface(pGeometrySink);
    ReleaseInterface(pFontFace); // This was AddRef-ed when passed in from managed code.
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        MilGlyphRun_ReleasePathGeometryData
//
//    Synopsis:
//        Frees the glyph data returned to managed code by MilGlyphRun_GetGlyphOutline.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilGlyphRun_ReleasePathGeometryData(
    _In_ byte* pPathGeometryData
    )
{
    MilPathGeometry *pFigureData = reinterpret_cast<MilPathGeometry*>(pPathGeometryData);
    delete(pFigureData);
    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        GetNextPerfElementId
//
//    Synopsis:
//        Gets an ID that will be unique across appdomains for taging elements so they
//        can be identified by tools like VisualProfiler.
//
//------------------------------------------------------------------------------

// Definition for _InterlockedCompareExchange64 from intrin.h
extern "C" {
#if defined(_M_AMD64)
__int64 _InterlockedCompareExchange64_np(__int64 volatile *, __int64, __int64);
#define _InterlockedCompareExchange64 _InterlockedCompareExchange64_np
#else
__int64 _InterlockedCompareExchange64(__int64 volatile *, __int64, __int64);
#endif
}

EXTERN_C
LONGLONG WINAPI
GetNextPerfElementId()
{
    static volatile __int64 id = 0;
    // _InterlockedIncrement64 is not available on Windows XP so use InterlockedCompareExchange to emulate it.
    __int64 old;
    do
    {
        old = id;
    } while(_InterlockedCompareExchange64(&id, old + 1, old) != old);
    return old + 1;
}


}