#include "uDWM.h"
#include "CDesktopManager.hpp"

#include <debug.h>
#define DWMPRIORITY 128
DWORD WINAPI DwmGetPriorityClass()
{
  return DWMPRIORITY;
}


CDesktopManager::CDesktopManager()
{
    // Init CBaseObject?
    // Grab all the function pointers
    // Init WindowAnimationSettings
    //  CDesktopManager->IsCompEnabled (1733) = 1;
}

CDesktopManager::~CDesktopManager()
{

}

DWORD WINAPI
CDesktopManager::uDWMWndProc(LPVOID lpThreadParameter)
{
    return 0;
}

//    *((struct HMIL_CONNECTION__ **)this + 4),
HRESULT
CDesktopManager::Initialize(StartupInfo *StartupInfo, CompositionInfo *CompInfo)
{
    DWORD PriorityClass;
    HANDLE DwmProcess;
    HMIL_CONNECTION HMil_Connection = CompInfo->hConnection;
    MIL_CHANNEL  hRedirectionStateChannel = CompInfo->hSourceChannel;
    m_hChannel = NULL;
    DwmRedirectionManager* RedirMgr ;
    HRESULT Status = 0;
    __debugbreak();
    UINT32 pMarshalType;
    PriorityClass = DwmGetPriorityClass();
    DwmProcess = GetCurrentProcess();
    SetPriorityClass(DwmProcess, PriorityClass);
    Status = DwmRedirectionManagerInitialize(CompInfo,
                                    NULL, //Window List?
                                    (MIL_MESSAGE*)((ULONG_PTR)StartupInfo + 9),
                                    &RedirMgr);
        if (Status != STATUS_SUCCESS)
    {
        DPRINT1("CDesktopManager::Initializ: Failed with Status %d", Status);
        return Status;
    }
    Status = MilConnection_CreateChannel(HMil_Connection,
             hRedirectionStateChannel,
             &m_hChannel);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("CDesktopManager::MilConnection_CreateChannel Failed with Status %d", Status);
        return Status;
    }
    Status = MilChannel_GetMarshalType(m_hChannel, &pMarshalType);
        if (Status != STATUS_SUCCESS)
    {
        DPRINT1("CDeskastopManager::MilChannel_GetMarshalType Failed with Status %d", Status);
        return Status;
    }

     DwmRedirectionManagerSetClientChannel(m_hChannel);

    MilChannel_CommitChannel(m_hChannel);
        if (Status != STATUS_SUCCESS)
    {
        DPRINT1("CDeskastopManager::MilChannel_CommitChannel Failed with Status %d", Status);
        return Status;
    }

     HANDLE EventStarted = CreateEventW(0, 1, 0, 0);
     CreateThread(0,
                 0,
                 uDWMWndProc,
                 EventStarted,
                 0,
                 0);
    //(MilConnection_CreateChannel( m_hConnection, pCompositionInfo->hRedirectionStateChannel, &m_hChannel )))",
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("DwmRedirectionManagerInitialize: Failed with Status %d", Status);
        return Status;
    }

    return 0;
}

HRESULT
CDesktopManager::LoadTheme()
{
    return S_OK;
}

VOID
CDesktopManager::SetupColorization(BOOLEAN A1, BOOLEAN A2)
{

}

VOID
CDesktopManager::GetBlendedColorizationColor(BOOLEAN a1, BOOLEAN a2, float a3, D3DCOLORVALUE *ColorValue)
{

}

HRESULT
CDesktopManager::SetColorizationColorResource(struct CResource *a1, bool a2, bool a3, float a4)
{
    return S_OK;
}
VOID
CDesktopManager::UnloadTheme()
{

}
