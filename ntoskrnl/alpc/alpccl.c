/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC completion-list message delivery (lock-free ring)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * A completion list lets a server collect messages from a shared ring in a
 * registered section view instead of blocking in NtAlpcSendWaitReceivePort. The
 * kernel locks the buffer pages with an MDL and obtains a system address (as the
 * Win10 kernel does), writes each received message into a fixed-size slot and
 * publishes it by advancing the ring head; the user side polls the slots through
 * AlpcGetMessageFromCompletionList (ntdll). Producers are serialized under
 * AlpcpLock; the single consumer reads without the lock.
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/**
 * @brief Registers a mapped buffer as a port's completion list.
 *
 * The buffer is a section view previously mapped on @p Port
 * (NtAlpcCreateSectionView). Its pages are locked with an MDL and a system
 * address is obtained so the kernel can write to the same physical pages the
 * caller polls through its own view.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
AlpcpRegisterCompletionList(
    _Inout_ PALPC_PORT Port,
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PALPC_COMPLETION_LIST CompletionList;
    PALPC_COMPLETION_LIST_HEADER Header;
    PMDL Mdl;
    PVOID SystemVa = NULL;
    ULONG SlotCount;
    NTSTATUS Status = STATUS_SUCCESS;

    if (Size <= ALPC_COMPLETION_LIST_LIST_OFFSET + ALPC_COMPLETION_LIST_SLOT_SIZE)
        return STATUS_INVALID_PARAMETER;

    CompletionList = ExAllocatePoolWithTag(NonPagedPool, sizeof(ALPC_COMPLETION_LIST),
                                           TAG_ALPC_PORT);
    if (CompletionList == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Mdl = IoAllocateMdl(Buffer, Size, FALSE, FALSE, NULL);
    if (Mdl == NULL)
    {
        ExFreePoolWithTag(CompletionList, TAG_ALPC_PORT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Lock the caller's buffer pages and obtain a system-space address. */
    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, PreviousMode, IoModifyAccess);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        IoFreeMdl(Mdl);
        ExFreePoolWithTag(CompletionList, TAG_ALPC_PORT);
        return Status;
    }

    SystemVa = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);
    if (SystemVa == NULL)
    {
        MmUnlockPages(Mdl);
        IoFreeMdl(Mdl);
        ExFreePoolWithTag(CompletionList, TAG_ALPC_PORT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize the ring header at the start of the buffer. */
    Header = (PALPC_COMPLETION_LIST_HEADER)SystemVa;
    SlotCount = (Size - ALPC_COMPLETION_LIST_LIST_OFFSET) / ALPC_COMPLETION_LIST_SLOT_SIZE;

    RtlZeroMemory(Header, sizeof(*Header));
    Header->StartMagic = ALPC_COMPLETION_LIST_START_MAGIC;
    Header->TotalSize = Size;
    Header->ListOffset = ALPC_COMPLETION_LIST_LIST_OFFSET;
    Header->SlotSize = ALPC_COMPLETION_LIST_SLOT_SIZE;
    Header->SlotCount = SlotCount;
    Header->ListSize = SlotCount * ALPC_COMPLETION_LIST_SLOT_SIZE;
    Header->Head = 0;
    Header->Tail = 0;
    Header->EndMagic = ALPC_COMPLETION_LIST_END_MAGIC;

    CompletionList->Mdl = Mdl;
    CompletionList->SystemVa = SystemVa;
    CompletionList->UserVa = Buffer;
    CompletionList->Size = Size;
    CompletionList->Header = Header;

    KeAcquireGuardedMutex(&AlpcpLock);
    if (Port->CompletionList != NULL)
    {
        KeReleaseGuardedMutex(&AlpcpLock);
        MmUnlockPages(Mdl);
        IoFreeMdl(Mdl);
        ExFreePoolWithTag(CompletionList, TAG_ALPC_PORT);
        return STATUS_PORT_ALREADY_SET;
    }
    Port->CompletionList = (struct _ALPC_COMPLETION_LIST *)CompletionList;
    Port->u1.HasCompletionList = TRUE;
    KeReleaseGuardedMutex(&AlpcpLock);

    return STATUS_SUCCESS;
}

/**
 * @brief Tears down a port's completion list.
 *
 * @return STATUS_SUCCESS, even if no completion list is registered.
 */
NTSTATUS
NTAPI
AlpcpUnregisterCompletionList(
    _Inout_ PALPC_PORT Port)
{
    PALPC_COMPLETION_LIST CompletionList;

    KeAcquireGuardedMutex(&AlpcpLock);
    CompletionList = (PALPC_COMPLETION_LIST)Port->CompletionList;
    Port->CompletionList = NULL;
    Port->u1.HasCompletionList = FALSE;
    KeReleaseGuardedMutex(&AlpcpLock);

    if (CompletionList != NULL)
    {
        if (CompletionList->Mdl != NULL)
        {
            MmUnlockPages(CompletionList->Mdl);
            IoFreeMdl(CompletionList->Mdl);
        }
        ExFreePoolWithTag(CompletionList, TAG_ALPC_PORT);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Writes a received message into a port's completion-list ring.
 *
 * Called from the send path for a datagram targeting a completion-list port. The
 * message is serialized into the next free slot and published by advancing the
 * ring head. Producers serialize under AlpcpLock.
 *
 * @return TRUE if the message was delivered to the ring; FALSE if there is no
 * completion list, the message is too large for a slot, or the ring is full
 * (the caller then falls back to the normal queue).
 */
BOOLEAN
NTAPI
AlpcpInsertCompletionList(
    _In_ PALPC_PORT Port,
    _In_ CLIENT_ID ClientId,
    _In_reads_bytes_opt_(DataLength) PVOID Data,
    _In_ ULONG DataLength)
{
    PALPC_COMPLETION_LIST CompletionList;
    PALPC_COMPLETION_LIST_HEADER Header;
    PPORT_MESSAGE Slot;
    ULONG TotalLength = sizeof(PORT_MESSAGE) + DataLength;
    ULONG Index;
    BOOLEAN Delivered = FALSE;

    if (TotalLength > ALPC_COMPLETION_LIST_SLOT_SIZE)
        return FALSE;

    KeAcquireGuardedMutex(&AlpcpLock);

    CompletionList = (PALPC_COMPLETION_LIST)Port->CompletionList;
    if (CompletionList != NULL)
    {
        Header = CompletionList->Header;
        if ((Header->Head - Header->Tail) < Header->SlotCount)
        {
            Index = Header->Head % Header->SlotCount;
            Slot = (PPORT_MESSAGE)((PUCHAR)Header + Header->ListOffset +
                                   (SIZE_T)Index * Header->SlotSize);

            RtlZeroMemory(Slot, sizeof(PORT_MESSAGE));
            Slot->u1.s1.DataLength = (CSHORT)DataLength;
            Slot->u1.s1.TotalLength = (CSHORT)TotalLength;
            Slot->u2.s2.Type = LPC_REQUEST;
            Slot->ClientId = ClientId;
            Slot->MessageId = (ULONG)InterlockedIncrement(&AlpcpNextMessageId) & 0x7FFFFFFF;
            if (DataLength != 0)
                RtlCopyMemory((PUCHAR)Slot + sizeof(PORT_MESSAGE), Data, DataLength);

            /* Publish: the slot contents must be visible before the head moves. */
            KeMemoryBarrier();
            Header->Head++;
            Delivered = TRUE;
        }
    }

    KeReleaseGuardedMutex(&AlpcpLock);
    return Delivered;
}
