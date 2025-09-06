/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Condition Variable Routines
 * PROGRAMMERS:       Thomas Weidenmueller <w3seek@reactos.com>
 *                    Stephan A. R�ger
 */

/* NOTE: This functionality can be optimized for releasing single
   threads or for releasing all waiting threads at once. This
   implementation is optimized for releasing a single thread at a time.
   It wakes up sleeping threads in FIFO order. */

/* INCLUDES ******************************************************************/

#include <rtl_vista.h>

#define NDEBUG
#include <debug.h>

/* INTERNAL TYPES ************************************************************/

/* Internal wait-node used for condition variable queues. Cooperates with SRW. */
typedef struct DECLSPEC_ALIGN(16) _CV_WAIT_NODE
{
    struct _CV_WAIT_NODE* prev;     /* stack link while building */
    struct _CV_WAIT_NODE* notify;   /* head of a FIFO chain to wake */
    struct _CV_WAIT_NODE* next;     /* temporary forward link */
    SIZE_T  shared;                 /* snapshot of shared count for SRW handoff */
    SIZE_T  flags;                  /* bit 0: exclusive-hint, bit 1: spin, bit 2: shared-hint */
    PRTL_SRWLOCK  Srw;              /* optional SRW lock to requeue onto */
    SIZE_T wakerTid;                /* used by 2-phase keyed-event wake */
} CV_WAIT_NODE;

#define CV_MASK  (0x0000000F)
#define CV_NODE(CV) ((CV_WAIT_NODE*)((SIZE_T)(CV) & (~(SIZE_T)CV_MASK)))

/* Flag helpers (kept numerically identical to preserve behavior) */
#define CVF_EXCLUSIVE_HINT  0x1
#define CVF_SPIN            0x2
#define CVF_SHARED_HINT     0x4

/* Shared keyed event handle used across SRW and condvars. */
HANDLE GlobalKeyedEventHandle = NULL;

static VOID InitializeGlobalKeyedEventHandle(void)
{
    if (GlobalKeyedEventHandle == NULL)
    {
        NtCreateKeyedEvent(&GlobalKeyedEventHandle, EVENT_ALL_ACCESS, NULL, 0);
    }
}

/* Keep compatibility with ntdll initialization which calls these. */
VOID NTAPI RtlpInitializeKeyedEvent(VOID)
{
    InitializeGlobalKeyedEventHandle();
}

VOID NTAPI RtlpCloseKeyedEvent(VOID)
{
    HANDLE h = (HANDLE)InterlockedExchangePointer((PVOID*)&GlobalKeyedEventHandle, NULL);
    if (h)
    {
        NtClose(h);
    }
}

static NTSTATUS NTAPI CvWaitKeyedEventSecondPhase(
    IN HANDLE KeyedEventHandle,
    IN CV_WAIT_NODE* Key,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL)
{
    if (Timeout == NULL)
    {
        LARGE_INTEGER _t; _t.QuadPart = 0;
        while (Key->wakerTid == 0)
        {
            NTSTATUS s = NtWaitForKeyedEvent(KeyedEventHandle, Key, Alertable, &_t);
            if (s != STATUS_TIMEOUT) return STATUS_TIMEOUT;
        }
        _t.QuadPart = -50000; /* ~5ms */
        NtWaitForKeyedEvent(KeyedEventHandle, Key, Alertable, &_t);
        return STATUS_SUCCESS;
    }
    return NtWaitForKeyedEvent(KeyedEventHandle, Key, Alertable, Timeout);
}

static NTSTATUS NTAPI CvSignalKeyedEventWithMark(
    IN HANDLE KeyedEventHandle,
    IN CV_WAIT_NODE* Key,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL)
{
    InterlockedExchangePointer((PVOID*)&Key->wakerTid, NtCurrentTeb()->ClientId.UniqueThread);
    return NtReleaseKeyedEvent(KeyedEventHandle, Key, Alertable, Timeout);
}

static VOID __fastcall CvNormalizeWaitChain(PRTL_CONDITION_VARIABLE ConditionVariable, SIZE_T ConditionVariableStatus)
{
    CV_WAIT_NODE *pWaitNode;
    CV_WAIT_NODE *pItem;
    CV_WAIT_NODE *temp;
    SIZE_T LastStatus;

    for (;;)
    {
        pWaitNode = CV_NODE(ConditionVariableStatus);
        pItem = pWaitNode;

        for (; pItem->notify == NULL;)
        {
            temp = pItem;
            pItem = pItem->prev;
            pItem->next = temp;
        }

        pWaitNode->notify = pItem->notify;

        LastStatus = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)pWaitNode, (PVOID)ConditionVariableStatus);
        if (LastStatus == ConditionVariableStatus) return;
        if (LastStatus & 7)
        {
            /* Someone is waking; delegate. */
            /* fall back to wake path below */
            break;
        }
        ConditionVariableStatus = LastStatus;
    }
}

