/*
 * PROJECT:     ReactOS — Task Manager 8
 * LICENSE:     GPL-2.0-or-later OR LGPL-2.1-or-later
 */

#include "taskmgr8_details.h"
#include "taskmgr8_helpers.h"
#include "taskmgr8_listutil.h"
#include "taskmgr8_shared.h"

#define TM8_ProcessCommandLineInformation 60
#define TM8_DETAILS_ROW_CAP 2048

typedef struct _TM8_UNICODE_STRING_CMD
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} TM8_UNICODE_STRING_CMD;

typedef LONG(NTAPI *PFN_NtQueryInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass,
                                                 PVOID ProcessInformation, ULONG ProcessInformationLength,
                                                 PULONG ReturnLength);

typedef struct _TM8_DETAILS_ROW
{
    DWORD pid;
    DWORD sessionId;
    double cpuPct;
    SIZE_T ws;
    int iconIdx;
    WCHAR exe[MAX_PATH];
    WCHAR user[128];
    WCHAR cmdline[512];
} TM8_DETAILS_ROW;

static PFN_NtQueryInformationProcess s_pfnNtQueryInformationProcess;

static int __cdecl
Tm8DetailsCmpRows(const void *a, const void *b)
{
    const TM8_DETAILS_ROW *x = (const TM8_DETAILS_ROW *)a;
    const TM8_DETAILS_ROW *y = (const TM8_DETAILS_ROW *)b;
    int c = _wcsicmp(x->exe, y->exe);
    if (c != 0)
        return c;
    if (x->pid < y->pid)
        return -1;
    if (x->pid > y->pid)
        return 1;
    return 0;
}

static void
Tm8EnsureNtQueryInformationProcess(void)
{
    HMODULE ntdll;

    if (s_pfnNtQueryInformationProcess)
        return;
    ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll)
        return;
    s_pfnNtQueryInformationProcess =
        (PFN_NtQueryInformationProcess)(void *)GetProcAddress(ntdll, "NtQueryInformationProcess");
}

static BOOL
Tm8TryReadRemoteCommandLine(HANDLE hProc, WCHAR *out, DWORD cchOut)
{
    TM8_UNICODE_STRING_CMD us;
    ULONG retLen = 0;
    SIZE_T want, got;
    NTSTATUS st;

    Tm8EnsureNtQueryInformationProcess();
    if (!s_pfnNtQueryInformationProcess || cchOut < 2)
        return FALSE;

    ZeroMemory(&us, sizeof(us));
    st = s_pfnNtQueryInformationProcess(hProc, TM8_ProcessCommandLineInformation, &us, sizeof(us), &retLen);
    if (!TM8_NT_SUCCESS(st) || us.Length == 0 || !us.Buffer)
        return FALSE;

    want = (SIZE_T)us.Length;
    if (want > (cchOut - 1) * sizeof(WCHAR))
        want = (cchOut - 1) * sizeof(WCHAR);

    if (!ReadProcessMemory(hProc, us.Buffer, out, want, &got))
        return FALSE;

    out[got / sizeof(WCHAR)] = 0;
    return TRUE;
}

static BOOL
Tm8FillProcessUser(HANDLE hProc, WCHAR *user, DWORD cchUser)
{
    HANDLE tok = NULL;
    DWORD need = 0;
    TOKEN_USER *tu = NULL;
    WCHAR name[96], dom[96];
    DWORD nl, dl;
    SID_NAME_USE nu;

    user[0] = 0;
    if (!OpenProcessToken(hProc, TOKEN_QUERY, &tok))
        return FALSE;

    if (!GetTokenInformation(tok, TokenUser, NULL, 0, &need) && need == 0)
    {
        CloseHandle(tok);
        return FALSE;
    }

    tu = (TOKEN_USER *)HeapAlloc(GetProcessHeap(), 0, need);
    if (!tu || !GetTokenInformation(tok, TokenUser, tu, need, &need))
    {
        if (tu)
            HeapFree(GetProcessHeap(), 0, tu);
        CloseHandle(tok);
        return FALSE;
    }

    nl = _countof(name);
    dl = _countof(dom);
    if (LookupAccountSidW(NULL, tu->User.Sid, name, &nl, dom, &dl, &nu))
    {
        if (dom[0] && (DWORD)(nl + dl + 2) < cchUser)
            StringCchPrintfW(user, cchUser, L"%s\\%s", dom, name);
        else
            StringCchCopyW(user, cchUser, name);
    }

    HeapFree(GetProcessHeap(), 0, tu);
    CloseHandle(tok);
    return user[0] != 0;
}

void
Tm8Details_SetupListView(HWND hLv)
{
    WCHAR b[96];
    LVCOLUMNW col;

    if (!hLv)
        return;

    Tm8LvResetContentAndColumns(hLv);

    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;

    LoadStr(IDS_COL_NAME, b, _countof(b));
    col.pszText = b;
    col.cx = 200;
    ListView_InsertColumn(hLv, 0, &col);

    LoadStr(IDS_COL_PID, b, _countof(b));
    col.pszText = b;
    col.cx = 72;
    ListView_InsertColumn(hLv, 1, &col);

    LoadStr(IDS_COL_STATUS, b, _countof(b));
    col.pszText = b;
    col.cx = 88;
    ListView_InsertColumn(hLv, 2, &col);

    LoadStr(IDS_COL_SESSION, b, _countof(b));
    col.pszText = b;
    col.cx = 72;
    ListView_InsertColumn(hLv, 3, &col);

    LoadStr(IDS_COL_USER, b, _countof(b));
    col.pszText = b;
    col.cx = 160;
    ListView_InsertColumn(hLv, 4, &col);

    LoadStr(IDS_COL_CPU, b, _countof(b));
    col.pszText = b;
    col.cx = 88;
    ListView_InsertColumn(hLv, 5, &col);

    LoadStr(IDS_COL_MEM, b, _countof(b));
    col.pszText = b;
    col.cx = 120;
    ListView_InsertColumn(hLv, 6, &col);

    LoadStr(IDS_COL_CMDLINE, b, _countof(b));
    col.pszText = b;
    col.cx = 360;
    ListView_InsertColumn(hLv, 7, &col);
}

