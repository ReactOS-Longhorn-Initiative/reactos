/*
 * Services tab — SCM service enumeration.
 */

#pragma once

#include "taskmgr8_common.h"

void Tm8Services_SetupListView(HWND hLv);
void Tm8Services_RefreshList(HWND hLv, BOOL force, BOOL vscrollDragging, DWORD *pResumeDeadline);
void Tm8Services_UpdateContextMenu(HMENU hMenu, HWND hLv);
void Tm8ServiceShowProperties(HWND hwnd, HWND hLv);
void Tm8ServiceStart(HWND hwnd, HWND hLv, DWORD *pResumeDeadline);
void Tm8ServiceStop(HWND hwnd, HWND hLv, DWORD *pResumeDeadline);
void Tm8ServiceRestart(HWND hwnd, HWND hLv, DWORD *pResumeDeadline);
