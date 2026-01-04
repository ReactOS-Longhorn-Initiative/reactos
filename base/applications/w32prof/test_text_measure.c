#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

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
W32Prof_Test_TextMeasure(const ProfilerConfig* cfg)
{
    RECT r;
    int w, h;

    HWND hRender = NULL;
    HDC hdc = NULL;
    HFONT hFont = NULL;
    HFONT hOldFont = NULL;

    DWORD iters;
    DWORD i;

    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    static const TCHAR* strings[] =
    {
        TEXT("This is a longer filename that should be ellipsized.txt"),
        TEXT("Program Files"),
        TEXT("ReactOS_Profiler")
    };

    if (!cfg || !cfg->hTestWnd)
        return;

    GetClientRect(cfg->hTestWnd, &r);
    w = r.right - r.left;
    h = r.bottom - r.top;
    if (w <= 0) w = 640;
    if (h <= 0) h = 480;

    hRender = CreateWindowEx(0,
                             TEXT("STATIC"),
                             TEXT(""),
                             WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                             0, 0, w, h,
                             cfg->hTestWnd,
                             NULL,
                             GetModuleHandle(NULL),
                             NULL);
    if (!hRender)
    {
        ResultsPrint(TEXT("Text Measure: failed to create render child"));
        return;
    }

    hdc = GetDC(hRender);
    if (!hdc)
    {
        DestroyWindow(hRender);
        return;
    }

    hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (hFont)
        hOldFont = (HFONT)SelectObject(hdc, hFont);

    iters = (cfg->TextOutIterations != 0) ? cfg->TextOutIterations : 50000;

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    while (1)
    {
        RECT rc;
        const TCHAR* s;
        int idx;

        if (!cfg->Continuous && i >= iters)
            break;
        if ((i & 1023) == 0 && ShouldStop(cfg))
            break;

        idx = (int)(i % (sizeof(strings) / sizeof(strings[0])));
        s = strings[idx];

        rc.left = 4;
        rc.top = 4 + (idx * 24);
        rc.right = w - 8;
        rc.bottom = rc.top + 20;

        /* Explorer-like path: measure and ellipsize in a constrained rect. */
        DrawText(hdc, s, -1, &rc,
                 DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | DT_VCENTER | DT_CALCRECT);

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, (LONGLONG)qf.QuadPart, TEXT("Text Measure"));
    }

    QueryPerformanceCounter(&q1);

    {
        double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
        double ops = (ms > 0.0) ? ((double)i * 1000.0 / ms) : 0.0;
        ResultsPrint(TEXT("Text Measure: %lu ops in %.3f ms (%.2f ops/s)"), (ULONG)i, ms, ops);
    }

    if (hOldFont)
        SelectObject(hdc, hOldFont);

    if (hdc)
        ReleaseDC(hRender, hdc);

    DestroyWindow(hRender);
}
