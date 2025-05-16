/*
 * PROJECT:     RWM UxSms Service
 * LICENSE:     MIT (https://opensource.org/licenses/MIT)
 * PURPOSE:     Entry points for UxSms
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <uxsms.h>

/* FUNCTIONS *****************************************************************/

EXTERN_C
BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

EXTERN_C
VOID
WINAPI
ServiceMain(DWORD argc, LPTSTR *argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //TODO: check error code
    ServiceStartup();
}
