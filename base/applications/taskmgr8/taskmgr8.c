/*
 * PROJECT:     ReactOS — modern Task Manager (Win8-style shell)
 * LICENSE:     GPL-2.0-or-later OR LGPL-2.1-or-later
 * PURPOSE:     Separate lightweight task manager: left nav, processes, performance + CPU speed
 */

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

/* Vista+ macro; ReactOS PSDK lists GetTopIndex but not SetTopIndex. */
#ifndef LVM_SETTOPINDEX
#define LVM_SETTOPINDEX (LVM_FIRST + 51)
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
#include <intrin.h>
#define ROS_HAVE_CPUID 1
#else
#define ROS_HAVE_CPUID 0
#endif

#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION (0x1000)
#endif

#define NAV_WIDTH       186
#define NAV_SPARK_SAMP  16
/* Square-ish mini graph left of labels (Win10/11 Performance nav) */
#define NAV_SPARK_BOX   38
#define NAV_SPARK_GAP   8
/* Performance nav: tiled cards — inner tile + vertical gap shows column bg */
#define NAV_TILE_PAD_X  8
#define NAV_TILE_TOP    6
#define NAV_TILE_VGAP   8
/* Inner tile height: left spark + padding; text column vertically fits beside */
#define NAV_TILE_CPU_H  (6 + NAV_SPARK_BOX + 6)
#define NAV_TILE_MEM_H  (6 + NAV_SPARK_BOX + 6)
#define NAV_ITEM_H_CPU  (NAV_TILE_TOP + NAV_TILE_CPU_H + NAV_TILE_VGAP)
#define NAV_ITEM_H_MEM  (NAV_TILE_MEM_H + NAV_TILE_VGAP)
#define NAV_TILE_BORDER RGB(218, 218, 218)
#define PAGE_PROCESSES  0
#define PAGE_CPU        1
#define PAGE_MEMORY     2
#define PAGE_STUB       3
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
/* Minimum logical width for CPU stats strip (three columns); narrower viewports scroll horizontally. */
#define TM8_CPU_STATS_MIN_INNER_W 600

/* Must match SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION (class 8). */
typedef struct _TM8_PROC_PERF_INFO
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} TM8_PROC_PERF_INFO;

/* Kernel uses 0x30 bytes per SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION entry. */
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

#if defined(_WIN64)
typedef ULONGLONG (WINAPI *PFN_GetTickCount64)(void);
#endif

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

/* SYSTEM_INFORMATION_CLASS 80 — standby breakdown (ndk extypes.h). */
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

/* Win10+ compressed-RAM info class (undocumented; fails on ReactOS). */
#define TM8_SystemCompressionInformation 147

/*
 * Task Manager’s “Speed” on Win10+ matches PDH, not raw CallNtPowerInformation CurrentMhz:
 * "\\Processor Information(_Total)\\Processor Frequency" stays near nominal MHz while
 * "% Processor Performance" scales above 100% under turbo. Effective MHz ≈ Freq × (Perf/100).
 * (typeperf on a Ryzen box: Frequency≈4300, Performance≈123 → ~5.3 GHz.)
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

/* Win8-ish performance pane */
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

static HINSTANCE s_hInst;
static HWND s_hwndMain;
static HWND s_hwndStatus;
static HWND s_hwndNav;
static HWND s_hwndNavSep;
static HWND s_hwndList;
static HWND s_hwndCpuLbl;
static HWND s_hwndCpuBar;
static HWND s_hwndMemLbl;
static HWND s_hwndMemBar;
static HWND s_hwndSpeed;
static HWND s_hwndGraphCpu;
static HWND s_hwndGraphMem;
static HWND s_hwndMemDetails;
static HWND s_hwndCpuTitle;
static HWND s_hwndCpuModel;
static HWND s_hwndCpuLiveLbl;
static HWND s_hwndCpuLiveVal;
static HWND s_hwndCpuStatSep;
static HWND s_hwndCpuSpecLbl;
static HWND s_hwndCpuSpecVal;
static HWND s_hwndCpuGraphSub;
static HWND s_hwndCpuStatsPanel;
static int s_CpuStatsHScrollPos;
static int s_CpuStatsContentW;
static HWND s_hwndMemTitle;
static HWND s_hwndMemModel;
static HWND s_hwndMemGraphSub;
static HWND s_hwndMemCompSub;
static HWND s_hwndGraphMemComp;
static HWND s_hwndMemStatsPanel;
static HWND s_hwndMainTab;
static HWND s_hwndStub;
static HWND s_hwndEndTask;
static HWND s_hwndFewerDetails;
static HMENU s_hProcMenuRoot;
static HMENU s_hCtxMenu;
static int s_iPage;
static int s_iPerfNavSel;

static FILETIME s_ftIdle0, s_ftKernel0, s_ftUser0;
static BOOL s_bCpuTimesInit;

typedef struct _CPU_TRACK
{
    DWORD Pid;
    ULONGLONG PrevTotal100Ns;
} CPU_TRACK;

static CPU_TRACK s_CpuTrack[MAX_CPU_TRACK];
static int s_CpuTrackCount;

static HIMAGELIST s_hProcSmIl;
#define TM8_ICON_CACHE_MAX 384
static int s_IconCacheCount;
static WCHAR s_IconCachePath[TM8_ICON_CACHE_MAX][MAX_PATH];
static int s_IconCacheIdx[TM8_ICON_CACHE_MAX];
static int s_ProcListRows;
static double s_ProcCpuDbl[TM8_MAX_PROC_ROWS];
static SIZE_T s_ProcMemWs[TM8_MAX_PROC_ROWS];
static SIZE_T s_ProcMemMax;

#define TM8_PROCSORT_COL_NONE (-1)
#define TM8_PROCSORT_COL_CPU   2
#define TM8_PROCSORT_COL_MEM   4
/* 0 = alphabetical by name (default); 1 = descending by metric; 2 = ascending */
static int s_ProcSortCol = TM8_PROCSORT_COL_NONE;
static int s_ProcSortPhase;

/* Pause process-list refresh while dragging the vertical scrollbar; 1s cooldown after. */
static BOOL s_ProcListVScrollDragging;
static DWORD s_ProcListResumeDeadline;

typedef struct _TM8_SCRATCH_PROC
{
    DWORD pid;
    double cpuPct;
    SIZE_T ws;
    WCHAR exe[MAX_PATH];
    WCHAR path[MAX_PATH];
    int iconIdx;
} TM8_SCRATCH_PROC;
static TM8_SCRATCH_PROC s_ProcScratch[TM8_MAX_PROC_ROWS];

static void SyncProcEndTaskUi(void);
static DWORD GetSelectedPid(void);
static void RefreshProcessList(void);
static void RefreshProcessListEx(BOOL force);

static BYTE s_CpuHist[CPU_HIST_LEN];
static int s_CpuHistPos;
static BYTE s_MemHist[CPU_HIST_LEN];
static int s_MemHistPos;
static BOOL s_PerfHistPrimed;
static int s_LastCpuPct;
static int s_LastMemUsagePct;
static ULONGLONG s_MemTotalPhysNav;
static ULONGLONG s_MemUsedPhysNav;
static BYTE s_CpuHistPer[TM8_MAX_LOGICAL_CPU][CPU_HIST_LEN];
static int s_LastCpuPctPer[TM8_MAX_LOGICAL_CPU];
static TM8_PROC_PERF_INFO s_PrevProcPerf[TM8_MAX_LOGICAL_CPU];
static BOOL s_ProcPerfInited;
static DWORD s_NumLogicalCpus;
static BOOL s_CpuGraphPerLogical = TRUE;
static DWORD s_CurrentCpuMhzLive;
static PFN_NtQuerySystemInformation s_pNtQSI;
static PFN_CallNtPowerInformation s_pCallNtPI;
static PFN_GetPhysicallyInstalledSystemMemory s_pfnGetPhysMem;
static PFN_GetSystemFirmwareTable s_pfnGetSystemFirmwareTable;
/* PDH optional (Windows); ReactOS may have no pdh.dll — fall back to power API. */
static HMODULE s_hPdh;
static void *s_pdhQuery;
static void *s_pdhCntCpuFreq;
static void *s_pdhCntCpuPerf;
static void *s_pdhCntCpuUtil;
static void *s_pdhCntProc[TM8_MAX_LOGICAL_CPU];
static BYTE s_pdhMhzState; /* 0=uninit 1=ok 2=failed */
static PFN_PdhOpenQueryW s_pfnPdhOpenQueryW;
static PFN_PdhAddEnglishCounterW s_pfnPdhAddEnglishCounterW;
static PFN_PdhCollectQueryData s_pfnPdhCollectQueryData;
static PFN_PdhGetFormattedCounterValue s_pfnPdhGetFormattedCounterValue;
static PFN_PdhCloseQuery s_pfnPdhCloseQuery;
#if defined(_WIN64)
static PFN_GetTickCount64 s_pfnGetTickCount64;
#endif
static int s_StatusBarCY;

typedef struct _TM8_CPU_STATS_PAINT
{
    WCHAR utilLbl[48], utilVal[32];
    WCHAR speedLbl[48], speedVal[64];
    WCHAR procLbl[48], procVal[40];
    WCHAR threadLbl[48], threadVal[40];
    WCHAR handleLbl[48], handleVal[40];
    WCHAR upLbl[48], upVal[96];
    WCHAR specMidLblBuf[768];
    WCHAR specMidValBuf[768];
    WCHAR specCacheLblBuf[768];
    WCHAR specCacheValBuf[768];
} TM8_CPU_STATS_PAINT;
static TM8_CPU_STATS_PAINT s_CpuStatsPaint;

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
static TM8_MEM_STATS_PAINT s_MemStatsPaint;
static WCHAR s_szMemGraphYMax[72];
static double s_MemCompFrac[3]; /* in use, cached, free */

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
static TM8_MEM_DIMM_INFO s_MemDimm;

static LRESULT CALLBACK GraphWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK CpuStatsPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void Tm8CpuStatsUpdateScrollInfo(HWND hwnd);
static void DrawCpuPerfStatsPanel(HDC hdc, const RECT *rcPanel);
static void DrawMemPerfStatsPanel(HDC hdc, const RECT *rcPanel);
static void DrawMemoryUsageHistoryGraph(HDC mem, const RECT *rc, const BYTE *hist, int histLen,
                                        int histWritePos, COLORREF fillCol, COLORREF lineCol,
                                        int lineWidth);
static void DrawMemCompositionBar(HDC hdc, const RECT *rc);
static void DrawCpuStatsValueColumn(HDC hdc, const RECT *rcBox, HWND hwndVal);
static void ShowCpuGraphModeContextMenu(HWND hwndMain, HWND hwndGraph, LPARAM lParam);
static WNDPROC s_pfnOldGraph;
static WNDPROC s_pfnOldListView;
static void NavDrawMiniSpark(HDC hdc, const RECT *rcBox, const BYTE *hist, int histLen, int writePos,
                             COLORREF fillRgb, COLORREF lineRgb, BOOL fillUnderCurve);
static HBRUSH s_brNavColumn;
static HFONT s_hFontPerf;
static HFONT s_hFontTitle;
static HFONT s_hFontNavBold;
static HFONT s_hFontNavMeta;
static HFONT s_hFontTab;
static HFONT s_hFontCpuLbl;
static HFONT s_hFontCpuVal;
static HFONT s_hFontCpuHero;
static WCHAR s_szCpuModel[260];
static DWORD s_NominalCpuMhz;
static WCHAR s_MainTabText[7][48];

static void
LoadStr(UINT id, WCHAR *buf, size_t cch)
{
    if (LoadStringW(s_hInst, id, buf, (int)cch) <= 0)
        buf[0] = 0;
}

