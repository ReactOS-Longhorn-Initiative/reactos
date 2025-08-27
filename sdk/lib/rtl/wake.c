/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation for WaitOnAddress / WakeByAddress APIs
 * COPYRIGHT:   Copyright 2025 <justin.miller@reactos.org>
 */

#include <rtl.h>
#define NDEBUG
#include <intrin.h>
#include <debug.h>

typedef struct _SYNC_WAIT_BLOCK {
    struct _SYNC_WAIT_BLOCK *Next;
    struct _SYNC_WAIT_BLOCK *Prev;
    volatile LONG State;
    BOOLEAN IsShared;
    RTL_SRWLOCK SRWLock;
} SYNC_WAIT_BLOCK, *PSYNC_WAIT_BLOCK;

/**
 * @struct WAKEADDRESS_BLOCK
 * @brief Represents a single thread waiting on a specific address.
 */
typedef struct _WAKEADDRESS_BLOCK {
    PVOID Address;                              // The address being monitored.
    HANDLE EventHandle;                         // Event handle for this specific wait.
    struct _WAKEADDRESS_BLOCK *Next;            // Next block in the hash bucket's list.
    struct _WAKEADDRESS_BLOCK *Previous;        // Previous block in the hash bucket's list.
} WAKEADDRESS_BLOCK, *PWAKEADDRESS_BLOCK;

/**
 * @struct WAKEADDRESS_HASH_BUCKET
 * @brief A bucket in the hash table containing a linked list of waiters.
 */
typedef struct _WAKEADDRESS_HASH_BUCKET {
    RTL_SRWLOCK        Lock;                    // Lock protecting this bucket.
    PWAKEADDRESS_BLOCK WaitBlocks;              // Head of the waiter list.
} WAKEADDRESS_HASH_BUCKET, *PWAKEADDRESS_HASH_BUCKET;

#define WAKEADDRESS_HASH_ENTRIES 128
static WAKEADDRESS_HASH_BUCKET GlobalWakeAddressHashTable[WAKEADDRESS_HASH_ENTRIES] = {0};

static PWAKEADDRESS_HASH_BUCKET GetWakeAddressHashBucket(PVOID Address) {
    return &GlobalWakeAddressHashTable[(((ULONG_PTR)Address) >> 4) & (WAKEADDRESS_HASH_ENTRIES - 1)];
}

static BOOLEAN IsMemoryEqual(volatile VOID *Address1, PVOID Address2, SIZE_T Size) {
    switch (Size) {
        case 1: return (*(volatile UCHAR*)Address1 == *(UCHAR*)Address2);
        case 2: return (*(volatile USHORT*)Address1 == *(USHORT*)Address2);
        case 4: return (*(volatile ULONG*)Address1 == *(ULONG*)Address2);
        case 8: return (*(volatile ULONGLONG*)Address1 == *(ULONGLONG*)Address2);
        default: return FALSE;
    }
}

/**
 * @brief Atomically removes a wait block from its hash bucket's list.
 * @note This must be called while the bucket lock is held.
 */
static void RemoveWakeAddressWaitBlock(PWAKEADDRESS_HASH_BUCKET HashBucket, PWAKEADDRESS_BLOCK WaitBlock) {
    if (WaitBlock->Previous == NULL) {
        return;
    }

    if (WaitBlock->Next == WaitBlock) {
        HashBucket->WaitBlocks = NULL;
    } else {
        WaitBlock->Previous->Next = WaitBlock->Next;
        WaitBlock->Next->Previous = WaitBlock->Previous;
        if (HashBucket->WaitBlocks == WaitBlock) {
            HashBucket->WaitBlocks = WaitBlock->Next;
        }
    }
    WaitBlock->Previous = NULL;
    WaitBlock->Next = NULL;
}

