#include "profiler.h"
#include "tga.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

#include <math.h>
#include <stddef.h>

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
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

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
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint location, GLint v0);

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
static PFNGLUNIFORM1IPROC pglUniform1i;

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
        return FALSE;

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
    pglUniform1i = (PFNGLUNIFORM1IPROC)GetAnyGlProcAddress("glUniform1i");
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
        "layout(location=1) in vec2 aTex;\n"
        "uniform mat4 uMVP;\n"
        "out vec2 vTex;\n"
        "void main(){ vTex=aTex; gl_Position=uMVP*vec4(aPos,1.0); }\n";

    const GLchar* fsSrc =
        "#version 420 core\n"
        "in vec2 vTex;\n"
        "uniform sampler2D uTex;\n"
        "out vec4 fragColor;\n"
        "void main(){ fragColor = texture(uTex, vTex); }\n";

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

static void
MatIdentity(float* m)
{
    int i;
    for (i = 0; i < 16; i++)
        m[i] = 0.0f;
    m[0] = 1.0f;
    m[5] = 1.0f;
    m[10] = 1.0f;
    m[15] = 1.0f;
}

static void
MatMul(float* out, const float* a, const float* b)
{
    float r[16];
    int c;
    int rI;

    for (c = 0; c < 4; c++)
    {
        for (rI = 0; rI < 4; rI++)
        {
            r[c * 4 + rI] =
                a[0 * 4 + rI] * b[c * 4 + 0] +
                a[1 * 4 + rI] * b[c * 4 + 1] +
                a[2 * 4 + rI] * b[c * 4 + 2] +
                a[3 * 4 + rI] * b[c * 4 + 3];
        }
    }

    for (c = 0; c < 16; c++)
        out[c] = r[c];
}

static void
MatRotateX(float* m, float a)
{
    MatIdentity(m);
    m[5] = (float)cos(a);
    m[6] = (float)sin(a);
    m[9] = (float)-sin(a);
    m[10] = (float)cos(a);
}

static void
MatRotateY(float* m, float a)
{
    MatIdentity(m);
    m[0] = (float)cos(a);
    m[8] = (float)sin(a);
    m[2] = (float)-sin(a);
    m[10] = (float)cos(a);
}

