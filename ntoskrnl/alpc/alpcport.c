/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC port creation, query and configuration
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* Port type stored in ALPC_PORT.u1.Type. */
#define ALPC_PORT_TYPE_CONNECTION 1
#define ALPC_PORT_TYPE_COMMUNICATION 2

/* PRIVATE FUNCTIONS ********************************************************/

/**
 * @brief Fills in the default attributes used when a caller passes none.
 *
 * Mirrors AlpcpInitializeDefaultPortAttributes in the Windows kernel.
 */
static
VOID
AlpcpInitializeDefaultPortAttributes(
    _Out_ PALPC_PORT_ATTRIBUTES PortAttributes)
{
    RtlZeroMemory(PortAttributes, sizeof(*PortAttributes));
    PortAttributes->Flags = 0;
    PortAttributes->MaxMessageLength = 256;
    PortAttributes->MemoryBandwidth = 0;
    PortAttributes->MaxPoolUsage = 0x4000;
    PortAttributes->MaxSectionSize = 0x4000;
    PortAttributes->MaxViewSize = 0;
    PortAttributes->MaxTotalSectionSize = 0x20000;
    PortAttributes->DupObjectTypes = 0;
    PortAttributes->SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    PortAttributes->SecurityQos.ImpersonationLevel = SecurityAnonymous;
    PortAttributes->SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    PortAttributes->SecurityQos.EffectiveOnly = TRUE;
}

/**
 * @brief Allocates a zeroed ALPC port object of the "ALPC Port" type.
 */
static
NTSTATUS
AlpcpCreatePort(
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Outptr_ PALPC_PORT *PortObject)
{
    NTSTATUS Status;
    PALPC_PORT Port;

    Status = ObCreateObject(PreviousMode,
                            AlpcPortObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(ALPC_PORT),
                            0,
                            0,
                            (PVOID *)&Port);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlZeroMemory(Port, sizeof(ALPC_PORT));
    *PortObject = Port;
    return STATUS_SUCCESS;
}

/**
 * @brief Initializes the queues, locks, wait object and state of a fresh port.
 *
 * @param[in,out] Port      The freshly allocated port.
 * @param[in]     Type      Port type (connection / communication).
 * @param[in]     Waitable  Whether the port owns its own semaphore.
 */
