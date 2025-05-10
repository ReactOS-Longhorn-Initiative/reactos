
#include <win32k.h>
#include <debug.h>

BOOLEAN IsCompositionEnabled = FALSE;
PEPROCESS GlobalDwmProcessPtr;
HANDLE GlobalDwmApiPort;
extern PDESKTOP gpdeskInputDesktop;

BOOL FASTCALL
IntSetLayeredWindowAttributes(PWND pWnd,
                              COLORREF crKey,
                              BYTE bAlpha,
                              DWORD dwFlags);

NTSTATUS
WINAPI
IntLpcStyleDwmNewWindow(PWND Pwnd, BOOLEAN NewWindow)
{

    if (NewWindow)
    {
        if (/*LayeredWindowSupportIsOff*/1)
        {
            IntInvalidateWindows(Pwnd,
                (PREGION)1,
                RDW_FRAME | RDW_ERASE |
                RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
        else
        {
            SetLayeredStatus(Pwnd, 0);
            IntSetLayeredWindowAttributes(Pwnd,(COLORREF)NULL,255,2);
        }
    }

    return 0;
}

VOID
WINAPI
IntLpcStyleDwmRenderDesktop(PWND Pwnd, BOOLEAN Add)
{
    DPRINT1("IntLpcStyleDwmRenderDesktop Entry\n");
    PWND pwndNode;
    HWND *phwnd;
    if ( Pwnd )
    {
        PWINDOWLIST WindowList = IntBuildHwndList(Pwnd->spwndChild, IACE_LIST, 0);
        if ( WindowList )
        {

          for (phwnd = WindowList->ahwnd; *phwnd != HWND_TERMINATOR; ++phwnd)
          {
                pwndNode = ValidateHwndNoErr(*phwnd);
                if (pwndNode)
                    IntLpcStyleDwmNewWindow(pwndNode, Add);
          }

          DbgBreakPoint();
          IntFreeHwndList(WindowList);
      }
    }

  //  IntLpcStyleDwmRegenerateChildren(Pwnd, Add);
}

//DwmDesktopSwitch
NTSTATUS
WINAPI
IntLpcStyleDwmNewDesktop(VOID)
{
    return 0;
}


NTSTATUS
InternalDwmStartup()
{
    DPRINT1("Starting ReactOS Win32k DWM Internal\n");
    IsCompositionEnabled = TRUE;
    PWND DesktopWindow = ValidateHwndNoErr(gpdeskInputDesktop->DesktopWindow);
    IntLpcStyleDwmRenderDesktop(DesktopWindow, TRUE);
    __debugbreak();
    return 0;
}

/* EXPORTS */


NTSTATUS
APIENTRY
NtUserDwmShutdown(VOID)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
NtUserDwmStartup(HANDLE Handle)
{
    NTSTATUS Status;
    if ( GlobalDwmProcessPtr )
        return TRUE;

    /* Set the state and reference the handle */
    Status = ObReferenceObjectByHandle(Handle, 0, LpcPortObjectType, 1, &Handle, 0);
    GlobalDwmApiPort = Handle;

    if (Status == STATUS_SUCCESS)
    {
        GlobalDwmProcessPtr = PsGetCurrentProcess();
        Status = InternalDwmStartup();

        if (Status == STATUS_SUCCESS)
            return TRUE;

        /* We failed, let's clean up */
        if (GlobalDwmApiPort)
          ObfDereferenceObject(GlobalDwmApiPort);

        GlobalDwmApiPort = 0;
        GlobalDwmProcessPtr = (PEPROCESS)NULL;
    }

    /* Dwm is not on :( */
    return FALSE;
}


NTSTATUS
APIENTRY
NtUserUpdateWindowTransform(HWND Hwnd, PVOID pTransform)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
NtUserDwmGetSurfaceData(PVOID Hdev, PVOID Surface)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
NtUserSetWindowRgnEx(HWND Hwnd, HRGN Rgn, UINT32 Flags)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_UNSUCCESSFUL;
}
