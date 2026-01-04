#include "profiler.h"
#include "tga.h"

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

typedef struct _W32PROF_D3D9TVERT
{
    float x, y, z;
    DWORD color;
    float u, v;
} W32PROF_D3D9TVERT;

#define W32PROF_D3D9_TEX_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

static void
BuildCubeTex(W32PROF_D3D9TVERT* v, float s)
{
    float hs = s * 0.5f;

    const W32PROF_D3D9TVERT verts[36] =
    {
        /* +Z */
        { -hs, -hs, +hs, 0xFFFFFFFF, 0.0f, 0.0f }, { -hs, +hs, +hs, 0xFFFFFFFF, 0.0f, 1.0f }, { +hs, +hs, +hs, 0xFFFFFFFF, 1.0f, 1.0f },
        { -hs, -hs, +hs, 0xFFFFFFFF, 0.0f, 0.0f }, { +hs, +hs, +hs, 0xFFFFFFFF, 1.0f, 1.0f }, { +hs, -hs, +hs, 0xFFFFFFFF, 1.0f, 0.0f },
        /* -Z */
        { +hs, -hs, -hs, 0xFFFFFFFF, 0.0f, 0.0f }, { +hs, +hs, -hs, 0xFFFFFFFF, 0.0f, 1.0f }, { -hs, +hs, -hs, 0xFFFFFFFF, 1.0f, 1.0f },
        { +hs, -hs, -hs, 0xFFFFFFFF, 0.0f, 0.0f }, { -hs, +hs, -hs, 0xFFFFFFFF, 1.0f, 1.0f }, { -hs, -hs, -hs, 0xFFFFFFFF, 1.0f, 0.0f },
        /* +X */
        { +hs, -hs, +hs, 0xFFFFFFFF, 0.0f, 0.0f }, { +hs, +hs, +hs, 0xFFFFFFFF, 0.0f, 1.0f }, { +hs, +hs, -hs, 0xFFFFFFFF, 1.0f, 1.0f },
        { +hs, -hs, +hs, 0xFFFFFFFF, 0.0f, 0.0f }, { +hs, +hs, -hs, 0xFFFFFFFF, 1.0f, 1.0f }, { +hs, -hs, -hs, 0xFFFFFFFF, 1.0f, 0.0f },
        /* -X */
        { -hs, -hs, -hs, 0xFFFFFFFF, 0.0f, 0.0f }, { -hs, +hs, -hs, 0xFFFFFFFF, 0.0f, 1.0f }, { -hs, +hs, +hs, 0xFFFFFFFF, 1.0f, 1.0f },
        { -hs, -hs, -hs, 0xFFFFFFFF, 0.0f, 0.0f }, { -hs, +hs, +hs, 0xFFFFFFFF, 1.0f, 1.0f }, { -hs, -hs, +hs, 0xFFFFFFFF, 1.0f, 0.0f },
        /* +Y */
        { -hs, +hs, +hs, 0xFFFFFFFF, 0.0f, 0.0f }, { -hs, +hs, -hs, 0xFFFFFFFF, 0.0f, 1.0f }, { +hs, +hs, -hs, 0xFFFFFFFF, 1.0f, 1.0f },
        { -hs, +hs, +hs, 0xFFFFFFFF, 0.0f, 0.0f }, { +hs, +hs, -hs, 0xFFFFFFFF, 1.0f, 1.0f }, { +hs, +hs, +hs, 0xFFFFFFFF, 1.0f, 0.0f },
        /* -Y */
        { -hs, -hs, -hs, 0xFFFFFFFF, 0.0f, 0.0f }, { -hs, -hs, +hs, 0xFFFFFFFF, 0.0f, 1.0f }, { +hs, -hs, +hs, 0xFFFFFFFF, 1.0f, 1.0f },
        { -hs, -hs, -hs, 0xFFFFFFFF, 0.0f, 0.0f }, { +hs, -hs, +hs, 0xFFFFFFFF, 1.0f, 1.0f }, { +hs, -hs, -hs, 0xFFFFFFFF, 1.0f, 0.0f },
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
UploadTextureA8R8G8B8(IDirect3DDevice9* dev, const W32PROF_IMAGE_RGBA* img, IDirect3DTexture9** outTex)
{
    HRESULT hr;
    IDirect3DTexture9* tex;
    D3DLOCKED_RECT lr;
    UINT y;

    if (!dev || !img || !img->Pixels || !outTex)
        return FALSE;

    *outTex = NULL;
    tex = NULL;
    ZeroMemory(&lr, sizeof(lr));

    hr = IDirect3DDevice9_CreateTexture(dev,
                                       img->Width,
                                       img->Height,
                                       1,
                                       0,
                                       D3DFMT_A8R8G8B8,
                                       D3DPOOL_MANAGED,
                                       &tex,
                                       NULL);
    if (FAILED(hr) || !tex)
        return FALSE;

    hr = IDirect3DTexture9_LockRect(tex, 0, &lr, NULL, 0);
    if (FAILED(hr) || !lr.pBits)
    {
        IDirect3DTexture9_Release(tex);
        return FALSE;
    }

    for (y = 0; y < img->Height; y++)
    {
        const BYTE* src = img->Pixels + (SIZE_T)y * img->StrideBytes;
        BYTE* dst = (BYTE*)lr.pBits + (SIZE_T)y * (SIZE_T)lr.Pitch;
        UINT x;
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

    IDirect3DTexture9_UnlockRect(tex, 0);

    *outTex = tex;
    return TRUE;
}

void
W32Prof_Test_D3D9TexturedCube(const ProfilerConfig* cfg)
{
    IDirect3D9* d3d;
    IDirect3DDevice9* dev;
    IDirect3DVertexBuffer9* vb;
    IDirect3DTexture9* tex;
    D3DPRESENT_PARAMETERS pp;
    D3DMATRIX world, rx, ry, view, proj, tmp;
    HRESULT hr;
    RECT rc;
    UINT w, h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;
    HWND hRender;
    W32PROF_IMAGE_RGBA img;

    d3d = NULL;
    dev = NULL;
    vb = NULL;
    tex = NULL;
    ZeroMemory(&img, sizeof(img));

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
        ResultsPrint(TEXT("D3D9 Textured: failed to create render child window"));
        return;
    }

    if (!W32Prof_LoadLogoTestTgaFromResource(GetModuleHandle(NULL), &img))
    {
        ResultsPrint(TEXT("D3D9 Textured: failed to load embedded TGA"));
        DestroyWindow(hRender);
        return;
    }

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        ResultsPrint(TEXT("D3D9 Textured: Direct3DCreate9 failed"));
        W32Prof_ImageFree(&img);
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
        ResultsPrint(TEXT("D3D9 Textured: CreateDevice failed: 0x%08lx"), (ULONG)hr);
        IDirect3D9_Release(d3d);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    if (!UploadTextureA8R8G8B8(dev, &img, &tex))
    {
        ResultsPrint(TEXT("D3D9 Textured: texture upload failed"));
        IDirect3DDevice9_Release(dev);
        IDirect3D9_Release(d3d);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(dev,
                                            sizeof(W32PROF_D3D9TVERT) * 36,
                                            0,
                                            W32PROF_D3D9_TEX_FVF,
                                            D3DPOOL_MANAGED,
                                            &vb,
                                            NULL);
    if (FAILED(hr) || !vb)
    {
        ResultsPrint(TEXT("D3D9 Textured: CreateVertexBuffer failed: 0x%08lx"), (ULONG)hr);
        IDirect3DTexture9_Release(tex);
        IDirect3DDevice9_Release(dev);
        IDirect3D9_Release(d3d);
        W32Prof_ImageFree(&img);
        DestroyWindow(hRender);
        return;
    }

    {
        void* p;
        W32PROF_D3D9TVERT cube[36];
        BuildCubeTex(cube, 1.0f);
        p = NULL;
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
    IDirect3DDevice9_SetFVF(dev, W32PROF_D3D9_TEX_FVF);

    IDirect3DDevice9_SetSamplerState(dev, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    IDirect3DDevice9_SetSamplerState(dev, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    IDirect3DDevice9_SetTexture(dev, 0, (IDirect3DBaseTexture9*)tex);

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);

    i = 0;
    while (1)
    {
        if (cfg && cfg->StopEvent && WaitForSingleObject(cfg->StopEvent, 0) == WAIT_OBJECT_0)
            break;
        if (frames != 0 && i >= frames)
            break;

        {
            float a = (float)i * 0.01f;
            MatRotationX(&rx, a * 0.7f);
            MatRotationY(&ry, a);
            MatMul(&tmp, &rx, &ry);
            world = tmp;
        }

        IDirect3DDevice9_SetTransform(dev, D3DTS_WORLD, &world);
        IDirect3DDevice9_SetTransform(dev, D3DTS_VIEW, &view);
        IDirect3DDevice9_SetTransform(dev, D3DTS_PROJECTION, &proj);

        IDirect3DDevice9_Clear(dev, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF202020, 1.0f, 0);
        if (SUCCEEDED(IDirect3DDevice9_BeginScene(dev)))
        {
            IDirect3DDevice9_SetStreamSource(dev, 0, vb, 0, sizeof(W32PROF_D3D9TVERT));
            IDirect3DDevice9_DrawPrimitive(dev, D3DPT_TRIANGLELIST, 0, 12);
            IDirect3DDevice9_EndScene(dev);
        }

        IDirect3DDevice9_Present(dev, NULL, NULL, NULL, NULL);

        i++;
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("D3D9 Textured Cube: %lu frames in %.3f ms (%.2f fps)"),
                 (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                     ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                     : 0.0);

    W32Prof_ImageFree(&img);
    if (vb) IDirect3DVertexBuffer9_Release(vb);
    if (tex) IDirect3DTexture9_Release(tex);
    if (dev) IDirect3DDevice9_Release(dev);
    if (d3d) IDirect3D9_Release(d3d);

    DestroyWindow(hRender);
}
