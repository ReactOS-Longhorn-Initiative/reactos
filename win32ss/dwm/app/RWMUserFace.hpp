#pragma once

#include <rwm.hpp>

class RWMUserFace {
public:
    HRESULT  WINAPI Initialize(HINSTANCE hInstance);
    HRESULT  WINAPI Run();
    HRESULT  WINAPI WaitForAndProcessEvent();
    VOID     WINAPI Cleanup();
    VOID     WINAPI OnClose();
    HRESULT  WINAPI InitializeWindow();
    LRESULT  WINAPI NotifyWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    /* Global objects */
    BOOLEAN    IsCompositionEnabled;
    DWORD      DWRMsgThreadId;
    HINSTANCE  DWRInstance;
    HWND       DWRWndNotify;
private:
};
