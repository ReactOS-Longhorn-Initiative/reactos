#include "profiler.h"
#include "tga.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>
#include <math.h>

#define INITGUID
#include <initguid.h>
#include <ddraw.h>
#include <d3d.h>

/*
 * D3D7 textured spinning cube test (fullscreen).
 */

typedef struct _W32PROF_D3D7TVERT
{
    float x, y, z;
    float u, v;
} W32PROF_D3D7TVERT;

#define W32PROF_D3D7_TEX_FVF (D3DFVF_XYZ | D3DFVF_TEX1)

typedef struct _W32PROF_ZENUM_CTX
{
    DDPIXELFORMAT Pf;
    BOOL Found;
} W32PROF_ZENUM_CTX;

static HRESULT WINAPI
EnumZBufCallback(DDPIXELFORMAT* ddpf, void* context)
{
    W32PROF_ZENUM_CTX* ctx = (W32PROF_ZENUM_CTX*)context;
    if (!ctx || !ddpf)
        return D3DENUMRET_OK;

    if ((ddpf->dwFlags & DDPF_ZBUFFER) && ddpf->dwZBufferBitDepth == 16)
    {
        ctx->Pf = *ddpf;
        ctx->Found = TRUE;
        return D3DENUMRET_CANCEL;
    }

    if (!ctx->Found && (ddpf->dwFlags & DDPF_ZBUFFER) && ddpf->dwZBufferBitDepth != 0)
    {
        ctx->Pf = *ddpf;
        ctx->Found = TRUE;
    }

    return D3DENUMRET_OK;
}

static void
BuildCubeTex(W32PROF_D3D7TVERT* v, float s)
{
    float hs = s * 0.5f;

    const W32PROF_D3D7TVERT verts[36] =
    {
        /* +Z */
        { -hs, -hs, +hs, 0.0f, 0.0f }, { -hs, +hs, +hs, 0.0f, 1.0f }, { +hs, +hs, +hs, 1.0f, 1.0f },
        { -hs, -hs, +hs, 0.0f, 0.0f }, { +hs, +hs, +hs, 1.0f, 1.0f }, { +hs, -hs, +hs, 1.0f, 0.0f },
        /* -Z */
        { +hs, -hs, -hs, 0.0f, 0.0f }, { +hs, +hs, -hs, 0.0f, 1.0f }, { -hs, +hs, -hs, 1.0f, 1.0f },
        { +hs, -hs, -hs, 0.0f, 0.0f }, { -hs, +hs, -hs, 1.0f, 1.0f }, { -hs, -hs, -hs, 1.0f, 0.0f },
        /* +X */
        { +hs, -hs, +hs, 0.0f, 0.0f }, { +hs, +hs, +hs, 0.0f, 1.0f }, { +hs, +hs, -hs, 1.0f, 1.0f },
        { +hs, -hs, +hs, 0.0f, 0.0f }, { +hs, +hs, -hs, 1.0f, 1.0f }, { +hs, -hs, -hs, 1.0f, 0.0f },
        /* -X */
        { -hs, -hs, -hs, 0.0f, 0.0f }, { -hs, +hs, -hs, 0.0f, 1.0f }, { -hs, +hs, +hs, 1.0f, 1.0f },
        { -hs, -hs, -hs, 0.0f, 0.0f }, { -hs, +hs, +hs, 1.0f, 1.0f }, { -hs, -hs, +hs, 1.0f, 0.0f },
        /* +Y */
        { -hs, +hs, +hs, 0.0f, 0.0f }, { -hs, +hs, -hs, 0.0f, 1.0f }, { +hs, +hs, -hs, 1.0f, 1.0f },
        { -hs, +hs, +hs, 0.0f, 0.0f }, { +hs, +hs, -hs, 1.0f, 1.0f }, { +hs, +hs, +hs, 1.0f, 0.0f },
        /* -Y */
        { -hs, -hs, -hs, 0.0f, 0.0f }, { -hs, -hs, +hs, 0.0f, 1.0f }, { +hs, -hs, +hs, 1.0f, 1.0f },
        { -hs, -hs, -hs, 0.0f, 0.0f }, { +hs, -hs, +hs, 1.0f, 1.0f }, { +hs, -hs, -hs, 1.0f, 0.0f },
    };

    CopyMemory(v, verts, sizeof(verts));
}

