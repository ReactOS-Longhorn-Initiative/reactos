#include "profiler.h"

#include <stdio.h>
#include <stdarg.h>

static HANDLE g_hLog = NULL;
static HWND g_hEdit = NULL;

static const W32PROF_TEST_ENTRY g_Tests[] =
{
    { W32PROF_TEST_ALL,           TEXT("All Tests"),        TEXT("Runs the full W32Prof suite") },
    { W32PROF_TEST_BITBLT,        TEXT("BitBlt Throughput"),TEXT("BitBlt memory DC -> test window DC") },
    { W32PROF_TEST_GDI_HANDLES,   TEXT("GDI Handle Lock"),  TEXT("Create/Delete pens+brushes in a tight loop") },
//    { W32PROF_TEST_USER_MESSAGES, TEXT("User Messages"),    TEXT("SendMessage vs PostMessage latency") },
    { W32PROF_TEST_WINDOWPOS,     TEXT("Window Manager"),   TEXT("SetWindowPos back/forth to stress win32k") },
    { W32PROF_TEST_GETDC,         TEXT("GetDC/ReleaseDC"),  TEXT("Repeated DC acquisition/release") },
    { W32PROF_TEST_COMPATDC,      TEXT("CompatDC Create"),  TEXT("CreateCompatibleDC/DeleteDC loop") },
    { W32PROF_TEST_COMPATBMP,     TEXT("CompatBitmap Create"),TEXT("CreateCompatibleBitmap/DeleteObject loop") },
    { W32PROF_TEST_SELECTOBJECT,  TEXT("SelectObject Pressure"),TEXT("Rapid SelectObject toggling (pens/brushes)") },
    { W32PROF_TEST_TEXTOUT,       TEXT("TextOut Throughput"),TEXT("TextOut to window DC (GDI text path)") },
    { W32PROF_TEST_INVALIDATE_UPDATE, TEXT("Invalidate/Update"),TEXT("InvalidateRect + UpdateWindow paint stress") },
    { W32PROF_TEST_LISTVIEW_POPULATE, TEXT("ListView Populate"), TEXT("Populate a report-mode ListView (Explorer-like item insertion)") },
    { W32PROF_TEST_TREEVIEW_POPULATE, TEXT("TreeView Populate"), TEXT("Populate/expand a TreeView (Explorer-like navigation tree)") },
    { W32PROF_TEST_IMAGELIST_DRAW, TEXT("ImageList Draw"), TEXT("ImageList_Draw grid (Explorer-like icon view)") },
    { W32PROF_TEST_TEXT_MEASURE,  TEXT("Text Measure"),     TEXT("DrawText(DT_CALCRECT|ELLIPSIS) sizing path") },
  //  { W32PROF_TEST_DEFERWINDOWPOS, TEXT("DeferWindowPos"),  TEXT("Begin/Defer/EndDeferWindowPos batch moves") },
    { W32PROF_TEST_WINDOW_CREATE_DESTROY, TEXT("Create/Destroy Windows"), TEXT("CreateWindowEx/DestroyWindow churn (Explorer-like UI construction)") },
    { W32PROF_TEST_DRAWICON_GRID, TEXT("DrawIcon Grid"), TEXT("DrawIconEx grid (Explorer-like icon painting)") },
    { W32PROF_TEST_REGISTRY_QUERY, TEXT("Registry Query"), TEXT("RegQueryValueEx loop (Explorer-like settings reads)") },
    { W32PROF_TEST_FILE_ENUM_SYSTEM32, TEXT("File Enum (System32)"), TEXT("FindFirstFile/FindNextFile enumeration of System32") },
    { W32PROF_TEST_GDI_PATH_STROKE, TEXT("GDI Path Stroke"), TEXT("BeginPath/Polyline/StrokePath loop") },
    { W32PROF_TEST_WIN32_TGA_BLIT, TEXT("Win32 TGA Blit"),   TEXT("GDI StretchBlt of embedded TGA (DIBSection -> window)") },
    { W32PROF_TEST_DDRAW_TGA_BLIT, TEXT("DirectDraw TGA Blit"), TEXT("DirectDraw7 Blt of embedded TGA (systemmem surface -> primary)") },
    { W32PROF_TEST_D3D7_CUBE,     TEXT("Direct3D7 Cube"),     TEXT("Create D3D7 device and render a spinning cube") },
    { W32PROF_TEST_D3D8_CUBE,     TEXT("Direct3D8 Cube"),     TEXT("Create D3D8 device and render a spinning cube") },
    { W32PROF_TEST_D3D9_CUBE,     TEXT("Direct3D9 Cube"),    TEXT("Create D3D9 device and render a spinning cube") },
    { W32PROF_TEST_GL11_CUBE,     TEXT("OpenGL 1.1 Cube"),   TEXT("Create WGL context (fixed pipeline) and render a spinning cube") },
    { W32PROF_TEST_GL20_CUBE,     TEXT("OpenGL 2.0 Cube"),   TEXT("Create WGL context + GLSL program and render a spinning cube") },
    { W32PROF_TEST_GL42_CUBE,     TEXT("OpenGL 4.2 Cube"),   TEXT("Create a 4.2 context (best-effort) and render a spinning cube") },

    { W32PROF_TEST_D3D7_TEX_CUBE,    TEXT("Direct3D7 Textured Cube"), TEXT("D3D7 device textured cube (embedded TGA)") },
    { W32PROF_TEST_D3D7_TEX_CUBE_FS, TEXT("Direct3D7 Textured Cube (Fullscreen)"), TEXT("Exclusive fullscreen D3D7 textured cube (embedded TGA)") },
    { W32PROF_TEST_D3D9_TEX_CUBE,    TEXT("Direct3D9 Textured Cube"), TEXT("D3D9 device textured cube (embedded TGA)") },
    { W32PROF_TEST_D3D9_TEX_CUBE_FS, TEXT("Direct3D9 Textured Cube (Fullscreen)"), TEXT("Fullscreen D3D9 textured cube (embedded TGA)") },
    { W32PROF_TEST_GL11_TEX_CUBE,    TEXT("OpenGL 1.1 Textured Cube"), TEXT("Fixed pipeline textured cube (embedded TGA)") },
    { W32PROF_TEST_GL11_TEX_CUBE_FS, TEXT("OpenGL 1.1 Textured Cube (Fullscreen)"), TEXT("Fullscreen fixed pipeline textured cube (embedded TGA)") },
    { W32PROF_TEST_GL20_TEX_CUBE,    TEXT("OpenGL 2.0 Textured Cube"), TEXT("GLSL textured cube (embedded TGA)") },
    { W32PROF_TEST_GL42_TEX_CUBE,    TEXT("OpenGL 4.2 Textured Cube"), TEXT("GL 4.2 textured cube (embedded TGA)") },
    { W32PROF_TEST_GL42_TEX_CUBE_FS, TEXT("OpenGL 4.2 Textured Cube (Fullscreen)"), TEXT("Fullscreen GL 4.2 textured cube (embedded TGA)") },
};

