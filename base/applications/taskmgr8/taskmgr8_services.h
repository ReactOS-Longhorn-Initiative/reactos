/*
 * Services tab — SCM service enumeration.
 */

#pragma once

#include "taskmgr8_common.h"

void Tm8Services_SetupListView(HWND hLv);
void Tm8Services_RefreshList(HWND hLv, BOOL force, BOOL vscrollDragging, DWORD *pResumeDeadline);