static BOOL
Tm8DetailsShouldSkipRefresh(BOOL force, BOOL vscrollDragging, DWORD *pResumeDeadline)
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
Tm8Details_RefreshList(HWND hLv, DWORD msElapsed, BOOL force, BOOL vscrollDragging, DWORD *pResumeDeadline,
                       TM8_DETAILS_ROW_METRICS *metrics, int metricsMax, int *outCount, SIZE_T *outMemMax)
{
    HANDLE hSnap;
    PROCESSENTRY32W pe;
    SYSTEM_INFO si;
    UINT nCpu;
    TM8_DETAILS_ROW *rows = NULL;
    int nRows = 0;
    SIZE_T memMax = 1;
    WCHAR statRun[48];
    int i;

    if (!hLv)
        return;
    if (Tm8DetailsShouldSkipRefresh(force, vscrollDragging, pResumeDeadline))
        return;

    if (outCount)
        *outCount = 0;
    if (outMemMax)
        *outMemMax = 1;

    GetSystemInfo(&si);
    nCpu = si.dwNumberOfProcessors;
    if (nCpu == 0)
        nCpu = 1;

    rows = (TM8_DETAILS_ROW *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        sizeof(TM8_DETAILS_ROW) * TM8_DETAILS_ROW_CAP);
    if (!rows)
        return;

    LoadStr(IDS_STAT_RUNNING, statRun, _countof(statRun));

    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        HeapFree(GetProcessHeap(), 0, rows);
        return;
    }

    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe))
    {
        do
        {
            TM8_DETAILS_ROW *r;
            HANDLE hOpen;
            PROCESS_MEMORY_COUNTERS pmc;
            ULONGLONG dummy;
            WCHAR path[MAX_PATH];

            if (nRows >= TM8_DETAILS_ROW_CAP)
                break;

            r = &rows[nRows];
            r->pid = pe.th32ProcessID;
            StringCchCopyW(r->exe, _countof(r->exe), pe.szExeFile);
            r->cpuPct = Tm8ProcessCpuUsagePercent(pe.th32ProcessID, &dummy, msElapsed, nCpu);
            r->ws = 0;
            r->sessionId = 0;
            r->user[0] = 0;
            r->cmdline[0] = 0;
            path[0] = 0;

            if (!ProcessIdToSessionId(r->pid, &r->sessionId))
                r->sessionId = (DWORD)-1;

            hOpen = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
            if (!hOpen)
                hOpen = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);

            if (hOpen)
            {
                pmc.cb = sizeof(pmc);
                if (GetProcessMemoryInfo(hOpen, &pmc, sizeof(pmc)))
                    r->ws = pmc.WorkingSetSize;
                Tm8QueryProcessImagePath(hOpen, path, _countof(path));
                Tm8FillProcessUser(hOpen, r->user, _countof(r->user));
                if (!Tm8TryReadRemoteCommandLine(hOpen, r->cmdline, _countof(r->cmdline)) && path[0])
                    StringCchCopyW(r->cmdline, _countof(r->cmdline), path);
                CloseHandle(hOpen);
            }

            if (r->ws > memMax)
                memMax = r->ws;

            r->iconIdx = Tm8IconForExePath(path[0] ? path : NULL);
            nRows++;
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);

    if (nRows > 1)
        qsort(rows, (size_t)nRows, sizeof(rows[0]), Tm8DetailsCmpRows);

    ListView_DeleteAllItems(hLv);

    for (i = 0; i < nRows; i++)
    {
        TM8_DETAILS_ROW *r = &rows[i];
        LVITEMW it;
        WCHAR num[32], sess[32], line[96];
        int row;

        ZeroMemory(&it, sizeof(it));
        it.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        it.iItem = i;
        it.iSubItem = 0;
        it.pszText = r->exe;
        it.lParam = (LPARAM)r->pid;
        it.iImage = r->iconIdx;
        row = ListView_InsertItem(hLv, &it);

        StringCchPrintfW(num, _countof(num), L"%lu", (ULONG)r->pid);
        ListView_SetItemText(hLv, row, 1, num);

        ListView_SetItemText(hLv, row, 2, statRun);

        if (r->sessionId == (DWORD)-1)
            sess[0] = 0;
        else
            StringCchPrintfW(sess, _countof(sess), L"%lu", (ULONG)r->sessionId);
        ListView_SetItemText(hLv, row, 3, sess);

        ListView_SetItemText(hLv, row, 4, r->user);

        StringCchPrintfW(line, _countof(line), L"%.1f%%", r->cpuPct);
        ListView_SetItemText(hLv, row, 5, line);

        {
            double mb = (double)r->ws / (1024.0 * 1024.0);
            Tm8FmtMbComma1(mb, line, _countof(line));
            ListView_SetItemText(hLv, row, 6, line);
        }

        ListView_SetItemText(hLv, row, 7, r->cmdline);

        if (metrics && i < metricsMax)
        {
            metrics[i].cpuPct = r->cpuPct;
            metrics[i].ws = r->ws;
        }
    }

    if (outCount)
        *outCount = nRows;
    if (outMemMax)
        *outMemMax = memMax;

    HeapFree(GetProcessHeap(), 0, rows);
}
