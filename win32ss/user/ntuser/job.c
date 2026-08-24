/*
 * PROJECT:     ReactOS Win32k Subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Job object UI restrictions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/*
 * A job object may restrict what the processes it contains are allowed to do
 * to the user interface (JOB_OBJECT_UILIMIT_*). The kernel stores the mask,
 * but it is win32k that has to honour it, so the kernel hands each restricted
 * job over to us through a callout (see PspInvokeW32JobCallout).
 *
 * For every such job we keep a W32JOB, which owns the list of USER-connected
 * processes of the job, the handles that have been explicitly granted to it
 * (see NtUserUserHandleGrantAccess) and, for JOB_OBJECT_UILIMIT_GLOBALATOMS,
 * a private global atom table.
 *
 * Locking: the kernel job lock is always acquired before the USER lock, never
 * the other way around. Callouts arrive with the job lock already held.
 */

#include <win32k.h>

DBG_DEFAULT_CHANNEL(UserProcess);

/* GLOBALS ********************************************************************/

static PW32JOB gpW32JobList = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * Looks up the W32JOB of a kernel job object.
 * The USER lock must be held.
 */
static
PW32JOB
IntFindW32Job(_In_ PEJOB pEJob)
{
    PW32JOB pJob;

    for (pJob = gpW32JobList; pJob != NULL; pJob = pJob->pNext)
    {
        if (pJob->pEJob == pEJob)
            return pJob;
    }

    return NULL;
}

/*
 * Creates the W32JOB of a kernel job object and links it into the global list.
 * The USER lock must be held.
 */
static
PW32JOB
IntCreateW32Job(_In_ PEJOB pEJob)
{
    PW32JOB pJob;

    pJob = ExAllocatePoolZero(PagedPool, sizeof(W32JOB), USERTAG_W32JOB);
    if (pJob == NULL)
    {
        ERR("Failed to allocate a W32JOB for job %p\n", pEJob);
        return NULL;
    }

    pJob->pEJob = pEJob;

    pJob->pNext = gpW32JobList;
    gpW32JobList = pJob;

    TRACE("Created W32JOB %p for job %p\n", pJob, pEJob);
    return pJob;
}

/*
 * Unlinks and frees a W32JOB, detaching any process still pointing at it.
 * The USER lock must be held.
 */
static
VOID
IntDestroyW32Job(_In_ PW32JOB pJob)
{
    PW32JOB *ppJob;
    ULONG i;

    /* Unlink it first, so that nothing can find it any more */
    for (ppJob = &gpW32JobList; *ppJob != NULL; ppJob = &(*ppJob)->pNext)
    {
        if (*ppJob == pJob)
        {
            *ppJob = pJob->pNext;
            break;
        }
    }

    /* Detach the processes that are still around */
    for (i = 0; i < pJob->uProcessCount; i++)
    {
        if (pJob->ppiTable[i] != NULL)
            pJob->ppiTable[i]->pW32Job = NULL;
    }

    if (pJob->ppiTable != NULL)
        ExFreePoolWithTag(pJob->ppiTable, USERTAG_W32JOBEXTRA);

    if (pJob->pgh != NULL)
        ExFreePoolWithTag(pJob->pgh, USERTAG_W32JOBEXTRA);

    if (pJob->pAtomTable != NULL)
        RtlDestroyAtomTable(pJob->pAtomTable);

    TRACE("Destroyed W32JOB %p\n", pJob);
    ExFreePoolWithTag(pJob, USERTAG_W32JOB);
}

/*
 * Grows one of the job's arrays geometrically.
 * Returns the new array, or NULL when out of memory (the old one is kept).
 */
static
PVOID
IntGrowJobArray(
    _In_opt_ PVOID pOldArray,
    _In_ ULONG uUsed,
    _Inout_ PULONG puMax,
    _In_ ULONG cbEntry)
{
    PVOID pNewArray;
    ULONG uNewMax;

    uNewMax = (*puMax == 0) ? 4 : (*puMax * 2);

    pNewArray = ExAllocatePoolZero(PagedPool, uNewMax * cbEntry, USERTAG_W32JOBEXTRA);
    if (pNewArray == NULL)
        return NULL;

    if (pOldArray != NULL)
    {
        RtlCopyMemory(pNewArray, pOldArray, uUsed * cbEntry);
        ExFreePoolWithTag(pOldArray, USERTAG_W32JOBEXTRA);
    }

    *puMax = uNewMax;
    return pNewArray;
}

/*
 * Adds a USER-connected process to a job.
 * The job lock and the USER lock must be held.
 */