const W32PROF_TEST_ENTRY*
W32ProfGetTestList(UINT* count)
{
    if (count)
        *count = (UINT)(sizeof(g_Tests) / sizeof(g_Tests[0]));
    return g_Tests;
}

static LONGLONG
QpcNow(void)
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

static LONGLONG
QpcFreq(void)
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

static double
QpcToMs(LONGLONG ticks, LONGLONG freq)
{
    if (freq <= 0)
        return 0.0;
    return ((double)ticks * 1000.0) / (double)freq;
}

static __inline BOOL
ShouldStop(const ProfilerConfig* cfg)
{
    if (!cfg || !cfg->StopEvent)
        return FALSE;
    return (WaitForSingleObject(cfg->StopEvent, 0) == WAIT_OBJECT_0);
}

static void
ResultsEnsureLogOpen(void)
{
    if (g_hLog)
        return;

    g_hLog = CreateFile(TEXT("w32prof.log"),
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
}

void
ResultsInit(HWND hEditOptional)
{
    g_hEdit = hEditOptional;
    ResultsEnsureLogOpen();
}

void
ResultsShutdown(void)
{
    if (g_hLog && g_hLog != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hLog);
    }
    g_hLog = NULL;
    g_hEdit = NULL;
}

void
ResultsPrint(const TCHAR* format, ...)
{
    TCHAR buf[1024];
    TCHAR line[1100];
    DWORD written;

    va_list args;
    va_start(args, format);

#if defined(_MSC_VER)
    _vsntprintf(buf, (sizeof(buf) / sizeof(buf[0])) - 1, format, args);
    buf[(sizeof(buf) / sizeof(buf[0])) - 1] = 0;
#else
    _vsntprintf(buf, (sizeof(buf) / sizeof(buf[0])) - 1, format, args);
    buf[(sizeof(buf) / sizeof(buf[0])) - 1] = 0;
#endif

    va_end(args);

    _sntprintf(line, (sizeof(line) / sizeof(line[0])) - 1, TEXT("%s\r\n"), buf);
    line[(sizeof(line) / sizeof(line[0])) - 1] = 0;

    OutputDebugString(line);

    ResultsEnsureLogOpen();
    if (g_hLog && g_hLog != INVALID_HANDLE_VALUE)
    {
        WriteFile(g_hLog, line, (DWORD)(_tcslen(line) * sizeof(TCHAR)), &written, NULL);
    }

    if (g_hEdit)
    {
        SendMessage(g_hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessage(g_hEdit, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)line);
    }
}

