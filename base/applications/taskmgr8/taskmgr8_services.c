/*
 * PROJECT:     ReactOS — Task Manager 8
 * LICENSE:     GPL-2.0-or-later OR LGPL-2.1-or-later
 */

#include "taskmgr8_services.h"
#include "taskmgr8_helpers.h"
#include "taskmgr8_listutil.h"

#include <winsvc.h>
#include <winreg.h>

#ifndef SC_STATUS_PROCESS_INFO
#define SC_STATUS_PROCESS_INFO 0
#endif

#define TM8_SVC_OPEN_ACCESS                                                                 \
    (SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_START | SERVICE_STOP |             \
     SERVICE_INTERROGATE | SERVICE_USER_DEFINED_CONTROL)
#define TM8_SVC_READ_ACCESS (SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG)

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

    {
        WCHAR selKey[256], topKey[256];
        int nOld, topIdx, selRow;

        selKey[0] = 0;
        topKey[0] = 0;
        nOld = ListView_GetItemCount(hLv);
        topIdx = ListView_GetTopIndex(hLv);
        selRow = ListView_GetNextItem(hLv, -1, LVNI_SELECTED);
        if (selRow >= 0 && selRow < nOld)
            ListView_GetItemText(hLv, selRow, 1, selKey, _countof(selKey));
        if (topIdx >= 0 && topIdx < nOld)
            ListView_GetItemText(hLv, topIdx, 1, topKey, _countof(topKey));

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

        if (topKey[0])
        {
            int r, nlv = ListView_GetItemCount(hLv);
            for (r = 0; r < nlv; r++)
            {
                WCHAR buf[256];
                ListView_GetItemText(hLv, r, 1, buf, _countof(buf));
                if (lstrcmpiW(buf, topKey) == 0)
                {
                    SendMessageW(hLv, LVM_SETTOPINDEX, (WPARAM)r, 0);
                    break;
                }
            }
        }
        if (selKey[0])
        {
            int r, nlv = ListView_GetItemCount(hLv);
            for (r = 0; r < nlv; r++)
            {
                WCHAR buf[256];
                ListView_GetItemText(hLv, r, 1, buf, _countof(buf));
                if (lstrcmpiW(buf, selKey) == 0)
                {
                    ListView_SetItemState(hLv, r, LVIS_FOCUSED | LVIS_SELECTED,
                                          LVIS_FOCUSED | LVIS_SELECTED);
                    break;
                }
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, order);
    HeapFree(GetProcessHeap(), 0, buf);
    CloseServiceHandle(scm);
}

static BOOL
Tm8SvcGetSelectedInternalName(HWND hLv, WCHAR *name, DWORD cchName)
{
    int i;

    if (!hLv || !name || cchName < 2)
        return FALSE;
    name[0] = 0;
    i = ListView_GetNextItem(hLv, -1, LVNI_SELECTED);
    if (i < 0)
        return FALSE;
    ListView_GetItemText(hLv, i, 1, name, cchName);
    return name[0] != 0;
}

static void
Tm8SvcShowError(HWND hwnd, UINT idsMsg)
{
    WCHAR t[320], cap[96];
    LoadStr(idsMsg, t, _countof(t));
    LoadStr(IDS_APP_TITLE, cap, _countof(cap));
    MessageBoxW(hwnd, t, cap, MB_OK | MB_ICONINFORMATION);
}

static BOOL
Tm8SvcQueryState(SC_HANDLE svc, DWORD *pState)
{
    SERVICE_STATUS_PROCESS ssp;
    DWORD need;

    if (!pState)
        return FALSE;
    if (!QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &need))
        return FALSE;
    *pState = ssp.dwCurrentState;
    return TRUE;
}

static BOOL
Tm8SvcWaitStopped(SC_HANDLE svc, DWORD timeoutMs)
{
    DWORD t0 = GetTickCount();

    for (;;)
    {
        DWORD st;
        if (!Tm8SvcQueryState(svc, &st))
            return FALSE;
        if (st == SERVICE_STOPPED)
            return TRUE;
        if (GetTickCount() - t0 > timeoutMs)
            return FALSE;
        Sleep(200);
    }
}

