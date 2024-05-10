
#include <k32_vista.h>

#if _WIN32_WINNT != _WIN32_WINNT_VISTA
#error "This file must be compiled with _WIN32_WINNT == _WIN32_WINNT_VISTA"
#endif

// This is defined only in ntifs.h
#define REPARSE_DATA_BUFFER_HEADER_SIZE   FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

#define NDEBUG
#include <debug.h>



typedef struct _PERFORMANCE_INFORMATION {
	DWORD cb;
	SIZE_T CommitTotal;
	SIZE_T CommitLimit;
	SIZE_T CommitPeak;
	SIZE_T PhysicalTotal;
	SIZE_T PhysicalAvailable;
	SIZE_T SystemCache;
	SIZE_T KernelTotal;
	SIZE_T KernelPaged;
	SIZE_T KernelNonpaged;
	SIZE_T PageSize;
	DWORD HandleCount;
	DWORD ProcessCount;
	DWORD ThreadCount;
} PERFORMANCE_INFORMATION, *PPERFORMANCE_INFORMATION;


/***********************************************************************
 *         K32GetPerformanceInfo   (kernelbase.@)
 */
BOOL WINAPI K32GetPerformanceInfo( PPERFORMANCE_INFORMATION info, DWORD size )
{
    SYSTEM_PERFORMANCE_INFORMATION perf;
    SYSTEM_BASIC_INFORMATION basic;
    SYSTEM_PROCESS_INFORMATION *process, *spi;
    DWORD info_size;
    NTSTATUS status;

    DPRINT1( "(%p, %d)\n", info, size );

    if (size < sizeof(*info))
    {
        SetLastError( ERROR_BAD_LENGTH );
        return FALSE;
    }

    status = NtQuerySystemInformation( SystemPerformanceInformation, &perf, sizeof(perf), NULL );
    if (!NT_SUCCESS( status )) return FALSE;
    status = NtQuerySystemInformation( SystemBasicInformation, &basic, sizeof(basic), NULL );
    if (!NT_SUCCESS( status )) return FALSE;

    info->cb                 = sizeof(*info);
    info->CommitTotal        = perf.CommittedPages;
    info->CommitLimit        = perf.CommitLimit;
    info->CommitPeak         = perf.PeakCommitment;
    info->PhysicalTotal      = basic.NumberOfPhysicalPages;
    info->PhysicalAvailable  = perf.AvailablePages;
    info->SystemCache        = 0;
    info->KernelTotal        = perf.PagedPoolPages + perf.NonPagedPoolPages;
    info->KernelPaged        = perf.PagedPoolPages;
    info->KernelNonpaged     = perf.NonPagedPoolPages;
    info->PageSize           = basic.PageSize;

    /* fields from SYSTEM_PROCESS_INFORMATION */
    NtQuerySystemInformation( SystemProcessInformation, NULL, 0, &info_size );
    for (;;)
    {
        process = HeapAlloc( GetProcessHeap(), 0, info_size );
        if (!process)
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        status = NtQuerySystemInformation( SystemProcessInformation, process, info_size, &info_size );
        if (!status) break;
        HeapFree( GetProcessHeap(), 0, process );
        if (status != STATUS_INFO_LENGTH_MISMATCH)
        {
            SetLastError( RtlNtStatusToDosError( status ) );
            return FALSE;
        }
    }
    info->HandleCount = info->ProcessCount = info->ThreadCount = 0;
    spi = process;
    for (;;)
    {
        info->ProcessCount++;
        info->HandleCount += spi->HandleCount;
        info->ThreadCount += spi->NumberOfThreads;
        if (spi->NextEntryOffset == 0) break;
        spi = (SYSTEM_PROCESS_INFORMATION *)((char *)spi + spi->NextEntryOffset);
    }
    HeapFree( GetProcessHeap(), 0, process );
    return TRUE;
}


