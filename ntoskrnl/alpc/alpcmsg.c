/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC message allocation, transmission and reception
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* Native ALPC types both data requests and replies as LPC_REQUEST; the high
 * bits mirror what the Windows kernel folds into the type field. */
#define ALPC_REQUEST_TYPE (0x3000 | LPC_REQUEST)
#define ALPC_RECEIVED_FLAG 0x2000

/* MESSAGE ALLOCATION ******************************************************/

static
ULONG
AlpcpAllocateMessageId(VOID)
{
    LONG Id;

    do
    {
        Id = InterlockedIncrement(&AlpcpNextMessageId);
    } while (Id == 0);

    return (ULONG)Id & 0x7FFFFFFF;
}

PKALPC_MESSAGE
NTAPI
AlpcpAllocateMessage(
    _In_ ULONG DataLength)
{
    PKALPC_MESSAGE Message;
    SIZE_T Size;

    Size = FIELD_OFFSET(KALPC_MESSAGE, PortMessage) + sizeof(PORT_MESSAGE) + DataLength;
    Message = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_ALPC_MESSAGE);
    if (Message == NULL)
        return NULL;

    RtlZeroMemory(Message, Size);
    InitializeListHead(&Message->Entry);
    Message->PortMessage.u1.s1.TotalLength = (CSHORT)(sizeof(PORT_MESSAGE) + DataLength);
    Message->PortMessage.u1.s1.DataLength = (CSHORT)DataLength;
    Message->PortMessage.MessageId = AlpcpAllocateMessageId();
    return Message;
}

VOID
NTAPI
AlpcpFreeMessage(
    _In_ PKALPC_MESSAGE Message)
{
    /* A message carrying a VIEW attribute holds a reference on the section. */
    if (Message->ExtensionBuffer != NULL)
        ObDereferenceObject(Message->ExtensionBuffer);

    /* A message carrying a HANDLE attribute holds a reference on the sender. */
    if (Message->MessageAttributes.HandleData != NULL)
    {
        if (Message->MessageAttributes.HandleData->DuplicateContext.SourceProcess != NULL)
            ObDereferenceObject(Message->MessageAttributes.HandleData->DuplicateContext.SourceProcess);
        ExFreePoolWithTag(Message->MessageAttributes.HandleData, TAG_ALPC_HANDLE);
    }

    ExFreePoolWithTag(Message, TAG_ALPC_MESSAGE);
}

/* MESSAGE ATTRIBUTES *****************************************************/

/**
 * @brief Byte offset of an attribute within an ALPC_MESSAGE_ATTRIBUTES buffer.
 *
 * Attributes follow the header in a fixed order; returns 0 if not allocated.
 */
static
ULONG
AlpcpAttributeOffset(
    _In_ ULONG AllocatedAttributes,
    _In_ ULONG AttributeFlag)
{
    ULONG Offset = sizeof(ALPC_MESSAGE_ATTRIBUTES);

#define ALPC_ATTR_STEP(Flag, Type)                              \
    if ((AttributeFlag) == (Flag)) return Offset;               \
    if (AllocatedAttributes & (Flag)) Offset += sizeof(Type)

    ALPC_ATTR_STEP(ALPC_MESSAGE_SECURITY_ATTRIBUTE, ALPC_SECURITY_ATTR);
    ALPC_ATTR_STEP(ALPC_MESSAGE_VIEW_ATTRIBUTE, ALPC_DATA_VIEW_ATTR);
    ALPC_ATTR_STEP(ALPC_MESSAGE_CONTEXT_ATTRIBUTE, ALPC_CONTEXT_ATTR);
    ALPC_ATTR_STEP(ALPC_MESSAGE_HANDLE_ATTRIBUTE, ALPC_HANDLE_ATTR);
    ALPC_ATTR_STEP(ALPC_MESSAGE_TOKEN_ATTRIBUTE, ALPC_TOKEN_ATTR);
#undef ALPC_ATTR_STEP

    return 0;
}

/**
 * @brief Maps a message's transferred section into the receiver and fills in the
 *        VIEW attribute of the caller's receive-attribute buffer.
 */
static
VOID
AlpcpExposeViewAttribute(
    _In_ PALPC_PORT Port,
    _In_ PKALPC_MESSAGE Message,
    _Inout_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    ULONG Allocated;
    ULONG Offset;
    PVOID ViewBase;
    SIZE_T ViewSize;
    NTSTATUS Status;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(ReceiveMessageAttributes, sizeof(ALPC_MESSAGE_ATTRIBUTES), sizeof(ULONG));
        Allocated = ReceiveMessageAttributes->AllocatedAttributes;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return);
    }
    _SEH2_END;

    if (!(Allocated & ALPC_MESSAGE_VIEW_ATTRIBUTE))
        return;

    Offset = AlpcpAttributeOffset(Allocated, ALPC_MESSAGE_VIEW_ATTRIBUTE);
    if (Offset == 0)
        return;

    Status = AlpcpMapReceivedView(Port, Message->ExtensionBuffer,
                                  Message->ExtensionBufferSize, &ViewBase, &ViewSize);
    if (!NT_SUCCESS(Status))
        return;

    _SEH2_TRY
    {
        PALPC_DATA_VIEW_ATTR Dst = (PALPC_DATA_VIEW_ATTR)((PUCHAR)ReceiveMessageAttributes + Offset);

        if (PreviousMode != KernelMode)
            ProbeForWrite(Dst, sizeof(ALPC_DATA_VIEW_ATTR), sizeof(ULONG));

        Dst->Flags = 0;
        Dst->SectionHandle = NULL;
        Dst->ViewBase = ViewBase;
        Dst->ViewSize = ViewSize;
        ReceiveMessageAttributes->ValidAttributes |= ALPC_MESSAGE_VIEW_ATTRIBUTE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }
    _SEH2_END;
}

