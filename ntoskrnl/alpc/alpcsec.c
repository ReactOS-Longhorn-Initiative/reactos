/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC security contexts and client impersonation
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/**
 * @brief Creates a security context capturing the caller's credentials.
 *
 * The captured context is stored in the port's handle table and its handle is
 * returned through @p SecurityAttribute->ContextHandle. The @p Flags argument
 * must be zero.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcCreateSecurityContext(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _Inout_ PALPC_SECURITY_ATTR SecurityAttribute)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_SECURITY_DATA Data;
    ALPC_SECURITY_ATTR LocalAttr;
    SECURITY_QUALITY_OF_SERVICE Qos;
    ALPC_HANDLE Handle;
    NTSTATUS Status;

    if (Flags != 0)
        return STATUS_INVALID_PARAMETER;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForWrite(SecurityAttribute, sizeof(ALPC_SECURITY_ATTR), sizeof(ULONG));
        LocalAttr = *SecurityAttribute;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Data = ExAllocatePoolWithTag(NonPagedPool, sizeof(KALPC_SECURITY_DATA), TAG_ALPC_HANDLE);
    if (Data == NULL)
    {
        ObDereferenceObject(Port);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(Data, sizeof(KALPC_SECURITY_DATA));

    Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Qos.ImpersonationLevel = SecurityImpersonation;
    Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    Qos.EffectiveOnly = FALSE;
    Status = SeCreateClientSecurity(PsGetCurrentThread(), &Qos, FALSE, &Data->DynamicSecurity);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Data, TAG_ALPC_HANDLE);
        ObDereferenceObject(Port);
        return Status;
    }

    Data->OwnerPort = Port;
    Data->OwningProcess = PsGetCurrentProcess();
    Data->HandleTable = &Port->CommunicationInfo->HandleTable;

    Handle = AlpcpInsertHandle(&Port->CommunicationInfo->HandleTable, Data);
    if (Handle == NULL)
    {
        SeDeleteClientSecurity(&Data->DynamicSecurity);
        ExFreePoolWithTag(Data, TAG_ALPC_HANDLE);
        ObDereferenceObject(Port);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Data->ContextHandle = Handle;

    LocalAttr.ContextHandle = Handle;
    _SEH2_TRY
    {
        *SecurityAttribute = LocalAttr;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ObDereferenceObject(Port);
    return Status;
}

/**
 * @brief Deletes a security context.
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcDeleteSecurityContext(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_ ALPC_HANDLE ContextHandle)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_SECURITY_DATA Data;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Data = AlpcpRemoveHandle(&Port->CommunicationInfo->HandleTable, ContextHandle);
    if (Data == NULL)
    {
        ObDereferenceObject(Port);
        return STATUS_INVALID_HANDLE;
    }

    SeDeleteClientSecurity(&Data->DynamicSecurity);
    ExFreePoolWithTag(Data, TAG_ALPC_HANDLE);

    ObDereferenceObject(Port);
    return STATUS_SUCCESS;
}

/**
 * @brief Revokes a security context so it can no longer be used.
 *
 * The handle remains valid (a later delete still succeeds); only the context is
 * marked revoked.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcRevokeSecurityContext(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_ ALPC_HANDLE ContextHandle)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_SECURITY_DATA Data;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Data = AlpcpReferenceHandle(&Port->CommunicationInfo->HandleTable, ContextHandle);
    if (Data == NULL)
    {
        ObDereferenceObject(Port);
        return STATUS_INVALID_HANDLE;
    }

    Data->u1.Revoked = TRUE;

    ObDereferenceObject(Port);
    return STATUS_SUCCESS;
}

/* ALPC port attribute flag that gates impersonation on the connected port. */
#define ALPC_PORFLG_IMPERSONATION 0x10000

/**
 * @brief Whether a received message permits impersonation through @p Port.
 *
 * Mirrors AlpcpIsImpersonationAllowed: the message must be in the pending queue
 * (QueueType 3), must not be marked for direct/callback handling (Type & 0x4000),
 * and its owning queue must be the passed port (or, for a communication port,
 * that port's connection port).
 */
