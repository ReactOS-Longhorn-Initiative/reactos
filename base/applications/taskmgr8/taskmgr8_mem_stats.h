/*
 * Performance tab — Memory statistics strip (RosTm8MemStats window class).
 */

#pragma once

#include "taskmgr8_common.h"

#define TM8_MEM_STATS_WNDCLASS L"RosTm8MemStats"

BOOL Tm8MemStats_RegisterClass(HINSTANCE hInst);
void Tm8MemStats_OnLeaveMemoryPage(int page);
void Tm8MemStats_UpdateScroll(void);