/**
 * @brief Waits for the value at a specified address to change.
 *
 * This function atomically checks if the value at `Address` matches `CompareAddress`.
 * If they are the same, the thread sleeps until another thread calls
 * RtlWaitOnAddress or RtlWakeByAddressAll on the same address, or until
 * the timeout elapses.
 *
 * @param Address The address to wait on.
 * @param CompareAddress A pointer to the value to compare against the value at `Address`.
 * @param AddressSize The size of the value in bytes (1, 2, 4, or 8).
 * @param Timeout An optional pointer to a timeout value. NULL for an infinite wait.
 * @return STATUS_SUCCESS if the wait succeeded or if the values did not match initially.
 * STATUS_TIMEOUT if the timeout elapsed.
 * STATUS_INVALID_PARAMETER for invalid arguments.
 */
NTSTATUS
NTAPI
RtlWaitOnAddress(PVOID Address, PVOID CompareAddress, SIZE_T AddressSize, PLARGE_INTEGER Timeout) {
    if (!Address || !CompareAddress || (AddressSize != 1 && AddressSize != 2 && AddressSize != 4 && AddressSize != 8)) {
        return STATUS_INVALID_PARAMETER;
    }

    PWAKEADDRESS_HASH_BUCKET bucket = GetWakeAddressHashBucket(Address);
    RtlAcquireSRWLockExclusive(&bucket->Lock);

    if (!IsMemoryEqual(Address, CompareAddress, AddressSize)) {
        RtlReleaseSRWLockExclusive(&bucket->Lock);
        return STATUS_SUCCESS;
    }

    WAKEADDRESS_BLOCK waitBlock = { .Address = Address };
    NTSTATUS status = NtCreateEvent(&waitBlock.EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(status)) {
        RtlReleaseSRWLockExclusive(&bucket->Lock);
        return status;
    }

    if (bucket->WaitBlocks == NULL) {
        waitBlock.Next = &waitBlock;
        waitBlock.Previous = &waitBlock;
        bucket->WaitBlocks = &waitBlock;
    } else {
        waitBlock.Previous = bucket->WaitBlocks->Previous;
        waitBlock.Next = bucket->WaitBlocks;
        bucket->WaitBlocks->Previous->Next = &waitBlock;
        bucket->WaitBlocks->Previous = &waitBlock;
    }

    RtlReleaseSRWLockExclusive(&bucket->Lock);

    status = NtWaitForSingleObject(waitBlock.EventHandle, FALSE, Timeout);

    if (status != STATUS_SUCCESS) {
        RtlAcquireSRWLockExclusive(&bucket->Lock);
        RemoveWakeAddressWaitBlock(bucket, &waitBlock);
        RtlReleaseSRWLockExclusive(&bucket->Lock);
    }

    NtClose(waitBlock.EventHandle);
    return status;
}

static VOID WakeByAddressInternal(PVOID Address, BOOLEAN WakeAll) {
    PWAKEADDRESS_HASH_BUCKET bucket = GetWakeAddressHashBucket(Address);
    RtlAcquireSRWLockExclusive(&bucket->Lock);

    PWAKEADDRESS_BLOCK current = bucket->WaitBlocks;
    if (!current) {
        RtlReleaseSRWLockExclusive(&bucket->Lock);
        return;
    }
    
    PWAKEADDRESS_BLOCK startNode = current;
    do {
        PWAKEADDRESS_BLOCK next = current->Next;

        if (current->Address == Address) {
            HANDLE eventToSignal = current->EventHandle;
            RemoveWakeAddressWaitBlock(bucket, current);
            NtSetEvent(eventToSignal, NULL);

            if (!WakeAll) {
                break;
            }
        }
        
        if (bucket->WaitBlocks == NULL) {
            break;
        }

        current = next;
    } while (current != startNode);

    RtlReleaseSRWLockExclusive(&bucket->Lock);
}

/**
 * @brief Wakes one thread that is waiting on a specified address.
 * @param Address The address to signal.
 */
VOID NTAPI RtlWakeAddressSingle(PVOID Address) {
    WakeByAddressInternal(Address, FALSE);
}

/**
 * @brief Wakes all threads that are waiting on a specified address.
 * @param Address The address to signal.
 */
VOID NTAPI RtlWakeAddressAll(PVOID Address) {
    WakeByAddressInternal(Address, TRUE);
}
