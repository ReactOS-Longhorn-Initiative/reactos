/*
 * PROJECT:         ReactOS Win32k
 * LICENSE:         GPL-2.0-or-later
 * PURPOSE:         Vista / Longhorn DWM-related syscalls (NtUser*)
 *
 * NtUserDwmStartup / NtUserDwmShutdown follow Windows Vista build 5048 win32k (see LH win32k.sys).
 * NtUserDwmGetSurfaceData: 5048 win32k (EnterCrit + GreDwmGetSurfaceData for gpepDwm only).
 * NtUserUpdateWindowTransform follows the same build’s per-window MIL transform slot (64 bytes).
 * 5048 MIL op 24 (hit-test wait/reply) is win32k-internal (DwmHitTestQuery + LpcRequestWaitReplyPort), not an NtUser syscall.
 */

#include <win32k.h>
#include <ndk/lpcfuncs.h>
#include <ntgdibad.h>

#include "dwm.h"
#include "dwmnotify.h"

DBG_DEFAULT_CHANNEL(UserMisc);

#include <debug.h>

#define MIL_TRANSFORM_SIZE 64ul

#ifndef PORT_MAXIMUM_MESSAGE_LENGTH
#define DWM_LPC_MSG_CAP 512
#else
#define DWM_LPC_MSG_CAP PORT_MAXIMUM_MESSAGE_LENGTH
#endif

/* Registered milcore LPC connection (object pointer, not user handle). */
static PVOID gpvDwmApiPort = NULL;

PEPROCESS gpepDwm = NULL;
BOOLEAN gfbDwmCompositing = FALSE;

BOOLEAN
FASTCALL
DwmIsDwmClientProcess(VOID)
{
    return (gpepDwm != NULL) && (PsGetCurrentProcess() == gpepDwm);
}

