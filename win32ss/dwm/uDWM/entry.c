
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

HRESULT
WINAPI
DwmClientStartup()
{
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