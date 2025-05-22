#pragma once

class WindowClientData
{
public:
    WindowClientData(); 
    ~WindowClientData();
    wchar_t *pszTitle;
    CompositedWindow *CompositedWindow;
    HWND hWnd;
    HSPRITE SpriteHandle;
};
