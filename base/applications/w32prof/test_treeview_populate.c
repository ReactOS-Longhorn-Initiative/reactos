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

static void
PopulateTree(HWND hTree, DWORD nodeCount, const ProfilerConfig* cfg)
{
    TVINSERTSTRUCT ins;
    HTREEITEM hRoot;
    DWORD i;

    ZeroMemory(&ins, sizeof(ins));
    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_LAST;
    ins.item.mask = TVIF_TEXT;
    ins.item.pszText = TEXT("Root");
    hRoot = (HTREEITEM)SendMessage(hTree, TVM_INSERTITEM, 0, (LPARAM)&ins);

    if (!hRoot)
        return;

    ZeroMemory(&ins, sizeof(ins));
    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_LAST;
    ins.item.mask = TVIF_TEXT;
    ins.item.pszText = TEXT("Node");

    for (i = 0; i < nodeCount; i++)
    {
        SendMessage(hTree, TVM_INSERTITEM, 0, (LPARAM)&ins);
        if (((i & 255) == 0) && ShouldStop(cfg))
            break;
    }

    TreeView_Expand(hTree, hRoot, TVE_EXPAND);
}

void
W32Prof_Test_TreeViewPopulate(const ProfilerConfig* cfg)
{
    RECT r;
    int w, h;
    HWND hContainer = NULL;
    HWND hTree = NULL;

    INITCOMMONCONTROLSEX icc;

    DWORD nodeCount;
    DWORD passes = 0;

    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    if (!cfg || !cfg->hTestWnd)
        return;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TREEVIEW_CLASSES;
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
        ResultsPrint(TEXT("TreeView Populate: failed to create container window"));
        return;
    }

    hTree = CreateWindowEx(WS_EX_CLIENTEDGE,
                           WC_TREEVIEW,
                           TEXT(""),
                           WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
                           0, 0, w, h,
                           hContainer,
                           NULL,
                           GetModuleHandle(NULL),
                           NULL);
    if (!hTree)
    {
        ResultsPrint(TEXT("TreeView Populate: failed to create treeview"));
        DestroyWindow(hContainer);
        return;
    }

    nodeCount = 10000;

    QueryPerformanceFrequency(&qf);
    W32Prof_FpsInit(&fps);

    if (!cfg->Continuous)
    {
        QueryPerformanceCounter(&q0);

        SendMessage(hTree, WM_SETREDRAW, FALSE, 0);
        TreeView_DeleteAllItems(hTree);
        PopulateTree(hTree, nodeCount, cfg);
        SendMessage(hTree, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hTree, NULL, TRUE);
        UpdateWindow(hTree);

        QueryPerformanceCounter(&q1);

        {
            double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
            double nps = (ms > 0.0) ? ((double)nodeCount * 1000.0 / ms) : 0.0;
            ResultsPrint(TEXT("TreeView Populate: %lu nodes in %.3f ms (%.2f nodes/s)"),
                         (ULONG)nodeCount, ms, nps);
        }

        DestroyWindow(hContainer);
        return;
    }

    while (!ShouldStop(cfg))
    {
        passes++;

        SendMessage(hTree, WM_SETREDRAW, FALSE, 0);
        TreeView_DeleteAllItems(hTree);
        PopulateTree(hTree, nodeCount, cfg);
        SendMessage(hTree, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hTree, NULL, TRUE);
        UpdateWindow(hTree);

        W32Prof_FpsMaybeReport(cfg, &fps, passes, (LONGLONG)qf.QuadPart, TEXT("TreeView Populate"));
    }

    DestroyWindow(hContainer);
}