void
Tm8Services_UpdateContextMenu(HMENU hMenu, HWND hLv)
{
    WCHAR name[256];
    SC_HANDLE scm = NULL, svcOp = NULL, svcRo = NULL;
    DWORD st = 0;
    BOOL haveState = FALSE;

    EnableMenuItem(hMenu, ID_SVC_START, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, ID_SVC_STOP, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, ID_SVC_RESTART, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, IDM_PROPERTIES, MF_BYCOMMAND | MF_GRAYED);

    if (!hMenu || !hLv)
        return;
    if (!Tm8SvcGetSelectedInternalName(hLv, name, _countof(name)))
        return;

    EnableMenuItem(hMenu, IDM_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
        return;

    svcOp = OpenServiceW(scm, name, TM8_SVC_OPEN_ACCESS);
    svcRo = svcOp ? svcOp : OpenServiceW(scm, name, TM8_SVC_READ_ACCESS);
    if (!svcRo)
    {
        CloseServiceHandle(scm);
        return;
    }

    if (Tm8SvcQueryState(svcRo, &st))
        haveState = TRUE;

    if (svcOp && haveState)
    {
        if (st == SERVICE_STOPPED)
            EnableMenuItem(hMenu, ID_SVC_START, MF_BYCOMMAND | MF_ENABLED);
        if (st == SERVICE_RUNNING || st == SERVICE_PAUSED)
        {
            EnableMenuItem(hMenu, ID_SVC_STOP, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_SVC_RESTART, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    if (svcOp && svcOp != svcRo)
        CloseServiceHandle(svcOp);
    CloseServiceHandle(svcRo);
    CloseServiceHandle(scm);
}

static void
Tm8SvcAppendLine(WCHAR *buf, size_t cchBuf, const WCHAR *line)
{
    StringCchCatW(buf, cchBuf, line);
    StringCchCatW(buf, cchBuf, L"\r\n");
}

void
Tm8ServiceShowProperties(HWND hwnd, HWND hLv)
{
    WCHAR internal[256], disp[512], cap[96], regPath[512], desc[512];
    WCHAR *msg = NULL;
    size_t msgCch = 8192;
    SC_HANDLE scm = NULL, svc = NULL;
    LPQUERY_SERVICE_CONFIGW qc = NULL;
    DWORD need = 0, qerr;
    HKEY hk = NULL;
    int row;
    SERVICE_STATUS_PROCESS ssp;
    LPCWSTR stStr;

    LoadStr(IDS_SVC_PROPS_CAP, cap, _countof(cap));

    if (!Tm8SvcGetSelectedInternalName(hLv, internal, _countof(internal)))
    {
        WCHAR none[128];
        LoadStr(IDS_SVC_PROPS_NONAME, none, _countof(none));
        MessageBoxW(hwnd, none, cap, MB_OK | MB_ICONINFORMATION);
        return;
    }

    disp[0] = 0;
    row = ListView_GetNextItem(hLv, -1, LVNI_SELECTED);
    if (row >= 0)
        ListView_GetItemText(hLv, row, 0, disp, _countof(disp));

    msg = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, msgCch * sizeof(WCHAR));
    if (!msg)
        return;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
        goto fail;

    svc = OpenServiceW(scm, internal, TM8_SVC_READ_ACCESS);
    if (!svc)
        goto fail;

    QueryServiceConfigW(svc, NULL, 0, &need);
    qerr = GetLastError();
    if (qerr != ERROR_INSUFFICIENT_BUFFER || need == 0)
        goto fail;

    qc = (LPQUERY_SERVICE_CONFIGW)HeapAlloc(GetProcessHeap(), 0, need);
    if (!qc)
        goto fail;
    if (!QueryServiceConfigW(svc, qc, need, &need))
        goto fail;

    ZeroMemory(&ssp, sizeof(ssp));
    if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &need))
    {
        WCHAR bRun[48], bStop[48];
        LoadStr(IDS_SVC_RUNNING, bRun, _countof(bRun));
        LoadStr(IDS_SVC_STOPPED, bStop, _countof(bStop));
        stStr = Tm8SvcStateString(ssp.dwCurrentState, bRun, bStop);
    }
    else
        stStr = L"";

    StringCchPrintfW(msg, msgCch, L"%s: %s\r\n\r\n", disp[0] ? disp : internal, internal);
    StringCchCatW(msg, msgCch, L"Status: ");
    StringCchCatW(msg, msgCch, stStr);
    StringCchCatW(msg, msgCch, L"\r\n\r\n");

    if (qc->lpBinaryPathName && qc->lpBinaryPathName[0])
    {
        Tm8SvcAppendLine(msg, msgCch, L"Path to executable:");
        Tm8SvcAppendLine(msg, msgCch, qc->lpBinaryPathName);
        StringCchCatW(msg, msgCch, L"\r\n");
    }
    if (qc->lpLoadOrderGroup && qc->lpLoadOrderGroup[0])
    {
        Tm8SvcAppendLine(msg, msgCch, L"Load order group:");
        Tm8SvcAppendLine(msg, msgCch, qc->lpLoadOrderGroup);
        StringCchCatW(msg, msgCch, L"\r\n");
    }

    {
        WCHAR bAuto[48], bMan[48], bDis[48], bBoot[48], bSys[48];
        LPCWSTR stStart;
        LoadStr(IDS_SVC_START_AUTO, bAuto, _countof(bAuto));
        LoadStr(IDS_SVC_START_MAN, bMan, _countof(bMan));
        LoadStr(IDS_SVC_START_DIS, bDis, _countof(bDis));
        LoadStr(IDS_SVC_START_BOOT, bBoot, _countof(bBoot));
        LoadStr(IDS_SVC_START_SYS, bSys, _countof(bSys));
        stStart = Tm8SvcStartTypeString(qc->dwStartType, bAuto, bMan, bDis, bBoot, bSys);
        if (stStart && stStart[0])
        {
            Tm8SvcAppendLine(msg, msgCch, L"Startup type:");
            Tm8SvcAppendLine(msg, msgCch, stStart);
            StringCchCatW(msg, msgCch, L"\r\n");
        }
    }

    StringCchPrintfW(regPath, _countof(regPath),
                     L"SYSTEM\\CurrentControlSet\\Services\\%s", internal);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        DWORD sz = sizeof(desc);
        desc[0] = 0;
        if (RegQueryValueExW(hk, L"Description", NULL, NULL, (LPBYTE)desc, &sz) == ERROR_SUCCESS &&
            desc[0])
        {
            Tm8SvcAppendLine(msg, msgCch, L"Description:");
            Tm8SvcAppendLine(msg, msgCch, desc);
        }
        RegCloseKey(hk);
    }

    MessageBoxW(hwnd, msg, cap, MB_OK | MB_ICONINFORMATION);

    HeapFree(GetProcessHeap(), 0, qc);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    HeapFree(GetProcessHeap(), 0, msg);
    return;

fail:
    Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
    if (qc)
        HeapFree(GetProcessHeap(), 0, qc);
    if (svc)
        CloseServiceHandle(svc);
    if (scm)
        CloseServiceHandle(scm);
    HeapFree(GetProcessHeap(), 0, msg);
}

