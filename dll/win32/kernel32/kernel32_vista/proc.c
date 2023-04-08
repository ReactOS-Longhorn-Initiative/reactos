#include "k32_vista.h"

#define NDEBUG
#include <debug.h>


/***********************************************************************
 *           K32EnumProcesses (KERNEL32.@)
 */
BOOL WINAPI K32EnumProcesses(DWORD *lpdwProcessIDs, DWORD cb, DWORD *lpcbUsed)
{
    SYSTEM_PROCESS_INFORMATION *spi;
    ULONG size = 0x4000;
    void *buf = NULL;
    NTSTATUS status;

    do {
        size *= 2;
        HeapFree(GetProcessHeap(), 0, buf);
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (!buf)
            return FALSE;

        status = NtQuerySystemInformation(SystemProcessInformation, buf, size, NULL);
    } while(status == STATUS_INFO_LENGTH_MISMATCH);

    if (status != STATUS_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, buf);
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    spi = buf;

    for (*lpcbUsed = 0; cb >= sizeof(DWORD); cb -= sizeof(DWORD))
    {
        *lpdwProcessIDs++ = HandleToUlong(spi->UniqueProcessId);
        *lpcbUsed += sizeof(DWORD);

        if (spi->NextEntryOffset == 0)
            break;

        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->NextEntryOffset);
    }

    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

typedef struct _PROCESS_MEMORY_COUNTERS {
  DWORD  cb;
  DWORD  PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;
typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;

/***********************************************************************
 *           K32GetProcessMemoryInfo (KERNEL32.@)
 *
 * Retrieve memory usage information for a given process
 *
 */
BOOL WINAPI K32GetProcessMemoryInfo(HANDLE process,
                                    PPROCESS_MEMORY_COUNTERS pmc, DWORD cb)
{
    NTSTATUS status;
    VM_COUNTERS vmc;

    if (cb < sizeof(PROCESS_MEMORY_COUNTERS))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    status = NtQueryInformationProcess(process, ProcessVmCounters,
                                       &vmc, sizeof(vmc), NULL);

    if (status)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    pmc->cb = sizeof(PROCESS_MEMORY_COUNTERS);
    pmc->PageFaultCount = vmc.PageFaultCount;
    pmc->PeakWorkingSetSize = vmc.PeakWorkingSetSize;
    pmc->WorkingSetSize = vmc.WorkingSetSize;
    pmc->QuotaPeakPagedPoolUsage = vmc.QuotaPeakPagedPoolUsage;
    pmc->QuotaPagedPoolUsage = vmc.QuotaPagedPoolUsage;
    pmc->QuotaPeakNonPagedPoolUsage = vmc.QuotaPeakNonPagedPoolUsage;
    pmc->QuotaNonPagedPoolUsage = vmc.QuotaNonPagedPoolUsage;
    pmc->PagefileUsage = vmc.PagefileUsage;
    pmc->PeakPagefileUsage = vmc.PeakPagefileUsage;

    return TRUE;
}



/***********************************************************************
 *           InitializeProcThreadAttributeList   (kernelbase.@)
 */
struct proc_thread_attr
{
    DWORD_PTR attr;
    SIZE_T size;
    void *value;
};
struct _PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD mask;  /* bitmask of items in list */
    DWORD size;  /* max number of items in list */
    DWORD count; /* number of items in list */
    DWORD pad;
    DWORD_PTR unk;
    struct proc_thread_attr attrs[10];
};

BOOL WINAPI DECLSPEC_HOTPATCH InitializeProcThreadAttributeList( struct _PROC_THREAD_ATTRIBUTE_LIST *list,
                                                                 DWORD count, DWORD flags, SIZE_T *size )
{
    SIZE_T needed;
    BOOL ret = FALSE;

    needed = FIELD_OFFSET( struct _PROC_THREAD_ATTRIBUTE_LIST, attrs[count] );
    if (list && *size >= needed)
    {
        list->mask = 0;
        list->size = count;
        list->count = 0;
        list->unk = 0;
        ret = TRUE;
    }
    else SetLastError( ERROR_INSUFFICIENT_BUFFER );

    *size = needed;
    return ret;
}

BOOL 
K32QueryWorkingSet(
        HANDLE hProcess,
        PVOID  pv,
        DWORD  cb
)
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOL 
K32QueryWorkingSetEx(
        HANDLE hProcess,
        PVOID  pv,
        DWORD  cb
)
{
    UNIMPLEMENTED;
    return TRUE;
}