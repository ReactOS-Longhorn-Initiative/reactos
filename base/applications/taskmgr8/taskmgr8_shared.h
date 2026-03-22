/*
 * Symbols defined in taskmgr8.c and read by taskmgr8_helpers.c / taskmgr8_cpu_stats.c.
 * Keep this list minimal to avoid a wide “global struct” refactor.
 */

#pragma once

#include "taskmgr8_common.h"

extern HINSTANCE s_hInst;
extern HWND s_hwndCpuStatsPanel;
extern HWND s_hwndMemStatsPanel;
extern TM8_MEM_STATS_PAINT s_MemStatsPaint;
extern HFONT s_hFontCpuLbl;
extern HFONT s_hFontCpuVal;
extern DWORD s_NominalCpuMhz;
extern DWORD s_CurrentCpuMhzLive;
extern DWORD s_NumLogicalCpus;
extern int s_LastCpuPct;

#if defined(_WIN64)
extern PFN_GetTickCount64 s_pfnGetTickCount64;
#endif

BOOL Tm8QueryProcessImagePath(HANDLE hProc, WCHAR *path, DWORD cchPath);
int Tm8IconForExePath(const WCHAR *path);
void Tm8FmtMbComma1(double mbVal, WCHAR *dst, size_t cch);
double Tm8ProcessCpuUsagePercent(DWORD pid, ULONGLONG *pTotal100Ns, DWORD msElapsed, UINT nCpu);