static void
ApplyAffinityAndRealtime(const ProfilerConfig* cfg, DWORD* oldPriorityClass, DWORD_PTR* oldAffinity)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    ResultsPrint(TEXT("Detected processors: %lu"), (ULONG)si.dwNumberOfProcessors);

    if (oldPriorityClass)
        *oldPriorityClass = GetPriorityClass(GetCurrentProcess());

    if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
    {
        ResultsPrint(TEXT("WARNING: SetPriorityClass(REALTIME) failed: %lu"), GetLastError());
    }

    if (oldAffinity)
        *oldAffinity = 0;

    /* Legacy behavior pins to CPU0 for stability; multicore mode leaves affinity unchanged. */
    if (!cfg || cfg->PinSingleCore)
    {
        DWORD_PTR prev = SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1);
        if (oldAffinity)
            *oldAffinity = prev;
        if (oldAffinity && *oldAffinity == 0)
            ResultsPrint(TEXT("WARNING: SetThreadAffinityMask failed: %lu"), GetLastError());
    }
}

static void
RestoreAffinityAndPriority(DWORD oldPriorityClass, DWORD_PTR oldAffinity)
{
    if (oldAffinity)
        SetThreadAffinityMask(GetCurrentThread(), oldAffinity);

    if (oldPriorityClass)
        SetPriorityClass(GetCurrentProcess(), oldPriorityClass);
}

