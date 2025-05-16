/*
 * PROJECT:     RWM UxSms Service
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UxSms Service Port
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <uxsms.h>
#include <wtsapi32.h>
//#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

HANDLE GlobalServiceApiThreadHandle;
HANDLE PortHandle;
LpcCreateLib* lpcCreateLib;
WCHAR* DwmSessionPort;

/* FUNCTIONS ****************************************************************/

VOID
WINAPI
RwmServiceConnect(PLPC_RWM_MESSAGE LpcReply)
{
    PRWMSERVCMD_CONNECT_SESSION_PORT pSessionPath = {0};
    DPRINT("RWM_SERVICE_CONNECT received\n");
    pSessionPath = (PRWMSERVCMD_CONNECT_SESSION_PORT)LpcReply->Data;
    DwmSessionPort = pSessionPath->PortPathStr;
    DPRINT1("Path: %ls\n", pSessionPath->PortPathStr);

    PRWMSERVCMD_CONNECT_SESSIONINFO pSessionInit = (PRWMSERVCMD_CONNECT_SESSIONINFO)LpcReply->Data;
    pSessionInit->SessionId = WTS_CURRENT_SESSION;
    pSessionInit->ProcessId = GetCurrentProcessId();
    LpcReply->Header.u1.s1.DataLength = sizeof(ULONG) + sizeof(ULONG) + sizeof(RWMSERVCMD_CONNECT_SESSIONINFO);
}

VOID
WINAPI
RwmServiceQuertyPortName(PLPC_RWM_MESSAGE LpcReply)
{
    PRWMSERVCMD_CONNECT_SESSION_PORT GetSessionPort = {0};
    DPRINT("RWM_SERVICE_CONNECT received\n");
    GetSessionPort = (PRWMSERVCMD_CONNECT_SESSION_PORT)LpcReply->Data;
    ULONG PortNameSize = lstrlenW(DwmSessionPort) * sizeof(WCHAR);
    RtlCopyMemory(GetSessionPort->PortPathStr, DwmSessionPort, PortNameSize);
    LpcReply->Header.u1.s1.DataLength = sizeof(ULONG) + sizeof(ULONG) + PortNameSize;
}


NTSTATUS
WINAPI
HandleServiceLpcOperations(PLPC_RWM_MESSAGE LpcReply, PVOID PortContext)
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

    RtlInitUnicodeString(&PortString, RWMUXSMS_APIPORTNAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &PortString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreatePort(&PortHandle,
                          &ObjectAttributes,
                          RWMUXSMS_APIPORTDESCRIPTIONLEN,
                          sizeof(LPC_RWM_MESSAGE),
                          16 * sizeof(LPC_RWM_MESSAGE));

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

VOID
WINAPI
DestroyServicePort()
{
    NTSTATUS Status;
    if (lpcCreateLib)
    {
        lpcCreateLib->StopPortThread();
        delete lpcCreateLib;
        lpcCreateLib = NULL;
    }
    if (PortHandle)
    {
        Status = NtClose(PortHandle);
        PortHandle = NULL;
    }
}