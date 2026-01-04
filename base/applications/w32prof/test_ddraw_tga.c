#include "profiler.h"
#include "tga.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

#define INITGUID
#include <initguid.h>
#include <ddraw.h>

static double
TicksToMs(LONGLONG ticks, LONGLONG freq)
{
    if (freq <= 0)
        return 0.0;
    return ((double)ticks * 1000.0) / (double)freq;
}

static BOOL
CreateSurfaceX8R8G8B8(LPDIRECTDRAW7 dd, DWORD w, DWORD h, LPDIRECTDRAWSURFACE7* outSurf)
{
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    if (!dd || !outSurf)
        return FALSE;

    *outSurf = NULL;

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth = w;
    ddsd.dwHeight = h;

    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
    ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
    ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
    ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;

    hr = IDirectDraw7_CreateSurface(dd, &ddsd, outSurf, NULL);
    return SUCCEEDED(hr) && *outSurf;
}

static BOOL
UploadSurfaceFromRgba(LPDIRECTDRAWSURFACE7 surf, const W32PROF_IMAGE_RGBA* img)
{
    DDSURFACEDESC2 lock;
    HRESULT hr;
    DWORD y;

    if (!surf || !img || !img->Pixels)
        return FALSE;

    ZeroMemory(&lock, sizeof(lock));
    lock.dwSize = sizeof(lock);

    hr = IDirectDrawSurface7_Lock(surf, NULL, &lock, DDLOCK_WAIT, NULL);
    if (FAILED(hr) || !lock.lpSurface)
        return FALSE;

    for (y = 0; y < img->Height; y++)
    {
        const BYTE* src = img->Pixels + (SIZE_T)y * img->StrideBytes;
        BYTE* dstRow = (BYTE*)lock.lpSurface + (SIZE_T)y * (SIZE_T)lock.lPitch;
        DWORD x;

        for (x = 0; x < img->Width; x++)
        {
            BYTE r = src[x * 4 + 0];
            BYTE g = src[x * 4 + 1];
            BYTE b = src[x * 4 + 2];
            dstRow[x * 4 + 0] = b;
            dstRow[x * 4 + 1] = g;
            dstRow[x * 4 + 2] = r;
            dstRow[x * 4 + 3] = 0xFF;
        }
    }

    IDirectDrawSurface7_Unlock(surf, NULL);
    return TRUE;
}

void
W32Prof_Test_DDrawTgaBlit(const ProfilerConfig* cfg)
{
    HRESULT hr;
    RECT rc;
    int w, h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;

    HWND hRender = NULL;

    LPDIRECTDRAW7 dd = NULL;
    LPDIRECTDRAWSURFACE7 primary = NULL;
    LPDIRECTDRAWCLIPPER clipper = NULL;
    LPDIRECTDRAWSURFACE7 srcSurf = NULL;

    W32PROF_IMAGE_RGBA img;
    W32PROF_FPS_STATE fps;

    ZeroMemory(&img, sizeof(img));

    if (!cfg || !cfg->hTestWnd)
        return;

    if (cfg->Continuous)
        frames = 0;
    else
        frames = (cfg->GpuFrames != 0) ? cfg->GpuFrames : 600;

    GetClientRect(cfg->hTestWnd, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
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
        ResultsPrint(TEXT("DirectDraw TGA Blit: failed to create render child window"));
        return;
    }

    if (!W32Prof_LoadLogoTestTgaFromResource(GetModuleHandle(NULL), &img))
    {
        ResultsPrint(TEXT("DirectDraw TGA Blit: failed to load embedded TGA"));
        DestroyWindow(hRender);
        return;
    }

    hr = DirectDrawCreateEx(NULL, (void**)&dd, &IID_IDirectDraw7, NULL);
    if (FAILED(hr) || !dd)
    {
        ResultsPrint(TEXT("DirectDraw TGA Blit: DirectDrawCreateEx failed: 0x%08lx"), (ULONG)hr);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    hr = IDirectDraw7_SetCooperativeLevel(dd, hRender, DDSCL_NORMAL);
    if (FAILED(hr))
    {
        ResultsPrint(TEXT("DirectDraw TGA Blit: SetCooperativeLevel failed: 0x%08lx"), (ULONG)hr);
        IDirectDraw7_Release(dd);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    {
        DDSURFACEDESC2 ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = IDirectDraw7_CreateSurface(dd, &ddsd, &primary, NULL);
        if (FAILED(hr) || !primary)
        {
            ResultsPrint(TEXT("DirectDraw TGA Blit: Create primary surface failed: 0x%08lx"), (ULONG)hr);
            IDirectDraw7_Release(dd);
            W32Prof_ImageFree(&img);
            DestroyWindow(hRender);
            return;
        }

        hr = IDirectDraw7_CreateClipper(dd, 0, &clipper, NULL);
        if (SUCCEEDED(hr) && clipper)
        {
            IDirectDrawClipper_SetHWnd(clipper, 0, hRender);
            IDirectDrawSurface7_SetClipper(primary, clipper);
        }
    }

    if (!CreateSurfaceX8R8G8B8(dd, img.Width, img.Height, &srcSurf))
    {
        ResultsPrint(TEXT("DirectDraw TGA Blit: failed to create source surface"));
        if (clipper) IDirectDrawClipper_Release(clipper);
        if (primary) IDirectDrawSurface7_Release(primary);
        IDirectDraw7_Release(dd);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    if (!UploadSurfaceFromRgba(srcSurf, &img))
    {
        ResultsPrint(TEXT("DirectDraw TGA Blit: failed to upload pixels"));
        IDirectDrawSurface7_Release(srcSurf);
        if (clipper) IDirectDrawClipper_Release(clipper);
        if (primary) IDirectDrawSurface7_Release(primary);
        IDirectDraw7_Release(dd);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    while (1)
    {
        RECT dst;
        POINT pt;

        if (cfg && cfg->StopEvent && WaitForSingleObject(cfg->StopEvent, 0) == WAIT_OBJECT_0)
            break;
        if (frames != 0 && i >= frames)
            break;

        GetClientRect(hRender, &dst);
        pt.x = dst.left;
        pt.y = dst.top;
        ClientToScreen(hRender, &pt);
        OffsetRect(&dst, pt.x, pt.y);

        IDirectDrawSurface7_Blt(primary, &dst, srcSurf, NULL, DDBLT_WAIT, NULL);

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("DirectDraw TGA Blit"));
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("DirectDraw TGA Blit: %lu frames in %.3f ms (%.2f fps)"),
                 (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                    ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                    : 0.0);

    if (srcSurf) IDirectDrawSurface7_Release(srcSurf);
    if (clipper) IDirectDrawClipper_Release(clipper);
    if (primary) IDirectDrawSurface7_Release(primary);
    if (dd) IDirectDraw7_Release(dd);

    W32Prof_ImageFree(&img);
    DestroyWindow(hRender);
}