static void
Test_BitBltThroughput(const ProfilerConfig* cfg, LONGLONG freq)
{
    HDC hdcDst;
    HDC hdcSrc;
    HBITMAP hbmp;
    HGDIOBJ oldBmp;
    RECT rc;
    INT w, h;
    DWORD i;

    if (!cfg || !cfg->hTestWnd)
        return;

    GetClientRect(cfg->hTestWnd, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    if (w <= 0) w = cfg->TestWidth;
    if (h <= 0) h = cfg->TestHeight;

    hdcDst = GetDC(cfg->hTestWnd);
    if (!hdcDst)
    {
        ResultsPrint(TEXT("BitBlt: GetDC failed"));
        return;
    }

    hdcSrc = CreateCompatibleDC(hdcDst);
    hbmp = CreateCompatibleBitmap(hdcDst, w, h);
    if (!hdcSrc || !hbmp)
    {
        ResultsPrint(TEXT("BitBlt: CreateCompatible* failed"));
        if (hbmp) DeleteObject(hbmp);
        if (hdcSrc) DeleteDC(hdcSrc);
        ReleaseDC(cfg->hTestWnd, hdcDst);
        return;
    }

    oldBmp = SelectObject(hdcSrc, hbmp);

    {
        RECT fillRc;
        HBRUSH br1;
        HBRUSH br2;
        fillRc.left = 0;
        fillRc.top = 0;
        fillRc.right = w;
        fillRc.bottom = h;

        br1 = CreateSolidBrush(RGB(0, 128, 255));
        br2 = CreateSolidBrush(RGB(255, 128, 0));
        if (br1 && br2)
            FillRect(hdcSrc, &fillRc, br1);
        else
            PatBlt(hdcSrc, 0, 0, w, h, WHITENESS);

        {
            LONGLONG t0 = QpcNow();
            for (i = 0; i < cfg->BitBltIterations; i++)
            {
                if (((i & 1023) == 0) && ShouldStop(cfg))
                    break;
                if (br1 && br2 && ((i & 63) == 0))
                {
                    FillRect(hdcSrc, &fillRc, (i & 64) ? br2 : br1);
                }
                BitBlt(hdcDst, 0, 0, w, h, hdcSrc, 0, 0, SRCCOPY);
            }
        GdiFlush();
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("BitBlt Throughput: %lu ops in %.3f ms (%.2f ops/ms)"),
                         (ULONG)cfg->BitBltIterations,
                         ms,
                         (ms > 0.0) ? ((double)cfg->BitBltIterations / ms) : 0.0);
        }
        }

        if (br1) DeleteObject(br1);
        if (br2) DeleteObject(br2);
    }

    SelectObject(hdcSrc, oldBmp);
    DeleteObject(hbmp);
    DeleteDC(hdcSrc);
    ReleaseDC(cfg->hTestWnd, hdcDst);
}

static void
Test_GdiHandleLockContention(const ProfilerConfig* cfg, LONGLONG freq)
{
    DWORD i;
    DWORD failures = 0;

    if (!cfg)
        return;

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->GdiObjectIterations; i++)
        {
            if (((i & 2047) == 0) && ShouldStop(cfg))
                break;
            HBRUSH hbr = CreateSolidBrush(RGB(0, 0, 0));
            HPEN hpen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));

            if (hbr)
                DeleteObject(hbr);
            else
                failures++;

            if (hpen)
                DeleteObject(hpen);
            else
                failures++;
        }
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("GDI Handle Lock Contention: %lu iters in %.3f ms (failures=%lu)"),
                         (ULONG)cfg->GdiObjectIterations,
                         ms,
                         (ULONG)failures);
        }
    }
}

static void
Test_UserMessageLatency(const ProfilerConfig* cfg, LONGLONG freq)
{
    DWORD i;

    if (!cfg || !cfg->hTestWnd || !cfg->TestState)
        return;

    ResultsPrint(TEXT("User Message Latency: iterations=%lu"), (ULONG)cfg->MessageIterations);

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->MessageIterations; i++)
        {
            if (((i & 4095) == 0) && ShouldStop(cfg))
                break;
            SendMessage(cfg->hTestWnd, W32PROF_WM_TESTMSG, 0, 0);
        }
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("  SendMessage: %.3f ms total (%.3f us/msg)"),
                         ms,
                         (cfg->MessageIterations ? (ms * 1000.0 / (double)cfg->MessageIterations) : 0.0));
        }
    }

    if (!cfg->TestState->PostDoneEvent)
    {
        ResultsPrint(TEXT("  PostMessage: skipped (no PostDoneEvent)"));
        return;
    }

    cfg->TestState->PostReceived = 0;
    cfg->TestState->PostTarget = (LONG)cfg->MessageIterations;
    ResetEvent(cfg->TestState->PostDoneEvent);

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->MessageIterations; i++)
        {
            if (((i & 4095) == 0) && ShouldStop(cfg))
                break;
            PostMessage(cfg->hTestWnd, W32PROF_WM_TESTMSG, 0, 0);
        }

        if (!ShouldStop(cfg))
            WaitForSingleObject(cfg->TestState->PostDoneEvent, INFINITE);

        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("  PostMessage: %.3f ms total (%.3f us/msg)"),
                         ms,
                         (cfg->MessageIterations ? (ms * 1000.0 / (double)cfg->MessageIterations) : 0.0));
        }
    }
}

