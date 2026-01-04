#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>
#include <math.h>

#define INITGUID
#include <initguid.h>
#include <ddraw.h>
#include <d3d.h>

/*
 * D3D7 spinning cube test.
 *
 * Notes:
 * - Uses DirectDraw7 + Direct3D7 immediate-mode (DrawPrimitive).
 * - Renders to an offscreen 3D surface and blits to the window.
 */

typedef struct _W32PROF_D3D7VERT
{
    float x, y, z;
    DWORD color;
} W32PROF_D3D7VERT;

#define W32PROF_D3D7_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)

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

    /* Prefer a 16-bit Z buffer if offered. */
    if ((ddpf->dwFlags & DDPF_ZBUFFER) && ddpf->dwZBufferBitDepth == 16)
    {
        ctx->Pf = *ddpf;
        ctx->Found = TRUE;
        return D3DENUMRET_CANCEL;
    }

    /* Otherwise keep the first Z format we see as a fallback. */
    if (!ctx->Found && (ddpf->dwFlags & DDPF_ZBUFFER) && ddpf->dwZBufferBitDepth != 0)
    {
        ctx->Pf = *ddpf;
        ctx->Found = TRUE;
    }

    return D3DENUMRET_OK;
}

static void
BuildCube(W32PROF_D3D7VERT* v, float s)
{
    const float a = s;
    const W32PROF_D3D7VERT verts[36] =
    {
        /* +Z */
        { -a, -a,  a, 0xFFFF0000 }, { -a,  a,  a, 0xFFFF0000 }, {  a,  a,  a, 0xFFFF0000 },
        { -a, -a,  a, 0xFFFF0000 }, {  a,  a,  a, 0xFFFF0000 }, {  a, -a,  a, 0xFFFF0000 },
        /* -Z */
        {  a, -a, -a, 0xFF00FF00 }, {  a,  a, -a, 0xFF00FF00 }, { -a,  a, -a, 0xFF00FF00 },
        {  a, -a, -a, 0xFF00FF00 }, { -a,  a, -a, 0xFF00FF00 }, { -a, -a, -a, 0xFF00FF00 },
        /* +X */
        {  a, -a,  a, 0xFF0000FF }, {  a,  a,  a, 0xFF0000FF }, {  a,  a, -a, 0xFF0000FF },
        {  a, -a,  a, 0xFF0000FF }, {  a,  a, -a, 0xFF0000FF }, {  a, -a, -a, 0xFF0000FF },
        /* -X */
        { -a, -a, -a, 0xFFFFFF00 }, { -a,  a, -a, 0xFFFFFF00 }, { -a,  a,  a, 0xFFFFFF00 },
        { -a, -a, -a, 0xFFFFFF00 }, { -a,  a,  a, 0xFFFFFF00 }, { -a, -a,  a, 0xFFFFFF00 },
        /* +Y */
        { -a,  a,  a, 0xFFFF00FF }, { -a,  a, -a, 0xFFFF00FF }, {  a,  a, -a, 0xFFFF00FF },
        { -a,  a,  a, 0xFFFF00FF }, {  a,  a, -a, 0xFFFF00FF }, {  a,  a,  a, 0xFFFF00FF },
        /* -Y */
        { -a, -a, -a, 0xFF00FFFF }, { -a, -a,  a, 0xFF00FFFF }, {  a, -a,  a, 0xFF00FFFF },
        { -a, -a, -a, 0xFF00FFFF }, {  a, -a,  a, 0xFF00FFFF }, {  a, -a, -a, 0xFF00FFFF },
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

void
W32Prof_Test_D3D7Cube(const ProfilerConfig* cfg)
{
    HRESULT hr;
    RECT rc;
    int w, h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    HWND hRender = NULL;
    LPDIRECTDRAW7 dd = NULL;
    LPDIRECT3D7 d3d = NULL;
    LPDIRECT3DDEVICE7 dev = NULL;
    LPDIRECTDRAWSURFACE7 surf3d = NULL;
    LPDIRECTDRAWSURFACE7 zsurf = NULL;
    LPDIRECTDRAWSURFACE7 primary = NULL;
    LPDIRECTDRAWCLIPPER clipper = NULL;

    CLSID deviceClsid;

    W32PROF_D3D7VERT cube[36];

    if (!cfg || !cfg->hTestWnd)
        return;

    GetClientRect(cfg->hTestWnd, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    if (w <= 0) w = 640;
    if (h <= 0) h = 480;

    if (cfg->Continuous)
        frames = 0;
    else
        frames = (cfg->GpuFrames != 0) ? cfg->GpuFrames : 600;

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
        ResultsPrint(TEXT("D3D7: failed to create render child window"));
        return;
    }

    hr = DirectDrawCreateEx(NULL, (void**)&dd, &IID_IDirectDraw7, NULL);
    if (FAILED(hr) || !dd)
    {
        ResultsPrint(TEXT("D3D7: DirectDrawCreateEx failed: 0x%08lx"), (ULONG)hr);
        DestroyWindow(hRender);
        return;
    }

    hr = IDirectDraw7_SetCooperativeLevel(dd, hRender, DDSCL_NORMAL);
    if (FAILED(hr))
    {
        ResultsPrint(TEXT("D3D7: SetCooperativeLevel failed: 0x%08lx"), (ULONG)hr);
        IDirectDraw7_Release(dd);
        DestroyWindow(hRender);
        return;
    }

    {
        DDSURFACEDESC2 ddsd;
        DDSURFACEDESC2 primaryDesc;
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = IDirectDraw7_CreateSurface(dd, &ddsd, &primary, NULL);
        if (FAILED(hr) || !primary)
        {
            ResultsPrint(TEXT("D3D7: Create primary surface failed: 0x%08lx"), (ULONG)hr);
            IDirectDraw7_Release(dd);
            DestroyWindow(hRender);
            return;
        }

        hr = IDirectDraw7_CreateClipper(dd, 0, &clipper, NULL);
        if (SUCCEEDED(hr) && clipper)
        {
            IDirectDrawClipper_SetHWnd(clipper, 0, hRender);
            IDirectDrawSurface7_SetClipper(primary, clipper);
        }

        ZeroMemory(&primaryDesc, sizeof(primaryDesc));
        primaryDesc.dwSize = sizeof(primaryDesc);
        if (FAILED(IDirectDrawSurface7_GetSurfaceDesc(primary, &primaryDesc)))
            ZeroMemory(&primaryDesc, sizeof(primaryDesc));

        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
        ddsd.dwWidth = (DWORD)w;
        ddsd.dwHeight = (DWORD)h;

        /* Make the render surface match the primary pixel format. This avoids
         * driver/emulation conversion artifacts when blitting in windowed mode.
         */
        if (primaryDesc.dwSize == sizeof(primaryDesc) &&
            primaryDesc.ddpfPixelFormat.dwSize == sizeof(DDPIXELFORMAT) &&
            primaryDesc.ddpfPixelFormat.dwFlags != 0)
        {
            ddsd.dwFlags |= DDSD_PIXELFORMAT;
            ddsd.ddpfPixelFormat = primaryDesc.ddpfPixelFormat;
        }

        hr = IDirectDraw7_CreateSurface(dd, &ddsd, &surf3d, NULL);
        if (FAILED(hr) || !surf3d)
        {
            ResultsPrint(TEXT("D3D7: Create 3D surface failed: 0x%08lx"), (ULONG)hr);
            if (clipper) IDirectDrawClipper_Release(clipper);
            IDirectDrawSurface7_Release(primary);
            IDirectDraw7_Release(dd);
            DestroyWindow(hRender);
            return;
        }

        /* Do NOT create a Z surface yet: the correct Z pixel format depends on the
         * chosen D3D device, so we create it after CreateDevice via EnumZBufferFormats.
         */
    }

    hr = IDirectDraw7_QueryInterface(dd, &IID_IDirect3D7, (void**)&d3d);
    if (FAILED(hr) || !d3d)
    {
        ResultsPrint(TEXT("D3D7: QI IID_IDirect3D7 failed: 0x%08lx"), (ULONG)hr);
        if (zsurf) IDirectDrawSurface7_Release(zsurf);
        IDirectDrawSurface7_Release(surf3d);
        if (clipper) IDirectDrawClipper_Release(clipper);
        IDirectDrawSurface7_Release(primary);
        IDirectDraw7_Release(dd);
        DestroyWindow(hRender);
        return;
    }

    deviceClsid = IID_IDirect3DHALDevice;
    hr = IDirect3D7_CreateDevice(d3d, &deviceClsid, surf3d, &dev);
    if (FAILED(hr) || !dev)
    {
        deviceClsid = IID_IDirect3DRGBDevice;
        hr = IDirect3D7_CreateDevice(d3d, &deviceClsid, surf3d, &dev);
    }

    if (FAILED(hr) || !dev)
    {
        ResultsPrint(TEXT("D3D7: CreateDevice failed: 0x%08lx"), (ULONG)hr);
        IDirect3D7_Release(d3d);
        if (zsurf) IDirectDrawSurface7_Release(zsurf);
        IDirectDrawSurface7_Release(surf3d);
        if (clipper) IDirectDrawClipper_Release(clipper);
        IDirectDrawSurface7_Release(primary);
        IDirectDraw7_Release(dd);
        DestroyWindow(hRender);
        return;
    }

    /* Now that we know the device type, query a supported Z format and recreate Z. */
    if (zsurf)
    {
        IDirectDrawSurface7_DeleteAttachedSurface(surf3d, 0, zsurf);
        IDirectDrawSurface7_Release(zsurf);
        zsurf = NULL;
    }

    {
        W32PROF_ZENUM_CTX zctx;
        DDSURFACEDESC2 ddsd;
        DDSURFACEDESC2 surfDesc;
        DWORD memCaps;
        ZeroMemory(&zctx, sizeof(zctx));
        zctx.Pf.dwSize = sizeof(zctx.Pf);

        hr = IDirect3D7_EnumZBufferFormats(d3d, &deviceClsid, (LPD3DENUMPIXELFORMATSCALLBACK)EnumZBufCallback, &zctx);
        (void)hr;

        if (!zctx.Found)
        {
            ResultsPrint(TEXT("D3D7: no Z buffer formats reported; disabling Z"));
        }
        else
        {
            /* Match the Z surface memory pool to the render surface; some drivers
             * reject Z buffers in a different memory type.
             */
            ZeroMemory(&surfDesc, sizeof(surfDesc));
            surfDesc.dwSize = sizeof(surfDesc);
            memCaps = 0;
            if (SUCCEEDED(IDirectDrawSurface7_GetSurfaceDesc(surf3d, &surfDesc)))
            {
                memCaps = surfDesc.ddsCaps.dwCaps & (DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY |
                                                     DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM);
            }

            ZeroMemory(&ddsd, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | memCaps;
            ddsd.dwWidth = (DWORD)w;
            ddsd.dwHeight = (DWORD)h;
            ddsd.ddpfPixelFormat = zctx.Pf;

            hr = IDirectDraw7_CreateSurface(dd, &ddsd, &zsurf, NULL);
            if (FAILED(hr) || !zsurf)
            {
                ResultsPrint(TEXT("D3D7: Create Z surface failed (enum-picked): 0x%08lx"), (ULONG)hr);
                zsurf = NULL;
            }
            else
            {
                IDirectDrawSurface7_AddAttachedSurface(surf3d, zsurf);
            }
        }
    }

    BuildCube(cube, 1.0f);

    IDirect3DDevice7_SetRenderState(dev, D3DRENDERSTATE_LIGHTING, FALSE);
    IDirect3DDevice7_SetRenderState(dev, D3DRENDERSTATE_ZENABLE, (zsurf != NULL));
    IDirect3DDevice7_SetRenderState(dev, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

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
    while (1)
    {
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
                                  0x00202020,
                                  1.0f,
                                  0);

            if (SUCCEEDED(IDirect3DDevice7_BeginScene(dev)))
            {
                IDirect3DDevice7_DrawPrimitive(dev, D3DPT_TRIANGLELIST, W32PROF_D3D7_FVF, cube, 36, 0);
                IDirect3DDevice7_EndScene(dev);
            }

            /* Present: blit offscreen surface to primary/window */
            {
                BOOL presented = FALSE;

                /* Prefer a GDI blit directly to our child window. On modern Windows
                 * the "primary surface" path is frequently emulated and can produce
                 * smearing/overdraw artifacts.
                 */
                {
                    HDC hdcSrc = NULL;
                    if (SUCCEEDED(IDirectDrawSurface7_GetDC(surf3d, &hdcSrc)) && hdcSrc)
                    {
                        HDC hdcDst = GetDC(hRender);
                        if (hdcDst)
                        {
                            BitBlt(hdcDst, 0, 0, w, h, hdcSrc, 0, 0, SRCCOPY);
                            ReleaseDC(hRender, hdcDst);
                            presented = TRUE;
                        }
                        IDirectDrawSurface7_ReleaseDC(surf3d, hdcSrc);
                    }
                }

                if (!presented)
                {
                    RECT dst;
                    POINT pt;
                    GetClientRect(hRender, &dst);
                    pt.x = dst.left;
                    pt.y = dst.top;
                    ClientToScreen(hRender, &pt);
                    OffsetRect(&dst, pt.x, pt.y);
                    IDirectDrawSurface7_Blt(primary, &dst, surf3d, NULL, DDBLT_WAIT, NULL);
                }
            }
        }

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("D3D7 Cube"));
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("D3D7 Cube: %lu frames in %.3f ms (%.2f fps)"),
                 (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                    ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                    : 0.0);

    if (dev) IDirect3DDevice7_Release(dev);
    if (d3d) IDirect3D7_Release(d3d);
    if (zsurf) IDirectDrawSurface7_Release(zsurf);
    if (surf3d) IDirectDrawSurface7_Release(surf3d);
    if (clipper) IDirectDrawClipper_Release(clipper);
    if (primary) IDirectDrawSurface7_Release(primary);
    if (dd) IDirectDraw7_Release(dd);

    DestroyWindow(hRender);
}
