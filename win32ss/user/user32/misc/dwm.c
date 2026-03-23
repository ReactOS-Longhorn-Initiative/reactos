/*
 * PROJECT:     ReactOS user32.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Longhorn / Vista-era DWM-related exports (LH 5048 / milcore expectations)
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/**********************************************************************
 *              SetWindowRgnEx [USER32.@]
 */
INT
WINAPI
SetWindowRgnEx(HWND hWnd, HRGN hRgn, DWORD dwFlags)
{
    BOOL Hook;
    INT Ret = 0;

    LoadUserApiHook();

    Hook = BeginIfHookedUserApiHook();

    if (!Hook)
    {
        Ret = NtUserSetWindowRgnEx(hWnd, hRgn, dwFlags);
        if (Ret)
            DeleteObject(hRgn);
        return Ret;
    }

    _SEH2_TRY
    {
        Ret = guah.SetWindowRgn(hWnd, hRgn, (dwFlags & 1u) ? TRUE : FALSE);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    EndUserApiHook();

    return Ret;
}

/**********************************************************************
 *              UpdateWindowTransform [USER32.@]
 */
BOOL
WINAPI
UpdateWindowTransform(HWND hwnd, const void *pTransform, DWORD cbOrFlags)
{
    return NtUserUpdateWindowTransform(hwnd, (PVOID)pTransform, cbOrFlags);
}

/**********************************************************************
 *              DwmGetSurfaceData [USER32.@]
 *
 * Longhorn 5048: win32k NtUserDwmGetSurfaceData (not NtGdi*).
 */
BOOL
WINAPI
DwmGetSurfaceData(HWND hwnd, PVOID pSurfaceDataOut)
{
    return NtUserDwmGetSurfaceData(hwnd, pSurfaceDataOut);
}

/**********************************************************************
 *              DwmStartup [USER32.@]
 */
BOOL
WINAPI
DwmStartup(DWORD dwReserved)
{
    /* milcore passes the connected LPC port handle (see Longhorn milcore DwmStartup). */
    return NtUserDwmStartup((HANDLE)(ULONG_PTR)dwReserved);
}

/**********************************************************************
 *              DwmShutdown [USER32.@]
 */
BOOL
WINAPI
DwmShutdown(void)
{
    return NtUserDwmShutdown();
}
