/*
 * PROJECT:     RWM - User ReactOS Window Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:        
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

#include <uDWM.h>
#include <debug.h>
IDwmRedirectionManager* m_pIDwmRedirectionManager = NULL;
DwmVisual* GlobalRootVisual;
DwmDesktop *DwmDesktopInstance;
DWORD WINAPI
uDWMWndProc(LPVOID lpThreadParameter)
{
    while(1)
    {
        Sleep(10000);
       // DPRINT1("uDWMWndProc Called:\n");
    }
    return 0;
}

HRESULT
WINAPI
CreateDwmDesktop(PRWM_STARTUPINFO StartupInfo, PRWM_COMPOSITIONINFO CompInfo)
{
    DwmDesktopInstance = new DwmDesktop();
    if (DwmDesktopInstance == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = DwmDesktopInstance->Initialize(StartupInfo, CompInfo);
    if (FAILED(hr))
    {
        delete DwmDesktopInstance;
        return hr;
    }

    return S_OK;
}

DwmDesktop::DwmDesktop()
{
    GlobalChannel = NULL;
    InitializeCriticalSection(&CsDwmInstance);
}

DwmDesktop::~DwmDesktop()
{
    DeleteCriticalSection(&CsDwmInstance);
}

HRESULT (__fastcall *pfnNotificationCallbackTest)(const MIL_MESSAGE*);

HRESULT
__fastcall
NoteCallback(const MIL_MESSAGE* Message)
{
    DPRINT1("NoteCallback Called:\n");
    return 0;
}
EXTERN_C
VOID
WINAPI 
UpdateWindowList( IDwmWindowList* WindowListInstance);
EXTERN_C
HRESULT
WINAPI
DwmRedirectionManagerInitialize(PRWM_COMPOSITIONINFO Compinfo ,
                                IDwmWindowList* Interface ,
                                HRESULT (__fastcall *pfnNotificationCallback)(const MIL_MESSAGE*),
                                IDwmRedirectionManager ** Redir);

EXTERN_C
VOID WINAPI
DwmRedirectionManagerSetClientChannel(MIL_CHANNEL MilCoreHandle);

HRESULT DwmDesktop::Initialize(PRWM_STARTUPINFO StartupInfo, PRWM_COMPOSITIONINFO CompInfo)
{
    HRESULT hr = S_OK;
    DPRINT1("DwmDesktop::Initialize Called:\n");
    IDwmWindowList* WindowListInstance = (IDwmWindowList*)malloc(sizeof(IDwmWindowList));
    WindowListInstance->lpVtbl = (WindowListVtbl*)malloc(sizeof(WindowListVtbl));
    UpdateWindowList(WindowListInstance);
    hr = DwmRedirectionManagerInitialize(CompInfo, (IDwmWindowList*)WindowListInstance,
                                        StartupInfo->pfnNotificationCallback,
                                        &m_pIDwmRedirectionManager);
    if (SUCCEEDED(hr))
    {
        DPRINT1("DwmRedirectionManagerInitialize Succeeded:\n");
    }
    else
    {
        DPRINT1("DwmRedirectionManagerInitialize Failed:\n");
    }
    hr = MilConnection_CreateChannel(CompInfo->hConnection,
                                    CompInfo->hRedirectionStateChannel,
                                    &GlobalChannel);
    if (SUCCEEDED(hr))
    {
        DPRINT1("MilConnection_CreateChannel Succeeded:\n");
    }
    else
    {
        DPRINT1("MilConnection_CreateChannel Failed:\n");
    }

    hr = MilChannel_GetMarshalType(GlobalChannel, (MilMarshalType::Enum *)&MarshalType);
    if (SUCCEEDED(hr))
    {
        DPRINT1("MilChannel_GetMarshalType Succeeded:\n");
    }
    else
    {
        DPRINT1("MilChannel_GetMarshalType Failed:\n");
    }

    DwmRedirectionManagerSetClientChannel(GlobalChannel);
    CreateDwmVisual(GlobalChannel, &GlobalRootVisual);

     HANDLE EventStarted = CreateEventW(0, 1, 0, 0);
     CreateThread(0,
                 0,
                 uDWMWndProc,
                 EventStarted,
                 0,
                 0);

    MilChannel_CommitChannel(GlobalChannel);
    DPRINT1("DwmDesktop::Initialize Succeeded:\n");
    return hr;
}   

WindowClientData::WindowClientData()
{

}

WindowClientData::~WindowClientData()
{
    if (pszTitle)
    {
        free(pszTitle);
        pszTitle = NULL;
    }
    if (CompositedWindow)
    {
        CompositedWindow = NULL;
    }
}