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