static
NTSTATUS
IntJobAddProcessLocked(
    _In_ PW32JOB pJob,
    _In_ PPROCESSINFO ppi)
{
    ULONG i;

    /* Already a member? */
    for (i = 0; i < pJob->uProcessCount; i++)
    {
        if (pJob->ppiTable[i] == ppi)
            return STATUS_SUCCESS;
    }

    if (pJob->uProcessCount >= pJob->uMaxProcesses)
    {
        PPROCESSINFO *ppiTable;

        ppiTable = IntGrowJobArray(pJob->ppiTable,
                                   pJob->uProcessCount,
                                   &pJob->uMaxProcesses,
                                   sizeof(PPROCESSINFO));
        if (ppiTable == NULL)
            return STATUS_NO_MEMORY;

        pJob->ppiTable = ppiTable;
    }

    pJob->ppiTable[pJob->uProcessCount++] = ppi;
    ppi->pW32Job = pJob;

    TRACE("Process %p joined W32JOB %p (restrictions 0x%lx)\n",
          ppi, pJob, pJob->Restrictions);

    return STATUS_SUCCESS;
}

/*
 * Applies a new restriction mask to a job, creating its W32JOB on first use.
 * The job lock must be held.
 */
static
NTSTATUS
IntJobSetRestrictions(
    _In_ PEJOB pEJob,
    _In_ ULONG Restrictions)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PW32JOB pJob;

    UserEnterExclusive();

    pJob = IntFindW32Job(pEJob);

    /* Dropping all restrictions: the job no longer needs any state here */
    if (Restrictions == 0)
    {
        if (pJob != NULL)
            IntDestroyW32Job(pJob);

        goto Quit;
    }

    if (pJob == NULL)
    {
        pJob = IntCreateW32Job(pEJob);
        if (pJob == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Quit;
        }

        /* The job may already contain processes that connected to USER before
           the restrictions were applied. Collect them now. */
        {
            PPROCESSINFO ppi;

            for (ppi = gppiList; ppi != NULL; ppi = ppi->ppiNext)
            {
                if (ppi->peProcess != NULL &&
                    PsGetProcessJob(ppi->peProcess) == pEJob)
                {
                    Status = IntJobAddProcessLocked(pJob, ppi);
                    if (!NT_SUCCESS(Status))
                    {
                        IntDestroyW32Job(pJob);
                        goto Quit;
                    }
                }
            }
        }
    }

    /* A private atom table is only needed while GLOBALATOMS is restricted */
    if (Restrictions & JOB_OBJECT_UILIMIT_GLOBALATOMS)
    {
        if (pJob->pAtomTable == NULL)
        {
            Status = RtlCreateAtomTable(37, &pJob->pAtomTable);
            if (!NT_SUCCESS(Status))
            {
                ERR("Failed to create the atom table of job %p: 0x%lx\n", pEJob, Status);
                goto Quit;
            }
        }
    }
    else if (pJob->pAtomTable != NULL)
    {
        RtlDestroyAtomTable(pJob->pAtomTable);
        pJob->pAtomTable = NULL;
    }

    pJob->Restrictions = Restrictions;

Quit:
    UserLeave();
    return Status;
}

/*
 * Handles PsW32JobCalloutAddProcess. The job lock is held by the caller.
 */
static
NTSTATUS
IntJobAddProcess(
    _In_ PEJOB pEJob,
    _In_ PPROCESSINFO ppi)
{
    NTSTATUS Status;
    PW32JOB pJob;

    UserEnterExclusive();

    pJob = IntFindW32Job(pEJob);
    if (pJob == NULL)
    {
        /* The kernel only calls us for jobs that have restrictions, so we
           should have been told about this job already. */
        ERR("No W32JOB for job %p\n", pEJob);
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        Status = IntJobAddProcessLocked(pJob, ppi);
    }

    UserLeave();
    return Status;
}

/*
 * Handles PsW32JobCalloutTerminate. The job object is going away.
 */