DWORD
WINAPI
K32GetMappedFileNameW( HANDLE process, void *addr, WCHAR *name, DWORD size )
{
    ULONG_PTR buffer[(sizeof(MEMORY_SECTION_NAME) + MAX_PATH * sizeof(WCHAR)) / sizeof(ULONG_PTR)];
    MEMORY_SECTION_NAME *mem = (MEMORY_SECTION_NAME *)buffer;
    DWORD len;

    if (size && !name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
   NtQueryVirtualMemory( process, addr, 0,
                                             mem, sizeof(buffer), NULL );

    len = mem->SectionFileName.Length / sizeof(WCHAR);
    memcpy( name, mem->SectionFileName.Buffer, min( mem->SectionFileName.Length, size * sizeof(WCHAR) ));
    if (len >= size) SetLastError( ERROR_INSUFFICIENT_BUFFER );
    name[min(len, size - 1)] = 0;
    return len;
}

typedef struct _PEBB32
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    BOOLEAN SpareBool;
    DWORD   Mutant;
    DWORD   ImageBaseAddress;
    DWORD   LdrData;
} PEBB32;
/*
typedef struct _LIST_ENTRY32
{
  DWORD Flink;
  DWORD Blink;
} LIST_ENTRY32;
*/
typedef struct _PEB_LDR_DATA32
{
    ULONG        Length;
    BOOLEAN      Initialized;
    DWORD        SsHandle;
    LIST_ENTRY32 InLoadOrderModuleList;
} PEB_LDR_DATA32;
/*
typedef *struct _UNICODE_STRING32
{
  USHORT Length;
  USHORT MaximumLength;
  DWORD  Buffer;
} UNICODE_STRING32;
*/

typedef struct _LDR_MODULE
{
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
    void*               BaseAddress;
    void*               EntryPoint;
    ULONG               SizeOfImage;
    UNICODE_STRING      FullDllName;
    UNICODE_STRING      BaseDllName;
    ULONG               Flags;
    SHORT               LoadCount;
    SHORT               TlsIndex;
    HANDLE              SectionHandle;
    ULONG               CheckSum;
    ULONG               TimeDateStamp;
    HANDLE              ActivationContext;
} LDR_MODULE, *PLDR_MODULE;


typedef struct _LDR_MODULE32
{
    LIST_ENTRY32        InLoadOrderModuleList;
    LIST_ENTRY32        InMemoryOrderModuleList;
    LIST_ENTRY32        InInitializationOrderModuleList;
    DWORD               BaseAddress;
    DWORD               EntryPoint;
    ULONG               SizeOfImage;
    UNICODE_STRING32    FullDllName;
    UNICODE_STRING32    BaseDllName;
} LDR_MODULE32;

typedef struct {
    HANDLE process;
    PLIST_ENTRY head, current;
    LDR_MODULE ldr_module;
    BOOL wow64;
    LDR_MODULE32 ldr_module32;
} MODULE_ITERATOR;



/***********************************************************************
 *           K32EnumDeviceDrivers (KERNEL32.@)
 */
BOOL WINAPI K32EnumDeviceDrivers(void **image_base, DWORD cb, DWORD *needed)
{
    //FIXME("(%p, %d, %p): stub\n", image_base, cb, needed);

    if (needed)
        *needed = 0;

    return TRUE;
}


