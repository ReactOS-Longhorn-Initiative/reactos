/*
 * Formatting and lightweight system queries shared by the main window and CPU stats.
 */

#pragma once

#include "taskmgr8_common.h"

void LoadStr(UINT id, WCHAR *buf, size_t cch);

void FormatSpeedFromMhz(DWORD mhz, WCHAR *out, size_t cch);
void FormatUptimeString(WCHAR *buf, size_t cch);
void FormatULongGrouped(ULONG n, WCHAR *buf, size_t cch);

void AppendSpecLbl(WCHAR *buf, size_t cch, UINT id);
void AppendCacheKvLine(WCHAR *lblDest, size_t cchLbl, WCHAR *valDest, size_t cchVal, UINT idsLbl,
                       DWORD kb);

BOOL ReadProcessor0CacheValue(PCWSTR valueName, DWORD *outKb);
DWORD CountPhysicalCores(void);
BOOL CpuVirtHardwarePresent(void);

#if ROS_HAVE_CPUID
void AppendMissingCachesFromCpuid4(WCHAR *specLbl, size_t cchLbl, WCHAR *specVal, size_t cchVal,
                                   int haveL1, int haveL2, int haveL3);
#endif
