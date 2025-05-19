
#include "rwm.hpp"
#include <ndk/lpcfuncs.h>
#include <strsafe.h>

HANDLE GlobalSessionPortHandle;
HANDLE GlobalApiThreadHandle;
WCHAR PortName[MAX_PATH];
#include <debug.h>

LpcCreateLib* lpcCreateLib;

DWORD
WINAPI
RWMSessionApiPortThread(LPVOID lpParameter)
{
    __debugbreak();
    return 0;
}

NTSTATUS
WINAPI
HandleDwmApiPortLpcOperations(PLPC_RWM_MESSAGE LpcReply, PVOID PortContext)
{

    ULONG MessageType;
    NTSTATUS Status;
    MessageType = LpcReply->Message;

    LpcCreateLib* pThis = (LpcCreateLib*)PortContext;
    switch (MessageType)
    {
        case RWMCMD_NOTIFY_SETTINGS_CHANGE:
            DPRINT("RWMCMD_NOTIFY_SETTINGS_CHANGE\n");
            __debugbreak();
            break;
        default:
            DPRINT("Unknown message type: %lx\n", MessageType);
            __debugbreak();
            break;
    }
    Status = NtReplyPort(pThis->InstancePort, &LpcReply->Header);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to reply to port: %lx\n", Status);
        return Status;
    }
    
    return Status;
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
                L"\\Dwm-%04X-ApiPort-%04X", rand() % 0xFFFF, rand() % 0xFFFF);
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
    lpcCreateLib = new LpcCreateLib();
    lpcCreateLib->LpcHandler = (PINTERNALLPCHANDLER)HandleDwmApiPortLpcOperations;
    lpcCreateLib->StartPortThread(GlobalSessionPortHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to thread port: %lx\n", Status);
        return;
    }
}