static double
TicksToMs(LONGLONG ticks, LONGLONG freq)
{
    if (freq <= 0)
        return 0.0;
    return ((double)ticks * 1000.0) / (double)freq;
}

static BOOL
CreateTextureFromRgba(LPDIRECTDRAW7 dd, const W32PROF_IMAGE_RGBA* img, LPDIRECTDRAWSURFACE7* outTex, BOOL* outIs565)
{
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    LPDIRECTDRAWSURFACE7 tex;

    if (!dd || !img || !img->Pixels || !outTex)
        return FALSE;

    *outTex = NULL;
    if (outIs565)
        *outIs565 = FALSE;

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth = img->Width;
    ddsd.dwHeight = img->Height;

    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
    ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
    ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
    ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;
    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;

    tex = NULL;
    hr = IDirectDraw7_CreateSurface(dd, &ddsd, &tex, NULL);
    if (FAILED(hr) || !tex)
    {
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
        ddsd.dwWidth = img->Width;
        ddsd.dwHeight = img->Height;

        ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
        ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x07E0;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x001F;

        tex = NULL;
        hr = IDirectDraw7_CreateSurface(dd, &ddsd, &tex, NULL);
        if (FAILED(hr) || !tex)
        {
            ResultsPrint(TEXT("D3D7 Textured FS: Create texture surface failed: 0x%08lx"), (ULONG)hr);
            return FALSE;
        }

        if (outIs565)
            *outIs565 = TRUE;
    }

    {
        DDSURFACEDESC2 lock;
        DWORD y;

        ZeroMemory(&lock, sizeof(lock));
        lock.dwSize = sizeof(lock);

        hr = IDirectDrawSurface7_Lock(tex, NULL, &lock, DDLOCK_WAIT, NULL);
        if (FAILED(hr) || !lock.lpSurface)
        {
            IDirectDrawSurface7_Release(tex);
            ResultsPrint(TEXT("D3D7 Textured FS: Lock texture failed: 0x%08lx"), (ULONG)hr);
            return FALSE;
        }

        for (y = 0; y < img->Height; y++)
        {
            const BYTE* src = img->Pixels + (SIZE_T)y * img->StrideBytes;
            BYTE* dstRow = (BYTE*)lock.lpSurface + (SIZE_T)y * (SIZE_T)lock.lPitch;
            DWORD x;

            if (outIs565 && *outIs565)
            {
                WORD* dst16 = (WORD*)dstRow;
                for (x = 0; x < img->Width; x++)
                {
                    BYTE r = src[x * 4 + 0];
                    BYTE g = src[x * 4 + 1];
                    BYTE b = src[x * 4 + 2];
                    dst16[x] = (WORD)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
                }
            }
            else
            {
                DWORD* dst32 = (DWORD*)dstRow;
                for (x = 0; x < img->Width; x++)
                {
                    BYTE r = src[x * 4 + 0];
                    BYTE g = src[x * 4 + 1];
                    BYTE b = src[x * 4 + 2];
                    BYTE a = src[x * 4 + 3];
                    dst32[x] = ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
                }
            }
        }

        IDirectDrawSurface7_Unlock(tex, NULL);
    }

    *outTex = tex;
    return TRUE;
}

