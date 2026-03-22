/*
 * PROJECT:     ReactOS — modern Task Manager (Win8-style shell)
 * LICENSE:     GPL-2.0-or-later OR LGPL-2.1-or-later
 * PURPOSE:     Main window: tabs, process list, performance graphs, layout, timers.
 *
 * Supporting translation units: taskmgr8_helpers.c (strings/format/registry/CPUID),
 * taskmgr8_cpu_stats.c (CPU statistics strip + RosTm8CpuStats control).
 * taskmgr8_mem_stats.c (Memory statistics strip + RosTm8MemStats control).
 */

#include "taskmgr8_common.h"
#include "taskmgr8_helpers.h"
#include "taskmgr8_cpu_stats.h"
#include "taskmgr8_mem_stats.h"
#include <iphlpapi.h>
#include <ipifcons.h>
#include <string.h>

/* See taskmgr8_shared.h — exported for helpers / cpu_stats only. */
HINSTANCE s_hInst;
HWND s_hwndCpuStatsPanel;
HFONT s_hFontCpuLbl;
HFONT s_hFontCpuVal;
DWORD s_NominalCpuMhz;
DWORD s_CurrentCpuMhzLive;
DWORD s_NumLogicalCpus;
int s_LastCpuPct;
#if defined(_WIN64)
PFN_GetTickCount64 s_pfnGetTickCount64;
#endif
HWND s_hwndMemStatsPanel;
TM8_MEM_STATS_PAINT s_MemStatsPaint;

static HWND s_hwndMain;
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
static HWND s_hwndMemTitle;
static HWND s_hwndMemModel;
static HWND s_hwndMemGraphSub;
static HWND s_hwndMemCompSub;
static HWND s_hwndGraphMemComp;
static HWND s_hwndMainTab;
static HWND s_hwndStub;
static HWND s_hwndEndTask;
static HMENU s_hProcMenuRoot;
static HMENU s_hCtxMenu;
static int s_iPage;
static int s_iPerfNavSel;

typedef struct _TM8_NET_ADAPT
{
    DWORD dwIndex;
    DWORD ifType;
    DWORD physLen;
    UCHAR physAddr[MAXLEN_PHYSADDR];
    DWORD descrLen;
    BYTE bDescrCopy[MAXLEN_IFDESCR];
    WCHAR wszListTitle[128];
    ULONG64 prevInOctets;
    ULONG64 prevOutOctets;
    BOOL havePrev;
} TM8_NET_ADAPT;

static TM8_NET_ADAPT s_NetAdapters[TM8_MAX_NET_ADAPTERS];
static int s_NetAdapterCount;
static BYTE s_NetHist[TM8_MAX_NET_ADAPTERS][CPU_HIST_LEN];
static WCHAR s_NetMetaLine[TM8_MAX_NET_ADAPTERS][120];
static int s_iNetAdapterSel;
static HWND s_hwndNetTitle;
static HWND s_hwndNetSub;
static HWND s_hwndGraphNet;

static FILETIME s_ftIdle0, s_ftKernel0, s_ftUser0;
static BOOL s_bCpuTimesInit;

static CPU_TRACK s_CpuTrack[MAX_CPU_TRACK];
static int s_CpuTrackCount;

static HIMAGELIST s_hProcSmIl;
static int s_IconCacheCount;
static WCHAR s_IconCachePath[TM8_ICON_CACHE_MAX][MAX_PATH];
static int s_IconCacheIdx[TM8_ICON_CACHE_MAX];
static int s_ProcListRows;
static double s_ProcCpuDbl[TM8_MAX_PROC_ROWS];
static SIZE_T s_ProcMemWs[TM8_MAX_PROC_ROWS];
static SIZE_T s_ProcMemMax;

/* 0 = alphabetical by name (default); 1 = descending by metric; 2 = ascending */
static int s_ProcSortCol = TM8_PROCSORT_COL_NONE;
static int s_ProcSortPhase;