static void
ApplyPerfTypography(void)
{
    LOGFONTW lf;
    HFONT hGui = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    GetObjectW(hGui, sizeof(lf), &lf);
    /* Sizes tuned to sit closer to Win10/11 Task Manager (compact system UI). */
    lf.lfHeight = -15;
    lf.lfWeight = FW_NORMAL;
    StringCchCopyW(lf.lfFaceName, LF_FACESIZE, L"Segoe UI");
    s_hFontPerf = CreateFontIndirectW(&lf);
    if (!s_hFontPerf)
        s_hFontPerf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    lf.lfHeight = -20;
    lf.lfWeight = FW_SEMIBOLD;
    s_hFontTitle = CreateFontIndirectW(&lf);
    if (!s_hFontTitle)
        s_hFontTitle = s_hFontPerf;

    lf.lfHeight = -12;
    lf.lfWeight = FW_SEMIBOLD;
    s_hFontNavBold = CreateFontIndirectW(&lf);
    if (!s_hFontNavBold)
        s_hFontNavBold = s_hFontPerf;

    lf.lfHeight = -9;
    lf.lfWeight = FW_NORMAL;
    s_hFontNavMeta = CreateFontIndirectW(&lf);
    if (!s_hFontNavMeta)
        s_hFontNavMeta = s_hFontPerf;

    lf.lfHeight = -11;
    lf.lfWeight = FW_NORMAL;
    s_hFontTab = CreateFontIndirectW(&lf);
    if (!s_hFontTab)
        s_hFontTab = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    GetObjectW(hGui, sizeof(lf), &lf);
    StringCchCopyW(lf.lfFaceName, LF_FACESIZE, L"Segoe UI");
    lf.lfHeight = -9;
    lf.lfWeight = FW_NORMAL;
    s_hFontCpuLbl = CreateFontIndirectW(&lf);
    if (!s_hFontCpuLbl)
        s_hFontCpuLbl = s_hFontNavMeta;
    lf.lfHeight = -13;
    lf.lfWeight = FW_SEMIBOLD;
    s_hFontCpuVal = CreateFontIndirectW(&lf);
    if (!s_hFontCpuVal)
        s_hFontCpuVal = s_hFontTitle;

    /* Same as stats values (Win TM does not use a separate oversized hero weight). */
    s_hFontCpuHero = s_hFontCpuVal;

    SendMessageW(s_hwndCpuLbl, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    SendMessageW(s_hwndMemLbl, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    SendMessageW(s_hwndSpeed, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    SendMessageW(s_hwndNav, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    if (s_hwndMainTab)
        SendMessageW(s_hwndMainTab, WM_SETFONT, (WPARAM)s_hFontTab, TRUE);
    SendMessageW(s_hwndMemDetails, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    SendMessageW(s_hwndCpuTitle, WM_SETFONT, (WPARAM)s_hFontTitle, FALSE);
    SendMessageW(s_hwndMemTitle, WM_SETFONT, (WPARAM)s_hFontTitle, FALSE);
    SendMessageW(s_hwndMemModel, WM_SETFONT, (WPARAM)s_hFontTitle, FALSE);
    SendMessageW(s_hwndMemGraphSub, WM_SETFONT, (WPARAM)s_hFontNavMeta, FALSE);
    SendMessageW(s_hwndMemCompSub, WM_SETFONT, (WPARAM)s_hFontNavMeta, FALSE);
    SendMessageW(s_hwndCpuModel, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    SendMessageW(s_hwndCpuGraphSub, WM_SETFONT, (WPARAM)s_hFontNavMeta, FALSE);
    SendMessageW(s_hwndCpuLiveLbl, WM_SETFONT, (WPARAM)s_hFontCpuLbl, FALSE);
    SendMessageW(s_hwndCpuLiveVal, WM_SETFONT, (WPARAM)s_hFontCpuVal, FALSE);
    SendMessageW(s_hwndCpuSpecLbl, WM_SETFONT, (WPARAM)s_hFontCpuLbl, FALSE);
    SendMessageW(s_hwndCpuSpecVal, WM_SETFONT, (WPARAM)s_hFontCpuVal, FALSE);
    if (s_hwndCpuStatsPanel)
        SendMessageW(s_hwndCpuStatsPanel, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    if (s_hwndStub)
        SendMessageW(s_hwndStub, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    if (s_hwndEndTask)
        SendMessageW(s_hwndEndTask, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    if (s_hwndFewerDetails)
        SendMessageW(s_hwndFewerDetails, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
}

static CPU_TRACK *
FindCpuTrack(DWORD pid)
{
    int i;
    for (i = 0; i < s_CpuTrackCount; i++)
    {
        if (s_CpuTrack[i].Pid == pid)
            return &s_CpuTrack[i];
    }
    if (s_CpuTrackCount >= MAX_CPU_TRACK)
        return NULL;
    s_CpuTrack[s_CpuTrackCount].Pid = pid;
    s_CpuTrack[s_CpuTrackCount].PrevTotal100Ns = 0;
    return &s_CpuTrack[s_CpuTrackCount++];
}

static BOOL
Tm8QueryProcessImagePath(HANDLE hProc, WCHAR *path, DWORD cchPath)
{
    typedef BOOL(WINAPI *PFN_Qfpn)(HANDLE, DWORD, LPWSTR, PDWORD);
    static PFN_Qfpn pQfpn = (PFN_Qfpn)(ULONG_PTR)-1;
    DWORD n;

    if (pQfpn == (PFN_Qfpn)(ULONG_PTR)-1)
    {
        HMODULE m = GetModuleHandleW(L"kernel32.dll");
        pQfpn = m ? (PFN_Qfpn)(void *)GetProcAddress(m, "QueryFullProcessImageNameW") : NULL;
    }
    if (pQfpn)
    {
        n = cchPath;
        if (pQfpn(hProc, 0, path, &n))
            return TRUE;
    }
    return GetModuleFileNameExW(hProc, NULL, path, cchPath) != 0;
}

static void
Tm8ProcEnsureImageList(void)
{
    if (s_hProcSmIl)
        return;
    s_hProcSmIl = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 48, 48);
    if (s_hProcSmIl)
    {
        HICON hi = LoadIconW(NULL, IDI_APPLICATION);
        if (hi)
        {
            ImageList_AddIcon(s_hProcSmIl, hi);
            DestroyIcon(hi);
        }
    }
}

static int
Tm8IconCacheLookup(const WCHAR *path)
{
    int i;
    for (i = 0; i < s_IconCacheCount; i++)
    {
        if (lstrcmpiW(s_IconCachePath[i], path) == 0)
            return s_IconCacheIdx[i];
    }
    return -1;
}

static void
Tm8IconCacheStore(const WCHAR *path, int idx)
{
    if (s_IconCacheCount >= TM8_ICON_CACHE_MAX)
        return;
    StringCchCopyW(s_IconCachePath[s_IconCacheCount], MAX_PATH, path);
    s_IconCacheIdx[s_IconCacheCount] = idx;
    s_IconCacheCount++;
}

static int
Tm8IconForExePath(const WCHAR *path)
{
    SHFILEINFOW sfi;
    HICON hIcon;
    int idx;

    Tm8ProcEnsureImageList();
    if (!s_hProcSmIl)
        return 0;
    if (!path || !path[0])
        return 0;

    idx = Tm8IconCacheLookup(path);
    if (idx >= 0)
        return idx;

    ZeroMemory(&sfi, sizeof(sfi));
    if (!SHGetFileInfoW(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON))
        return 0;
    hIcon = sfi.hIcon;
    if (!hIcon)
        return 0;
    idx = ImageList_AddIcon(s_hProcSmIl, hIcon);
    DestroyIcon(hIcon);
    if (idx < 0)
        return 0;
    Tm8IconCacheStore(path, idx);
    return idx;
}

static void
Tm8FmtMbComma1(double mbVal, WCHAR *dst, size_t cch)
{
    WCHAR num[64], out[96];

    StringCchPrintfW(num, _countof(num), L"%.1f", mbVal);
    if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, num, NULL, out, (int)_countof(out)) == 0)
        StringCchCopyW(out, _countof(out), num);
    StringCchPrintfW(dst, cch, L"%s MB", out);
}

static COLORREF
Tm8HeatBgCpu(double pct)
{
    double t = pct / 35.0;
    int r1 = 255, g1 = 253, b1 = 245;
    int r2 = 255, g2 = 165, b2 = 70;
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;
    return RGB((BYTE)(r1 + (r2 - r1) * t), (BYTE)(g1 + (g2 - g1) * t), (BYTE)(b1 + (b2 - b1) * t));
}

static COLORREF
Tm8HeatBgMem(SIZE_T ws, SIZE_T mx)
{
    double t = 0.0;
    if (mx > 0 && ws > 0)
        t = (double)ws / (double)mx;
    if (t > 1.0)
        t = 1.0;
    return Tm8HeatBgCpu(t * 40.0);
}

static void
UpdateProcListHeaders(void)
{
    LVCOLUMNW c;
    WCHAR b[96], name[48], st[48], cpu[48], disk[48], mem[48], net[48];

    if (!s_hwndList || s_iPage != PAGE_PROCESSES)
        return;

    LoadStr(IDS_COL_NAME, name, _countof(name));
    LoadStr(IDS_COL_STATUS, st, _countof(st));
    LoadStr(IDS_COL_CPU, cpu, _countof(cpu));
    LoadStr(IDS_COL_DISK, disk, _countof(disk));
    LoadStr(IDS_COL_MEM, mem, _countof(mem));
    LoadStr(IDS_COL_NETWORK, net, _countof(net));

    c.mask = LVCF_TEXT;

    c.pszText = name;
    ListView_SetColumn(s_hwndList, 0, &c);
    c.pszText = st;
    ListView_SetColumn(s_hwndList, 1, &c);

    {
        const WCHAR *preCpu = L"";
        if (s_ProcSortCol == TM8_PROCSORT_COL_CPU && s_ProcSortPhase == 1)
            preCpu = L"\x25BC ";
        else if (s_ProcSortCol == TM8_PROCSORT_COL_CPU && s_ProcSortPhase == 2)
            preCpu = L"\x25B2 ";
        StringCchPrintfW(b, _countof(b), L"%s%u%% %s", preCpu, (UINT)s_LastCpuPct, cpu);
    }
    c.pszText = b;
    ListView_SetColumn(s_hwndList, 2, &c);

    StringCchPrintfW(b, _countof(b), L"0%% %s", disk);
    c.pszText = b;
    ListView_SetColumn(s_hwndList, 3, &c);

    {
        const WCHAR *preMem = L"";
        if (s_ProcSortCol == TM8_PROCSORT_COL_MEM && s_ProcSortPhase == 1)
            preMem = L"\x25BC ";
        else if (s_ProcSortCol == TM8_PROCSORT_COL_MEM && s_ProcSortPhase == 2)
            preMem = L"\x25B2 ";
        StringCchPrintfW(b, _countof(b), L"%s%lu%% %s", preMem, (ULONG)s_LastMemUsagePct, mem);
    }
    c.pszText = b;
    ListView_SetColumn(s_hwndList, 4, &c);

    StringCchPrintfW(b, _countof(b), L"0%% %s", net);
    c.pszText = b;
    ListView_SetColumn(s_hwndList, 5, &c);
}

static DWORD
ReadNominalCpuMhz(void)
{
    HKEY hKey;
    DWORD mhz = 0, cb = sizeof(mhz);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS)
        return 0;
    RegQueryValueExW(hKey, L"~MHz", NULL, NULL, (BYTE *)&mhz, &cb);
    RegCloseKey(hKey);
    return mhz;
}

static void
ReadCpuModelString(void)
{
    HKEY hKey;
    WCHAR buf[260];
    DWORD cb = sizeof(buf);
    DWORD type = 0;

    s_szCpuModel[0] = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS)
        return;
    if (RegQueryValueExW(hKey, L"ProcessorNameString", NULL, &type, (BYTE *)buf, &cb) ==
            ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ) && cb >= sizeof(WCHAR))
    {
        buf[(cb / sizeof(WCHAR)) - 1] = 0;
        StringCchCopyW(s_szCpuModel, _countof(s_szCpuModel), buf);
    }
    RegCloseKey(hKey);
    if (s_szCpuModel[0] && !StrStrIW(s_szCpuModel, L"Processor") && !StrStrIW(s_szCpuModel, L"CPU"))
        StringCchCatW(s_szCpuModel, _countof(s_szCpuModel), L" Processor");
}

static void
FormatSpeedFromMhz(DWORD mhz, WCHAR *out, size_t cch)
{
    /* NBSP keeps "4.30 GHz" on one line in multiline statics (no wrap before unit). */
    if (mhz >= 1000)
        StringCchPrintfW(out, cch, L"%.2f\u00A0GHz", mhz / 1000.0);
    else if (mhz)
        StringCchPrintfW(out, cch, L"%lu\u00A0MHz", mhz);
    else
        out[0] = 0;
}

static void
FormatUptimeString(WCHAR *buf, size_t cch)
{
    /*
     * i386 (and other 32-bit) MSVC links a CRT that often omits __aullrem;
     * ULONGLONG % pulls that symbol. Use DWORD time-of-day math on 32-bit only.
     */
#if defined(_WIN64)
    ULONGLONG sec;

    if (!s_pfnGetTickCount64)
    {
        HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
        if (k32)
            s_pfnGetTickCount64 = (PFN_GetTickCount64)(void *)GetProcAddress(k32, "GetTickCount64");
    }
    if (s_pfnGetTickCount64)
        sec = s_pfnGetTickCount64() / 1000ULL;
    else
        sec = GetTickCount() / 1000U;

    StringCchPrintfW(buf, cch, L"%llu:%02llu:%02llu:%02llu",
                     sec / 86400ULL, (sec / 3600ULL) % 24ULL, (sec / 60ULL) % 60ULL, sec % 60ULL);
#else
    DWORD sec = GetTickCount() / 1000U;
    DWORD s = sec % 60U;
    DWORD m = (sec / 60U) % 60U;
    DWORD h = (sec / 3600U) % 24U;
    DWORD d = sec / 86400U;

    StringCchPrintfW(buf, cch, L"%lu:%02lu:%02lu:%02lu", d, h, m, s);
#endif
}

static DWORD
CountPhysicalCores(void)
{
    DWORD len = 0, i, n;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buf;
    DWORD cores = 0;

    if (!GetLogicalProcessorInformation(NULL, &len) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return 0;
    if (len == 0)
        return 0;
    buf = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    if (!buf)
        return 0;
    if (!GetLogicalProcessorInformation(buf, &len))
    {
        HeapFree(GetProcessHeap(), 0, buf);
        return 0;
    }
    n = len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    for (i = 0; i < n; i++)
    {
        if (buf[i].Relationship == RelationProcessorCore)
            cores++;
    }
    HeapFree(GetProcessHeap(), 0, buf);
    return cores;
}

static BOOL
CpuVirtHardwarePresent(void)
{
#if ROS_HAVE_CPUID
    int info[4];
    int maxLeaf;
    char vendor[13];

    __cpuid(info, 0);
    maxLeaf = info[0];
    CopyMemory(vendor + 0, &info[1], sizeof(int));
    CopyMemory(vendor + 4, &info[3], sizeof(int));
    CopyMemory(vendor + 8, &info[2], sizeof(int));
    vendor[12] = 0;

    __cpuid(info, 1);
    if (memcmp(vendor, "GenuineIntel", 12) == 0)
        return (((unsigned)info[2] >> 5) & 1u) != 0; /* VMX */
    if (memcmp(vendor, "AuthenticAMD", 12) == 0 || memcmp(vendor, "HygonGenuine", 12) == 0)
    {
        if (maxLeaf >= 0x80000000)
        {
            __cpuid(info, 0x80000000);
            if ((unsigned)info[0] >= 0x80000001u)
            {
                __cpuid(info, 0x80000001);
                return (((unsigned)info[2] >> 2) & 1u) != 0; /* SVM */
            }
        }
        return FALSE;
    }
    /* Unknown vendor: accept either flag */
    if (((unsigned)info[2] & (1u << 5)) != 0)
        return TRUE;
    if (maxLeaf >= 0x80000000)
    {
        __cpuid(info, 0x80000000);
        if ((unsigned)info[0] >= 0x80000001u)
        {
            __cpuid(info, 0x80000001);
            if (((unsigned)info[2] & (1u << 2)) != 0)
                return TRUE;
        }
    }
#endif
    return FALSE;
}

static void
FormatCacheKbHuman(DWORD kb, WCHAR *dst, size_t cch)
{
    if (!dst || cch == 0)
        return;
    if (kb == 0)
    {
        dst[0] = 0;
        return;
    }
    if (kb >= 1024)
        StringCchPrintfW(dst, cch, L"%.1f MB", (double)kb / 1024.0);
    else
        StringCchPrintfW(dst, cch, L"%lu KB", (ULONG)kb);
}

static void
AppendCacheKvLine(WCHAR *lblDest, size_t cchLbl, WCHAR *valDest, size_t cchVal, UINT idsLbl, DWORD kb)
{
    WCHAR name[48], val[72];
    if (kb == 0)
        return;
    LoadStr(idsLbl, name, _countof(name));
    StringCchCatW(lblDest, cchLbl, name);
    StringCchCatW(lblDest, cchLbl, L":\r\n");
    FormatCacheKbHuman(kb, val, _countof(val));
    StringCchCatW(valDest, cchVal, val);
    StringCchCatW(valDest, cchVal, L"\r\n");
}

static BOOL
ReadProcessor0CacheValue(PCWSTR valueName, DWORD *outKb)
{
    HKEY hKey;
    BYTE buf[96];
    DWORD cb = sizeof(buf), typ = 0;

    *outKb = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS)
        return FALSE;
    if (RegQueryValueExW(hKey, valueName, NULL, &typ, buf, &cb) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    RegCloseKey(hKey);

    if (typ == REG_DWORD && cb >= sizeof(DWORD))
    {
        *outKb = *(DWORD *)buf;
        return *outKb != 0;
    }
    if ((typ == REG_SZ || typ == REG_EXPAND_SZ) && cb >= sizeof(WCHAR))
    {
        WCHAR *s = (WCHAR *)buf;
        size_t nwc = cb / sizeof(WCHAR);
        if (nwc >= sizeof(buf) / sizeof(WCHAR))
            nwc = sizeof(buf) / sizeof(WCHAR) - 1;
        s[nwc] = 0;
        *outKb = wcstoul(s, NULL, 0);
        return *outKb != 0;
    }
    return FALSE;
}

#if ROS_HAVE_CPUID
static void
AppendMissingCachesFromCpuid4(WCHAR *specLbl, size_t cchLbl, WCHAR *specVal, size_t cchVal, int haveL1,
                              int haveL2, int haveL3)
{
    int i;
    double l1Bytes = 0, l2Bytes = 0, l3Bytes = 0;
    int maxf[4];

    __cpuid(maxf, 0);
    if ((unsigned)maxf[0] < 4)
        return;

    for (i = 0; i < 32; i++)
    {
        int r[4];
        unsigned type, level;
        unsigned ways, parts, ls;
        unsigned sets;
        double bytes;

        __cpuidex(r, 4, i);
        type = (unsigned)r[0] & 0x1Fu;
        if (type == 0)
            break;
        level = ((unsigned)r[0] >> 5) & 7u;
        ways = ((unsigned)r[2] >> 22) + 1u;
        parts = (((unsigned)r[2] >> 12) & 0x3FFu) + 1u;
        ls = ((unsigned)r[2] & 0xFFFu) + 1u;
        sets = (unsigned)r[3] + 1u;
        bytes = (double)ways * (double)parts * (double)ls * (double)sets;
        if (level == 1)
        {
            if (type == 1 || type == 2 || type == 3)
                l1Bytes += bytes;
        }
        else if (level == 2)
        {
            if (bytes > l2Bytes)
                l2Bytes = bytes;
        }
        else if (level == 3)
        {
            if (bytes > l3Bytes)
                l3Bytes = bytes;
        }
    }
    if (!haveL1 && l1Bytes > 0)
        AppendCacheKvLine(specLbl, cchLbl, specVal, cchVal, IDS_LBL_L1, (DWORD)(l1Bytes / 1024.0 + 0.5));
    if (!haveL2 && l2Bytes > 0)
        AppendCacheKvLine(specLbl, cchLbl, specVal, cchVal, IDS_LBL_L2, (DWORD)(l2Bytes / 1024.0 + 0.5));
    if (!haveL3 && l3Bytes > 0)
        AppendCacheKvLine(specLbl, cchLbl, specVal, cchVal, IDS_LBL_L3, (DWORD)(l3Bytes / 1024.0 + 0.5));
}
#endif /* ROS_HAVE_CPUID */

static void
AppendLbl(WCHAR *buf, size_t cch, UINT id)
{
    WCHAR t[64];
    LoadStr(id, t, _countof(t));
    StringCchCatW(buf, cch, t);
    StringCchCatW(buf, cch, L"\r\n");
}

static void
AppendSpecLbl(WCHAR *buf, size_t cch, UINT id)
{
    WCHAR t[72];
    LoadStr(id, t, _countof(t));
    StringCchCatW(buf, cch, t);
    StringCchCatW(buf, cch, L":\r\n");
}

static void
FormatULongGrouped(ULONG n, WCHAR *buf, size_t cch)
{
    WCHAR tmp[24], grp[32];
    NUMBERFMTW fmt;

    StringCchPrintfW(tmp, _countof(tmp), L"%lu", n);
    fmt.NumDigits = 0;
    fmt.LeadingZero = FALSE;
    fmt.Grouping = 3;
    fmt.lpDecimalSep = L".";
    fmt.lpThousandSep = L",";
    fmt.NegativeOrder = 0;
    if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, tmp, &fmt, grp, (int)_countof(grp)))
        StringCchCopyW(buf, cch, grp);
    else
        StringCchCopyW(buf, cch, tmp);
}

static void
RefreshCpuStatsPanel(void)
{
    WCHAR specMidLbl[768], specMidVal[768], cacheLbl[768], cacheVal[768];
    WCHAR spd[48], liveSpd[48], up[48], virtLbl[48], line[180];
    PERFORMANCE_INFORMATION pi;
    SYSTEM_INFO si;
    DWORD cores;
    WCHAR grp[32];
    TM8_CPU_STATS_PAINT *ps = &s_CpuStatsPaint;

    if (!s_hwndCpuStatsPanel)
        return;

    GetSystemInfo(&si);
    FormatSpeedFromMhz(s_NominalCpuMhz, spd, _countof(spd));

    ZeroMemory(ps, sizeof(*ps));
    specMidLbl[0] = specMidVal[0] = 0;
    cacheLbl[0] = cacheVal[0] = 0;

    LoadStr(IDS_LBL_UTIL, ps->utilLbl, _countof(ps->utilLbl));
    StringCchPrintfW(ps->utilVal, _countof(ps->utilVal), L"%lu%%", (ULONG)s_LastCpuPct);

    LoadStr(IDS_LBL_SPEED, ps->speedLbl, _countof(ps->speedLbl));
    {
        DWORD mhzLive = s_CurrentCpuMhzLive ? s_CurrentCpuMhzLive : s_NominalCpuMhz;
        FormatSpeedFromMhz(mhzLive, liveSpd, _countof(liveSpd));
        if (liveSpd[0])
            StringCchCopyW(ps->speedVal, _countof(ps->speedVal), liveSpd);
    }

    ZeroMemory(&pi, sizeof(pi));
    pi.cb = sizeof(pi);
    GetPerformanceInfo(&pi, sizeof(pi));
    LoadStr(IDS_LBL_PROCS, ps->procLbl, _countof(ps->procLbl));
    FormatULongGrouped(pi.ProcessCount, grp, _countof(grp));
    StringCchCopyW(ps->procVal, _countof(ps->procVal), grp);

    LoadStr(IDS_LBL_THREADS, ps->threadLbl, _countof(ps->threadLbl));
    FormatULongGrouped(pi.ThreadCount, grp, _countof(grp));
    StringCchCopyW(ps->threadVal, _countof(ps->threadVal), grp);

    LoadStr(IDS_LBL_HANDLES, ps->handleLbl, _countof(ps->handleLbl));
    FormatULongGrouped(pi.HandleCount, grp, _countof(grp));
    StringCchCopyW(ps->handleVal, _countof(ps->handleVal), grp);

    FormatUptimeString(up, _countof(up));
    LoadStr(IDS_LBL_UPTIME, ps->upLbl, _countof(ps->upLbl));
    StringCchCopyW(ps->upVal, _countof(ps->upVal), up);

    if (spd[0])
    {
        AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_BASE);
        StringCchCatW(specMidVal, _countof(specMidVal), spd);
        StringCchCatW(specMidVal, _countof(specMidVal), L"\r\n");
    }
    AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_SOCKETS);
    StringCchCatW(specMidVal, _countof(specMidVal), L"1\r\n");

    cores = CountPhysicalCores();
    if (cores > 0)
    {
        AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_CORES);
        StringCchPrintfW(line, _countof(line), L"%lu\r\n", cores);
        StringCchCatW(specMidVal, _countof(specMidVal), line);
    }

    AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_LOGICAL);
    StringCchPrintfW(line, _countof(line), L"%lu\r\n", (ULONG)s_NumLogicalCpus);
    StringCchCatW(specMidVal, _countof(specMidVal), line);

#if ROS_HAVE_CPUID
    LoadStr(CpuVirtHardwarePresent() ? IDS_VIRT_ENABLED : IDS_VIRT_DISABLED, virtLbl, _countof(virtLbl));
#else
    LoadStr(IDS_VIRT_UNKNOWN, virtLbl, _countof(virtLbl));
#endif
    AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_VIRT);
    StringCchCatW(specMidVal, _countof(specMidVal), virtLbl);
    StringCchCatW(specMidVal, _countof(specMidVal), L"\r\n");

    {
        DWORD l1dKB = 0, l1iKB = 0, l1KB = 0, l2KB = 0, l3KB = 0;
        int haveL1 = 0, haveL2 = 0, haveL3 = 0;

        ReadProcessor0CacheValue(L"L1DataCacheSize", &l1dKB);
        ReadProcessor0CacheValue(L"L1InstructionCacheSize", &l1iKB);
        l1KB = l1dKB + l1iKB;
        if (l1KB == 0)
            ReadProcessor0CacheValue(L"L1CacheSize", &l1KB);
        if (l1KB != 0)
        {
            AppendCacheKvLine(cacheLbl, _countof(cacheLbl), cacheVal, _countof(cacheVal), IDS_LBL_L1, l1KB);
            haveL1 = 1;
        }
        if (ReadProcessor0CacheValue(L"L2CacheSize", &l2KB))
        {
            AppendCacheKvLine(cacheLbl, _countof(cacheLbl), cacheVal, _countof(cacheVal), IDS_LBL_L2, l2KB);
            haveL2 = 1;
        }
        if (ReadProcessor0CacheValue(L"ThirdLevelCacheSize", &l3KB) ||
            ReadProcessor0CacheValue(L"L3CacheSize", &l3KB))
        {
            AppendCacheKvLine(cacheLbl, _countof(cacheLbl), cacheVal, _countof(cacheVal), IDS_LBL_L3, l3KB);
            haveL3 = 1;
        }
#if ROS_HAVE_CPUID
        if (!haveL1 || !haveL2 || !haveL3)
            AppendMissingCachesFromCpuid4(cacheLbl, _countof(cacheLbl), cacheVal, _countof(cacheVal), haveL1,
                                          haveL2, haveL3);
#endif
    }

    StringCchCopyW(ps->specMidLblBuf, _countof(ps->specMidLblBuf), specMidLbl);
    StringCchCopyW(ps->specMidValBuf, _countof(ps->specMidValBuf), specMidVal);
    StringCchCopyW(ps->specCacheLblBuf, _countof(ps->specCacheLblBuf), cacheLbl);
    StringCchCopyW(ps->specCacheValBuf, _countof(ps->specCacheValBuf), cacheVal);
    InvalidateRect(s_hwndCpuStatsPanel, NULL, TRUE);
}

static int
ComputeTotalCpuPercent(void)
{
    FILETIME idle, kr, us;
    ULARGE_INTEGER li, lk, lu, li0, lk0, lu0;
    ULONGLONG idleD, totalD;

    if (!GetSystemTimes(&idle, &kr, &us))
        return s_LastCpuPct;

    li.LowPart = idle.dwLowDateTime;
    li.HighPart = idle.dwHighDateTime;
    lk.LowPart = kr.dwLowDateTime;
    lk.HighPart = kr.dwHighDateTime;
    lu.LowPart = us.dwLowDateTime;
    lu.HighPart = us.dwHighDateTime;

    if (!s_bCpuTimesInit)
    {
        s_ftIdle0 = idle;
        s_ftKernel0 = kr;
        s_ftUser0 = us;
        s_bCpuTimesInit = TRUE;
        return 0;
    }

    li0.LowPart = s_ftIdle0.dwLowDateTime;
    li0.HighPart = s_ftIdle0.dwHighDateTime;
    lk0.LowPart = s_ftKernel0.dwLowDateTime;
    lk0.HighPart = s_ftKernel0.dwHighDateTime;
    lu0.LowPart = s_ftUser0.dwLowDateTime;
    lu0.HighPart = s_ftUser0.dwHighDateTime;

    idleD = li.QuadPart - li0.QuadPart;
    totalD = (li.QuadPart + lk.QuadPart + lu.QuadPart) -
             (li0.QuadPart + lk0.QuadPart + lu0.QuadPart);

    s_ftIdle0 = idle;
    s_ftKernel0 = kr;
    s_ftUser0 = us;

    if (totalD == 0)
        return s_LastCpuPct;

    /* Use double so MSVC x86 does not pull in __allmul from 64-bit mul */
    s_LastCpuPct = (int)(100.0 - (double)idleD / (double)totalD * 100.0);
    if (s_LastCpuPct < 0)
        s_LastCpuPct = 0;
    if (s_LastCpuPct > 100)
        s_LastCpuPct = 100;
    return s_LastCpuPct;
}

static void
InitPerfApis(void)
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    HMODULE pp = LoadLibraryW(L"powrprof.dll");
    if (ntdll)
        s_pNtQSI = (PFN_NtQuerySystemInformation)(void *)GetProcAddress(ntdll, "NtQuerySystemInformation");
    if (pp)
        s_pCallNtPI = (PFN_CallNtPowerInformation)(void *)GetProcAddress(pp, "CallNtPowerInformation");
}

static void
InitMemoryExtraApis(void)
{
    HMODULE k = GetModuleHandleW(L"kernel32.dll");
    if (k)
    {
        s_pfnGetPhysMem =
            (PFN_GetPhysicallyInstalledSystemMemory)(void *)GetProcAddress(k, "GetPhysicallyInstalledSystemMemory");
        s_pfnGetSystemFirmwareTable =
            (PFN_GetSystemFirmwareTable)(void *)GetProcAddress(k, "GetSystemFirmwareTable");
    }
}

static void
PdhCpuMhzShutdown(void)
{
    if (s_pdhQuery && s_pfnPdhCloseQuery)
    {
        s_pfnPdhCloseQuery(s_pdhQuery);
        s_pdhQuery = NULL;
        s_pdhCntCpuFreq = NULL;
        s_pdhCntCpuPerf = NULL;
        s_pdhCntCpuUtil = NULL;
        FillMemory(s_pdhCntProc, sizeof(s_pdhCntProc), 0);
    }
    if (s_hPdh)
    {
        FreeLibrary(s_hPdh);
        s_hPdh = NULL;
    }
    s_pfnPdhOpenQueryW = NULL;
    s_pfnPdhAddEnglishCounterW = NULL;
    s_pfnPdhCollectQueryData = NULL;
    s_pfnPdhGetFormattedCounterValue = NULL;
    s_pfnPdhCloseQuery = NULL;
    s_pdhMhzState = 0;
}

static BOOL
PdhCpuMhzTryInit(void)
{
    LONG st;

    if (s_pdhMhzState == 2)
        return FALSE;
    if (s_pdhMhzState == 1)
        return TRUE;

    s_hPdh = LoadLibraryW(L"pdh.dll");
    if (!s_hPdh)
        goto fail;

    s_pfnPdhOpenQueryW = (PFN_PdhOpenQueryW)(void *)GetProcAddress(s_hPdh, "PdhOpenQueryW");
    s_pfnPdhAddEnglishCounterW = (PFN_PdhAddEnglishCounterW)(void *)GetProcAddress(s_hPdh, "PdhAddEnglishCounterW");
    s_pfnPdhCollectQueryData = (PFN_PdhCollectQueryData)(void *)GetProcAddress(s_hPdh, "PdhCollectQueryData");
    s_pfnPdhGetFormattedCounterValue =
        (PFN_PdhGetFormattedCounterValue)(void *)GetProcAddress(s_hPdh, "PdhGetFormattedCounterValue");
    s_pfnPdhCloseQuery = (PFN_PdhCloseQuery)(void *)GetProcAddress(s_hPdh, "PdhCloseQuery");
    if (!s_pfnPdhOpenQueryW || !s_pfnPdhAddEnglishCounterW || !s_pfnPdhCollectQueryData ||
        !s_pfnPdhGetFormattedCounterValue || !s_pfnPdhCloseQuery)
        goto fail_free;

    st = s_pfnPdhOpenQueryW(NULL, 0, &s_pdhQuery);
    if (st != 0 || !s_pdhQuery)
        goto fail_free;

    st = s_pfnPdhAddEnglishCounterW(s_pdhQuery,
                                    L"\\Processor Information(_Total)\\Processor Frequency", 0,
                                    &s_pdhCntCpuFreq);
    if (st != 0 || !s_pdhCntCpuFreq)
        goto fail_close;

    st = s_pfnPdhAddEnglishCounterW(s_pdhQuery,
                                    L"\\Processor Information(_Total)\\% Processor Performance", 0,
                                    &s_pdhCntCpuPerf);
    if (st != 0 || !s_pdhCntCpuPerf)
        goto fail_close;

    st = s_pfnPdhAddEnglishCounterW(s_pdhQuery,
                                    L"\\Processor Information(_Total)\\% Processor Utility", 0,
                                    &s_pdhCntCpuUtil);
    if (st != 0 || !s_pdhCntCpuUtil)
        goto fail_close;

    FillMemory(s_pdhCntProc, sizeof(s_pdhCntProc), 0);
    {
        DWORD ip, np = s_NumLogicalCpus;
        WCHAR path[96];

        if (np > TM8_MAX_LOGICAL_CPU)
            np = TM8_MAX_LOGICAL_CPU;
        for (ip = 0; ip < np; ip++)
        {
            StringCchPrintfW(path, _countof(path), L"\\Processor(%lu)\\%% Processor Time",
                             (unsigned long)ip);
            st = s_pfnPdhAddEnglishCounterW(s_pdhQuery, path, 0, &s_pdhCntProc[ip]);
            if (st != 0 || !s_pdhCntProc[ip])
                s_pdhCntProc[ip] = NULL;
        }
    }

    s_pfnPdhCollectQueryData(s_pdhQuery);
    s_pfnPdhCollectQueryData(s_pdhQuery);

    s_pdhMhzState = 1;
    return TRUE;

fail_close:
    FillMemory(s_pdhCntProc, sizeof(s_pdhCntProc), 0);
    s_pfnPdhCloseQuery(s_pdhQuery);
    s_pdhQuery = NULL;
    s_pdhCntCpuFreq = NULL;
    s_pdhCntCpuPerf = NULL;
    s_pdhCntCpuUtil = NULL;
fail_free:
    FreeLibrary(s_hPdh);
    s_hPdh = NULL;
fail:
    s_pfnPdhOpenQueryW = NULL;
    s_pfnPdhAddEnglishCounterW = NULL;
    s_pfnPdhCollectQueryData = NULL;
    s_pfnPdhGetFormattedCounterValue = NULL;
    s_pfnPdhCloseQuery = NULL;
    s_pdhMhzState = 2;
    return FALSE;
}

