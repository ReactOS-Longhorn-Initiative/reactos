#include "profiler.h"
#include "tga.h"
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

static void
RgbaToBgra32(BYTE* dstBgra, DWORD dstStride, const W32PROF_IMAGE_RGBA* img)
{
    DWORD y;
    for (y = 0; y < img->Height; y++)
    {
        const BYTE* src = img->Pixels + (SIZE_T)y * img->StrideBytes;
        BYTE* dst = dstBgra + (SIZE_T)y * (SIZE_T)dstStride;
        DWORD x;
        for (x = 0; x < img->Width; x++)
        {
            BYTE r = src[x * 4 + 0];
            BYTE g = src[x * 4 + 1];
            BYTE b = src[x * 4 + 2];
            BYTE a = src[x * 4 + 3];
            dst[x * 4 + 0] = b;
            dst[x * 4 + 1] = g;
            dst[x * 4 + 2] = r;
            dst[x * 4 + 3] = a;
        }
    }
}

void
W32Prof_Test_Win32TgaBlit(const ProfilerConfig* cfg)
{
    RECT r;
    int w;
    int h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;

    HWND hRender;
    HDC hdcDst;
    HDC hdcMem;
    HBITMAP hbmp;
    HGDIOBJ oldObj;

    BITMAPINFO bmi;
    void* bits;

    W32PROF_IMAGE_RGBA img;
    W32PROF_FPS_STATE fps;

    hRender = NULL;
    hdcDst = NULL;
    hdcMem = NULL;
    hbmp = NULL;
    oldObj = NULL;
    bits = NULL;
    ZeroMemory(&img, sizeof(img));

    if (!cfg || !cfg->hTestWnd)
        return;

    if (cfg->Continuous)
        frames = 0;
    else
        frames = (cfg->GpuFrames != 0) ? cfg->GpuFrames : 600;

    GetClientRect(cfg->hTestWnd, &r);
    w = r.right - r.left;
    h = r.bottom - r.top;
    if (w <= 0) w = 640;
    if (h <= 0) h = 480;

    hRender = CreateWindowEx(0,
                             TEXT("W32ProfRenderChild"),
                             TEXT(""),
                             WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                             0, 0, w, h,
                             cfg->hTestWnd,
                             NULL,
                             GetModuleHandle(NULL),
                             NULL);
    if (!hRender)
    {
        ResultsPrint(TEXT("Win32 TGA Blit: failed to create render child window"));
        return;
    }

    if (!W32Prof_LoadLogoTestTgaFromResource(GetModuleHandle(NULL), &img))
    {
        ResultsPrint(TEXT("Win32 TGA Blit: failed to load embedded TGA"));
        DestroyWindow(hRender);
        return;
    }

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = (LONG)img.Width;
    bmi.bmiHeader.biHeight = -(LONG)img.Height; /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hdcDst = GetDC(hRender);
    if (!hdcDst)
    {
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    hbmp = CreateDIBSection(hdcDst, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!hbmp || !bits)
    {
        ResultsPrint(TEXT("Win32 TGA Blit: CreateDIBSection failed"));
        ReleaseDC(hRender, hdcDst);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    RgbaToBgra32((BYTE*)bits, img.Width * 4, &img);

    hdcMem = CreateCompatibleDC(hdcDst);
    if (!hdcMem)
    {
        DeleteObject(hbmp);
        ReleaseDC(hRender, hdcDst);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    oldObj = SelectObject(hdcMem, hbmp);

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    while (1)
    {
        if (cfg && cfg->StopEvent && WaitForSingleObject(cfg->StopEvent, 0) == WAIT_OBJECT_0)
            break;
        if (frames != 0 && i >= frames)
            break;

        /* Simple stretch to fill the window */
        StretchBlt(hdcDst,
                   0, 0, w, h,
                   hdcMem,
                   0, 0, (int)img.Width, (int)img.Height,
                   SRCCOPY);

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("Win32 TGA Blit"));
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("Win32 TGA Blit: %lu frames in %.3f ms (%.2f fps)"),
                 (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                    ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                    : 0.0);

    if (hdcMem)
    {
        if (oldObj)
            SelectObject(hdcMem, oldObj);
        DeleteDC(hdcMem);
    }

    if (hbmp)
        DeleteObject(hbmp);

    if (hdcDst)
        ReleaseDC(hRender, hdcDst);

    W32Prof_ImageFree(&img);

    DestroyWindow(hRender);
}
