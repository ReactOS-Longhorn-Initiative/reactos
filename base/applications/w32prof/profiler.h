#pragma once

#include <windows.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W32PROF_WM_TESTMSG (WM_APP + 0x413)

typedef enum _W32PROF_TEST_ID
{
    W32PROF_TEST_ALL = 0,
    W32PROF_TEST_BITBLT,
    W32PROF_TEST_GDI_HANDLES,
    W32PROF_TEST_USER_MESSAGES,
    W32PROF_TEST_WINDOWPOS,
    W32PROF_TEST_GETDC,
    W32PROF_TEST_COMPATDC,
    W32PROF_TEST_COMPATBMP,
    W32PROF_TEST_SELECTOBJECT,
    W32PROF_TEST_TEXTOUT,
    W32PROF_TEST_INVALIDATE_UPDATE,
    W32PROF_TEST_WIN32_TGA_BLIT,
    W32PROF_TEST_DDRAW_TGA_BLIT,
    W32PROF_TEST_D3D7_CUBE,
    W32PROF_TEST_D3D8_CUBE,
    W32PROF_TEST_D3D9_CUBE,
    W32PROF_TEST_GL11_CUBE,
    W32PROF_TEST_GL20_CUBE,
    W32PROF_TEST_GL42_CUBE,

    W32PROF_TEST_D3D7_TEX_CUBE,
    W32PROF_TEST_D3D7_TEX_CUBE_FS,
    W32PROF_TEST_D3D9_TEX_CUBE,
    W32PROF_TEST_D3D9_TEX_CUBE_FS,
    W32PROF_TEST_GL11_TEX_CUBE,
    W32PROF_TEST_GL11_TEX_CUBE_FS,
    W32PROF_TEST_GL20_TEX_CUBE,
    W32PROF_TEST_GL42_TEX_CUBE,
    W32PROF_TEST_GL42_TEX_CUBE_FS,
} W32PROF_TEST_ID;

typedef struct _W32PROF_TEST_ENTRY
{
    W32PROF_TEST_ID Id;
    const TCHAR* Name;
    const TCHAR* What;
} W32PROF_TEST_ENTRY;

typedef struct _W32PROF_TESTWND_STATE
{
    volatile LONG PostReceived;
    volatile LONG PostTarget;
    HANDLE PostDoneEvent;
} W32PROF_TESTWND_STATE;

typedef struct _ProfilerConfig
{
    DWORD BitBltIterations;
    DWORD GdiObjectIterations;
    DWORD MessageIterations;
    DWORD WindowPosIterations;
    DWORD GetDcIterations;
    DWORD CompatDcIterations;
    DWORD CompatBmpIterations;
    DWORD SelectObjectIterations;
    DWORD TextOutIterations;
    DWORD InvalidateIterations;

    DWORD GpuFrames;

    INT TestWidth;
    INT TestHeight;

    BOOL Headless;

    W32PROF_TEST_ID TestId;

    BOOL Continuous;
    HANDLE StopEvent;

    HWND hTestWnd;
    W32PROF_TESTWND_STATE* TestState;
} ProfilerConfig;

void ResultsInit(HWND hEditOptional);
void ResultsShutdown(void);
void ResultsPrint(const TCHAR* format, ...);

const W32PROF_TEST_ENTRY* W32ProfGetTestList(UINT* count);
void ProfilerRunTest(const ProfilerConfig* cfg, W32PROF_TEST_ID id);
void ProfilerRunContinuous(const ProfilerConfig* cfg, W32PROF_TEST_ID id);
void ProfilerRunAll(const ProfilerConfig* cfg);

void W32Prof_Test_D3D9Cube(const ProfilerConfig* cfg);
void W32Prof_Test_GL11Cube(const ProfilerConfig* cfg);
void W32Prof_Test_GL20Cube(const ProfilerConfig* cfg);
void W32Prof_Test_D3D7Cube(const ProfilerConfig* cfg);
void W32Prof_Test_D3D8Cube(const ProfilerConfig* cfg);
void W32Prof_Test_GL42Cube(const ProfilerConfig* cfg);

void W32Prof_Test_D3D7TexturedCube(const ProfilerConfig* cfg);
void W32Prof_Test_D3D7TexturedCubeFullscreen(const ProfilerConfig* cfg);
void W32Prof_Test_D3D9TexturedCube(const ProfilerConfig* cfg);
void W32Prof_Test_D3D9TexturedCubeFullscreen(const ProfilerConfig* cfg);
void W32Prof_Test_GL11TexturedCube(const ProfilerConfig* cfg);
void W32Prof_Test_GL11TexturedCubeFullscreen(const ProfilerConfig* cfg);
void W32Prof_Test_GL20TexturedCube(const ProfilerConfig* cfg);
void W32Prof_Test_GL42TexturedCube(const ProfilerConfig* cfg);
void W32Prof_Test_GL42TexturedCubeFullscreen(const ProfilerConfig* cfg);

void W32Prof_Test_Win32TgaBlit(const ProfilerConfig* cfg);
void W32Prof_Test_DDrawTgaBlit(const ProfilerConfig* cfg);

#ifdef __cplusplus
}
#endif