static void
Tm8PdhApplyCpuUtilityPct(void)
{
    TM8_PDH_FMT_COUNTERVALUE fv;
    DWORD ty;
    int p;

    if (s_pdhMhzState != 1 || !s_pdhCntCpuUtil || !s_pfnPdhGetFormattedCounterValue)
        return;
    if (s_pfnPdhGetFormattedCounterValue(s_pdhCntCpuUtil, PDH_FMT_DOUBLE, &ty, &fv) != 0)
        return;
    if (fv.CStatus != 0)
        return;
    p = (int)(fv.u.DoubleValue + 0.5);
    if (p < 0)
        p = 0;
    if (p > 100)
        p = 100;
    s_LastCpuPct = p;
}

static void
Tm8PdhApplyPerProcessorPct(void)
{
    TM8_PDH_FMT_COUNTERVALUE fv;
    DWORD ty;
    DWORD i, n;
    int p;

    if (s_pdhMhzState != 1 || !s_pfnPdhGetFormattedCounterValue)
        return;
    n = s_NumLogicalCpus;
    if (n > TM8_MAX_LOGICAL_CPU)
        n = TM8_MAX_LOGICAL_CPU;
    for (i = 0; i < n; i++)
    {
        if (!s_pdhCntProc[i])
            continue;
        if (s_pfnPdhGetFormattedCounterValue(s_pdhCntProc[i], PDH_FMT_DOUBLE, &ty, &fv) != 0)
            continue;
        if (fv.CStatus != 0)
            continue;
        p = (int)(fv.u.DoubleValue + 0.5);
        if (p < 0)
            p = 0;
        if (p > 100)
            p = 100;
        s_LastCpuPctPer[i] = p;
    }
}

static void
UpdateCurrentCpuMhzFromPowerInfo(void)
{
    TM8_PROCESSOR_POWER_INFORMATION ppi[TM8_MAX_LOGICAL_CPU];
    ULONG n, i;
    DWORD hiCur = 0, sumMax = 0, nMax = 0;

    s_CurrentCpuMhzLive = 0;
    if (!s_pCallNtPI || s_NumLogicalCpus == 0)
        return;
    n = s_NumLogicalCpus;
    if (n > TM8_MAX_LOGICAL_CPU)
        n = TM8_MAX_LOGICAL_CPU;
    if (s_pCallNtPI(TM8_ProcessorInformationLevel, NULL, 0, ppi, (ULONG)(n * sizeof(ppi[0]))) < 0)
        return;
    for (i = 0; i < n; i++)
    {
        DWORD cur = ppi[i].CurrentMhz;
        if (cur != 0 && cur > hiCur)
            hiCur = cur;
        if (ppi[i].MaxMhz != 0)
        {
            sumMax += ppi[i].MaxMhz;
            nMax++;
        }
    }
    if (hiCur != 0)
        s_CurrentCpuMhzLive = hiCur;
    else if (nMax != 0)
        s_CurrentCpuMhzLive = (sumMax + nMax / 2) / nMax;
}

static void
UpdateCurrentCpuMhzSample(void)
{
    TM8_PDH_FMT_COUNTERVALUE fvFreq, fvPerf;
    DWORD ty;
    double mhzD;

    s_CurrentCpuMhzLive = 0;

    if (PdhCpuMhzTryInit())
    {
        if (s_pfnPdhCollectQueryData(s_pdhQuery) == 0)
        {
            Tm8PdhApplyCpuUtilityPct();
            Tm8PdhApplyPerProcessorPct();

            if (s_pfnPdhGetFormattedCounterValue(s_pdhCntCpuFreq, PDH_FMT_DOUBLE, &ty, &fvFreq) == 0 &&
                s_pfnPdhGetFormattedCounterValue(s_pdhCntCpuPerf, PDH_FMT_DOUBLE, &ty, &fvPerf) == 0 &&
                fvFreq.CStatus == 0 && fvPerf.CStatus == 0)
            {
                mhzD = fvFreq.u.DoubleValue * (fvPerf.u.DoubleValue / 100.0);
                if (mhzD >= 1.0 && mhzD < 200000.0)
                {
                    s_CurrentCpuMhzLive = (DWORD)(mhzD + 0.5);
                    return;
                }
            }
        }
    }

    UpdateCurrentCpuMhzFromPowerInfo();
}

