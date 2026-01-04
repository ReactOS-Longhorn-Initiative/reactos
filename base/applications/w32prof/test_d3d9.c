#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

#include <math.h>

#include <d3d9.h>

static void
MatIdentity(D3DMATRIX* m)
{
    ZeroMemory(m, sizeof(*m));
    m->_11 = 1.0f; m->_22 = 1.0f; m->_33 = 1.0f; m->_44 = 1.0f;
}

static void
MatMul(D3DMATRIX* out, const D3DMATRIX* a, const D3DMATRIX* b)
{
    D3DMATRIX r;
    int row, col;
    const float* A = (const float*)a;
    const float* B = (const float*)b;
    float* R = (float*)&r;

    for (row = 0; row < 4; row++)
    {
        for (col = 0; col < 4; col++)
        {
            R[row * 4 + col] =
                A[row * 4 + 0] * B[0 * 4 + col] +
                A[row * 4 + 1] * B[1 * 4 + col] +
                A[row * 4 + 2] * B[2 * 4 + col] +
                A[row * 4 + 3] * B[3 * 4 + col];
        }
    }

    *out = r;
}

static void
MatRotationY(D3DMATRIX* m, float a)
{
    MatIdentity(m);
    m->_11 = (float)cos(a);
    m->_13 = (float)sin(a);
    m->_31 = (float)-sin(a);
    m->_33 = (float)cos(a);
}

static void
MatRotationX(D3DMATRIX* m, float a)
{
    MatIdentity(m);
    m->_22 = (float)cos(a);
    m->_23 = (float)-sin(a);
    m->_32 = (float)sin(a);
    m->_33 = (float)cos(a);
}

static void
MatTranslation(D3DMATRIX* m, float x, float y, float z)
{
    MatIdentity(m);
    m->_41 = x;
    m->_42 = y;
    m->_43 = z;
}

static void
MatPerspectiveFovLH(D3DMATRIX* m, float fovy, float aspect, float zn, float zf)
{
    float yScale = 1.0f / (float)tan(fovy * 0.5f);
    float xScale = yScale / aspect;

    ZeroMemory(m, sizeof(*m));
    m->_11 = xScale;
    m->_22 = yScale;
    m->_33 = zf / (zf - zn);
    m->_34 = 1.0f;
    m->_43 = (-zn * zf) / (zf - zn);
}

typedef struct _W32PROF_D3DVERT
{
    float x, y, z;
    DWORD color;
} W32PROF_D3DVERT;

#define W32PROF_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)