static void
Test_WindowManagerLock(const ProfilerConfig* cfg, LONGLONG freq)
{
    RECT rc;
    LONG x, y;
    DWORD i;

    if (!cfg || !cfg->hTestWnd)
        return;

    GetWindowRect(cfg->hTestWnd, &rc);
    x = rc.left;
    y = rc.top;

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->WindowPosIterations; i++)
        {
            if (((i & 2047) == 0) && ShouldStop(cfg))
                break;
            SetWindowPos(cfg->hTestWnd, NULL, x + 1, y, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(cfg->hTestWnd, NULL, x, y, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("Window Manager Lock (SetWindowPos): %lu iters in %.3f ms"),
                         (ULONG)cfg->WindowPosIterations,
                         ms);
        }
    }
}

static void
Test_GetDcRelease(const ProfilerConfig* cfg, LONGLONG freq)
{
    DWORD i;
    if (!cfg || !cfg->hTestWnd)
        return;

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->GetDcIterations; i++)
        {
            if (((i & 4095) == 0) && ShouldStop(cfg))
                break;
            HDC hdc = GetDC(cfg->hTestWnd);
            if (hdc)
                ReleaseDC(cfg->hTestWnd, hdc);
        }
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("GetDC/ReleaseDC: %lu iters in %.3f ms"),
                         (ULONG)cfg->GetDcIterations,
                         ms);
        }
    }
}

static void
Test_CompatDcCreateDelete(const ProfilerConfig* cfg, LONGLONG freq)
{
    DWORD i;
    HDC hdcWnd;

    if (!cfg || !cfg->hTestWnd)
        return;

    hdcWnd = GetDC(cfg->hTestWnd);
    if (!hdcWnd)
        return;

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->CompatDcIterations; i++)
        {
            if (((i & 2047) == 0) && ShouldStop(cfg))
                break;
            HDC hdc = CreateCompatibleDC(hdcWnd);
            if (hdc)
                DeleteDC(hdc);
        }
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("CreateCompatibleDC/DeleteDC: %lu iters in %.3f ms"),
                         (ULONG)cfg->CompatDcIterations,
                         ms);
        }
    }

    ReleaseDC(cfg->hTestWnd, hdcWnd);
}

static void
Test_CompatBitmapCreateDelete(const ProfilerConfig* cfg, LONGLONG freq)
{
    DWORD i;
    HDC hdcWnd;
    RECT rc;
    INT w, h;

    if (!cfg || !cfg->hTestWnd)
        return;

    GetClientRect(cfg->hTestWnd, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    if (w <= 0) w = cfg->TestWidth;
    if (h <= 0) h = cfg->TestHeight;

    hdcWnd = GetDC(cfg->hTestWnd);
    if (!hdcWnd)
        return;

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->CompatBmpIterations; i++)
        {
            if (((i & 1023) == 0) && ShouldStop(cfg))
                break;
            HBITMAP hbmp = CreateCompatibleBitmap(hdcWnd, w, h);
            if (hbmp)
                DeleteObject(hbmp);
        }
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("CreateCompatibleBitmap/DeleteObject: %lu iters in %.3f ms"),
                         (ULONG)cfg->CompatBmpIterations,
                         ms);
        }
    }

    ReleaseDC(cfg->hTestWnd, hdcWnd);
}