void
W32Prof_Test_D3D7TexturedCubeFullscreen(const ProfilerConfig* cfg)
{
    HRESULT hr;
    int w, h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    HWND hRender = NULL;
    LPDIRECTDRAW7 dd = NULL;
    LPDIRECT3D7 d3d = NULL;
    LPDIRECT3DDEVICE7 dev = NULL;
    LPDIRECTDRAWSURFACE7 primary = NULL;
    LPDIRECTDRAWSURFACE7 back = NULL;
    LPDIRECTDRAWSURFACE7 zsurf = NULL;
    LPDIRECTDRAWSURFACE7 tex = NULL;
    BOOL texIs565 = FALSE;

    DDSCAPS2 caps;
    CLSID deviceClsid;

    W32PROF_D3D7TVERT cube[36];
    W32PROF_IMAGE_RGBA img;

    MSG msg;
    BOOL quit;

    ZeroMemory(&img, sizeof(img));

    if (!cfg)
        return;

    if (cfg->Continuous)
        frames = 0;
    else
        frames = (cfg->GpuFrames != 0) ? cfg->GpuFrames : 600;

    w = (int)GetSystemMetrics(SM_CXSCREEN);
    h = (int)GetSystemMetrics(SM_CYSCREEN);
    if (w <= 0) w = 640;
    if (h <= 0) h = 480;

    hRender = CreateWindowEx(WS_EX_TOPMOST,
                             TEXT("W32ProfRenderChild"),
                             TEXT(""),
                             WS_POPUP | WS_VISIBLE,
                             0, 0, w, h,
                             NULL,
                             NULL,
                             GetModuleHandle(NULL),
                             NULL);
    if (!hRender)
    {
        ResultsPrint(TEXT("D3D7 Textured FS: failed to create fullscreen window"));
        return;
    }

    if (!W32Prof_LoadLogoTestTgaFromResource(GetModuleHandle(NULL), &img))
    {
        ResultsPrint(TEXT("D3D7 Textured FS: failed to load embedded TGA"));
        DestroyWindow(hRender);
        return;
    }

    hr = DirectDrawCreateEx(NULL, (void**)&dd, &IID_IDirectDraw7, NULL);
    if (FAILED(hr) || !dd)
    {
        ResultsPrint(TEXT("D3D7 Textured FS: DirectDrawCreateEx failed: 0x%08lx"), (ULONG)hr);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    hr = IDirectDraw7_SetCooperativeLevel(dd, hRender, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT);
    if (FAILED(hr))
    {
        ResultsPrint(TEXT("D3D7 Textured FS: SetCooperativeLevel failed: 0x%08lx"), (ULONG)hr);
        IDirectDraw7_Release(dd);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    hr = IDirectDraw7_SetDisplayMode(dd, (DWORD)w, (DWORD)h, 32, 0, 0);
    if (FAILED(hr))
    {
        ResultsPrint(TEXT("D3D7 Textured FS: SetDisplayMode failed: 0x%08lx"), (ULONG)hr);
        IDirectDraw7_Release(dd);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    {
        DDSURFACEDESC2 ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        ddsd.dwBackBufferCount = 1;

        hr = IDirectDraw7_CreateSurface(dd, &ddsd, &primary, NULL);
        if (FAILED(hr) || !primary)
        {
            ResultsPrint(TEXT("D3D7 Textured FS: Create primary surface failed: 0x%08lx"), (ULONG)hr);
            IDirectDraw7_RestoreDisplayMode(dd);
            IDirectDraw7_Release(dd);
            W32Prof_ImageFree(&img);
            DestroyWindow(hRender);
            return;
        }

        ZeroMemory(&caps, sizeof(caps));
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        hr = IDirectDrawSurface7_GetAttachedSurface(primary, &caps, &back);
        if (FAILED(hr) || !back)
        {
            ResultsPrint(TEXT("D3D7 Textured FS: Get backbuffer failed: 0x%08lx"), (ULONG)hr);
            IDirectDrawSurface7_Release(primary);
            IDirectDraw7_RestoreDisplayMode(dd);
            IDirectDraw7_Release(dd);
            W32Prof_ImageFree(&img);
            DestroyWindow(hRender);
            return;
        }
    }

    hr = IDirectDraw7_QueryInterface(dd, &IID_IDirect3D7, (void**)&d3d);
    if (FAILED(hr) || !d3d)
    {
        ResultsPrint(TEXT("D3D7 Textured FS: QI IID_IDirect3D7 failed: 0x%08lx"), (ULONG)hr);
        IDirectDrawSurface7_Release(back);
        IDirectDrawSurface7_Release(primary);
        IDirectDraw7_RestoreDisplayMode(dd);
        IDirectDraw7_Release(dd);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    deviceClsid = IID_IDirect3DHALDevice;
    hr = IDirect3D7_CreateDevice(d3d, &deviceClsid, back, &dev);
    if (FAILED(hr) || !dev)
    {
        deviceClsid = IID_IDirect3DRGBDevice;
        hr = IDirect3D7_CreateDevice(d3d, &deviceClsid, back, &dev);
    }

    if (FAILED(hr) || !dev)
    {
        ResultsPrint(TEXT("D3D7 Textured FS: CreateDevice failed: 0x%08lx"), (ULONG)hr);
        IDirect3D7_Release(d3d);
        IDirectDrawSurface7_Release(back);
        IDirectDrawSurface7_Release(primary);
        IDirectDraw7_RestoreDisplayMode(dd);
        IDirectDraw7_Release(dd);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    /* Z buffer */
    {
        W32PROF_ZENUM_CTX zctx;
        DDSURFACEDESC2 ddsd;
        ZeroMemory(&zctx, sizeof(zctx));
        zctx.Pf.dwSize = sizeof(zctx.Pf);

        hr = IDirect3D7_EnumZBufferFormats(d3d, &deviceClsid, (LPD3DENUMPIXELFORMATSCALLBACK)EnumZBufCallback, &zctx);
        (void)hr;

        if (!zctx.Found)
        {
            ResultsPrint(TEXT("D3D7 Textured FS: no Z buffer formats; disabling Z"));
        }
        else
        {
            ZeroMemory(&ddsd, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
            ddsd.dwWidth = (DWORD)w;
            ddsd.dwHeight = (DWORD)h;
            ddsd.ddpfPixelFormat = zctx.Pf;

            hr = IDirectDraw7_CreateSurface(dd, &ddsd, &zsurf, NULL);
            if (FAILED(hr) || !zsurf)
            {
                ResultsPrint(TEXT("D3D7 Textured FS: Create Z surface failed: 0x%08lx"), (ULONG)hr);
                zsurf = NULL;
            }
            else
            {
                IDirectDrawSurface7_AddAttachedSurface(back, zsurf);
            }
        }
    }

    if (!CreateTextureFromRgba(dd, &img, &tex, &texIs565))
    {
        if (dev) IDirect3DDevice7_Release(dev);
        if (d3d) IDirect3D7_Release(d3d);
        if (zsurf) IDirectDrawSurface7_Release(zsurf);
        if (back) IDirectDrawSurface7_Release(back);
        if (primary) IDirectDrawSurface7_Release(primary);
        if (dd)
        {
            IDirectDraw7_RestoreDisplayMode(dd);
            IDirectDraw7_Release(dd);
        }
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    BuildCubeTex(cube, 2.0f);

    IDirect3DDevice7_SetRenderState(dev, D3DRENDERSTATE_LIGHTING, FALSE);
    IDirect3DDevice7_SetRenderState(dev, D3DRENDERSTATE_ZENABLE, (zsurf != NULL));
    IDirect3DDevice7_SetRenderState(dev, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

    IDirect3DDevice7_SetTexture(dev, 0, tex);
    IDirect3DDevice7_SetTextureStageState(dev, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    IDirect3DDevice7_SetTextureStageState(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    IDirect3DDevice7_SetTextureStageState(dev, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    IDirect3DDevice7_SetTextureStageState(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    IDirect3DDevice7_SetTextureStageState(dev, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    IDirect3DDevice7_SetTextureStageState(dev, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    IDirect3DDevice7_SetTextureStageState(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    IDirect3DDevice7_SetTextureStageState(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

    {
        D3DVIEWPORT7 vp;
        ZeroMemory(&vp, sizeof(vp));
        vp.dwX = 0;
        vp.dwY = 0;
        vp.dwWidth = (DWORD)w;
        vp.dwHeight = (DWORD)h;
        vp.dvMinZ = 0.0f;
        vp.dvMaxZ = 1.0f;
        IDirect3DDevice7_SetViewport(dev, &vp);
    }

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    quit = FALSE;
    while (1)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                quit = TRUE;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (quit)
            break;

        if (cfg && cfg->StopEvent && WaitForSingleObject(cfg->StopEvent, 0) == WAIT_OBJECT_0)
            break;
        if (frames != 0 && i >= frames)
            break;

        {
            float a = (float)i * 0.01f;
            D3DMATRIX mWorld, mView, mProj, mx, my;
            D3DRECT clearRect;

            ZeroMemory(&mWorld, sizeof(mWorld));
            ZeroMemory(&mView, sizeof(mView));
            ZeroMemory(&mProj, sizeof(mProj));
            ZeroMemory(&mx, sizeof(mx));
            ZeroMemory(&my, sizeof(my));

            mx._11 = 1.0f;
            mx._22 = (float)cos(a * 0.7f);
            mx._23 = (float)sin(a * 0.7f);
            mx._32 = -(float)sin(a * 0.7f);
            mx._33 = (float)cos(a * 0.7f);
            mx._44 = 1.0f;

            my._11 = (float)cos(a);
            my._13 = -(float)sin(a);
            my._22 = 1.0f;
            my._31 = (float)sin(a);
            my._33 = (float)cos(a);
            my._44 = 1.0f;

            /* mWorld = mx * my */
            {
                int rI, cI;
                float* t = (float*)&mWorld;
                float* A = (float*)&mx;
                float* B = (float*)&my;
                for (rI = 0; rI < 4; rI++)
                {
                    for (cI = 0; cI < 4; cI++)
                    {
                        t[rI * 4 + cI] =
                            A[rI * 4 + 0] * B[0 * 4 + cI] +
                            A[rI * 4 + 1] * B[1 * 4 + cI] +
                            A[rI * 4 + 2] * B[2 * 4 + cI] +
                            A[rI * 4 + 3] * B[3 * 4 + cI];
                    }
                }
            }

            mView._11 = 1.0f; mView._22 = 1.0f; mView._33 = 1.0f; mView._44 = 1.0f;
            mView._43 = 3.0f;

            {
                float fov = 1.0f;
                float aspect = (h != 0) ? ((float)w / (float)h) : 1.0f;
                float zn = 0.1f;
                float zf = 100.0f;
                float yScale = 1.0f / (float)tan(fov * 0.5f);
                float xScale = yScale / aspect;
                mProj._11 = xScale;
                mProj._22 = yScale;
                mProj._33 = zf / (zf - zn);
                mProj._34 = 1.0f;
                mProj._43 = (-zn * zf) / (zf - zn);
            }

            IDirect3DDevice7_SetTransform(dev, D3DTRANSFORMSTATE_WORLD, &mWorld);
            IDirect3DDevice7_SetTransform(dev, D3DTRANSFORMSTATE_VIEW, &mView);
            IDirect3DDevice7_SetTransform(dev, D3DTRANSFORMSTATE_PROJECTION, &mProj);

            clearRect.x1 = 0;
            clearRect.y1 = 0;
            clearRect.x2 = (LONG)w;
            clearRect.y2 = (LONG)h;

            IDirect3DDevice7_Clear(dev,
                                  1,
                                  &clearRect,
                                  D3DCLEAR_TARGET | ((zsurf != NULL) ? D3DCLEAR_ZBUFFER : 0),
                                  0x00101010,
                                  1.0f,
                                  0);

            if (SUCCEEDED(IDirect3DDevice7_BeginScene(dev)))
            {
                IDirect3DDevice7_DrawPrimitive(dev, D3DPT_TRIANGLELIST, W32PROF_D3D7_TEX_FVF, cube, 36, 0);
                IDirect3DDevice7_EndScene(dev);
            }

            IDirectDrawSurface7_Flip(primary, NULL, DDFLIP_WAIT);
        }

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("D3D7 Textured Cube (Fullscreen)"));
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("D3D7 Textured Cube (Fullscreen): %lu frames in %.3f ms (%.2f fps)"),
                 (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                    ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                    : 0.0);

    if (tex) IDirectDrawSurface7_Release(tex);
    if (dev) IDirect3DDevice7_Release(dev);
    if (d3d) IDirect3D7_Release(d3d);
    if (zsurf) IDirectDrawSurface7_Release(zsurf);
    if (back) IDirectDrawSurface7_Release(back);
    if (primary) IDirectDrawSurface7_Release(primary);

    if (dd)
    {
        IDirectDraw7_RestoreDisplayMode(dd);
        IDirectDraw7_Release(dd);
    }

    W32Prof_ImageFree(&img);

    DestroyWindow(hRender);
}
