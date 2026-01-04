#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

#include <GL/gl.h>

#ifndef APIENTRY
#define APIENTRY WINAPI
#endif

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif

/* Minimal OpenGL 2.0 entry points (loaded via wglGetProcAddress) */
typedef char GLchar;
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint program);

static PFNGLCREATESHADERPROC pglCreateShader;
static PFNGLSHADERSOURCEPROC pglShaderSource;
static PFNGLCOMPILESHADERPROC pglCompileShader;
static PFNGLGETSHADERIVPROC pglGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC pglGetShaderInfoLog;
static PFNGLCREATEPROGRAMPROC pglCreateProgram;
static PFNGLATTACHSHADERPROC pglAttachShader;
static PFNGLLINKPROGRAMPROC pglLinkProgram;
static PFNGLGETPROGRAMIVPROC pglGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC pglGetProgramInfoLog;
static PFNGLUSEPROGRAMPROC pglUseProgram;
static PFNGLDELETESHADERPROC pglDeleteShader;
static PFNGLDELETEPROGRAMPROC pglDeleteProgram;

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

static FARPROC
GetAnyGlProcAddress(const char* name)
{
    FARPROC p = (FARPROC)wglGetProcAddress(name);
    if (p)
        return p;

    /* Some loaders also try opengl32.dll, though GL2+ typically isn't exported there. */
    {
        HMODULE mod = GetModuleHandle(TEXT("opengl32.dll"));
        if (!mod)
            mod = LoadLibrary(TEXT("opengl32.dll"));
        if (mod)
            return GetProcAddress(mod, name);
    }

    return NULL;
}

static void
LoadGl2Procs(void)
{
    pglCreateShader = (PFNGLCREATESHADERPROC)GetAnyGlProcAddress("glCreateShader");
    pglShaderSource = (PFNGLSHADERSOURCEPROC)GetAnyGlProcAddress("glShaderSource");
    pglCompileShader = (PFNGLCOMPILESHADERPROC)GetAnyGlProcAddress("glCompileShader");
    pglGetShaderiv = (PFNGLGETSHADERIVPROC)GetAnyGlProcAddress("glGetShaderiv");
    pglGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GetAnyGlProcAddress("glGetShaderInfoLog");
    pglCreateProgram = (PFNGLCREATEPROGRAMPROC)GetAnyGlProcAddress("glCreateProgram");
    pglAttachShader = (PFNGLATTACHSHADERPROC)GetAnyGlProcAddress("glAttachShader");
    pglLinkProgram = (PFNGLLINKPROGRAMPROC)GetAnyGlProcAddress("glLinkProgram");
    pglGetProgramiv = (PFNGLGETPROGRAMIVPROC)GetAnyGlProcAddress("glGetProgramiv");
    pglGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GetAnyGlProcAddress("glGetProgramInfoLog");
    pglUseProgram = (PFNGLUSEPROGRAMPROC)GetAnyGlProcAddress("glUseProgram");
    pglDeleteShader = (PFNGLDELETESHADERPROC)GetAnyGlProcAddress("glDeleteShader");
    pglDeleteProgram = (PFNGLDELETEPROGRAMPROC)GetAnyGlProcAddress("glDeleteProgram");
}

static void
DumpShaderLog(GLuint obj, BOOL isProgram)
{
    GLchar buf[1024];
    GLsizei len = 0;
    if (isProgram)
    {
        if (pglGetProgramInfoLog)
            pglGetProgramInfoLog(obj, (GLsizei)(sizeof(buf) - 1), &len, buf);
    }
    else
    {
        if (pglGetShaderInfoLog)
            pglGetShaderInfoLog(obj, (GLsizei)(sizeof(buf) - 1), &len, buf);
    }
    buf[(len >= (GLsizei)sizeof(buf)) ? (sizeof(buf) - 1) : len] = 0;
    if (len > 0)
        ResultsPrint(TEXT("GL2 log: %hs"), buf);
}

static GLuint
BuildProgram(void)
{
    const GLchar* vsSrc =
        "void main()\n"
        "{\n"
        "  gl_FrontColor = gl_Color;\n"
        "  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n";

    const GLchar* fsSrc =
        "void main()\n"
        "{\n"
        "  gl_FragColor = gl_Color;\n"
        "}\n";

    GLuint vs, fs, prog;
    GLint ok = 0;

    if (!pglCreateShader || !pglShaderSource || !pglCompileShader || !pglGetShaderiv ||
        !pglCreateProgram || !pglAttachShader || !pglLinkProgram || !pglGetProgramiv ||
        !pglUseProgram || !pglDeleteShader || !pglDeleteProgram)
        return 0;

    vs = pglCreateShader(GL_VERTEX_SHADER);
    fs = pglCreateShader(GL_FRAGMENT_SHADER);
    if (!vs || !fs)
        return 0;

    pglShaderSource(vs, 1, &vsSrc, NULL);
    pglCompileShader(vs);
    pglGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        DumpShaderLog(vs, FALSE);
        pglDeleteShader(vs);
        pglDeleteShader(fs);
        return 0;
    }

    pglShaderSource(fs, 1, &fsSrc, NULL);
    pglCompileShader(fs);
    pglGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        DumpShaderLog(fs, FALSE);
        pglDeleteShader(vs);
        pglDeleteShader(fs);
        return 0;
    }

    prog = pglCreateProgram();
    pglAttachShader(prog, vs);
    pglAttachShader(prog, fs);
    pglLinkProgram(prog);
    pglGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        DumpShaderLog(prog, TRUE);
        pglDeleteShader(vs);
        pglDeleteShader(fs);
        pglDeleteProgram(prog);
        return 0;
    }

    pglDeleteShader(vs);
    pglDeleteShader(fs);

    return prog;
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
W32Prof_Test_GL20Cube(const ProfilerConfig* cfg)
{
    HDC hdc;
    HGLRC rc;
    GLuint prog = 0;
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
        ResultsPrint(TEXT("OpenGL 2.0: failed to create render child window"));
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
        ResultsPrint(TEXT("OpenGL 2.0: Choose/SetPixelFormat failed"));
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    rc = wglCreateContext(hdc);
    if (!rc)
    {
        ResultsPrint(TEXT("OpenGL 2.0: wglCreateContext failed: %lu"), GetLastError());
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    if (!wglMakeCurrent(hdc, rc))
    {
        ResultsPrint(TEXT("OpenGL 2.0: wglMakeCurrent failed: %lu"), GetLastError());
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    LoadGl2Procs();
    prog = BuildProgram();
    if (!prog)
    {
        ResultsPrint(TEXT("OpenGL 2.0: shader program not available (missing GL2 entry points?)"));
        wglMakeCurrent(NULL, NULL);
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

    pglUseProgram(prog);

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

        glClearColor(0.10f, 0.10f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -6.0f);
        glRotatef(a, 0.6f, 1.0f, 0.2f);

            DrawCube();
            SwapBuffers(hdc);
        }

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("OpenGL 2.0 Cube"));
    }

    QueryPerformanceCounter(&q1);

     ResultsPrint(TEXT("OpenGL 2.0 Cube: %lu frames in %.3f ms (%.2f fps)"),
                      (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                          ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                    : 0.0);

    pglUseProgram(0);
    pglDeleteProgram(prog);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(hRender, hdc);
    DestroyWindow(hRender);
}