static BOOL init_module_iterator(MODULE_ITERATOR *iter, HANDLE process)
{
    PROCESS_BASIC_INFORMATION pbi;
    PPEB_LDR_DATA ldr_data;
    NTSTATUS status;

    if (!IsWow64Process(process, &iter->wow64))
        return FALSE;

    /* Get address of PEB */
    status = NtQueryInformationProcess(process, ProcessBasicInformation,
                                       &pbi, sizeof(pbi), NULL);
    if (status != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    if (sizeof(void *) == 8 && iter->wow64)
    {
        PEB_LDR_DATA32 *ldr_data32_ptr;
        DWORD ldr_data32, first_module;
        PEBB32 *peb32;

        peb32 = (PEBB32 *)(DWORD_PTR)pbi.PebBaseAddress;

        if (!ReadProcessMemory(process, &peb32->LdrData, &ldr_data32,
                               sizeof(ldr_data32), NULL))
            return FALSE;
        ldr_data32_ptr = (PEB_LDR_DATA32 *)(DWORD_PTR) ldr_data32;

        if (!ReadProcessMemory(process,
                               &ldr_data32_ptr->InLoadOrderModuleList.Flink,
                               &first_module, sizeof(first_module), NULL))
            return FALSE;
        iter->head = (LIST_ENTRY *)&ldr_data32_ptr->InLoadOrderModuleList;
        iter->current = (LIST_ENTRY *)(DWORD_PTR) first_module;
        iter->process = process;

        return TRUE;
    }

    /* Read address of LdrData from PEB */
    if (!ReadProcessMemory(process, &pbi.PebBaseAddress->Ldr,
                           &ldr_data, sizeof(ldr_data), NULL))
        return FALSE;

    /* Read address of first module from LdrData */
    if (!ReadProcessMemory(process,
                           &ldr_data->InLoadOrderModuleList.Flink,
                           &iter->current, sizeof(iter->current), NULL))
        return FALSE;

    iter->head = &ldr_data->InLoadOrderModuleList;
    iter->process = process;

    return TRUE;
}

static int module_iterator_next(MODULE_ITERATOR *iter)
{
    if (iter->current == iter->head)
        return 0;

    if (sizeof(void *) == 8 && iter->wow64)
    {
        LIST_ENTRY32 *entry32 = (LIST_ENTRY32 *)iter->current;

        if (!ReadProcessMemory(iter->process,
                               CONTAINING_RECORD(entry32, LDR_MODULE32, InLoadOrderModuleList),
                               &iter->ldr_module32, sizeof(iter->ldr_module32), NULL))
            return -1;

        iter->current = (LIST_ENTRY *)(DWORD_PTR) iter->ldr_module32.InLoadOrderModuleList.Flink;
        return 1;
    }

    if (!ReadProcessMemory(iter->process,
                           CONTAINING_RECORD(iter->current, LDR_MODULE, InLoadOrderModuleList),
                           &iter->ldr_module, sizeof(iter->ldr_module), NULL))
         return -1;

    iter->current = iter->ldr_module.InLoadOrderModuleList.Flink;
    return 1;
}

static BOOL get_ldr_module(HANDLE process, HMODULE module, LDR_MODULE *ldr_module)
{
    MODULE_ITERATOR iter;
    INT ret;

    if (!init_module_iterator(&iter, process))
        return FALSE;

    while ((ret = module_iterator_next(&iter)) > 0)
        /* When hModule is NULL we return the process image - which will be
         * the first module since our iterator uses InLoadOrderModuleList */
        if (!module || module == iter.ldr_module.BaseAddress)
        {
            *ldr_module = iter.ldr_module;
            return TRUE;
        }

    if (ret == 0)
        SetLastError(ERROR_INVALID_HANDLE);

    return FALSE;
}

static BOOL get_ldr_module32(HANDLE process, HMODULE module, LDR_MODULE32 *ldr_module)
{
    MODULE_ITERATOR iter;
    INT ret;

    if (!init_module_iterator(&iter, process))
        return FALSE;

    while ((ret = module_iterator_next(&iter)) > 0)
        /* When hModule is NULL we return the process image - which will be
         * the first module since our iterator uses InLoadOrderModuleList */
        if (!module || (DWORD)(DWORD_PTR) module == iter.ldr_module32.BaseAddress)
        {
            *ldr_module = iter.ldr_module32;
            return TRUE;
        }

    if (ret == 0)
        SetLastError(ERROR_INVALID_HANDLE);

    return FALSE;
}

/***********************************************************************
 *           K32EnumProcessModules (KERNEL32.@)
 *
 * NOTES
 *  Returned list is in load order.
 */
BOOL WINAPI K32EnumProcessModules(HANDLE process, HMODULE *lphModule,
                                  DWORD cb, DWORD *needed)
{
    MODULE_ITERATOR iter;
    DWORD size = 0;
    INT ret;

    if (!init_module_iterator(&iter, process))
        return FALSE;

    if (cb && !lphModule)
    {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }

    while ((ret = module_iterator_next(&iter)) > 0)
    {
        if (cb >= sizeof(HMODULE))
        {
            if (sizeof(void *) == 8 && iter.wow64)
                *lphModule++ = (HMODULE) (DWORD_PTR)iter.ldr_module32.BaseAddress;
            else
                *lphModule++ = iter.ldr_module.BaseAddress;
            cb -= sizeof(HMODULE);
        }
        size += sizeof(HMODULE);
    }

    if (!needed)
    {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }
    *needed = size;

    return ret == 0;
}

/***********************************************************************
 *           K32EnumProcessModulesEx (KERNEL32.@)
 *
 * NOTES
 *  Returned list is in load order.
 */
BOOL WINAPI K32EnumProcessModulesEx(HANDLE process, HMODULE *lphModule,
                                    DWORD cb, DWORD *needed, DWORD filter)
{
    //FIXME("(%p, %p, %d, %p, %d) semi-stub\n",
      //    process, lphModule, cb, needed, filter);
    return K32EnumProcessModules(process, lphModule, cb, needed);
}


/***********************************************************************
 *          K32GetDeviceDriverBaseNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetDeviceDriverBaseNameA(void *image_base, LPSTR base_name, DWORD size)
{
    //FIXME("(%p, %p, %d): stub\n", image_base, base_name, size);

    if (base_name && size)
        base_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           K32GetDeviceDriverBaseNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetDeviceDriverBaseNameW(void *image_base, LPWSTR base_name, DWORD size)
{
    //FIXME("(%p, %p, %d): stub\n", image_base, base_name, size);

    if (base_name && size)
        base_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           K32GetDeviceDriverFileNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetDeviceDriverFileNameA(void *image_base, LPSTR file_name, DWORD size)
{
    //FIXME("(%p, %p, %d): stub\n", image_base, file_name, size);

    if (file_name && size)
        file_name[0] = '\0';

    return 0;
}

/***********************************************************************
 *           K32GetDeviceDriverFileNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetDeviceDriverFileNameW(void *image_base, LPWSTR file_name, DWORD size)
{
    //FIXME("(%p, %p, %d): stub\n", image_base, file_name, size);

    if (file_name && size)
        file_name[0] = '\0';

    return 0;
}


/***********************************************************************
 *           K32GetModuleBaseNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetModuleBaseNameW(HANDLE process, HMODULE module,
                                   LPWSTR base_name, DWORD size)
{
    LDR_MODULE ldr_module;
    BOOL wow64;

    if (!IsWow64Process(process, &wow64))
        return 0;

    if (sizeof(void *) == 8 && wow64)
    {
        LDR_MODULE32 ldr_module32;

        if (!get_ldr_module32(process, module, &ldr_module32))
            return 0;

        size = min(ldr_module32.BaseDllName.Length / sizeof(WCHAR), size);
        if (!ReadProcessMemory(process, (void *)(DWORD_PTR)ldr_module32.BaseDllName.Buffer,
                               base_name, size * sizeof(WCHAR), NULL))
            return 0;
    }
    else
    {
        if (!get_ldr_module(process, module, &ldr_module))
            return 0;

        size = min(ldr_module.BaseDllName.Length / sizeof(WCHAR), size);
        if (!ReadProcessMemory(process, ldr_module.BaseDllName.Buffer,
                               base_name, size * sizeof(WCHAR), NULL))
            return 0;
    }

    base_name[size] = 0;
    return size;
}


/***********************************************************************
 *           K32GetModuleBaseNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetModuleBaseNameA(HANDLE process, HMODULE module,
                                   LPSTR base_name, DWORD size)
{
    WCHAR *base_name_w;
    DWORD len, ret = 0;

    if(!base_name || !size) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    base_name_w = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * size);
    if(!base_name_w)
        return 0;

    len = K32GetModuleBaseNameW(process, module, base_name_w, size);
    //TRACE("%d, %s\n", len, debugstr_w(base_name_w));
    if (len)
    {
        ret = WideCharToMultiByte(CP_ACP, 0, base_name_w, len,
                                  base_name, size, NULL, NULL);
        if (ret < size) base_name[ret] = 0;
    }
    HeapFree(GetProcessHeap(), 0, base_name_w);
    return ret;
}

/***********************************************************************
 *           K32GetModuleFileNameExW (KERNEL32.@)
 */
DWORD WINAPI K32GetModuleFileNameExW(HANDLE process, HMODULE module,
                                     LPWSTR file_name, DWORD size)
{
    LDR_MODULE ldr_module;
    BOOL wow64;
    DWORD len;

    if (!size) return 0;

    if (!IsWow64Process(process, &wow64))
        return 0;

    if (sizeof(void *) == 8 && wow64)
    {
        LDR_MODULE32 ldr_module32;

        if (!get_ldr_module32(process, module, &ldr_module32))
            return 0;

        len = ldr_module32.FullDllName.Length / sizeof(WCHAR);
        if (!ReadProcessMemory(process, (void *)(DWORD_PTR)ldr_module32.FullDllName.Buffer,
                               file_name, min( len, size ) * sizeof(WCHAR), NULL))
            return 0;
    }
    else
    {
        if (!get_ldr_module(process, module, &ldr_module))
            return 0;

        len = ldr_module.FullDllName.Length / sizeof(WCHAR);
        if (!ReadProcessMemory(process, ldr_module.FullDllName.Buffer,
                               file_name, min( len, size ) * sizeof(WCHAR), NULL))
            return 0;
    }

    if (len < size)
    {
        file_name[len] = 0;
        return len;
    }
    else
    {
        file_name[size - 1] = 0;
        return size;
    }
}

/***********************************************************************
 *           K32GetModuleFileNameExA (KERNEL32.@)
 */
DWORD WINAPI K32GetModuleFileNameExA(HANDLE process, HMODULE module,
                                     LPSTR file_name, DWORD size)
{
    WCHAR *ptr;
    DWORD len;

    //TRACE("(hProcess=%p, hModule=%p, %p, %d)\n", process, module, file_name, size);

    if (!file_name || !size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if ( process == GetCurrentProcess() )
    {
        len = GetModuleFileNameA( module, file_name, size );
        if (size) file_name[size - 1] = '\0';
        return len;
    }

    if (!(ptr = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR)))) return 0;

    len = K32GetModuleFileNameExW(process, module, ptr, size);
    if (!len)
    {
        file_name[0] = '\0';
    }
    else
    {
        if (!WideCharToMultiByte( CP_ACP, 0, ptr, -1, file_name, size, NULL, NULL ))
        {
            file_name[size - 1] = 0;
            len = size;
        }
        else if (len < size) len = strlen( file_name );
    }

    HeapFree(GetProcessHeap(), 0, ptr);
    return len;
}

