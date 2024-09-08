#include "KB.h"


BOOL
WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{

    return TRUE;
}

BOOL WINAPI DllMainCRTStartup(HANDLE hinstDll, DWORD fdwReason, LPVOID fImpLoad)
{
  return TRUE;
}