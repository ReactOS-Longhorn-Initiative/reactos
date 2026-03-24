/*
 * Internal DWM / MIL bring-up state (Vista Longhorn 5112-style).
 */
#pragma once

struct _EPROCESS;
struct _PORT_MESSAGE;
struct _WND;
struct _SURFACE;

extern BOOLEAN gfbDwmCompositing;
extern struct _EPROCESS *gpepDwm;

BOOLEAN FASTCALL DwmIsDwmClientProcess(VOID);

VOID FASTCALL IntDwmSendLpcDatagram(_In_ struct _PORT_MESSAGE *Msg);

/*
 * Longhorn 5112 win32k DwmHitTestQuery: MIL op 24, LPC Type 0x8000, wait/reply (see dwm.c).
 * Primary path: xxxDCEWindowHitTest2 (DCE args in Arg2/Arg3/Arg4). WinPos mirrors this in
 * co_WinPosSearchChildren with Arg2/3/4 = 0 when no DCE context is available.
 * On success: *pHitValue = mil hit code; *pMilHandledNonZero != 0 means skip WM_NCHITTEST.
 */
NTSTATUS FASTCALL IntDwmHitTestQuery(
    _In_ HWND hwnd,
    _In_ ULONG Arg2,
    _In_ LONG PtX,
    _In_ LONG PtY,
    _In_ ULONG Arg3,
    _In_ ULONG Arg4,
    _Out_ PULONG pHitValue,
    _Out_ PULONG pMilHandledNonZero);

NTSTATUS
APIENTRY
IntGreDwmResolveSurface(
    _In_ HDEV hdev,
    _In_opt_ HWND hwnd,
    _Out_ struct _WND **ppWnd,
    _Out_ struct _SURFACE **ppsurf);
