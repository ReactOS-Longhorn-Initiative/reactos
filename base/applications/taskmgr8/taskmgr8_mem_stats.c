/*
 * Memory statistics strip under the performance graph: three text columns.
 * Custom child with WS_HSCROLL so narrow windows match Windows Task Manager.
 */

#include "taskmgr8_mem_stats.h"
#include "taskmgr8_shared.h"

#include <windowsx.h>

#define TM8_MEM_STATS_MIN_INNER_W 620

static int s_MemStatsHScrollPos;
static int s_MemStatsContentW;
static BOOL s_MemStatsThumbTrackActive;

static void
DrawMemPerfStatsPanel(HDC hdc, const RECT *rcPanel)
{
    int w = rcPanel->right - rcPanel->left;
    int pad = 8;
    int gapCol = 14;
    int gapRow = 8;
    int colW, c0, c1, c2;
    int y, lblH, midH;
    HFONT oldF;
    TEXTMETRICW tm;
    RECT rL, rV;
    TM8_MEM_STATS_PAINT *ps = &s_MemStatsPaint;

    if (w < 120 || rcPanel->bottom <= rcPanel->top + 8)
        return;

    FillRect(hdc, rcPanel, Tm8ThemePanelBrush());
    SetBkMode(hdc, TRANSPARENT);

    colW = (w - 2 * pad - 2 * gapCol) / 3;
    if (colW < 80)
        colW = (w - 2 * pad) / 3;
    c0 = rcPanel->left + pad;
    c1 = c0 + colW + gapCol;
    c2 = c1 + colW + gapCol;

    oldF = (HFONT)SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    GetTextMetricsW(hdc, &tm);
    lblH = tm.tmHeight + 1;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    GetTextMetricsW(hdc, &tm);
    midH = tm.tmHeight + 2;

    y = rcPanel->top + 6;
    rL.left = c0;
    rL.right = c0 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c1Lbl1, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.left = c0;
    rV.right = c0 + colW;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c1Val1, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c1Lbl2, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c1Val2, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c1Lbl3, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c1Val3, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    y = rcPanel->top + 6;
    rL.left = c1;
    rL.right = c1 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c2Lbl1, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.left = c1;
    rV.right = c1 + colW;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c2Val1, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.left = c1;
    rL.right = c1 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c2Lbl2, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c2Val2, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.left = c1;
    rL.right = c1 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c2Lbl3, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c2Val3, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    y = rcPanel->top + 6;
    rL.left = c2;
    rL.right = c2 + colW;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c3Lbl1, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.left = c2;
    rV.right = c2 + colW;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c3Val1, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c3Lbl2, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c3Val2, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c3Lbl3, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c3Val3, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += lblH + 2 + midH + gapRow;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textMuted);
    DrawTextW(hdc, ps->c3Lbl4, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, g_Tm8Theme.textPrimary);
    DrawTextW(hdc, ps->c3Val4, -1, &rV, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    SelectObject(hdc, oldF);
}

static void
Tm8MemStatsUpdateScrollInfo(HWND hwnd)
{
    RECT rc;
    SCROLLINFO si;
    int clientW;
    int page;
    BOOL needScroll;

    if (!hwnd || !IsWindow(hwnd))
        return;
    GetClientRect(hwnd, &rc);
    clientW = rc.right - rc.left;
    if (clientW < 0)
        clientW = 0;

    s_MemStatsContentW = TM8_MEM_STATS_MIN_INNER_W;
    if (s_MemStatsContentW < clientW)
        s_MemStatsContentW = clientW;

    needScroll = (s_MemStatsContentW > clientW);

    if (!needScroll)
        s_MemStatsHScrollPos = 0;
    else
    {
        int maxPos = s_MemStatsContentW - clientW;
        if (s_MemStatsHScrollPos > maxPos)
            s_MemStatsHScrollPos = maxPos;
        if (s_MemStatsHScrollPos < 0)
            s_MemStatsHScrollPos = 0;
    }

    page = clientW > 0 ? clientW : 1;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = (s_MemStatsContentW > 1) ? (s_MemStatsContentW - 1) : 0;
    si.nPage = (UINT)page;
    si.nPos = s_MemStatsHScrollPos;
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
    ShowScrollBar(hwnd, SB_HORZ, needScroll);
}

