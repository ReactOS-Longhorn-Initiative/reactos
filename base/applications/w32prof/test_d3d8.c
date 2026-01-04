#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

#include <math.h>

#define INITGUID
#include <initguid.h>
#include <d3d8.h>

/* Minimal D3D8 spinning cube (fixed function) */

typedef struct _W32PROF_D3D8VERT
{
    float x, y, z;
    DWORD color;
} W32PROF_D3D8VERT;

#define W32PROF_D3D8_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)

static void
BuildCube(W32PROF_D3D8VERT* v, float s)
{
    const float a = s;
    const W32PROF_D3D8VERT verts[36] =
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

static BOOL
ChooseDepthFormat(IDirect3D8* d3d, D3DFORMAT adapterFormat, D3DFORMAT targetFormat, D3DFORMAT* outDepth)
{
    static const D3DFORMAT kFormats[] =
    {
        D3DFMT_D24S8,
        D3DFMT_D24X8,
        D3DFMT_D16,
        D3DFMT_D32,
    };
    UINT i;
    HRESULT hr;

    if (!d3d || !outDepth)
        return FALSE;

    for (i = 0; i < (UINT)(sizeof(kFormats) / sizeof(kFormats[0])); i++)
    {
        D3DFORMAT fmt = kFormats[i];
        hr = IDirect3D8_CheckDeviceFormat(d3d,
                                         D3DADAPTER_DEFAULT,
                                         D3DDEVTYPE_HAL,
                                         adapterFormat,
                                         D3DUSAGE_DEPTHSTENCIL,
                                         D3DRTYPE_SURFACE,
                                         fmt);
        if (FAILED(hr))
            continue;

        hr = IDirect3D8_CheckDepthStencilMatch(d3d,
                                              D3DADAPTER_DEFAULT,
                                              D3DDEVTYPE_HAL,
                                              adapterFormat,
                                              targetFormat,
                                              fmt);
        if (FAILED(hr))
            continue;

        *outDepth = fmt;
        return TRUE;
    }

    return FALSE;
}

static HRESULT
CreateDeviceWithFallbacks(IDirect3D8* d3d, HWND hwnd, UINT w, UINT h, IDirect3DDevice8** outDev)
{
    D3DDISPLAYMODE mode;
    D3DPRESENT_PARAMETERS pp;
    D3DFORMAT depthFmt;
    BOOL haveMode;
    BOOL haveDepth;
    HRESULT hr;

    if (!d3d || !hwnd || !outDev)
        return E_INVALIDARG;

    *outDev = NULL;
    ZeroMemory(&mode, sizeof(mode));
    haveMode = SUCCEEDED(IDirect3D8_GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &mode));

    ZeroMemory(&pp, sizeof(pp));
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = hwnd;

    /* Some implementations want explicit sizing, others want 0/0. We'll try both. */
    pp.BackBufferWidth = w;
    pp.BackBufferHeight = h;
    pp.BackBufferCount = 1;
    pp.MultiSampleType = D3DMULTISAMPLE_NONE;
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    pp.FullScreen_RefreshRateInHz = 0;
    pp.Flags = 0;

    /* Some implementations reject D3DFMT_UNKNOWN; prefer the current display format. */
    pp.BackBufferFormat = haveMode ? mode.Format : D3DFMT_UNKNOWN;

    depthFmt = D3DFMT_D16;
    haveDepth = haveMode && ChooseDepthFormat(d3d, mode.Format, pp.BackBufferFormat, &depthFmt);
    pp.EnableAutoDepthStencil = haveDepth;
    pp.AutoDepthStencilFormat = depthFmt;

    hr = IDirect3D8_CreateDevice(d3d,
                                D3DADAPTER_DEFAULT,
                                D3DDEVTYPE_HAL,
                                hwnd,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                &pp,
                                outDev);
    if (SUCCEEDED(hr) && *outDev)
        return hr;

    /* Retry without depth/stencil. */
    pp.EnableAutoDepthStencil = FALSE;
    pp.AutoDepthStencilFormat = 0;
    hr = IDirect3D8_CreateDevice(d3d,
                                D3DADAPTER_DEFAULT,
                                D3DDEVTYPE_HAL,
                                hwnd,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                &pp,
                                outDev);
    if (SUCCEEDED(hr) && *outDev)
        return hr;

    /* Retry with 0/0 sizing. */
    pp.BackBufferWidth = 0;
    pp.BackBufferHeight = 0;
    hr = IDirect3D8_CreateDevice(d3d,
                                D3DADAPTER_DEFAULT,
                                D3DDEVTYPE_HAL,
                                hwnd,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                &pp,
                                outDev);
    if (SUCCEEDED(hr) && *outDev)
        return hr;

    /* Retry with BackBufferFormat = UNKNOWN (some implementations prefer it). */
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    hr = IDirect3D8_CreateDevice(d3d,
                                D3DADAPTER_DEFAULT,
                                D3DDEVTYPE_HAL,
                                hwnd,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                &pp,
                                outDev);
    if (SUCCEEDED(hr) && *outDev)
        return hr;

    /* Retry with SwapEffect = COPY (occasionally required in windowed paths). */
    pp.SwapEffect = D3DSWAPEFFECT_COPY;
    hr = IDirect3D8_CreateDevice(d3d,
                                D3DADAPTER_DEFAULT,
                                D3DDEVTYPE_HAL,
                                hwnd,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                &pp,
                                outDev);
    if (SUCCEEDED(hr) && *outDev)
        return hr;

    /* Final fallback: REF device. */
    hr = IDirect3D8_CreateDevice(d3d,
                                D3DADAPTER_DEFAULT,
                                D3DDEVTYPE_REF,
                                hwnd,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                &pp,
                                outDev);
    return hr;
}

