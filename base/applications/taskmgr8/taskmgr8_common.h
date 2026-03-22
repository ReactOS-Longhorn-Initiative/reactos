/*
 * PROJECT:     ReactOS — Task Manager 8 (Win8/10-style shell)
 * LICENSE:     GPL-2.0-or-later OR LGPL-2.1-or-later
 *
 * Shared types, constants, and includes for all taskmgr8 translation units.
 *
 * Layout:
 *   taskmgr8.c           — Main window, tabs, list views, performance graphs, layout.
 *   taskmgr8_helpers.c   — Resource strings, locale formatting, registry, CPU topology/cache.
 *   taskmgr8_cpu_stats.c — Performance / CPU bottom statistics strip (custom control + paint).
 *   taskmgr8_mem_stats.c — Performance / Memory bottom statistics strip (custom control + paint).
 *   taskmgr8_listutil.c  — Shared ListView column reset helpers.
 *   taskmgr8_details.c   — Details tab (extended process list).
 *   taskmgr8_services.c  — Services tab (SCM enumeration).
 */

#pragma once

#include <windows.h>
#include <winnls.h>

#define TASKMGR8_NEED_LOGICAL_PROCESSOR_INFORMATION 1
#include "reactos_missing.h"

#include <windowsx.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <stdlib.h>

#include "resource.h"

#ifndef DWORD_MAX
#define DWORD_MAX ((DWORD)-1)
#endif

#ifndef LVM_SETTOPINDEX
#define LVM_SETTOPINDEX (LVM_FIRST + 51)
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
#define ROS_HAVE_CPUID 1
#else
#define ROS_HAVE_CPUID 0
#endif

#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION (0x1000)
#endif

#if defined(_WIN64)
typedef ULONGLONG(WINAPI *PFN_GetTickCount64)(void);
#endif

/* --- Navigation / pages -------------------------------------------------- */

#define NAV_WIDTH       186
#define NAV_SPARK_SAMP  16
#define NAV_SPARK_BOX   38
#define NAV_SPARK_GAP   8
#define NAV_TILE_PAD_X  8
#define NAV_TILE_TOP    6
#define NAV_TILE_VGAP   8
#define NAV_TILE_CPU_H  (6 + NAV_SPARK_BOX + 6)
#define NAV_TILE_MEM_H  (6 + NAV_SPARK_BOX + 6)
#define NAV_ITEM_H_CPU  (NAV_TILE_TOP + NAV_TILE_CPU_H + NAV_TILE_VGAP)
#define NAV_ITEM_H_MEM  (NAV_TILE_MEM_H + NAV_TILE_VGAP)
#define NAV_TILE_BORDER RGB(218, 218, 218)

#define PAGE_PROCESSES  0
#define PAGE_CPU        1
#define PAGE_MEMORY     2
#define PAGE_NETWORK    3
#define PAGE_DETAILS    4
#define PAGE_SERVICES   5
#define PAGE_STUB       6

#define TM8_MAX_NET_ADAPTERS 24

#define TAB_MAIN_PROCESSES 0
#define TAB_MAIN_PERF      1
#define TAB_MAIN_APPHIST   2
#define TAB_MAIN_STARTUP   3
#define TAB_MAIN_USERS     4
#define TAB_MAIN_DETAILS   5
#define TAB_MAIN_SERVICES  6

#define MAIN_TAB_FALLBACK_H 24
#define MAIN_TAB_MIN_H      20
#define MAIN_TAB_ROW_MAX_H  27

#define TIMER_ID        1
#define TIMER_MS        500
#define CPU_HIST_LEN    120
#define MAX_CPU_TRACK   4096
#define TM8_MAX_PROC_ROWS MAX_CPU_TRACK
#define TM8_MAX_LOGICAL_CPU 64

/* --- Kernel / NT query shapes (must match ntos layout) ------------------- */

typedef struct _TM8_PROC_PERF_INFO
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} TM8_PROC_PERF_INFO;

typedef char tm8_spi_size_must_match_kernel[sizeof(TM8_PROC_PERF_INFO) == 48 ? 1 : -1];

#ifndef TM8_NT_SUCCESS
#define TM8_NT_SUCCESS(Status) (((LONG)(Status)) >= 0)
#endif

typedef LONG(NTAPI *PFN_NtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength);

typedef struct _TM8_PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} TM8_PROCESSOR_POWER_INFORMATION;

typedef LONG(WINAPI *PFN_CallNtPowerInformation)(
    INT InformationLevel,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength);

typedef BOOL(WINAPI *PFN_GetPhysicallyInstalledSystemMemory)(PULONGLONG TotalMemoryInKilobytes);
typedef UINT(WINAPI *PFN_GetSystemFirmwareTable)(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID,
                                                 PVOID pFirmwareTableBuffer, DWORD BufferSize);

typedef struct _TM8_SYSTEM_BASIC_INFORMATION
{
    ULONG Reserved;
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPageNumber;
    ULONG HighestPhysicalPageNumber;
    ULONG AllocationGranularity;
    ULONG_PTR MinimumUserModeAddress;
    ULONG_PTR MaximumUserModeAddress;
    ULONG_PTR ActiveProcessorsAffinityMask;
    CCHAR NumberOfProcessors;
} TM8_SYSTEM_BASIC_INFORMATION;

