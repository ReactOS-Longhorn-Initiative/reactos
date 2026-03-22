/*
 * PROJECT:     ReactOS — Task Manager 8
 * LICENSE:     GPL-2.0-or-later OR LGPL-2.1-or-later
 */

#include "taskmgr8_services.h"
#include "taskmgr8_helpers.h"
#include "taskmgr8_listutil.h"

#include <winsvc.h>

#ifndef SERVICE_WIN32_OWN_PROCESS
#define SERVICE_WIN32_OWN_PROCESS 0x00000010
#endif
#ifndef SERVICE_WIN32_SHARE_PROCESS
#define SERVICE_WIN32_SHARE_PROCESS 0x00000020
#endif
#ifndef SERVICE_WIN32
#define SERVICE_WIN32 (SERVICE_WIN32_OWN_PROCESS | SERVICE_WIN32_SHARE_PROCESS)
#endif
#ifndef SERVICE_AUTO_START
#define SERVICE_AUTO_START 2
#endif
#ifndef SERVICE_DEMAND_START
#define SERVICE_DEMAND_START 3
#endif
#ifndef SERVICE_DISABLED
#define SERVICE_DISABLED 4
#endif
#ifndef SERVICE_BOOT_START
#define SERVICE_BOOT_START 0
#endif
#ifndef SERVICE_SYSTEM_START
#define SERVICE_SYSTEM_START 1
#endif

typedef struct _TM8_SVC_SORT
{
    LPENUM_SERVICE_STATUS_PROCESSW ep;
} TM8_SVC_SORT;

static int __cdecl
Tm8SvcCmpDisplay(const void *a, const void *b)
{
    const TM8_SVC_SORT *x = (const TM8_SVC_SORT *)a;
    const TM8_SVC_SORT *y = (const TM8_SVC_SORT *)b;
    LPCWSTR dx = x->ep->lpDisplayName && x->ep->lpDisplayName[0] ? x->ep->lpDisplayName
                                                                   : x->ep->lpServiceName;
    LPCWSTR dy = y->ep->lpDisplayName && y->ep->lpDisplayName[0] ? y->ep->lpDisplayName
                                                                   : y->ep->lpServiceName;
    if (!dx)
        dx = L"";
    if (!dy)
        dy = L"";
    return _wcsicmp(dx, dy);
}

static LPCWSTR
Tm8SvcStateString(DWORD state, LPCWSTR sRun, LPCWSTR sStop)
{
    switch (state)
    {
    case SERVICE_RUNNING:
        return sRun;
    case SERVICE_STOPPED:
        return sStop;
    case SERVICE_START_PENDING:
        return L"Start pending";
    case SERVICE_STOP_PENDING:
        return L"Stop pending";
    case SERVICE_PAUSED:
        return L"Paused";
    default:
        return L"";
    }
}

static LPCWSTR
Tm8SvcStartTypeString(DWORD st, WCHAR *bAuto, WCHAR *bMan, WCHAR *bDis, WCHAR *bBoot, WCHAR *bSys)
{
    switch (st)
    {
    case SERVICE_AUTO_START:
        return bAuto;
    case SERVICE_DEMAND_START:
        return bMan;
    case SERVICE_DISABLED:
        return bDis;
    case SERVICE_BOOT_START:
        return bBoot;
    case SERVICE_SYSTEM_START:
        return bSys;
    default:
        return L"";
    }
}

void
Tm8Services_SetupListView(HWND hLv)
{
    WCHAR b[96];
    LVCOLUMNW col;

    if (!hLv)
        return;

    Tm8LvResetContentAndColumns(hLv);

    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;

    LoadStr(IDS_SVC_DISPLAY, b, _countof(b));
    col.pszText = b;
    col.cx = 260;
    ListView_InsertColumn(hLv, 0, &col);

    LoadStr(IDS_SVC_INTERNAL, b, _countof(b));
    col.pszText = b;
    col.cx = 160;
    ListView_InsertColumn(hLv, 1, &col);

    LoadStr(IDS_SVC_STATUS_COL, b, _countof(b));
    col.pszText = b;
    col.cx = 100;
    ListView_InsertColumn(hLv, 2, &col);

    LoadStr(IDS_SVC_START_COL, b, _countof(b));
    col.pszText = b;
    col.cx = 120;
    ListView_InsertColumn(hLv, 3, &col);
}

static BOOL
Tm8ServicesShouldSkipRefresh(BOOL force, BOOL vscrollDragging, DWORD *pResumeDeadline)
{
    if (force)
        return FALSE;
    if (vscrollDragging)
        return TRUE;
    if (pResumeDeadline && *pResumeDeadline != 0)
    {
        DWORD now = GetTickCount();
        if ((LONG)(now - *pResumeDeadline) < 0)
            return TRUE;
        *pResumeDeadline = 0;
    }
    return FALSE;
}