typedef struct _MODULEINFO {
  LPVOID lpBaseOfDll;
  DWORD SizeOfImage;
  LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

/***********************************************************************
 *           K32GetModuleInformation (KERNEL32.@)
 */
BOOL WINAPI K32GetModuleInformation(HANDLE process, HMODULE module,
                                    MODULEINFO *modinfo, DWORD cb)
{
    LDR_MODULE ldr_module;
    BOOL wow64;

    if (cb < sizeof(MODULEINFO))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (!IsWow64Process(process, &wow64))
        return FALSE;

    if (sizeof(void *) == 8 && wow64)
    {
        LDR_MODULE32 ldr_module32;

        if (!get_ldr_module32(process, module, &ldr_module32))
            return FALSE;

        modinfo->lpBaseOfDll = (void *)(DWORD_PTR)ldr_module32.BaseAddress;
        modinfo->SizeOfImage = ldr_module32.SizeOfImage;
        modinfo->EntryPoint  = (void *)(DWORD_PTR)ldr_module32.EntryPoint;
    }
    else
    {
        if (!get_ldr_module(process, module, &ldr_module))
            return FALSE;

        modinfo->lpBaseOfDll = ldr_module.BaseAddress;
        modinfo->SizeOfImage = ldr_module.SizeOfImage;
        modinfo->EntryPoint  = ldr_module.EntryPoint;
    }
    return TRUE;
}



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

BOOL WINAPI InitializeProcThreadAttributeList(struct _PROC_THREAD_ATTRIBUTE_LIST *list,
                                              DWORD count, DWORD flags, SIZE_T *size)
{
    SIZE_T needed;
    BOOL ret = FALSE;
    needed = FIELD_OFFSET(struct _PROC_THREAD_ATTRIBUTE_LIST, attrs[count]);
    if (list && *size >= needed)
    {
        list->mask = 0;
        list->size = count;
        list->count = 0;
        list->unk = 0;
        ret = TRUE;
    }
    else
        SetLastError(ERROR_INSUFFICIENT_BUFFER);

    *size = needed;
    return ret;
}