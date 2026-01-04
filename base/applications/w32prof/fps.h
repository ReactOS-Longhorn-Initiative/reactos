#pragma once

#include "profiler.h"

#include <windows.h>
#include <tchar.h>

typedef struct _W32PROF_FPS_STATE
{
    LONGLONG LastQpc;
    DWORD LastFrames;
} W32PROF_FPS_STATE;

static __inline void
W32Prof_FpsInit(W32PROF_FPS_STATE* s)
{
    LARGE_INTEGER li;

    if (!s)
        return;

    QueryPerformanceCounter(&li);
    s->LastQpc = li.QuadPart;
    s->LastFrames = 0;
}

static __inline void
W32Prof_FpsMaybeReport(const ProfilerConfig* cfg,
                       W32PROF_FPS_STATE* s,
                       DWORD framesNow,
                       LONGLONG qpcFreq,
                       const TCHAR* tag)
{
    LARGE_INTEGER li;
    LONGLONG dt;
    DWORD df;
    double fps;

    if (!cfg || !cfg->Continuous)
        return;
    if (!cfg->StopEvent)
        return;
    if (!s)
        return;
    if (qpcFreq <= 0)
        return;

    QueryPerformanceCounter(&li);
    dt = li.QuadPart - s->LastQpc;
    if (dt < qpcFreq)
        return;

    df = framesNow - s->LastFrames;
    if (df == 0)
    {
        s->LastQpc = li.QuadPart;
        s->LastFrames = framesNow;
        return;
    }

    fps = ((double)df * (double)qpcFreq) / (double)dt;
    ResultsPrint(TEXT("%s: %.2f fps"), tag ? tag : TEXT("FPS"), fps);

    s->LastQpc = li.QuadPart;
    s->LastFrames = framesNow;
}
