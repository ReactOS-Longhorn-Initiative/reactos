#include <LpcConnectLib.hpp>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>


LpcConnectLib::LpcConnectLib()
{
    InstancePort = NULL;
}
LpcConnectLib::~LpcConnectLib()
{
    if (InstancePort)
        DisconnectFromPort();
}

NTSTATUS
LpcConnectLib::ConnectToPortHandle(HANDLE PortHandle)
{
    /* eh i dont know if i need this */
    return 1;
}

NTSTATUS
LpcConnectLib::ConnectToPortString(PCWSTR SourceString,
                                   PCWSTR SourceDesc)
{
    NTSTATUS Status;
    WCHAR* PortDesc;
    ULONG PortDescLength;
    UNICODE_STRING PortString;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;

    SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQos.ImpersonationLevel = SecurityImpersonation;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly = TRUE;

    RtlInitUnicodeString(&PortString, SourceString);
    if (SourceDesc)
    {
        PortDescLength = wcslen(SourceDesc) + 1;
        PortDesc = (WCHAR*)RtlAllocateHeap(GetProcessHeap(),
                                  0,
                                  PortDescLength * sizeof(WCHAR));
        StringCchCopyW(PortDesc, PortDescLength, SourceDesc);
    }
    else
    {
        PortDescLength = 0;
    }

    /* Now connect the port */
    Status = NtConnectPort(&InstancePort,
                           &PortString,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           PortDesc,
                           &PortDescLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to connect to port %wZ\n", &PortString);
    }

    return Status;
}

NTSTATUS
LpcConnectLib::DisconnectFromPort()
{
    NTSTATUS Status;
    if (InstancePort)
    {
        Status = NtClose(InstancePort);
        InstancePort = NULL;
    }
    return Status;
}

NTSTATUS
LpcConnectLib::SendComplexAsyncRequest(UINT32 Command,
                                       LPVOID InData,
                                       SIZE_T InSize)
{
    NTSTATUS Status;
    PLPC_MAX_MESSAGE PortMessage;

    if (InSize > MAX_MESSAGE_DATA - sizeof(ULONG) - sizeof(ULONG))
    {
        DPRINT1("Data size too large\n");
        return STATUS_INVALID_PARAMETER;
    }
    PortMessage = (PLPC_MAX_MESSAGE)RtlAllocateHeap(GetProcessHeap(),
                                                   0,
                                                   sizeof(LPC_MAX_MESSAGE) + InSize);

    RtlZeroMemory(&PortMessage, sizeof(PortMessage));
    PortMessage->Header.u1.s1.TotalLength = sizeof(PortMessage) + (USHORT)InSize;
    PortMessage->Header.u1.s1.DataLength = (USHORT)InSize + sizeof(ULONG) + sizeof(ULONG);
    RtlCopyMemory(&PortMessage->Data, InData, InSize);
    PortMessage->Message = Command;

    Status = NtRequestPort(InstancePort,
                            &PortMessage->Header);

    return Status;
}

NTSTATUS
LpcConnectLib::SendComplexSyncRequest(UINT32 Command,
                                LPVOID InData,
                                SIZE_T InSize,
                                LPVOID OutData,
                                SIZE_T OutSize,
                                HRESULT* Request)
{
    NTSTATUS Status;
    PLPC_MAX_MESSAGE PortMessage;

    if (InSize > MAX_MESSAGE_DATA - sizeof(ULONG) - sizeof(ULONG))
    {
        DPRINT1("Data size too large\n");
        return STATUS_INVALID_PARAMETER;
    }
    PortMessage = (PLPC_MAX_MESSAGE)RtlAllocateHeap(GetProcessHeap(),
                                                   0,
                                                   sizeof(LPC_MAX_MESSAGE) + InSize);

    RtlZeroMemory(&PortMessage, sizeof(PortMessage));
    PortMessage->Header.u1.s1.TotalLength = sizeof(PortMessage) + (USHORT)InSize;
    PortMessage->Header.u1.s1.DataLength = (USHORT)InSize + sizeof(ULONG) + sizeof(ULONG);
    RtlCopyMemory(&PortMessage->Data, InData, InSize);
    PortMessage->Message = Command;

    Status = NtRequestWaitReplyPort(InstancePort,
                                    &PortMessage->Header,
                                    &PortMessage->Header);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to send request to port\n");
        return Status;
    }
    RtlCopyMemory(OutData, &PortMessage->Data, OutSize);
    *Request = PortMessage->Status;
    return Status;
}

NTSTATUS
LpcConnectLib::SendSimpleAsyncRequest(UINT32 Command)
{
    NTSTATUS Status;
    PLPC_MAX_MESSAGE PortMessage;

    PortMessage = (PLPC_MAX_MESSAGE)RtlAllocateHeap(GetProcessHeap(),
                                                   0,
                                                   sizeof(LPC_MAX_MESSAGE) + sizeof(ULONG));

    /* We only need to pass a command so let's simplify that. */
    RtlZeroMemory(&PortMessage, sizeof(PortMessage));
    PortMessage->Header.u1.s1.TotalLength = sizeof(PortMessage) + sizeof(ULONG);
    PortMessage->Header.u1.s1.DataLength = sizeof(ULONG);
    PortMessage->Message = Command;

    Status = NtRequestPort(InstancePort,
                            &PortMessage->Header);
    return Status;
}