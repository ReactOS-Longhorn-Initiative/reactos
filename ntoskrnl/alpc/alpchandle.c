/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC per-port handle table and communication-info allocation
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

#define ALPC_HANDLE_TABLE_INITIAL 16

/* HANDLE TABLE ************************************************************/

/**
 * @brief Initializes an ALPC handle table with an initial slot array.
 */
NTSTATUS
NTAPI
AlpcpInitializeHandleTable(
    _Out_ PALPC_HANDLE_TABLE Table)
{
    Table->Flags = 0;
    Table->Lock.Value = 0;
    Table->TotalHandles = ALPC_HANDLE_TABLE_INITIAL;
    Table->Handles = ExAllocatePoolWithTag(PagedPool,
                                           ALPC_HANDLE_TABLE_INITIAL * sizeof(ALPC_HANDLE_ENTRY),
                                           TAG_ALPC_HANDLE);
    if (Table->Handles == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Table->Handles, ALPC_HANDLE_TABLE_INITIAL * sizeof(ALPC_HANDLE_ENTRY));
    return STATUS_SUCCESS;
}

/**
 * @brief Stores an object in the table and returns its (one-based) handle.
 *
 * @return A non-NULL ALPC_HANDLE, or NULL if the table is full.
 */
ALPC_HANDLE
NTAPI
AlpcpInsertHandle(
    _In_ PALPC_HANDLE_TABLE Table,
    _In_ PVOID Object)
{
    ALPC_HANDLE Handle = NULL;
    ULONG i;

    KeAcquireGuardedMutex(&AlpcpLock);
    for (i = 0; i < Table->TotalHandles; i++)
    {
        if (Table->Handles[i].Object == NULL)
        {
            Table->Handles[i].Object = Object;
            Handle = (ALPC_HANDLE)(ULONG_PTR)(i + 1);
            break;
        }
    }
    KeReleaseGuardedMutex(&AlpcpLock);

    return Handle;
}

/**
 * @brief Looks up the object backing a handle without removing it.
 */
PVOID
NTAPI
AlpcpReferenceHandle(
    _In_ PALPC_HANDLE_TABLE Table,
    _In_ ALPC_HANDLE Handle)
{
    ULONG_PTR Index = (ULONG_PTR)Handle;
    PVOID Object = NULL;

    if (Index == 0 || Index > Table->TotalHandles)
        return NULL;

    KeAcquireGuardedMutex(&AlpcpLock);
    Object = Table->Handles[Index - 1].Object;
    KeReleaseGuardedMutex(&AlpcpLock);

    return Object;
}

/**
 * @brief Removes a handle from the table, returning its object (or NULL).
 */
PVOID
NTAPI
AlpcpRemoveHandle(
    _In_ PALPC_HANDLE_TABLE Table,
    _In_ ALPC_HANDLE Handle)
{
    ULONG_PTR Index = (ULONG_PTR)Handle;
    PVOID Object = NULL;

    if (Index == 0 || Index > Table->TotalHandles)
        return NULL;

    KeAcquireGuardedMutex(&AlpcpLock);
    Object = Table->Handles[Index - 1].Object;
    Table->Handles[Index - 1].Object = NULL;
    KeReleaseGuardedMutex(&AlpcpLock);

    return Object;
}

/* COMMUNICATION INFO *****************************************************/

/**
 * @brief Allocates a port's communication info, including its handle table.
 *
 * @param[in,out] Port           The port to attach the info to.
 * @param[in]     ConnectionPort The owning connection port (== Port for a
 *                               connection port). A routing reference is taken
 *                               when it differs from @p Port.
 */
NTSTATUS
NTAPI
AlpcpAllocateCommunicationInfo(
    _Inout_ PALPC_PORT Port,
    _In_ PALPC_PORT ConnectionPort)
{
    PALPC_COMMUNICATION_INFO Info;
    NTSTATUS Status;

    Info = ExAllocatePoolWithTag(NonPagedPool, sizeof(ALPC_COMMUNICATION_INFO), TAG_ALPC_COMM);
    if (Info == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Info, sizeof(ALPC_COMMUNICATION_INFO));
    InitializeListHead(&Info->CommunicationList);

    Status = AlpcpInitializeHandleTable(&Info->HandleTable);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Info, TAG_ALPC_COMM);
        return Status;
    }

    if (ConnectionPort == Port)
    {
        Info->ConnectionPort = Port;
    }
    else
    {
        ObReferenceObject(ConnectionPort);
        Info->ConnectionPort = ConnectionPort;
        Info->ClientCommunicationPort = Port;
    }

    Port->CommunicationInfo = Info;
    return STATUS_SUCCESS;
}
