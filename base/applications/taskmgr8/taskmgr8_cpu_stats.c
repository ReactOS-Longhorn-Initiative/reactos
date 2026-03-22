/*
 * CPU statistics strip under the performance graph: three text columns (live counters,
 * socket/core/virtualization, caches). Implemented as a custom child with WS_HSCROLL so
 * narrow windows match Windows Task Manager (horizontal scroll, no overlap).
 */

#include "taskmgr8_cpu_stats.h"
#include "taskmgr8_helpers.h"
#include "taskmgr8_shared.h"

#include <windowsx.h>

#define TM8_CPU_STATS_MIN_INNER_W 600

typedef struct _TM8_CPU_STATS_PAINT
{
    WCHAR utilLbl[48], utilVal[32];
    WCHAR speedLbl[48], speedVal[64];
    WCHAR procLbl[48], procVal[40];
    WCHAR threadLbl[48], threadVal[40];
    WCHAR handleLbl[48], handleVal[40];
    WCHAR upLbl[48], upVal[96];
    WCHAR specMidLblBuf[768];
    WCHAR specMidValBuf[768];
    WCHAR specCacheLblBuf[768];
    WCHAR specCacheValBuf[768];
} TM8_CPU_STATS_PAINT;

static TM8_CPU_STATS_PAINT s_CpuStatsPaint;
static int s_CpuStatsHScrollPos;
static int s_CpuStatsContentW;

static void
Tm8CopyOneLine(const WCHAR **pp, WCHAR *dst, size_t cchDst)
{
    const WCHAR *p = *pp;
    size_t n = 0;

    if (!dst || cchDst == 0)
        return;
    if (!p)
    {
        dst[0] = 0;
        return;
    }
    while (*p && *p != L'\r' && *p != L'\n' && n + 1 < cchDst)
        dst[n++] = *p++;
    dst[n] = 0;
    if (*p == L'\r' && p[1] == L'\n')
        p += 2;
    else if (*p == L'\n' || *p == L'\r')
        p++;
    *pp = p;
}

