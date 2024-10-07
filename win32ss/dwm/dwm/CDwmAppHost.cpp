#include <dwm.hpp>
#include <debug.h>

EXTERN_C
LRESULT WINAPI NotifyWndProcLoc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
}


LRESULT
CDwmAppHost::NotifyWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

/*
 * @PURPOSE: Create an Empty Win32 Window
 */
HRESULT
WINAPI
CDwmAppHost::InitializeWindow()
{
    WNDCLASSEXW Class;
    memset(&Class.style, 0, 0x2Cu);
    Class.cbSize = 48;
    Class.lpfnWndProc = NotifyWndProcLoc;
    Class.hInstance = s_hInstance;
    Class.lpszClassName = L"Dwm";
    DPRINT1("DWM: Creating Window\n");

    RegisterClassExW(&Class);
    s_hWndNotify = CreateWindowExW(0,
                                   L"Dwm",
                                   L"DWM Notification Window",
                                   0xA0000000,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   (HINSTANCE)s_hInstance,
                                   0);

    return S_OK;
}

/*
 *
 */
HRESULT
WINAPI
CDwmAppHost::Initialize(HINSTANCE hInstance)
{
    s_hInstance = hInstance;
    s_dwMsgThreadId = GetCurrentThreadId();
    return CDwmAppHost::InitializeWindow();
    return S_OK;
}

HRESULT
WINAPI
CDwmAppHost::WaitForAndProcessEvent()
{
    return S_OK;
}

HRESULT
WINAPI
CDwmAppHost::Run()
{
    MSG Msg;
    UINT ExitCode;
    HRESULT HResult;
    ExitCode = 0;
    DPRINT1("DWM: Starting the DWM service\n");
    while (1)
    {
        Msg.hwnd = 0;
        Msg.message = 0;
        Msg.wParam = 0;
        Msg.lParam = 0;
        Msg.time = 0;
        Msg.pt.x = 0;
        Msg.pt.y = 0;
        while (PeekMessageW(&Msg, 0, 0, 0, 1u) && Msg.message != 18)
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }
        if (Msg.message == 18)
            break;
        HResult = CDwmAppHost::WaitForAndProcessEvent();
        if (HResult != S_OK)
        {
            ExitCode = HResult;
            goto StartShutdown;
        }
    }
    // CDwmAppHost::OnClose();
StartShutdown:
    DPRINT1("Shutting down the DWM with return code: %d\n", ExitCode);
    return ExitCode;
}

/* On the event of Dwm Closing*/
VOID
WINAPI
CDwmAppHost::OnClose()
{
    //TODO: We should clean up all globals
}

/* Preparing to exit service */
VOID
WINAPI
CDwmAppHost::Cleanup()
{
    //TODO: We should clean up all globals
}