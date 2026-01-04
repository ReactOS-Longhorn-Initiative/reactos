#include "profiler.h"
#include "tga.h"
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

    if (!SetPixelFormat(hdc, pf, &pfd))
        return TRUE;

    return TRUE;
}

static void
DrawTexturedCube(void)
{
    glBegin(GL_QUADS);

    glColor3f(1, 1, 1);

    /* +Z */
    glTexCoord2f(0, 0); glVertex3f(-1, -1,  1);
    glTexCoord2f(0, 1); glVertex3f(-1,  1,  1);
    glTexCoord2f(1, 1); glVertex3f( 1,  1,  1);
    glTexCoord2f(1, 0); glVertex3f( 1, -1,  1);

    /* -Z */
    glTexCoord2f(0, 0); glVertex3f( 1, -1, -1);
    glTexCoord2f(0, 1); glVertex3f( 1,  1, -1);
    glTexCoord2f(1, 1); glVertex3f(-1,  1, -1);
    glTexCoord2f(1, 0); glVertex3f(-1, -1, -1);

    /* +X */
    glTexCoord2f(0, 0); glVertex3f( 1, -1,  1);
    glTexCoord2f(0, 1); glVertex3f( 1,  1,  1);
    glTexCoord2f(1, 1); glVertex3f( 1,  1, -1);
    glTexCoord2f(1, 0); glVertex3f( 1, -1, -1);

    /* -X */
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(0, 1); glVertex3f(-1,  1, -1);
    glTexCoord2f(1, 1); glVertex3f(-1,  1,  1);
    glTexCoord2f(1, 0); glVertex3f(-1, -1,  1);

    /* +Y */
    glTexCoord2f(0, 0); glVertex3f(-1,  1,  1);
    glTexCoord2f(0, 1); glVertex3f(-1,  1, -1);
    glTexCoord2f(1, 1); glVertex3f( 1,  1, -1);
    glTexCoord2f(1, 0); glVertex3f( 1,  1,  1);

    /* -Y */
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(0, 1); glVertex3f(-1, -1,  1);
    glTexCoord2f(1, 1); glVertex3f( 1, -1,  1);
    glTexCoord2f(1, 0); glVertex3f( 1, -1, -1);

    glEnd();
}

void
W32Prof_Test_GL11TexturedCubeFullscreen(const ProfilerConfig* cfg)
{
    HDC hdc;
    HGLRC rc;
    int w, h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;
    HWND hRender;
    GLuint tex = 0;
    W32PROF_IMAGE_RGBA img;
    MSG msg;
    BOOL quit;

    if (!cfg)
        return;

    ZeroMemory(&img, sizeof(img));

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
        ResultsPrint(TEXT("OpenGL 1.1 Textured FS: failed to create fullscreen window"));
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
        ResultsPrint(TEXT("OpenGL 1.1 Textured FS: Choose/SetPixelFormat failed"));
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    rc = wglCreateContext(hdc);
    if (!rc)
    {
        ResultsPrint(TEXT("OpenGL 1.1 Textured FS: wglCreateContext failed: %lu"), GetLastError());
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    if (!wglMakeCurrent(hdc, rc))
    {
        ResultsPrint(TEXT("OpenGL 1.1 Textured FS: wglMakeCurrent failed: %lu"), GetLastError());
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    if (!W32Prof_LoadLogoTestTgaFromResource(GetModuleHandle(NULL), &img))
    {
        ResultsPrint(TEXT("OpenGL 1.1 Textured FS: failed to load embedded TGA"));
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)img.Width, (GLsizei)img.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Pixels);

    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

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
            float a = (float)i * 0.6f;

            glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glTranslatef(0.0f, 0.0f, -6.0f);
            glRotatef(a, 0.7f, 1.0f, 0.0f);

            glBindTexture(GL_TEXTURE_2D, tex);
            DrawTexturedCube();

            SwapBuffers(hdc);
        }

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("OpenGL 1.1 Textured Cube (Fullscreen)"));
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("OpenGL 1.1 Textured Cube (Fullscreen): %lu frames in %.3f ms (%.2f fps)"),
                 (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                     ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                     : 0.0);

    if (tex)
        glDeleteTextures(1, &tex);
    W32Prof_ImageFree(&img);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(hRender, hdc);
    DestroyWindow(hRender);
}