static
BOOLEAN
AlpcpIsImpersonationAllowed(
    _In_ PALPC_PORT Port,
    _In_ PKALPC_MESSAGE Message)
{
    PALPC_PORT PortQueue;

    if ((Message->u1.State & 7) != 3)
        return FALSE;
    if ((Message->PortMessage.u2.s2.Type & 0x4000) != 0)
        return FALSE;

    PortQueue = Message->PortQueue;
    if (PortQueue == Port)
        return TRUE;
    if ((Port->u1.State & 6) == 6 &&
        Port->CommunicationInfo != NULL &&
        PortQueue == Port->CommunicationInfo->ConnectionPort)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Resolves the connection port that holds @p Port's received messages.
 */
static
PALPC_PORT
AlpcpResolveConnectionPort(
    _In_ PALPC_PORT Port)
{
    if ((Port->u1.State & 6) == 2)
        return Port; /* already a connection port */
    if (Port->CommunicationInfo != NULL && Port->CommunicationInfo->ConnectionPort != NULL)
        return Port->CommunicationInfo->ConnectionPort;
    return Port;
}

/**
 * @brief Impersonates the client that sent a pending message on @p Port.
 *
 * Mirrors AlpcpImpersonateMessage: gate the request, then build a client
 * security context from the sender's waiting thread (using the connection port's
 * security QoS) and impersonate. The connection port must permit impersonation.
 */
NTSTATUS
NTAPI
AlpcpImpersonateMessage(
    _In_ PALPC_PORT Port,
    _In_ ULONG MessageId)
{
    PALPC_PORT ConnectionPort;
    PKALPC_MESSAGE Message;
    PLIST_ENTRY Entry;
    PETHREAD WaitingThread = NULL;
    SECURITY_QUALITY_OF_SERVICE Qos;
    SECURITY_CLIENT_CONTEXT ClientContext;
    NTSTATUS Status;

    ConnectionPort = AlpcpResolveConnectionPort(Port);

    KeAcquireGuardedMutex(&AlpcpLock);
    for (Entry = ConnectionPort->PendingQueue.Flink;
         Entry != &ConnectionPort->PendingQueue;
         Entry = Entry->Flink)
    {
        Message = CONTAINING_RECORD(Entry, KALPC_MESSAGE, Entry);
        if (Message->PortMessage.MessageId == MessageId &&
            AlpcpIsImpersonationAllowed(Port, Message))
        {
            WaitingThread = Message->WaitingThread;
            if (WaitingThread != NULL)
                ObReferenceObject(WaitingThread);
            break;
        }
    }
    KeReleaseGuardedMutex(&AlpcpLock);

    if (WaitingThread == NULL)
        return STATUS_ACCESS_DENIED;

    /* The connection port must allow impersonation. */
    if ((ConnectionPort->PortAttributes.Flags & ALPC_PORFLG_IMPERSONATION) == 0)
    {
        ObDereferenceObject(WaitingThread);
        return STATUS_ACCESS_DENIED;
    }

    Qos = ConnectionPort->PortAttributes.SecurityQos;
    if (Qos.Length != sizeof(SECURITY_QUALITY_OF_SERVICE))
    {
        Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        Qos.ImpersonationLevel = SecurityImpersonation;
        Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Qos.EffectiveOnly = FALSE;
    }

    Status = SeCreateClientSecurity(WaitingThread, &Qos, FALSE, &ClientContext);
    if (NT_SUCCESS(Status))
    {
        Status = SeImpersonateClientEx(&ClientContext, NULL);
        SeDeleteClientSecurity(&ClientContext);
    }

    ObDereferenceObject(WaitingThread);
    return Status;
}

/**
 * @brief Impersonates the client that sent a given message.
 *
 * @return STATUS_SUCCESS on success, STATUS_ACCESS_DENIED if impersonation is
 * not permitted for the message, or another appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcImpersonateClientOfPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ PVOID Flags)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    ULONG MessageId;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Flags);

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(Message, sizeof(PORT_MESSAGE), sizeof(ULONG));
        MessageId = Message->MessageId;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = AlpcpImpersonateMessage(Port, MessageId);

    ObDereferenceObject(Port);
    return Status;
}