static BOOL __fastcall CvTryAttachToSrw(CV_WAIT_NODE* node, RTL_SRWLOCK *SRWLock, ULONG SrwSharedMark)
{
    SIZE_T shared;
    SIZE_T Current;
    SIZE_T New;
    ULONG backoff = 0;

    for (;;)
    {
        Current = (SIZE_T)SRWLock->Ptr;
        if ((Current & 0x1) == 0) break;

        if (SrwSharedMark == 0)
        {
            node->flags |= CVF_EXCLUSIVE_HINT;
        }
        else if ((Current & 0x2) == 0 && (Current & ~0xF))
        {
            return FALSE;
        }

        node->next = NULL;
        if (Current & 0x2)
        {
            node->notify = NULL;
            node->shared = 0;
            node->prev = (CV_WAIT_NODE*)(Current & ~0xF);
            New = (SIZE_T)node | (Current & 0xF);
        }
        else
        {
            shared = Current >> 4;
            node->shared = shared;
            node->notify = node;
            New = shared <= 1 ? (SIZE_T)node | 0x3 : (SIZE_T)node | 0xB;
        }

        if ((SIZE_T)InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)New, (PVOID)Current) == Current)
            return TRUE;
        backoff++;
        YieldProcessor();
    }
    return FALSE;
}

static VOID __fastcall CvDispatchWake(PRTL_CONDITION_VARIABLE *ConditionVariable, SIZE_T ConditionVariableStatus, SIZE_T WakeCount)
{
    CV_WAIT_NODE* notify = NULL;
    CV_WAIT_NODE* toWake = NULL;
    CV_WAIT_NODE* waitHead;
    CV_WAIT_NODE* block;
    CV_WAIT_NODE* tmp;
    CV_WAIT_NODE* next;
    CV_WAIT_NODE* prev;
    CV_WAIT_NODE** insertAt = &toWake;
    SIZE_T LastStatus;
    SIZE_T MaxWakeCount;
    SIZE_T Count = 0;

    for (;;)
    {
        waitHead = CV_NODE(ConditionVariableStatus);
        if ((ConditionVariableStatus & 0x7) == 0x7)
        {
            ConditionVariableStatus = (SIZE_T)InterlockedExchangePointer((PVOID*)ConditionVariable, 0);
            *insertAt = CV_NODE(ConditionVariableStatus);
            break;
        }

        MaxWakeCount = WakeCount + (ConditionVariableStatus & 7);
        block = waitHead;
        for (; block->notify == NULL;)
        {
            tmp = block;
            block = block->prev;
            block->next = tmp;
        }

        if (MaxWakeCount <= Count)
        {
            LastStatus = (SIZE_T)InterlockedCompareExchangePointer(&(*ConditionVariable)->Ptr, (PVOID)(waitHead), (PVOID)ConditionVariableStatus);
            if (LastStatus == ConditionVariableStatus) break;
            ConditionVariableStatus = LastStatus;
        }
        else
        {
            notify = block->notify;
            while (MaxWakeCount > Count && notify->next)
            {
                ++Count;
                *insertAt = notify;
                notify->prev = NULL;
                next = notify->next;
                waitHead->notify = next;
                next->prev = NULL;
                insertAt = &notify->prev;
                notify = next;
            }

            if (MaxWakeCount <= Count)
            {
                LastStatus = (SIZE_T)InterlockedCompareExchangePointer(&(*ConditionVariable)->Ptr, (PVOID)(waitHead), (PVOID)ConditionVariableStatus);
                if (LastStatus == ConditionVariableStatus) break;
                ConditionVariableStatus = LastStatus;
            }
            else
            {
                LastStatus = (SIZE_T)InterlockedCompareExchangePointer(&(*ConditionVariable)->Ptr, 0, (PVOID)ConditionVariableStatus);
                if (LastStatus == ConditionVariableStatus)
                {
                    *insertAt = notify;
                    notify->prev = 0;
                    break;
                }
                ConditionVariableStatus = LastStatus;
            }
        }
    }

    for (; toWake;)
    {
        prev = toWake->prev;
        if (!InterlockedBitTestAndReset((PLONG)&toWake->flags, 1))
        {
            if (toWake->Srw == NULL || CvTryAttachToSrw(toWake, toWake->Srw, (ULONG)((toWake->flags >> 2) & 0x1)) == FALSE)
            {
                CvSignalKeyedEventWithMark(GlobalKeyedEventHandle, toWake, FALSE, NULL);
            }
        }
        toWake = prev;
    }
}