/* Pause process-list refresh while dragging the vertical scrollbar; 1s cooldown after. */
static BOOL s_ProcListVScrollDragging;
static DWORD s_ProcListResumeDeadline;

static TM8_SCRATCH_PROC s_ProcScratch[TM8_MAX_PROC_ROWS];

static void SyncProcEndTaskUi(void);
static DWORD GetSelectedPid(void);
static void RefreshProcessList(void);
static void RefreshProcessListEx(BOOL force);
static void Tm8PerfReloadNetAdapters(void);
static void Tm8SampleNetAdapters(int histPos);
static void RefreshNetworkDetailUi(void);

static BYTE s_CpuHist[CPU_HIST_LEN];
static int s_CpuHistPos;
static BYTE s_MemHist[CPU_HIST_LEN];
static int s_MemHistPos;
static BOOL s_PerfHistPrimed;
static int s_LastMemUsagePct;
static ULONGLONG s_MemTotalPhysNav;
static ULONGLONG s_MemUsedPhysNav;
static BYTE s_CpuHistPer[TM8_MAX_LOGICAL_CPU][CPU_HIST_LEN];
static int s_LastCpuPctPer[TM8_MAX_LOGICAL_CPU];
static TM8_PROC_PERF_INFO s_PrevProcPerf[TM8_MAX_LOGICAL_CPU];
static BOOL s_ProcPerfInited;
static BOOL s_CpuGraphPerLogical = TRUE;
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

static WCHAR s_szMemGraphYMax[72];
static double s_MemCompFrac[3]; /* in use, cached, free */

static TM8_MEM_DIMM_INFO s_MemDimm;