static DWORD
Tm8CountLogicalCpusFromLpi(void)
{
    DWORD bytes = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION info;
    DWORD i, nent;
    DWORD logical = 0;

    if (!GetLogicalProcessorInformation(NULL, &bytes) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return 0;
    info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
    if (!info)
        return 0;
    if (!GetLogicalProcessorInformation(info, &bytes))
    {
        HeapFree(GetProcessHeap(), 0, info);
        return 0;
    }
    nent = bytes / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    for (i = 0; i < nent; i++)
    {
        if (info[i].Relationship == RelationProcessorCore)
        {
            ULONG_PTR mask = info[i].ProcessorMask;
            while (mask)
            {
                logical += (DWORD)(mask & 1);
                mask >>= 1;
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, info);
    return logical;
}

static void
CpuGraphGridDims(int n, int *cols, int *rows)
{
    /* Match Windows 10 Task Manager: 8 columns when many logical processors */
    if (n <= 0)
    {
        *cols = 1;
        *rows = 1;
        return;
    }
    if (n == 1)
    {
        *cols = 1;
        *rows = 1;
        return;
    }
    if (n == 2)
    {
        *cols = 2;
        *rows = 1;
        return;
    }
    if (n <= 4)
    {
        *cols = 2;
        *rows = 2;
        return;
    }
    if (n <= 8)
    {
        *cols = 4;
        *rows = 2;
        return;
    }
    *cols = 8;
    *rows = (n + *cols - 1) / *cols;
}

/*
 * Per-logical CPU and overall CPU % from one SPI snapshot so the main graph
 * matches GetSystemTimes (ReactOS sums the same fields) and the mini CPU graphs.
 */
static DWORD
Tm8GetLogicalCpuCount(void)
{
    typedef DWORD(WINAPI *PFN_GetActiveProcessorCount)(WORD Group);
    PFN_GetActiveProcessorCount pfn;
    HMODULE k32;
    TM8_SYSTEM_BASIC_INFORMATION sbi;
    ULONG rl;
    LONG st;
    DWORD n;

    /*
     * Prefer APIs that report logical processors (SMT). SystemBasicInformation.NumberOfProcessors
     * is a CCHAR and can match physical cores on some hosts (e.g. 16 vs 32 threads).
     */
    k32 = GetModuleHandleW(L"kernel32.dll");
    if (k32)
    {
        pfn = (PFN_GetActiveProcessorCount)(void *)GetProcAddress(k32, "GetActiveProcessorCount");
        if (pfn)
        {
            n = pfn((WORD)0xffff); /* ALL_PROCESSOR_GROUPS */
            if (n >= 1 && n <= TM8_MAX_LOGICAL_CPU)
                return n;
        }
    }

    n = Tm8CountLogicalCpusFromLpi();
    if (n >= 1 && n <= TM8_MAX_LOGICAL_CPU)
        return n;

    if (s_pNtQSI)
    {
        st = s_pNtQSI(TM8_SystemBasicInformation, &sbi, sizeof(sbi), &rl);
        if (TM8_NT_SUCCESS(st) && rl >= sizeof(sbi))
        {
            DWORD n2 = (DWORD)(unsigned char)sbi.NumberOfProcessors;
            if (n2 >= 1 && n2 <= TM8_MAX_LOGICAL_CPU)
                return n2;
        }
    }

    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        if (si.dwNumberOfProcessors >= 1 && si.dwNumberOfProcessors <= TM8_MAX_LOGICAL_CPU)
            return si.dwNumberOfProcessors;
    }
    return 1;
}

static void
SampleCpuMetrics(void)
{
    TM8_PROC_PERF_INFO cur[TM8_MAX_LOGICAL_CPU];
    ULONG n, i, rl, need;
    LONG st;

    n = s_NumLogicalCpus;
    if (n > TM8_MAX_LOGICAL_CPU)
        n = TM8_MAX_LOGICAL_CPU;

    if (!s_pNtQSI || n == 0)
    {
        for (i = 0; i < TM8_MAX_LOGICAL_CPU; i++)
            s_LastCpuPctPer[i] = s_LastCpuPct;
        return;
    }

    need = (ULONG)(n * sizeof(cur[0]));
    st = s_pNtQSI(TM8_SystemProcessorPerformanceInformation, cur, need, &rl);
    /*
     * Same rule as kernel32 GetSystemTimes: require exact ReturnLength or the
     * buffer layout may not match what the kernel wrote (wrong stride → every
     * “CPU” looks like the same sample, mirrored graphs).
     */
    if (!TM8_NT_SUCCESS(st) || rl != need)
    {
        for (i = 0; i < n; i++)
            s_LastCpuPctPer[i] = s_LastCpuPct;
        return;
    }

    if (!s_ProcPerfInited)
    {
        CopyMemory(s_PrevProcPerf, cur, n * sizeof(cur[0]));
        s_ProcPerfInited = TRUE;
        for (i = 0; i < n; i++)
            s_LastCpuPctPer[i] = s_LastCpuPct;
        return;
    }

    /*
     * Per-CPU time regression: handle each logical CPU alone. A global “any CPU went
     * backwards → bail” left every s_LastCpuPctPer[] stuck at the same headline % (mirrored
     * graphs). Dpc/Interrupt are not reliably monotonic — do not use them here.
     */
    for (i = 0; i < n; i++)
    {
        ULONGLONG idleD, krD, usD, totalD;
        int pct;

        if (cur[i].IdleTime.QuadPart < s_PrevProcPerf[i].IdleTime.QuadPart ||
            cur[i].KernelTime.QuadPart < s_PrevProcPerf[i].KernelTime.QuadPart ||
            cur[i].UserTime.QuadPart < s_PrevProcPerf[i].UserTime.QuadPart)
        {
            s_PrevProcPerf[i] = cur[i];
            continue;
        }

        idleD = cur[i].IdleTime.QuadPart - s_PrevProcPerf[i].IdleTime.QuadPart;
        krD = cur[i].KernelTime.QuadPart - s_PrevProcPerf[i].KernelTime.QuadPart;
        usD = cur[i].UserTime.QuadPart - s_PrevProcPerf[i].UserTime.QuadPart;
        totalD = idleD + krD + usD;
        if (totalD == 0)
            pct = s_LastCpuPctPer[i];
        else
        {
            pct = (int)(100.0 * (double)(krD + usD) / (double)totalD);
            if (pct < 0)
                pct = 0;
            if (pct > 100)
                pct = 100;
        }
        s_LastCpuPctPer[i] = pct;
        s_PrevProcPerf[i] = cur[i];
    }
}

static void
SyncPerfNavVisibility(void)
{
    BOOL perf = (s_hwndMainTab &&
                 TabCtrl_GetCurSel(s_hwndMainTab) == TAB_MAIN_PERF);
    ShowWindow(s_hwndNav, perf ? SW_SHOW : SW_HIDE);
    if (s_hwndNavSep)
        ShowWindow(s_hwndNavSep, perf ? SW_SHOW : SW_HIDE);
}

static void
LayoutChildren(HWND hwnd)
{
    RECT rc;
    RECT rcSt;
    int statusH, navW = NAV_WIDTH, margin = 6;
    int pageLeft, pageW, pageTop, pageH;
    int graphTop, graphH;
    int tabTop, contentTop, tabBarH;
    int tabW;

    GetClientRect(hwnd, &rc);
    GetClientRect(s_hwndStatus, &rcSt);
    if (rcSt.bottom > 0)
        s_StatusBarCY = rcSt.bottom;
    if (s_StatusBarCY <= 0)
        s_StatusBarCY = 22;

    if (s_hwndMainTab && TabCtrl_GetCurSel(s_hwndMainTab) == TAB_MAIN_PERF)
    {
        statusH = 0;
        if (s_hwndStatus)
        {
            ShowWindow(s_hwndStatus, SW_HIDE);
            SetWindowPos(s_hwndStatus, NULL, 0, rc.bottom, rc.right, 0,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    else
    {
        statusH = s_StatusBarCY;
        if (s_hwndStatus)
        {
            ShowWindow(s_hwndStatus, SW_SHOW);
            SetWindowPos(s_hwndStatus, NULL, 0, rc.bottom - statusH, rc.right, statusH,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    if (rc.bottom <= statusH + margin * 2 && statusH != 0)
        return;
    if (rc.bottom <= margin * 2 && statusH == 0)
        return;

    tabTop = margin;
    tabBarH = 0;
    tabW = rc.right - 2 * margin;
    if (tabW < 120)
        tabW = 120;

    if (s_hwndMainTab)
    {
        RECT crCli, adj;
        int hMeas;

        /* Tall temp height so AdjustRect / GetItemRect are meaningful */
        SetWindowPos(s_hwndMainTab, NULL, margin, tabTop, tabW, 48,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        tabBarH = MAIN_TAB_FALLBACK_H;
        GetClientRect(s_hwndMainTab, &crCli);
        if (crCli.right > 8 && TabCtrl_GetItemCount(s_hwndMainTab) > 0)
        {
            SetRect(&adj, 0, 0, crCli.right, 0);
            TabCtrl_AdjustRect(s_hwndMainTab, TRUE, &adj);
            hMeas = adj.bottom - adj.top;
            if (hMeas < MAIN_TAB_MIN_H || hMeas > 44)
            {
                if (TabCtrl_GetItemRect(s_hwndMainTab, 0, &crCli))
                    hMeas = (crCli.bottom - crCli.top) + 3;
                else
                    hMeas = MAIN_TAB_FALLBACK_H;
            }
            tabBarH = hMeas;
            if (tabBarH < MAIN_TAB_MIN_H)
                tabBarH = MAIN_TAB_MIN_H;
            if (tabBarH > MAIN_TAB_ROW_MAX_H)
                tabBarH = MAIN_TAB_ROW_MAX_H;
        }
        SetWindowPos(s_hwndMainTab, HWND_TOP, margin, tabTop, tabW, tabBarH,
                     SWP_SHOWWINDOW | SWP_NOACTIVATE);
        contentTop = tabTop + tabBarH + 2;
    }
    else
    {
        contentTop = tabTop;
    }

    pageTop = contentTop;
    pageH = rc.bottom - statusH - margin - pageTop;

    SyncPerfNavVisibility();

    if (s_hwndMainTab)
    {
        int mtab = (int)TabCtrl_GetCurSel(s_hwndMainTab);
        if (mtab == TAB_MAIN_PERF)
        {
            pageLeft = navW + margin;
            pageW = rc.right - pageLeft - margin;
        }
        else
        {
            pageLeft = margin;
            pageW = rc.right - 2 * margin;
        }
    }
    else
    {
        pageLeft = margin;
        pageW = rc.right - 2 * margin;
    }

    {
        int navH = pageH;
        if (s_hwndNav && s_hwndMainTab && TabCtrl_GetCurSel(s_hwndMainTab) == TAB_MAIN_PERF)
        {
            int cnt = (int)SendMessageW(s_hwndNav, LB_GETCOUNT, 0, 0);
            if (cnt > 0)
            {
                if (cnt == 2)
                    navH = NAV_ITEM_H_CPU + NAV_ITEM_H_MEM + 4;
                else
                    navH = cnt * NAV_ITEM_H_MEM + 4;
                if (navH > pageH)
                    navH = pageH;
            }
            ShowScrollBar(s_hwndNav, SB_VERT, cnt > 6);
        }
        else if (s_hwndNav)
            ShowScrollBar(s_hwndNav, SB_VERT, TRUE);
        SetWindowPos(s_hwndNav, NULL, margin, pageTop, navW - margin, navH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (s_hwndNavSep)
    {
        if (s_hwndMainTab && TabCtrl_GetCurSel(s_hwndMainTab) == TAB_MAIN_PERF)
        {
            int navRight = margin + (navW - margin);
            SetWindowPos(s_hwndNavSep, NULL, navRight, pageTop, 2, pageH,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            ShowWindow(s_hwndNavSep, SW_SHOW);
        }
        else
            ShowWindow(s_hwndNavSep, SW_HIDE);
    }

    if (pageW < 50 || pageH < 50)
        return;

    {
        int listH = pageH;
        int footerH = 0;
        int mtab = s_hwndMainTab ? (int)TabCtrl_GetCurSel(s_hwndMainTab) : TAB_MAIN_PROCESSES;

        if (mtab == TAB_MAIN_PROCESSES && s_iPage == PAGE_PROCESSES)
        {
            footerH = 44;
            if (listH > footerH)
                listH -= footerH;
        }

        SetWindowPos(s_hwndList, NULL, pageLeft, pageTop, pageW, listH,
                     SWP_NOZORDER | SWP_NOACTIVATE);

        if (footerH > 0 && s_hwndFewerDetails && s_hwndEndTask)
        {
            int btnW = 100, btnH = 26, yBtn = pageTop + listH + 8;

            SetWindowPos(s_hwndFewerDetails, NULL, pageLeft + 4, yBtn, 180, 24,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(s_hwndEndTask, NULL, pageLeft + pageW - btnW - 8, yBtn - 1, btnW, btnH,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    if (s_iPage == PAGE_CPU)
    {
        const int titleY = pageTop + 4;
        const int titleH = 26;
        const int subH = 15;
        const int subGap = 1;
        const int graphAfterSub = 5;
        int statsH = (pageH * 30) / 100;
        if (statsH < 158)
            statsH = 158;
        if (statsH > 228)
            statsH = 228;
        int statsTop = pageTop + pageH - statsH - 8;
        int pageBottom = pageTop + pageH;
        int titleW = (pageW - 16) / 5;
        if (titleW < 96)
            titleW = 96;
        if (titleW > 200)
            titleW = 200;
        if (titleW > pageW - 16 - 120)
            titleW = (pageW - 16) - 120;
        if (titleW < 72)
            titleW = 72;
        int subY = titleY + titleH + subGap;
        int graphTopCpu = subY + subH + graphAfterSub;
        int statX = pageLeft + 4;

        graphTop = graphTopCpu;
        graphH = statsTop - graphTop - 8;
        if (graphH < 56)
        {
            graphH = 56;
            statsTop = graphTop + graphH + 8;
        }
        if (statsTop + statsH > pageBottom)
            statsTop = pageBottom - statsH - 4;
        if (statsTop < graphTop + graphH + 4)
        {
            statsTop = graphTop + graphH + 4;
            if (statsTop + statsH > pageBottom)
                statsTop = pageBottom - statsH - 4;
            if (statsTop < graphTop + 4)
                statsTop = graphTop + 4;
        }

        SetWindowPos(s_hwndCpuTitle, NULL, pageLeft + 4, titleY, titleW - 4, titleH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndCpuModel, NULL, pageLeft + 4 + titleW, titleY, pageW - 8 - titleW, titleH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndCpuGraphSub, NULL, pageLeft + 4, subY, pageW - 8, subH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndCpuLbl, NULL, pageLeft + 4, titleY, 1, 1,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndCpuBar, NULL, pageLeft + 4, titleY, 1, 1,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        if (s_hwndCpuStatsPanel)
            SetWindowPos(s_hwndCpuStatsPanel, NULL, statX, statsTop, pageW - 8, statsH,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        if (s_hwndCpuStatsPanel)
            InvalidateRect(s_hwndCpuStatsPanel, NULL, TRUE);
        if (s_hwndCpuTitle)
            InvalidateRect(s_hwndCpuTitle, NULL, TRUE);
        if (s_hwndCpuModel)
            InvalidateRect(s_hwndCpuModel, NULL, TRUE);
        if (s_hwndCpuGraphSub)
            InvalidateRect(s_hwndCpuGraphSub, NULL, TRUE);
    }
    else if (s_iPage == PAGE_MEMORY)
    {
        const int titleY = pageTop + 4;
        const int titleH = 28;
        const int subH = 15;
        const int gapSm = 6;
        const int compGraphH = 48;
        const int statsMinH = 128;
        int pageBottom = pageTop + pageH;
        int titleW = (pageW - 16) / 5;
        int sub1Y, g1Top, statsTop, statsH, compSubY, compGTop, usageH;

        if (titleW < 96)
            titleW = 96;
        if (titleW > 200)
            titleW = 200;
        if (titleW > pageW - 16 - 100)
            titleW = (pageW - 16) - 100;
        if (titleW < 72)
            titleW = 72;

        sub1Y = titleY + titleH + 2;
        g1Top = sub1Y + subH + 4;

        statsH = (pageH * 36) / 100;
        if (statsH < statsMinH)
            statsH = statsMinH;
        if (statsH > pageH / 2)
            statsH = pageH / 2;
        statsTop = pageBottom - 8 - statsH;
        compGTop = statsTop - gapSm - compGraphH;
        compSubY = compGTop - 4 - subH;
        usageH = compSubY - gapSm - g1Top;
        if (usageH < 56)
        {
            usageH = 56;
            compSubY = g1Top + usageH + gapSm;
            compGTop = compSubY + subH + 4;
            statsTop = compGTop + compGraphH + gapSm;
            statsH = pageBottom - 8 - statsTop;
            if (statsH < 100)
                statsH = pageBottom - 8 - statsTop;
        }

        graphTop = g1Top;
        graphH = usageH;
        if (graphH > pageH - 8)
            graphH = pageH - 8;

        SetWindowPos(s_hwndMemTitle, NULL, pageLeft + 4, titleY, titleW - 4, titleH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndMemModel, NULL, pageLeft + 4 + titleW, titleY, pageW - 8 - titleW, titleH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndMemGraphSub, NULL, pageLeft + 4, sub1Y, pageW - 8, subH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndMemBar, NULL, pageLeft + 4, titleY, 1, 1,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndMemCompSub, NULL, pageLeft + 4, compSubY, pageW - 8, subH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndGraphMemComp, NULL, pageLeft + 4, compGTop, pageW - 8, compGraphH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndMemStatsPanel, NULL, pageLeft + 4, statsTop, pageW - 8, statsH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(s_hwndMemStatsPanel, NULL, TRUE);
    }
    else
    {
        graphTop = pageTop;
        graphH = 1;
    }

    if (s_iPage == PAGE_CPU || s_iPage == PAGE_MEMORY)
    {
        if (graphH > pageH - 8)
            graphH = pageH - 8;
        if (s_iPage == PAGE_CPU && graphH < 100)
            graphH = 100;
    }

    SetWindowPos(s_hwndGraphCpu, NULL, pageLeft + 4, graphTop, pageW - 8, graphH,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(s_hwndGraphMem, NULL, pageLeft + 4, graphTop, pageW - 8, graphH,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    if (s_hwndStub)
    {
        int mtab = s_hwndMainTab ? (int)TabCtrl_GetCurSel(s_hwndMainTab) : TAB_MAIN_PROCESSES;
        if (mtab >= TAB_MAIN_APPHIST)
        {
            SetWindowPos(s_hwndStub, NULL, pageLeft + 8, pageTop + 16, pageW - 16, pageH - 24,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

static void
ShowPage(int page)
{
    s_iPage = page;

    if (page != PAGE_CPU && s_hwndCpuStatsPanel && IsWindow(s_hwndCpuStatsPanel))
    {
        s_CpuStatsHScrollPos = 0;
        Tm8CpuStatsUpdateScrollInfo(s_hwndCpuStatsPanel);
    }

    ShowWindow(s_hwndList, (page == PAGE_PROCESSES) ? SW_SHOW : SW_HIDE);
    if (s_hwndEndTask)
        ShowWindow(s_hwndEndTask, (page == PAGE_PROCESSES) ? SW_SHOW : SW_HIDE);
    if (s_hwndFewerDetails)
        ShowWindow(s_hwndFewerDetails, (page == PAGE_PROCESSES) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndStub, (page == PAGE_STUB) ? SW_SHOW : SW_HIDE);

    ShowWindow(s_hwndCpuTitle, (page == PAGE_CPU) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndCpuModel, (page == PAGE_CPU) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndCpuLbl, SW_HIDE);
    ShowWindow(s_hwndCpuBar, SW_HIDE);
    ShowWindow(s_hwndCpuGraphSub, (page == PAGE_CPU) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndCpuLiveLbl, SW_HIDE);
    ShowWindow(s_hwndCpuLiveVal, SW_HIDE);
    ShowWindow(s_hwndCpuStatSep, SW_HIDE);
    ShowWindow(s_hwndCpuSpecLbl, SW_HIDE);
    ShowWindow(s_hwndCpuSpecVal, SW_HIDE);
    ShowWindow(s_hwndCpuStatsPanel, (page == PAGE_CPU) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndSpeed, SW_HIDE);
    ShowWindow(s_hwndGraphCpu, (page == PAGE_CPU) ? SW_SHOW : SW_HIDE);

    ShowWindow(s_hwndMemTitle, (page == PAGE_MEMORY) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndMemModel, (page == PAGE_MEMORY) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndMemGraphSub, (page == PAGE_MEMORY) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndMemCompSub, (page == PAGE_MEMORY) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndGraphMemComp, (page == PAGE_MEMORY) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndMemStatsPanel, (page == PAGE_MEMORY) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndMemLbl, SW_HIDE);
    ShowWindow(s_hwndMemBar, SW_HIDE);
    ShowWindow(s_hwndMemDetails, SW_HIDE);
    ShowWindow(s_hwndGraphMem, (page == PAGE_MEMORY) ? SW_SHOW : SW_HIDE);

    if (s_hwndMain)
        LayoutChildren(s_hwndMain);
    if (s_hwndNav)
        InvalidateRect(s_hwndNav, NULL, TRUE);
    if (page == PAGE_PROCESSES)
        SyncProcEndTaskUi();
}

static double
ProcessCpuUsagePercent(DWORD pid, ULONGLONG *pTotal100Ns, DWORD msElapsed, UINT nCpu)
{
    HANDLE hProc;
    FILETIME fc, fe, fk, fu;
    ULARGE_INTEGER uk, uu;
    ULONGLONG total, prev;
    CPU_TRACK *tr;

    *pTotal100Ns = 0;

    hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc)
        hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProc)
        return 0.0;

    if (!GetProcessTimes(hProc, &fc, &fe, &fk, &fu))
    {
        CloseHandle(hProc);
        return 0.0;
    }
    CloseHandle(hProc);

    uk.LowPart = fk.dwLowDateTime;
    uk.HighPart = fk.dwHighDateTime;
    uu.LowPart = fu.dwLowDateTime;
    uu.HighPart = fu.dwHighDateTime;
    total = uk.QuadPart + uu.QuadPart;
    *pTotal100Ns = total;

    tr = FindCpuTrack(pid);
    if (!tr)
        return 0.0;

    prev = tr->PrevTotal100Ns;
    tr->PrevTotal100Ns = total;
    if (prev == 0 || msElapsed == 0 || nCpu == 0)
        return 0.0;

    {
        ULONGLONG diff = (total > prev) ? (total - prev) : 0;
        double denom_d = (double)msElapsed * 10000.0 * (double)nCpu;
        double pct;

        if (denom_d <= 0.0)
            return 0.0;
        pct = (double)diff * 100.0 / denom_d;
        if (pct > 100.0 * (double)nCpu)
            pct = 100.0 * (double)nCpu;
        return pct;
    }
}

static int
Tm8NamePidTie(const TM8_SCRATCH_PROC *x, const TM8_SCRATCH_PROC *y)
{
    int c = _wcsicmp(x->exe, y->exe);
    if (c != 0)
        return c;
    if (x->pid < y->pid)
        return -1;
    if (x->pid > y->pid)
        return 1;
    return 0;
}

static int __cdecl
Tm8ScratchCmpUser(const void *a, const void *b)
{
    const TM8_SCRATCH_PROC *x = (const TM8_SCRATCH_PROC *)a;
    const TM8_SCRATCH_PROC *y = (const TM8_SCRATCH_PROC *)b;

    if (s_ProcSortPhase == 0 || s_ProcSortCol == TM8_PROCSORT_COL_NONE)
        return Tm8NamePidTie(x, y);

    if (s_ProcSortCol == TM8_PROCSORT_COL_CPU)
    {
        if (s_ProcSortPhase == 1)
        {
            if (x->cpuPct > y->cpuPct)
                return -1;
            if (x->cpuPct < y->cpuPct)
                return 1;
        }
        else
        {
            if (x->cpuPct < y->cpuPct)
                return -1;
            if (x->cpuPct > y->cpuPct)
                return 1;
        }
        return Tm8NamePidTie(x, y);
    }

    if (s_ProcSortCol == TM8_PROCSORT_COL_MEM)
    {
        if (s_ProcSortPhase == 1)
        {
            if (x->ws > y->ws)
                return -1;
            if (x->ws < y->ws)
                return 1;
        }
        else
        {
            if (x->ws < y->ws)
                return -1;
            if (x->ws > y->ws)
                return 1;
        }
        return Tm8NamePidTie(x, y);
    }

    return Tm8NamePidTie(x, y);
}

static DWORD
Tm8ListViewGetItemPid(int idx)
{
    LVITEMW it;
    ZeroMemory(&it, sizeof(it));
    it.mask = LVIF_PARAM;
    it.iItem = idx;
    if (!ListView_GetItem(s_hwndList, &it))
        return DWORD_MAX;
    return (DWORD)it.lParam;
}

static TM8_SCRATCH_PROC *
Tm8ScratchFindPid(DWORD pid, int nScratch)
{
    int i;
    if (pid == DWORD_MAX)
        return NULL;
    for (i = 0; i < nScratch; i++)
    {
        if (s_ProcScratch[i].pid == pid)
            return &s_ProcScratch[i];
    }
    return NULL;
}

static void
Tm8SetProcRowStrings(int row, TM8_SCRATCH_PROC *sp)
{
    WCHAR line[96];
    double mb;
    LVITEMW it;

    ZeroMemory(&it, sizeof(it));
    it.mask = LVIF_TEXT | LVIF_IMAGE;
    it.iItem = row;
    it.iSubItem = 0;
    it.pszText = sp->exe;
    it.iImage = sp->iconIdx;
    ListView_SetItem(s_hwndList, &it);

    ListView_SetItemText(s_hwndList, row, 1, L"");

    StringCchPrintfW(line, _countof(line), L"%.1f%%", sp->cpuPct);
    ListView_SetItemText(s_hwndList, row, 2, line);

    StringCchCopyW(line, _countof(line), L"0.0 MB/s");
    ListView_SetItemText(s_hwndList, row, 3, line);

    mb = (double)sp->ws / (1024.0 * 1024.0);
    Tm8FmtMbComma1(mb, line, _countof(line));
    ListView_SetItemText(s_hwndList, row, 4, line);

    StringCchCopyW(line, _countof(line), L"0 Mbps");
    ListView_SetItemText(s_hwndList, row, 5, line);
}

static void
Tm8InsertProcRowAt(int row, TM8_SCRATCH_PROC *sp)
{
    LVITEMW it;
    ZeroMemory(&it, sizeof(it));
    it.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    it.iItem = row;
    it.iSubItem = 0;
    it.pszText = sp->exe;
    it.lParam = (LPARAM)sp->pid;
    it.iImage = sp->iconIdx;
    ListView_InsertItem(s_hwndList, &it);
    Tm8SetProcRowStrings(row, sp);
}

static int
Tm8FindRowByPid(DWORD pid)
{
    int i, n;
    if (!pid)
        return -1;
    n = ListView_GetItemCount(s_hwndList);
    for (i = 0; i < n; i++)
    {
        if (Tm8ListViewGetItemPid(i) == pid)
            return i;
    }
    return -1;
}

static void
Tm8OnProcColumnClick(int col)
{
    if (col != TM8_PROCSORT_COL_CPU && col != TM8_PROCSORT_COL_MEM)
        return;
    if (s_ProcSortCol != col)
    {
        s_ProcSortCol = col;
        s_ProcSortPhase = 1;
    }
    else
    {
        s_ProcSortPhase++;
        if (s_ProcSortPhase > 2)
        {
            s_ProcSortPhase = 0;
            s_ProcSortCol = TM8_PROCSORT_COL_NONE;
        }
    }
    if (s_hwndMain && s_iPage == PAGE_PROCESSES)
        RefreshProcessListEx(TRUE);
}

static void
Tm8SyncHeatArraysFromListView(int nScratch)
{
    int n, i;
    n = ListView_GetItemCount(s_hwndList);
    s_ProcListRows = n;
    for (i = 0; i < n && i < TM8_MAX_PROC_ROWS; i++)
    {
        DWORD pid = Tm8ListViewGetItemPid(i);
        TM8_SCRATCH_PROC *sp = (pid != DWORD_MAX) ? Tm8ScratchFindPid(pid, nScratch) : NULL;
        if (sp)
        {
            s_ProcCpuDbl[i] = sp->cpuPct;
            s_ProcMemWs[i] = sp->ws;
        }
        else
        {
            s_ProcCpuDbl[i] = 0.0;
            s_ProcMemWs[i] = 0;
        }
    }
}

static void
RefreshProcessListEx(BOOL force)
{
    HANDLE hSnap;
    PROCESSENTRY32W pe;
    int nScratch;
    SYSTEM_INFO si;
    DWORD ms = TIMER_MS;
    UINT nCpu;
    SIZE_T memMax;
    BOOL sameOrder;
    int i;
    DWORD topPid, selPid;
    int topIdx, nLvOld;

    if (s_iPage != PAGE_PROCESSES)
        return;
    if (!force)
    {
        DWORD now = GetTickCount();
        if (s_ProcListVScrollDragging)
            return;
        if (s_ProcListResumeDeadline != 0 && (LONG)(now - s_ProcListResumeDeadline) < 0)
            return;
        if (s_ProcListResumeDeadline != 0 && (LONG)(now - s_ProcListResumeDeadline) >= 0)
            s_ProcListResumeDeadline = 0;
    }

    GetSystemInfo(&si);
    nCpu = si.dwNumberOfProcessors;
    if (nCpu == 0)
        nCpu = 1;

    memMax = 1;
    nScratch = 0;

    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
        return;

    pe.dwSize = sizeof(pe);
    if (!Process32FirstW(hSnap, &pe))
    {
        CloseHandle(hSnap);
        return;
    }

    do
    {
        TM8_SCRATCH_PROC *sp;
        ULONGLONG dummy;
        HANDLE hOpen;
        PROCESS_MEMORY_COUNTERS pmc;
        SIZE_T ws = 0;

        if (nScratch >= (int)_countof(s_ProcScratch))
            continue;

        sp = &s_ProcScratch[nScratch];
        sp->pid = pe.th32ProcessID;
        StringCchCopyW(sp->exe, _countof(sp->exe), pe.szExeFile);
        sp->cpuPct = ProcessCpuUsagePercent(pe.th32ProcessID, &dummy, ms, nCpu);
        sp->path[0] = 0;

        hOpen = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
        if (!hOpen)
            hOpen = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
        if (hOpen)
        {
            pmc.cb = sizeof(pmc);
            if (GetProcessMemoryInfo(hOpen, &pmc, sizeof(pmc)))
                ws = pmc.WorkingSetSize;
            Tm8QueryProcessImagePath(hOpen, sp->path, _countof(sp->path));
            CloseHandle(hOpen);
        }
        sp->ws = ws;
        if (ws > memMax)
            memMax = ws;

        sp->iconIdx = Tm8IconForExePath(sp->path[0] ? sp->path : NULL);
        nScratch++;
    } while (Process32NextW(hSnap, &pe));

    CloseHandle(hSnap);

    s_ProcMemMax = memMax;

    if (nScratch > 1)
        qsort(s_ProcScratch, (size_t)nScratch, sizeof(s_ProcScratch[0]), Tm8ScratchCmpUser);

    sameOrder = TRUE;
    nLvOld = ListView_GetItemCount(s_hwndList);
    if (nLvOld != nScratch)
        sameOrder = FALSE;
    else
    {
        for (i = 0; i < nScratch; i++)
        {
            if (Tm8ListViewGetItemPid(i) != s_ProcScratch[i].pid)
            {
                sameOrder = FALSE;
                break;
            }
        }
    }

    topPid = 0;
    selPid = GetSelectedPid();
    topIdx = ListView_GetTopIndex(s_hwndList);
    if (topIdx >= 0 && topIdx < nLvOld)
        topPid = Tm8ListViewGetItemPid(topIdx);

    LockWindowUpdate(s_hwndList);
    if (!sameOrder)
    {
        ListView_DeleteAllItems(s_hwndList);
        for (i = 0; i < nScratch; i++)
            Tm8InsertProcRowAt(i, &s_ProcScratch[i]);
        if (topPid != 0 && topPid != DWORD_MAX)
        {
            int r = Tm8FindRowByPid(topPid);
            if (r >= 0)
                SendMessageW(s_hwndList, LVM_SETTOPINDEX, (WPARAM)(INT)r, 0);
        }
        if (selPid)
        {
            int r = Tm8FindRowByPid(selPid);
            if (r >= 0)
                ListView_SetItemState(s_hwndList, r, LVIS_SELECTED | LVIS_FOCUSED,
                                      LVIS_SELECTED | LVIS_FOCUSED);
        }
    }
    else
    {
        for (i = 0; i < nScratch; i++)
            Tm8SetProcRowStrings(i, &s_ProcScratch[i]);
    }
    LockWindowUpdate(NULL);

    Tm8SyncHeatArraysFromListView(nScratch);

    UpdateProcListHeaders();
    SyncProcEndTaskUi();
}

static void
RefreshProcessList(void)
{
    RefreshProcessListEx(FALSE);
}

static void
SamplePerfCounters(void)
{
    int cpuPct;
    MEMORYSTATUSEX msx;
    PERFORMANCE_INFORMATION pi;
    DWORD memPct = 0;
    DWORD i, n;

    SampleCpuMetrics();
    ComputeTotalCpuPercent();
    UpdateCurrentCpuMhzSample();
    /* PDH % Processor Utility when available (Win10+ Task Manager); else GetSystemTimes above. */
    cpuPct = s_LastCpuPct;

    ZeroMemory(&pi, sizeof(pi));
    pi.cb = sizeof(pi);
    if (GetPerformanceInfo(&pi, sizeof(pi)) && pi.PhysicalTotal > 0)
    {
        DWORD tot = pi.PhysicalTotal;
        DWORD av = pi.PhysicalAvailable;
        if (av > tot)
            av = tot;
        memPct = (DWORD)((100.0 * (double)(tot - av)) / (double)tot);
    }
    else
    {
        msx.dwLength = sizeof(msx);
        if (GlobalMemoryStatusEx(&msx) && msx.ullTotalPhys > 0)
            memPct = (DWORD)(100.0 -
                (double)msx.ullAvailPhys / (double)msx.ullTotalPhys * 100.0);
    }
    s_LastMemUsagePct = memPct;

    msx.dwLength = sizeof(msx);
    if (GlobalMemoryStatusEx(&msx) && msx.ullTotalPhys > 0)
    {
        s_MemTotalPhysNav = msx.ullTotalPhys;
        if (msx.ullAvailPhys > msx.ullTotalPhys)
            s_MemUsedPhysNav = 0;
        else
            s_MemUsedPhysNav = msx.ullTotalPhys - msx.ullAvailPhys;
    }

    n = s_NumLogicalCpus;
    if (n > TM8_MAX_LOGICAL_CPU)
        n = TM8_MAX_LOGICAL_CPU;

    if (!s_PerfHistPrimed)
    {
        /*
         * Prime with zeros so the graph fills in from real samples. Priming with
         * the current % paints a long flat line until the whole ring scrolls out.
         */
        FillMemory(s_CpuHist, sizeof(s_CpuHist), 0);
        FillMemory(s_MemHist, sizeof(s_MemHist), 0);
        for (i = 0; i < n; i++)
            FillMemory(s_CpuHistPer[i], CPU_HIST_LEN, 0);
        s_CpuHistPos = 0;
        s_MemHistPos = 0;
        s_PerfHistPrimed = TRUE;
    }

    s_CpuHist[s_CpuHistPos] = (BYTE)cpuPct;
    s_MemHist[s_MemHistPos] = (BYTE)memPct;
    for (i = 0; i < n; i++)
        s_CpuHistPer[i][s_CpuHistPos] = (BYTE)s_LastCpuPctPer[i];

    s_CpuHistPos = (s_CpuHistPos + 1) % CPU_HIST_LEN;
    s_MemHistPos = (s_MemHistPos + 1) % CPU_HIST_LEN;

    if (s_hwndNav)
        InvalidateRect(s_hwndNav, NULL, FALSE);
}

static void
RefreshCpuDetailUi(void)
{
    WCHAR wbuf[96];

    if (s_iPage != PAGE_CPU)
        return;

    InvalidateRect(s_hwndGraphCpu, NULL, FALSE);

    LoadStr(IDS_PANEL_CPU, wbuf, _countof(wbuf));
    SetWindowTextW(s_hwndCpuTitle, wbuf);
    SetWindowTextW(s_hwndCpuModel, s_szCpuModel);
    LoadStr(IDS_CPU_UTIL_60S, wbuf, _countof(wbuf));
    SetWindowTextW(s_hwndCpuGraphSub, wbuf);

    RefreshCpuStatsPanel();
}

/* MSVC x86 emits __allmul for 64x64 multiply; ReactOS taskmgr8 links without that CRT helper. */
static ULONGLONG
Tm8MulPagesToBytes(SIZE_T pages, SIZE_T pageBytes)
{
    return (ULONGLONG)((double)(ULONG_PTR)pages * (double)(ULONG_PTR)pageBytes);
}

static ULONGLONG
Tm8KilobytesToBytes64(ULONGLONG kilobytes)
{
    return (ULONGLONG)((double)kilobytes * 1024.0);
}

static ULONGLONG
Tm8StandbyListBytes(SIZE_T pageBytes)
{
    TM8_SYSTEM_MEMORY_LIST_INFORMATION mli;
    ULONG rl = 0;
    SIZE_T standbyPages = 0;
    int i;
    LONG st;

    if (!s_pNtQSI || !pageBytes)
        return 0;
    ZeroMemory(&mli, sizeof(mli));
    st = s_pNtQSI(TM8_SystemMemoryListInformation, &mli, sizeof(mli), &rl);
    if (!TM8_NT_SUCCESS(st) || rl < sizeof(mli))
        return 0;
    for (i = 0; i < 8; i++)
        standbyPages += mli.PageCountByPriority[i];
    return Tm8MulPagesToBytes(standbyPages, pageBytes);
}

static BOOL
Tm8TryCompressedRamBytes(ULONGLONG *pOutBytes, ULONGLONG totalPhys)
{
    UCHAR buf[128];
    ULONG rl = 0;
    LONG st;
    size_t off;
    ULONGLONG bestNarrow = 0;
    ULONGLONG bestWide = 0;
    ULONGLONG cap;

    if (!s_pNtQSI || !pOutBytes || !totalPhys)
        return FALSE;

    cap = totalPhys / 2;
    if (cap > 2ULL * 1024 * 1024 * 1024)
        cap = 2ULL * 1024 * 1024 * 1024;

    ZeroMemory(buf, sizeof(buf));
    st = s_pNtQSI(TM8_SystemCompressionInformation, buf, sizeof(buf), &rl);
    if (!TM8_NT_SUCCESS(st) || rl < 16)
        return FALSE;

    for (off = 0; off + sizeof(ULONGLONG) <= rl && off <= 96; off += sizeof(ULONGLONG))
    {
        ULONGLONG cand;
        CopyMemory(&cand, buf + off, sizeof(cand));
        if (cand >= 1024ULL * 1024 && cand <= 512ULL * 1024 * 1024 && cand < totalPhys)
        {
            if (cand > bestNarrow)
                bestNarrow = cand;
        }
        if (cand >= 64 * 1024 && cand < totalPhys && cand <= cap)
        {
            if (cand > bestWide)
                bestWide = cand;
        }
    }

    if (bestNarrow)
    {
        *pOutBytes = bestNarrow;
        return TRUE;
    }
    if (bestWide)
    {
        *pOutBytes = bestWide;
        return TRUE;
    }
    return FALSE;
}

#define TM8_RSMB_SIGNATURE 0x52534D42

static void
Tm8FmtBytesGbOneDecimal(ULONGLONG bytes, WCHAR *dst, size_t cch)
{
    double gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);
    StringCchPrintfW(dst, cch, L"%.1f GB", gb);
}

static void
Tm8FmtCompressedParens(ULONGLONG bytes, WCHAR *dst, size_t cch)
{
    if (bytes >= 1024ULL * 1024 * 1024)
        Tm8FmtBytesGbOneDecimal(bytes, dst, cch);
    else
        StringCchPrintfW(dst, cch, L"%.1f MB", (double)bytes / (1024.0 * 1024.0));
}

static const BYTE *
Tm8SmbiosSkipStrings(const BYTE *p, BYTE slen, const BYTE *end)
{
    const BYTE *s = p + slen;
    while (s + 1 < end)
    {
        if (s[0] == 0 && s[1] == 0)
            return s + 2;
        s++;
    }
    return NULL;
}

static BOOL
Tm8SmbiosType17Installed(const BYTE *p, BYTE slen)
{
    WORD sz;
    if (slen < 0x0E)
        return FALSE;
    sz = *(const WORD *)(p + 0x0C);
    if (sz == 0 || sz == 0xFFFF)
        return FALSE;
    return TRUE;
}

static UINT
Tm8SmbiosType17SpeedMt(const BYTE *p, BYTE slen)
{
    WORD w;
    if (slen >= 0x22)
    {
        w = *(const WORD *)(p + 0x20);
        if (w != 0 && w != 0xFFFF)
            return (UINT)w;
    }
    if (slen >= 0x14)
    {
        w = *(const WORD *)(p + 0x12);
        if (w != 0 && w != 0xFFFF)
            return (UINT)w;
    }
    return 0;
}

static void
Tm8SmbiosFormFactorToString(BYTE ff, WCHAR *dst, size_t cch)
{
    switch (ff)
    {
    case 4:
    case 10:
    case 12:
        StringCchCopyW(dst, cch, L"SODIMM");
        return;
    default:
        StringCchCopyW(dst, cch, L"DIMM");
        return;
    }
}

static BOOL
Tm8LoadSmbiosTable(BYTE **ppOut, DWORD *pcb)
{
    DWORD n, chk;
    BYTE *buf = NULL;
    HKEY hk;

    *ppOut = NULL;
    *pcb = 0;
    if (s_pfnGetSystemFirmwareTable)
    {
        n = s_pfnGetSystemFirmwareTable(TM8_RSMB_SIGNATURE, 0, NULL, 0);
        if (n >= 8)
        {
            buf = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n);
            if (!buf)
                return FALSE;
            chk = s_pfnGetSystemFirmwareTable(TM8_RSMB_SIGNATURE, 0, buf, n);
            if (chk == n)
            {
                *ppOut = buf;
                *pcb = n;
                return TRUE;
            }
            HeapFree(GetProcessHeap(), 0, buf);
        }
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\mssmbios\\Data", 0,
                      KEY_READ, &hk) == ERROR_SUCCESS)
    {
        DWORD typ, cb = 0;
        if (RegQueryValueExW(hk, L"SMBiosData", NULL, &typ, NULL, &cb) == ERROR_SUCCESS &&
            typ == REG_BINARY && cb >= 8)
        {
            buf = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
            if (buf && RegQueryValueExW(hk, L"SMBiosData", NULL, &typ, buf, &cb) == ERROR_SUCCESS)
            {
                RegCloseKey(hk);
                *ppOut = buf;
                *pcb = cb;
                return TRUE;
            }
            HeapFree(GetProcessHeap(), 0, buf);
        }
        RegCloseKey(hk);
    }
    return FALSE;
}

static const BYTE *
Tm8SmbiosTablePtr(const BYTE *buf, DWORD cb, DWORD *pTableLen)
{
    DWORD len;
    if (cb >= 8)
    {
        len = *(const DWORD *)(buf + 4);
        if (len > 0 && 8 + len <= cb)
        {
            *pTableLen = len;
            return buf + 8;
        }
    }
    *pTableLen = cb;
    return buf;
}

static void
Tm8RefreshMemDimmInfo(void)
{
    BYTE *raw = NULL;
    DWORD rawCb = 0;
    const BYTE *tbl, *end;
    DWORD tblLen;
    UINT pop = 0, tot = 0, bestSpd = 0;
    BYTE firstFf = 0xFF;
    BOOL haveFf = FALSE;
    DWORD now;
    TM8_MEM_DIMM_INFO out;

    now = GetTickCount();
    if (s_MemDimm.tickCached && (DWORD)(now - s_MemDimm.tickCached) < 20000)
        return;

    ZeroMemory(&out, sizeof(out));
    out.tickCached = now;

    if (!Tm8LoadSmbiosTable(&raw, &rawCb))
    {
        s_MemDimm.tickCached = now;
        return;
    }

    tbl = Tm8SmbiosTablePtr(raw, rawCb, &tblLen);
    end = tbl + tblLen;

    while (tbl + 4 <= end)
    {
        BYTE t = tbl[0];
        BYTE slen = tbl[1];
        const BYTE *next;

        if (slen < 4 || tbl + slen > end)
            break;

        if (t == 16 && slen >= 0x11)
        {
            WORD n = *(const WORD *)(tbl + 0x0F);
            if (n > 0 && n < 512 && (UINT)n > tot)
                tot = (UINT)n;
        }
        else if (t == 17 && Tm8SmbiosType17Installed(tbl, slen))
        {
            UINT sp = Tm8SmbiosType17SpeedMt(tbl, slen);
            pop++;
            if (sp > bestSpd)
                bestSpd = sp;
            if (!haveFf && slen > 0x0E)
            {
                firstFf = tbl[0x0E];
                haveFf = TRUE;
            }
        }

        if (t == 127 && slen == 4)
            break;

        next = Tm8SmbiosSkipStrings(tbl, slen, end);
        if (!next || next > end)
            break;
        tbl = next;
    }

    HeapFree(GetProcessHeap(), 0, raw);

    if (pop > 0)
    {
        out.slotsPopulated = pop;
        out.haveSlots = TRUE;
        if (tot < pop)
            tot = pop;
        out.slotsTotal = tot > 0 ? tot : pop;
    }
    if (bestSpd > 0)
    {
        out.speedMt = bestSpd;
        out.haveSpeed = TRUE;
    }
    if (haveFf)
    {
        Tm8SmbiosFormFactorToString(firstFf, out.formFactor, _countof(out.formFactor));
        out.haveForm = TRUE;
    }

    s_MemDimm = out;
}

static void
RefreshMemoryStatsPanel(void)
{
    MEMORYSTATUSEX msx;
    PERFORMANCE_INFORMATION pi;
    TM8_MEM_STATS_PAINT *ps = &s_MemStatsPaint;
    WCHAR unk[16];
    WCHAR szU[64], szZ[32];
    ULONGLONG total, avail, used, pageSize = 0;
    ULONGLONG commitCur = 0, commitLim = 0;
    ULONGLONG pagedB = 0, nonPagedB = 0, cachedB = 0;
    ULONGLONG cachedShow = 0;
    ULONGLONG compBytes = 0;
    ULONGLONG installedBytes = 0;
    BOOL havePi = FALSE;
    double fu, denom;

    if (!s_hwndMemStatsPanel)
        return;

    LoadStr(IDS_VIRT_UNKNOWN, unk, _countof(unk));
    szZ[0] = 0;
    StrFormatByteSizeW(0, szZ, _countof(szZ));

    ZeroMemory(ps, sizeof(*ps));
    msx.dwLength = sizeof(msx);
    GlobalMemoryStatusEx(&msx);
    total = msx.ullTotalPhys;
    avail = msx.ullAvailPhys;
    if (avail > total)
        avail = total;
    used = total - avail;

    Tm8RefreshMemDimmInfo();

    if (s_pfnGetPhysMem)
    {
        ULONGLONG kb = 0;
        if (s_pfnGetPhysMem(&kb) && kb > 0)
            installedBytes = Tm8KilobytesToBytes64(kb);
    }

    ZeroMemory(&pi, sizeof(pi));
    pi.cb = sizeof(pi);
    if (GetPerformanceInfo(&pi, sizeof(pi)) && pi.PageSize)
    {
        havePi = TRUE;
        pageSize = (ULONGLONG)pi.PageSize;
        commitCur = Tm8MulPagesToBytes(pi.CommitTotal, pi.PageSize);
        commitLim = Tm8MulPagesToBytes(pi.CommitLimit, pi.PageSize);
        pagedB = Tm8MulPagesToBytes(pi.KernelPaged, pi.PageSize);
        nonPagedB = Tm8MulPagesToBytes(pi.KernelNonpaged, pi.PageSize);
        cachedB = Tm8MulPagesToBytes(pi.SystemCache, pi.PageSize);
    }

    LoadStr(IDS_MEM_LBL_INUSE, ps->c1Lbl1, _countof(ps->c1Lbl1));
    Tm8FmtBytesGbOneDecimal(used, szU, _countof(szU));
    if (Tm8TryCompressedRamBytes(&compBytes, total))
        Tm8FmtCompressedParens(compBytes, szZ, _countof(szZ));
    else
        Tm8FmtCompressedParens(0, szZ, _countof(szZ));
    StringCchPrintfW(ps->c1Val1, _countof(ps->c1Val1), L"%ls (%ls)", szU, szZ);

    LoadStr(IDS_MEM_LBL_COMMIT, ps->c1Lbl2, _countof(ps->c1Lbl2));
    if (havePi && commitLim)
    {
        double gCur = (double)commitCur / (1024.0 * 1024.0 * 1024.0);
        double gLim = (double)commitLim / (1024.0 * 1024.0 * 1024.0);
        StringCchPrintfW(ps->c1Val2, _countof(ps->c1Val2), L"%.1f/%.1f GB", gCur, gLim);
    }
    else
        StringCchCopyW(ps->c1Val2, _countof(ps->c1Val2), unk);

    LoadStr(IDS_MEM_LBL_PAGED, ps->c1Lbl3, _countof(ps->c1Lbl3));
    if (havePi)
        Tm8FmtBytesGbOneDecimal(pagedB, ps->c1Val3, _countof(ps->c1Val3));
    else
        StringCchCopyW(ps->c1Val3, _countof(ps->c1Val3), unk);

    LoadStr(IDS_MEM_LBL_AVAIL, ps->c2Lbl1, _countof(ps->c2Lbl1));
    Tm8FmtBytesGbOneDecimal(avail, ps->c2Val1, _countof(ps->c2Val1));

    LoadStr(IDS_MEM_LBL_CACHED, ps->c2Lbl2, _countof(ps->c2Lbl2));
    if (havePi)
    {
        ULONGLONG standbyB = Tm8StandbyListBytes((SIZE_T)pi.PageSize);
        if (standbyB > 0)
            cachedShow = standbyB;
        else
            cachedShow = cachedB;
        if (cachedShow > total)
            cachedShow = total;
        if (cachedShow > avail)
            cachedShow = avail;
        Tm8FmtBytesGbOneDecimal(cachedShow, ps->c2Val2, _countof(ps->c2Val2));
    }
    else
        StringCchCopyW(ps->c2Val2, _countof(ps->c2Val2), unk);

    LoadStr(IDS_MEM_LBL_NONPAGED, ps->c2Lbl3, _countof(ps->c2Lbl3));
    if (havePi)
        Tm8FmtBytesGbOneDecimal(nonPagedB, ps->c2Val3, _countof(ps->c2Val3));
    else
        StringCchCopyW(ps->c2Val3, _countof(ps->c2Val3), unk);

    LoadStr(IDS_MEM_LBL_SPEED, ps->c3Lbl1, _countof(ps->c3Lbl1));
    if (s_MemDimm.haveSpeed && s_MemDimm.speedMt > 0)
        StringCchPrintfW(ps->c3Val1, _countof(ps->c3Val1), L"%u MT/s", s_MemDimm.speedMt);
    else
        StringCchCopyW(ps->c3Val1, _countof(ps->c3Val1), unk);

    LoadStr(IDS_MEM_LBL_SLOTS, ps->c3Lbl2, _countof(ps->c3Lbl2));
    if (s_MemDimm.haveSlots)
        StringCchPrintfW(ps->c3Val2, _countof(ps->c3Val2), L"%u of %u", s_MemDimm.slotsPopulated,
                         s_MemDimm.slotsTotal);
    else
        StringCchCopyW(ps->c3Val2, _countof(ps->c3Val2), unk);

    LoadStr(IDS_MEM_LBL_FORM, ps->c3Lbl3, _countof(ps->c3Lbl3));
    if (s_MemDimm.haveForm)
        StringCchCopyW(ps->c3Val3, _countof(ps->c3Val3), s_MemDimm.formFactor);
    else
        StringCchCopyW(ps->c3Val3, _countof(ps->c3Val3), unk);

    LoadStr(IDS_MEM_LBL_HWRES, ps->c3Lbl4, _countof(ps->c3Lbl4));
    if (s_pfnGetPhysMem)
    {
        ULONGLONG kb = 0;
        if (s_pfnGetPhysMem(&kb) && kb > 0)
        {
            ULONGLONG inst = Tm8KilobytesToBytes64(kb);
            if (inst > total)
                Tm8FmtBytesGbOneDecimal(inst - total, ps->c3Val4, _countof(ps->c3Val4));
            else
                Tm8FmtBytesGbOneDecimal(0, ps->c3Val4, _countof(ps->c3Val4));
        }
        else
            StringCchCopyW(ps->c3Val4, _countof(ps->c3Val4), unk);
    }
    else
        StringCchCopyW(ps->c3Val4, _countof(ps->c3Val4), unk);

    denom = (double)total;
    if (installedBytes > total)
        denom = (double)installedBytes;
    if (denom <= 0.0)
        fu = 0.0;
    else
    {
        fu = (double)used / denom;
        if (fu > 1.0)
            fu = 1.0;
    }
    /* Composition bar: Win10-style “in use” vs “available” (single rest segment). */
    s_MemCompFrac[0] = fu;
    s_MemCompFrac[1] = 0.0;
    s_MemCompFrac[2] = (fu <= 1.0) ? (1.0 - fu) : 0.0;

    InvalidateRect(s_hwndMemStatsPanel, NULL, TRUE);
}

static void
RefreshMemoryDetailUi(void)
{
    MEMORYSTATUSEX msx;
    WCHAR wbuf[160];

    if (s_iPage != PAGE_MEMORY)
        return;

    LoadStr(IDS_PANEL_MEMORY, wbuf, _countof(wbuf));
    SetWindowTextW(s_hwndMemTitle, wbuf);

    msx.dwLength = sizeof(msx);
    GlobalMemoryStatusEx(&msx);
    /* Win10 TM headline = DIMM total from firmware; graph scale = OS-visible RAM. */
    if (s_pfnGetPhysMem)
    {
        ULONGLONG kb = 0;
        if (s_pfnGetPhysMem(&kb) && kb > 0)
            Tm8FmtBytesGbOneDecimal(Tm8KilobytesToBytes64(kb), wbuf, _countof(wbuf));
        else
            Tm8FmtBytesGbOneDecimal(msx.ullTotalPhys, wbuf, _countof(wbuf));
    }
    else
        Tm8FmtBytesGbOneDecimal(msx.ullTotalPhys, wbuf, _countof(wbuf));
    SetWindowTextW(s_hwndMemModel, wbuf);

    LoadStr(IDS_GRAPH_MEMCAP, wbuf, _countof(wbuf));
    SetWindowTextW(s_hwndMemGraphSub, wbuf);
    LoadStr(IDS_MEM_COMPOSITION, wbuf, _countof(wbuf));
    SetWindowTextW(s_hwndMemCompSub, wbuf);

    Tm8FmtBytesGbOneDecimal(msx.ullTotalPhys, s_szMemGraphYMax, _countof(s_szMemGraphYMax));

    InvalidateRect(s_hwndGraphMem, NULL, FALSE);
    InvalidateRect(s_hwndGraphMemComp, NULL, FALSE);

    RefreshMemoryStatsPanel();
}

static void
OnTimer(void)
{
    SamplePerfCounters();
    RefreshProcessList();
    RefreshCpuDetailUi();
    RefreshMemoryDetailUi();

    if (s_hwndStatus && IsWindowVisible(s_hwndStatus))
    {
        WCHAR b[160];
        StringCchPrintfW(b, _countof(b), L"CPU %d%%   RAM %lu%%", s_LastCpuPct,
                         (ULONG)s_LastMemUsagePct);
        SendMessageW(s_hwndStatus, SB_SETTEXT, 0, (LPARAM)b);
    }
}

static DWORD
GetSelectedPid(void)
{
    int i = ListView_GetNextItem(s_hwndList, -1, LVNI_SELECTED);
    LVITEMW it;
    if (i < 0)
        return 0;
    ZeroMemory(&it, sizeof(it));
    it.mask = LVIF_PARAM;
    it.iItem = i;
    ListView_GetItem(s_hwndList, &it);
    return (DWORD)it.lParam;
}

static void
SyncProcEndTaskUi(void)
{
    BOOL en = (s_iPage == PAGE_PROCESSES && GetSelectedPid() != 0);
    if (s_hwndEndTask)
        EnableWindow(s_hwndEndTask, en);
}

static LRESULT CALLBACK
ListViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN && wParam == VK_DELETE)
    {
        if (s_hwndMainTab && TabCtrl_GetCurSel(s_hwndMainTab) == TAB_MAIN_PROCESSES &&
            s_iPage == PAGE_PROCESSES && s_hwndMain)
        {
            PostMessageW(s_hwndMain, WM_COMMAND, MAKEWPARAM(IDM_ENDTASK, 0), 0);
            return 0;
        }
    }
    if (msg == WM_VSCROLL)
    {
        UINT code = LOWORD(wParam);
        switch (code)
        {
        case SB_THUMBTRACK:
            s_ProcListVScrollDragging = TRUE;
            s_ProcListResumeDeadline = 0;
            break;
        case SB_ENDSCROLL:
            s_ProcListVScrollDragging = FALSE;
            s_ProcListResumeDeadline = GetTickCount() + 1000;
            break;
        case SB_THUMBPOSITION:
            s_ProcListVScrollDragging = FALSE;
            s_ProcListResumeDeadline = GetTickCount() + 1000;
            break;
        case SB_LINEUP:
        case SB_LINEDOWN:
        case SB_PAGEUP:
        case SB_PAGEDOWN:
        case SB_TOP:
        case SB_BOTTOM:
            s_ProcListResumeDeadline = GetTickCount() + 1000;
            break;
        default:
            break;
        }
    }
    return CallWindowProcW(s_pfnOldListView, hwnd, msg, wParam, lParam);
}

static void
EndSelectedTask(HWND hwnd)
{
    DWORD pid = GetSelectedPid();
    WCHAR t[256], cap[64];
    HANDLE hProc;

    if (!pid)
        return;

    if (pid == GetCurrentProcessId())
    {
        LoadStr(IDS_ENDTASK_SELF, t, _countof(t));
        LoadStr(IDS_APP_TITLE, cap, _countof(cap));
        MessageBoxW(hwnd, t, cap, MB_ICONINFORMATION | MB_OK);
        return;
    }

    LoadStr(IDS_ENDTASK_WARN, t, _countof(t));
    LoadStr(IDS_APP_TITLE, cap, _countof(cap));
    if (MessageBoxW(hwnd, t, cap, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) != IDYES)
        return;

    hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProc)
    {
        LoadStr(IDS_ERR_OPEN, t, _countof(t));
        MessageBoxW(hwnd, t, cap, MB_ICONERROR);
        return;
    }
    if (TerminateProcess(hProc, 1))
    {
        CloseHandle(hProc);
        RefreshProcessListEx(TRUE);
        return;
    }
    CloseHandle(hProc);
    LoadStr(IDS_ERR_OPEN, t, _countof(t));
    MessageBoxW(hwnd, t, cap, MB_ICONERROR);
}

static void
FillNavList(void)
{
    WCHAR b[128];
    SendMessageW(s_hwndNav, LB_RESETCONTENT, 0, 0);
    LoadStr(IDS_NAV_CPU, b, _countof(b));
    SendMessageW(s_hwndNav, LB_ADDSTRING, 0, (LPARAM)b);
    LoadStr(IDS_NAV_MEMORY, b, _countof(b));
    SendMessageW(s_hwndNav, LB_ADDSTRING, 0, (LPARAM)b);
    SendMessageW(s_hwndNav, LB_SETCURSEL, s_iPerfNavSel, 0);
}

static void
InitMainTabs(void)
{
    TCITEMW ti;
    UINT ids[7] = {
        IDS_NAV_PROCESSES, IDS_TAB_PERFORMANCE, IDS_TAB_APPHISTORY, IDS_TAB_STARTUP,
        IDS_TAB_USERS, IDS_TAB_DETAILS, IDS_TAB_SERVICES
    };
    int i;

    if (!s_hwndMainTab)
        return;

    TabCtrl_DeleteAllItems(s_hwndMainTab);

    ZeroMemory(&ti, sizeof(ti));
    ti.mask = TCIF_TEXT;

    for (i = 0; i < 7; i++)
    {
        LoadStr(ids[i], s_MainTabText[i], _countof(s_MainTabText[i]));
        ti.pszText = s_MainTabText[i];
        TabCtrl_InsertItem(s_hwndMainTab, i, &ti);
    }

    TabCtrl_SetCurSel(s_hwndMainTab, TAB_MAIN_PROCESSES);
}

static void
SetupListColumns(void)
{
    WCHAR b[64];
    LVCOLUMNW col;
    int wName = 240, wSt = 72, wCpu = 88, wDisk = 100, wMem = 120, wNet = 100;

    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;

    LoadStr(IDS_COL_NAME, b, _countof(b));
    col.pszText = b;
    col.cx = wName;
    ListView_InsertColumn(s_hwndList, 0, &col);

    LoadStr(IDS_COL_STATUS, b, _countof(b));
    col.pszText = b;
    col.cx = wSt;
    ListView_InsertColumn(s_hwndList, 1, &col);

    LoadStr(IDS_COL_CPU, b, _countof(b));
    col.pszText = b;
    col.cx = wCpu;
    ListView_InsertColumn(s_hwndList, 2, &col);

    LoadStr(IDS_COL_DISK, b, _countof(b));
    col.pszText = b;
    col.cx = wDisk;
    ListView_InsertColumn(s_hwndList, 3, &col);

    LoadStr(IDS_COL_MEM, b, _countof(b));
    col.pszText = b;
    col.cx = wMem;
    ListView_InsertColumn(s_hwndList, 4, &col);

    LoadStr(IDS_COL_NETWORK, b, _countof(b));
    col.pszText = b;
    col.cx = wNet;
    ListView_InsertColumn(s_hwndList, 5, &col);
}

static void
Tm8CopyOneLine(const WCHAR **pp, WCHAR *dst, size_t cchDst)
{
    const WCHAR *p = *pp;
    size_t n = 0;

    if (!dst || cchDst == 0)
        return;
    if (!p)
    {
        dst[0] = 0;
        return;
    }
    while (*p && *p != L'\r' && *p != L'\n' && n + 1 < cchDst)
        dst[n++] = *p++;
    dst[n] = 0;
    if (*p == L'\r' && p[1] == L'\n')
        p += 2;
    else if (*p == L'\n' || *p == L'\r')
        p++;
    *pp = p;
}

static void
DrawCpuSpecKvColumn(HDC hdc, const RECT *rcCol, const WCHAR *pl, const WCHAR *pv, int rowSpecH)
{
    int y = rcCol->top;
    WCHAR lineL[128], lineV[256];
    int guard = 0;
    int specWcol = rcCol->right - rcCol->left;
    int valBand;

    if (specWcol < 32 || !pl || !pv)
        return;

    valBand = (specWcol * 42) / 100;
    if (valBand < 52)
        valBand = 52;
    if (valBand > 120)
        valBand = 120;
    if (valBand > specWcol - 10)
        valBand = (specWcol * 48) / 100;
    if (valBand < 40)
        valBand = 40;

    while (*pl && *pv && y + rowSpecH <= rcCol->bottom && guard++ < 48)
    {
        RECT rL, rVal;

        Tm8CopyOneLine(&pl, lineL, _countof(lineL));
        Tm8CopyOneLine(&pv, lineV, _countof(lineV));
        rL.left = rcCol->left;
        rL.right = rcCol->right - valBand - 4;
        rL.top = y;
        rL.bottom = y + rowSpecH;
        SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(hdc, RGB(96, 96, 96));
        DrawTextW(hdc, lineL, -1, &rL, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        rVal.left = rcCol->right - valBand;
        rVal.right = rcCol->right;
        rVal.top = y;
        rVal.bottom = y + rowSpecH;
        SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(hdc, RGB(32, 32, 32));
        DrawTextW(hdc, lineV, -1, &rVal,
                  DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        y += rowSpecH + 1;
    }
}

static void
DrawCpuPerfStatsPanel(HDC hdc, const RECT *rcPanel)
{
    int w = rcPanel->right - rcPanel->left;
    int sep1, sep2, y;
    HFONT oldF;
    TEXTMETRICW tmLbl, tmMid, tmSpec;
    int gapRow = 6;
    int lblH, midH, rowSpecH;
    int half, tw, c0, c1, c2;
    RECT rcLeft, rcMid, rcCache;
    RECT rU, rS, rL, rV;

    if (w < 100 || rcPanel->bottom <= rcPanel->top + 12)
        return;

    FillRect(hdc, rcPanel, (HBRUSH)GetStockObject(WHITE_BRUSH));

    /*
     * Win10/11 CPU stats: three stable columns — live metrics | socket/core/virt | caches.
     * Equal thirds of inner width so widening the window does not change structure.
     */
    {
        const int padOut = 8;
        const int inset = 6;
        int L = rcPanel->left + padOut;
        int R = rcPanel->right - padOut;
        int W = R - L;
        int w1, w2;

        if (W < 60)
            return;
        w1 = W / 3;
        w2 = W / 3;
        sep1 = L + w1;
        sep2 = L + w1 + w2;

        rcLeft.left = L + inset;
        rcLeft.right = sep1 - inset;
        rcMid.left = sep1 + inset;
        rcMid.right = sep2 - inset;
        rcCache.left = sep2 + inset;
        rcCache.right = R - inset;

        if (rcLeft.right < rcLeft.left + 40)
            rcLeft.right = rcLeft.left + 40;
        if (rcMid.right < rcMid.left + 40)
            rcMid.right = rcMid.left + 40;
        if (rcCache.right < rcCache.left + 40)
            rcCache.right = rcCache.left + 40;
    }
    rcLeft.top = rcPanel->top + 6;
    rcLeft.bottom = rcPanel->bottom - 6;
    rcMid.top = rcLeft.top;
    rcMid.bottom = rcLeft.bottom;
    rcCache.top = rcLeft.top;
    rcCache.bottom = rcLeft.bottom;

    SetBkMode(hdc, TRANSPARENT);

    oldF = (HFONT)SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    GetTextMetricsW(hdc, &tmLbl);
    lblH = tmLbl.tmHeight + 1;

    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    GetTextMetricsW(hdc, &tmMid);
    midH = tmMid.tmHeight + 2;

    y = rcLeft.top;
    /* Utilization | Speed */
    half = (rcLeft.right - rcLeft.left) / 2;
    rU.left = rcLeft.left;
    rU.right = rcLeft.left + half - 6;
    rU.top = y;
    rU.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.utilLbl, -1, &rU, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rU.top = y + lblH + 2;
    rU.bottom = rU.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.utilVal, -1, &rU,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    rS.left = rcLeft.left + half;
    rS.right = rcLeft.right;
    rS.top = y;
    rS.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.speedLbl, -1, &rS, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rS.top = y + lblH + 2;
    rS.bottom = rS.top + midH;
    if (s_CpuStatsPaint.speedVal[0])
    {
        SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(hdc, RGB(32, 32, 32));
        DrawTextW(hdc, s_CpuStatsPaint.speedVal, -1, &rS,
                  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    }

    y += lblH + 2 + midH + gapRow;

    /* Processes | Threads | Handles — fixed gutters so columns don’t drift apart */
    {
        int innerL = rcLeft.right - rcLeft.left;
        int gapMid = 10;
        tw = (innerL - 2 * gapMid) / 3;
        if (tw < 52)
            tw = 52;
        c0 = rcLeft.left;
        c1 = c0 + tw + gapMid;
        c2 = c1 + tw + gapMid;
    }

    rL.left = c0;
    rL.right = c0 + tw - 2;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.procLbl, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rV.left = rL.left;
    rV.right = rL.right;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.procVal, -1, &rV,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    rL.left = c1;
    rL.right = c1 + tw - 2;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.threadLbl, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rV.left = rL.left;
    rV.right = rL.right;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.threadVal, -1, &rV,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    rL.left = c2;
    rL.right = rcLeft.right;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.handleLbl, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rV.left = rL.left;
    rV.right = rL.right;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.handleVal, -1, &rV,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    y += lblH + 2 + midH + gapRow;

    /* Up time */
    rL.left = rcLeft.left;
    rL.right = rcLeft.right;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.upLbl, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rV.left = rcLeft.left;
    rV.right = rcLeft.right;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.upVal, -1, &rV,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    SelectObject(hdc, oldF);

    /* Middle + cache columns (same row height as former single spec strip). */
    {
        int specLblH, specValH;
        SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
        GetTextMetricsW(hdc, &tmSpec);
        specLblH = tmSpec.tmHeight;
        SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
        GetTextMetricsW(hdc, &tmSpec);
        specValH = tmSpec.tmHeight;
        rowSpecH = (specLblH > specValH ? specLblH : specValH) + 4;
        if (lblH + 2 > rowSpecH)
            rowSpecH = lblH + 2;
    }
    DrawCpuSpecKvColumn(hdc, &rcMid, s_CpuStatsPaint.specMidLblBuf, s_CpuStatsPaint.specMidValBuf,
                        rowSpecH);
    DrawCpuSpecKvColumn(hdc, &rcCache, s_CpuStatsPaint.specCacheLblBuf, s_CpuStatsPaint.specCacheValBuf,
                        rowSpecH);
}

static void
Tm8CpuStatsUpdateScrollInfo(HWND hwnd)
{
    RECT rc;
    SCROLLINFO si;
    int clientW;
    int page;
    BOOL needScroll;

    if (!hwnd || !IsWindow(hwnd))
        return;
    GetClientRect(hwnd, &rc);
    clientW = rc.right - rc.left;
    if (clientW < 0)
        clientW = 0;

    s_CpuStatsContentW = TM8_CPU_STATS_MIN_INNER_W;
    if (s_CpuStatsContentW < clientW)
        s_CpuStatsContentW = clientW;

    needScroll = (s_CpuStatsContentW > clientW);

    if (!needScroll)
        s_CpuStatsHScrollPos = 0;
    else
    {
        int maxPos = s_CpuStatsContentW - clientW;
        if (s_CpuStatsHScrollPos > maxPos)
            s_CpuStatsHScrollPos = maxPos;
        if (s_CpuStatsHScrollPos < 0)
            s_CpuStatsHScrollPos = 0;
    }

    page = clientW > 0 ? clientW : 1;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = (s_CpuStatsContentW > 1) ? (s_CpuStatsContentW - 1) : 0;
    si.nPage = (UINT)page;
    si.nPos = s_CpuStatsHScrollPos;
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
    ShowScrollBar(hwnd, SB_HORZ, needScroll);
}

static LRESULT CALLBACK
CpuStatsPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        s_CpuStatsHScrollPos = 0;
        s_CpuStatsContentW = TM8_CPU_STATS_MIN_INNER_W;
        return 0;

    case WM_SIZE:
        Tm8CpuStatsUpdateScrollInfo(hwnd);
        return 0;

    case WM_HSCROLL:
    {
        RECT rc;
        int clientW, maxPos, pos = s_CpuStatsHScrollPos;
        SCROLLINFO si;

        GetClientRect(hwnd, &rc);
        clientW = rc.right - rc.left;
        maxPos = (s_CpuStatsContentW > clientW) ? (s_CpuStatsContentW - clientW) : 0;

        switch (LOWORD(wParam))
        {
        case SB_LINELEFT:
            pos -= 24;
            break;
        case SB_LINERIGHT:
            pos += 24;
            break;
        case SB_PAGELEFT:
            pos -= clientW > 0 ? clientW : 1;
            break;
        case SB_PAGERIGHT:
            pos += clientW > 0 ? clientW : 1;
            break;
        case SB_THUMBTRACK:
            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;
            if (GetScrollInfo(hwnd, SB_HORZ, &si))
                pos = si.nTrackPos;
            break;
        case SB_THUMBPOSITION:
            pos = (short)HIWORD(wParam);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        if (pos < 0)
            pos = 0;
        if (pos > maxPos)
            pos = maxPos;
        if (pos != s_CpuStatsHScrollPos)
        {
            s_CpuStatsHScrollPos = pos;
            Tm8CpuStatsUpdateScrollInfo(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
        if (GetKeyState(VK_SHIFT) & 0x8000)
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            RECT rc;
            int clientW, maxPos, pos = s_CpuStatsHScrollPos;
            const int step = 48;

            GetClientRect(hwnd, &rc);
            clientW = rc.right - rc.left;
            maxPos = (s_CpuStatsContentW > clientW) ? (s_CpuStatsContentW - clientW) : 0;
            if (delta > 0)
                pos -= step;
            else
                pos += step;
            if (pos < 0)
                pos = 0;
            if (pos > maxPos)
                pos = maxPos;
            if (pos != s_CpuStatsHScrollPos)
            {
                s_CpuStatsHScrollPos = pos;
                Tm8CpuStatsUpdateScrollInfo(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT rcC, rcDraw;
        int drawW, dcSave;

        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rcC);
        drawW = s_CpuStatsContentW;
        if (drawW < rcC.right)
            drawW = rcC.right;

        FillRect(hdc, &rcC, (HBRUSH)GetStockObject(WHITE_BRUSH));

        rcDraw.left = 0;
        rcDraw.top = 0;
        rcDraw.right = drawW;
        rcDraw.bottom = rcC.bottom;

        dcSave = SaveDC(hdc);
        SetViewportOrgEx(hdc, -s_CpuStatsHScrollPos, 0, NULL);
        DrawCpuPerfStatsPanel(hdc, &rcDraw);
        RestoreDC(hdc, dcSave);
        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

static BOOL
Tm8RegisterCpuStatsPanelClass(HINSTANCE hInst)
{
    WNDCLASSW wc;

    if (GetClassInfoW(hInst, L"RosTm8CpuStats", &wc))
        return TRUE;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = CpuStatsPanelProc;
    wc.cbWndExtra = 0;
    wc.cbClsExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"RosTm8CpuStats";
    if (!RegisterClassW(&wc))
        return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    return TRUE;
}

static void
DrawCpuStatsValueColumn(HDC hdc, const RECT *rcBox, HWND hwndVal)
{
    WCHAR buf[1024];
    const WCHAR *line, *p;
    RECT rc;
    HFONT hf, oldf;
    TEXTMETRICW tm;
    int lineH;
    int lineIdx = 0;

    buf[0] = 0;
    if (!GetWindowTextW(hwndVal, buf, _countof(buf)))
        buf[0] = 0;
    hf = (HFONT)SendMessageW(hwndVal, WM_GETFONT, 0, 0);
    if (!hf)
        hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    oldf = (HFONT)SelectObject(hdc, hf);
    GetTextMetricsW(hdc, &tm);
    lineH = tm.tmHeight + tm.tmExternalLeading;
    if (lineH < 1)
        lineH = 18;

    FillRect(hdc, rcBox, (HBRUSH)GetStockObject(WHITE_BRUSH));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(32, 32, 32));

    rc.left = rcBox->left;
    rc.right = rcBox->right;
    rc.top = rcBox->top;
    rc.bottom = rc.top + lineH;
    line = buf;

    while (*line && rc.top < rcBox->bottom)
    {
        p = line;
        while (*p && *p != L'\r' && *p != L'\n')
            p++;
        {
            WCHAR tmp[160];
            size_t n = (size_t)(p - line);
            HFONT hfLine = hf;
            if (hwndVal == s_hwndCpuLiveVal && lineIdx < 2 && s_hFontCpuHero)
                hfLine = s_hFontCpuHero;
            SelectObject(hdc, hfLine);
            GetTextMetricsW(hdc, &tm);
            lineH = tm.tmHeight + tm.tmExternalLeading;
            if (lineH < 1)
                lineH = 18;
            rc.bottom = rc.top + lineH;
            if (n >= _countof(tmp))
                n = _countof(tmp) - 1;
            CopyMemory(tmp, line, n * sizeof(WCHAR));
            tmp[n] = 0;
            DrawTextW(hdc, tmp, -1, &rc,
                      DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS);
        }
        if (*p == L'\r' && p[1] == L'\n')
            line = p + 2;
        else if (*p == L'\n' || *p == L'\r')
            line = p + 1;
        else
            break;
        rc.top = rc.bottom;
        lineIdx++;
    }
    SelectObject(hdc, oldf);
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        WCHAR cap[64];

        s_hwndMain = hwnd;
        {
            INITCOMMONCONTROLSEX icc;
            icc.dwSize = sizeof(icc);
            icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS |
                        ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
            InitCommonControlsEx(&icc);
        }

        LoadStr(IDS_APP_TITLE, cap, _countof(cap));

        s_brNavColumn = CreateSolidBrush(COL_NAV_BG);

        s_hwndStatus = CreateStatusWindowW(WS_CHILD | WS_VISIBLE, L"", hwnd, 100);
        SendMessageW(s_hwndStatus, SB_SETTEXT, 0, (LPARAM)L"");

        s_hwndMainTab = CreateWindowExW(0, WC_TABCONTROLW, L"",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS |
                                        TCS_FOCUSNEVER,
                                        0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MAIN_TAB,
                                        s_hInst, NULL);
        if (s_hwndMainTab)
        {
            SendMessageW(s_hwndMainTab, CCM_SETVERSION, 6, 0);
        }

        s_hwndNav = CreateWindowExW(0, L"ListBox", NULL,
                                    WS_CHILD | WS_VISIBLE | LBS_NOTIFY |
                                    LBS_OWNERDRAWVARIABLE | LBS_HASSTRINGS |
                                    LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP,
                                    0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_NAV,
                                    s_hInst, NULL);

        s_hwndNavSep = CreateWindowW(L"Static", NULL,
                                     WS_CHILD | SS_ETCHEDVERT,
                                     0, 0, 2, 10, hwnd, (HMENU)(UINT_PTR)IDC_NAV_SEP,
                                     s_hInst, NULL);

        s_hwndList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, NULL,
                                     WS_CHILD | WS_VISIBLE | LVS_REPORT |
                                     LVS_SHOWSELALWAYS | LVS_SINGLESEL |
                                     WS_TABSTOP,
                                     0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_PROCLIST,
                                     s_hInst, NULL);
        ListView_SetExtendedListViewStyle(s_hwndList,
                                          LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        s_pfnOldListView =
            (WNDPROC)SetWindowLongPtrW(s_hwndList, GWLP_WNDPROC, (LONG_PTR)ListViewSubclassProc);

        {
            WCHAR et[48], fd[48];
            LoadStr(IDS_ENDTASK, et, _countof(et));
            LoadStr(IDS_FEWER_DETAILS, fd, _countof(fd));
            s_hwndEndTask =
                CreateWindowW(L"Button", et,
                              WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | WS_VISIBLE,
                              0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_ENDTASK_BTN, s_hInst, NULL);
            s_hwndFewerDetails =
                CreateWindowW(L"Static", fd,
                              WS_CHILD | SS_LEFT | SS_NOTIFY | WS_VISIBLE,
                              0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_FEWER_DETAILS, s_hInst, NULL);
            EnableWindow(s_hwndEndTask, FALSE);
        }

        s_hwndCpuLbl = CreateWindowW(L"Static", L"", WS_CHILD, 0, 0, 10, 10,
                                     hwnd, (HMENU)(UINT_PTR)IDC_CPU_LABEL, s_hInst, NULL);
        s_hwndCpuBar = CreateWindowW(PROGRESS_CLASSW, NULL,
                                     WS_CHILD | PBS_SMOOTH, 0, 0, 10, 10,
                                     hwnd, (HMENU)(UINT_PTR)IDC_CPU_BAR, s_hInst, NULL);
        SendMessageW(s_hwndCpuBar, CCM_SETVERSION, 6, 0);
        SendMessageW(s_hwndCpuBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(s_hwndCpuBar, PBM_SETBARCOLOR, 0, COL_BAR_GREEN);
        SendMessageW(s_hwndCpuBar, PBM_SETBKCOLOR, 0, COL_BAR_TRACK);

        s_hwndMemLbl = CreateWindowW(L"Static", L"", WS_CHILD, 0, 0, 10, 10,
                                     hwnd, (HMENU)(UINT_PTR)IDC_MEM_LABEL, s_hInst, NULL);
        s_hwndMemBar = CreateWindowW(PROGRESS_CLASSW, NULL,
                                     WS_CHILD | PBS_SMOOTH, 0, 0, 10, 10,
                                     hwnd, (HMENU)(UINT_PTR)IDC_MEM_BAR, s_hInst, NULL);
        SendMessageW(s_hwndMemBar, CCM_SETVERSION, 6, 0);
        SendMessageW(s_hwndMemBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(s_hwndMemBar, PBM_SETBARCOLOR, 0, COL_BAR_GREEN);
        SendMessageW(s_hwndMemBar, PBM_SETBKCOLOR, 0, COL_BAR_TRACK);

        s_hwndSpeed = CreateWindowW(L"Static", L"", WS_CHILD, 0, 0, 10, 10,
                                    hwnd, (HMENU)(UINT_PTR)IDC_SPEED_LABEL, s_hInst, NULL);

        s_hwndMemDetails = CreateWindowW(L"Static", L"",
                                         WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                         0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MEM_DETAILS,
                                         s_hInst, NULL);

        s_hwndCpuTitle = CreateWindowW(L"Static", L"",
                                       WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                       0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_TITLE,
                                       s_hInst, NULL);
        s_hwndCpuModel = CreateWindowW(L"Static", L"",
                                       WS_CHILD | SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS,
                                       0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_MODEL,
                                       s_hInst, NULL);
        s_hwndCpuGraphSub = CreateWindowW(L"Static", L"",
                                          WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                          0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_GRAPH_SUB,
                                          s_hInst, NULL);
        s_hwndCpuLiveLbl = CreateWindowW(L"Static", L"",
                                        WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                        0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_LIVE_LBL,
                                        s_hInst, NULL);
        s_hwndCpuLiveVal = CreateWindowW(L"Static", L"",
                                         WS_CHILD | SS_OWNERDRAW | SS_NOPREFIX,
                                         0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_LIVE_VAL,
                                         s_hInst, NULL);
        s_hwndCpuStatSep = CreateWindowW(L"Static", L"",
                                         WS_CHILD | SS_ETCHEDVERT,
                                         0, 0, 2, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_STAT_SEP,
                                         s_hInst, NULL);
        s_hwndCpuSpecLbl = CreateWindowW(L"Static", L"",
                                         WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                         0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_SPEC_LBL,
                                         s_hInst, NULL);
        s_hwndCpuSpecVal = CreateWindowW(L"Static", L"",
                                         WS_CHILD | SS_OWNERDRAW | SS_NOPREFIX,
                                         0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_SPEC_VAL,
                                         s_hInst, NULL);
        Tm8RegisterCpuStatsPanelClass(s_hInst);
        s_hwndCpuStatsPanel =
            CreateWindowW(L"RosTm8CpuStats", L"", WS_CHILD | WS_HSCROLL, 0, 0, 10, 10, hwnd,
                          (HMENU)(UINT_PTR)IDC_CPU_STATS_PANEL, s_hInst, NULL);
        s_hwndMemTitle = CreateWindowW(L"Static", L"",
                                       WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                       0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MEM_TITLE,
                                       s_hInst, NULL);
        s_hwndMemModel = CreateWindowW(L"Static", L"",
                                       WS_CHILD | SS_RIGHT | SS_NOPREFIX | SS_ENDELLIPSIS,
                                       0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MEM_MODEL,
                                       s_hInst, NULL);
        s_hwndMemGraphSub = CreateWindowW(L"Static", L"",
                                          WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                          0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MEM_GRAPH_SUB,
                                          s_hInst, NULL);
        s_hwndMemCompSub = CreateWindowW(L"Static", L"",
                                         WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                         0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MEM_COMP_SUB,
                                         s_hInst, NULL);
        s_hwndMemStatsPanel = CreateWindowW(L"Static", L"",
                                            WS_CHILD | SS_OWNERDRAW | SS_NOPREFIX,
                                            0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MEM_STATS_PANEL,
                                            s_hInst, NULL);

        s_hwndStub = CreateWindowW(L"Static", L"",
                                   WS_CHILD | SS_CENTER | SS_NOPREFIX,
                                   0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_STUB,
                                   s_hInst, NULL);
        {
            WCHAR st[256];
            LoadStr(IDS_STUB_BODY, st, _countof(st));
            SetWindowTextW(s_hwndStub, st);
        }

        s_hwndGraphCpu = CreateWindowW(L"Static", L"",
                                       WS_CHILD | WS_VISIBLE | WS_BORDER,
                                       0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_CPU_GRAPH,
                                       s_hInst, NULL);
        s_pfnOldGraph = (WNDPROC)SetWindowLongPtrW(s_hwndGraphCpu, GWLP_WNDPROC,
                                                   (LONG_PTR)GraphWndProc);

        s_hwndGraphMem = CreateWindowW(L"Static", L"",
                                       WS_CHILD | WS_VISIBLE | WS_BORDER,
                                       0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MEM_GRAPH,
                                       s_hInst, NULL);
        SetWindowLongPtrW(s_hwndGraphMem, GWLP_WNDPROC, (LONG_PTR)GraphWndProc);

        s_hwndGraphMemComp = CreateWindowW(L"Static", L"",
                                           WS_CHILD | WS_VISIBLE | WS_BORDER,
                                           0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_MEM_COMP_GRAPH,
                                           s_hInst, NULL);
        SetWindowLongPtrW(s_hwndGraphMemComp, GWLP_WNDPROC, (LONG_PTR)GraphWndProc);

        s_hProcMenuRoot = LoadMenuW(s_hInst, MAKEINTRESOURCEW(IDR_PROC_MENU));
        s_hCtxMenu = s_hProcMenuRoot ? GetSubMenu(s_hProcMenuRoot, 0) : NULL;

        InitMainTabs();
        FillNavList();
        SetupListColumns();
        Tm8ProcEnsureImageList();
        if (s_hProcSmIl)
            ListView_SetImageList(s_hwndList, s_hProcSmIl, LVSIL_SMALL);
        ReadCpuModelString();
        s_NominalCpuMhz = ReadNominalCpuMhz();
        InitPerfApis();
        InitMemoryExtraApis();
        s_NumLogicalCpus = Tm8GetLogicalCpuCount();
        ApplyPerfTypography();
        ShowPage(PAGE_PROCESSES);
        LayoutChildren(hwnd);
        if (s_hwndMainTab)
        {
            InvalidateRect(s_hwndMainTab, NULL, TRUE);
            UpdateWindow(s_hwndMainTab);
        }

        SetTimer(hwnd, TIMER_ID, TIMER_MS, NULL);
        OnTimer();
        return 0;
    }

    case WM_INITMENUPOPUP:
    {
        HMENU hmPop = (HMENU)wParam;
        HMENU hmBar = GetMenu(hwnd);
        if (hmBar && hmPop == GetSubMenu(hmBar, 0))
        {
            BOOL onProc = s_hwndMainTab && TabCtrl_GetCurSel(s_hwndMainTab) == TAB_MAIN_PROCESSES &&
                          s_iPage == PAGE_PROCESSES;
            BOOL canEnd = onProc && GetSelectedPid() != 0;
            EnableMenuItem(hmPop, IDM_ENDTASK, MF_BYCOMMAND | (canEnd ? MF_ENABLED : MF_GRAYED));
        }
        if (hmBar && hmPop == GetSubMenu(hmBar, 1))
        {
            CheckMenuItem(hmPop, ID_OPTIONS_TOPMOST,
                          MF_BYCOMMAND |
                          ((GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) ? MF_CHECKED
                                                                                  : MF_UNCHECKED));
        }
        if (hmBar)
        {
            HMENU hmView = GetSubMenu(hmBar, 2);
            HMENU hmCpuG = hmView ? GetSubMenu(hmView, 1) : NULL;
            if (hmCpuG && hmPop == hmCpuG)
            {
                CheckMenuItem(hmPop, ID_VIEW_CPU_GRAPH_OVERALL,
                              MF_BYCOMMAND | (s_CpuGraphPerLogical ? MF_UNCHECKED : MF_CHECKED));
                CheckMenuItem(hmPop, ID_VIEW_CPU_GRAPH_PERCPU,
                              MF_BYCOMMAND | (s_CpuGraphPerLogical ? MF_CHECKED : MF_UNCHECKED));
            }
        }
        return 0;
    }

    case WM_CONTEXTMENU:
    {
        HWND hwFrom = (HWND)wParam;
        if (hwFrom == s_hwndGraphCpu && s_iPage == PAGE_CPU && s_hwndMainTab &&
            TabCtrl_GetCurSel(s_hwndMainTab) == TAB_MAIN_PERF)
        {
            ShowCpuGraphModeContextMenu(hwnd, s_hwndGraphCpu, lParam);
            return 0;
        }
        break;
    }

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
            LayoutChildren(hwnd);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID)
            OnTimer();
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_FILE_EXIT)
        {
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == ID_VIEW_REFRESH)
        {
            OnTimer();
            return 0;
        }
        if (LOWORD(wParam) == ID_VIEW_CPU_GRAPH_OVERALL)
        {
            s_CpuGraphPerLogical = FALSE;
            if (s_hwndGraphCpu)
                InvalidateRect(s_hwndGraphCpu, NULL, FALSE);
            if (s_iPage == PAGE_CPU)
                RefreshCpuDetailUi();
            return 0;
        }
        if (LOWORD(wParam) == ID_VIEW_CPU_GRAPH_PERCPU)
        {
            s_CpuGraphPerLogical = TRUE;
            if (s_hwndGraphCpu)
                InvalidateRect(s_hwndGraphCpu, NULL, FALSE);
            if (s_iPage == PAGE_CPU)
                RefreshCpuDetailUi();
            return 0;
        }
        if (LOWORD(wParam) == ID_OPTIONS_TOPMOST)
        {
            HMENU hm;
            BOOL onTop;
            onTop = (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
            SetWindowPos(hwnd, onTop ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            hm = GetMenu(hwnd);
            if (hm)
            {
                CheckMenuItem(GetSubMenu(hm, 1), ID_OPTIONS_TOPMOST,
                              MF_BYCOMMAND |
                              ((GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
                                   ? MF_CHECKED
                                   : MF_UNCHECKED));
            }
            return 0;
        }
        if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_NAV)
        {
            int sel;
            if (!s_hwndMainTab || TabCtrl_GetCurSel(s_hwndMainTab) != TAB_MAIN_PERF)
                return 0;
            sel = (int)SendMessageW(s_hwndNav, LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel <= 1)
                s_iPerfNavSel = sel;
            if (sel == 0)
                ShowPage(PAGE_CPU);
            else if (sel == 1)
                ShowPage(PAGE_MEMORY);
            InvalidateRect(s_hwndNav, NULL, TRUE);
            return 0;
        }
        if (LOWORD(wParam) == IDM_ENDTASK)
        {
            EndSelectedTask(hwnd);
            return 0;
        }
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_ENDTASK_BTN)
        {
            EndSelectedTask(hwnd);
            return 0;
        }
        break;

    case WM_MEASUREITEM:
    {
        LPMEASUREITEMSTRUCT mis = (LPMEASUREITEMSTRUCT)lParam;
        if (mis->CtlID == IDC_NAV)
        {
            mis->itemHeight = (mis->itemID == 0) ? NAV_ITEM_H_CPU : NAV_ITEM_H_MEM;
            return TRUE;
        }
        break;
    }

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID == IDC_MEM_STATS_PANEL)
        {
            DrawMemPerfStatsPanel(dis->hDC, &dis->rcItem);
            return TRUE;
        }
        if (dis->CtlID == IDC_CPU_LIVE_VAL || dis->CtlID == IDC_CPU_SPEC_VAL)
        {
            DrawCpuStatsValueColumn(dis->hDC, &dis->rcItem, dis->hwndItem);
            return TRUE;
        }
        if (dis->CtlID == IDC_NAV && dis->itemID != (UINT)-1)
        {
            WCHAR text[160], metaPct[24], metaSpd[24], metaLine[96];
            int n;
            BOOL isSel = ((int)SendMessageW(s_hwndNav, LB_GETCURSEL, 0, 0) == (int)dis->itemID);
            HBRUSH brGutter, brTile;
            RECT rcTile, rcMeta, rcName;
            HPEN penFrame, oldPen;
            HBRUSH oldBr;
            COLORREF fg = COL_NAV_TEXT;
            COLORREF borderCol = isSel ? RGB(210, 214, 220) : NAV_TILE_BORDER;
            TEXTMETRICW tm;

            n = (int)SendMessageW(s_hwndNav, LB_GETTEXTLEN, dis->itemID, 0);
            if (n < 0)
                n = 0;
            if (n >= (int)_countof(text))
                n = (int)_countof(text) - 1;
            SendMessageW(s_hwndNav, LB_GETTEXT, dis->itemID, (LPARAM)text);

            brGutter = CreateSolidBrush(COL_NAV_BG);
            FillRect(dis->hDC, &dis->rcItem, brGutter);
            DeleteObject(brGutter);

            rcTile.left = dis->rcItem.left + NAV_TILE_PAD_X;
            rcTile.right = dis->rcItem.right - NAV_TILE_PAD_X;
            if (dis->itemID == 0)
            {
                rcTile.top = dis->rcItem.top + NAV_TILE_TOP;
                rcTile.bottom = rcTile.top + NAV_TILE_CPU_H;
            }
            else
            {
                rcTile.top = dis->rcItem.top;
                rcTile.bottom = rcTile.top + NAV_TILE_MEM_H;
            }

            brTile = CreateSolidBrush(isSel ? RGB(248, 249, 251) : RGB(255, 255, 255));
            FillRect(dis->hDC, &rcTile, brTile);
            DeleteObject(brTile);

            penFrame = CreatePen(PS_SOLID, 1, borderCol);
            oldPen = (HPEN)SelectObject(dis->hDC, penFrame);
            oldBr = (HBRUSH)SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
            Rectangle(dis->hDC, rcTile.left, rcTile.top, rcTile.right, rcTile.bottom);
            SelectObject(dis->hDC, oldBr);
            SelectObject(dis->hDC, oldPen);
            DeleteObject(penFrame);

            /* Accent after frame so the border does not cover it (Win10/11 nav strip). */
            if (isSel)
            {
                RECT rcBar;
                HBRUSH brBar;

                rcBar.left = rcTile.left;
                rcBar.top = rcTile.top + 1;
                rcBar.right = rcTile.left + 4;
                rcBar.bottom = rcTile.bottom - 1;
                brBar = CreateSolidBrush(COL_NAV_SEL);
                FillRect(dis->hDC, &rcBar, brBar);
                DeleteObject(brBar);
            }

            {
                RECT rcSpark;
                int tileH = rcTile.bottom - rcTile.top;
                rcSpark.left = rcTile.left + 6;
                rcSpark.right = rcSpark.left + NAV_SPARK_BOX;
                rcSpark.top = rcTile.top + (tileH - NAV_SPARK_BOX) / 2;
                if (rcSpark.top < rcTile.top + 4)
                    rcSpark.top = rcTile.top + 4;
                rcSpark.bottom = rcSpark.top + NAV_SPARK_BOX;
                if (dis->itemID == 0)
                {
                    NavDrawMiniSpark(dis->hDC, &rcSpark, s_CpuHist, CPU_HIST_LEN, s_CpuHistPos,
                                     COL_GRAPH_FILL, COL_GRAPH_LINE, FALSE);
                }
                else if (dis->itemID == 1)
                {
                    NavDrawMiniSpark(dis->hDC, &rcSpark, s_MemHist, CPU_HIST_LEN, s_MemHistPos,
                                     COL_MEM_GRAPH_FILL, COL_MEM_GRAPH_LINE, FALSE);
                }
            }

            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, fg);

            {
                int textL = rcTile.left + 6 + NAV_SPARK_BOX + NAV_SPARK_GAP;
                int textR = rcTile.right - 6;
                int nameH, metaH;

                if (dis->itemID == 0)
                {
                    SelectObject(dis->hDC, s_hFontNavBold ? s_hFontNavBold : s_hFontPerf);
                    GetTextMetricsW(dis->hDC, &tm);
                    nameH = tm.tmHeight + 1;

                    rcName.left = textL;
                    rcName.right = textR;
                    rcName.top = rcTile.top + 6;
                    rcName.bottom = rcName.top + nameH;
                    DrawTextW(dis->hDC, text, -1, &rcName,
                              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

                    SelectObject(dis->hDC, s_hFontNavMeta ? s_hFontNavMeta : s_hFontPerf);
                    SetTextColor(dis->hDC, RGB(96, 96, 96));
                    GetTextMetricsW(dis->hDC, &tm);
                    metaH = tm.tmHeight + 2;
                    {
                        DWORD mhzNav = s_CurrentCpuMhzLive ? s_CurrentCpuMhzLive : s_NominalCpuMhz;
                        StringCchPrintfW(metaPct, _countof(metaPct), L"%d%%", s_LastCpuPct);
                        FormatSpeedFromMhz(mhzNav, metaSpd, _countof(metaSpd));
                        if (metaSpd[0])
                            StringCchPrintfW(metaLine, _countof(metaLine), L"%ls   %ls", metaPct,
                                             metaSpd);
                        else
                            StringCchCopyW(metaLine, _countof(metaLine), metaPct);
                    }
                    rcMeta.left = textL;
                    rcMeta.right = textR;
                    rcMeta.top = rcName.bottom + 3;
                    rcMeta.bottom = rcMeta.top + metaH;
                    if (rcMeta.bottom > rcTile.bottom - 5)
                        rcMeta.bottom = rcTile.bottom - 5;
                    DrawTextW(dis->hDC, metaLine, -1, &rcMeta,
                              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
                }
                else
                {
                    WCHAR memSub[120];
                    double uGb, tGb;

                    SelectObject(dis->hDC, s_hFontNavBold ? s_hFontNavBold : s_hFontPerf);
                    SetTextColor(dis->hDC, fg);
                    GetTextMetricsW(dis->hDC, &tm);
                    nameH = tm.tmHeight + 1;

                    rcName.left = textL;
                    rcName.right = textR;
                    rcName.top = rcTile.top + 6;
                    rcName.bottom = rcName.top + nameH;
                    DrawTextW(dis->hDC, text, -1, &rcName,
                              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

                    SelectObject(dis->hDC, s_hFontNavMeta ? s_hFontNavMeta : s_hFontPerf);
                    SetTextColor(dis->hDC, RGB(96, 96, 96));
                    GetTextMetricsW(dis->hDC, &tm);
                    metaH = tm.tmHeight + 2;

                    uGb = (double)s_MemUsedPhysNav / (1024.0 * 1024.0 * 1024.0);
                    tGb = (double)s_MemTotalPhysNav / (1024.0 * 1024.0 * 1024.0);
                    if (s_MemTotalPhysNav > 0)
                        StringCchPrintfW(memSub, _countof(memSub), L"%.1f/%.1f GB (%lu%%)", uGb, tGb,
                                         (ULONG)s_LastMemUsagePct);
                    else
                        StringCchPrintfW(memSub, _countof(memSub), L"%lu%%", (ULONG)s_LastMemUsagePct);

                    rcMeta.left = textL;
                    rcMeta.right = textR;
                    rcMeta.top = rcName.bottom + 3;
                    rcMeta.bottom = rcMeta.top + metaH;
                    if (rcMeta.bottom > rcTile.bottom - 5)
                        rcMeta.bottom = rcTile.bottom - 5;
                    DrawTextW(dis->hDC, memSub, -1, &rcMeta,
                              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
                }
            }
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORLISTBOX:
        if ((HWND)lParam == s_hwndNav && s_brNavColumn)
        {
            HDC hdcList = (HDC)wParam;
            SetBkColor(hdcList, COL_NAV_BG);
            return (INT_PTR)s_brNavColumn;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == s_hwndCpuTitle)
        {
            HDC hdcSt = (HDC)wParam;
            SetBkColor(hdcSt, RGB(255, 255, 255));
            SetTextColor(hdcSt, RGB(32, 32, 32));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
        if ((HWND)lParam == s_hwndCpuGraphSub || (HWND)lParam == s_hwndCpuLiveLbl ||
            (HWND)lParam == s_hwndCpuSpecLbl)
        {
            HDC hdcSt = (HDC)wParam;
            SetBkColor(hdcSt, RGB(255, 255, 255));
            SetTextColor(hdcSt, RGB(96, 96, 96));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
        if ((HWND)lParam == s_hwndCpuModel)
        {
            HDC hdcSt = (HDC)wParam;
            SetBkColor(hdcSt, RGB(255, 255, 255));
            SetTextColor(hdcSt, RGB(64, 64, 64));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
        if ((HWND)lParam == s_hwndCpuLbl || (HWND)lParam == s_hwndMemLbl ||
            (HWND)lParam == s_hwndSpeed || (HWND)lParam == s_hwndMemDetails ||
            (HWND)lParam == s_hwndMemTitle || (HWND)lParam == s_hwndMemModel ||
            (HWND)lParam == s_hwndMemGraphSub || (HWND)lParam == s_hwndMemCompSub ||
            (HWND)lParam == s_hwndStub)
        {
            HDC hdcSt = (HDC)wParam;
            SetBkColor(hdcSt, RGB(255, 255, 255));
            SetTextColor(hdcSt, RGB(32, 32, 32));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_NOTIFY:
    {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm->hwndFrom == s_hwndMainTab && pnm->code == TCN_SELCHANGE)
        {
            int t = (int)TabCtrl_GetCurSel(s_hwndMainTab);
            if (t == TAB_MAIN_PROCESSES)
                ShowPage(PAGE_PROCESSES);
            else if (t == TAB_MAIN_PERF)
            {
                SendMessageW(s_hwndNav, LB_SETCURSEL, s_iPerfNavSel, 0);
                ShowPage(s_iPerfNavSel == 1 ? PAGE_MEMORY : PAGE_CPU);
            }
            else
                ShowPage(PAGE_STUB);
            SyncProcEndTaskUi();
            return 0;
        }
        if (pnm->hwndFrom == s_hwndList && pnm->code == LVN_COLUMNCLICK)
        {
            if (s_hwndMainTab && TabCtrl_GetCurSel(s_hwndMainTab) == TAB_MAIN_PROCESSES &&
                s_iPage == PAGE_PROCESSES)
            {
                LPNMLISTVIEW pnl = (LPNMLISTVIEW)lParam;
                Tm8OnProcColumnClick(pnl->iSubItem);
            }
            return 0;
        }
        if (pnm->hwndFrom == s_hwndList && pnm->code == NM_CUSTOMDRAW)
        {
            LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lParam;
            int item = (int)lvcd->nmcd.dwItemSpec;
            int sub = (int)lvcd->iSubItem;

            switch (lvcd->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
                return CDRF_NOTIFYSUBITEMDRAW;
            case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                if (item >= 0 && item < s_ProcListRows && sub >= 2 && sub <= 5)
                {
                    if (sub == 2)
                        lvcd->clrTextBk = Tm8HeatBgCpu(s_ProcCpuDbl[item]);
                    else if (sub == 4)
                        lvcd->clrTextBk = Tm8HeatBgMem(s_ProcMemWs[item], s_ProcMemMax);
                    else
                        lvcd->clrTextBk = Tm8HeatBgCpu(0.0);
                    lvcd->clrText = RGB(32, 32, 32);
                }
                return CDRF_DODEFAULT;
            default:
                break;
            }
            return CDRF_DODEFAULT;
        }
        if (pnm->hwndFrom == s_hwndList && pnm->code == LVN_ITEMCHANGED)
        {
            LPNMLISTVIEW pnl = (LPNMLISTVIEW)lParam;
            if ((pnl->uChanged & LVIF_STATE) &&
                ((pnl->uNewState ^ pnl->uOldState) & LVIS_SELECTED))
                SyncProcEndTaskUi();
            return 0;
        }
        if (pnm->hwndFrom == s_hwndList)
        {
            if (pnm->code == NM_RCLICK)
            {
                LVHITTESTINFO ht;
                POINT ptCli;
                POINT ptScr;

                GetCursorPos(&ptScr);
                ptCli = ptScr;
                ScreenToClient(s_hwndList, &ptCli);
                ZeroMemory(&ht, sizeof(ht));
                ht.pt = ptCli;
                if (ListView_HitTest(s_hwndList, &ht) >= 0 && ht.iItem >= 0)
                {
                    ListView_SetItemState(s_hwndList, ht.iItem, LVIS_FOCUSED | LVIS_SELECTED,
                                          LVIS_FOCUSED | LVIS_SELECTED);
                }
                SyncProcEndTaskUi();
                if (s_hCtxMenu)
                {
                    SetForegroundWindow(hwnd);
                    TrackPopupMenu(s_hCtxMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptScr.x, ptScr.y, 0,
                                   hwnd, NULL);
                    PostMessageW(hwnd, WM_NULL, 0, 0);
                }
                return TRUE;
            }
        }
        break;
    }

    case WM_DESTROY:
        if (s_hwndList && s_pfnOldListView)
        {
            SetWindowLongPtrW(s_hwndList, GWLP_WNDPROC, (LONG_PTR)s_pfnOldListView);
            s_pfnOldListView = NULL;
        }
        KillTimer(hwnd, TIMER_ID);
        PdhCpuMhzShutdown();
        if (s_hwndList && s_hProcSmIl && IsWindow(s_hwndList))
            ListView_SetImageList(s_hwndList, NULL, LVSIL_SMALL);
        if (s_hProcSmIl)
        {
            ImageList_Destroy(s_hProcSmIl);
            s_hProcSmIl = NULL;
        }
        s_IconCacheCount = 0;
        if (s_hProcMenuRoot)
        {
            DestroyMenu(s_hProcMenuRoot);
            s_hProcMenuRoot = NULL;
            s_hCtxMenu = NULL;
        }
        if (s_brNavColumn)
        {
            DeleteObject(s_brNavColumn);
            s_brNavColumn = NULL;
        }
        if (s_hFontTitle && s_hFontTitle != s_hFontPerf)
            DeleteObject(s_hFontTitle);
        if (s_hFontNavBold && s_hFontNavBold != s_hFontPerf)
            DeleteObject(s_hFontNavBold);
        if (s_hFontNavMeta && s_hFontNavMeta != s_hFontPerf)
            DeleteObject(s_hFontNavMeta);
        if (s_hFontTab && s_hFontTab != s_hFontPerf &&
            s_hFontTab != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
            DeleteObject(s_hFontTab);
        if (s_hFontCpuLbl && s_hFontCpuLbl != s_hFontNavMeta && s_hFontCpuLbl != s_hFontPerf)
            DeleteObject(s_hFontCpuLbl);
        if (s_hFontCpuVal && s_hFontCpuVal != s_hFontTitle && s_hFontCpuVal != s_hFontPerf)
            DeleteObject(s_hFontCpuVal);
        if (s_hFontCpuHero && s_hFontCpuHero != s_hFontCpuVal && s_hFontCpuHero != s_hFontTitle &&
            s_hFontCpuHero != s_hFontPerf)
            DeleteObject(s_hFontCpuHero);
        if (s_hFontPerf && s_hFontPerf != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
            DeleteObject(s_hFontPerf);
        s_hFontCpuLbl = NULL;
        s_hFontCpuVal = NULL;
        s_hFontCpuHero = NULL;
        s_hFontPerf = NULL;
        s_hFontTitle = NULL;
        s_hFontNavBold = NULL;
        s_hFontNavMeta = NULL;
        s_hFontTab = NULL;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void
DrawMemoryUsageHistoryGraph(HDC mem, const RECT *rc, const BYTE *hist, int histLen, int histWritePos,
                             COLORREF fillCol, COLORREF lineCol, int lineWidth)
{
    int w = rc->right - rc->left;
    int h = rc->bottom - rc->top;
    int pad, baseY, topMargin, innerW, i, denom;
    POINT poly[CPU_HIST_LEN + 2];
    POINT linePts[CPU_HIST_LEN];
    HPEN nullPen, linePen, oldPen;
    HBRUSH fillBr, oldBr;
    HPEN gridPen, oldGp;
    int gi;

    if (w < 6 || h < 6 || !hist || histLen < 2)
        return;

    pad = (w < 48) ? 3 : 8;
    if (w <= pad * 2 || h <= pad * 2)
        return;

    baseY = rc->bottom - pad;
    topMargin = rc->top + pad + 4;
    if (topMargin >= baseY - 4)
        topMargin = rc->top + pad + 2;
    innerW = w - 2 * pad;
    denom = (histLen > 1) ? (histLen - 1) : 1;

    gridPen = CreatePen(PS_SOLID, 1, COL_MEM_GRAPH_GRID);
    oldGp = (HPEN)SelectObject(mem, gridPen);
    for (gi = 1; gi < 4; gi++)
    {
        int yy = topMargin + (gi * (baseY - topMargin)) / 4;
        MoveToEx(mem, rc->left + pad, yy, NULL);
        LineTo(mem, rc->right - pad, yy);
    }
    for (gi = 1; gi < 8; gi++)
    {
        int xx = rc->left + pad + (gi * innerW) / 8;
        MoveToEx(mem, xx, topMargin, NULL);
        LineTo(mem, xx, baseY);
    }
    SelectObject(mem, oldGp);
    DeleteObject(gridPen);

    poly[0].x = rc->left + pad;
    poly[0].y = baseY;
    for (i = 0; i < histLen; i++)
    {
        int age = histLen - 1 - i;
        int idxb = (histWritePos - 1 - age + histLen * 64) % histLen;
        int v = hist[idxb];
        int y = baseY - (v * (baseY - topMargin)) / 100;
        if (y < topMargin)
            y = topMargin;
        linePts[i].x = rc->left + pad + (i * (innerW - 1)) / denom;
        linePts[i].y = y;
        poly[i + 1].x = linePts[i].x;
        poly[i + 1].y = linePts[i].y;
    }
    poly[histLen + 1].x = rc->left + pad + innerW - 1;
    poly[histLen + 1].y = baseY;

    nullPen = CreatePen(PS_NULL, 0, 0);
    oldPen = (HPEN)SelectObject(mem, nullPen);
    fillBr = CreateSolidBrush(fillCol);
    oldBr = (HBRUSH)SelectObject(mem, fillBr);
    SetPolyFillMode(mem, WINDING);
    Polygon(mem, poly, histLen + 2);
    SelectObject(mem, oldBr);
    DeleteObject(fillBr);
    SelectObject(mem, oldPen);
    DeleteObject(nullPen);

    /* Subtle top edge like Win10/11 (fill is primary; avoid heavy outline). */
    linePen = CreatePen(PS_SOLID, 1, COL_MEM_GRAPH_EDGE);
    oldPen = (HPEN)SelectObject(mem, linePen);
    Polyline(mem, linePts, histLen);
    SelectObject(mem, oldPen);
    DeleteObject(linePen);
    (void)lineWidth;
    (void)lineCol;

    {
        WCHAR g60[96], gzero[8];
        HFONT hfScale = s_hFontNavMeta ? s_hFontNavMeta : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT oldF = (HFONT)SelectObject(mem, hfScale);
        RECT rcTxt;
        SetBkMode(mem, TRANSPARENT);
        SetTextColor(mem, RGB(105, 105, 105));
        LoadStr(IDS_MEM_GRAPH_60S, g60, _countof(g60));
        rcTxt.left = rc->left + pad;
        rcTxt.right = rc->left + pad + (innerW * 2) / 5;
        rcTxt.top = baseY + 1;
        rcTxt.bottom = rc->bottom - 1;
        DrawTextW(mem, g60, -1, &rcTxt, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);

        StringCchCopyW(gzero, _countof(gzero), L"0");
        rcTxt.left = rc->right - pad - 40;
        rcTxt.right = rc->right - pad;
        rcTxt.top = baseY + 1;
        rcTxt.bottom = rc->bottom - 1;
        DrawTextW(mem, gzero, -1, &rcTxt, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);

        rcTxt.left = rc->right - pad - 200;
        rcTxt.right = rc->right - pad;
        rcTxt.top = rc->top + 1;
        rcTxt.bottom = rc->top + 16;
        DrawTextW(mem, s_szMemGraphYMax, -1, &rcTxt, DT_TOP | DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        SelectObject(mem, oldF);
    }
}

static void
DrawMemCompositionBar(HDC hdc, const RECT *rc)
{
    const int pad = 8;
    RECT inner = *rc;
    HBRUSH bg = CreateSolidBrush(COL_GRAPH_BG);
    HPEN frame, oldPen, innerFrame;
    HBRUSH oldBr;
    int wbar, wu, wf;
    COLORREF colUse = RGB(138, 194, 244);
    COLORREF colAvail = RGB(255, 255, 255);

    FillRect(hdc, rc, bg);
    DeleteObject(bg);

    InflateRect(&inner, -pad, -10);
    if (inner.right <= inner.left + 2 || inner.bottom <= inner.top + 2)
        return;

    frame = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
    oldPen = (HPEN)SelectObject(hdc, frame);
    oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, inner.left, inner.top, inner.right, inner.bottom);
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPen);
    DeleteObject(frame);

    InflateRect(&inner, -1, -1);
    wbar = inner.right - inner.left;
    if (wbar < 1)
        return;

    wu = (int)(s_MemCompFrac[0] * wbar + 0.5);
    if (wu > wbar)
        wu = wbar;
    wf = wbar - wu;

    if (wu > 0)
    {
        RECT r = inner;
        HBRUSH b = CreateSolidBrush(colUse);
        r.right = r.left + wu;
        FillRect(hdc, &r, b);
        DeleteObject(b);
    }
    if (wf > 0)
    {
        RECT r = inner;
        HBRUSH b = CreateSolidBrush(colAvail);
        r.left = inner.left + wu;
        r.right = inner.right;
        FillRect(hdc, &r, b);
        DeleteObject(b);
        innerFrame = CreatePen(PS_SOLID, 1, RGB(200, 220, 245));
        oldPen = (HPEN)SelectObject(hdc, innerFrame);
        MoveToEx(hdc, r.left, r.top, NULL);
        LineTo(hdc, r.left, r.bottom - 1);
        SelectObject(hdc, oldPen);
        DeleteObject(innerFrame);
    }
}

static void
DrawMemPerfStatsPanel(HDC hdc, const RECT *rcPanel)
{
    int w = rcPanel->right - rcPanel->left;
    int pad = 8;
    int gapCol = 14;
    int gapRow = 8;
    int colW, c0, c1, c2;
    int y, lblH, midH;
    HFONT oldF;
    TEXTMETRICW tm;
    RECT rL, rV;
    TM8_MEM_STATS_PAINT *ps = &s_MemStatsPaint;

    if (w < 120 || rcPanel->bottom <= rcPanel->top + 8)
        return;

    FillRect(hdc, rcPanel, (HBRUSH)GetStockObject(WHITE_BRUSH));
    SetBkMode(hdc, TRANSPARENT);

    colW = (w - 2 * pad - 2 * gapCol) / 3;
    if (colW < 80)
        colW = (w - 2 * pad) / 3;
    c0 = rcPanel->left + pad;
    c1 = c0 + colW + gapCol;
    c2 = c1 + colW + gapCol;

    oldF = (HFONT)SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    GetTextMetricsW(hdc, &tm);
    lblH = tm.tmHeight + 1;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    GetTextMetricsW(hdc, &tm);
    midH = tm.tmHeight + 2;

    y = rcPanel->top + 6;
    rL.left = c0;
    rL.right = c0 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c1Lbl1, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.left = c0;
    rV.right = c0 + colW;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c1Val1, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c1Lbl2, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c1Val2, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c1Lbl3, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c1Val3, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    y = rcPanel->top + 6;
    rL.left = c1;
    rL.right = c1 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c2Lbl1, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.left = c1;
    rV.right = c1 + colW;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c2Val1, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.left = c1;
    rL.right = c1 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c2Lbl2, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c2Val2, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.left = c1;
    rL.right = c1 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c2Lbl3, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c2Val3, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    y = rcPanel->top + 6;
    rL.left = c2;
    rL.right = c2 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c3Lbl1, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.left = c2;
    rV.right = c2 + colW;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c3Val1, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c3Lbl2, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c3Val2, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c3Lbl3, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c3Val3, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, ps->c3Lbl4, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, ps->c3Val4, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    SelectObject(hdc, oldF);
}

static void
DrawPerfHistoryGraph(HDC mem, const RECT *rc, const BYTE *hist, int histLen, int histWritePos,
                     COLORREF fillCol, COLORREF lineCol, UINT captionLeftStrId, BOOL draw100Pct,
                     int lineWidth)
{
    int w = rc->right - rc->left;
    int h = rc->bottom - rc->top;
    int pad, baseY, topMargin, innerW, i, denom;
    BOOL drawCaptions = (captionLeftStrId != 0) || draw100Pct;
    POINT poly[CPU_HIST_LEN + 2];
    POINT linePts[CPU_HIST_LEN];
    HPEN nullPen, linePen, oldPen;
    HBRUSH fillBr, oldBr;
    HPEN gridPen, oldGp;
    int gi;

    if (w < 6 || h < 6 || !hist || histLen < 2)
        return;

    pad = (w < 48) ? 3 : 8;
    if (w <= pad * 2 || h <= pad * 2)
        return;

    baseY = rc->bottom - pad;
    topMargin = rc->top + pad + (drawCaptions ? 11 : 4);
    if (topMargin >= baseY - 4)
        topMargin = rc->top + pad + 2;
    innerW = w - 2 * pad;
    denom = (histLen > 1) ? (histLen - 1) : 1;

    gridPen = CreatePen(PS_SOLID, 1, COL_GRAPH_GRID);
    oldGp = (HPEN)SelectObject(mem, gridPen);
    for (gi = 1; gi < 4; gi++)
    {
        int yy = topMargin + (gi * (baseY - topMargin)) / 4;
        MoveToEx(mem, rc->left + pad, yy, NULL);
        LineTo(mem, rc->right - pad, yy);
    }
    for (gi = 1; gi < 8; gi++)
    {
        int xx = rc->left + pad + (gi * innerW) / 8;
        MoveToEx(mem, xx, topMargin, NULL);
        LineTo(mem, xx, baseY);
    }
    SelectObject(mem, oldGp);
    DeleteObject(gridPen);

    poly[0].x = rc->left + pad;
    poly[0].y = baseY;
    for (i = 0; i < histLen; i++)
    {
        int age = histLen - 1 - i;
        int idxb = (histWritePos - 1 - age + histLen * 64) % histLen;
        int v = hist[idxb];
        int y = baseY - (v * (baseY - topMargin)) / 100;
        if (y < topMargin)
            y = topMargin;
        linePts[i].x = rc->left + pad + (i * (innerW - 1)) / denom;
        linePts[i].y = y;
        poly[i + 1].x = linePts[i].x;
        poly[i + 1].y = linePts[i].y;
    }
    poly[histLen + 1].x = rc->left + pad + innerW - 1;
    poly[histLen + 1].y = baseY;

    nullPen = CreatePen(PS_NULL, 0, 0);
    oldPen = (HPEN)SelectObject(mem, nullPen);
    fillBr = CreateSolidBrush(fillCol);
    oldBr = (HBRUSH)SelectObject(mem, fillBr);
    SetPolyFillMode(mem, WINDING);
    Polygon(mem, poly, histLen + 2);
    SelectObject(mem, oldBr);
    DeleteObject(fillBr);
    SelectObject(mem, oldPen);
    DeleteObject(nullPen);

    linePen = CreatePen(PS_SOLID, lineWidth, lineCol);
    oldPen = (HPEN)SelectObject(mem, linePen);
    Polyline(mem, linePts, histLen);
    SelectObject(mem, oldPen);
    DeleteObject(linePen);

    if (captionLeftStrId != 0)
    {
        WCHAR gtxt[112];
        HFONT hfCap = s_hFontNavMeta ? s_hFontNavMeta : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT oldF = (HFONT)SelectObject(mem, hfCap);
        int capLen;
        SetBkMode(mem, TRANSPARENT);
        SetTextColor(mem, RGB(105, 105, 105));
        LoadStr(captionLeftStrId, gtxt, _countof(gtxt));
        capLen = lstrlenW(gtxt);
        TextOutW(mem, rc->left + pad, rc->top + 2, gtxt, capLen);
        SelectObject(mem, oldF);
    }
    if (draw100Pct)
    {
        WCHAR gtxt[112];
        HFONT hfScale = s_hFontNavMeta ? s_hFontNavMeta : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT oldF = (HFONT)SelectObject(mem, hfScale);
        RECT rcScale;
        SetBkMode(mem, TRANSPARENT);
        SetTextColor(mem, RGB(105, 105, 105));
        LoadStr(IDS_GRAPH_SCALE, gtxt, _countof(gtxt));
        rcScale.left = rc->right - pad - 44;
        rcScale.right = rc->right - pad;
        rcScale.top = rc->top + 1;
        rcScale.bottom = rc->top + 14;
        DrawTextW(mem, gtxt, -1, &rcScale, DT_TOP | DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(mem, oldF);
    }
}

/* Mini utilization graph inside rcBox (left column of perf nav tile). */
static void
NavDrawMiniSpark(HDC hdc, const RECT *rcBox, const BYTE *hist, int histLen, int writePos,
                 COLORREF fillRgb, COLORREF lineRgb, BOOL fillUnderCurve)
{
    RECT box;
    int sp = NAV_SPARK_SAMP;
    int i, iw, baseY, topM, denom, idx, v, y;
    POINT poly[64];
    POINT linePts[64];
    HPEN pen, oldPen;
    HBRUSH fillBr, oldBr;

    if (sp + 2 > 64 || histLen < 2 || !rcBox)
        return;

    box = *rcBox;
    if (box.right <= box.left + 4 || box.bottom <= box.top + 4)
        return;

    {
        HBRUSH b = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &box, b);
        DeleteObject(b);
    }
    pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    oldPen = (HPEN)SelectObject(hdc, pen);
    Rectangle(hdc, box.left, box.top, box.right, box.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    InflateRect(&box, -2, -2);
    iw = box.right - box.left;
    if (iw < 4)
        return;
    baseY = box.bottom - 1;
    topM = box.top + 1;
    denom = (sp > 1) ? (sp - 1) : 1;

    poly[0].x = box.left;
    poly[0].y = baseY;
    for (i = 0; i < sp; i++)
    {
        int age = sp - 1 - i;
        int x;
        idx = (writePos - 1 - age + histLen * 64) % histLen;
        v = hist[idx];
        x = box.left + (i * (iw - 1)) / denom;
        y = baseY - (v * (baseY - topM)) / 100;
        if (y < topM)
            y = topM;
        linePts[i].x = x;
        linePts[i].y = y;
        poly[i + 1].x = x;
        poly[i + 1].y = y;
    }
    poly[sp + 1].x = box.right - 1;
    poly[sp + 1].y = baseY;

    if (fillUnderCurve)
    {
        pen = CreatePen(PS_NULL, 0, 0);
        oldPen = (HPEN)SelectObject(hdc, pen);
        fillBr = CreateSolidBrush(fillRgb);
        oldBr = (HBRUSH)SelectObject(hdc, fillBr);
        SetPolyFillMode(hdc, WINDING);
        Polygon(hdc, poly, sp + 2);
        SelectObject(hdc, oldBr);
        DeleteObject(fillBr);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    pen = CreatePen(PS_SOLID, 1, lineRgb);
    oldPen = (HPEN)SelectObject(hdc, pen);
    Polyline(hdc, linePts, sp);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static void
ShowCpuGraphModeContextMenu(HWND hwndMain, HWND hwndGraph, LPARAM lParam)
{
    HMENU pop;
    WCHAR m1[96], m2[96];
    POINT pt;

    if (!hwndMain || !hwndGraph)
        return;
    pop = CreatePopupMenu();
    if (!pop)
        return;

    if (lParam == (LPARAM)-1)
    {
        RECT wr;
        GetWindowRect(hwndGraph, &wr);
        pt.x = (wr.left + wr.right) / 2;
        pt.y = (wr.top + wr.bottom) / 2;
    }
    else
    {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
    }

    LoadStr(IDS_VIEW_CPU_OVERALL, m1, _countof(m1));
    LoadStr(IDS_VIEW_CPU_PERLOG, m2, _countof(m2));
    AppendMenuW(pop, MF_STRING, ID_VIEW_CPU_GRAPH_OVERALL, m1);
    AppendMenuW(pop, MF_STRING, ID_VIEW_CPU_GRAPH_PERCPU, m2);
    CheckMenuRadioItem(pop, ID_VIEW_CPU_GRAPH_OVERALL, ID_VIEW_CPU_GRAPH_PERCPU,
                       s_CpuGraphPerLogical ? ID_VIEW_CPU_GRAPH_PERCPU : ID_VIEW_CPU_GRAPH_OVERALL,
                       MF_BYCOMMAND);

    SetForegroundWindow(hwndMain);
    TrackPopupMenu(pop, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwndMain, NULL);
    PostMessageW(hwndMain, WM_NULL, 0, 0);
    DestroyMenu(pop);
}

static LRESULT CALLBACK
GraphWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_RBUTTONUP && hwnd == s_hwndGraphCpu)
    {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ClientToScreen(hwnd, &pt);
        SendMessageW(s_hwndMain, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELPARAM(pt.x, pt.y));
        return 0;
    }

    if (msg == WM_ERASEBKGND)
        return 1;

    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT rc;
        int w, h;
        HDC mem;
        HBITMAP bmp, oldBmp;
        HBRUSH bgBr;

        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        w = rc.right - rc.left;
        h = rc.bottom - rc.top;
        if (w < 1)
            w = 1;
        if (h < 1)
            h = 1;

        mem = CreateCompatibleDC(hdc);
        bmp = CreateCompatibleBitmap(hdc, w, h);
        oldBmp = (HBITMAP)SelectObject(mem, bmp);

        bgBr = CreateSolidBrush(COL_GRAPH_BG);
        FillRect(mem, &rc, bgBr);
        DeleteObject(bgBr);

        if (hwnd == s_hwndGraphMem)
        {
            DrawMemoryUsageHistoryGraph(mem, &rc, s_MemHist, CPU_HIST_LEN, s_MemHistPos,
                                        COL_MEM_GRAPH_FILL, COL_MEM_GRAPH_LINE, 1);
        }
        else if (hwnd == s_hwndGraphMemComp)
        {
            DrawMemCompositionBar(mem, &rc);
        }
        else if (hwnd == s_hwndGraphCpu)
        {
            DWORD n = s_NumLogicalCpus;
            if (n > TM8_MAX_LOGICAL_CPU)
                n = TM8_MAX_LOGICAL_CPU;

            if (s_CpuGraphPerLogical && n > 1 && s_pNtQSI)
            {
                RECT gridRc = rc;
                int cols, rows, cw, ch, r, c, idx;

                CpuGraphGridDims((int)n, &cols, &rows);
                cw = (gridRc.right - gridRc.left) / cols;
                ch = (gridRc.bottom - gridRc.top) / rows;
                if (cw < 20 || ch < 20)
                {
                    DrawPerfHistoryGraph(mem, &gridRc, s_CpuHist, CPU_HIST_LEN, s_CpuHistPos,
                                         COL_GRAPH_FILL, COL_GRAPH_LINE, 0, TRUE, 1);
                }
                else
                {
                    for (r = 0; r < rows; r++)
                    {
                        for (c = 0; c < cols; c++)
                        {
                            RECT cell;
                            HPEN cellFrame, oldCellPen;

                            idx = r * cols + c;
                            if (idx >= (int)n)
                                continue;
                            cell.left = gridRc.left + c * cw;
                            cell.top = gridRc.top + r * ch;
                            cell.right = cell.left + cw;
                            cell.bottom = cell.top + ch;

                            cellFrame = CreatePen(PS_SOLID, 1, RGB(235, 235, 235));
                            oldCellPen = (HPEN)SelectObject(mem, cellFrame);
                            Rectangle(mem, cell.left, cell.top, cell.right - 1, cell.bottom - 1);
                            SelectObject(mem, oldCellPen);
                            DeleteObject(cellFrame);

                            InflateRect(&cell, -2, -2);
                            DrawPerfHistoryGraph(mem, &cell, s_CpuHistPer[idx], CPU_HIST_LEN,
                                                 s_CpuHistPos, COL_GRAPH_FILL, COL_GRAPH_LINE, 0, FALSE,
                                                 1);
                        }
                    }
                    {
                        WCHAR gtxt[112];
                        HFONT hfScale = s_hFontNavMeta ? s_hFontNavMeta : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                        HFONT oldF = (HFONT)SelectObject(mem, hfScale);
                        RECT rcScale;
                        SetBkMode(mem, TRANSPARENT);
                        SetTextColor(mem, RGB(105, 105, 105));
                        LoadStr(IDS_GRAPH_SCALE, gtxt, _countof(gtxt));
                        rcScale.left = gridRc.right - 44;
                        rcScale.right = gridRc.right - 4;
                        rcScale.top = gridRc.top + 1;
                        rcScale.bottom = gridRc.top + 14;
                        DrawTextW(mem, gtxt, -1, &rcScale, DT_TOP | DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);
                        SelectObject(mem, oldF);
                    }
                }
            }
            else
            {
                DrawPerfHistoryGraph(mem, &rc, s_CpuHist, CPU_HIST_LEN, s_CpuHistPos,
                                     COL_GRAPH_FILL, COL_GRAPH_LINE, 0, TRUE, 1);
            }
        }

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        SelectObject(mem, oldBmp);
        DeleteObject(bmp);
        DeleteDC(mem);

        EndPaint(hwnd, &ps);
        return 0;
    }
    return CallWindowProcW(s_pfnOldGraph, hwnd, msg, wParam, lParam);
}

int WINAPI
wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmd, int show)
{
    WCHAR cap[64];
    HWND hwnd;
    MSG msg;
    WNDCLASSW wc;

    (void)hPrev;
    (void)cmd;

    s_hInst = hInst;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = L"RosTaskMgr8";
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAINMENU);
    RegisterClassW(&wc);

    LoadStr(IDS_APP_TITLE, cap, _countof(cap));

    hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"RosTaskMgr8", cap,
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                           CW_USEDEFAULT, CW_USEDEFAULT, 720, 520,
                           NULL, NULL, hInst, NULL);
    if (!hwnd)
        return 1;

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