static BOOL __fastcall CvTryStealSelf(PRTL_CONDITION_VARIABLE ConditionVariable, CV_WAIT_NODE* node)
{
    SIZE_T Current = (SIZE_T)ConditionVariable->Ptr;
    CV_WAIT_NODE *pWaitNode;
    CV_WAIT_NODE *pSuccessor;
    SIZE_T Last;
    SIZE_T New;
    SIZE_T back;
    CV_WAIT_NODE* notify;
    BOOL bRet;

    while (Current && (Current & 0x7) != 0x7)
    {
        if (Current & 0x8)
        {
            Last = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)(Current | 0x7), (PVOID)Current);
            if (Last == Current) return FALSE;
            Current = Last;
        }
        else
        {
            New = Current | 0x8;
            Last = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)New, (PVOID)Current);
            if (Last == Current)
            {
                Current = New;
                notify = NULL;
                bRet = FALSE;
                pWaitNode = CV_NODE(Current);
                pSuccessor = pWaitNode;
                if (pWaitNode)
                {
                    while (pWaitNode)
                    {
                        if (pWaitNode == node)
                        {
                            if (notify)
                            {
                                pWaitNode = pWaitNode->prev;
                                bRet = TRUE;
                                notify->prev = pWaitNode;
                                if (!pWaitNode) break;
                                pWaitNode->next = notify;
                            }
                            else
                            {
                                back = (SIZE_T)(pWaitNode->prev);
                                New = back == 0 ? back : back ^ ((New ^ back) & 0xF);
                                Last = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)New, (PVOID)Current);
                                if (Last == Current)
                                {
                                    Current = New;
                                    if (back == 0) return TRUE;
                                    bRet = TRUE;
                                }
                                else
                                {
                                    Current = Last;
                                }
                                pSuccessor = pWaitNode = CV_NODE(Current);
                                notify = NULL;
                            }
                        }
                        else
                        {
                            pWaitNode->next = notify;
                            notify = pWaitNode;
                            pWaitNode = pWaitNode->prev;
                        }
                    }
                    if (pSuccessor) pSuccessor->notify = notify;
                }
                CvDispatchWake(&ConditionVariable, Current, 0);
                return bRet;
            }
            Current = Last;
        }
    }
    return FALSE;
}

/* EXPORTED FUNCTIONS ********************************************************/

VOID NTAPI RtlInitializeConditionVariable(OUT PRTL_CONDITION_VARIABLE ConditionVariable)
{
    ConditionVariable->Ptr = NULL;
}

VOID NTAPI RtlWakeConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable)
{
    SIZE_T Current;
    SIZE_T Last;

    Current = (SIZE_T)ConditionVariable->Ptr;
    for (; Current; Current = Last)
    {
        if (Current & 0x8)
        {
            if ((Current & 0x7) == 0x7) return;
            Last = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)(Current + 1), (PVOID)Current);
            if (Last == Current) return;
        }
        else
        {
            Last = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)(Current | 0x8), (PVOID)Current);
            if (Last == Current)
            {
                InitializeGlobalKeyedEventHandle();
                CvDispatchWake(&ConditionVariable, Current + 8, 1);
                return;
            }
        }
    }
}

VOID NTAPI RtlWakeAllConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable)
{
    SIZE_T Current = (SIZE_T)ConditionVariable->Ptr;
    SIZE_T Last;
    CV_WAIT_NODE* node;
    CV_WAIT_NODE* Tmp;

    for (; Current && (Current & 0x7) != 0x7; Current = Last)
    {
        if (Current & 0x8)
        {
            Last = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)(Current | 0x7), (PVOID)Current);
            if (Last == Current) return;
        }
        else
        {
            Last = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, 0, (PVOID)Current);
            if (Last == Current)
            {
                InitializeGlobalKeyedEventHandle();
                for (node = CV_NODE(Current); node;)
                {
                    Tmp = node->prev;
                    if (!InterlockedBitTestAndReset((PLONG)&node->flags, 1))
                    {
                        CvSignalKeyedEventWithMark(GlobalKeyedEventHandle, node, FALSE, NULL);
                    }
                    node = Tmp;
                }
                return;
            }
        }
    }
}