/* WAIT-QUEUE HELPERS ******************************************************/

FORCEINLINE
BOOLEAN
AlpcpThreadEnrolled(
    _In_ PETHREAD Thread)
{
    return Thread->AlpcWaitListEntry.Flink != NULL &&
           Thread->AlpcWaitListEntry.Flink != &Thread->AlpcWaitListEntry;
}

PETHREAD
NTAPI
AlpcpDequeueReceiver(
    _In_ PALPC_PORT Port)
{
    PLIST_ENTRY Entry;

    if (IsListEmpty(&Port->WaitQueue))
        return NULL;

    Entry = RemoveHeadList(&Port->WaitQueue);
    Port->WaitQueueLength--;
    InitializeListHead(Entry);
    return CONTAINING_RECORD(Entry, ETHREAD, AlpcWaitListEntry);
}

/**
 * @brief Duplicates a message's transferred handle into the receiver and fills
 *        in the HANDLE attribute of the caller's receive-attribute buffer.
 */
static
VOID
AlpcpExposeHandleAttribute(
    _In_ PKALPC_MESSAGE Message,
    _Inout_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PKALPC_HANDLE_DATA HandleData = Message->MessageAttributes.HandleData;
    ULONG Allocated;
    ULONG Offset;
    ULONG Options = 0;
    ULONG HandleAttributes = 0;
    HANDLE TargetHandle = NULL;
    NTSTATUS Status;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(ReceiveMessageAttributes, sizeof(ALPC_MESSAGE_ATTRIBUTES), sizeof(ULONG));
        Allocated = ReceiveMessageAttributes->AllocatedAttributes;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return);
    }
    _SEH2_END;

    if (!(Allocated & ALPC_MESSAGE_HANDLE_ATTRIBUTE))
        return;

    Offset = AlpcpAttributeOffset(Allocated, ALPC_MESSAGE_HANDLE_ATTRIBUTE);
    if (Offset == 0)
        return;

    if (HandleData->Count & ALPC_HANDLEFLG_DUPLICATE_SAME_ACCESS)
        Options |= DUPLICATE_SAME_ACCESS;
    if (HandleData->Count & ALPC_HANDLEFLG_DUPLICATE_SAME_ATTRIBUTES)
        Options |= DUPLICATE_SAME_ATTRIBUTES;
    if (HandleData->Count & ALPC_HANDLEFLG_DUPLICATE_INHERIT)
        HandleAttributes |= OBJ_INHERIT;

    Status = ObDuplicateObject(HandleData->DuplicateContext.SourceProcess,
                               HandleData->DuplicateContext.SourceHandle,
                               PsGetCurrentProcess(),
                               &TargetHandle,
                               HandleData->DuplicateContext.TargetAccess,
                               HandleAttributes,
                               Options,
                               KernelMode);
    if (!NT_SUCCESS(Status))
        return;

    _SEH2_TRY
    {
        PALPC_HANDLE_ATTR Dst = (PALPC_HANDLE_ATTR)((PUCHAR)ReceiveMessageAttributes + Offset);

        if (PreviousMode != KernelMode)
            ProbeForWrite(Dst, sizeof(ALPC_HANDLE_ATTR), sizeof(ULONG));

        Dst->Flags = (ULONG)HandleData->Count;
        Dst->Handle = TargetHandle;
        Dst->ObjectType = 0;
        Dst->DesiredAccess = HandleData->DuplicateContext.TargetAccess;
        ReceiveMessageAttributes->ValidAttributes |= ALPC_MESSAGE_HANDLE_ATTRIBUTE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ObCloseHandle(TargetHandle, PreviousMode);
    }
    _SEH2_END;
}

/**
 * @brief Fills in the CONTEXT attribute of the caller's receive-attribute buffer
 *        (message id, sequence, and the receiving port's context).
 */
static
VOID
AlpcpExposeContextAttribute(
    _In_ PALPC_PORT Port,
    _In_ PKALPC_MESSAGE Message,
    _Inout_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    ULONG Allocated;
    ULONG Offset;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(ReceiveMessageAttributes, sizeof(ALPC_MESSAGE_ATTRIBUTES), sizeof(ULONG));
        Allocated = ReceiveMessageAttributes->AllocatedAttributes;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return);
    }
    _SEH2_END;

    if (!(Allocated & ALPC_MESSAGE_CONTEXT_ATTRIBUTE))
        return;

    Offset = AlpcpAttributeOffset(Allocated, ALPC_MESSAGE_CONTEXT_ATTRIBUTE);
    if (Offset == 0)
        return;

    _SEH2_TRY
    {
        PALPC_CONTEXT_ATTR Dst = (PALPC_CONTEXT_ATTR)((PUCHAR)ReceiveMessageAttributes + Offset);

        if (PreviousMode != KernelMode)
            ProbeForWrite(Dst, sizeof(ALPC_CONTEXT_ATTR), sizeof(ULONG));

        Dst->PortContext = Port->PortContext;
        Dst->MessageContext = NULL;
        Dst->Sequence = (ULONG)Message->SequenceNo;
        Dst->MessageId = Message->PortMessage.MessageId;
        Dst->CallbackId = 0;
        ReceiveMessageAttributes->ValidAttributes |= ALPC_MESSAGE_CONTEXT_ATTRIBUTE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }
    _SEH2_END;
}

/**
 * @brief Fills in the TOKEN attribute (the sender's token / authentication /
 *        modified ids) of the caller's receive-attribute buffer.
 */
