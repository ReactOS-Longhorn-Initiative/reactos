#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

#include <math.h>
#include <stddef.h>

#include <GL/gl.h>

#ifndef APIENTRY
#define APIENTRY WINAPI
#endif

/* Minimal GL constants (not in 1.1 headers) */
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
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif

/* WGL context attributes */
#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#endif
#ifndef WGL_CONTEXT_MINOR_VERSION_ARB
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#endif
#ifndef WGL_CONTEXT_PROFILE_MASK_ARB
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#endif
#ifndef WGL_CONTEXT_CORE_PROFILE_BIT_ARB
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int* attribList);

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
typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrays);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, ptrdiff_t size, const void* data, GLenum usage);
typedef GLint (APIENTRY *PFNGLGETATTRIBLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

static PFNWGLCREATECONTEXTATTRIBSARBPROC pwglCreateContextAttribsARB;

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
static PFNGLGENVERTEXARRAYSPROC pglGenVertexArrays;
static PFNGLBINDVERTEXARRAYPROC pglBindVertexArray;
static PFNGLGENBUFFERSPROC pglGenBuffers;
static PFNGLBINDBUFFERPROC pglBindBuffer;
static PFNGLBUFFERDATAPROC pglBufferData;
static PFNGLGETATTRIBLOCATIONPROC pglGetAttribLocation;
static PFNGLVERTEXATTRIBPOINTERPROC pglVertexAttribPointer;
static PFNGLENABLEVERTEXATTRIBARRAYPROC pglEnableVertexAttribArray;
static PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation;
static PFNGLUNIFORMMATRIX4FVPROC pglUniformMatrix4fv;

static FARPROC
GetAnyGlProcAddress(const char* name)
{
    FARPROC p = (FARPROC)wglGetProcAddress(name);
    if (p)
        return p;

    {
        HMODULE mod = GetModuleHandle(TEXT("opengl32.dll"));
        if (!mod)
            mod = LoadLibrary(TEXT("opengl32.dll"));
        if (mod)
            return GetProcAddress(mod, name);
    }

    return NULL;
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
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    pf = ChoosePixelFormat(hdc, &pfd);
    if (pf == 0)
        return FALSE;

    if (!SetPixelFormat(hdc, pf, &pfd))
        return TRUE;

    return TRUE;
}

static void
LoadGl42Procs(void)
{
    pwglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetAnyGlProcAddress("wglCreateContextAttribsARB");

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

    pglGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GetAnyGlProcAddress("glGenVertexArrays");
    pglBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GetAnyGlProcAddress("glBindVertexArray");
    pglGenBuffers = (PFNGLGENBUFFERSPROC)GetAnyGlProcAddress("glGenBuffers");
    pglBindBuffer = (PFNGLBINDBUFFERPROC)GetAnyGlProcAddress("glBindBuffer");
    pglBufferData = (PFNGLBUFFERDATAPROC)GetAnyGlProcAddress("glBufferData");
    pglGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)GetAnyGlProcAddress("glGetAttribLocation");
    pglVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)GetAnyGlProcAddress("glVertexAttribPointer");
    pglEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)GetAnyGlProcAddress("glEnableVertexAttribArray");
    pglGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GetAnyGlProcAddress("glGetUniformLocation");
    pglUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)GetAnyGlProcAddress("glUniformMatrix4fv");
}

static void
DumpInfoLog(GLuint obj, BOOL isProgram)
{
    GLchar buf[2048];
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
        ResultsPrint(TEXT("GL42 log: %hs"), buf);
}

static GLuint
BuildProgram(void)
{
    const GLchar* vsSrc =
        "#version 420 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "layout(location=1) in vec3 aColor;\n"
        "uniform mat4 uMVP;\n"
        "out vec3 vColor;\n"
        "void main(){ vColor=aColor; gl_Position=uMVP*vec4(aPos,1.0); }\n";

    const GLchar* fsSrc =
        "#version 420 core\n"
        "in vec3 vColor;\n"
        "out vec4 fragColor;\n"
        "void main(){ fragColor=vec4(vColor,1.0); }\n";

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
        DumpInfoLog(vs, FALSE);
        pglDeleteShader(vs);
        pglDeleteShader(fs);
        return 0;
    }

    pglShaderSource(fs, 1, &fsSrc, NULL);
    pglCompileShader(fs);
    pglGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        DumpInfoLog(fs, FALSE);
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
        DumpInfoLog(prog, TRUE);
        pglDeleteShader(vs);
        pglDeleteShader(fs);
        pglDeleteProgram(prog);
        return 0;
    }

    pglDeleteShader(vs);
    pglDeleteShader(fs);

    return prog;
}