NTSTATUS NTAPI RtlSleepConditionVariableCS(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                                           IN OUT PRTL_CRITICAL_SECTION CriticalSection,
                                           IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    CV_WAIT_NODE StackNode;
    SIZE_T OldConditionVariable;
    SIZE_T NewConditionVariable;
    SIZE_T LastConditionVariable;
    SIZE_T SpinCount;
    NTSTATUS Status = STATUS_SUCCESS;

    StackNode.next = NULL;
    StackNode.flags = CVF_SPIN;
    StackNode.Srw = NULL;
    StackNode.wakerTid = 0;
    OldConditionVariable = (SIZE_T)ConditionVariable->Ptr;

    for (;;)
    {
        NewConditionVariable = (SIZE_T)(&StackNode) | (OldConditionVariable & CV_MASK);
        StackNode.prev = CV_NODE(OldConditionVariable);
        if (StackNode.prev)
        {
            StackNode.notify = NULL;
            NewConditionVariable |= 0x8;
        }
        else
        {
            StackNode.notify = &StackNode;
        }
        LastConditionVariable = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)NewConditionVariable, (PVOID)OldConditionVariable);
        if (LastConditionVariable == OldConditionVariable) break;
        OldConditionVariable = LastConditionVariable;
    }

    RtlLeaveCriticalSection(CriticalSection);
    if ((OldConditionVariable ^ NewConditionVariable) & 0x8)
    {
    CvNormalizeWaitChain(ConditionVariable, NewConditionVariable);
    }

    InitializeGlobalKeyedEventHandle();
    for (SpinCount = 1024; SpinCount; --SpinCount)
    {
        if (!(StackNode.flags & CVF_SPIN)) break;
        YieldProcessor();
    }
    if (InterlockedBitTestAndReset((PLONG)&StackNode.flags, 1))
    {
        Status = NtWaitForKeyedEvent(GlobalKeyedEventHandle, &StackNode, FALSE, TimeOut);
        if (Status == STATUS_TIMEOUT && CvTryStealSelf(ConditionVariable, &StackNode) == FALSE)
        {
            CvWaitKeyedEventSecondPhase(GlobalKeyedEventHandle, &StackNode, FALSE, NULL);
            Status = STATUS_SUCCESS;
        }
    }
    RtlEnterCriticalSection(CriticalSection);
    return Status;
}

NTSTATUS NTAPI RtlSleepConditionVariableSRW(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                                            IN OUT PRTL_SRWLOCK SRWLock,
                                            IN PLARGE_INTEGER TimeOut OPTIONAL,
                                            IN ULONG Flags)
{
    SIZE_T SpinCount;
    CV_WAIT_NODE StackNode;
    SIZE_T Current;
    SIZE_T New;
    SIZE_T Last;
    NTSTATUS Status = STATUS_SUCCESS;

    if (Flags & ~RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        return STATUS_INVALID_PARAMETER;

    StackNode.next = NULL;
    StackNode.flags = CVF_SPIN;
    StackNode.Srw = NULL;
    StackNode.wakerTid = 0;
    if (Flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED) StackNode.flags |= CVF_SHARED_HINT;

    Current = (SIZE_T)ConditionVariable->Ptr;
    for (;;)
    {
        New = (SIZE_T)(&StackNode) | (Current & CV_MASK);
        StackNode.prev = CV_NODE(Current);
        if (StackNode.prev)
        {
            StackNode.notify = NULL;
            New |= 0x8;
        }
        else
        {
            StackNode.notify = &StackNode;
        }
        Last = (SIZE_T)InterlockedCompareExchangePointer(&ConditionVariable->Ptr, (PVOID)New, (PVOID)Current);
        if (Last == Current) break;
        Current = Last;
    }

    if (Flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        RtlReleaseSRWLockShared(SRWLock);
    else
        RtlReleaseSRWLockExclusive(SRWLock);

    if ((Current ^ New) & 0x8) CvNormalizeWaitChain(ConditionVariable, New);
    InitializeGlobalKeyedEventHandle();
    for (SpinCount = 1024; SpinCount; --SpinCount)
    {
        if (!(StackNode.flags & CVF_SPIN)) break;
        YieldProcessor();
    }
    if (InterlockedBitTestAndReset((PLONG)&StackNode.flags, 1))
    {
        Status = NtWaitForKeyedEvent(GlobalKeyedEventHandle, &StackNode, FALSE, TimeOut);
        if (Status == STATUS_TIMEOUT && CvTryStealSelf(ConditionVariable, &StackNode) == FALSE)
        {
            CvWaitKeyedEventSecondPhase(GlobalKeyedEventHandle, &StackNode, FALSE, NULL);
            Status = STATUS_SUCCESS;
        }
    }

    if (Flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
    RtlAcquireSRWLockShared(SRWLock);
    else
    RtlAcquireSRWLockExclusive(SRWLock);

    return Status;
}

/* EOF */