static
VOID
AlpcpExposeTokenAttribute(
    _In_ PKALPC_MESSAGE Message,
    _Inout_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    ULONG Allocated;
    ULONG Offset;
    PEPROCESS Process;
    PACCESS_TOKEN Token;
    PTOKEN_STATISTICS Statistics;
    NTSTATUS Status;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(ReceiveMessageAttributes, sizeof(ALPC_MESSAGE_ATTRIBUTES), sizeof(ULONG));
        Allocated = ReceiveMessageAttributes->AllocatedAttributes;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return);
    }
    _SEH2_END;

    if (!(Allocated & ALPC_MESSAGE_TOKEN_ATTRIBUTE))
        return;

    Offset = AlpcpAttributeOffset(Allocated, ALPC_MESSAGE_TOKEN_ATTRIBUTE);
    if (Offset == 0)
        return;

    Status = PsLookupProcessByProcessId(Message->PortMessage.ClientId.UniqueProcess, &Process);
    if (!NT_SUCCESS(Status))
        return;

    Token = PsReferencePrimaryToken(Process);
    Status = SeQueryInformationToken(Token, TokenStatistics, (PVOID *)&Statistics);
    PsDereferencePrimaryToken(Token);
    ObDereferenceObject(Process);
    if (!NT_SUCCESS(Status))
        return;

    _SEH2_TRY
    {
        PALPC_TOKEN_ATTR Dst = (PALPC_TOKEN_ATTR)((PUCHAR)ReceiveMessageAttributes + Offset);

        if (PreviousMode != KernelMode)
            ProbeForWrite(Dst, sizeof(ALPC_TOKEN_ATTR), sizeof(ULONG));

        Dst->TokenId = Statistics->TokenId;
        Dst->AuthenticationId = Statistics->AuthenticationId;
        Dst->ModifiedId = Statistics->ModifiedId;
        ReceiveMessageAttributes->ValidAttributes |= ALPC_MESSAGE_TOKEN_ATTRIBUTE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }
    _SEH2_END;

    ExFreePool(Statistics);
}

/**
 * @brief Exposes a transferred SECURITY attribute to the receiver.
 *
 * A SECURITY attribute is only delivered when the sender attached a security
 * context (created with NtAlpcCreateSecurityContext) to the message; for a plain
 * message nothing is exposed and the ValidAttributes bit stays clear, matching
 * the Win11 oracle. When present, the receiver gets the context's opaque handle.
 */
static
VOID
AlpcpExposeSecurityAttribute(
    _In_ PKALPC_MESSAGE Message,
    _Inout_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PKALPC_SECURITY_DATA SecurityData = Message->MessageAttributes.SecurityData;
    ULONG Allocated;
    ULONG Offset;

    if (SecurityData == NULL)
        return;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(ReceiveMessageAttributes, sizeof(ALPC_MESSAGE_ATTRIBUTES), sizeof(ULONG));
        Allocated = ReceiveMessageAttributes->AllocatedAttributes;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return);
    }
    _SEH2_END;

    if (!(Allocated & ALPC_MESSAGE_SECURITY_ATTRIBUTE))
        return;

    Offset = AlpcpAttributeOffset(Allocated, ALPC_MESSAGE_SECURITY_ATTRIBUTE);
    if (Offset == 0)
        return;

    _SEH2_TRY
    {
        PALPC_SECURITY_ATTR Dst = (PALPC_SECURITY_ATTR)((PUCHAR)ReceiveMessageAttributes + Offset);

        if (PreviousMode != KernelMode)
            ProbeForWrite(Dst, sizeof(ALPC_SECURITY_ATTR), sizeof(ULONG));

        Dst->Flags = 0;
        Dst->QoS = NULL;
        Dst->ContextHandle = (ALPC_HANDLE)SecurityData->ContextHandle;
        ReceiveMessageAttributes->ValidAttributes |= ALPC_MESSAGE_SECURITY_ATTRIBUTE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }
    _SEH2_END;
}

/* RECEIVE *****************************************************************/

/**
 * @brief Waits for and dequeues the next message on a port, copying it into the
 *        caller's receive buffer.
 *
 * After delivery a synchronous request (one whose sender is blocked waiting for
 * a reply) is moved to the port's pending queue so a later reply can be matched
 * to it by message id. A connection request stays owned by the handshake; a
 * datagram is freed.
 */