static void
MatTranslate(float* m, float x, float y, float z)
{
    MatIdentity(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

static void
MatPerspective(float* m, float fovy, float aspect, float zn, float zf)
{
    float yScale;
    float xScale;
    int i;

    yScale = 1.0f / (float)tan(fovy * 0.5f);
    xScale = yScale / aspect;

    for (i = 0; i < 16; i++)
        m[i] = 0.0f;

    m[0] = xScale;
    m[5] = yScale;
    m[10] = -(zf + zn) / (zf - zn);
    m[11] = -1.0f;
    m[14] = -(2.0f * zf * zn) / (zf - zn);
}

static double
TicksToMs(LONGLONG ticks, LONGLONG freq)
{
    if (freq <= 0)
        return 0.0;
    return ((double)ticks * 1000.0) / (double)freq;
}

void
W32Prof_Test_GL42TexturedCubeFullscreen(const ProfilerConfig* cfg)
{
    HDC hdc;
    HGLRC rc;
    HGLRC tmpRc;
    int w;
    int h;
    DWORD frames;
    DWORD i;
    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;
    HWND hRender;

    GLuint prog;
    GLuint vao;
    GLuint vbo;
    GLint uMvp;
    GLint uTex;
    GLuint tex;

    W32PROF_IMAGE_RGBA img;
    MSG msg;
    BOOL quit;

    /* position (3) + uv (2) */
    static const float cubeVerts[36 * 5] =
    {
        /* +Z */
        -1, -1,  1, 0, 0,   -1,  1,  1, 0, 1,    1,  1,  1, 1, 1,
        -1, -1,  1, 0, 0,    1,  1,  1, 1, 1,    1, -1,  1, 1, 0,
        /* -Z */
         1, -1, -1, 0, 0,    1,  1, -1, 0, 1,   -1,  1, -1, 1, 1,
         1, -1, -1, 0, 0,   -1,  1, -1, 1, 1,   -1, -1, -1, 1, 0,
        /* +X */
         1, -1,  1, 0, 0,    1,  1,  1, 0, 1,    1,  1, -1, 1, 1,
         1, -1,  1, 0, 0,    1,  1, -1, 1, 1,    1, -1, -1, 1, 0,
        /* -X */
        -1, -1, -1, 0, 0,   -1,  1, -1, 0, 1,   -1,  1,  1, 1, 1,
        -1, -1, -1, 0, 0,   -1,  1,  1, 1, 1,   -1, -1,  1, 1, 0,
        /* +Y */
        -1,  1,  1, 0, 0,   -1,  1, -1, 0, 1,    1,  1, -1, 1, 1,
        -1,  1,  1, 0, 0,    1,  1, -1, 1, 1,    1,  1,  1, 1, 0,
        /* -Y */
        -1, -1, -1, 0, 0,   -1, -1,  1, 0, 1,    1, -1,  1, 1, 1,
        -1, -1, -1, 0, 0,    1, -1,  1, 1, 1,    1, -1, -1, 1, 0,
    };

    hdc = NULL;
    rc = NULL;
    tmpRc = NULL;
    hRender = NULL;
    prog = 0;
    vao = 0;
    vbo = 0;
    uMvp = -1;
    uTex = -1;
    tex = 0;
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
        ResultsPrint(TEXT("OpenGL 4.2 Textured FS: failed to create fullscreen window"));
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
        ResultsPrint(TEXT("OpenGL 4.2 Textured FS: Choose/SetPixelFormat failed"));
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    tmpRc = wglCreateContext(hdc);
    if (!tmpRc)
    {
        ResultsPrint(TEXT("OpenGL 4.2 Textured FS: wglCreateContext failed: %lu"), GetLastError());
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    if (!wglMakeCurrent(hdc, tmpRc))
    {
        ResultsPrint(TEXT("OpenGL 4.2 Textured FS: wglMakeCurrent failed: %lu"), GetLastError());
        wglDeleteContext(tmpRc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    LoadGl42Procs();

    rc = NULL;
    if (pwglCreateContextAttribsARB)
    {
        int attribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 2,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

        rc = pwglCreateContextAttribsARB(hdc, 0, attribs);
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

        LoadGl42Procs();
    }

    if (!pglGenVertexArrays || !pglBindVertexArray || !pglGenBuffers || !pglBindBuffer || !pglBufferData ||
        !pglGetAttribLocation || !pglVertexAttribPointer || !pglEnableVertexAttribArray ||
        !pglGetUniformLocation || !pglUniformMatrix4fv || !pglUniform1i)
    {
        ResultsPrint(TEXT("OpenGL 4.2 Textured FS: required entry points missing"));
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    if (!W32Prof_LoadLogoTestTgaFromResource(GetModuleHandle(NULL), &img))
    {
        ResultsPrint(TEXT("OpenGL 4.2 Textured FS: failed to load embedded TGA"));
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    prog = BuildProgram();
    if (!prog)
    {
        ResultsPrint(TEXT("OpenGL 4.2 Textured FS: shader program not available"));
        W32Prof_ImageFree(&img);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(rc);
        ReleaseDC(hRender, hdc);
        DestroyWindow(hRender);
        return;
    }

    pglUseProgram(prog);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)img.Width, (GLsizei)img.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.Pixels);

    uTex = pglGetUniformLocation(prog, "uTex");
    if (uTex >= 0)
        pglUniform1i(uTex, 0);

    pglGenVertexArrays(1, &vao);
    pglBindVertexArray(vao);

    pglGenBuffers(1, &vbo);
    pglBindBuffer(GL_ARRAY_BUFFER, vbo);
    pglBufferData(GL_ARRAY_BUFFER, (ptrdiff_t)sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

    {
        GLint locPos = pglGetAttribLocation(prog, "aPos");
        GLint locTex = pglGetAttribLocation(prog, "aTex");
        if (locPos < 0) locPos = 0;
        if (locTex < 0) locTex = 1;

        pglVertexAttribPointer((GLuint)locPos, 3, GL_FLOAT, FALSE, 5 * (GLsizei)sizeof(float), (const void*)0);
        pglEnableVertexAttribArray((GLuint)locPos);

        pglVertexAttribPointer((GLuint)locTex, 2, GL_FLOAT, FALSE, 5 * (GLsizei)sizeof(float), (const void*)(3 * sizeof(float)));
        pglEnableVertexAttribArray((GLuint)locTex);
    }

    uMvp = pglGetUniformLocation(prog, "uMVP");

    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);

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
            float a;
            float mx[16], my[16], mw[16], mv[16], mp[16], mvw[16], mvp[16];

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

            glBindTexture(GL_TEXTURE_2D, tex);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            SwapBuffers(hdc);

            i++;
            W32Prof_FpsMaybeReport(cfg, &fps, i, qf.QuadPart, TEXT("OpenGL 4.2 Textured Cube (Fullscreen)"));
        }
    }

    QueryPerformanceCounter(&q1);

    ResultsPrint(TEXT("OpenGL 4.2 Textured Cube (Fullscreen): %lu frames in %.3f ms (%.2f fps)"),
                 (ULONG)i,
                 TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart),
                 (TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart) > 0.0)
                    ? ((double)i * 1000.0 / TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart))
                    : 0.0);

    if (tex) glDeleteTextures(1, &tex);
    W32Prof_ImageFree(&img);

    pglUseProgram(0);
    pglDeleteProgram(prog);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);

    ReleaseDC(hRender, hdc);
    DestroyWindow(hRender);
}
