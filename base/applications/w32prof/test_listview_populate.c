#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

static double
TicksToMs(LONGLONG ticks, LONGLONG freq)
{
    if (freq <= 0)
        return 0.0;
    return ((double)ticks * 1000.0) / (double)freq;
}

static BOOL
ShouldStop(const ProfilerConfig* cfg)
{
    if (!cfg || !cfg->StopEvent)
        return FALSE;
    return (WaitForSingleObject(cfg->StopEvent, 0) == WAIT_OBJECT_0);
}

void
W32Prof_Test_ListViewPopulate(const ProfilerConfig* cfg)
{
    RECT r;
    int w, h;
    HWND hContainer = NULL;
    HWND hList = NULL;

    INITCOMMONCONTROLSEX icc;
    LVCOLUMN col;

    DWORD itemCount;
    DWORD passes = 0;

    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    if (!cfg || !cfg->hTestWnd)
        return;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    GetClientRect(cfg->hTestWnd, &r);
    w = r.right - r.left;
    h = r.bottom - r.top;
    if (w <= 0) w = 640;
    if (h <= 0) h = 480;

    hContainer = CreateWindowEx(0,
                                TEXT("STATIC"),
                                TEXT(""),
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                0, 0, w, h,
                                cfg->hTestWnd,
                                NULL,
                                GetModuleHandle(NULL),
                                NULL);
    if (!hContainer)
    {
        ResultsPrint(TEXT("ListView Populate: failed to create container window"));
        return;
    }

    hList = CreateWindowEx(WS_EX_CLIENTEDGE,
                           WC_LISTVIEW,
                           TEXT(""),
                           WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
                           0, 0, w, h,
                           hContainer,
                           NULL,
                           GetModuleHandle(NULL),
                           NULL);
    if (!hList)
    {
        ResultsPrint(TEXT("ListView Populate: failed to create listview"));
        DestroyWindow(hContainer);
        return;
    }

    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = (w > 8) ? (w - 8) : w;
    col.pszText = TEXT("Name");
    ListView_InsertColumn(hList, 0, &col);

    /* Keep this bounded: enough to feel like Explorer, not enough to explode memory. */
    itemCount = 10000;

    QueryPerformanceFrequency(&qf);
    W32Prof_FpsInit(&fps);

    if (!cfg->Continuous)
    {
        DWORD i;
        QueryPerformanceCounter(&q0);

        SendMessage(hList, WM_SETREDRAW, FALSE, 0);
        for (i = 0; i < itemCount; i++)
        {
            LVITEM item;
            item.mask = LVIF_TEXT;
            item.iItem = (int)i;
            item.iSubItem = 0;
            item.pszText = TEXT("Item");
            ListView_InsertItem(hList, &item);
        }
        SendMessage(hList, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hList, NULL, TRUE);
        UpdateWindow(hList);

        QueryPerformanceCounter(&q1);

        {
            double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
            double ips = (ms > 0.0) ? ((double)itemCount * 1000.0 / ms) : 0.0;
            ResultsPrint(TEXT("ListView Populate: %lu items in %.3f ms (%.2f items/s)"),
                         (ULONG)itemCount, ms, ips);
        }

        DestroyWindow(hContainer);
        return;
    }

    while (!ShouldStop(cfg))
    {
        DWORD i;

        passes++;

        SendMessage(hList, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(hList);

        for (i = 0; i < itemCount; i++)
        {
            LVITEM item;
            item.mask = LVIF_TEXT;
            item.iItem = (int)i;
            item.iSubItem = 0;
            item.pszText = TEXT("Item");
            ListView_InsertItem(hList, &item);

            if (((i & 255) == 0) && ShouldStop(cfg))
                break;
        }

        SendMessage(hList, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hList, NULL, TRUE);
        UpdateWindow(hList);

        W32Prof_FpsMaybeReport(cfg, &fps, passes, (LONGLONG)qf.QuadPart, TEXT("ListView Populate"));
    }

    DestroyWindow(hContainer);
}
