#include "dwrite_private.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    case DLL_PROCESS_DETACH:
 
    }
    return TRUE;
}

HRESULT WINAPI DWriteCreateFactory_loc(ULONG type, REFIID riid, IUnknown **ret)
{
    return 0;
}
