
/* INCLUDES *****************************************************************/

#include "uxsms.h"

#include <debug.h>

#define DWMUXSMS_APIPORTDESCRIPTION L"User Experience SubSystem API Port"

/* GLOBALS ******************************************************************/

static WCHAR PortName[] = L"\\UxSmsApiPort";

/* FUNCTIONS ****************************************************************/
typedef struct  _SESSION_PORTPATH
{
    WCHAR PortPathStr[MAX_PATH];
} SESSION_PORTPATH, *PSESSION_PORTPATH;

    HANDLE PortHandle;
typedef struct _SESSION_INIT
{
    ULONG SessionId;
    ULONG ProcessId;
} SESSION_INIT, *PSESSION_INIT;
HANDLE GlobalServiceApiThreadHandle;


VOID
WINAPI
ProcessLpcOperation(PLPC_MAX_MESSAGE LpcReply)
{
    __debugbreak();
    if (LpcReply->Message)
    {
        switch (LpcReply->Message)
        {
            case 1: //DwmConnect:
            {
                DPRINT1("UXSS.EXE has requested to connect to the UxSms service\n");
                WCHAR* UXSSPortName = (WCHAR*)LpcReply->Data;
                DPRINT1("PortName: %ls\n", UXSSPortName);
                NtReplyPort(PortHandle, &LpcReply->Header);
                break;
            }
            default:
            {
                DbgPrint("Unknown message: %lx\n", LpcReply->Message);
                break;
            }
        }
    }
}


DWORD
WINAPI
RWMServiceApiPortThread(LPVOID lpParameter)
{
    PLPC_MAX_MESSAGE LpcReply;
     PVOID PortContext;
     NTSTATUS Status;
    LpcReply = (PLPC_MAX_MESSAGE)RtlAllocateHeap(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          256);
    while(1)
    {
        Status = NtReplyWaitReceivePort(PortHandle, &PortContext, 0, &LpcReply->Header);
        /* Check if we didn't get success */
        if (Status != STATUS_SUCCESS)
        {
            /* If we only got a warning, keep going */
            if (NT_SUCCESS(Status)) continue;

            /* We failed big time, so start out fresh */
            DPRINT1("RWMServiceApiPortThread: ReceivePort failed - Status == %X\n", Status);
            continue;
        }

        DPRINT1("lpc message: %lx\n", LpcReply->Message);
        ProcessLpcOperation(LpcReply);
    }
    __debugbreak();
    return 0;
}

VOID
WINAPI
InitializeServicePort()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PortString;

    RtlInitUnicodeString(&PortString, PortName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &PortString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreatePort(&PortHandle,
                          &ObjectAttributes,
                          (sizeof(L"User Experience Session Management Service API Port")),
                          256,
                          16 * 256);

    if (!NT_SUCCESS(Status))
        DbgPrint("Failed to create port: %lx\n", Status);
     GlobalServiceApiThreadHandle = CreateThread(0, 0, RWMServiceApiPortThread, NULL, 0, 0);
}
