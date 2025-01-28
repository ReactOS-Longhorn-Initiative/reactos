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
#include <winbase.h>
#define NDEBUG
#include <debug.h>

HDC
NTAPI
NtUserGetDC(
    HWND hWnd);

/*
 * @stub
 */
UINT
WINAPI
GetDpiForSystem(VOID)
{
    HDC hDC;
    UINT Dpi;
    hDC = NtUserGetDC(NULL);
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
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
IsProcessDPIAware(void)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
SetProcessDPIAware(void)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT context)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
GetProcessDpiAwarenessInternal(HANDLE process, DPI_AWARENESS *awareness)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
SetProcessDpiAwarenessInternal(DPI_AWARENESS awareness)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
GetDpiForMonitorInternal(HMONITOR monitor, UINT type, UINT *x, UINT *y)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
LogicalToPhysicalPoint( HWND hwnd, POINT *point )
{
    UNIMPLEMENTED;
    return TRUE;
}