enum
{
    TM8_SystemBasicInformation = 0,
    TM8_SystemProcessorPerformanceInformation = 8,
    TM8_ProcessorInformationLevel = 11,
    TM8_SystemMemoryListInformation = 80
};

typedef struct _TM8_SYSTEM_MEMORY_LIST_INFORMATION
{
    SIZE_T ZeroPageCount;
    SIZE_T FreePageCount;
    SIZE_T ModifiedPageCount;
    SIZE_T ModifiedNoWritePageCount;
    SIZE_T BadPageCount;
    SIZE_T PageCountByPriority[8];
    SIZE_T RepurposedPagesByPriority[8];
    SIZE_T ModifiedPageCountPageFile;
} TM8_SYSTEM_MEMORY_LIST_INFORMATION;

#define TM8_SystemCompressionInformation 147

/*
 * On Windows 10+, Task Manager’s “Speed” aligns with PDH counters, not raw
 * CallNtPowerInformation CurrentMhz alone. Optional pdh.dll is loaded at runtime
 * in taskmgr8.c; helpers here only declare the function-pointer shapes.
 */
#ifndef PDH_FMT_DOUBLE
#define PDH_FMT_DOUBLE 0x00000200
#endif

typedef struct _TM8_PDH_FMT_COUNTERVALUE
{
    DWORD CStatus;
    union
    {
        LONG LongValue;
        double DoubleValue;
    } u;
} TM8_PDH_FMT_COUNTERVALUE;

typedef LONG(WINAPI *PFN_PdhOpenQueryW)(LPCWSTR szDataSource, DWORD_PTR dwUserData, void **phQuery);
typedef LONG(WINAPI *PFN_PdhAddEnglishCounterW)(void *hQuery, LPCWSTR szFullCounterPath,
                                                DWORD_PTR dwUserData, void **phCounter);
typedef LONG(WINAPI *PFN_PdhCollectQueryData)(void *hQuery);
typedef LONG(WINAPI *PFN_PdhGetFormattedCounterValue)(void *hCounter, DWORD dwFormat, LPDWORD lpdwType,
                                                      TM8_PDH_FMT_COUNTERVALUE *pValue);
typedef LONG(WINAPI *PFN_PdhCloseQuery)(void *hQuery);

/* --- UI colors (performance pane) ---------------------------------------- */

#define COL_NAV_BG      RGB(243, 243, 243)
#define COL_NAV_SEL     RGB(0, 120, 215)
#define COL_NAV_TEXT    RGB(32, 32, 32)
#define COL_GRAPH_BG    RGB(252, 252, 252)
#define COL_GRAPH_GRID  RGB(238, 240, 245)
#define COL_GRAPH_FILL  RGB(228, 238, 252)
#define COL_GRAPH_LINE  RGB(88, 152, 218)
#define COL_BAR_GREEN   RGB(77, 181, 89)
#define COL_BAR_TRACK   RGB(237, 237, 237)
#define COL_MEM_GRAPH_FILL  RGB(228, 236, 252)
#define COL_MEM_GRAPH_LINE  RGB(56, 112, 188)
#define COL_MEM_GRAPH_GRID  RGB(244, 246, 250)
#define COL_MEM_GRAPH_EDGE  RGB(120, 170, 220)

/* --- Process list / sorting ---------------------------------------------- */

typedef struct _CPU_TRACK
{
    DWORD Pid;
    ULONGLONG PrevTotal100Ns;
} CPU_TRACK;

typedef struct _TM8_SCRATCH_PROC
{
    DWORD pid;
    double cpuPct;
    SIZE_T ws;
    WCHAR exe[MAX_PATH];
    WCHAR path[MAX_PATH];
    int iconIdx;
} TM8_SCRATCH_PROC;

#define TM8_PROCSORT_COL_NONE (-1)
#define TM8_PROCSORT_COL_CPU   2
#define TM8_PROCSORT_COL_MEM   4

#define TM8_ICON_CACHE_MAX 384

/* --- Memory stats paint snapshot (owner-draw memory panel) ---------------- */

typedef struct _TM8_MEM_STATS_PAINT
{
    WCHAR c1Lbl1[56], c1Val1[112];
    WCHAR c1Lbl2[56], c1Val2[112];
    WCHAR c1Lbl3[56], c1Val3[96];
    WCHAR c2Lbl1[56], c2Val1[96];
    WCHAR c2Lbl2[56], c2Val2[96];
    WCHAR c2Lbl3[56], c2Val3[96];
    WCHAR c3Lbl1[56], c3Val1[64];
    WCHAR c3Lbl2[56], c3Val2[64];
    WCHAR c3Lbl3[56], c3Val3[64];
    WCHAR c3Lbl4[56], c3Val4[64];
} TM8_MEM_STATS_PAINT;

typedef struct _TM8_MEM_DIMM_INFO
{
    UINT speedMt;
    UINT slotsPopulated;
    UINT slotsTotal;
    WCHAR formFactor[24];
    BOOL haveSpeed;
    BOOL haveSlots;
    BOOL haveForm;
    DWORD tickCached;
} TM8_MEM_DIMM_INFO;