NTSTATUS
NTAPI
AlpcpInitializePort(
    _Inout_ PALPC_PORT Port,
    _In_ ULONG Type,
    _In_ BOOLEAN Waitable)
{
    PKSEMAPHORE Semaphore;

    Port->PortObjectLock.Value = 0;
    Port->ResourceListLock.Value = 0;
    Port->IncomingQueueLock.Value = 0;
    Port->PendingQueueLock.Value = 0;
    Port->DirectQueueLock.Value = 0;
    Port->WaitQueueLock.Value = 0;
    InitializeListHead(&Port->ResourceListHead);
    InitializeListHead(&Port->MainQueue);
    InitializeListHead(&Port->PendingQueue);
    InitializeListHead(&Port->LargeMessageQueue);
    InitializeListHead(&Port->CanceledQueue);
    InitializeListHead(&Port->DirectQueue);
    InitializeListHead(&Port->WaitQueue);

    Port->u1.State = (Port->u1.State & ~0x206u) |
                     ((Type & 3) << 1) |
                     ((Waitable ? 1u : 0u) << 9);

    if (Waitable)
    {
        Port->PortAttributes.Flags |= ALPC_PORFLG_WAITABLE_PORT;
        Semaphore = ExAllocatePoolWithTag(NonPagedPool, sizeof(KSEMAPHORE), TAG_ALPC_PORT);
        if (Semaphore == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
        KeInitializeSemaphore(Semaphore, 0, MAXLONG);
        Port->Semaphore = Semaphore;
    }
    else
    {
        Port->Semaphore = (PKSEMAPHORE)AlpcpDummyEvent;
    }

    Port->u1.Initialized = TRUE;

    KeAcquireGuardedMutex(&AlpcpPortListLock);
    InsertTailList(&AlpcpPortList, &Port->PortListEntry);
    KeReleaseGuardedMutex(&AlpcpPortListLock);

    return STATUS_SUCCESS;
}

/**
 * @brief Validates caller-supplied attributes and commits them to the port.
 *
 * Mirrors AlpcpValidateAndSetPortAttributes for the connection-port case.
 */
static
NTSTATUS
AlpcpValidateAndSetPortAttributes(
    _Inout_ PALPC_PORT Port,
    _In_opt_ PALPC_PORT_ATTRIBUTES CapturedAttributes,
    _In_ ULONG MaxMessageLength,
    _In_ BOOLEAN Waitable,
    _In_ BOOLEAN LegacyPort)
{
    ALPC_PORT_ATTRIBUTES DefaultAttributes;
    PALPC_PORT_ATTRIBUTES Attributes;
    ULONG Type;

    if (CapturedAttributes != NULL)
    {
        if (CapturedAttributes->MaxMessageLength < sizeof(PORT_MESSAGE) ||
            CapturedAttributes->MaxMessageLength > 0xFFEF ||
            ((CapturedAttributes->Flags & ALPC_PORFLG_SYSTEM_PROCESS) &&
             KeGetPreviousMode() != KernelMode))
        {
            return STATUS_INVALID_PARAMETER;
        }

        CapturedAttributes->Flags &= 0x03FF0000;
        CapturedAttributes->DupObjectTypes &= 0xFFD;
        Attributes = CapturedAttributes;
    }
    else
    {
        AlpcpInitializeDefaultPortAttributes(&DefaultAttributes);
        if (LegacyPort)
            DefaultAttributes.Flags |= 0x1000;
        Attributes = &DefaultAttributes;
    }

    Type = (Port->u1.State >> 1) & 3;
    if (Type == ALPC_PORT_TYPE_CONNECTION)
    {
        Attributes->Flags |= ALPC_PORFLG_ALLOW_LPC_REQUESTS;
        if (LegacyPort)
        {
            if (Waitable)
                Attributes->Flags |= ALPC_PORFLG_WAITABLE_PORT;
            Attributes->MaxMessageLength = MaxMessageLength;
        }
    }

    Port->PortAttributes = *Attributes;
    return STATUS_SUCCESS;
}

/**
 * @brief References and records the process that owns a port.
 */
static
VOID
AlpcpSetOwnerProcessPort(
    _Inout_ PALPC_PORT Port,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes)
{
    PEPROCESS Process;

    if (PortAttributes != NULL && (PortAttributes->Flags & ALPC_PORFLG_SYSTEM_PROCESS))
        Process = PsInitialSystemProcess;
    else
        Process = PsGetCurrentProcess();

    ObReferenceObject(Process);
    Port->OwnerProcess = Process;
}

/**
 * @brief Shared back-end for NtAlpcCreatePort and the legacy LPC port creation.
 *
 * Mirrors AlpcpCreateConnectionPort: build the port object, initialize it,
 * apply attributes, attach the communication info and publish a handle.
 */
NTSTATUS
NTAPI
AlpcpCreateConnectionPort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
    _In_ ULONG MaxMessageLength,
    _In_ BOOLEAN Waitable,
    _In_ BOOLEAN LegacyPort)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    ALPC_PORT_ATTRIBUTES CapturedAttributes;
    BOOLEAN HaveAttributes = FALSE;
    PALPC_PORT Port;
    HANDLE Handle;
    NTSTATUS Status;

    /* Probe outputs and capture the attribute block. */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(PortHandle);
            if (PortAttributes != NULL)
            {
                ProbeForRead(PortAttributes, sizeof(*PortAttributes), sizeof(ULONG));
                CapturedAttributes = *(volatile ALPC_PORT_ATTRIBUTES *)PortAttributes;
                HaveAttributes = TRUE;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else if (PortAttributes != NULL)
    {
        CapturedAttributes = *PortAttributes;
        HaveAttributes = TRUE;
    }

    Status = AlpcpCreatePort(PreviousMode, ObjectAttributes, &Port);
    if (!NT_SUCCESS(Status))
        return Status;

    if (HaveAttributes && (CapturedAttributes.Flags & ALPC_PORFLG_WAITABLE_PORT))
        Waitable = TRUE;

    Status = AlpcpInitializePort(Port, ALPC_PORT_TYPE_CONNECTION, Waitable);
    if (NT_SUCCESS(Status))
    {
        Status = AlpcpValidateAndSetPortAttributes(Port,
                                                   HaveAttributes ? &CapturedAttributes : NULL,
                                                   MaxMessageLength,
                                                   Waitable,
                                                   LegacyPort);
        if (NT_SUCCESS(Status))
        {
            if (LegacyPort)
                Port->u1.State |= 0x3000; /* Lpc | LpcToLpc */

            AlpcpSetOwnerProcessPort(Port, HaveAttributes ? &CapturedAttributes : NULL);

            Status = AlpcpAllocateCommunicationInfo(Port, Port);
            if (NT_SUCCESS(Status))
            {
                Status = ObInsertObject(Port, NULL, PORT_ALL_ACCESS, 0, NULL, &Handle);
                if (NT_SUCCESS(Status))
                {
                    /* ObInsertObject consumed our reference; the handle owns it now. */
                    *PortHandle = Handle;
                    return STATUS_SUCCESS;
                }
                /* ObInsertObject already dereferenced (and thus deleted) the port. */
                return Status;
            }
        }
    }

    ObDereferenceObject(Port);
    return Status;
}

