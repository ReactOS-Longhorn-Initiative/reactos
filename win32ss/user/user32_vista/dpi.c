/*
 * PROJECT:     ReactOS Kernel - Vista+ APIs
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     DPI functions for user32 and user32_vista.
 * COPYRIGHT:   Copyright 2024 Carl Bialorucki <cbialo2@outlook.com>
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <wingdi.h>
#include <winuser.h>

#define NDEBUG
#include <debug.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER
#include <windef.h>
#include <winuser.h>


BOOL WINAPI ChangeWindowMessageFilterEx(
   HWND                hwnd,
   UINT                message,
   DWORD               action,
   PVOID pChangeFilterStruct
)
{
    UNIMPLEMENTED;
    return 1;
}
BOOL
WINAPI
ChangeWindowMessageFilter(
    UINT  message,
    DWORD dwFlag
)
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOL WINAPI ShutdownBlockReasonCreate(_In_ HWND    hWnd,
                                      _In_ LPCWSTR pwszReason)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(pwszReason);
    //UNIMPLEMENTED;
    return TRUE;
}


BOOL WINAPI IsThreadDesktopComposited()
{
   // UNIMPLEMENTED;
    return FALSE;
}

HWND WINAPI GhostWindowFromHungWindow (HWND hwndGhost)
{
   // UNIMPLEMENTED;
    return hwndGhost;
}

HWND WINAPI HungWindowFromGhostWindow (HWND hwndGhost)
{
  //  UNIMPLEMENTED;
    return hwndGhost;
}

BOOL WINAPI IsProcessDPIAware()
{
    return FALSE;
}

BOOL WINAPI SetProcessDPIAware()
{
    return FALSE;
}
/*
 * @stub
 */
UINT
WINAPI
GetDpiForSystem(VOID)
{
    HDC hDC;
    UINT Dpi;
    hDC = GetDC(NULL);
    Dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);
    return Dpi;
}

/*
 * @stub
 */
UINT
WINAPI
GetDpiForWindow(
    _In_ HWND hWnd)
{
    UNIMPLEMENTED_ONCE;
    UNREFERENCED_PARAMETER(hWnd);
    return GetDpiForSystem();
}









BOOL WINAPI SetGestureConfig(
     HWND           hwnd,
     DWORD          dwReserved,
     UINT           cIDs,
     PVOID  pGestureConfig,
     UINT           cbSize
)
{
    return FALSE;
}