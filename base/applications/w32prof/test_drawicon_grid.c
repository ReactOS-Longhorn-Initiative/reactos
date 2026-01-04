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
W32Prof_Test_DrawIconGrid(const ProfilerConfig* cfg)
{
    RECT r;
    int w, h;

    HWND hRender;
    HDC hdc;
    HICON ico;

    DWORD frames;
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
        ResultsPrint(TEXT("DrawIcon Grid: failed to create render child"));
        return;
    }

    hdc = GetDC(hRender);
    if (!hdc)
    {
        DestroyWindow(hRender);
        return;
    }

    ico = LoadIcon(NULL, IDI_APPLICATION);
    if (!ico)
    {
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    if (cfg->Continuous)
        frames = 0;
    else
        frames = (cfg->GpuFrames != 0) ? cfg->GpuFrames : 600;

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    while (1)
    {
        int x, y;
        DWORD draws = 0;

        if (ShouldStop(cfg))
            break;
        if (frames != 0 && i >= frames)
            break;

        PatBlt(hdc, 0, 0, w, h, BLACKNESS);

        for (y = 0; y < h; y += 36)
        {
            for (x = 0; x < w; x += 36)
            {
                DrawIconEx(hdc, x, y, ico, 32, 32, 0, NULL, DI_NORMAL);
                draws++;
                if ((draws & 255) == 0 && ShouldStop(cfg))
                    break;
            }
            if ((draws & 255) == 0 && ShouldStop(cfg))
                break;
        }

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, (LONGLONG)qf.QuadPart, TEXT("DrawIcon Grid"));
    }

    QueryPerformanceCounter(&q1);

    {
        double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
        double fpsCalc = (ms > 0.0) ? ((double)i * 1000.0 / ms) : 0.0;
        ResultsPrint(TEXT("DrawIcon Grid: %lu frames in %.3f ms (%.2f fps)"), (ULONG)i, ms, fpsCalc);
    }

    ReleaseDC(hRender, hdc);
    DestroyWindow(hRender);
}
