
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#ifdef __REACTOS__
BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ PVOID pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);

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
#endif
