/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Slim Reader/Writer (SRW) Routines
 * PROGRAMMER:        Thomas Weidenmueller <w3seek@reactos.com>
 *
 * NOTES:             The algorithms used in this implementation
 *                    may be different from Vista's implementation.
 *                    Since applications should treat the RTL_SRWLOCK
 *                    structure as opaque data, it should not matter.
 *                    The algorithms are probably not as optimized.
 */

/* INCLUDES *****************************************************************/

#include <rtl_vista.h>

#define NDEBUG
#include <debug.h>

/* SRW implementation using keyed events for wait/wake coordination. */

/* Shared keyed-event handle owned by condvar module. */
extern HANDLE GlobalKeyedEventHandle;

/* Pointer-sized interlocked helpers for user mode. */
#ifndef InterlockedAndPointer
 #if defined(_WIN64)
  #define InterlockedAndPointer(ptr, val) (PVOID)InterlockedAnd64((volatile LONG64*)(ptr), (LONG64)(val))
 #else
  #define InterlockedAndPointer(ptr, val) (PVOID)InterlockedAnd((volatile LONG*)(ptr), (LONG)(val))
 #endif
#endif

#ifndef InterlockedExchangeAddPointer
 #if defined(_WIN64)
  #define InterlockedExchangeAddPointer(ptr, val) (PVOID)InterlockedExchangeAdd64((volatile LONG64*)(ptr), (LONG64)(val))
 #else
  #define InterlockedExchangeAddPointer(ptr, val) (PVOID)InterlockedExchangeAdd((volatile LONG*)(ptr), (LONG)(val))
 #endif
#endif

/* Bit layout for internal SRW state and wait list flags. */
#if defined(_WIN64)
 #define SRWM_ITEM  0xFFFFFFFFFFFFFFF0ULL
#else
 #define SRWM_ITEM  0xFFFFFFF0U
#endif
#define SRWM_FLAG  (~SRWM_ITEM)

#define SRWF_Free   0x0
#define SRWF_Hold   0x1
#define SRWF_Wait   0x2
#define SRWF_Link   0x4
#define SRWF_Many   0x8

#define SRW_COUNT_BIT   4
#define SRW_HOLD_BIT    0
#define NODE_SPIN_BIT   1

#define NODEF_EXCL      0x01
#define NODEF_SPIN      0x02

typedef struct _SRW_WAIT_NODE
{
    struct _SRW_WAIT_NODE* prev;
    struct _SRW_WAIT_NODE* head;
    struct _SRW_WAIT_NODE* next;
    ULONG shareSnapshot;    /* shared count snapshot */
    ULONG flags;            /* NODEF_EXCL | NODEF_SPIN */
    PRTL_SRWLOCK lock;      /* reserved */
} SRW_WAIT_NODE;

/* SRW internal state is encoded in the pointer-sized value of SRWLock->Ptr. */
typedef ULONG_PTR SRW_STATE;

static ULONG SRWLockSpinCount = 1024;

/* Safely set bit 0 of a pointer-sized value; returns previous bit value (0/1). */
__forceinline LONG RtlpInterlockedBit0SetPointer(volatile PVOID* Target)
{
    PVOID OldValue = *Target;
    for (;;)
    {
        ULONG_PTR OldBits = (ULONG_PTR)OldValue;
        if (OldBits & SRWF_Hold) return 1; /* bit was already set */
        PVOID NewValue = (PVOID)(OldBits | SRWF_Hold);
        PVOID Prev = InterlockedCompareExchangePointer((PVOID*)Target, NewValue, OldValue);
        if (Prev == OldValue) return 0;     /* successfully set from 0 to 1 */
        OldValue = Prev;
    }
}

static VOID NTAPI RtlpBackoffExp(ULONG* pCount)
{
    ULONG n = *pCount;
    if (n == 0)
    {
        if (NtCurrentTeb()->ProcessEnvironmentBlock->NumberOfProcessors == 1) return;
        n = 64;
    }
    else if (n < 0x2000)
    {
        n <<= 1;
    }
    *pCount = n;
    while (n--) YieldProcessor();
}

__forceinline BOOLEAN RtlpWaitCouldDeadlock(void)
{
    return FALSE;
}

