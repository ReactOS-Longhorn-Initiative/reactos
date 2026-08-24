#pragma once

/*
 * USER side state of a kernel job object that has UI restrictions applied.
 * One of these exists for every job whose UIRestrictionsClass is non-zero.
 */
typedef struct _W32JOB
{
    struct _W32JOB *pNext;          /* Next job in gpW32JobList */
    PEJOB           pEJob;          /* The kernel job object we belong to */
    PRTL_ATOM_TABLE pAtomTable;     /* Private global atom table, when the job
                                       is JOB_OBJECT_UILIMIT_GLOBALATOMS */
    ULONG           Restrictions;   /* JOB_OBJECT_UILIMIT_* */
    ULONG           uProcessCount;  /* Entries used in ppiTable */
    ULONG           uMaxProcesses;  /* Entries allocated in ppiTable */
    PPROCESSINFO   *ppiTable;       /* Processes of this job known to USER */
    ULONG           ughCrt;         /* Entries used in pgh */
    ULONG           ughMax;         /* Entries allocated in pgh */
    HANDLE         *pgh;            /* USER handles explicitly granted to the job */
} W32JOB, *PW32JOB;

CODE_SEG("INIT") NTSTATUS NTAPI InitJobImpl(VOID);

NTSTATUS NTAPI Win32kJobCallout(_In_ PWIN32_JOBCALLOUT_PARAMETERS Parameters);

NTSTATUS FASTCALL IntJobConnectProcess(_In_ PPROCESSINFO ppi);
VOID FASTCALL IntJobDisconnectProcess(_In_ PPROCESSINFO ppi);

BOOL FASTCALL IntIsJobRestricted(_In_ PPROCESSINFO ppi, _In_ ULONG Restriction);
BOOL FASTCALL IntIsCurrentJobRestricted(_In_ ULONG Restriction);

BOOL FASTCALL IntUserHandleGrantAccess(
    _In_ HANDLE hUserHandle,
    _In_ PW32JOB pJob,
    _In_ BOOL bGrant);

BOOL FASTCALL IntIsHandleGrantedToJob(_In_ PW32JOB pJob, _In_ HANDLE hUserHandle);
