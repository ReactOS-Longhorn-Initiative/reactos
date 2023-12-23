#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER
#include <windef.h>
#include <winuser.h>


BOOL WINAPI ChangeWindowMessageFilter( UINT message, DWORD flag )
{
  //  DbgPrint( "%x %08lx\n", message, flag );
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