/* Helpers to test node attributes; avoid raw bit-twiddling at callsites. */
__forceinline BOOLEAN SrwNodeIsExclusive(const SRW_WAIT_NODE* n) { return (n->flags & NODEF_EXCL) != 0; }
__forceinline BOOLEAN SrwNodeIsSpinning(const SRW_WAIT_NODE* n)  { return (n->flags & NODEF_SPIN) != 0; }

static VOID NTAPI RtlpSrwWake(PRTL_SRWLOCK SRWLock, SRW_STATE OldStatus)
{
    SRW_STATE CurrStatus;
    SRW_WAIT_NODE* tail;
    SRW_WAIT_NODE* first;

    while (1)
    {
        if (OldStatus & SRWF_Hold)
        {
            do
            {
                CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)(OldStatus - SRWF_Link), (PVOID)OldStatus);
                if (CurrStatus == OldStatus) return;
                OldStatus = (SRW_STATE)CurrStatus;
            } while (OldStatus & SRWF_Hold);
        }

        tail = (SRW_WAIT_NODE*)(OldStatus & SRWM_ITEM);
        first = tail->head;
        if (first == NULL)
        {
            SRW_WAIT_NODE* curr = tail;
            do
            {
                curr->prev->next = curr;
                curr = curr->prev;
                first = curr->head;
            } while (first == NULL);
            if (tail != curr) tail->head = first;
        }

        if ((first->next != NULL) && SrwNodeIsExclusive(first))
        {
            tail->head = first->next;
            first->next = NULL;
            InterlockedAndPointer(&SRWLock->Ptr, (PVOID)(~SRWF_Link));
            break;
        }
        else
        {
            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, NULL, (PVOID)OldStatus);
            if ((SRW_STATE)CurrStatus == OldStatus) break;
            tail->head = first;
            OldStatus = (SRW_STATE)CurrStatus;
        }
    }

    do
    {
        SRW_WAIT_NODE* next = first->next;
        if (InterlockedBitTestAndReset((PLONG)&first->flags, NODE_SPIN_BIT) == 0)
        {
            NtReleaseKeyedEvent(GlobalKeyedEventHandle, first, FALSE, NULL);
        }
        first = next;
    } while (first != NULL);
}

static VOID NTAPI RtlpSrwCompressQueue(PRTL_SRWLOCK SRWLock, SRW_STATE OldStatus)
{
    SRW_STATE CurrStatus;
    if (OldStatus & SRWF_Hold)
    {
        do
        {
            SRW_WAIT_NODE* tail = (SRW_WAIT_NODE*)(OldStatus & SRWM_ITEM);
            if (tail != NULL)
            {
                SRW_WAIT_NODE* curr = tail;
                while (curr->head == NULL)
                {
                    curr->prev->next = curr;
                    curr = curr->prev;
                }
                tail->head = curr->head;
            }
            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)(OldStatus - SRWF_Link), (PVOID)OldStatus);
            if (CurrStatus == OldStatus) return;
            OldStatus = (SRW_STATE)CurrStatus;
        } while (OldStatus & SRWF_Hold);
    }
    RtlpSrwWake(SRWLock, OldStatus);
}

VOID NTAPI RtlInitializeSRWLock(PRTL_SRWLOCK SRWLock)
{
    SRWLock->Ptr = NULL;
}