void
Tm8Services_RefreshList(HWND hLv, BOOL force, BOOL vscrollDragging, DWORD *pResumeDeadline)
{
    SC_HANDLE scm = NULL;
    DWORD need = 0, nret = 0, resume = 0;
    BYTE *buf = NULL;
    LPENUM_SERVICE_STATUS_PROCESSW epBase;
    TM8_SVC_SORT *order = NULL;
    WCHAR bRun[48], bStop[48], bAuto[48], bMan[48], bDis[48], bBoot[48], bSys[48];
    int i;

    if (!hLv)
        return;
    if (Tm8ServicesShouldSkipRefresh(force, vscrollDragging, pResumeDeadline))
        return;

    LoadStr(IDS_SVC_RUNNING, bRun, _countof(bRun));
    LoadStr(IDS_SVC_STOPPED, bStop, _countof(bStop));
    LoadStr(IDS_SVC_START_AUTO, bAuto, _countof(bAuto));
    LoadStr(IDS_SVC_START_MAN, bMan, _countof(bMan));
    LoadStr(IDS_SVC_START_DIS, bDis, _countof(bDis));
    LoadStr(IDS_SVC_START_BOOT, bBoot, _countof(bBoot));
    LoadStr(IDS_SVC_START_SYS, bSys, _countof(bSys));

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm)
        return;

    if (!EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &need,
                               &nret, &resume, NULL))
    {
        DWORD err = GetLastError();
        if ((err != ERROR_MORE_DATA && err != ERROR_INSUFFICIENT_BUFFER) || need == 0)
        {
            CloseServiceHandle(scm);
            return;
        }
    }

    buf = (BYTE *)HeapAlloc(GetProcessHeap(), 0, need);
    if (!buf)
    {
        CloseServiceHandle(scm);
        return;
    }

    resume = 0;
    nret = 0;
    if (!EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, buf, need, &need,
                               &nret, &resume, NULL))
    {
        HeapFree(GetProcessHeap(), 0, buf);
        CloseServiceHandle(scm);
        return;
    }

    epBase = (LPENUM_SERVICE_STATUS_PROCESSW)buf;
    order = (TM8_SVC_SORT *)HeapAlloc(GetProcessHeap(), 0, sizeof(TM8_SVC_SORT) * (nret ? nret : 1));
    if (!order)
    {
        HeapFree(GetProcessHeap(), 0, buf);
        CloseServiceHandle(scm);
        return;
    }

    for (i = 0; i < (int)nret; i++)
    {
        order[i].ep = &epBase[i];
    }
    if (nret > 1)
        qsort(order, (size_t)nret, sizeof(order[0]), Tm8SvcCmpDisplay);

    ListView_DeleteAllItems(hLv);

    for (i = 0; i < (int)nret; i++)
    {
        LPENUM_SERVICE_STATUS_PROCESSW ep = order[i].ep;
        LVITEMW it;
        WCHAR startBuf[64];
        LPCWSTR disp, internal, stStr, startStr;
        SC_HANDLE svc;
        BYTE qstack[4096];
        LPQUERY_SERVICE_CONFIGW qc = (LPQUERY_SERVICE_CONFIGW)qstack;
        DWORD qneed = sizeof(qstack);
        int row;

        disp = (ep->lpDisplayName && ep->lpDisplayName[0]) ? ep->lpDisplayName : ep->lpServiceName;
        internal = ep->lpServiceName ? ep->lpServiceName : L"";

        stStr = Tm8SvcStateString(ep->ServiceStatusProcess.dwCurrentState, bRun, bStop);

        startBuf[0] = 0;
        startStr = startBuf;
        svc = OpenServiceW(scm, ep->lpServiceName, SERVICE_QUERY_CONFIG);
        if (svc)
        {
            if (QueryServiceConfigW(svc, qc, sizeof(qstack), &qneed))
                startStr = Tm8SvcStartTypeString(qc->dwStartType, bAuto, bMan, bDis, bBoot, bSys);
            CloseServiceHandle(svc);
        }
        if (!startStr || !startStr[0])
            startStr = L"";

        ZeroMemory(&it, sizeof(it));
        it.mask = LVIF_TEXT | LVIF_PARAM;
        it.iItem = i;
        it.iSubItem = 0;
        it.pszText = (LPWSTR)disp;
        it.lParam = 0;
        row = ListView_InsertItem(hLv, &it);

        ListView_SetItemText(hLv, row, 1, (LPWSTR)internal);
        ListView_SetItemText(hLv, row, 2, (LPWSTR)stStr);
        ListView_SetItemText(hLv, row, 3, (LPWSTR)startStr);
    }

    HeapFree(GetProcessHeap(), 0, order);
    HeapFree(GetProcessHeap(), 0, buf);
    CloseServiceHandle(scm);
}
