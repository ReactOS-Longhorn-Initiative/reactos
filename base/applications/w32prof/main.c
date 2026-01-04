#include "profiler.h"

#include <windows.h>
#include <tchar.h>

#define W32PROF_WM_RUNTESTS (WM_APP + 0x414)
#define W32PROF_WM_TESTDONE (WM_APP + 0x415)

#define IDC_TESTSELECT 100
#define IDC_RUNTEST    101
#define IDC_RUNCONT    102
#define IDC_STOP       103

static HINSTANCE g_hInst;
static HWND g_hResultsWnd;
static HWND g_hEdit;
static HWND g_hCombo;
static HWND g_hRunBtn;
static HWND g_hRunContBtn;
static HWND g_hStopBtn;
static HWND g_hTestWnd;
static W32PROF_TESTWND_STATE g_TestState;
static HANDLE g_hTestThread;
static DWORD g_TestThreadId;
static HANDLE g_hTestReadyEvent;
static HANDLE g_hProfilerThread;
static ProfilerConfig g_RunCfg;
static HANDLE g_hStopEvent;

static int W32ProfMain(HINSTANCE hInstance);

static void
BuildDefaultConfig(ProfilerConfig* cfg, BOOL headless)
{
    ZeroMemory(cfg, sizeof(*cfg));

    cfg->BitBltIterations = 10000;
    cfg->GdiObjectIterations = 50000;
    cfg->MessageIterations = 50000;
    cfg->WindowPosIterations = 10000;
    cfg->GetDcIterations = 100000;
    cfg->CompatDcIterations = 50000;
    cfg->CompatBmpIterations = 20000;
    cfg->SelectObjectIterations = 200000;
    cfg->TextOutIterations = 50000;
    cfg->InvalidateIterations = 2000;
    cfg->GpuFrames = 600;

    cfg->TestWidth = 320;
    cfg->TestHeight = 240;

    cfg->Headless = headless;
    cfg->hTestWnd = g_hTestWnd;
    cfg->TestState = &g_TestState;
    cfg->TestId = W32PROF_TEST_ALL;

    cfg->Continuous = FALSE;
    cfg->StopEvent = g_hStopEvent;
}

static BOOL
CommandLineHasHeadless(void)
{
    const TCHAR* cmd = GetCommandLine();
    if (!cmd)
        return FALSE;

    return (_tcsstr(cmd, TEXT("-headless")) != NULL);
}

static void
CreateResultsControls(HWND hWnd)
{
    UINT count = 0;
    const W32PROF_TEST_ENTRY* tests = W32ProfGetTestList(&count);
    UINT i;

    g_hCombo = CreateWindowEx(0,
                              TEXT("COMBOBOX"),
                              TEXT(""),
                              WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
                              0, 0, 0, 0,
                              hWnd,
                              (HMENU)IDC_TESTSELECT,
                              g_hInst,
                              NULL);

    g_hRunBtn = CreateWindowEx(0,
                               TEXT("BUTTON"),
                               TEXT("Run (600 frames / once)"),
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                               0, 0, 0, 0,
                               hWnd,
                               (HMENU)IDC_RUNTEST,
                               g_hInst,
                               NULL);

    g_hRunContBtn = CreateWindowEx(0,
                                   TEXT("BUTTON"),
                                   TEXT("Run Continuous"),
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                   0, 0, 0, 0,
                                   hWnd,
                                   (HMENU)IDC_RUNCONT,
                                   g_hInst,
                                   NULL);

    g_hStopBtn = CreateWindowEx(0,
                                TEXT("BUTTON"),
                                TEXT("Stop"),
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                0, 0, 0, 0,
                                hWnd,
                                (HMENU)IDC_STOP,
                                g_hInst,
                                NULL);

    if (g_hStopBtn)
        EnableWindow(g_hStopBtn, FALSE);

    g_hEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
                             TEXT("EDIT"),
                             TEXT(""),
                             WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                             0, 0, 0, 0,
                             hWnd,
                             (HMENU)1,
                             g_hInst,
                             NULL);

    if (g_hCombo && tests && count)
    {
        for (i = 0; i < count; i++)
        {
            LRESULT idx = SendMessage(g_hCombo, CB_ADDSTRING, 0, (LPARAM)tests[i].Name);
            if (idx >= 0)
                SendMessage(g_hCombo, CB_SETITEMDATA, (WPARAM)idx, (LPARAM)tests[i].Id);
        }
        SendMessage(g_hCombo, CB_SETCURSEL, 0, 0);
    }
}

