#include <LpcCreateLib.hpp>
//#define NDEBUG
#include <debug.h>

LpcCreateLib::LpcCreateLib()
{
    InstancePort = NULL;
}

LpcCreateLib::~LpcCreateLib()
{
    if (InstancePort)
    {
        StopPortThread();
        CloseHandle(InstancePort);
        InstancePort = NULL;
    }
}

NTSTATUS
LpcCreateLib::StopPortThread()
{
    NTSTATUS Status;

    DPRINT("Stopping port thread\n");

    // Close the port handle
    if (InstancePort)
    {
        CloseHandle(InstancePort);
        InstancePort = NULL;
    }

    Status = WaitForSingleObject(GlobalGenericThread, INFINITE);
    if (Status != WAIT_OBJECT_0)
    {
        DPRINT1("Failed to wait for port thread to finish: %lx\n", Status);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}   

DWORD WINAPI
LpcCreateLib::ProcessCompleteConnect(PLPC_MAX_MESSAGE LpcInput)
{
    HANDLE PortHandle;
    NTSTATUS Status;
    // Process the connection request
    Status = NtAcceptConnectPort(&PortHandle, this, &LpcInput->Header, TRUE, 0, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to complete connect port: %lx\n", Status);
        return Status;
    }
    NtCompleteConnectPort(PortHandle);

    return STATUS_SUCCESS;
}

VOID
WINAPI
LpcCreateLib::ProcessLpcOperation(PLPC_MAX_MESSAGE LpcReply, PVOID PortContext)
{
    ULONG MessageType;

    MessageType = LpcReply->Header.u2.s2.Type;

    switch (MessageType)
    {
        case LPC_REQUEST:
            if (LpcHandler)
                LpcHandler(LpcReply, PortContext);
            break;
        case LPC_DATAGRAM:
            DPRINT("LPC_DATAGRAM request received\n");
            __debugbreak();
            break;
        case LPC_PORT_CLOSED:
        case LPC_CLIENT_DIED:
            DPRINT("LPC_PORT_CLOSED or LPC_CLIENT_DIED received\n");
            __debugbreak();
            break;
        case LPC_CONNECTION_REQUEST:
            DPRINT("LPC_CONNECTION_REQUEST received\n");
            ProcessCompleteConnect(LpcReply);
            break;
        default:
            DPRINT("Unknown message type: %lx\n", MessageType);
            __debugbreak();
            break;
    }
}


DWORD
WINAPI
GenericPortThread(LPVOID lpParameter)
{
    LpcCreateLib* pThis = (LpcCreateLib*)lpParameter;
    NTSTATUS Status;
    PLPC_MAX_MESSAGE LpcReply;
    PVOID PortContext;

    // Wait for a message from the port
    while (TRUE)
    {
        // Allocate memory for the message
        LpcReply = (PLPC_MAX_MESSAGE)(PLPC_MAX_MESSAGE)RtlAllocateHeap(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          256);
        if (!LpcReply)
        {
            DPRINT1("GenericPortThread: Failed to allocate memory for LPC message\n");
            break;
        }

        // Wait for a message
        Status = NtReplyWaitReceivePort(pThis->InstancePort, &PortContext, NULL, &LpcReply->Header);
        if (Status != STATUS_SUCCESS)
        {
            DPRINT1("GenericPortThread: Failed to receive message from port: %lx\n", Status);
            RtlFreeHeap(GetProcessHeap(), 0, LpcReply);
            break;
        }

        pThis->ProcessLpcOperation(LpcReply, PortContext);

        /* Free the message */
        RtlFreeHeap(GetProcessHeap(), 0, LpcReply);
    }

    return S_OK;
}

NTSTATUS
LpcCreateLib::StartPortThread(HANDLE hSourceHandle)
{
    HANDLE hThread;

    if (! DuplicateHandle( GetCurrentProcess(), hSourceHandle, GetCurrentProcess(), &InstancePort, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        DPRINT1("Failed to duplicate handle\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Create the port thread
    hThread = CreateThread(NULL,
                           0,
                           GenericPortThread,
                           this,
                           0,
                           0);
    if (hThread == NULL)
    {
        DPRINT1("Failed to create port thread\n");
        return STATUS_UNSUCCESSFUL;
    }

    GlobalGenericThread = hThread;
    return STATUS_SUCCESS;
}