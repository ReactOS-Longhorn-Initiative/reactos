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
W32Prof_Test_ImageListDraw(const ProfilerConfig* cfg)
{
    RECT r;
    int w, h;
    HWND hRender = NULL;
    HDC hdc = NULL;

    INITCOMMONCONTROLSEX icc;

    HIMAGELIST himl = NULL;
    HICON ico = NULL;

    DWORD frames;
    DWORD i;

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
        ResultsPrint(TEXT("ImageList Draw: failed to create render child"));
        return;
    }

    hdc = GetDC(hRender);
    if (!hdc)
    {
        DestroyWindow(hRender);
        return;
    }

    himl = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 64, 64);
    ico = LoadIcon(NULL, IDI_APPLICATION);
    if (!himl || !ico)
    {
        ResultsPrint(TEXT("ImageList Draw: ImageList_Create/LoadIcon failed"));
        if (himl) ImageList_Destroy(himl);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    {
        DWORD k;
        for (k = 0; k < 512; k++)
            ImageList_AddIcon(himl, ico);
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
        DWORD drawCount;

        if (ShouldStop(cfg))
            break;
        if (frames != 0 && i >= frames)
            break;

        PatBlt(hdc, 0, 0, w, h, BLACKNESS);

        /* Draw a small grid each frame to approximate explorer-ish icon view. */
        drawCount = 0;
        for (y = 0; y < h; y += 36)
        {
            for (x = 0; x < w; x += 36)
            {
                int idx = (int)(drawCount & 511);
                ImageList_Draw(himl, idx, hdc, x, y, ILD_NORMAL);
                drawCount++;

                if ((drawCount & 255) == 0 && ShouldStop(cfg))
                    break;
            }
            if ((drawCount & 255) == 0 && ShouldStop(cfg))
                break;
        }

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, (LONGLONG)qf.QuadPart, TEXT("ImageList Draw"));
    }

    QueryPerformanceCounter(&q1);

    {
        double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
        double fpsCalc = (ms > 0.0) ? ((double)i * 1000.0 / ms) : 0.0;
        ResultsPrint(TEXT("ImageList Draw: %lu frames in %.3f ms (%.2f fps)"), (ULONG)i, ms, fpsCalc);
    }

    if (himl)
        ImageList_Destroy(himl);

    if (hdc)
        ReleaseDC(hRender, hdc);

    DestroyWindow(hRender);
}
