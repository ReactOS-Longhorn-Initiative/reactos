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
typedef enum _DWRITE_FACTORY_TYPE {
    DWRITE_FACTORY_TYPE_ISOLATED = 1
} DWRITE_FACTORY_TYPE;

HRESULT WINAPI DWriteCreateFactory(DWRITE_FACTORY_TYPE type, REFIID riid, IUnknown **ret)
{
    return 0;
}
