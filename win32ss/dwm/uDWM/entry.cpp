
#include "uDWM.h"
#include <debug.h>
EXTERN_C
BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                    DWORD fdwReason,
                    LPVOID fImpLoad)

{
    /* For now, there isn't much to do */
    if (fdwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hinstDLL);
    return TRUE;
}

EXTERN_C
HRESULT
WINAPI
DwmClientStartup(PRWM_STARTUPINFO StartupInfo, PRWM_COMPOSITIONINFO CompInfo)
{
    DPRINT1("DwmClientStartup Called:\n");
    return CreateDwmDesktop(StartupInfo, CompInfo);
}

EXTERN_C
HRESULT
WINAPI
DwmClientNotifyRedirectionShutdown()
{
    DPRINT1("DwmClientNotifyRedirectionShutdown Called:\n");
    __debugbreak();
    return 0;
}

EXTERN_C
HRESULT
WINAPI
DwmClientShutdown()
{
    DPRINT1("DwmClientShutdown Called:\n");
    return 0;
}
