
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/heap.h"
#include "wine/debug.h"
#include "commoncontrols.h"

WINE_DEFAULT_DEBUG_CHANNEL(comctl32);

VOID
WINAPI
DrawSizeBox(HWND hwnd, HDC hdc, int x, int y);

VOID
WINAPI
DrawScrollBar(HWND hwnd, HDC hdc, RECT *prcOverrideClient, int fVert);

HWND
WINAPI
SizeBoxHwnd(HWND hwnd);

HRESULT
WINAPI
ScrollBar_MouseMove(HWND hwnd, POINT *ppt, int fVert);

VOID
WINAPI
ScrollBar_Menu(HWND hwndNotify, HWND hwnd, LPARAM lParam, int fVert);

VOID
WINAPI
HandleScrollCmd(HWND hwnd, WPARAM wParam, LPARAM lParam);

VOID
WINAPI
DetachScrollBars(HWND hwnd);

VOID
WINAPI
AttachScrollBars(HWND hwnd);

LRESULT 
WINAPI
CCSetScrollInfo(HWND *hwnd, int code, SCROLLINFO *lpsi, int fRedraw);

LRESULT 
WINAPI
CCGetScrollInfo(HWND *hwnd, int Bar, SCROLLINFO *psi);

LRESULT 
WINAPI
CCEnableScrollBar(HWND *hwnd, UINT32 Flags, UINT32 Arrows);

VOID
WINAPI
DrawSizeBox(HWND hwnd, HDC hdc, int x, int y)
{
    UNIMPLEMENTED;
    __debugbreak();
}

VOID
WINAPI
DrawScrollBar(HWND hwnd, HDC hdc, RECT *prcOverrideClient, int fVert)
{
    UNIMPLEMENTED;
    __debugbreak();
}

HWND
WINAPI
SizeBoxHwnd(HWND hwnd)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

HRESULT
WINAPI
ScrollBar_MouseMove(HWND hwnd, POINT *ppt, int fVert)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0; 
}

VOID
WINAPI
ScrollBar_Menu(HWND hwndNotify, HWND hwnd, LPARAM lParam, int fVert)
{
    UNIMPLEMENTED;
    __debugbreak();
}

VOID
WINAPI
HandleScrollCmd(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    UNIMPLEMENTED;
    __debugbreak();  
}

VOID
WINAPI
DetachScrollBars(HWND hwnd)
{
    UNIMPLEMENTED;
    __debugbreak();  
}

VOID
WINAPI
AttachScrollBars(HWND hwnd)
{
    UNIMPLEMENTED;
    __debugbreak();  
}

/* maybe win10? */
LRESULT 
WINAPI
CCSetScrollInfo(HWND *hwnd, int code, SCROLLINFO *lpsi, int fRedraw)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0; 
}

LRESULT 
WINAPI
CCGetScrollInfo(HWND *hwnd, int Bar, SCROLLINFO *psi)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0; 
}

LRESULT 
WINAPI
CCEnableScrollBar(HWND *hwnd, UINT32 Flags, UINT32 Arrows)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0; 
}