static NTSTATUS
IntXxxDwmStartup(VOID)
{
    HDEV hdev;

    if (gfbDwmCompositing)
    {
        DPRINT1("[DWM] IntXxxDwmStartup: already compositing\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!gpmdev || !gpmdev->ppdevGlobal)
    {
        DPRINT1("[DWM] IntXxxDwmStartup: no gpmdev/ppdevGlobal gpmdev=%p\n", gpmdev);
        return STATUS_DEVICE_NOT_READY;
    }

    hdev = (HDEV)gpmdev->ppdevGlobal;

    if (!GreDwmStartup(hdev))
    {
        DPRINT1("[DWM] IntXxxDwmStartup: GreDwmStartup failed hdev=%p\n", hdev);
        return STATUS_UNSUCCESSFUL;
    }

    gfbDwmCompositing = TRUE;
    DPRINT1("[DWM] IntXxxDwmStartup: OK hdev=%p compositing=1\n", hdev);
    /* 5048 xxxComposeDesktop: DwmNotifyChildrenAddRemove for existing WS_CHILD windows. */
    IntDwmNotifyDesktopChildrenAddRemove(TRUE);
    UserRedrawDesktop();
    return STATUS_SUCCESS;
}

static NTSTATUS
IntXxxDwmShutdown(VOID)
{
    HDEV hdev;

    if (!gfbDwmCompositing)
    {
        DPRINT1("[DWM] IntXxxDwmShutdown: not compositing\n");
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT1("[DWM] IntXxxDwmShutdown: begin\n");
    IntDwmNotifyDesktopChildrenAddRemove(FALSE);
    IntDwmUnbindAllActiveRedirectDcs();
    IntDwmFreeAllRedirectBitmaps();

    if (!gpmdev || !gpmdev->ppdevGlobal)
    {
        DPRINT1("[DWM] IntXxxDwmShutdown: no pdev (compositing cleared)\n");
        gfbDwmCompositing = FALSE;
        return STATUS_SUCCESS;
    }

    hdev = (HDEV)gpmdev->ppdevGlobal;

    UserRedrawDesktop();

    if (!GreDwmShutdown(hdev))
    {
        DPRINT1("[DWM] IntXxxDwmShutdown: GreDwmShutdown failed hdev=%p\n", hdev);
        gfbDwmCompositing = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    gfbDwmCompositing = FALSE;
    DPRINT1("[DWM] IntXxxDwmShutdown: OK\n");
    UserRedrawDesktop();
    return STATUS_SUCCESS;
}

BOOL
APIENTRY
NtUserDwmStartup(_In_ HANDLE hDwmApiPort)
{
    NTSTATUS Status;
    PVOID PortObject = NULL;
    BOOL ret = FALSE;

    TRACE("NtUserDwmStartup: port handle %p\n", hDwmApiPort);

    UserEnterExclusive();

    if (gpepDwm)
    {
        DPRINT1("[DWM] NtUserDwmStartup: DWM already registered\n");
        EngSetLastError(ERROR_ACCESS_DENIED);
        goto leave;
    }

    if (!hDwmApiPort)
    {
        DPRINT1("[DWM] NtUserDwmStartup: null port handle\n");
        EngSetLastError(ERROR_INVALID_HANDLE);
        goto leave;
    }

    Status = ObReferenceObjectByHandle(hDwmApiPort,
                                       0,
                                       LpcPortObjectType,
                                       UserMode,
                                       &PortObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("[DWM] NtUserDwmStartup: ObReferenceObjectByHandle failed %08lX\n", Status);
        EngSetLastError(RtlNtStatusToDosError(Status));
        goto leave;
    }

    gpvDwmApiPort = PortObject;
    gpepDwm = PsGetCurrentProcess();
    DPRINT1("[DWM] NtUserDwmStartup: port=%p DWM process=%p\n", gpvDwmApiPort, gpepDwm);

    Status = IntXxxDwmStartup();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("[DWM] NtUserDwmStartup: IntXxxDwmStartup failed %08lX\n", Status);
        ObDereferenceObject(gpvDwmApiPort);
        gpvDwmApiPort = NULL;
        gpepDwm = NULL;
        EngSetLastError(RtlNtStatusToDosError(Status));
        goto leave;
    }

    ret = TRUE;
    DPRINT1("[DWM] NtUserDwmStartup: success\n");

leave:
    UserLeave();
    return ret;
}

BOOL
APIENTRY
NtUserDwmShutdown(VOID)
{
    NTSTATUS Status;
    BOOL ret = FALSE;

    DPRINT1("NtUserDwmShutdown\n");

    UserEnterExclusive();

    if (!gpepDwm)
    {
        DPRINT1("[DWM] NtUserDwmShutdown: no DWM registered\n");
        goto leave;
    }

    if (PsGetCurrentProcess() != gpepDwm)
    {
        DPRINT1("[DWM] NtUserDwmShutdown: wrong process cur=%p expected=%p\n",
                PsGetCurrentProcess(), gpepDwm);
        EngSetLastError(ERROR_ACCESS_DENIED);
        goto leave;
    }

    Status = IntXxxDwmShutdown();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("[DWM] NtUserDwmShutdown: IntXxxDwmShutdown failed %08lX\n", Status);
        EngSetLastError(RtlNtStatusToDosError(Status));
        goto leave;
    }

    if (gpvDwmApiPort)
    {
        ObDereferenceObject(gpvDwmApiPort);
        gpvDwmApiPort = NULL;
    }
    gpepDwm = NULL;
    ret = TRUE;
    DPRINT1("[DWM] NtUserDwmShutdown: success\n");

leave:
    UserLeave();
    return ret;
}

/*
 * Longhorn 5048 (win32k): only PsGetCurrentProcess() == gpepDwm; maps NTSTATUS to last error; returns BOOL (NT_SUCCESS).
 */
BOOL
APIENTRY
NtUserDwmGetSurfaceData(_In_opt_ HWND hwnd, _In_ PVOID pSurfaceDataOut)
{
    NTSTATUS Status;
    HDEV hdev;
    BOOL ret = FALSE;

    UserEnterExclusive();

    if (!pSurfaceDataOut)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto leave;
    }

    if (PsGetCurrentProcess() != gpepDwm)
    {
        EngSetLastError(ERROR_ACCESS_DENIED);
        goto leave;
    }

    if (!gpmdev || !gpmdev->ppdevGlobal)
    {
        EngSetLastError(ERROR_NOT_READY);
        goto leave;
    }

    hdev = (HDEV)gpmdev->ppdevGlobal;
    Status = GreDwmGetSurfaceData(hdev, hwnd, pSurfaceDataOut);
    EngSetLastError(RtlNtStatusToDosError(Status));
    ret = NT_SUCCESS(Status) ? TRUE : FALSE;

leave:
    UserLeave();
    return ret;
}

INT
APIENTRY
NtUserSetWindowRgnEx(
    _In_ HWND hWnd,
    _In_opt_ HRGN hRgn,
    _In_ DWORD dwFlags)
{
    TRACE("NtUserSetWindowRgnEx: (%p %p %#lx)\n", hWnd, hRgn, dwFlags);
    return NtUserSetWindowRgn(hWnd, hRgn, (dwFlags & 1u) ? TRUE : FALSE);
}

/*
 * Longhorn 5048 (NtUserUpdateWindowTransform):
 *  - Only the registered DWM (milcore) process may call.
 *  - Third argument must be 1 (uDWM always passes 1).
 *  - Window must be on the input desktop when it is known.
 *  - Optional user pointer to 64 bytes replaces kernel-side MIL transform for the WND.
 */
BOOL
APIENTRY
NtUserUpdateWindowTransform(
    _In_ HWND hwnd,
    _In_opt_ PVOID pTransform,
    _In_ DWORD dwFlags)
{
    PWND Wnd;
    PVOID NewBuf = NULL;
    BOOL ret = FALSE;

    TRACE("NtUserUpdateWindowTransform: (%p %p %#lx)\n", hwnd, pTransform, dwFlags);

    UserEnterExclusive();

    Wnd = UserGetWindowObject(hwnd);
    if (!Wnd)
    {
        EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
        goto done;
    }

    if (dwFlags != 1)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (gpdeskInputDesktop && Wnd->head.rpdesk != gpdeskInputDesktop)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (!DwmIsDwmClientProcess())
    {
        EngSetLastError(ERROR_ACCESS_DENIED);
        goto done;
    }

    if (Wnd->pMilTransform)
    {
        ExFreePoolWithTag(Wnd->pMilTransform, USERTAG_MILTRANSFORM);
        Wnd->pMilTransform = NULL;
    }

    if (!pTransform)
    {
        ret = TRUE;
        goto done;
    }

    NewBuf = ExAllocatePoolWithTag(PagedPool, MIL_TRANSFORM_SIZE, USERTAG_MILTRANSFORM);
    if (!NewBuf)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto done;
    }

    _SEH2_TRY
    {
        ProbeForRead(pTransform, MIL_TRANSFORM_SIZE, 1);
        RtlCopyMemory(NewBuf, pTransform, MIL_TRANSFORM_SIZE);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(NewBuf, USERTAG_MILTRANSFORM);
        NewBuf = NULL;
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (!NewBuf)
        goto done;

    Wnd->pMilTransform = NewBuf;
    NewBuf = NULL;
    ret = TRUE;

done:
    UserLeave();
    return ret;
}

#define DWM_MIL_OP_HITTEST 24u

#include <pshpack1.h>
typedef struct _DWM_MIL_HITTEST_BODY
{
    ULONG Opcode;
    ULONG Hwnd;
    ULONG Arg2;
    ULONG Arg3;
    ULONG Arg4;
    LONG PtX;
    LONG PtY;
    ULONG ReplyHit;
    ULONG ReplyMilHandled;
} DWM_MIL_HITTEST_BODY;
#include <poppack.h>

C_ASSERT(sizeof(DWM_MIL_HITTEST_BODY) == 36);

NTSTATUS
FASTCALL
IntDwmHitTestQuery(
    _In_ HWND hwnd,
    _In_ ULONG Arg2,
    _In_ LONG PtX,
    _In_ LONG PtY,
    _In_ ULONG Arg3,
    _In_ ULONG Arg4,
    _Out_ PULONG pHitValue,
    _Out_ PULONG pMilHandledNonZero)
{
    NTSTATUS Status;
    UCHAR Raw[256];
    PPORT_MESSAGE H = (PPORT_MESSAGE)Raw;
    DWM_MIL_HITTEST_BODY *Body;

    if (!pHitValue || !pMilHandledNonZero)
        return STATUS_INVALID_PARAMETER;

    *pHitValue = 0;
    *pMilHandledNonZero = 0;

    if (!gpvDwmApiPort || !gfbDwmCompositing)
        return STATUS_DEVICE_NOT_CONNECTED;

    if (sizeof(PORT_MESSAGE) + sizeof(DWM_MIL_HITTEST_BODY) > sizeof(Raw))
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(H, sizeof(PORT_MESSAGE) + sizeof(DWM_MIL_HITTEST_BODY));
    H->u1.s1.DataLength = sizeof(DWM_MIL_HITTEST_BODY);
    H->u1.s1.TotalLength = (CSHORT)(sizeof(PORT_MESSAGE) + sizeof(DWM_MIL_HITTEST_BODY));
    H->u2.s2.Type = (CSHORT)(USHORT)0x8000u;
    H->u2.s2.DataInfoOffset = 0;

    Body = (DWM_MIL_HITTEST_BODY *)((PUCHAR)H + sizeof(PORT_MESSAGE));
    Body->Opcode = DWM_MIL_OP_HITTEST;
    Body->Hwnd = (ULONG)(ULONG_PTR)hwnd;
    Body->Arg2 = Arg2;
    Body->Arg3 = Arg3;
    Body->Arg4 = Arg4;
    Body->PtX = PtX;
    Body->PtY = PtY;
    Body->ReplyHit = 0;
    Body->ReplyMilHandled = 0;

    UserLeave();

    Status = LpcRequestWaitReplyPort(gpvDwmApiPort, H, H);

    UserEnterExclusive();

    if (!NT_SUCCESS(Status))
        return Status;

    *pHitValue = Body->ReplyHit;
    *pMilHandledNonZero = Body->ReplyMilHandled;
    return STATUS_SUCCESS;
}

VOID
FASTCALL
IntDwmSendLpcDatagram(_In_ PPORT_MESSAGE Msg)
{
    PUCHAR pl;
    ULONG op;

    /* Port is set before GreDwmStartup; compositing flag is set after. Startup DCE walk must LPC first. */
    if (!gpvDwmApiPort || !Msg)
    {
        DPRINT1("[DWM] LPC blocked: port=%p msg=%p\n", gpvDwmApiPort, Msg);
        return;
    }

    if (Msg->u1.s1.TotalLength < sizeof(PORT_MESSAGE) ||
        Msg->u1.s1.TotalLength > DWM_LPC_MSG_CAP)
    {
        DPRINT1("[DWM] LPC blocked: bad TotalLength=%u (cap=%u)\n",
                (unsigned)Msg->u1.s1.TotalLength, (unsigned)DWM_LPC_MSG_CAP);
        return;
    }

    pl = (PUCHAR)Msg + sizeof(PORT_MESSAGE);
    op = *(PULONG)pl;
    DPRINT1("[DWM] LPC LpcRequestPort op=%lu TotalLength=%u port=%p\n",
            op, (unsigned)Msg->u1.s1.TotalLength, gpvDwmApiPort);
    LpcRequestPort(gpvDwmApiPort, Msg);
}
