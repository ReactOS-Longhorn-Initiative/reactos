#pragma once

#include "dce.h"

struct _DC;
typedef struct _DC *PDC;

struct _WND;
typedef struct _WND *PWND;
struct _SURFACE;
typedef struct _SURFACE *PSURFACE;

VOID FASTCALL IntDwmOnWindowCreated(_In_ PWND Wnd);
VOID FASTCALL IntDwmOnWindowDestroyed(_In_ PWND Wnd);
VOID FASTCALL IntDwmOnZorderChanged(_In_ PWND Wnd, _In_ HWND hwndInsertAfter);
/* After SetWindowPos commits rcWindow/rcClient: notify milcore + drop stale redirect surfaces. */
VOID FASTCALL IntDwmOnWindowPosChanged(_In_ PWND Wnd, _In_ UINT SwpFlags,
                                       _In_ PRECTL prcOldClient);
/* Window region / NC shape changed without client rect change (e.g. SetWindowRgn). */
VOID FASTCALL IntDwmOnWindowShapeChanged(_In_ PWND Wnd);
/* Layered alpha/colorkey or UpdateLayeredWindow presentation changed. */
VOID FASTCALL IntDwmOnLayeredPresentationChanged(_In_ PWND Wnd);
/* After applied display mode change while compositing: LPC + unbind active redirect DCs.
 * OldBitCount: gpsi->BitCount before the switch; used to drop redirect bitmaps if depth changed.
 * CdsFlags: CDS_* from caller; OR with DWM_DISPLAY_HINT_* so milcore can classify (5048+). */
#define DWM_DISPLAY_HINT_MONITOR    0x10000000u
#define DWM_DISPLAY_HINT_VIDRESCAN  0x20000000u
#define DWM_DISPLAY_HINT_SESSION    0x40000000u
#define DWM_DISPLAY_HINT_METRICS    0x80000000u
VOID FASTCALL IntDwmNotifyDisplaySettingsChanged(_In_ DWORD CdsFlags, _In_ ULONG OldBitCount);

/* Foreground message queue / active+focus HWND roots (opcode 13) — call after input desktop changes. */
VOID FASTCALL IntDwmSendForegroundInputLpc(VOID);

/* WS_DISABLED / minimize / maximize changed without a full SetWindowPos path. */
VOID FASTCALL IntDwmOnNonClientStateHintChanged(_In_ PWND Wnd);

VOID FASTCALL IntDwmFreeAllRedirectBitmaps(VOID);
VOID FASTCALL IntDwmOnWindowCaptionChanged(_In_ PWND Wnd);

VOID FASTCALL IntDwmOnOwnerChanged(_In_ PWND Wnd);
VOID FASTCALL IntDwmOnExStyleLayeredToggle(_In_ PWND Wnd);
VOID FASTCALL IntDwmOnVisibleStyleChanged(_In_ PWND Wnd, _In_ BOOL NowVisible);

NTSTATUS FASTCALL IntDwmPrepareRedirectSurface(_In_ HDEV hdev, _In_ PWND Wnd);
VOID FASTCALL IntDwmFreeRedirectSurface(_In_ PWND Wnd);

/* After GdiSelectVisRgn (client-local prgnVis), route GDI output to hbmDwmRedirect until ReleaseDC. */
VOID FASTCALL IntDwmBindRedirectDcLocked(_Inout_ PDC pdc, _Inout_ PDCE dce, _In_opt_ PWND Wnd, _In_ ULONG DcxFlags);
VOID FASTCALL IntDwmUnbindRedirectDcLocked(_Inout_ PDC pdc, _Inout_ PDCE dce);
VOID FASTCALL IntDwmUnbindRedirectDc(_Inout_ PDCE dce);
VOID FASTCALL IntDwmUnbindRedirectForWindow(_In_ PWND Wnd);
VOID FASTCALL IntDwmUnbindAllActiveRedirectDcs(VOID);

/* Longhorn 5048 DwmTopLevelCreate / DwmTopLevelUpdate LPC bodies (used from GreDwmStartup + runtime). */
VOID FASTCALL IntDwmTopLevelCreate(_In_ PWND Wnd, _In_opt_ PRECTL prcIn, _In_ ULONG HintFlagsAnd1);
VOID FASTCALL IntDwmTopLevelUpdate(_In_ PWND Wnd, _In_ ULONG UpdateArg, _In_opt_ PRECTL prcOptional);
VOID FASTCALL IntDwmGreStartupWalkDceList(_In_ HDEV hdev);
