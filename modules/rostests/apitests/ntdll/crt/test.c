
#include "test.h"
#include <stdarg.h>
#include <string.h>
#include <intrin.h>
#include <windows.h>

const char* winetest_module_name = "ntdll.dll";

void* load_function(const char *funcname)
{
    HMODULE hdll = LoadLibraryA(winetest_module_name);
    return GetProcAddress(hdll, funcname);
}

struct _TEB
{
    UCHAR Reserved1[0x1480];
    void* TlsSlots[64];
};

int* __cdecl ntdll_errno(void)
{
    /* In user mode errno is stored in TLS slot 16 in the TEB */
    return (int*)&NtCurrentTeb()->TlsSlots[16];
}
