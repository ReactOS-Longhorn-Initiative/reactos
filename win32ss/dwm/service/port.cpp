
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

LpcCreateLib* lpcCreateLib;

typedef enum _RWM_SERIVCE_MSGS
{
    RWM_SERVICE_CONNECT = 1,
    RWM_SERVICE_PUSH_OBJ = 2,
    RWM_SERVICE_POP_OBJ = 3,
    RWM_SERVICE_QUERY_PORTNAME = 4
} RWM_SERIVCE_MSGS, *PRWM_SERIVCE_MSGS;
WCHAR* DwmSessionPort;

VOID
WINAPI
RwmServiceConnect(PLPC_MAX_MESSAGE LpcReply)
{
    PSESSION_PORTPATH pSessionPath = {0};
    DPRINT("RWM_SERVICE_CONNECT received\n");
    pSessionPath = (PSESSION_PORTPATH)LpcReply->Data;
    DPRINT1("Path: %ls\n", pSessionPath->PortPathStr);
    DwmSessionPort = pSessionPath->PortPathStr;
    SESSION_INIT* pSessionInit = (SESSION_INIT*)LpcReply->Data;
    pSessionInit->SessionId = 0;
    pSessionInit->ProcessId = GetCurrentProcessId();
    LpcReply->Header.u1.s1.DataLength = sizeof(ULONG) + sizeof(ULONG) + sizeof(SESSION_INIT);
}

NTSTATUS
WINAPI
HandleServiceLpcOperations(PLPC_MAX_MESSAGE LpcReply, PVOID PortContext)
{

    ULONG MessageType;
    NTSTATUS Status;
    MessageType = LpcReply->Message;

    LpcCreateLib* pThis = (LpcCreateLib*)PortContext;
    switch (MessageType)
    {
        case RWM_SERVICE_CONNECT:
            RwmServiceConnect(LpcReply);
            LpcReply->Status = STATUS_SUCCESS;
            break;
        case RWM_SERVICE_PUSH_OBJ:
            DPRINT("RWM_SERVICE_PUSH_OBJ received\n");
            break;
        case RWM_SERVICE_POP_OBJ:   
            DPRINT("RWM_SERVICE_POP_OBJ received\n");
            break;
        case RWM_SERVICE_QUERY_PORTNAME:
            DPRINT("RWM_SERVICE_QUERY_PORTNAME received\n");
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

    lpcCreateLib = new LpcCreateLib();
    lpcCreateLib->LpcHandler = (PINTERNALLPCHANDLER)HandleServiceLpcOperations;
    lpcCreateLib->StartPortThread(PortHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to thread port: %lx\n", Status);
        return;
    }
    DPRINT1("Created thread: %lx\n", PortHandle);
}