static void
DrawCpuSpecKvColumn(HDC hdc, const RECT *rcCol, const WCHAR *pl, const WCHAR *pv, int rowSpecH)
{
    int y = rcCol->top;
    WCHAR lineL[128], lineV[256];
    int guard = 0;
    int specWcol = rcCol->right - rcCol->left;
    int valBand;

    if (specWcol < 32 || !pl || !pv)
        return;

    valBand = (specWcol * 42) / 100;
    if (valBand < 52)
        valBand = 52;
    if (valBand > 120)
        valBand = 120;
    if (valBand > specWcol - 10)
        valBand = (specWcol * 48) / 100;
    if (valBand < 40)
        valBand = 40;

    while (*pl && *pv && y + rowSpecH <= rcCol->bottom && guard++ < 48)
    {
        RECT rL, rVal;

        Tm8CopyOneLine(&pl, lineL, _countof(lineL));
        Tm8CopyOneLine(&pv, lineV, _countof(lineV));
        rL.left = rcCol->left;
        rL.right = rcCol->right - valBand - 4;
        rL.top = y;
        rL.bottom = y + rowSpecH;
        SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(hdc, RGB(96, 96, 96));
        DrawTextW(hdc, lineL, -1, &rL, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        rVal.left = rcCol->right - valBand;
        rVal.right = rcCol->right;
        rVal.top = y;
        rVal.bottom = y + rowSpecH;
        SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(hdc, RGB(32, 32, 32));
        DrawTextW(hdc, lineV, -1, &rVal,
                  DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        y += rowSpecH + 1;
    }
}

static void
DrawCpuPerfStatsPanel(HDC hdc, const RECT *rcPanel)
{
    int w = rcPanel->right - rcPanel->left;
    int sep1, sep2, y;
    HFONT oldF;
    TEXTMETRICW tmLbl, tmMid, tmSpec;
    int gapRow = 6;
    int lblH, midH, rowSpecH;
    int half, tw, c0, c1, c2;
    RECT rcLeft, rcMid, rcCache;
    RECT rU, rS, rL, rV;

    if (w < 100 || rcPanel->bottom <= rcPanel->top + 12)
        return;

    FillRect(hdc, rcPanel, (HBRUSH)GetStockObject(WHITE_BRUSH));

    /* Three equal thirds: live metrics | topology/virt | caches (Win10/11 style, no rules). */
    {
        const int padOut = 8;
        const int inset = 6;
        int L = rcPanel->left + padOut;
        int R = rcPanel->right - padOut;
        int W = R - L;
        int w1, w2;

        if (W < 60)
            return;
        w1 = W / 3;
        w2 = W / 3;
        sep1 = L + w1;
        sep2 = L + w1 + w2;

        rcLeft.left = L + inset;
        rcLeft.right = sep1 - inset;
        rcMid.left = sep1 + inset;
        rcMid.right = sep2 - inset;
        rcCache.left = sep2 + inset;
        rcCache.right = R - inset;

        if (rcLeft.right < rcLeft.left + 40)
            rcLeft.right = rcLeft.left + 40;
        if (rcMid.right < rcMid.left + 40)
            rcMid.right = rcMid.left + 40;
        if (rcCache.right < rcCache.left + 40)
            rcCache.right = rcCache.left + 40;
    }
    rcLeft.top = rcPanel->top + 6;
    rcLeft.bottom = rcPanel->bottom - 6;
    rcMid.top = rcLeft.top;
    rcMid.bottom = rcLeft.bottom;
    rcCache.top = rcLeft.top;
    rcCache.bottom = rcLeft.bottom;

    SetBkMode(hdc, TRANSPARENT);

    oldF = (HFONT)SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    GetTextMetricsW(hdc, &tmLbl);
    lblH = tmLbl.tmHeight + 1;

    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    GetTextMetricsW(hdc, &tmMid);
    midH = tmMid.tmHeight + 2;

    y = rcLeft.top;
    half = (rcLeft.right - rcLeft.left) / 2;
    rU.left = rcLeft.left;
    rU.right = rcLeft.left + half - 6;
    rU.top = y;
    rU.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.utilLbl, -1, &rU, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rU.top = y + lblH + 2;
    rU.bottom = rU.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.utilVal, -1, &rU,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    rS.left = rcLeft.left + half;
    rS.right = rcLeft.right;
    rS.top = y;
    rS.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.speedLbl, -1, &rS, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rS.top = y + lblH + 2;
    rS.bottom = rS.top + midH;
    if (s_CpuStatsPaint.speedVal[0])
    {
        SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(hdc, RGB(32, 32, 32));
        DrawTextW(hdc, s_CpuStatsPaint.speedVal, -1, &rS,
                  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    }

    y += lblH + 2 + midH + gapRow;

    {
        int innerL = rcLeft.right - rcLeft.left;
        int gapMid = 10;
        tw = (innerL - 2 * gapMid) / 3;
        if (tw < 52)
            tw = 52;
        c0 = rcLeft.left;
        c1 = c0 + tw + gapMid;
        c2 = c1 + tw + gapMid;
    }

    rL.left = c0;
    rL.right = c0 + tw - 2;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.procLbl, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rV.left = rL.left;
    rV.right = rL.right;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.procVal, -1, &rV,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    rL.left = c1;
    rL.right = c1 + tw - 2;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.threadLbl, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rV.left = rL.left;
    rV.right = rL.right;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.threadVal, -1, &rV,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    rL.left = c2;
    rL.right = rcLeft.right;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.handleLbl, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rV.left = rL.left;
    rV.right = rL.right;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.handleVal, -1, &rV,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    y += lblH + 2 + midH + gapRow;

    rL.left = rcLeft.left;
    rL.right = rcLeft.right;
    rL.top = y;
    rL.bottom = y + lblH;
    SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(96, 96, 96));
    DrawTextW(hdc, s_CpuStatsPaint.upLbl, -1, &rL, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    rV.left = rcLeft.left;
    rV.right = rcLeft.right;
    rV.top = y + lblH + 2;
    rV.bottom = rV.top + midH;
    SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(hdc, RGB(32, 32, 32));
    DrawTextW(hdc, s_CpuStatsPaint.upVal, -1, &rV,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    SelectObject(hdc, oldF);

    {
        int specLblH, specValH;
        SelectObject(hdc, s_hFontCpuLbl ? s_hFontCpuLbl : GetStockObject(DEFAULT_GUI_FONT));
        GetTextMetricsW(hdc, &tmSpec);
        specLblH = tmSpec.tmHeight;
        SelectObject(hdc, s_hFontCpuVal ? s_hFontCpuVal : GetStockObject(DEFAULT_GUI_FONT));
        GetTextMetricsW(hdc, &tmSpec);
        specValH = tmSpec.tmHeight;
        rowSpecH = (specLblH > specValH ? specLblH : specValH) + 4;
        if (lblH + 2 > rowSpecH)
            rowSpecH = lblH + 2;
    }
    DrawCpuSpecKvColumn(hdc, &rcMid, s_CpuStatsPaint.specMidLblBuf, s_CpuStatsPaint.specMidValBuf,
                        rowSpecH);
    DrawCpuSpecKvColumn(hdc, &rcCache, s_CpuStatsPaint.specCacheLblBuf, s_CpuStatsPaint.specCacheValBuf,
                        rowSpecH);
}

static void
Tm8CpuStatsUpdateScrollInfo(HWND hwnd)
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

    s_CpuStatsContentW = TM8_CPU_STATS_MIN_INNER_W;
    if (s_CpuStatsContentW < clientW)
        s_CpuStatsContentW = clientW;

    needScroll = (s_CpuStatsContentW > clientW);

    if (!needScroll)
        s_CpuStatsHScrollPos = 0;
    else
    {
        int maxPos = s_CpuStatsContentW - clientW;
        if (s_CpuStatsHScrollPos > maxPos)
            s_CpuStatsHScrollPos = maxPos;
        if (s_CpuStatsHScrollPos < 0)
            s_CpuStatsHScrollPos = 0;
    }

    page = clientW > 0 ? clientW : 1;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = (s_CpuStatsContentW > 1) ? (s_CpuStatsContentW - 1) : 0;
    si.nPage = (UINT)page;
    si.nPos = s_CpuStatsHScrollPos;
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
    ShowScrollBar(hwnd, SB_HORZ, needScroll);
}

static LRESULT CALLBACK
CpuStatsPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        s_CpuStatsHScrollPos = 0;
        s_CpuStatsContentW = TM8_CPU_STATS_MIN_INNER_W;
        return 0;

    case WM_SIZE:
        Tm8CpuStatsUpdateScrollInfo(hwnd);
        return 0;

    case WM_HSCROLL:
    {
        RECT rc;
        int clientW, maxPos, pos = s_CpuStatsHScrollPos;
        SCROLLINFO si;

        GetClientRect(hwnd, &rc);
        clientW = rc.right - rc.left;
        maxPos = (s_CpuStatsContentW > clientW) ? (s_CpuStatsContentW - clientW) : 0;

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
            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;
            if (GetScrollInfo(hwnd, SB_HORZ, &si))
                pos = si.nTrackPos;
            break;
        case SB_THUMBPOSITION:
            pos = (short)HIWORD(wParam);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        if (pos < 0)
            pos = 0;
        if (pos > maxPos)
            pos = maxPos;
        if (pos != s_CpuStatsHScrollPos)
        {
            s_CpuStatsHScrollPos = pos;
            Tm8CpuStatsUpdateScrollInfo(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
        if (GetKeyState(VK_SHIFT) & 0x8000)
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            RECT rc;
            int clientW, maxPos, pos = s_CpuStatsHScrollPos;
            const int step = 48;

            GetClientRect(hwnd, &rc);
            clientW = rc.right - rc.left;
            maxPos = (s_CpuStatsContentW > clientW) ? (s_CpuStatsContentW - clientW) : 0;
            if (delta > 0)
                pos -= step;
            else
                pos += step;
            if (pos < 0)
                pos = 0;
            if (pos > maxPos)
                pos = maxPos;
            if (pos != s_CpuStatsHScrollPos)
            {
                s_CpuStatsHScrollPos = pos;
                Tm8CpuStatsUpdateScrollInfo(hwnd);
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
        drawW = s_CpuStatsContentW;
        if (drawW < rcC.right)
            drawW = rcC.right;

        FillRect(hdc, &rcC, (HBRUSH)GetStockObject(WHITE_BRUSH));

        rcDraw.left = 0;
        rcDraw.top = 0;
        rcDraw.right = drawW;
        rcDraw.bottom = rcC.bottom;

        dcSave = SaveDC(hdc);
        SetViewportOrgEx(hdc, -s_CpuStatsHScrollPos, 0, NULL);
        DrawCpuPerfStatsPanel(hdc, &rcDraw);
        RestoreDC(hdc, dcSave);
        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

BOOL
Tm8CpuStats_RegisterClass(HINSTANCE hInst)
{
    WNDCLASSW wc;

    if (GetClassInfoW(hInst, TM8_CPU_STATS_WNDCLASS, &wc))
        return TRUE;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = CpuStatsPanelProc;
    wc.cbWndExtra = 0;
    wc.cbClsExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TM8_CPU_STATS_WNDCLASS;
    if (!RegisterClassW(&wc))
        return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    return TRUE;
}

void
Tm8CpuStats_Refresh(void)
{
    WCHAR specMidLbl[768], specMidVal[768], cacheLbl[768], cacheVal[768];
    WCHAR spd[48], liveSpd[48], up[48], virtLbl[48], line[180];
    PERFORMANCE_INFORMATION pi;
    DWORD cores;
    WCHAR grp[32];
    TM8_CPU_STATS_PAINT *ps = &s_CpuStatsPaint;

    if (!s_hwndCpuStatsPanel)
        return;

    FormatSpeedFromMhz(s_NominalCpuMhz, spd, _countof(spd));

    ZeroMemory(ps, sizeof(*ps));
    specMidLbl[0] = specMidVal[0] = 0;
    cacheLbl[0] = cacheVal[0] = 0;

    LoadStr(IDS_LBL_UTIL, ps->utilLbl, _countof(ps->utilLbl));
    StringCchPrintfW(ps->utilVal, _countof(ps->utilVal), L"%lu%%", (ULONG)s_LastCpuPct);

    LoadStr(IDS_LBL_SPEED, ps->speedLbl, _countof(ps->speedLbl));
    {
        DWORD mhzLive = s_CurrentCpuMhzLive ? s_CurrentCpuMhzLive : s_NominalCpuMhz;
        FormatSpeedFromMhz(mhzLive, liveSpd, _countof(liveSpd));
        if (liveSpd[0])
            StringCchCopyW(ps->speedVal, _countof(ps->speedVal), liveSpd);
    }

    ZeroMemory(&pi, sizeof(pi));
    pi.cb = sizeof(pi);
    GetPerformanceInfo(&pi, sizeof(pi));
    LoadStr(IDS_LBL_PROCS, ps->procLbl, _countof(ps->procLbl));
    FormatULongGrouped(pi.ProcessCount, grp, _countof(grp));
    StringCchCopyW(ps->procVal, _countof(ps->procVal), grp);

    LoadStr(IDS_LBL_THREADS, ps->threadLbl, _countof(ps->threadLbl));
    FormatULongGrouped(pi.ThreadCount, grp, _countof(grp));
    StringCchCopyW(ps->threadVal, _countof(ps->threadVal), grp);

    LoadStr(IDS_LBL_HANDLES, ps->handleLbl, _countof(ps->handleLbl));
    FormatULongGrouped(pi.HandleCount, grp, _countof(grp));
    StringCchCopyW(ps->handleVal, _countof(ps->handleVal), grp);

    FormatUptimeString(up, _countof(up));
    LoadStr(IDS_LBL_UPTIME, ps->upLbl, _countof(ps->upLbl));
    StringCchCopyW(ps->upVal, _countof(ps->upVal), up);

    if (spd[0])
    {
        AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_BASE);
        StringCchCatW(specMidVal, _countof(specMidVal), spd);
        StringCchCatW(specMidVal, _countof(specMidVal), L"\r\n");
    }
    AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_SOCKETS);
    StringCchCatW(specMidVal, _countof(specMidVal), L"1\r\n");

    cores = CountPhysicalCores();
    if (cores > 0)
    {
        AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_CORES);
        StringCchPrintfW(line, _countof(line), L"%lu\r\n", cores);
        StringCchCatW(specMidVal, _countof(specMidVal), line);
    }

    AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_LOGICAL);
    StringCchPrintfW(line, _countof(line), L"%lu\r\n", (ULONG)s_NumLogicalCpus);
    StringCchCatW(specMidVal, _countof(specMidVal), line);

#if ROS_HAVE_CPUID
    LoadStr(CpuVirtHardwarePresent() ? IDS_VIRT_ENABLED : IDS_VIRT_DISABLED, virtLbl, _countof(virtLbl));
#else
    LoadStr(IDS_VIRT_UNKNOWN, virtLbl, _countof(virtLbl));
#endif
    AppendSpecLbl(specMidLbl, _countof(specMidLbl), IDS_LBL_VIRT);
    StringCchCatW(specMidVal, _countof(specMidVal), virtLbl);
    StringCchCatW(specMidVal, _countof(specMidVal), L"\r\n");

    {
        DWORD l1dKB = 0, l1iKB = 0, l1KB = 0, l2KB = 0, l3KB = 0;
        int haveL1 = 0, haveL2 = 0, haveL3 = 0;

        ReadProcessor0CacheValue(L"L1DataCacheSize", &l1dKB);
        ReadProcessor0CacheValue(L"L1InstructionCacheSize", &l1iKB);
        l1KB = l1dKB + l1iKB;
        if (l1KB == 0)
            ReadProcessor0CacheValue(L"L1CacheSize", &l1KB);
        if (l1KB != 0)
        {
            AppendCacheKvLine(cacheLbl, _countof(cacheLbl), cacheVal, _countof(cacheVal), IDS_LBL_L1, l1KB);
            haveL1 = 1;
        }
        if (ReadProcessor0CacheValue(L"L2CacheSize", &l2KB))
        {
            AppendCacheKvLine(cacheLbl, _countof(cacheLbl), cacheVal, _countof(cacheVal), IDS_LBL_L2, l2KB);
            haveL2 = 1;
        }
        if (ReadProcessor0CacheValue(L"ThirdLevelCacheSize", &l3KB) ||
            ReadProcessor0CacheValue(L"L3CacheSize", &l3KB))
        {
            AppendCacheKvLine(cacheLbl, _countof(cacheLbl), cacheVal, _countof(cacheVal), IDS_LBL_L3, l3KB);
            haveL3 = 1;
        }
#if ROS_HAVE_CPUID
        if (!haveL1 || !haveL2 || !haveL3)
            AppendMissingCachesFromCpuid4(cacheLbl, _countof(cacheLbl), cacheVal, _countof(cacheVal), haveL1,
                                          haveL2, haveL3);
#endif
    }

    StringCchCopyW(ps->specMidLblBuf, _countof(ps->specMidLblBuf), specMidLbl);
    StringCchCopyW(ps->specMidValBuf, _countof(ps->specMidValBuf), specMidVal);
    StringCchCopyW(ps->specCacheLblBuf, _countof(ps->specCacheLblBuf), cacheLbl);
    StringCchCopyW(ps->specCacheValBuf, _countof(ps->specCacheValBuf), cacheVal);
    InvalidateRect(s_hwndCpuStatsPanel, NULL, TRUE);
}

void
Tm8CpuStats_OnLeaveCpuPage(int page)
{
    if (page == PAGE_CPU)
        return;
    if (!s_hwndCpuStatsPanel || !IsWindow(s_hwndCpuStatsPanel))
        return;
    s_CpuStatsHScrollPos = 0;
    Tm8CpuStatsUpdateScrollInfo(s_hwndCpuStatsPanel);
}