static void
BuildCube(W32PROF_D3DVERT* v, float s)
{
    float hs = s * 0.5f;

    /* 12 triangles, 36 verts */
    const W32PROF_D3DVERT verts[36] =
    {
        /* +Z */
        { -hs, -hs, +hs, 0xFFFF0000 }, { -hs, +hs, +hs, 0xFFFF0000 }, { +hs, +hs, +hs, 0xFFFF0000 },
        { -hs, -hs, +hs, 0xFFFF0000 }, { +hs, +hs, +hs, 0xFFFF0000 }, { +hs, -hs, +hs, 0xFFFF0000 },
        /* -Z */
        { +hs, -hs, -hs, 0xFF00FF00 }, { +hs, +hs, -hs, 0xFF00FF00 }, { -hs, +hs, -hs, 0xFF00FF00 },
        { +hs, -hs, -hs, 0xFF00FF00 }, { -hs, +hs, -hs, 0xFF00FF00 }, { -hs, -hs, -hs, 0xFF00FF00 },
        /* +X */
        { +hs, -hs, +hs, 0xFF0000FF }, { +hs, +hs, +hs, 0xFF0000FF }, { +hs, +hs, -hs, 0xFF0000FF },
        { +hs, -hs, +hs, 0xFF0000FF }, { +hs, +hs, -hs, 0xFF0000FF }, { +hs, -hs, -hs, 0xFF0000FF },
        /* -X */
        { -hs, -hs, -hs, 0xFFFFFF00 }, { -hs, +hs, -hs, 0xFFFFFF00 }, { -hs, +hs, +hs, 0xFFFFFF00 },
        { -hs, -hs, -hs, 0xFFFFFF00 }, { -hs, +hs, +hs, 0xFFFFFF00 }, { -hs, -hs, +hs, 0xFFFFFF00 },
        /* +Y */
        { -hs, +hs, +hs, 0xFFFF00FF }, { -hs, +hs, -hs, 0xFFFF00FF }, { +hs, +hs, -hs, 0xFFFF00FF },
        { -hs, +hs, +hs, 0xFFFF00FF }, { +hs, +hs, -hs, 0xFFFF00FF }, { +hs, +hs, +hs, 0xFFFF00FF },
        /* -Y */
        { -hs, -hs, -hs, 0xFF00FFFF }, { -hs, -hs, +hs, 0xFF00FFFF }, { +hs, -hs, +hs, 0xFF00FFFF },
        { -hs, -hs, -hs, 0xFF00FFFF }, { +hs, -hs, +hs, 0xFF00FFFF }, { +hs, -hs, -hs, 0xFF00FFFF },
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
W32Prof_Test_D3D9Cube(const ProfilerConfig* cfg)
{
    IDirect3D9* d3d = NULL;
    IDirect3DDevice9* dev = NULL;
    IDirect3DVertexBuffer9* vb = NULL;
    D3DPRESENT_PARAMETERS pp;
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
        ResultsPrint(TEXT("D3D9: failed to create render child window"));
        return;
    }

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        ResultsPrint(TEXT("D3D9: Direct3DCreate9 failed"));
        DestroyWindow(hRender);
        return;
    }

    ZeroMemory(&pp, sizeof(pp));
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = hRender;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.BackBufferWidth = w;
    pp.BackBufferHeight = h;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice(d3d,
                                D3DADAPTER_DEFAULT,
                                D3DDEVTYPE_HAL,
                                hRender,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                &pp,
                                &dev);
    if (FAILED(hr) || !dev)
    {
        ResultsPrint(TEXT("D3D9: CreateDevice failed: 0x%08lx"), (ULONG)hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(hRender);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(dev,
                                            sizeof(W32PROF_D3DVERT) * 36,
                                            0,
                                            W32PROF_FVF,
                                            D3DPOOL_MANAGED,
                                            &vb,
                                            NULL);
    if (FAILED(hr) || !vb)
    {
        ResultsPrint(TEXT("D3D9: CreateVertexBuffer failed: 0x%08lx"), (ULONG)hr);
        IDirect3DDevice9_Release(dev);
        IDirect3D9_Release(d3d);
        return;
    }

    {
        void* p;
        W32PROF_D3DVERT cube[36];
        BuildCube(cube, 1.0f);
        hr = IDirect3DVertexBuffer9_Lock(vb, 0, 0, &p, 0);
        if (SUCCEEDED(hr) && p)
        {
            CopyMemory(p, cube, sizeof(cube));
            IDirect3DVertexBuffer9_Unlock(vb);
        }
    }

    MatTranslation(&view, 0.0f, 0.0f, 3.0f);
    MatPerspectiveFovLH(&proj, 1.0f, (float)w / (float)h, 0.1f, 100.0f);

    IDirect3DDevice9_SetRenderState(dev, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetRenderState(dev, D3DRS_ZENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(dev, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice9_SetFVF(dev, W32PROF_FVF);

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

        float a = (float)i * 0.01f;
        MatRotationX(&rx, a * 0.7f);
        MatRotationY(&ry, a);
        MatMul(&tmp, &rx, &ry);
        world = tmp;

        IDirect3DDevice9_SetTransform(dev, D3DTS_WORLD, &world);
        IDirect3DDevice9_SetTransform(dev, D3DTS_VIEW, &view);
        IDirect3DDevice9_SetTransform(dev, D3DTS_PROJECTION, &proj);

        IDirect3DDevice9_Clear(dev, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF202020, 1.0f, 0);
        if (SUCCEEDED(IDirect3DDevice9_BeginScene(dev)))
        {
            IDirect3DDevice9_SetStreamSource(dev, 0, vb, 0, sizeof(W32PROF_D3DVERT));
            IDirect3DDevice9_DrawPrimitive(dev, D3DPT_TRIANGLELIST, 0, 12);
            IDirect3DDevice9_EndScene(dev);
        }

        IDirect3DDevice9_Present(dev, NULL, NULL, NULL, NULL);

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("D3D9 Cube"));
    }

    QueryPerformanceCounter(&q1);

     ResultsPrint(TEXT("D3D9 Cube: %lu frames in %.3f ms (%.2f fps)"),
                      (ULONG)i,
                      TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                      (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                          ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                          : 0.0);

    IDirect3DVertexBuffer9_Release(vb);
    IDirect3DDevice9_Release(dev);
    IDirect3D9_Release(d3d);

    DestroyWindow(hRender);
}