/* PUBLIC FUNCTIONS ********************************************************/

/**
 * @brief Creates an ALPC connection port.
 *
 * @param[out] PortHandle        Receives the new port handle.
 * @param[in]  ObjectAttributes  Optional name/attributes for the port.
 * @param[in]  PortAttributes    Optional ALPC port attributes.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcCreatePort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes)
{
    NTSTATUS Status;

    KeEnterCriticalRegion();
    Status = AlpcpCreateConnectionPort(PortHandle, ObjectAttributes, PortAttributes, 0, FALSE, FALSE);
    KeLeaveCriticalRegion();
    return Status;
}

/**
 * @brief Disconnects a port.
 *
 * Marks only the passed port disconnected, so a subsequent send issued on *this*
 * handle fails with STATUS_PORT_DISCONNECTED. The disconnect is deliberately not
 * propagated to the paired communication port: the Win11 oracle confirms that
 * disconnecting a communication port leaves the peer's path through the live
 * connection port fully functional (its send still succeeds).
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcDisconnectPort(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       AlpcPortObjectType,
                                       PreviousMode,
                                       (PVOID *)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    KeAcquireGuardedMutex(&AlpcpLock);
    Port->u1.Disconnected = TRUE;
    KeReleaseGuardedMutex(&AlpcpLock);

    ObDereferenceObject(Port);
    return STATUS_SUCCESS;
}

/**
 * @brief Returns ALPC_BASIC_INFORMATION for a port.
 */
static
NTSTATUS
AlpcpPortQueryBasicInfo(
    _In_ PALPC_PORT Port,
    _Out_ PALPC_BASIC_INFORMATION BasicInformation,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength)
{
    if (Port == NULL)
        return STATUS_INVALID_PARAMETER;

    if (Length < sizeof(ALPC_BASIC_INFORMATION))
        return STATUS_INFO_LENGTH_MISMATCH;

    BasicInformation->Flags = Port->PortAttributes.Flags & 0x03FF0000;
    BasicInformation->SequenceNo = Port->SequenceNo;
    BasicInformation->PortContext = Port->PortContext;

    if (ReturnLength != NULL)
        *ReturnLength = sizeof(ALPC_BASIC_INFORMATION);

    return STATUS_SUCCESS;
}