VOID NTAPI RtlAcquireSRWLockExclusive(PRTL_SRWLOCK SRWLock)
{
    __ALIGNED(16) SRW_WAIT_NODE node;
    BOOLEAN IsOptimize;
    SRW_STATE NewStatus;
    ULONG backoff = 0;
    SRW_STATE CurrStatus;
    SRW_STATE OldStatus;
    int i;

    if (RtlpInterlockedBit0SetPointer(&SRWLock->Ptr) == 0)
        return;

    OldStatus = (SRW_STATE)SRWLock->Ptr;

    while (1)
    {
        if (OldStatus & SRWF_Hold)
        {
            if (RtlpWaitCouldDeadlock()) NtTerminateProcess((HANDLE)-1, STATUS_THREAD_IS_TERMINATING);

            node.flags = NODEF_EXCL | NODEF_SPIN;
            node.next = NULL;
            IsOptimize = FALSE;

            if (OldStatus & SRWF_Wait)
            {
                node.head = NULL;
                node.shareSnapshot = 0;
                node.prev = (SRW_WAIT_NODE*)(OldStatus & SRWM_ITEM);
                NewStatus = (SRW_STATE)&node | (OldStatus & SRWF_Many) | (SRWF_Link | SRWF_Wait | SRWF_Hold);
                if (!(OldStatus & SRWF_Link)) IsOptimize = TRUE;
            }
            else
            {
                node.head = &node;
                node.shareSnapshot = (ULONG)(OldStatus >> SRW_COUNT_BIT);
                if (node.shareSnapshot > 1)
                    NewStatus = (SRW_STATE)&node | (SRWF_Many | SRWF_Wait | SRWF_Hold);
                else
                    NewStatus = (SRW_STATE)&node | (SRWF_Wait | SRWF_Hold);
            }

            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)NewStatus, (PVOID)OldStatus);
            if (CurrStatus == OldStatus)
            {
                if (IsOptimize) RtlpSrwCompressQueue(SRWLock, NewStatus);
                for (i = SRWLockSpinCount; i > 0; --i)
                {
                    if (!SrwNodeIsSpinning(&node)) break;
                    YieldProcessor();
                }
                if (InterlockedBitTestAndReset((PLONG)&node.flags, NODE_SPIN_BIT))
                    NtWaitForKeyedEvent(GlobalKeyedEventHandle, &node, FALSE, NULL);
                OldStatus = CurrStatus;
            }
            else
            {
                RtlpBackoffExp(&backoff);
                OldStatus = (SRW_STATE)SRWLock->Ptr;
            }
        }
        else
        {
            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)(OldStatus + SRWF_Hold), (PVOID)OldStatus);
            if (CurrStatus == OldStatus) return;
            RtlpBackoffExp(&backoff);
            OldStatus = (SRW_STATE)SRWLock->Ptr;
        }
    }
}

VOID NTAPI RtlAcquireSRWLockShared(PRTL_SRWLOCK SRWLock)
{
    __ALIGNED(16) SRW_WAIT_NODE node;
    BOOLEAN IsOptimize;
    ULONG backoff = 0;
    int i;

    SRW_STATE NewStatus;
    SRW_STATE CurrStatus;
    SRW_STATE OldStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)((1 << SRW_COUNT_BIT) | SRWF_Hold), NULL);
    if (OldStatus == 0) return;

    while (1)
    {
    if ((OldStatus & SRWF_Hold) && ((OldStatus & SRWF_Wait) || ((OldStatus & SRWM_ITEM) == (SRW_STATE)NULL)))
        {
            if (RtlpWaitCouldDeadlock()) NtTerminateProcess((HANDLE)-1, STATUS_THREAD_IS_TERMINATING);

            node.flags = NODEF_SPIN;
            node.shareSnapshot = 0;
            IsOptimize = FALSE;
            node.next = NULL;

            if (OldStatus & SRWF_Wait)
            {
                node.prev = (SRW_WAIT_NODE*)(OldStatus & SRWM_ITEM);
                NewStatus = (SRW_STATE)&node | (OldStatus & (SRWF_Many | SRWF_Hold)) | (SRWF_Link | SRWF_Wait);
                node.head = NULL;
                if (!(OldStatus & SRWF_Link)) IsOptimize = TRUE;
            }
            else
            {
                node.head = &node;
                NewStatus = (SRW_STATE)&node | (SRWF_Wait | SRWF_Hold);
            }

            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)NewStatus, (PVOID)OldStatus);
            if (CurrStatus == OldStatus)
            {
                if (IsOptimize) RtlpSrwCompressQueue(SRWLock, NewStatus);
                for (i = SRWLockSpinCount; i > 0; --i)
                {
                    if (!SrwNodeIsSpinning(&node)) break;
                    YieldProcessor();
                }
                if (InterlockedBitTestAndReset((PLONG)&node.flags, NODE_SPIN_BIT))
                    NtWaitForKeyedEvent(GlobalKeyedEventHandle, &node, FALSE, NULL);
                OldStatus = CurrStatus;
            }
            else
            {
                RtlpBackoffExp(&backoff);
                OldStatus = (SRW_STATE)SRWLock->Ptr;
            }
        }
        else
        {
            if (OldStatus & SRWF_Wait)
                NewStatus = OldStatus + SRWF_Hold;
            else
                NewStatus = (OldStatus + (1 << SRW_COUNT_BIT)) | SRWF_Hold;
            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)NewStatus, (PVOID)OldStatus);
            if (CurrStatus == OldStatus) return;
            RtlpBackoffExp(&backoff);
            OldStatus = (SRW_STATE)SRWLock->Ptr;
        }
    }
}