static void
Test_SelectObjectPressure(const ProfilerConfig* cfg, LONGLONG freq)
{
    DWORD i;
    HDC hdcWnd;
    HDC hdcMem;
    HPEN pen1, pen2;
    HBRUSH br1, br2;
    HBITMAP hbmp;
    HGDIOBJ oldBmp;

    if (!cfg || !cfg->hTestWnd)
        return;

    hdcWnd = GetDC(cfg->hTestWnd);
    if (!hdcWnd)
        return;

    hdcMem = CreateCompatibleDC(hdcWnd);
    hbmp = CreateCompatibleBitmap(hdcWnd, 64, 64);
    if (!hdcMem || !hbmp)
    {
        if (hbmp) DeleteObject(hbmp);
        if (hdcMem) DeleteDC(hdcMem);
        ReleaseDC(cfg->hTestWnd, hdcWnd);
        return;
    }

    oldBmp = SelectObject(hdcMem, hbmp);
    pen1 = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    pen2 = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
    br1 = CreateSolidBrush(RGB(0, 0, 255));
    br2 = CreateSolidBrush(RGB(255, 255, 0));

    if (!pen1 || !pen2 || !br1 || !br2)
    {
        if (pen1) DeleteObject(pen1);
        if (pen2) DeleteObject(pen2);
        if (br1) DeleteObject(br1);
        if (br2) DeleteObject(br2);
        SelectObject(hdcMem, oldBmp);
        DeleteObject(hbmp);
        DeleteDC(hdcMem);
        ReleaseDC(cfg->hTestWnd, hdcWnd);
        return;
    }

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->SelectObjectIterations; i++)
        {
            if (((i & 4095) == 0) && ShouldStop(cfg))
                break;
            SelectObject(hdcMem, (i & 1) ? pen1 : pen2);
            SelectObject(hdcMem, (i & 1) ? br1 : br2);
        }
        GdiFlush();
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("SelectObject Pressure: %lu iters in %.3f ms"),
                         (ULONG)cfg->SelectObjectIterations,
                         ms);
        }
    }

    DeleteObject(pen1);
    DeleteObject(pen2);
    DeleteObject(br1);
    DeleteObject(br2);
    SelectObject(hdcMem, oldBmp);
    DeleteObject(hbmp);
    DeleteDC(hdcMem);
    ReleaseDC(cfg->hTestWnd, hdcWnd);
}

static void
Test_TextOutThroughput(const ProfilerConfig* cfg, LONGLONG freq)
{
    DWORD i;
    HDC hdc;
    static const TCHAR text[] = TEXT("W32Prof: The quick brown fox jumps over the lazy dog 0123456789");

    if (!cfg || !cfg->hTestWnd)
        return;

    hdc = GetDC(cfg->hTestWnd);
    if (!hdc)
        return;

    SetBkMode(hdc, TRANSPARENT);

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->TextOutIterations; i++)
        {
            if (((i & 2047) == 0) && ShouldStop(cfg))
                break;
            int y = (int)((i % 64) * 4);
            TextOut(hdc, 4, y, text, (int)(_tcslen(text)));
        }
        GdiFlush();
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("TextOut Throughput: %lu calls in %.3f ms"),
                         (ULONG)cfg->TextOutIterations,
                         ms);
        }
    }

    ReleaseDC(cfg->hTestWnd, hdc);
}

static void
Test_InvalidateUpdate(const ProfilerConfig* cfg, LONGLONG freq)
{
    DWORD i;

    if (!cfg || !cfg->hTestWnd)
        return;

    {
        LONGLONG t0 = QpcNow();
        for (i = 0; i < cfg->InvalidateIterations; i++)
        {
            if (((i & 255) == 0) && ShouldStop(cfg))
                break;
            InvalidateRect(cfg->hTestWnd, NULL, FALSE);
            UpdateWindow(cfg->hTestWnd);
        }
        {
            LONGLONG dt = QpcNow() - t0;
            double ms = QpcToMs(dt, freq);
            ResultsPrint(TEXT("InvalidateRect/UpdateWindow: %lu iters in %.3f ms"),
                         (ULONG)cfg->InvalidateIterations,
                         ms);
        }
    }
}