void
Tm8ServiceStart(HWND hwnd, HWND hLv, DWORD *pResumeDeadline)
{
    WCHAR name[256];
    SC_HANDLE scm = NULL, svc = NULL;

    if (!Tm8SvcGetSelectedInternalName(hLv, name, _countof(name)))
        return;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
    {
        Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
        return;
    }
    svc = OpenServiceW(scm, name, SERVICE_START | SERVICE_QUERY_STATUS);
    if (!svc)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_ACCESS);
        else
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
        CloseServiceHandle(scm);
        return;
    }

    if (!StartServiceW(svc, 0, NULL))
    {
        DWORD e = GetLastError();
        if (e == ERROR_ACCESS_DENIED)
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_ACCESS);
        else if (e != ERROR_SERVICE_ALREADY_RUNNING)
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    Tm8Services_RefreshList(hLv, TRUE, FALSE, pResumeDeadline);
}

void
Tm8ServiceStop(HWND hwnd, HWND hLv, DWORD *pResumeDeadline)
{
    WCHAR name[256];
    SC_HANDLE scm = NULL, svc = NULL;
    SERVICE_STATUS ss;

    if (!Tm8SvcGetSelectedInternalName(hLv, name, _countof(name)))
        return;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
    {
        Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
        return;
    }
    svc = OpenServiceW(scm, name, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_ACCESS);
        else
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
        CloseServiceHandle(scm);
        return;
    }

    ZeroMemory(&ss, sizeof(ss));
    if (!ControlService(svc, SERVICE_CONTROL_STOP, &ss))
    {
        DWORD e = GetLastError();
        if (e == ERROR_ACCESS_DENIED)
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_ACCESS);
        else if (e != ERROR_SERVICE_NOT_ACTIVE)
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    Tm8Services_RefreshList(hLv, TRUE, FALSE, pResumeDeadline);
}

void
Tm8ServiceRestart(HWND hwnd, HWND hLv, DWORD *pResumeDeadline)
{
    WCHAR name[256];
    SC_HANDLE scm = NULL, svc = NULL;
    SERVICE_STATUS ss;

    if (!Tm8SvcGetSelectedInternalName(hLv, name, _countof(name)))
        return;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
    {
        Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
        return;
    }
    svc = OpenServiceW(scm, name, SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_ACCESS);
        else
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
        CloseServiceHandle(scm);
        return;
    }

    ZeroMemory(&ss, sizeof(ss));
    if (!ControlService(svc, SERVICE_CONTROL_STOP, &ss))
    {
        DWORD e = GetLastError();
        if (e == ERROR_ACCESS_DENIED)
        {
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_ACCESS);
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return;
        }
        if (e != ERROR_SERVICE_NOT_ACTIVE)
        {
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return;
        }
    }

    if (!Tm8SvcWaitStopped(svc, 60000))
    {
        Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        Tm8Services_RefreshList(hLv, TRUE, FALSE, pResumeDeadline);
        return;
    }

    if (!StartServiceW(svc, 0, NULL))
    {
        DWORD e = GetLastError();
        if (e == ERROR_ACCESS_DENIED)
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_ACCESS);
        else
            Tm8SvcShowError(hwnd, IDS_SVC_ERR_GENERIC);
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    Tm8Services_RefreshList(hLv, TRUE, FALSE, pResumeDeadline);
}