NTSTATUS
NTAPI
AlpcpReceiveMessage(
    _In_ PALPC_PORT Port,
    _Out_ PPORT_MESSAGE ReceiveMessage,
    _Inout_ PSIZE_T BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _Out_opt_ PVOID *OutServerContext,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    PETHREAD Thread = PsGetCurrentThread();
    PKALPC_MESSAGE Message;
    PLIST_ENTRY Entry;
    SIZE_T UserBufferLength = *BufferLength;
    ULONG TotalLength;
    USHORT BaseType;
    NTSTATUS Status;

    /* Messages are queued on the connection port. A receiver may wait on a
     * communication port (e.g. CSR receives on the per-client port after setting
     * it as the reply port), so operate on the connection port that actually
     * holds the queue and wait list. */
    if (Port->CommunicationInfo != NULL && Port->CommunicationInfo->ConnectionPort != NULL)
        Port = Port->CommunicationInfo->ConnectionPort;

    KeInitializeSemaphore(&Thread->AlpcWaitSemaphore, 0, MAXLONG);
    InitializeListHead(&Thread->AlpcWaitListEntry);

    for (;;)
    {
        KeAcquireGuardedMutex(&AlpcpLock);

        if (!IsListEmpty(&Port->MainQueue))
        {
            Entry = RemoveHeadList(&Port->MainQueue);
            Port->MainQueueLength--;
            Message = CONTAINING_RECORD(Entry, KALPC_MESSAGE, Entry);
            Message->u1.QueueType = 0;
            InitializeListHead(&Message->Entry);

            if (AlpcpThreadEnrolled(Thread))
            {
                RemoveEntryList(&Thread->AlpcWaitListEntry);
                Port->WaitQueueLength--;
                InitializeListHead(&Thread->AlpcWaitListEntry);
            }
            KeReleaseGuardedMutex(&AlpcpLock);

            TotalLength = Message->PortMessage.u1.s1.TotalLength;
            if (TotalLength > UserBufferLength)
            {
                KeAcquireGuardedMutex(&AlpcpLock);
                InsertHeadList(&Port->MainQueue, &Message->Entry);
                Port->MainQueueLength++;
                Message->u1.QueueType = 1;
                KeReleaseGuardedMutex(&AlpcpLock);
                return STATUS_BUFFER_TOO_SMALL;
            }

            _SEH2_TRY
            {
                if (PreviousMode != KernelMode)
                    ProbeForWrite(ReceiveMessage, TotalLength, sizeof(ULONG));

                RtlCopyMemory(ReceiveMessage, &Message->PortMessage, TotalLength);
                *BufferLength = TotalLength;
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Resolve the per-client port context for legacy LPC receivers: the
             * context the server set at NtAcceptConnectPort lives on the server
             * communication port paired with the sender's client port. */
            if (OutServerContext != NULL)
            {
                PALPC_PORT ServerComm = NULL;
                if (Message->OwnerPort != NULL &&
                    Message->OwnerPort->CommunicationInfo != NULL)
                {
                    ServerComm = Message->OwnerPort->CommunicationInfo->ServerCommunicationPort;
                }
                *OutServerContext = (ServerComm != NULL) ? ServerComm->PortContext : NULL;
            }

            /* Expose any transferred VIEW attribute to the receiver. */
            if (NT_SUCCESS(Status) &&
                ReceiveMessageAttributes != NULL &&
                Message->ExtensionBuffer != NULL)
            {
                AlpcpExposeViewAttribute(Port, Message, ReceiveMessageAttributes, PreviousMode);
            }

            /* Expose any transferred HANDLE attribute to the receiver. */
            if (NT_SUCCESS(Status) &&
                ReceiveMessageAttributes != NULL &&
                Message->MessageAttributes.HandleData != NULL)
            {
                AlpcpExposeHandleAttribute(Message, ReceiveMessageAttributes, PreviousMode);
            }

            /* Expose the CONTEXT attribute (message id / sequence) if requested. */
            if (NT_SUCCESS(Status) && ReceiveMessageAttributes != NULL)
                AlpcpExposeContextAttribute(Port, Message, ReceiveMessageAttributes, PreviousMode);

            /* Expose the TOKEN attribute (sender token ids) if requested. */
            if (NT_SUCCESS(Status) && ReceiveMessageAttributes != NULL)
                AlpcpExposeTokenAttribute(Message, ReceiveMessageAttributes, PreviousMode);

            /* Expose a transferred SECURITY attribute (sender's context). */
            if (NT_SUCCESS(Status) &&
                ReceiveMessageAttributes != NULL &&
                Message->MessageAttributes.SecurityData != NULL)
            {
                AlpcpExposeSecurityAttribute(Message, ReceiveMessageAttributes, PreviousMode);
            }

            BaseType = Message->PortMessage.u2.s2.Type & 0xFF;
            if (BaseType == LPC_CONNECTION_REQUEST)
            {
                /* Owned by the connect/accept handshake; leave it referenced. */
            }
            else if (NT_SUCCESS(Status))
            {
                /* Pend the received message - a synchronous request (for reply
                 * correlation) or a datagram (so a sender query such as
                 * OpenSenderProcess or the message SID can still find it). It is
                 * released by the reply, or when the port is torn down. */
                KeAcquireGuardedMutex(&AlpcpLock);
                InsertTailList(&Port->PendingQueue, &Message->Entry);
                Port->PendingQueueLength++;
                Message->u1.QueueType = 3;
                Message->PortQueue = Port;
                Message->PortMessage.u2.s2.Type |= ALPC_RECEIVED_FLAG;
                KeReleaseGuardedMutex(&AlpcpLock);
            }
            else
            {
                AlpcpFreeMessage(Message);
            }

            return Status;
        }

        if (!AlpcpThreadEnrolled(Thread))
        {
            InsertTailList(&Port->WaitQueue, &Thread->AlpcWaitListEntry);
            Port->WaitQueueLength++;
        }
        KeReleaseGuardedMutex(&AlpcpLock);

        Status = KeWaitForSingleObject(&Thread->AlpcWaitSemaphore,
                                       WrLpcReceive,
                                       PreviousMode,
                                       FALSE,
                                       Timeout);
        if (Status == STATUS_TIMEOUT)
        {
            KeAcquireGuardedMutex(&AlpcpLock);
            if (AlpcpThreadEnrolled(Thread))
            {
                RemoveEntryList(&Thread->AlpcWaitListEntry);
                Port->WaitQueueLength--;
                InitializeListHead(&Thread->AlpcWaitListEntry);
            }
            if (!IsListEmpty(&Port->MainQueue))
            {
                KeReleaseGuardedMutex(&AlpcpLock);
                continue;
            }
            KeReleaseGuardedMutex(&AlpcpLock);
            return STATUS_TIMEOUT;
        }
        else if (Status != STATUS_SUCCESS)
        {
            return Status;
        }
        /* Woken by a sender; loop to collect the message. */
    }
}

/* SEND ******************************************************************/

/**
 * @brief Sends a data message (request or datagram), routed from a communication
 *        port to its connection port, optionally waiting for the reply.
 */
NTSTATUS
NTAPI
AlpcpSendRequest(
    _In_ PALPC_PORT SourcePort,
    _In_ PPORT_MESSAGE Header,
    _In_reads_bytes_opt_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _In_ ULONG Flags,
    _In_opt_ PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
    _Out_opt_ PPORT_MESSAGE ReceiveMessage,
    _Inout_opt_ PSIZE_T BufferLength,
    _In_ SIZE_T UserBufferLength,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    PETHREAD Thread = PsGetCurrentThread();
    PALPC_PORT TargetPort;
    PKALPC_MESSAGE Message;
    PKALPC_MESSAGE Reply;
    PETHREAD Receiver;
    PVOID CompletionPort;
    PVOID CompletionKey;
    ULONG MessageId;
    BOOLEAN SyncRequest = (Flags & ALPC_MSGFLG_SYNC_REQUEST) != 0;
    ULONG TotalLength = Header->u1.s1.TotalLength;
    NTSTATUS Status;

    /* Send-side state gate (order matches the Windows kernel). */
    if (SourcePort->u1.ConnectionRefused)
        return STATUS_PORT_CONNECTION_REFUSED;
    if (SourcePort->u1.ConnectionPending)
        return STATUS_LPC_REQUESTS_NOT_ALLOWED;
    if (SourcePort->u1.Disconnected)
        return STATUS_PORT_DISCONNECTED;

    if (SourcePort->CommunicationInfo == NULL ||
        SourcePort->CommunicationInfo->ConnectionPort == NULL)
    {
        return STATUS_INVALID_PORT_HANDLE;
    }
    TargetPort = SourcePort->CommunicationInfo->ConnectionPort;

    /* Message-format validation. */
    if (TotalLength > SourcePort->PortAttributes.MaxMessageLength)
        return STATUS_PORT_MESSAGE_TOO_LONG;
    if ((ULONG)DataLength + sizeof(PORT_MESSAGE) != TotalLength)
        return STATUS_INVALID_PARAMETER;

    /* A message id with the high bit set carries a resource-reserve handle: claim
     * the reserve before sending (one in-flight send per reserve). */
    if ((LONG)Header->MessageId < 0)
    {
        Status = AlpcpConsumeReserve(SourcePort, Header->MessageId);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    /* A datagram targeting a port with a completion list is delivered into its
     * ring; the server collects it with AlpcGetMessageFromCompletionList instead
     * of blocking in receive. Fall through to the normal queue if it cannot fit. */
    if (!SyncRequest && TargetPort->CompletionList != NULL)
    {
        if (AlpcpInsertCompletionList(TargetPort, Thread->Cid, Data, DataLength))
            return STATUS_SUCCESS;
    }

    Message = AlpcpAllocateMessage(DataLength);
    if (Message == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Message->PortMessage.u2.s2.Type = ALPC_REQUEST_TYPE;
    Message->PortMessage.ClientId = Thread->Cid;
    Message->OwnerPort = SourcePort;
    Message->ConnectionPort = TargetPort;
    if (DataLength != 0)
        RtlCopyMemory(AlpcpGetMessageData(Message), Data, DataLength);

    /* Attach a transferred VIEW attribute (shared section) to the message. */
    if (SendMessageAttributes != NULL)
    {
        ULONG Allocated = 0;
        ULONG Valid = 0;
        ALPC_DATA_VIEW_ATTR SendView;

        SendView.ViewBase = NULL;
        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForRead(SendMessageAttributes, sizeof(ALPC_MESSAGE_ATTRIBUTES), sizeof(ULONG));
            Allocated = SendMessageAttributes->AllocatedAttributes;
            Valid = SendMessageAttributes->ValidAttributes;

            if ((Valid & Allocated & ALPC_MESSAGE_VIEW_ATTRIBUTE) != 0)
            {
                ULONG Offset = AlpcpAttributeOffset(Allocated, ALPC_MESSAGE_VIEW_ATTRIBUTE);
                if (Offset != 0)
                {
                    PALPC_DATA_VIEW_ATTR Src =
                        (PALPC_DATA_VIEW_ATTR)((PUCHAR)SendMessageAttributes + Offset);
                    if (PreviousMode != KernelMode)
                        ProbeForRead(Src, sizeof(ALPC_DATA_VIEW_ATTR), sizeof(ULONG));
                    SendView = *Src;
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SendView.ViewBase = NULL;
        }
        _SEH2_END;

        if (SendView.ViewBase != NULL)
        {
            ULONG ViewSize = 0;
            PVOID SectionObject = AlpcpReferenceViewSection(SourcePort, SendView.ViewBase, &ViewSize);
            if (SectionObject != NULL)
            {
                Message->ExtensionBuffer = SectionObject;
                Message->ExtensionBufferSize = ViewSize;
            }
        }
    }

    /* Attach a transferred HANDLE attribute: reference the sender process so the
     * object can be duplicated into the receiver when the message is received. */
    if (SendMessageAttributes != NULL)
    {
        ULONG Allocated = 0;
        ULONG Valid = 0;
        ALPC_HANDLE_ATTR SendHandle;

        SendHandle.Handle = NULL;
        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForRead(SendMessageAttributes, sizeof(ALPC_MESSAGE_ATTRIBUTES), sizeof(ULONG));
            Allocated = SendMessageAttributes->AllocatedAttributes;
            Valid = SendMessageAttributes->ValidAttributes;

            if ((Valid & Allocated & ALPC_MESSAGE_HANDLE_ATTRIBUTE) != 0)
            {
                ULONG Offset = AlpcpAttributeOffset(Allocated, ALPC_MESSAGE_HANDLE_ATTRIBUTE);
                if (Offset != 0)
                {
                    PALPC_HANDLE_ATTR Src =
                        (PALPC_HANDLE_ATTR)((PUCHAR)SendMessageAttributes + Offset);
                    if (PreviousMode != KernelMode)
                        ProbeForRead(Src, sizeof(ALPC_HANDLE_ATTR), sizeof(ULONG));
                    SendHandle = *Src;
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SendHandle.Handle = NULL;
        }
        _SEH2_END;

        if (SendHandle.Handle != NULL)
        {
            PKALPC_HANDLE_DATA HandleData =
                ExAllocatePoolWithTag(NonPagedPool, sizeof(KALPC_HANDLE_DATA), TAG_ALPC_HANDLE);
            if (HandleData != NULL)
            {
                PEPROCESS Self = PsGetCurrentProcess();

                RtlZeroMemory(HandleData, sizeof(KALPC_HANDLE_DATA));
                ObReferenceObject(Self);
                HandleData->Count = SendHandle.Flags;
                HandleData->DuplicateContext.SourceProcess = Self;
                HandleData->DuplicateContext.SourceHandle = SendHandle.Handle;
                HandleData->DuplicateContext.TargetAccess = SendHandle.DesiredAccess;
                Message->MessageAttributes.HandleData = HandleData;
            }
        }
    }

    /* Attach a transferred SECURITY attribute: resolve the sender's security
     * context (created with NtAlpcCreateSecurityContext) so the receiver can
     * learn its opaque handle. The context lives in the port's handle table for
     * the lifetime of the connection. */
    if (SendMessageAttributes != NULL)
    {
        ULONG Allocated = 0;
        ULONG Valid = 0;
        ALPC_HANDLE ContextHandle = NULL;

        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForRead(SendMessageAttributes, sizeof(ALPC_MESSAGE_ATTRIBUTES), sizeof(ULONG));
            Allocated = SendMessageAttributes->AllocatedAttributes;
            Valid = SendMessageAttributes->ValidAttributes;

            if ((Valid & Allocated & ALPC_MESSAGE_SECURITY_ATTRIBUTE) != 0)
            {
                ULONG Offset = AlpcpAttributeOffset(Allocated, ALPC_MESSAGE_SECURITY_ATTRIBUTE);
                if (Offset != 0)
                {
                    PALPC_SECURITY_ATTR Src =
                        (PALPC_SECURITY_ATTR)((PUCHAR)SendMessageAttributes + Offset);
                    if (PreviousMode != KernelMode)
                        ProbeForRead(Src, sizeof(ALPC_SECURITY_ATTR), sizeof(ULONG));
                    ContextHandle = Src->ContextHandle;
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ContextHandle = NULL;
        }
        _SEH2_END;

        if (ContextHandle != NULL && SourcePort->CommunicationInfo != NULL)
        {
            PKALPC_SECURITY_DATA SecurityData =
                AlpcpReferenceHandle(&SourcePort->CommunicationInfo->HandleTable, ContextHandle);
            if (SecurityData != NULL)
                Message->MessageAttributes.SecurityData = SecurityData;
        }
    }

    if (SyncRequest)
    {
        Message->WaitingThread = Thread;
        KeInitializeSemaphore(&Thread->AlpcWaitSemaphore, 0, MAXLONG);
        Thread->AlpcMessage = NULL;
        Thread->AlpcMessageId = Message->PortMessage.MessageId;
    }

    KeAcquireGuardedMutex(&AlpcpLock);
    Message->SequenceNo = ++TargetPort->SequenceNo;
    InsertTailList(&TargetPort->MainQueue, &Message->Entry);
    TargetPort->MainQueueLength++;
    Message->u1.QueueType = 1;
    Receiver = AlpcpDequeueReceiver(TargetPort);
    CompletionPort = (Receiver == NULL) ? TargetPort->CompletionPort : NULL;
    CompletionKey = TargetPort->CompletionKey;
    MessageId = Message->PortMessage.MessageId;
    KeReleaseGuardedMutex(&AlpcpLock);

    if (Receiver != NULL)
    {
        KeReleaseSemaphore(&Receiver->AlpcWaitSemaphore, 0, 1, FALSE);
    }
    else if (CompletionPort != NULL)
    {
        /* No thread is blocked in receive; notify the associated I/O completion
         * port so a worker can pick up the queued message. */
        IoSetIoCompletion(CompletionPort, CompletionKey, NULL,
                          STATUS_SUCCESS, MessageId, FALSE);
    }

    if (!SyncRequest)
        return STATUS_SUCCESS;

    Status = KeWaitForSingleObject(&Thread->AlpcWaitSemaphore,
                                   WrLpcReply,
                                   PreviousMode,
                                   FALSE,
                                   Timeout);

    Reply = (PKALPC_MESSAGE)Thread->AlpcMessage;
    Thread->AlpcMessage = NULL;
    Thread->AlpcMessageId = 0;

    if (Status == STATUS_SUCCESS && Reply == ALPC_WAKE_CANCELED)
    {
        /* The request was cancelled by the peer (it was already detached from the
         * pending queue); reclaim it and report the cancellation. */
        KeAcquireGuardedMutex(&AlpcpLock);
        if (Message->u1.QueueType == 1)
        {
            RemoveEntryList(&Message->Entry);
            TargetPort->MainQueueLength--;
        }
        else if (Message->u1.QueueType == 3)
        {
            RemoveEntryList(&Message->Entry);
            TargetPort->PendingQueueLength--;
        }
        Message->u1.QueueType = 0;
        KeReleaseGuardedMutex(&AlpcpLock);
        AlpcpFreeMessage(Message);
        /*
         * The peer cancelled an in-flight request: the blocked sender reports
         * STATUS_MESSAGE_LOST, not STATUS_CANCELLED. Verified on the Win11 oracle
         * (a server NtAlpcCancelMessage leaves the client's send returning
         * 0xC0000701).
         */
        return STATUS_MESSAGE_LOST;
    }

    if (Status == STATUS_SUCCESS && Reply != NULL)
    {
        /* The reply path detached our request from the pending queue; we still
         * own the request message and free it here along with the reply. */
        ULONG ReplyLength = Reply->PortMessage.u1.s1.TotalLength;

        if (ReceiveMessage != NULL && BufferLength != NULL && ReplyLength <= UserBufferLength)
        {
            _SEH2_TRY
            {
                if (PreviousMode != KernelMode)
                    ProbeForWrite(ReceiveMessage, ReplyLength, sizeof(ULONG));

                RtlCopyMemory(ReceiveMessage, &Reply->PortMessage, ReplyLength);
                *BufferLength = ReplyLength;
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
        else if (ReceiveMessage != NULL)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        AlpcpFreeMessage(Reply);
        AlpcpFreeMessage(Message);
        return Status;
    }

    /* No reply (timeout or failure): reclaim the request if still queued. */
    KeAcquireGuardedMutex(&AlpcpLock);
    if (Message->u1.QueueType == 1)
    {
        RemoveEntryList(&Message->Entry);
        TargetPort->MainQueueLength--;
        Message->u1.QueueType = 0;
    }
    else if (Message->u1.QueueType == 3)
    {
        RemoveEntryList(&Message->Entry);
        TargetPort->PendingQueueLength--;
        Message->u1.QueueType = 0;
    }
    KeReleaseGuardedMutex(&AlpcpLock);
    AlpcpFreeMessage(Message);
    if (Reply != NULL)
        AlpcpFreeMessage(Reply);

    return (Status == STATUS_SUCCESS) ? STATUS_TIMEOUT : Status;
}

/**
 * @brief Sends a reply, matching it to a pending request by message id and
 *        waking the blocked sender.
 */
NTSTATUS
NTAPI
AlpcpSendReply(
    _In_ PALPC_PORT ReplyPort,
    _In_ PPORT_MESSAGE Header,
    _In_reads_bytes_opt_(DataLength) PVOID Data,
    _In_ ULONG DataLength)
{
    PKALPC_MESSAGE Request = NULL;
    PKALPC_MESSAGE Reply;
    PETHREAD WaitingThread;
    PLIST_ENTRY Entry;
    PALPC_PORT QueuePort = ReplyPort;
    ULONG MessageId = Header->MessageId;

    /* Requests are pended on the connection port. A reply may be issued on a
     * communication port (e.g. CSR replies on the per-client port), so resolve
     * to the connection port that actually holds the pending request. */
    if (ReplyPort->CommunicationInfo != NULL &&
        ReplyPort->CommunicationInfo->ConnectionPort != NULL)
    {
        QueuePort = ReplyPort->CommunicationInfo->ConnectionPort;
    }

    KeAcquireGuardedMutex(&AlpcpLock);
    for (Entry = QueuePort->PendingQueue.Flink;
         Entry != &QueuePort->PendingQueue;
         Entry = Entry->Flink)
    {
        PKALPC_MESSAGE Candidate = CONTAINING_RECORD(Entry, KALPC_MESSAGE, Entry);
        if (Candidate->PortMessage.MessageId == MessageId)
        {
            Request = Candidate;
            break;
        }
    }
    if (Request == NULL)
    {
        KeReleaseGuardedMutex(&AlpcpLock);
        return STATUS_INVALID_PARAMETER;
    }
    RemoveEntryList(&Request->Entry);
    QueuePort->PendingQueueLength--;
    Request->u1.QueueType = 0;
    WaitingThread = Request->WaitingThread;
    KeReleaseGuardedMutex(&AlpcpLock);

    Reply = AlpcpAllocateMessage(DataLength);
    if (Reply == NULL)
    {
        /* Put the request back so the sender can still time out cleanly. */
        KeAcquireGuardedMutex(&AlpcpLock);
        InsertTailList(&QueuePort->PendingQueue, &Request->Entry);
        QueuePort->PendingQueueLength++;
        Request->u1.QueueType = 3;
        KeReleaseGuardedMutex(&AlpcpLock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Reply->PortMessage.u2.s2.Type = ALPC_REQUEST_TYPE;
    Reply->PortMessage.MessageId = MessageId;
    Reply->PortMessage.ClientId = Request->PortMessage.ClientId;
    if (DataLength != 0)
        RtlCopyMemory(AlpcpGetMessageData(Reply), Data, DataLength);

    if (WaitingThread != NULL)
    {
        /* Hand the reply to the blocked sender, which owns and frees both the
         * reply and its original request. */
        WaitingThread->AlpcMessage = Reply;
        KeReleaseSemaphore(&WaitingThread->AlpcWaitSemaphore, 0, 1, FALSE);
    }
    else
    {
        AlpcpFreeMessage(Reply);
    }

    return STATUS_SUCCESS;
}

/* PUBLIC ****************************************************************/

/**
 * @brief Sends a message and/or waits to receive one on a port.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcSendWaitReceivePort(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_opt_ PPORT_MESSAGE SendMessage,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
    _Out_opt_ PPORT_MESSAGE ReceiveMessage,
    _Inout_opt_ PSIZE_T BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PORT_MESSAGE SendHeader;
    PVOID SendData = NULL;
    ULONG SendDataLength = 0;
    SIZE_T CapturedBufferLength = 0;
    LARGE_INTEGER CapturedTimeout;
    PLARGE_INTEGER TimeoutPtr = NULL;
    NTSTATUS Status;

    if (SendMessage == NULL && ReceiveMessage == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Capture the in-parameters. */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            if (Timeout != NULL)
            {
                ProbeForRead(Timeout, sizeof(LARGE_INTEGER), sizeof(ULONG));
                CapturedTimeout = *Timeout;
                TimeoutPtr = &CapturedTimeout;
            }
            if (BufferLength != NULL)
            {
                ProbeForWrite(BufferLength, sizeof(SIZE_T), sizeof(ULONG));
                CapturedBufferLength = *BufferLength;
            }
            if (SendMessage != NULL)
            {
                ProbeForRead(SendMessage, sizeof(PORT_MESSAGE), sizeof(ULONG));
                SendHeader = *SendMessage;
                SendDataLength = SendHeader.u1.s1.DataLength;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        if (Timeout != NULL)
        {
            CapturedTimeout = *Timeout;
            TimeoutPtr = &CapturedTimeout;
        }
        if (BufferLength != NULL)
            CapturedBufferLength = *BufferLength;
        if (SendMessage != NULL)
        {
            SendHeader = *SendMessage;
            SendDataLength = SendHeader.u1.s1.DataLength;
        }
    }

    /* Capture the send payload into a kernel buffer. */
    if (SendMessage != NULL && SendDataLength != 0)
    {
        SendData = ExAllocatePoolWithTag(NonPagedPool, SendDataLength, TAG_ALPC_MESSAGE);
        if (SendData == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForRead((PUCHAR)SendMessage + sizeof(PORT_MESSAGE), SendDataLength, 1);
            RtlCopyMemory(SendData, (PUCHAR)SendMessage + sizeof(PORT_MESSAGE), SendDataLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ExFreePoolWithTag(SendData, TAG_ALPC_MESSAGE);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    Status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       AlpcPortObjectType,
                                       PreviousMode,
                                       (PVOID *)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        if (SendData != NULL)
            ExFreePoolWithTag(SendData, TAG_ALPC_MESSAGE);
        return Status;
    }

    if (SendMessage != NULL)
    {
        if (Flags & ALPC_MSGFLG_REPLY_MESSAGE)
        {
            Status = AlpcpSendReply(Port, &SendHeader, SendData, SendDataLength);
        }
        else
        {
            Status = AlpcpSendRequest(Port, &SendHeader, SendData, SendDataLength, Flags,
                                      SendMessageAttributes, ReceiveMessage, BufferLength,
                                      CapturedBufferLength, PreviousMode, TimeoutPtr);
        }
    }
    else
    {
        Status = AlpcpReceiveMessage(Port, ReceiveMessage, &CapturedBufferLength,
                                     ReceiveMessageAttributes, NULL, PreviousMode, TimeoutPtr);
        if ((NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL) && BufferLength != NULL)
        {
            _SEH2_TRY
            {
                *BufferLength = CapturedBufferLength;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                NOTHING;
            }
            _SEH2_END;
        }
    }

    ObDereferenceObject(Port);
    if (SendData != NULL)
        ExFreePoolWithTag(SendData, TAG_ALPC_MESSAGE);
    return Status;
}

/**
 * @brief Cancels a pending received message by id, waking its blocked sender.
 *
 * Mirrors AlpcpLookupMessage + AlpcpCancelMessage: the message is removed from
 * the pending queue; a synchronous sender is woken with STATUS_MESSAGE_LOST (and
 * reclaims its own request), while an unwaited datagram is freed here. The call
 * itself returns STATUS_PENDING once the cancellation is initiated.
 */
static
NTSTATUS
AlpcpCancelMessage(
    _In_ PALPC_PORT Port,
    _In_ ULONG MessageId)
{
    PKALPC_MESSAGE Message;
    PKALPC_MESSAGE Found = NULL;
    PETHREAD WaitingThread = NULL;
    PLIST_ENTRY Entry;

    KeAcquireGuardedMutex(&AlpcpLock);
    for (Entry = Port->PendingQueue.Flink;
         Entry != &Port->PendingQueue;
         Entry = Entry->Flink)
    {
        Message = CONTAINING_RECORD(Entry, KALPC_MESSAGE, Entry);
        if (Message->PortMessage.MessageId == MessageId)
        {
            Found = Message;
            WaitingThread = Message->WaitingThread;
            RemoveEntryList(&Found->Entry);
            Port->PendingQueueLength--;
            Found->u1.QueueType = 0;
            Found->u1.Canceled = TRUE;
            break;
        }
    }
    KeReleaseGuardedMutex(&AlpcpLock);

    if (Found == NULL)
        return STATUS_NOT_FOUND;

    if (WaitingThread != NULL)
    {
        /* The sender owns and reclaims the request; just wake it as cancelled. */
        WaitingThread->AlpcMessage = ALPC_WAKE_CANCELED;
        KeReleaseSemaphore(&WaitingThread->AlpcWaitSemaphore, 0, 1, FALSE);
    }
    else
    {
        AlpcpFreeMessage(Found);
    }

    /*
     * The cancellation has been initiated (the sender is woken asynchronously);
     * the call reports STATUS_PENDING, matching the Win11 oracle.
     */
    return STATUS_PENDING;
}

/**
 * @brief Cancels a message that is waiting for a reply.
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcCancelMessage(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_ PALPC_CONTEXT_ATTR MessageContext)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    ULONG MessageId;
    NTSTATUS Status;

    if ((Flags & ~0xFu) != 0)
        return STATUS_INVALID_PARAMETER;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(MessageContext, sizeof(ALPC_CONTEXT_ATTR), sizeof(ULONG));
        MessageId = MessageContext->MessageId;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (MessageId == 0)
        return STATUS_INVALID_PARAMETER;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = AlpcpCancelMessage(Port, MessageId);

    ObDereferenceObject(Port);
    return Status;
}