static DWORD WINAPI
ProfilerThreadProc(LPVOID Param)
{
    const ProfilerConfig* cfg = (const ProfilerConfig*)Param;

    if (cfg->Continuous)
        ProfilerRunContinuous(cfg, cfg->TestId);
    else
        ProfilerRunTest(cfg, cfg->TestId);

    if (g_hResultsWnd)
        PostMessage(g_hResultsWnd, W32PROF_WM_TESTDONE, 0, 0);
    return 0;
}

static LRESULT CALLBACK
ResultsWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                if (g_hProfilerThread && g_hStopEvent)
                    SetEvent(g_hStopEvent);
                return 0;
            }
            break;

        case WM_CREATE:
            CreateResultsControls(hWnd);
            ResultsInit(g_hEdit);
            ResultsPrint(TEXT("Select a test and click Run."));
            return 0;

        case WM_SIZE:
            if (g_hEdit)
            {
                int w = (int)LOWORD(lParam);
                int h = (int)HIWORD(lParam);
                int top = 28;
                int btnW = 160;
                int btnW2 = 140;
                int btnW3 = 80;
                if (g_hCombo)
                    MoveWindow(g_hCombo, 6, 4, w - (btnW + btnW2 + btnW3 + 24), 400, TRUE);
                if (g_hRunBtn)
                    MoveWindow(g_hRunBtn, w - (btnW + btnW2 + btnW3 + 6), 4, btnW, 20, TRUE);
                if (g_hRunContBtn)
                    MoveWindow(g_hRunContBtn, w - (btnW2 + btnW3 + 6), 4, btnW2, 20, TRUE);
                if (g_hStopBtn)
                    MoveWindow(g_hStopBtn, w - (btnW3 + 6), 4, btnW3, 20, TRUE);
                MoveWindow(g_hEdit, 0, top, w, h - top, TRUE);
            }
            return 0;

        case WM_COMMAND:
        {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);

            if ((id == IDC_RUNTEST || id == IDC_RUNCONT) && code == BN_CLICKED)
            {
                if (g_hProfilerThread)
                    return 0;

                if (g_hStopEvent)
                    ResetEvent(g_hStopEvent);

                BuildDefaultConfig(&g_RunCfg, FALSE);
                g_RunCfg.hTestWnd = g_hTestWnd;
                g_RunCfg.TestState = &g_TestState;

                g_RunCfg.Continuous = (id == IDC_RUNCONT);

                if (g_hCombo)
                {
                    LRESULT sel = SendMessage(g_hCombo, CB_GETCURSEL, 0, 0);
                    if (sel >= 0)
                    {
                        g_RunCfg.TestId = (W32PROF_TEST_ID)SendMessage(g_hCombo, CB_GETITEMDATA, (WPARAM)sel, 0);
                    }
                }

                /* One-shot mode keeps the legacy default (GpuFrames=600).
                   Continuous mode runs until Stop is pressed (GpuFrames=0). */
                if (g_RunCfg.Continuous)
                    g_RunCfg.GpuFrames = 0;

                if (g_hRunBtn)
                    EnableWindow(g_hRunBtn, FALSE);
                if (g_hRunContBtn)
                    EnableWindow(g_hRunContBtn, FALSE);
                if (g_hStopBtn)
                    EnableWindow(g_hStopBtn, TRUE);

                ResultsPrint(g_RunCfg.Continuous ? TEXT("Starting continuous run...") : TEXT("Starting one-shot run..."));
                g_hProfilerThread = CreateThread(NULL, 0, ProfilerThreadProc, &g_RunCfg, 0, NULL);
                if (!g_hProfilerThread)
                {
                    ResultsPrint(TEXT("ERROR: Failed to create profiler thread: %lu"), GetLastError());
                    if (g_hRunBtn)
                        EnableWindow(g_hRunBtn, TRUE);
                    if (g_hRunContBtn)
                        EnableWindow(g_hRunContBtn, TRUE);
                }
                return 0;
            }

            if (id == IDC_STOP && code == BN_CLICKED)
            {
                if (g_hStopEvent)
                    SetEvent(g_hStopEvent);
                return 0;
            }

            if (id == IDC_TESTSELECT && code == CBN_SELCHANGE)
            {
                UINT count = 0;
                const W32PROF_TEST_ENTRY* tests = W32ProfGetTestList(&count);
                LRESULT sel = SendMessage(g_hCombo, CB_GETCURSEL, 0, 0);
                if (sel >= 0 && tests)
                {
                    W32PROF_TEST_ID tid = (W32PROF_TEST_ID)SendMessage(g_hCombo, CB_GETITEMDATA, (WPARAM)sel, 0);
                    UINT i;
                    for (i = 0; i < count; i++)
                    {
                        if (tests[i].Id == tid)
                        {
                            ResultsPrint(TEXT("%s: %s"), tests[i].Name, tests[i].What);
                            break;
                        }
                    }
                }
                return 0;
            }

            break;
        }

        case W32PROF_WM_TESTDONE:
            if (g_hProfilerThread)
            {
                WaitForSingleObject(g_hProfilerThread, INFINITE);
                CloseHandle(g_hProfilerThread);
                g_hProfilerThread = NULL;
            }
            if (g_hRunBtn)
                EnableWindow(g_hRunBtn, TRUE);
            if (g_hRunContBtn)
                EnableWindow(g_hRunContBtn, TRUE);
            if (g_hStopBtn)
                EnableWindow(g_hStopBtn, FALSE);
            ResultsPrint(TEXT("Done."));
            return 0;

        case WM_CLOSE:
            if (g_hStopEvent)
                SetEvent(g_hStopEvent);
            if (g_hTestWnd)
                PostMessage(g_hTestWnd, WM_CLOSE, 0, 0);
            DestroyWindow(hWnd);
            return 0;

        case W32PROF_WM_RUNTESTS:
        {
            /* Legacy internal message (kept for compatibility); UI uses Run button now */
            return 0;
        }

        case WM_DESTROY:
            if (g_hProfilerThread)
            {
                if (g_hStopEvent)
                    SetEvent(g_hStopEvent);
                WaitForSingleObject(g_hProfilerThread, INFINITE);
                CloseHandle(g_hProfilerThread);
                g_hProfilerThread = NULL;
            }

            if (g_hTestWnd)
                PostMessage(g_hTestWnd, WM_CLOSE, 0, 0);
            if (g_hTestThread)
            {
                WaitForSingleObject(g_hTestThread, INFINITE);
                CloseHandle(g_hTestThread);
                g_hTestThread = NULL;
            }
            g_hTestWnd = NULL;

            if (g_TestState.PostDoneEvent)
            {
                CloseHandle(g_TestState.PostDoneEvent);
                g_TestState.PostDoneEvent = NULL;
            }

            if (g_hStopEvent)
            {
                CloseHandle(g_hStopEvent);
                g_hStopEvent = NULL;
            }

            ResultsShutdown();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK
RenderChildWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                if (g_hStopEvent)
                    SetEvent(g_hStopEvent);
                return 0;
            }
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            return 0;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK
TestWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                if (g_hStopEvent)
                    SetEvent(g_hStopEvent);
                return 0;
            }
            break;

        case W32PROF_WM_TESTMSG:
        {
            LONG v = InterlockedIncrement(&g_TestState.PostReceived);
            LONG target = g_TestState.PostTarget;
            if (target > 0 && v >= target && g_TestState.PostDoneEvent)
                SetEvent(g_TestState.PostDoneEvent);
            return 0;
        }

        case WM_CLOSE:
            if (g_hResultsWnd)
                PostMessage(g_hResultsWnd, WM_CLOSE, 0, 0);
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            if (g_hResultsWnd)
                PostMessage(g_hResultsWnd, WM_CLOSE, 0, 0);
            PostQuitMessage(0);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            if (hdc)
            {
                RECT rc;
                GetClientRect(hWnd, &rc);
                FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
            }
            EndPaint(hWnd, &ps);
            return 0;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static DWORD WINAPI
TestThreadProc(LPVOID Param)
{
    BOOL visible = (BOOL)(ULONG_PTR)Param;

    g_hTestWnd = CreateWindowEx(0,
                                TEXT("W32ProfTestWindow"),
                                TEXT("W32Prof Test Window"),
                                visible ? WS_OVERLAPPEDWINDOW : WS_OVERLAPPED,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                640, 480,
                                NULL,
                                NULL,
                                g_hInst,
                                NULL);

    if (g_hTestWnd && visible)
        ShowWindow(g_hTestWnd, SW_SHOW);

    if (g_hTestWnd)
        UpdateWindow(g_hTestWnd);

    if (g_hTestReadyEvent)
        SetEvent(g_hTestReadyEvent);

    if (g_hTestWnd)
    {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

static BOOL
RegisterWindows(void)
{
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ResultsWndProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = TEXT("W32ProfResultsWindow");

    if (!RegisterClassEx(&wc))
        return FALSE;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = TestWndProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = TEXT("W32ProfTestWindow");

    if (!RegisterClassEx(&wc))
        return FALSE;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = RenderChildWndProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = TEXT("W32ProfRenderChild");

    if (!RegisterClassEx(&wc))
        return FALSE;

    return TRUE;
}

static BOOL
CreateTestWindow(BOOL visible)
{
    g_hTestReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hTestReadyEvent)
        return FALSE;

    g_hTestThread = CreateThread(NULL, 0, TestThreadProc, (LPVOID)(ULONG_PTR)visible, 0, &g_TestThreadId);
    if (!g_hTestThread)
        return FALSE;

    WaitForSingleObject(g_hTestReadyEvent, INFINITE);
    CloseHandle(g_hTestReadyEvent);
    g_hTestReadyEvent = NULL;

    return (g_hTestWnd != NULL);
}

static BOOL
CreateResultsWindow(void)
{
    g_hResultsWnd = CreateWindowEx(0,
                                   TEXT("W32ProfResultsWindow"),
                                   TEXT("W32Prof Results"),
                                   WS_OVERLAPPEDWINDOW,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   800, 400,
                                   NULL,
                                   NULL,
                                   g_hInst,
                                   NULL);

    if (!g_hResultsWnd)
        return FALSE;

    ShowWindow(g_hResultsWnd, SW_SHOW);
    UpdateWindow(g_hResultsWnd);

    return TRUE;
}

int WINAPI
_tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;
    return W32ProfMain(hInstance);
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;
    return W32ProfMain(hInstance);
}

