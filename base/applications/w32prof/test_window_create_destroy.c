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
W32Prof_Test_WindowCreateDestroy(const ProfilerConfig* cfg)
{
    RECT r;
    int w, h;
    HWND hContainer;

    DWORD iters;
    DWORD i;

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
        ResultsPrint(TEXT("Create/Destroy Windows: failed to create container"));
        return;
    }

    iters = (cfg->WindowPosIterations != 0) ? cfg->WindowPosIterations : 10000;
    if (iters > 5000)
        iters = 5000;

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    while (1)
    {
        HWND hChild;

        if (!cfg->Continuous && i >= iters)
            break;
        if ((i & 255) == 0 && ShouldStop(cfg))
            break;

        hChild = CreateWindowEx(0,
                                TEXT("STATIC"),
                                TEXT(""),
                                WS_CHILD | WS_VISIBLE,
                                0, 0, 10, 10,
                                hContainer,
                                NULL,
                                GetModuleHandle(NULL),
                                NULL);
        if (hChild)
            DestroyWindow(hChild);

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, (LONGLONG)qf.QuadPart, TEXT("Create/Destroy Windows"));
    }

    QueryPerformanceCounter(&q1);

    {
        double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
        double ops = (ms > 0.0) ? ((double)i * 1000.0 / ms) : 0.0;
        ResultsPrint(TEXT("Create/Destroy Windows: %lu ops in %.3f ms (%.2f ops/s)"), (ULONG)i, ms, ops);
    }

    DestroyWindow(hContainer);
}