static LRESULT CALLBACK GraphWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
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
static HFONT s_hFontCpuHero;
static WCHAR s_szCpuModel[260];
static WCHAR s_MainTabText[7][48];

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
    if (s_hwndMemStatsPanel)
        SendMessageW(s_hwndMemStatsPanel, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    if (s_hwndNetTitle)
        SendMessageW(s_hwndNetTitle, WM_SETFONT, (WPARAM)s_hFontTitle, FALSE);
    if (s_hwndNetSub)
        SendMessageW(s_hwndNetSub, WM_SETFONT, (WPARAM)s_hFontNavMeta, FALSE);
    if (s_hwndStub)
        SendMessageW(s_hwndStub, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
    if (s_hwndEndTask)
        SendMessageW(s_hwndEndTask, WM_SETFONT, (WPARAM)s_hFontPerf, FALSE);
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
    const int statusH = 0; /* No status bar (Win10/11 Processes has no CPU/RAM strip here). */
    int navW = NAV_WIDTH, margin = 6;
    int pageLeft, pageW, pageTop, pageH;
    int graphTop, graphH;
    int tabTop, contentTop, tabBarH;
    int tabW;

    GetClientRect(hwnd, &rc);

    if (rc.bottom <= margin * 2)
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
            int navContentH = 0;
            if (cnt > 0)
            {
                navContentH = NAV_ITEM_H_CPU + (cnt - 1) * NAV_ITEM_H_MEM + 4;
                navH = navContentH;
                if (navH > pageH)
                    navH = pageH;
            }
            ShowScrollBar(s_hwndNav, SB_VERT, navContentH > pageH);
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

        if (footerH > 0 && s_hwndEndTask)
        {
            int btnW = 100, btnH = 26, yBtn = pageTop + listH + 8;

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
    else if (s_iPage == PAGE_NETWORK)
    {
        const int titleY = pageTop + 4;
        const int titleH = 26;
        const int subH = 15;
        const int subGap = 1;
        const int graphAfterSub = 5;
        int titleW = (pageW - 16) / 5;
        int subY;

        if (titleW < 96)
            titleW = 96;
        if (titleW > 200)
            titleW = 200;
        if (titleW > pageW - 16 - 120)
            titleW = (pageW - 16) - 120;
        if (titleW < 72)
            titleW = 72;
        subY = titleY + titleH + subGap;
        SetWindowPos(s_hwndNetTitle, NULL, pageLeft + 4, titleY, titleW - 4, titleH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(s_hwndNetSub, NULL, pageLeft + 4, subY, pageW - 8, subH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        graphTop = subY + subH + graphAfterSub;
        graphH = pageTop + pageH - graphTop - 8;
        if (graphH < 56)
            graphH = 56;
        InvalidateRect(s_hwndNetTitle, NULL, TRUE);
        InvalidateRect(s_hwndNetSub, NULL, TRUE);
    }
    else
    {
        graphTop = pageTop;
        graphH = 1;
    }

    if (s_iPage == PAGE_CPU || s_iPage == PAGE_MEMORY || s_iPage == PAGE_NETWORK)
    {
        if (graphH > pageH - 8)
            graphH = pageH - 8;
        if (s_iPage == PAGE_CPU && graphH < 100)
            graphH = 100;
    }

    SetWindowPos(s_hwndGraphCpu, NULL, pageLeft + 4, s_iPage == PAGE_CPU ? graphTop : pageTop,
                 pageW - 8, s_iPage == PAGE_CPU ? graphH : 1, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(s_hwndGraphMem, NULL, pageLeft + 4, s_iPage == PAGE_MEMORY ? graphTop : pageTop,
                 pageW - 8, s_iPage == PAGE_MEMORY ? graphH : 1, SWP_NOZORDER | SWP_NOACTIVATE);
    if (s_hwndGraphNet)
        SetWindowPos(s_hwndGraphNet, NULL, pageLeft + 4, s_iPage == PAGE_NETWORK ? graphTop : pageTop,
                     pageW - 8, s_iPage == PAGE_NETWORK ? graphH : 1, SWP_NOZORDER | SWP_NOACTIVATE);

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

    Tm8CpuStats_OnLeaveCpuPage(page);
    Tm8MemStats_OnLeaveMemoryPage(page);

    ShowWindow(s_hwndList, (page == PAGE_PROCESSES) ? SW_SHOW : SW_HIDE);
    if (s_hwndEndTask)
        ShowWindow(s_hwndEndTask, (page == PAGE_PROCESSES) ? SW_SHOW : SW_HIDE);
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

    ShowWindow(s_hwndNetTitle, (page == PAGE_NETWORK) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndNetSub, (page == PAGE_NETWORK) ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hwndGraphNet, (page == PAGE_NETWORK) ? SW_SHOW : SW_HIDE);

    if (s_hwndMain)
        LayoutChildren(s_hwndMain);
    if (s_hwndNav)
        InvalidateRect(s_hwndNav, NULL, TRUE);
    if (page == PAGE_NETWORK)
        RefreshNetworkDetailUi();
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
        for (i = 0; i < TM8_MAX_NET_ADAPTERS; i++)
            FillMemory(s_NetHist[i], sizeof(s_NetHist[i]), 0);
        s_CpuHistPos = 0;
        s_MemHistPos = 0;
        s_PerfHistPrimed = TRUE;
    }

    s_CpuHist[s_CpuHistPos] = (BYTE)cpuPct;
    s_MemHist[s_MemHistPos] = (BYTE)memPct;
    for (i = 0; i < n; i++)
        s_CpuHistPer[i][s_CpuHistPos] = (BYTE)s_LastCpuPctPer[i];
    Tm8SampleNetAdapters(s_CpuHistPos);

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

    Tm8CpuStats_Refresh();
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

    Tm8MemStats_UpdateScroll();
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
    RefreshNetworkDetailUi();
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

static int
Tm8CmpNetAdapt(const void *a, const void *b)
{
    const TM8_NET_ADAPT *pa = a, *pb = b;
    if (pa->dwIndex < pb->dwIndex)
        return -1;
    if (pa->dwIndex > pb->dwIndex)
        return 1;
    return 0;
}

static void
Tm8FmtNetRateKbps(ULONG64 deltaOctets, WCHAR *buf, size_t cch)
{
    double Bps, kbps;
    if (!buf || cch == 0)
        return;
    if (deltaOctets == 0 || TIMER_MS < 1)
    {
        StringCchCopyW(buf, cch, L"0");
        return;
    }
    Bps = (double)deltaOctets * 1000.0 / (double)TIMER_MS;
    kbps = Bps * 8.0 / 1000.0;
    if (kbps >= 10000.0)
        StringCchPrintfW(buf, cch, L"%.1f Mbps", kbps / 1000.0);
    else if (kbps >= 1.0)
        StringCchPrintfW(buf, cch, L"%.0f Kbps", kbps);
    else
        StringCchPrintfW(buf, cch, L"%.1f Kbps", kbps);
}

static BOOL
Tm8PhysAddrAllZero(const UCHAR *addr, DWORD len)
{
    DWORD i;
    if (!addr || len == 0)
        return TRUE;
    for (i = 0; i < len; i++)
    {
        if (addr[i] != 0)
            return FALSE;
    }
    return TRUE;
}

/* bDescr may be unterminated; only first dlen bytes are valid. */
static BOOL
Tm8NetDescrHasWanAsciiCi(const BYTE *bd, DWORD dlen)
{
    DWORD i;
    if (!bd || dlen < 3)
        return FALSE;
    for (i = 0; i + 3 <= dlen; i++)
    {
        char a = (char)bd[i], b = (char)bd[i + 1], c = (char)bd[i + 2];
        if ((a == 'W' || a == 'w') && (b == 'A' || b == 'a') && (c == 'N' || c == 'n'))
            return TRUE;
    }
    return FALSE;
}

/* hay is not necessarily NUL-terminated; only hayLen bytes are valid. */
static BOOL
Tm8NetDescrContainsAsciiCi(const BYTE *hay, DWORD hayLen, const char *needle)
{
    size_t n;
    DWORD i;
    size_t j;
    if (!hay || !needle)
        return FALSE;
    n = strlen(needle);
    if (n == 0 || hayLen < n)
        return FALSE;
    for (i = 0; i + (DWORD)n <= hayLen; i++)
    {
        for (j = 0; j < n; j++)
        {
            char a = (char)hay[i + (DWORD)j], b = needle[j];
            if (a >= 'A' && a <= 'Z')
                a = (char)(a + ('a' - 'A'));
            if (b >= 'A' && b <= 'Z')
                b = (char)(b + ('a' - 'A'));
            if (a != b)
                break;
        }
        if (j == n)
            return TRUE;
    }
    return FALSE;
}

static void
Tm8PerfReloadNetAdapters(void)
{
    ULONG size = 0;
    PMIB_IFTABLE pTab;
    DWORD err, j;
    int x;

    s_NetAdapterCount = 0;
    err = GetIfTable(NULL, &size, FALSE);
    if (err != ERROR_INSUFFICIENT_BUFFER || size < sizeof(MIB_IFTABLE))
        return;
    pTab = (PMIB_IFTABLE)HeapAlloc(GetProcessHeap(), 0, size);
    if (!pTab)
        return;
    err = GetIfTable(pTab, &size, FALSE);
    if (err != NO_ERROR)
    {
        HeapFree(GetProcessHeap(), 0, pTab);
        return;
    }
    for (j = 0; j < pTab->dwNumEntries && s_NetAdapterCount < TM8_MAX_NET_ADAPTERS; j++)
    {
        MIB_IFROW *row = &pTab->table[j];
        DWORD pl;
        DWORD dlen;
        WCHAR titleTmp[128];
        TM8_NET_ADAPT *na;

        if (row->dwType == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;

        /* Like Windows Task Manager: list only enabled interfaces with an active link. */
        if (row->dwAdminStatus == MIB_IF_ADMIN_STATUS_DOWN)
            continue;
        if (row->dwOperStatus != IF_OPER_STATUS_OPERATIONAL &&
            row->dwOperStatus != IF_OPER_STATUS_CONNECTED)
            continue;

        pl = row->dwPhysAddrLen;
        if (pl > MAXLEN_PHYSADDR)
            pl = MAXLEN_PHYSADDR;

        dlen = row->dwDescrLen;
        if (dlen > MAXLEN_IFDESCR)
            dlen = MAXLEN_IFDESCR;
        if (dlen == 0)
        {
            DWORD i;
            for (i = 0; i < MAXLEN_IFDESCR && row->bDescr[i]; i++)
                ;
            dlen = i;
        }

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)row->bDescr, -1, titleTmp,
                            (int)_countof(titleTmp));
        if (titleTmp[0] == 0)
            LoadStr(IDS_COL_NETWORK, titleTmp, _countof(titleTmp));

        /*
         * Skip WAN Miniport pseudo-adapters: dozens of NDIS bindings, usually idle graphs;
         * Task Manager–style lists focus on real Ethernet / Wi‑Fi / Hyper-V / VM adapters.
         */
        if (StrStrIW(titleTmp, L"WAN Miniport") != NULL ||
            Tm8NetDescrContainsAsciiCi(row->bDescr, dlen, "WAN Miniport"))
            continue;

        /*
         * GetIfTable often returns several MIB rows per real NIC (filters / NDIS stack),
         * same MAC and identical counters — collapse to one tile per hardware address.
         * Rows with no MAC (e.g. some WAN miniports): collapse by interface type + descr bytes.
         */
        if (pl > 0 && !Tm8PhysAddrAllZero(row->bPhysAddr, pl))
        {
            for (x = 0; x < s_NetAdapterCount; x++)
            {
                if (s_NetAdapters[x].physLen == pl &&
                    memcmp(s_NetAdapters[x].physAddr, row->bPhysAddr, pl) == 0)
                    goto skip_net_row;
            }
        }
        else
        {
            for (x = 0; x < s_NetAdapterCount; x++)
            {
                if (s_NetAdapters[x].physLen != 0)
                    continue;
                if (s_NetAdapters[x].ifType != row->dwType)
                    continue;
                if (dlen == 0 || s_NetAdapters[x].descrLen != dlen)
                    continue;
                if (memcmp(s_NetAdapters[x].bDescrCopy, row->bDescr, dlen) == 0)
                    goto skip_net_row;
            }
        }

        /*
         * Wi‑Fi / WLAN: many MIB rows use different virtual MACs but the same driver title — one tile.
         * Some stacks use a non‑802.11 ifType; match common title keywords. Never fold real Ethernet (type 6).
         * WAN miniports: descr strings differ (IKEv2 vs IP…) while the UI title matches — merge by title.
         */
        if (row->dwType != IF_TYPE_ETHERNET_CSMACD &&
            (row->dwType == IF_TYPE_IEEE80211 || StrStrIW(titleTmp, L"Wireless") != NULL ||
             StrStrIW(titleTmp, L"Wi-Fi") != NULL || StrStrIW(titleTmp, L"WiFi") != NULL ||
             StrStrIW(titleTmp, L"WLAN") != NULL || StrStrIW(titleTmp, L"802.11") != NULL))
        {
            for (x = 0; x < s_NetAdapterCount; x++)
            {
                if (s_NetAdapters[x].ifType == IF_TYPE_ETHERNET_CSMACD)
                    continue;
                if (_wcsicmp(s_NetAdapters[x].wszListTitle, titleTmp) == 0)
                    goto skip_net_row;
            }
        }
        if (StrStrIW(titleTmp, L"WAN") || Tm8NetDescrHasWanAsciiCi(row->bDescr, dlen))
        {
            for (x = 0; x < s_NetAdapterCount; x++)
            {
                if (_wcsicmp(s_NetAdapters[x].wszListTitle, titleTmp) == 0)
                    goto skip_net_row;
            }
        }

        na = &s_NetAdapters[s_NetAdapterCount];
        na->dwIndex = row->dwIndex;
        na->ifType = row->dwType;
        na->havePrev = FALSE;
        na->descrLen = dlen;
        if (dlen > 0)
            memcpy(na->bDescrCopy, row->bDescr, dlen);
        if (pl > 0 && !Tm8PhysAddrAllZero(row->bPhysAddr, pl))
        {
            na->physLen = pl;
            memcpy(na->physAddr, row->bPhysAddr, pl);
        }
        else
        {
            na->physLen = 0;
        }

        StringCchCopyW(na->wszListTitle, _countof(na->wszListTitle), titleTmp);
        s_NetMetaLine[s_NetAdapterCount][0] = 0;
        s_NetAdapterCount++;

    skip_net_row:
        ;
    }
    HeapFree(GetProcessHeap(), 0, pTab);

    if (s_NetAdapterCount > 1)
        qsort(s_NetAdapters, (size_t)s_NetAdapterCount, sizeof(s_NetAdapters[0]), Tm8CmpNetAdapt);

    for (j = 0; j < (DWORD)s_NetAdapterCount; j++)
        SendMessageW(s_hwndNav, LB_ADDSTRING, 0, (LPARAM)s_NetAdapters[j].wszListTitle);
}

static void
Tm8SampleNetAdapters(int histPos)
{
    ULONG size = 0;
    PMIB_IFTABLE pTab;
    DWORD err, j, k;

    if (s_NetAdapterCount <= 0)
        return;

    err = GetIfTable(NULL, &size, FALSE);
    if (err != ERROR_INSUFFICIENT_BUFFER || size < sizeof(MIB_IFTABLE))
        return;
    pTab = (PMIB_IFTABLE)HeapAlloc(GetProcessHeap(), 0, size);
    if (!pTab)
        return;
    err = GetIfTable(pTab, &size, FALSE);
    if (err != NO_ERROR)
    {
        HeapFree(GetProcessHeap(), 0, pTab);
        return;
    }

    for (k = 0; k < (DWORD)s_NetAdapterCount; k++)
    {
        TM8_NET_ADAPT *ad = &s_NetAdapters[k];
        MIB_IFROW *row = NULL;
        ULONG64 inOct, outOct, dIn, dOut, dTot;
        int pct;
        double bpsTot;
        WCHAR sRate[48], rRate[48];

        for (j = 0; j < pTab->dwNumEntries; j++)
        {
            if (pTab->table[j].dwIndex == ad->dwIndex)
            {
                row = &pTab->table[j];
                break;
            }
        }
        if (!row)
        {
            s_NetHist[k][histPos] = 0;
            continue;
        }

        inOct = row->dwInOctets;
        outOct = row->dwOutOctets;

        if (!ad->havePrev)
        {
            ad->prevInOctets = inOct;
            ad->prevOutOctets = outOct;
            ad->havePrev = TRUE;
            s_NetHist[k][histPos] = 0;
            StringCchCopyW(s_NetMetaLine[k], _countof(s_NetMetaLine[k]), L"S: 0  R: 0");
            continue;
        }

        if (inOct >= ad->prevInOctets)
            dIn = inOct - ad->prevInOctets;
        else
            dIn = (ULONG64)0x100000000ULL + inOct - ad->prevInOctets;
        if (outOct >= ad->prevOutOctets)
            dOut = outOct - ad->prevOutOctets;
        else
            dOut = (ULONG64)0x100000000ULL + outOct - ad->prevOutOctets;

        ad->prevInOctets = inOct;
        ad->prevOutOctets = outOct;

        dTot = dIn + dOut;
        /* Use double (like Tm8MulPagesToBytes) so x86 MSVC does not emit __allmul. */
        bpsTot = (TIMER_MS > 0) ? ((double)dTot * 1000.0 / (double)TIMER_MS) : 0.0;
        pct = (int)(bpsTot / 125000.0);
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;
        s_NetHist[k][histPos] = (BYTE)pct;

        Tm8FmtNetRateKbps(dOut, sRate, _countof(sRate));
        Tm8FmtNetRateKbps(dIn, rRate, _countof(rRate));
        StringCchPrintfW(s_NetMetaLine[k], _countof(s_NetMetaLine[k]), L"S: %ls  R: %ls", sRate, rRate);
    }

    HeapFree(GetProcessHeap(), 0, pTab);
}

static void
RefreshNetworkDetailUi(void)
{
    WCHAR cap[96];

    if (s_iPage != PAGE_NETWORK)
        return;
    if (!s_hwndNetTitle || !s_hwndGraphNet)
        return;
    if (s_iNetAdapterSel < 0 || s_iNetAdapterSel >= s_NetAdapterCount)
        return;

    StringCchCopyW(cap, _countof(cap), s_NetAdapters[s_iNetAdapterSel].wszListTitle);
    SetWindowTextW(s_hwndNetTitle, cap);

    LoadStr(IDS_NET_THROUGHPUT, cap, _countof(cap));
    SetWindowTextW(s_hwndNetSub, cap);

    InvalidateRect(s_hwndGraphNet, NULL, FALSE);
}

static void
FillNavList(void)
{
    WCHAR b[128];
    DWORD selIf = 0;
    BOOL hadNetSel = (s_iPerfNavSel >= 2 && s_NetAdapterCount > 0 &&
                      (s_iPerfNavSel - 2) < s_NetAdapterCount);

    if (hadNetSel)
        selIf = s_NetAdapters[s_iPerfNavSel - 2].dwIndex;

    SendMessageW(s_hwndNav, LB_RESETCONTENT, 0, 0);
    LoadStr(IDS_NAV_CPU, b, _countof(b));
    SendMessageW(s_hwndNav, LB_ADDSTRING, 0, (LPARAM)b);
    LoadStr(IDS_NAV_MEMORY, b, _countof(b));
    SendMessageW(s_hwndNav, LB_ADDSTRING, 0, (LPARAM)b);

    Tm8PerfReloadNetAdapters();

    {
        int cnt = 2 + s_NetAdapterCount;
        int newSel = s_iPerfNavSel;
        int k;

        if (cnt > 0)
        {
            if (newSel >= cnt)
                newSel = cnt - 1;
            if (newSel < 0)
                newSel = 0;
        }

        if (hadNetSel && s_NetAdapterCount > 0)
        {
            for (k = 0; k < s_NetAdapterCount; k++)
            {
                if (s_NetAdapters[k].dwIndex == selIf)
                {
                    newSel = 2 + k;
                    break;
                }
            }
        }

        s_iPerfNavSel = newSel;
        if (newSel >= 2)
            s_iNetAdapterSel = newSel - 2;
        else
            s_iNetAdapterSel = 0;
        if (s_iNetAdapterSel >= s_NetAdapterCount && s_NetAdapterCount > 0)
            s_iNetAdapterSel = s_NetAdapterCount - 1;

        SendMessageW(s_hwndNav, LB_SETCURSEL, newSel, 0);
    }
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
            WCHAR et[48];
            LoadStr(IDS_ENDTASK, et, _countof(et));
            s_hwndEndTask =
                CreateWindowW(L"Button", et,
                              WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | WS_VISIBLE,
                              0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_ENDTASK_BTN, s_hInst, NULL);
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
        Tm8CpuStats_RegisterClass(s_hInst);
        s_hwndCpuStatsPanel =
            CreateWindowW(TM8_CPU_STATS_WNDCLASS, L"", WS_CHILD | WS_HSCROLL, 0, 0, 10, 10, hwnd,
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
        Tm8MemStats_RegisterClass(s_hInst);
        s_hwndMemStatsPanel =
            CreateWindowW(TM8_MEM_STATS_WNDCLASS, L"", WS_CHILD | WS_HSCROLL, 0, 0, 10, 10, hwnd,
                          (HMENU)(UINT_PTR)IDC_MEM_STATS_PANEL, s_hInst, NULL);

        s_hwndNetTitle = CreateWindowW(L"Static", L"",
                                         WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                         0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_NET_TITLE,
                                         s_hInst, NULL);
        s_hwndNetSub = CreateWindowW(L"Static", L"",
                                     WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                     0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_NET_SUB,
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

        s_hwndGraphNet = CreateWindowW(L"Static", L"",
                                       WS_CHILD | WS_BORDER,
                                       0, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)IDC_NET_GRAPH,
                                       s_hInst, NULL);
        SetWindowLongPtrW(s_hwndGraphNet, GWLP_WNDPROC, (LONG_PTR)GraphWndProc);

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
            if (sel < 0)
                return 0;
            s_iPerfNavSel = sel;
            if (sel == 0)
                ShowPage(PAGE_CPU);
            else if (sel == 1)
                ShowPage(PAGE_MEMORY);
            else
            {
                s_iNetAdapterSel = sel - 2;
                if (s_iNetAdapterSel >= s_NetAdapterCount)
                    s_iNetAdapterSel = 0;
                ShowPage(PAGE_NETWORK);
            }
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
                else if ((int)dis->itemID >= 2)
                {
                    int ni = (int)dis->itemID - 2;
                    if (ni >= 0 && ni < s_NetAdapterCount)
                    {
                        NavDrawMiniSpark(dis->hDC, &rcSpark, s_NetHist[ni], CPU_HIST_LEN, s_CpuHistPos,
                                         COL_GRAPH_FILL, COL_GRAPH_LINE, FALSE);
                    }
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
                else if (dis->itemID == 1)
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
                else if ((int)dis->itemID >= 2)
                {
                    int ni = (int)dis->itemID - 2;

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

                    rcMeta.left = textL;
                    rcMeta.right = textR;
                    rcMeta.top = rcName.bottom + 3;
                    rcMeta.bottom = rcMeta.top + metaH;
                    if (rcMeta.bottom > rcTile.bottom - 5)
                        rcMeta.bottom = rcTile.bottom - 5;
                    if (ni >= 0 && ni < s_NetAdapterCount)
                        DrawTextW(dis->hDC, s_NetMetaLine[ni], -1, &rcMeta,
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
        if ((HWND)lParam == s_hwndCpuTitle || (HWND)lParam == s_hwndNetTitle)
        {
            HDC hdcSt = (HDC)wParam;
            SetBkColor(hdcSt, RGB(255, 255, 255));
            SetTextColor(hdcSt, RGB(32, 32, 32));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
        if ((HWND)lParam == s_hwndCpuGraphSub || (HWND)lParam == s_hwndNetSub ||
            (HWND)lParam == s_hwndCpuLiveLbl || (HWND)lParam == s_hwndCpuSpecLbl)
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
                FillNavList();
                SendMessageW(s_hwndNav, LB_SETCURSEL, s_iPerfNavSel, 0);
                if (s_iPerfNavSel == 0)
                    ShowPage(PAGE_CPU);
                else if (s_iPerfNavSel == 1)
                    ShowPage(PAGE_MEMORY);
                else
                    ShowPage(PAGE_NETWORK);
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
        else if (hwnd == s_hwndGraphNet && s_iNetAdapterSel >= 0 && s_iNetAdapterSel < s_NetAdapterCount)
        {
            DrawPerfHistoryGraph(mem, &rc, s_NetHist[s_iNetAdapterSel], CPU_HIST_LEN, s_CpuHistPos,
                                 COL_GRAPH_FILL, COL_GRAPH_LINE, IDS_NET_THROUGHPUT, TRUE, 1);
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