static double
TicksToMs(LONGLONG ticks, LONGLONG freq)
{
    if (freq <= 0)
        return 0.0;
    return ((double)ticks * 1000.0) / (double)freq;
}

static void
MatMul(float out[16], const float a[16], const float b[16])
{
    /* Column-major 4x4 multiply: out = a * b */
    int row, col;
    for (col = 0; col < 4; col++)
    {
        for (row = 0; row < 4; row++)
        {
            out[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
}

static void
MatIdentity(float m[16])
{
    ZeroMemory(m, sizeof(float) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void
MatPerspective(float m[16], float fov, float aspect, float zn, float zf)
{
    /* OpenGL perspective, column-major, right-handed, depth [-1..1] */
    float f = 1.0f / (float)tan(fov * 0.5f);
    MatIdentity(m);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zf + zn) / (zn - zf);
    m[11] = -1.0f;
    m[14] = (2.0f * zf * zn) / (zn - zf);
    m[15] = 0.0f;
}

static void
MatTranslate(float m[16], float x, float y, float z)
{
    MatIdentity(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

static void
MatRotateY(float m[16], float a)
{
    MatIdentity(m);
    m[0] = (float)cos(a);
    m[2] = -(float)sin(a);
    m[8] = (float)sin(a);
    m[10] = (float)cos(a);
}

static void
MatRotateX(float m[16], float a)
{
    MatIdentity(m);
    m[5] = (float)cos(a);
    m[6] = (float)sin(a);
    m[9] = -(float)sin(a);
    m[10] = (float)cos(a);
}

void
W32Prof_Test_GL42Cube(const ProfilerConfig* cfg)
{
    HDC hdc;
    HGLRC rc = NULL;
    HGLRC tmpRc = NULL;
    GLuint prog = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint uMvp = -1;
    RECT r;
    int w, h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;
    HWND hRender;

    /* Interleaved positions + colors (triangles for a cube) */
    static const float cubeVerts[] =
    {
        /* pos */            /* color */
        -1, -1,  1,          1, 0, 0,
        -1,  1,  1,          1, 0, 0,
         1,  1,  1,          1, 0, 0,
        -1, -1,  1,          1, 0, 0,
         1,  1,  1,          1, 0, 0,
         1, -1,  1,          1, 0, 0,

         1, -1, -1,          0, 1, 0,
         1,  1, -1,          0, 1, 0,
        -1,  1, -1,          0, 1, 0,
         1, -1, -1,          0, 1, 0,
        -1,  1, -1,          0, 1, 0,
        -1, -1, -1,          0, 1, 0,

         1, -1,  1,          0, 0, 1,
         1,  1,  1,          0, 0, 1,
         1,  1, -1,          0, 0, 1,
         1, -1,  1,          0, 0, 1,
         1,  1, -1,          0, 0, 1,
         1, -1, -1,          0, 0, 1,

        -1, -1, -1,          1, 1, 0,
        -1,  1, -1,          1, 1, 0,
        -1,  1,  1,          1, 1, 0,
        -1, -1, -1,          1, 1, 0,
        -1,  1,  1,          1, 1, 0,
        -1, -1,  1,          1, 1, 0,

        -1,  1,  1,          1, 0, 1,
        -1,  1, -1,          1, 0, 1,
         1,  1, -1,          1, 0, 1,
        -1,  1,  1,          1, 0, 1,
         1,  1, -1,          1, 0, 1,
         1,  1,  1,          1, 0, 1,

        -1, -1, -1,          0, 1, 1,
        -1, -1,  1,          0, 1, 1,
         1, -1,  1,          0, 1, 1,
        -1, -1, -1,          0, 1, 1,
         1, -1,  1,          0, 1, 1,
         1, -1, -1,          0, 1, 1,
    };

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
        ResultsPrint(TEXT("OpenGL 4.2: failed to create render child window"));
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
        ResultsPrint(TEXT("OpenGL 4.2: Choose/SetPixelFormat failed"));
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    /* Create a temporary context to load wglCreateContextAttribsARB. */
    tmpRc = wglCreateContext(hdc);
    if (!tmpRc)
    {
        ResultsPrint(TEXT("OpenGL 4.2: wglCreateContext failed: %lu"), GetLastError());
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    if (!wglMakeCurrent(hdc, tmpRc))
    {
        ResultsPrint(TEXT("OpenGL 4.2: wglMakeCurrent(tmp) failed: %lu"), GetLastError());
        wglDeleteContext(tmpRc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    LoadGl42Procs();

    if (pwglCreateContextAttribsARB)
    {
        int attribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 2,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

        rc = pwglCreateContextAttribsARB(hdc, NULL, attribs);
    }

    if (!rc)
    {
        /* Fallback: keep legacy context. Driver may still expose 4.x compat. */
        rc = tmpRc;
        tmpRc = NULL;
    }
    else
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tmpRc);
        tmpRc = NULL;
        wglMakeCurrent(hdc, rc);

        /* Reload procs under the real context. */
        LoadGl42Procs();
    }

    if (!pglGenVertexArrays || !pglBindVertexArray || !pglGenBuffers || !pglBindBuffer || !pglBufferData ||
        !pglGetAttribLocation || !pglVertexAttribPointer || !pglEnableVertexAttribArray ||
        !pglGetUniformLocation || !pglUniformMatrix4fv)
    {
        ResultsPrint(TEXT("OpenGL 4.2: required entry points missing"));
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    prog = BuildProgram();
    if (!prog)
    {
        ResultsPrint(TEXT("OpenGL 4.2: shader program not available"));
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    pglUseProgram(prog);

    pglGenVertexArrays(1, &vao);
    pglBindVertexArray(vao);

    pglGenBuffers(1, &vbo);
    pglBindBuffer(GL_ARRAY_BUFFER, vbo);
    pglBufferData(GL_ARRAY_BUFFER, (ptrdiff_t)sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

    {
        GLint locPos = pglGetAttribLocation(prog, "aPos");
        GLint locCol = pglGetAttribLocation(prog, "aColor");
        if (locPos < 0) locPos = 0;
        if (locCol < 0) locCol = 1;

        pglVertexAttribPointer((GLuint)locPos, 3, GL_FLOAT, FALSE, 6 * (GLsizei)sizeof(float), (const void*)0);
        pglEnableVertexAttribArray((GLuint)locPos);

        pglVertexAttribPointer((GLuint)locCol, 3, GL_FLOAT, FALSE, 6 * (GLsizei)sizeof(float), (const void*)(3 * sizeof(float)));
        pglEnableVertexAttribArray((GLuint)locCol);
    }

    uMvp = pglGetUniformLocation(prog, "uMVP");

    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    while (1)
    {
        float a;
        float mx[16], my[16], mw[16], mv[16], mp[16], mvw[16], mvp[16];

        if (cfg && cfg->StopEvent && WaitForSingleObject(cfg->StopEvent, 0) == WAIT_OBJECT_0)
            break;
        if (frames != 0 && i >= frames)
            break;

        a = (float)i * 0.01f;
        MatRotateX(mx, a * 0.7f);
        MatRotateY(my, a);
        MatMul(mw, mx, my);
        MatTranslate(mv, 0.0f, 0.0f, -6.0f);
        MatPerspective(mp, 1.0f, (h != 0) ? ((float)w / (float)h) : 1.0f, 0.1f, 100.0f);
        MatMul(mvw, mv, mw);
        MatMul(mvp, mp, mvw);

        if (uMvp >= 0)
            pglUniformMatrix4fv(uMvp, 1, FALSE, (const GLfloat*)mvp);

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 36);
        SwapBuffers(hdc);

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("OpenGL 4.2 Cube"));
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("OpenGL 4.2 Cube: %lu frames in %.3f ms (%.2f fps)"),
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
