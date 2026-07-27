/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC subsystem initialization and port object lifetime
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

POBJECT_TYPE AlpcPortObjectType;
PKEVENT AlpcpDummyEvent;
LIST_ENTRY AlpcpPortList;
KGUARDED_MUTEX AlpcpPortListLock;

/* Coarse lock guarding port queues and connection state. Held only around
 * list manipulation, never across a wait. */
KGUARDED_MUTEX AlpcpLock;
LONG AlpcpNextMessageId;

static GENERIC_MAPPING AlpcpPortMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    PORT_ALL_ACCESS
};

/* FUNCTIONS ******************************************************************/

/**
 * @brief Object-manager close callback for an ALPC port.
 *
 * Invoked as handles are closed. Teardown of the communication state happens in
 * the delete callback once the last reference is gone, so for now this only
 * exists to satisfy the object type.
 */
VOID
NTAPI
AlpcpClosePort(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID Object,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG ProcessHandleCount,
    _In_ ULONG SystemHandleCount)
{
    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(Object);
    UNREFERENCED_PARAMETER(GrantedAccess);
    UNREFERENCED_PARAMETER(ProcessHandleCount);
    UNREFERENCED_PARAMETER(SystemHandleCount);
}

/**
 * @brief Object-manager delete callback for an ALPC port.
 *
 * Frees the communication info, the waitable semaphore and the owning-process
 * reference, and unlinks the port from the global port list.
 *
 * @param[in] Object
 * The ALPC_PORT being destroyed.
 */
VOID
NTAPI
AlpcpDeletePort(
    _In_ PVOID Object)
{
    PALPC_PORT Port = Object;

    /* Unlink from the global port list if we ever made it that far. */
    if (Port->PortListEntry.Flink != NULL)
    {
        KeAcquireGuardedMutex(&AlpcpPortListLock);
        RemoveEntryList(&Port->PortListEntry);
        KeReleaseGuardedMutex(&AlpcpPortListLock);
        Port->PortListEntry.Flink = NULL;
    }

    /* Drain any messages still sitting on the queues (received datagrams that
     * were never released, requests that never got a reply, etc.). */
    if (Port->u1.Initialized)
    {
        LIST_ENTRY Drain;

        InitializeListHead(&Drain);
        KeAcquireGuardedMutex(&AlpcpLock);
        while (!IsListEmpty(&Port->MainQueue))
            InsertTailList(&Drain, RemoveHeadList(&Port->MainQueue));
        while (!IsListEmpty(&Port->PendingQueue))
            InsertTailList(&Drain, RemoveHeadList(&Port->PendingQueue));
        Port->MainQueueLength = 0;
        Port->PendingQueueLength = 0;
        KeReleaseGuardedMutex(&AlpcpLock);

        while (!IsListEmpty(&Drain))
        {
            PKALPC_MESSAGE Message = CONTAINING_RECORD(RemoveHeadList(&Drain), KALPC_MESSAGE, Entry);
            AlpcpFreeMessage(Message);
        }
    }

    /* Unlink from the communication peer so it does not dangle to a freed port. */
    if (Port->CommunicationInfo != NULL)
    {
        PALPC_PORT Peer;

        KeAcquireGuardedMutex(&AlpcpLock);
        Peer = Port->CommunicationInfo->ServerCommunicationPort;
        if (Peer == NULL)
            Peer = Port->CommunicationInfo->ClientCommunicationPort;
        if (Peer != NULL && Peer->CommunicationInfo != NULL)
        {
            if (Peer->CommunicationInfo->ServerCommunicationPort == Port)
                Peer->CommunicationInfo->ServerCommunicationPort = NULL;
            if (Peer->CommunicationInfo->ClientCommunicationPort == Port)
                Peer->CommunicationInfo->ClientCommunicationPort = NULL;
        }
        KeReleaseGuardedMutex(&AlpcpLock);
    }

    /* Release the communication info and its handle table. A communication port
     * holds a routing reference on its connection port (the connection port's
     * own info refers back to itself, so skip that case). */
    if (Port->CommunicationInfo != NULL)
    {
        if (Port->CommunicationInfo->ConnectionPort != NULL &&
            Port->CommunicationInfo->ConnectionPort != Port)
        {
            ObDereferenceObject(Port->CommunicationInfo->ConnectionPort);
        }

        if (Port->CommunicationInfo->HandleTable.Handles != NULL)
            ExFreePoolWithTag(Port->CommunicationInfo->HandleTable.Handles, TAG_ALPC_HANDLE);

        ExFreePoolWithTag(Port->CommunicationInfo, TAG_ALPC_COMM);
        Port->CommunicationInfo = NULL;
    }

    /* Release a legacy LPC connection view section. The mapped views are torn
     * down with their address spaces at process exit; just drop our reference. */
    if (Port->LpcViewSection != NULL)
    {
        ObDereferenceObject(Port->LpcViewSection);
        Port->LpcViewSection = NULL;
    }

    /* Tear down a registered completion list (unmaps the system view). */
    if (Port->CompletionList != NULL)
        AlpcpUnregisterCompletionList(Port);

    /* Release an associated I/O completion port, if any. */
    if (Port->CompletionPort != NULL)
    {
        ObDereferenceObject(Port->CompletionPort);
        Port->CompletionPort = NULL;
    }

    /* A waitable port owns a real semaphore; non-waitable ones borrow the dummy. */
    if (Port->u1.Waitable &&
        Port->Semaphore != NULL &&
        Port->Semaphore != (PKSEMAPHORE)AlpcpDummyEvent)
    {
        ExFreePoolWithTag(Port->Semaphore, TAG_ALPC_PORT);
    }

    if (Port->OwnerProcess != NULL)
    {
        ObDereferenceObject(Port->OwnerProcess);
        Port->OwnerProcess = NULL;
    }
}

/**
 * @brief Brings up the ALPC subsystem during Phase 1 of kernel init.
 *
 * Registers the "ALPC Port" object type, allocates the shared dummy event used
 * by non-waitable ports and initializes the global port list.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
AlpcpInitSystem(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    NTSTATUS Status;

    KeInitializeGuardedMutex(&AlpcpPortListLock);
    KeInitializeGuardedMutex(&AlpcpLock);
    InitializeListHead(&AlpcpPortList);

    /* Non-waitable ports point their wait object at this permanently-signalled
     * event so the wait paths never have to special-case them. */
    AlpcpDummyEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_ALPC_PORT);
    if (AlpcpDummyEvent == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    KeInitializeEvent(AlpcpDummyEvent, NotificationEvent, TRUE);

    /* Register the ALPC port object type. */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"ALPC Port");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(ALPC_PORT);
    ObjectTypeInitializer.GenericMapping = AlpcpPortMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = PORT_ALL_ACCESS;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.CloseProcedure = AlpcpClosePort;
    ObjectTypeInitializer.DeleteProcedure = AlpcpDeletePort;
    Status = ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &AlpcPortObjectType);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(AlpcpDummyEvent, TAG_ALPC_PORT);
        AlpcpDummyEvent = NULL;
        return Status;
    }

    return STATUS_SUCCESS;
}