static void
RunTestBody(const ProfilerConfig* cfg, W32PROF_TEST_ID id, LONGLONG freq)
{
    switch (id)
    {
        case W32PROF_TEST_ALL:
            Test_BitBltThroughput(cfg, freq);
            Test_GdiHandleLockContention(cfg, freq);
          //  Test_UserMessageLatency(cfg, freq);
            Test_WindowManagerLock(cfg, freq);
            Test_GetDcRelease(cfg, freq);
            Test_CompatDcCreateDelete(cfg, freq);
            Test_CompatBitmapCreateDelete(cfg, freq);
            Test_SelectObjectPressure(cfg, freq);
            Test_TextOutThroughput(cfg, freq);
            Test_InvalidateUpdate(cfg, freq);
            W32Prof_Test_ListViewPopulate(cfg);
            W32Prof_Test_TreeViewPopulate(cfg);
            W32Prof_Test_ImageListDraw(cfg);
            W32Prof_Test_TextMeasure(cfg);
         //   W32Prof_Test_DeferWindowPosBatch(cfg);
            W32Prof_Test_WindowCreateDestroy(cfg);
            W32Prof_Test_DrawIconGrid(cfg);
            W32Prof_Test_RegistryQuery(cfg);
            W32Prof_Test_FileEnumSystem32(cfg);
            W32Prof_Test_GdiPathStroke(cfg);
            W32Prof_Test_Win32TgaBlit(cfg);
            W32Prof_Test_DDrawTgaBlit(cfg);
            W32Prof_Test_D3D7Cube(cfg);
            W32Prof_Test_D3D8Cube(cfg);
            W32Prof_Test_D3D9Cube(cfg);
            W32Prof_Test_GL11Cube(cfg);
            W32Prof_Test_GL20Cube(cfg);
            W32Prof_Test_GL42Cube(cfg);

            /* Windowed textured tests only: fullscreen variants are opt-in. */
            W32Prof_Test_D3D7TexturedCube(cfg);
            W32Prof_Test_D3D9TexturedCube(cfg);
            W32Prof_Test_GL11TexturedCube(cfg);
            W32Prof_Test_GL20TexturedCube(cfg);
            W32Prof_Test_GL42TexturedCube(cfg);
            break;

        case W32PROF_TEST_BITBLT:
            Test_BitBltThroughput(cfg, freq);
            break;

        case W32PROF_TEST_GDI_HANDLES:
            Test_GdiHandleLockContention(cfg, freq);
            break;

        case W32PROF_TEST_USER_MESSAGES:
            Test_UserMessageLatency(cfg, freq);
            break;

        case W32PROF_TEST_WINDOWPOS:
            Test_WindowManagerLock(cfg, freq);
            break;

        case W32PROF_TEST_GETDC:
            Test_GetDcRelease(cfg, freq);
            break;

        case W32PROF_TEST_COMPATDC:
            Test_CompatDcCreateDelete(cfg, freq);
            break;

        case W32PROF_TEST_COMPATBMP:
            Test_CompatBitmapCreateDelete(cfg, freq);
            break;

        case W32PROF_TEST_SELECTOBJECT:
            Test_SelectObjectPressure(cfg, freq);
            break;

        case W32PROF_TEST_TEXTOUT:
            Test_TextOutThroughput(cfg, freq);
            break;

        case W32PROF_TEST_INVALIDATE_UPDATE:
            Test_InvalidateUpdate(cfg, freq);
            break;

        case W32PROF_TEST_LISTVIEW_POPULATE:
            W32Prof_Test_ListViewPopulate(cfg);
            break;

        case W32PROF_TEST_TREEVIEW_POPULATE:
            W32Prof_Test_TreeViewPopulate(cfg);
            break;

        case W32PROF_TEST_IMAGELIST_DRAW:
            W32Prof_Test_ImageListDraw(cfg);
            break;

        case W32PROF_TEST_TEXT_MEASURE:
            W32Prof_Test_TextMeasure(cfg);
            break;

        case W32PROF_TEST_DEFERWINDOWPOS:
            W32Prof_Test_DeferWindowPosBatch(cfg);
            break;

        case W32PROF_TEST_WINDOW_CREATE_DESTROY:
            W32Prof_Test_WindowCreateDestroy(cfg);
            break;

        case W32PROF_TEST_DRAWICON_GRID:
            W32Prof_Test_DrawIconGrid(cfg);
            break;

        case W32PROF_TEST_REGISTRY_QUERY:
            W32Prof_Test_RegistryQuery(cfg);
            break;

        case W32PROF_TEST_FILE_ENUM_SYSTEM32:
            W32Prof_Test_FileEnumSystem32(cfg);
            break;

        case W32PROF_TEST_GDI_PATH_STROKE:
            W32Prof_Test_GdiPathStroke(cfg);
            break;

        case W32PROF_TEST_WIN32_TGA_BLIT:
            W32Prof_Test_Win32TgaBlit(cfg);
            break;

        case W32PROF_TEST_DDRAW_TGA_BLIT:
            W32Prof_Test_DDrawTgaBlit(cfg);
            break;

        case W32PROF_TEST_D3D7_CUBE:
            W32Prof_Test_D3D7Cube(cfg);
            break;

        case W32PROF_TEST_D3D8_CUBE:
            W32Prof_Test_D3D8Cube(cfg);
            break;

        case W32PROF_TEST_D3D9_CUBE:
            W32Prof_Test_D3D9Cube(cfg);
            break;

        case W32PROF_TEST_GL11_CUBE:
            W32Prof_Test_GL11Cube(cfg);
            break;

        case W32PROF_TEST_GL20_CUBE:
            W32Prof_Test_GL20Cube(cfg);
            break;

        case W32PROF_TEST_GL42_CUBE:
            W32Prof_Test_GL42Cube(cfg);
            break;

        case W32PROF_TEST_D3D7_TEX_CUBE:
            W32Prof_Test_D3D7TexturedCube(cfg);
            break;

        case W32PROF_TEST_D3D7_TEX_CUBE_FS:
            W32Prof_Test_D3D7TexturedCubeFullscreen(cfg);
            break;

        case W32PROF_TEST_D3D9_TEX_CUBE:
            W32Prof_Test_D3D9TexturedCube(cfg);
            break;

        case W32PROF_TEST_D3D9_TEX_CUBE_FS:
            W32Prof_Test_D3D9TexturedCubeFullscreen(cfg);
            break;

        case W32PROF_TEST_GL11_TEX_CUBE:
            W32Prof_Test_GL11TexturedCube(cfg);
            break;

        case W32PROF_TEST_GL11_TEX_CUBE_FS:
            W32Prof_Test_GL11TexturedCubeFullscreen(cfg);
            break;

        case W32PROF_TEST_GL20_TEX_CUBE:
            W32Prof_Test_GL20TexturedCube(cfg);
            break;

        case W32PROF_TEST_GL42_TEX_CUBE:
            W32Prof_Test_GL42TexturedCube(cfg);
            break;

        case W32PROF_TEST_GL42_TEX_CUBE_FS:
            W32Prof_Test_GL42TexturedCubeFullscreen(cfg);
            break;

        default:
            ResultsPrint(TEXT("Unknown test id: %d"), (int)id);
            break;
    }
}