static int
W32ProfMain(HINSTANCE hInstance)
{
    MSG msg;
    BOOL headless;

    g_hInst = hInstance;

    g_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hStopEvent)
        return 1;

    ZeroMemory(&g_TestState, sizeof(g_TestState));
    g_TestState.PostDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_TestState.PostDoneEvent)
        return 1;

    headless = CommandLineHasHeadless();

    if (!RegisterWindows())
        return 1;

    if (!CreateTestWindow(!headless))
        return 1;

    if (!headless)
    {
        if (!CreateResultsWindow())
            return 1;

        /* User-triggered via UI */

        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return (int)msg.wParam;
    }

    ResultsInit(NULL);
    {
        ProfilerConfig cfg;
        BuildDefaultConfig(&cfg, TRUE);
        cfg.hTestWnd = g_hTestWnd;
        cfg.TestState = &g_TestState;
        cfg.TestId = W32PROF_TEST_ALL;
        ResultsPrint(TEXT("Headless mode"));
        ProfilerRunTest(&cfg, cfg.TestId);
    }
    ResultsShutdown();

    if (g_hTestWnd)
        PostMessage(g_hTestWnd, WM_CLOSE, 0, 0);
    if (g_hTestThread)
    {
        WaitForSingleObject(g_hTestThread, INFINITE);
        CloseHandle(g_hTestThread);
        g_hTestThread = NULL;
    }
    if (g_TestState.PostDoneEvent)
    {
        CloseHandle(g_TestState.PostDoneEvent);
        g_TestState.PostDoneEvent = NULL;
    }
    return 0;
}
