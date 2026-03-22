/*
 * Performance tab — CPU statistics strip (RosTm8CpuStats window class).
 */

#pragma once

#include "taskmgr8_common.h"

#define TM8_CPU_STATS_WNDCLASS L"RosTm8CpuStats"

BOOL Tm8CpuStats_RegisterClass(HINSTANCE hInst);
void Tm8CpuStats_Refresh(void);
void Tm8CpuStats_OnLeaveCpuPage(int page);
