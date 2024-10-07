
#include "uDWM.h"
#include <debug.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                    DWORD fdwReason,
                    LPVOID fImpLoad)

{
    /* For now, there isn't much to do */
    if (fdwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hinstDLL);
    return TRUE;
}

#define UDWM_VERSION_VISTA 0xBDDCB2B
HRESULT CDesktopManager_Create(StartupInfo *StartupInfo, CompositionInfo *CompInfo);

/* HALF-IMPLEMENTED */
HRESULT
WINAPI
DwmClientStartup(StartupInfo *StartupInfo, CompositionInfo *CompInfo)
{
    /* maybe a criticial section should be made and entered ? */
    HRESULT Status;
    DPRINT1("DwmClientStartup Called: StartupInfo: %X CompositionInfo: %X\n", StartupInfo, CompInfo);
    Status =  DwmVersionCheck(UDWM_VERSION_VISTA);
    if (Status != 0)
    {
        DPRINT1("dwmredir->DwmVersionCheck has returned with failure status: %d\n", Status);
        return Status;
    }
    Status = CDesktopManager_Create(StartupInfo, CompInfo);

#if DBG
    __debugbreak();
#endif
    return Status;
}

HRESULT
WINAPI
DwmClientNotifyRedirectionShutdown()
{
    DPRINT1("DwmClientNotifyRedirectionShutdown Called:\n");
    return 0;
}

HRESULT
WINAPI
DwmClientShutdown()
{
    DPRINT1("DwmClientShutdown Called:\n");
    /* looks like it's more so left over from longhorn dev, just releases some locks */
    return 0;
}