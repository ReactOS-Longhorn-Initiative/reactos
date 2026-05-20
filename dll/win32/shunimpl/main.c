

#include <windef.h>
#include <winbase.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // Prevent from being loaded by returning FALSE
            // TODO: Support "FailObsoleteShellAPIs" ATOM (see https://habr.com/en/articles/437436/)
            return FALSE;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

PSECURITY_DESCRIPTOR WINAPI UNIMPL_GetShellSecurityDescriptor(const PVOID* apUserPerm, int cUserPerm)
{
    return NULL;
}
