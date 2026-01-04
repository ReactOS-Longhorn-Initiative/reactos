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

void
W32Prof_Test_RegistryQuery(const ProfilerConfig* cfg)
{
    HKEY hKey;
    DWORD iters;
    DWORD i;

    LARGE_INTEGER q0, q1, qf;
    W32PROF_FPS_STATE fps;

    if (!cfg)
        return;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        ResultsPrint(TEXT("Registry Query: RegOpenKeyEx failed"));
        return;
    }

    iters = (cfg->MessageIterations != 0) ? cfg->MessageIterations : 50000;
    if (iters > 200000)
        iters = 200000;

    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    W32Prof_FpsInit(&fps);

    i = 0;
    while (1)
    {
        TCHAR buf[260];
        DWORD cb;
        DWORD type;

        if (!cfg->Continuous && i >= iters)
            break;
        if ((i & 1023) == 0 && ShouldStop(cfg))
            break;

        cb = sizeof(buf);
        type = 0;
        RegQueryValueEx(hKey, TEXT("Wallpaper"), NULL, &type, (LPBYTE)buf, &cb);

        cb = sizeof(buf);
        type = 0;
        RegQueryValueEx(hKey, TEXT("DragFullWindows"), NULL, &type, (LPBYTE)buf, &cb);

        i++;
        W32Prof_FpsMaybeReport(cfg, &fps, i, (LONGLONG)qf.QuadPart, TEXT("Registry Query"));
    }

    QueryPerformanceCounter(&q1);

    {
        double ms = TicksToMs((LONGLONG)(q1.QuadPart - q0.QuadPart), (LONGLONG)qf.QuadPart);
        double ops = (ms > 0.0) ? ((double)i * 1000.0 / ms) : 0.0;
        ResultsPrint(TEXT("Registry Query: %lu ops in %.3f ms (%.2f ops/s)"), (ULONG)i, ms, ops);
    }

    RegCloseKey(hKey);
}