/**
 * @brief Queries information about an ALPC port.
 *
 * @param[in]  PortHandle            Port to query (optional for some classes).
 * @param[in]  PortInformationClass  The class of information requested.
 * @param[out] PortInformation       Output buffer.
 * @param[in]  Length                Size of @p PortInformation.
 * @param[out] ReturnLength          Receives the number of bytes returned.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcQueryInformation(
    _In_opt_ HANDLE PortHandle,
    _In_ ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    _Inout_ PVOID PortInformation,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port = NULL;
    NTSTATUS Status;

    if (PortInformation == NULL)
        return STATUS_INVALID_PARAMETER;

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            if (Length != 0)
                ProbeForWrite(PortInformation, Length, sizeof(ULONG));
            if (ReturnLength != NULL)
                ProbeForWriteUlong(ReturnLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    if (PortHandle != NULL)
    {
        Status = ObReferenceObjectByHandle(PortHandle,
                                           READ_CONTROL,
                                           AlpcPortObjectType,
                                           PreviousMode,
                                           (PVOID *)&Port,
                                           NULL);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    switch (PortInformationClass)
    {
        case AlpcBasicInformation:
            Status = AlpcpPortQueryBasicInfo(Port, PortInformation, Length, ReturnLength);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    if (Port != NULL)
        ObDereferenceObject(Port);

    return Status;
}

/**
 * @brief Sets information on a port (completion-list register / unregister).
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcSetInformation(
    _In_ HANDLE PortHandle,
    _In_ ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    _In_opt_ PVOID PortInformation,
    _In_ ULONG Length)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    ALPC_PORT_COMPLETION_LIST_INFORMATION CompletionInfo;
    ALPC_PORT_ASSOCIATE_COMPLETION_PORT AssociateInfo;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (PortInformationClass)
    {
        case AlpcAssociateCompletionPortInformation:
        {
            PVOID CompletionObject;

            if (Length != sizeof(ALPC_PORT_ASSOCIATE_COMPLETION_PORT))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            _SEH2_TRY
            {
                if (PreviousMode != KernelMode)
                    ProbeForRead(PortInformation, sizeof(AssociateInfo), sizeof(ULONG));
                AssociateInfo = *(volatile ALPC_PORT_ASSOCIATE_COMPLETION_PORT *)PortInformation;
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            if (!NT_SUCCESS(Status))
                break;

            Status = ObReferenceObjectByHandle(AssociateInfo.CompletionPort,
                                               IO_COMPLETION_MODIFY_STATE,
                                               IoCompletionType,
                                               PreviousMode,
                                               &CompletionObject,
                                               NULL);
            if (!NT_SUCCESS(Status))
                break;

            KeAcquireGuardedMutex(&AlpcpLock);
            /* A port can only be associated with one completion port. */
            if (Port->CompletionPort != NULL)
            {
                KeReleaseGuardedMutex(&AlpcpLock);
                ObDereferenceObject(CompletionObject);
                Status = STATUS_PORT_ALREADY_SET;
                break;
            }
            Port->CompletionPort = CompletionObject;
            Port->CompletionKey = AssociateInfo.CompletionKey;
            KeReleaseGuardedMutex(&AlpcpLock);
            Status = STATUS_SUCCESS;
            break;
        }

        case AlpcRegisterCompletionListInformation:
            if (Length != sizeof(ALPC_PORT_COMPLETION_LIST_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }
            /* A completion list can only be attached to a connection port. */
            if ((Port->u1.State & 6) != 2)
            {
                Status = STATUS_INVALID_PORT_HANDLE;
                break;
            }

            _SEH2_TRY
            {
                if (PreviousMode != KernelMode)
                    ProbeForRead(PortInformation, sizeof(CompletionInfo), sizeof(ULONG));
                CompletionInfo = *(volatile ALPC_PORT_COMPLETION_LIST_INFORMATION *)PortInformation;
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            if (!NT_SUCCESS(Status))
                break;

            if (CompletionInfo.Buffer == NULL || CompletionInfo.Size == 0)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            Status = AlpcpRegisterCompletionList(Port, CompletionInfo.Buffer,
                                                 CompletionInfo.Size, PreviousMode);
            break;

        case AlpcUnregisterCompletionListInformation:
            Status = AlpcpUnregisterCompletionList(Port);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    ObDereferenceObject(Port);
    return Status;
}