static LRESULT CALLBACK
MemStatsPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        s_MemStatsHScrollPos = 0;
        s_MemStatsContentW = TM8_MEM_STATS_MIN_INNER_W;
        s_MemStatsThumbTrackActive = FALSE;
        return 0;

    case WM_SIZE:
        Tm8MemStatsUpdateScrollInfo(hwnd);
        return 0;

    case WM_HSCROLL:
    {
        RECT rc;
        int clientW, maxPos, pos = s_MemStatsHScrollPos;
        SCROLLINFO si;

        GetClientRect(hwnd, &rc);
        clientW = rc.right - rc.left;
        maxPos = (s_MemStatsContentW > clientW) ? (s_MemStatsContentW - clientW) : 0;

        switch (LOWORD(wParam))
        {
        case SB_LINELEFT:
            pos -= 24;
            break;
        case SB_LINERIGHT:
            pos += 24;
            break;
        case SB_PAGELEFT:
            pos -= clientW > 0 ? clientW : 1;
            break;
        case SB_PAGERIGHT:
            pos += clientW > 0 ? clientW : 1;
            break;
        case SB_THUMBTRACK:
            s_MemStatsThumbTrackActive = TRUE;
            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;
            if (GetScrollInfo(hwnd, SB_HORZ, &si))
                pos = si.nTrackPos;
            break;
        case SB_THUMBPOSITION:
            pos = (short)HIWORD(wParam);
            s_MemStatsThumbTrackActive = FALSE;
            break;
        case SB_ENDSCROLL:
            s_MemStatsThumbTrackActive = FALSE;
            Tm8MemStatsUpdateScrollInfo(hwnd);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        if (pos < 0)
            pos = 0;
        if (pos > maxPos)
            pos = maxPos;
        if (pos != s_MemStatsHScrollPos)
        {
            s_MemStatsHScrollPos = pos;
            Tm8MemStatsUpdateScrollInfo(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
        if (GetKeyState(VK_SHIFT) & 0x8000)
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            RECT rc;
            int clientW, maxPos, pos = s_MemStatsHScrollPos;
            const int step = 48;

            GetClientRect(hwnd, &rc);
            clientW = rc.right - rc.left;
            maxPos = (s_MemStatsContentW > clientW) ? (s_MemStatsContentW - clientW) : 0;
            if (delta > 0)
                pos -= step;
            else
                pos += step;
            if (pos < 0)
                pos = 0;
            if (pos > maxPos)
                pos = maxPos;
            if (pos != s_MemStatsHScrollPos)
            {
                s_MemStatsHScrollPos = pos;
                Tm8MemStatsUpdateScrollInfo(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT rcC, rcDraw;
        int drawW, dcSave;

        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rcC);
        drawW = s_MemStatsContentW;
        if (drawW < rcC.right)
            drawW = rcC.right;

        FillRect(hdc, &rcC, Tm8ThemePanelBrush());

        rcDraw.left = 0;
        rcDraw.top = 0;
        rcDraw.right = drawW;
        rcDraw.bottom = rcC.bottom;

        dcSave = SaveDC(hdc);
        SetViewportOrgEx(hdc, -s_MemStatsHScrollPos, 0, NULL);
        DrawMemPerfStatsPanel(hdc, &rcDraw);
        RestoreDC(hdc, dcSave);
        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

BOOL
Tm8MemStats_RegisterClass(HINSTANCE hInst)
{
    WNDCLASSW wc;

    if (GetClassInfoW(hInst, TM8_MEM_STATS_WNDCLASS, &wc))
        return TRUE;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MemStatsPanelProc;
    wc.cbWndExtra = 0;
    wc.cbClsExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TM8_MEM_STATS_WNDCLASS;
    if (!RegisterClassW(&wc))
        return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    return TRUE;
}

void
Tm8MemStats_UpdateScroll(void)
{
    if (!s_hwndMemStatsPanel || !IsWindow(s_hwndMemStatsPanel))
        return;
    if (s_MemStatsThumbTrackActive)
        return;
    Tm8MemStatsUpdateScrollInfo(s_hwndMemStatsPanel);
}

void
Tm8MemStats_OnLeaveMemoryPage(int page)
{
    if (page == PAGE_MEMORY)
        return;
    if (!s_hwndMemStatsPanel || !IsWindow(s_hwndMemStatsPanel))
        return;
    s_MemStatsThumbTrackActive = FALSE;
    s_MemStatsHScrollPos = 0;
    Tm8MemStatsUpdateScrollInfo(s_hwndMemStatsPanel);
}