void
ProfilerRunTest(const ProfilerConfig* cfg, W32PROF_TEST_ID id)
{
    LONGLONG freq;
    DWORD oldPriorityClass = 0;
    DWORD_PTR oldAffinity = 0;

    if (!cfg)
        return;

    freq = QpcFreq();
    ResultsPrint(TEXT("QPC Frequency: %lld"), freq);

    ApplyAffinityAndRealtime(cfg, &oldPriorityClass, &oldAffinity);

    ResultsPrint(TEXT("--- W32Prof starting ---"));

    RunTestBody(cfg, id, freq);

    ResultsPrint(TEXT("--- W32Prof done ---"));

    RestoreAffinityAndPriority(oldPriorityClass, oldAffinity);
}

void
ProfilerRunContinuous(const ProfilerConfig* cfg, W32PROF_TEST_ID id)
{
    LONGLONG freq;
    DWORD oldPriorityClass = 0;
    DWORD_PTR oldAffinity = 0;
    ULONG pass = 0;

    if (!cfg)
        return;

    freq = QpcFreq();
    ResultsPrint(TEXT("QPC Frequency: %lld"), freq);

    ApplyAffinityAndRealtime(cfg, &oldPriorityClass, &oldAffinity);
    ResultsPrint(TEXT("--- W32Prof continuous start ---"));

    while (!ShouldStop(cfg))
    {
        pass++;
        ResultsPrint(TEXT("[pass %lu]"), pass);
        RunTestBody(cfg, id, freq);
    }

    ResultsPrint(TEXT("--- W32Prof continuous stop ---"));
    RestoreAffinityAndPriority(oldPriorityClass, oldAffinity);
}

void
ProfilerRunAll(const ProfilerConfig* cfg)
{
    ProfilerRunTest(cfg, W32PROF_TEST_ALL);
}
