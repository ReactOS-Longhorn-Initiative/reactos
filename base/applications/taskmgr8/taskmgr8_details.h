/*
 * Details tab — extended process list (session, user, command line).
 */

#pragma once

#include "taskmgr8_common.h"

typedef struct _TM8_DETAILS_ROW_METRICS
{
    double cpuPct;
    SIZE_T ws;
} TM8_DETAILS_ROW_METRICS;

void Tm8Details_SetupListView(HWND hLv);
void Tm8Details_RefreshList(HWND hLv, DWORD msElapsed, BOOL force, BOOL vscrollDragging, DWORD *pResumeDeadline,
                           TM8_DETAILS_ROW_METRICS *metrics, int metricsMax, int *outCount, SIZE_T *outMemMax);
