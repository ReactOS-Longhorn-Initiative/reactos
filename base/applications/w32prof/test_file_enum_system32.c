#include "profiler.h"
#include "fps.h"

#include <windows.h>
#include <tchar.h>

static double
TicksToMs(LONGLONG ticks, LONGLONG freq)
{
    if (freq <= 0)
        return 0.0;
    return ((double)ticks * 1000.0) / (double)freq;
}

static BOOL
ShouldStop(const ProfilerConfig* cfg)
{
    if (!cfg || !cfg->StopEvent)
        return FALSE;
    return (WaitForSingleObject(cfg->StopEvent, 0) == WAIT_OBJECT_0);
}

static DWORD
EnumOnce(const TCHAR* pattern, const ProfilerConfig* cfg)
{
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    DWORD count = 0;

    hFind = FindFirstFile(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    do
    {
        count++;
        if ((count & 1023) == 0 && ShouldStop(cfg))
            break;
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);
    return count;
}

void
W32Prof_Test_FileEnumSystem32(const ProfilerConfig* cfg)
{
    TCHAR sysDir[MAX_PATH];
    TCHAR pattern[MAX_PATH * 2];

    DWORD passes = 0;
    DWORD count;

    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    if (!cfg)
        return;

    if (!GetSystemDirectory(sysDir, (UINT)(sizeof(sysDir) / sizeof(sysDir[0]))))
    {
        ResultsPrint(TEXT("File Enum: GetSystemDirectory failed"));
        return;
    }

    _sntprintf(pattern, (sizeof(pattern) / sizeof(pattern[0])) - 1, TEXT("%s\\*.*"), sysDir);
    pattern[(sizeof(pattern) / sizeof(pattern[0])) - 1] = 0;

    QueryPerformanceFrequency(&qf);
    W32Prof_FpsInit(&fps);

    if (!cfg->Continuous)
    {
        QueryPerformanceCounter(&q0);
        count = EnumOnce(pattern, cfg);
        QueryPerformanceCounter(&q1);

        {
            double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
            double eps = (ms > 0.0) ? ((double)count * 1000.0 / ms) : 0.0;
            ResultsPrint(TEXT("File Enum (System32): %lu entries in %.3f ms (%.2f entries/s)"),
                         (ULONG)count, ms, eps);
        }
        return;
    }

    while (!ShouldStop(cfg))
    {
        passes++;
        count = EnumOnce(pattern, cfg);
        (void)count;
        W32Prof_FpsMaybeReport(cfg, &fps, passes, (LONGLONG)qf.QuadPart, TEXT("File Enum (System32)"));
    }
}
