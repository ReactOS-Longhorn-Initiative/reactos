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
W32Prof_Test_DeferWindowPosBatch(const ProfilerConfig* cfg)
{
    RECT r;
    int w, h;

    HWND hContainer = NULL;
    HWND children[128];
    DWORD childCount = (DWORD)(sizeof(children) / sizeof(children[0]));
    DWORD i;

    DWORD iters;
    DWORD passes = 0;

    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    if (!cfg || !cfg->hTestWnd)
        return;

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
        ResultsPrint(TEXT("DeferWindowPos: failed to create container"));
        return;
    }

    ZeroMemory(children, sizeof(children));

    for (i = 0; i < childCount; i++)
    {
        children[i] = CreateWindowEx(0,
                                    TEXT("STATIC"),
                                    TEXT(""),
                                    WS_CHILD | WS_VISIBLE,
                                    0, 0, 10, 10,
                                    hContainer,
                                    NULL,
                                    GetModuleHandle(NULL),
                                    NULL);
        if (!children[i])
        {
            childCount = i;
            break;
        }
    }

    iters = (cfg->WindowPosIterations != 0) ? cfg->WindowPosIterations : 10000;
    if (iters > 2000)
        iters = 2000;

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    if (!cfg->Continuous)
    {
        DWORD iter;
        for (iter = 0; iter < iters; iter++)
        {
            HDWP hdwp;
            int x, y;
            DWORD k;

            if (((iter & 63) == 0) && ShouldStop(cfg))
                break;

            hdwp = BeginDeferWindowPos((int)childCount);
            if (!hdwp)
                continue;

            for (k = 0; k < childCount; k++)
            {
                x = (int)((k * 13 + iter * 7) % (DWORD)(w > 1 ? (w - 1) : 1));
                y = (int)((k * 17 + iter * 5) % (DWORD)(h > 1 ? (h - 1) : 1));

                hdwp = DeferWindowPos(hdwp,
                                      children[k],
                                      NULL,
                                      x, y,
                                      12, 12,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                if (!hdwp)
                    break;
            }

            if (hdwp)
                EndDeferWindowPos(hdwp);
        }

        QueryPerformanceCounter(&q1);

        {
            double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
            double ips = (ms > 0.0) ? ((double)iters * 1000.0 / ms) : 0.0;
            ResultsPrint(TEXT("DeferWindowPos: %lu batches in %.3f ms (%.2f batches/s)"),
                         (ULONG)iters, ms, ips);
        }

        DestroyWindow(hContainer);
        return;
    }

    while (!ShouldStop(cfg))
    {
        HDWP hdwp;
        int x, y;
        DWORD k;

        passes++;

        hdwp = BeginDeferWindowPos((int)childCount);
        if (hdwp)
        {
            for (k = 0; k < childCount; k++)
            {
                x = (int)((k * 13 + passes * 7) % (DWORD)(w > 1 ? (w - 1) : 1));
                y = (int)((k * 17 + passes * 5) % (DWORD)(h > 1 ? (h - 1) : 1));

                hdwp = DeferWindowPos(hdwp,
                                      children[k],
                                      NULL,
                                      x, y,
                                      12, 12,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                if (!hdwp)
                    break;
            }

            if (hdwp)
                EndDeferWindowPos(hdwp);
        }

        W32Prof_FpsMaybeReport(cfg, &fps, passes, (LONGLONG)qf.QuadPart, TEXT("DeferWindowPos"));
    }

    DestroyWindow(hContainer);
}
