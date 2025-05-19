#include <rwm.hpp>
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
RWMUserFace::NotifyWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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
RWMUserFace::InitializeWindow()
{
    WNDCLASSEXW Class;
    RtlZeroMemory(&Class, sizeof(Class));
    Class.cbSize = 48;
    Class.lpfnWndProc = NotifyWndProcLoc;
    Class.hInstance = DWRInstance;
    Class.lpszClassName = RWMAPP_WINDOWCLASS;
    DPRINT1("DWM: Creating Window\n");

    RegisterClassExW(&Class);
    DWRWndNotify = CreateWindowExW(0,
                                   RWMAPP_WINDOWCLASS,
                                   RWMAPP_WINDOWDESC,
                                   0xA0000000,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   (HINSTANCE)DWRInstance,
                                   0);

    return S_OK;
}

/*
 *
 */
HRESULT
WINAPI
RWMUserFace::Initialize(HINSTANCE hInstance)
{
    DWRInstance = hInstance;
    DWRMsgThreadId = GetCurrentThreadId();
    return RWMUserFace::InitializeWindow();
    return S_OK;
}

HRESULT
WINAPI
RWMUserFace::WaitForAndProcessEvent()
{
    return S_OK;
}

HRESULT
WINAPI
RWMUserFace::Run()
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
        HResult = RWMUserFace::WaitForAndProcessEvent();
        if (HResult != S_OK)
        {
            ExitCode = HResult;
            goto StartShutdown;
        }
    }

StartShutdown:
    DPRINT1("Shutting down the DWM with return code: %d\n", ExitCode);
    return ExitCode;
}

VOID    
WINAPI RWMUserFace::Cleanup(){
    DPRINT1("DWM: Cleaning up the DWM service\n");
    if (DWRWndNotify)
    {
        DestroyWindow(DWRWndNotify);
        DWRWndNotify = 0;
    }
    UnregisterClassW(RWMAPP_WINDOWCLASS, DWRInstance);
}