static
NTSTATUS
IntJobTerminate(_In_ PEJOB pEJob)
{
    PW32JOB pJob;

    UserEnterExclusive();

    pJob = IntFindW32Job(pEJob);
    if (pJob != NULL)
        IntDestroyW32Job(pJob);

    UserLeave();
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * The kernel's entry point into the job support of win32k.
 */
NTSTATUS
NTAPI
Win32kJobCallout(_In_ PWIN32_JOBCALLOUT_PARAMETERS Parameters)
{
    switch (Parameters->CalloutType)
    {
        case PsW32JobCalloutSetInformation:
            return IntJobSetRestrictions((PEJOB)Parameters->Job,
                                         PtrToUlong(Parameters->Data));

        case PsW32JobCalloutAddProcess:
            return IntJobAddProcess((PEJOB)Parameters->Job,
                                    (PPROCESSINFO)Parameters->Data);

        case PsW32JobCalloutTerminate:
            return IntJobTerminate((PEJOB)Parameters->Job);

        default:
            ERR("Unknown job callout %d\n", Parameters->CalloutType);
            return STATUS_INVALID_PARAMETER;
    }
}

/*
 * Called when a process connects to USER. A process is usually assigned to its
 * job while it is still suspended, long before it ever calls into win32k, so
 * this is where restricted processes normally join their job.
 *
 * Must be called without the USER lock held.
 */
NTSTATUS
FASTCALL
IntJobConnectProcess(_In_ PPROCESSINFO ppi)
{
    NTSTATUS Status;
    PERESOURCE JobLock;
    PEJOB pEJob;

    pEJob = PsGetProcessJob(ppi->peProcess);
    if (pEJob == NULL)
        return STATUS_SUCCESS;

    /* Unrestricted jobs have no state in win32k */
    if (PsGetJobUIRestrictionsClass(pEJob) == 0)
        return STATUS_SUCCESS;

    JobLock = (PERESOURCE)PsGetJobLock(pEJob);

    ExEnterCriticalRegionAndAcquireResourceExclusive(JobLock);
    Status = IntJobAddProcess(pEJob, ppi);
    ExReleaseResourceAndLeaveCriticalRegion(JobLock);

    return Status;
}

/*
 * Called when a process disconnects from USER.
 */
VOID
FASTCALL
IntJobDisconnectProcess(_In_ PPROCESSINFO ppi)
{
    PW32JOB pJob;
    ULONG i;

    UserEnterExclusive();

    pJob = ppi->pW32Job;
    if (pJob == NULL)
    {
        UserLeave();
        return;
    }

    for (i = 0; i < pJob->uProcessCount; i++)
    {
        if (pJob->ppiTable[i] == ppi)
        {
            /* Keep the table dense */
            pJob->ppiTable[i] = pJob->ppiTable[pJob->uProcessCount - 1];
            pJob->ppiTable[pJob->uProcessCount - 1] = NULL;
            pJob->uProcessCount--;
            break;
        }
    }

    ppi->pW32Job = NULL;

    UserLeave();
}

/*
 * Tests a single JOB_OBJECT_UILIMIT_* restriction of a process.
 */
BOOL
FASTCALL
IntIsJobRestricted(
    _In_ PPROCESSINFO ppi,
    _In_ ULONG Restriction)
{
    if (ppi == NULL || ppi->pW32Job == NULL)
        return FALSE;

    return (ppi->pW32Job->Restrictions & Restriction) != 0;
}

/*
 * Tests a single JOB_OBJECT_UILIMIT_* restriction of the calling process.
 */
BOOL
FASTCALL
IntIsCurrentJobRestricted(_In_ ULONG Restriction)
{
    PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();

    return IntIsJobRestricted(ppi, Restriction);
}

/*
 * Grants or revokes access to a USER handle for a job whose processes are
 * restricted by JOB_OBJECT_UILIMIT_HANDLES.
 */
BOOL
APIENTRY
NtUserUserHandleGrantAccess(
    IN HANDLE hUserHandle,
    IN HANDLE hJob,
    IN BOOL bGrant)
{
    PEJOB pEJob;
    PW32JOB pJob;
    NTSTATUS Status;
    BOOL Ret = FALSE;

    if (hUserHandle == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Status = ObReferenceObjectByHandle(hJob,
                                       JOB_OBJECT_SET_ATTRIBUTES,
                                       PsJobType,
                                       UserMode,
                                       (PVOID*)&pEJob,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    UserEnterExclusive();

    pJob = IntFindW32Job(pEJob);
    if (pJob == NULL)
    {
        /* Only a job that restricts USER handles keeps a granted list */
        EngSetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        Ret = IntUserHandleGrantAccess(hUserHandle, pJob, bGrant);
    }

    UserLeave();

    ObDereferenceObject(pEJob);
    return Ret;
}

/*
 * Tests whether a USER handle has been explicitly granted to a job.
 * The USER lock must be held.
 */
BOOL
FASTCALL
IntIsHandleGrantedToJob(
    _In_ PW32JOB pJob,
    _In_ HANDLE hUserHandle)
{
    ULONG i;

    for (i = 0; i < pJob->ughCrt; i++)
    {
        if (pJob->pgh[i] == hUserHandle)
            return TRUE;
    }

    return FALSE;
}

/*
 * Grants or revokes access to a USER handle for the processes of a job that is
 * otherwise restricted by JOB_OBJECT_UILIMIT_HANDLES.
 * The USER lock must be held.
 */
BOOL
FASTCALL
IntUserHandleGrantAccess(
    _In_ HANDLE hUserHandle,
    _In_ PW32JOB pJob,
    _In_ BOOL bGrant)
{
    ULONG i;

    if (!bGrant)
    {
        for (i = 0; i < pJob->ughCrt; i++)
        {
            if (pJob->pgh[i] == hUserHandle)
            {
                pJob->pgh[i] = pJob->pgh[pJob->ughCrt - 1];
                pJob->pgh[pJob->ughCrt - 1] = NULL;
                pJob->ughCrt--;
                return TRUE;
            }
        }

        /* Revoking an access that was never granted is not an error */
        return TRUE;
    }

    if (IntIsHandleGrantedToJob(pJob, hUserHandle))
        return TRUE;

    if (pJob->ughCrt >= pJob->ughMax)
    {
        HANDLE *pgh;

        pgh = IntGrowJobArray(pJob->pgh,
                              pJob->ughCrt,
                              &pJob->ughMax,
                              sizeof(HANDLE));
        if (pgh == NULL)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        pJob->pgh = pgh;
    }

    pJob->pgh[pJob->ughCrt++] = hUserHandle;
    return TRUE;
}

/* EOF */
