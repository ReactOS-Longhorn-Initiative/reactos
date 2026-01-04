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
W32Prof_Test_GdiPathStroke(const ProfilerConfig* cfg)
{
    RECT r;
    int w, h;

    HWND hRender;
    HDC hdc;

    DWORD iters;
    DWORD i;

    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    POINT pts[64];
    DWORD k;

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
        ResultsPrint(TEXT("GDI Path Stroke: failed to create render child"));
        return;
    }

    hdc = GetDC(hRender);
    if (!hdc)
    {
        DestroyWindow(hRender);
        return;
    }

    for (k = 0; k < (DWORD)(sizeof(pts) / sizeof(pts[0])); k++)
    {
        pts[k].x = (LONG)((k * 13) % (DWORD)(w > 1 ? (w - 1) : 1));
        pts[k].y = (LONG)((k * 17) % (DWORD)(h > 1 ? (h - 1) : 1));
    }

    iters = (cfg->SelectObjectIterations != 0) ? cfg->SelectObjectIterations : 200000;
    iters /= 10;
    if (iters < 1000)
        iters = 1000;
    if (iters > 100000)
        iters = 100000;

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    while (1)
    {
        if (!cfg->Continuous && i >= iters)
            break;
        if ((i & 1023) == 0 && ShouldStop(cfg))
            break;

        BeginPath(hdc);
        Polyline(hdc, pts, (int)(sizeof(pts) / sizeof(pts[0])));
        EndPath(hdc);
        StrokePath(hdc);

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, (LONGLONG)qf.QuadPart, TEXT("GDI Path Stroke"));
    }

    QueryPerformanceCounter(&q1);

    {
        double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
        double ops = (ms > 0.0) ? ((double)i * 1000.0 / ms) : 0.0;
        ResultsPrint(TEXT("GDI Path Stroke: %lu ops in %.3f ms (%.2f ops/s)"), (ULONG)i, ms, ops);
    }

    ReleaseDC(hRender, hdc);
    DestroyWindow(hRender);
}
