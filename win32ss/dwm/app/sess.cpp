
#include "dwr.hpp"
#include <ndk/lpcfuncs.h>
#include <strsafe.h>

HANDLE GlobalSessionPortHandle;
HANDLE GlobalApiThreadHandle;
WCHAR PortName[MAX_PATH];
#include <debug.h>


DWORD
WINAPI
RWMSessionApiPortThread(LPVOID lpParameter)
{
    __debugbreak();
    return 0;
}

VOID
WINAPI
RWMCreateSessionPort()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PortString;

    /*
     * This logical differences between this and offical DWM
     * are pretty absurd:
     * We're assuming that everything is running on session 0 here.
     * RWM as an architecture has multiple hacks in this area, and this is no different
     * otherwise the APIPort would be attached on a per session basis.
     * 
     * Here we just create the session using the same random generalizer as the longhorn 5112
     * fallback.
     * 
     * This works on windows likely, but only because of the fact we PASS the port info
     * down into win32k
     */
    StringCchPrintfW(PortName,  _countof(PortName),
                L"\\UxSs-%04X-ApiPort-%04X", rand() % 0xFFFF, rand() % 0xFFFF);
     RtlInitUnicodeString(&PortString, (PCWSTR)&PortName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &PortString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreatePort(&GlobalSessionPortHandle,
                          &ObjectAttributes,
                          (sizeof(L"User Experience SubSystem API Port")),
                          256,
                          16 * 256);

    if (!NT_SUCCESS(Status))
        DbgPrint("Failed to create port: %lx\n", Status);
    GlobalApiThreadHandle = CreateThread(0, 0, RWMSessionApiPortThread, NULL, 0, 0);
}