VOID NTAPI RtlReleaseSRWLockExclusive(PRTL_SRWLOCK SRWLock)
{
    SRW_STATE CurrStatus;
    SRW_STATE OldStatus = (SRW_STATE)InterlockedExchangeAddPointer(&SRWLock->Ptr, (PVOID)(-(LONG_PTR)SRWF_Hold));
    if ((OldStatus & SRWF_Wait) && !(OldStatus & SRWF_Link))
    {
        OldStatus -= SRWF_Hold;
        CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)(OldStatus + SRWF_Link), (PVOID)OldStatus);
        if (CurrStatus == OldStatus) RtlpSrwWake(SRWLock, OldStatus + SRWF_Link);
    }
}

VOID NTAPI RtlReleaseSRWLockShared(PRTL_SRWLOCK SRWLock)
{
    SRW_STATE CurrStatus, NewStatus;
    ULONG count;
    SRW_STATE OldStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, NULL, (PVOID)(((1 << SRW_COUNT_BIT) | SRWF_Hold)));
    if (OldStatus == ((1 << SRW_COUNT_BIT) | SRWF_Hold)) return;

    if (!(OldStatus & SRWF_Wait))
    {
        do
        {
            if ((OldStatus & (SRWM_ITEM)) <= (1 << SRW_COUNT_BIT)) NewStatus = 0;
            else NewStatus = OldStatus - (1 << SRW_COUNT_BIT);
            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)NewStatus, (PVOID)OldStatus);
            if (CurrStatus == OldStatus) return;
            OldStatus = CurrStatus;
        } while (!(OldStatus & SRWF_Wait));
    }

    if (OldStatus & SRWF_Many)
    {
    SRW_WAIT_NODE* curr = (SRW_WAIT_NODE*)(OldStatus & SRWM_ITEM);
    while (curr->head == NULL) curr = curr->prev;
    curr = curr->head;
    count = InterlockedDecrement((PLONG)&curr->shareSnapshot);
        if (count > 0) return;
    }

    while (1)
    {
        NewStatus = OldStatus & (~(SRWF_Many | SRWF_Hold));
        if (OldStatus & SRWF_Link)
        {
            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)NewStatus, (PVOID)OldStatus);
            if (CurrStatus == OldStatus) return;
        }
        else
        {
            NewStatus |= SRWF_Link;
            CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)NewStatus, (PVOID)OldStatus);
            if (CurrStatus == OldStatus)
            {
                RtlpSrwWake(SRWLock, NewStatus);
                return;
            }
        }
        OldStatus = CurrStatus;
    }
}

BOOLEAN NTAPI RtlTryAcquireSRWLockExclusive(PRTL_SRWLOCK SRWLock)
{
    return !(RtlpInterlockedBit0SetPointer(&SRWLock->Ptr) == TRUE);
}

BOOLEAN NTAPI RtlTryAcquireSRWLockShared(PRTL_SRWLOCK SRWLock)
{
    ULONG backoff = 0;
    SRW_STATE NewStatus;
    SRW_STATE CurrStatus;
    SRW_STATE OldStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)((1 << SRW_COUNT_BIT) | SRWF_Hold), NULL);
    if (OldStatus == 0) return TRUE;
    while (1)
    {
        if ((OldStatus & SRWF_Hold) && ((OldStatus & SRWF_Wait) || (OldStatus & SRWM_ITEM) == (SRW_STATE)NULL))
            return FALSE;
        if (OldStatus & SRWF_Wait) NewStatus = OldStatus + SRWF_Hold;
        else NewStatus = OldStatus + (1 << SRW_COUNT_BIT);
        CurrStatus = (SRW_STATE)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)NewStatus, (PVOID)OldStatus);
        if (CurrStatus == OldStatus) return TRUE;
        RtlpBackoffExp(&backoff);
        OldStatus = (SRW_STATE)SRWLock->Ptr;
    }
}
