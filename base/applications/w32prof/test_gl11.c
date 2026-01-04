#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

#include <GL/gl.h>

static double
TicksToMs(LONGLONG ticks, LONGLONG freq)
{
    if (freq <= 0)
        return 0.0;
    return ((double)ticks * 1000.0) / (double)freq;
}

static BOOL
EnsurePixelFormatSet(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    int pf;

    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    pf = ChoosePixelFormat(hdc, &pfd);
    if (pf == 0)
        return FALSE;

    /* If already set, SetPixelFormat may fail; treat that as non-fatal. */
    if (!SetPixelFormat(hdc, pf, &pfd))
        return TRUE;

    return TRUE;
}

static void
DrawCube(void)
{
    glBegin(GL_QUADS);

    /* +Z */
    glColor3f(1, 0, 0);
    glVertex3f(-1, -1,  1); glVertex3f(-1,  1,  1); glVertex3f( 1,  1,  1); glVertex3f( 1, -1,  1);
    /* -Z */
    glColor3f(0, 1, 0);
    glVertex3f( 1, -1, -1); glVertex3f( 1,  1, -1); glVertex3f(-1,  1, -1); glVertex3f(-1, -1, -1);
    /* +X */
    glColor3f(0, 0, 1);
    glVertex3f( 1, -1,  1); glVertex3f( 1,  1,  1); glVertex3f( 1,  1, -1); glVertex3f( 1, -1, -1);
    /* -X */
    glColor3f(1, 1, 0);
    glVertex3f(-1, -1, -1); glVertex3f(-1,  1, -1); glVertex3f(-1,  1,  1); glVertex3f(-1, -1,  1);
    /* +Y */
    glColor3f(1, 0, 1);
    glVertex3f(-1,  1,  1); glVertex3f(-1,  1, -1); glVertex3f( 1,  1, -1); glVertex3f( 1,  1,  1);
    /* -Y */
    glColor3f(0, 1, 1);
    glVertex3f(-1, -1, -1); glVertex3f(-1, -1,  1); glVertex3f( 1, -1,  1); glVertex3f( 1, -1, -1);

    glEnd();
}

void
W32Prof_Test_GL11Cube(const ProfilerConfig* cfg)
{
    HDC hdc;
    HGLRC rc;
    RECT r;
    int w, h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;
    HWND hRender;

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
        ResultsPrint(TEXT("OpenGL 1.1: failed to create render child window"));
        return;
    }

    hdc = GetDC(hRender);
    if (!hdc)
    {
        DestroyWindow(hRender);
        return;
    }

    if (!EnsurePixelFormatSet(hdc))
    {
        ResultsPrint(TEXT("OpenGL 1.1: Choose/SetPixelFormat failed"));
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    rc = wglCreateContext(hdc);
    if (!rc)
    {
        ResultsPrint(TEXT("OpenGL 1.1: wglCreateContext failed: %lu"), GetLastError());
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    if (!wglMakeCurrent(hdc, rc))
    {
        ResultsPrint(TEXT("OpenGL 1.1: wglMakeCurrent failed: %lu"), GetLastError());
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    {
        double aspect = (h != 0) ? ((double)w / (double)h) : 1.0;
        double f = 1.0;
        glFrustum(-aspect * f, aspect * f, -f, f, 1.5, 50.0);
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
            float a = (float)i * 0.6f;

        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -6.0f);
        glRotatef(a, 0.7f, 1.0f, 0.0f);

            DrawCube();
            SwapBuffers(hdc);
        }

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("OpenGL 1.1 Cube"));
    }

    QueryPerformanceCounter(&q1);

     ResultsPrint(TEXT("OpenGL 1.1 Cube: %lu frames in %.3f ms (%.2f fps)"),
                      (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                          ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                    : 0.0);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(hRender, hdc);
    DestroyWindow(hRender);
}