void
W32Prof_Test_D3D8Cube(const ProfilerConfig* cfg)
{
    IDirect3D8* d3d = NULL;
    IDirect3DDevice8* dev = NULL;
    IDirect3DVertexBuffer8* vb = NULL;
    D3DMATRIX world, rx, ry, view, proj, tmp;
    HRESULT hr;
    RECT rc;
    UINT w, h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;
    HWND hRender;

    if (!cfg || !cfg->hTestWnd)
        return;

    GetClientRect(cfg->hTestWnd, &rc);
    w = (UINT)(rc.right - rc.left);
    h = (UINT)(rc.bottom - rc.top);
    if (w == 0) w = 640;
    if (h == 0) h = 480;

    if (cfg->Continuous)
        frames = 0;
    else
        frames = (cfg->GpuFrames != 0) ? cfg->GpuFrames : 600;

    hRender = CreateWindowEx(0,
                             TEXT("W32ProfRenderChild"),
                             TEXT(""),
                             WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                             0, 0, (int)w, (int)h,
                             cfg->hTestWnd,
                             NULL,
                             GetModuleHandle(NULL),
                             NULL);

    if (!hRender)
    {
        ResultsPrint(TEXT("D3D8: failed to create render child window"));
        return;
    }

    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d)
    {
        ResultsPrint(TEXT("D3D8: Direct3DCreate8 failed"));
        DestroyWindow(hRender);
        return;
    }

    hr = CreateDeviceWithFallbacks(d3d, hRender, w, h, &dev);
    if (FAILED(hr) || !dev)
    {
        /* Some D3D8 implementations reject child windows as the device window. */
        dev = NULL;
        hr = CreateDeviceWithFallbacks(d3d, cfg->hTestWnd, w, h, &dev);
    }
    if (FAILED(hr) || !dev)
    {
        ResultsPrint(TEXT("D3D8: CreateDevice failed: 0x%08lx"), (ULONG)hr);
        IDirect3D8_Release(d3d);
        DestroyWindow(hRender);
        return;
    }

    hr = IDirect3DDevice8_CreateVertexBuffer(dev,
                                            sizeof(W32PROF_D3D8VERT) * 36,
                                            0,
                                            W32PROF_D3D8_FVF,
                                            D3DPOOL_MANAGED,
                                            &vb);
    if (FAILED(hr) || !vb)
    {
        ResultsPrint(TEXT("D3D8: CreateVertexBuffer failed: 0x%08lx"), (ULONG)hr);
        IDirect3DDevice8_Release(dev);
        IDirect3D8_Release(d3d);
        DestroyWindow(hRender);
        return;
    }

    {
        void* p;
        W32PROF_D3D8VERT cube[36];
        BuildCube(cube, 1.0f);
        hr = IDirect3DVertexBuffer8_Lock(vb, 0, 0, (BYTE**)&p, 0);
        if (SUCCEEDED(hr) && p)
        {
            CopyMemory(p, cube, sizeof(cube));
            IDirect3DVertexBuffer8_Unlock(vb);
        }
    }

    /* View/Projection */
    ZeroMemory(&view, sizeof(view));
    view._11 = 1.0f; view._22 = 1.0f; view._33 = 1.0f; view._44 = 1.0f;
    view._43 = 3.0f;

    ZeroMemory(&proj, sizeof(proj));
    {
        float fov = 1.0f;
        float aspect = (h != 0) ? ((float)w / (float)h) : 1.0f;
        float zn = 0.1f;
        float zf = 100.0f;
        float yScale = 1.0f / (float)tan(fov * 0.5f);
        float xScale = yScale / aspect;
        proj._11 = xScale;
        proj._22 = yScale;
        proj._33 = zf / (zf - zn);
        proj._34 = 1.0f;
        proj._43 = (-zn * zf) / (zf - zn);
    }

    IDirect3DDevice8_SetRenderState(dev, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice8_SetRenderState(dev, D3DRS_ZENABLE, TRUE);
    IDirect3DDevice8_SetRenderState(dev, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice8_SetVertexShader(dev, W32PROF_D3D8_FVF);

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
            ZeroMemory(&rx, sizeof(rx));
            ZeroMemory(&ry, sizeof(ry));
            ZeroMemory(&tmp, sizeof(tmp));
            ZeroMemory(&world, sizeof(world));

            rx._11 = 1.0f;
            rx._22 = (float)cos(a * 0.7f);
            rx._23 = (float)sin(a * 0.7f);
            rx._32 = -(float)sin(a * 0.7f);
            rx._33 = (float)cos(a * 0.7f);
            rx._44 = 1.0f;

            ry._11 = (float)cos(a);
            ry._13 = -(float)sin(a);
            ry._22 = 1.0f;
            ry._31 = (float)sin(a);
            ry._33 = (float)cos(a);
            ry._44 = 1.0f;

            /* tmp = rx * ry (very small/naive) */
            {
                int rI, cI;
                float* t = (float*)&tmp;
                float* A = (float*)&rx;
                float* B = (float*)&ry;
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
            world = tmp;

            IDirect3DDevice8_SetTransform(dev, D3DTS_WORLD, &world);
            IDirect3DDevice8_SetTransform(dev, D3DTS_VIEW, &view);
            IDirect3DDevice8_SetTransform(dev, D3DTS_PROJECTION, &proj);

            IDirect3DDevice8_Clear(dev, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF202020, 1.0f, 0);
            if (SUCCEEDED(IDirect3DDevice8_BeginScene(dev)))
            {
                IDirect3DDevice8_SetStreamSource(dev, 0, vb, sizeof(W32PROF_D3D8VERT));
                IDirect3DDevice8_DrawPrimitive(dev, D3DPT_TRIANGLELIST, 0, 12);
                IDirect3DDevice8_EndScene(dev);
            }

            IDirect3DDevice8_Present(dev, NULL, NULL, NULL, NULL);
        }

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("D3D8 Cube"));
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("D3D8 Cube: %lu frames in %.3f ms (%.2f fps)"),
                 (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                    ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                    : 0.0);

    IDirect3DVertexBuffer8_Release(vb);
    IDirect3DDevice8_Release(dev);
    IDirect3D8_Release(d3d);

    DestroyWindow(hRender);
}
