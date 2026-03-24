#pragma once

#include "dce.h"
#include "dwm.h"

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
/* While compositing: Longhorn 5112-style ResetRedirectedWindows — unbind active redirect DCs; if bit
 * depth changed, free redirect bitmaps under the desktop. Then DwmPowerNotification (op 16)
 * with argument CdsFlags (5112 packs one ULONG in the power slot). */
#define DWM_DISPLAY_HINT_MONITOR    0x10000000u
#define DWM_DISPLAY_HINT_VIDRESCAN  0x20000000u
#define DWM_DISPLAY_HINT_SESSION    0x40000000u
#define DWM_DISPLAY_HINT_METRICS    0x80000000u
VOID FASTCALL IntDwmNotifyDisplaySettingsChanged(_In_ DWORD CdsFlags, _In_ ULONG OldBitCount);

VOID FASTCALL IntDwmSendForegroundInputLpc(VOID);
VOID FASTCALL IntDwmSendDesktopSwitchLpc(VOID);
VOID FASTCALL IntDwmSendShellWindowLpc(_In_ HWND hwndShell);

/* WS_DISABLED / minimize / maximize changed without a full SetWindowPos path. */
VOID FASTCALL IntDwmOnNonClientStateHintChanged(_In_ PWND Wnd);

VOID FASTCALL IntDwmFreeAllRedirectBitmaps(VOID);
VOID FASTCALL IntDwmOnWindowCaptionChanged(_In_ PWND Wnd);

struct _CLS;
VOID FASTCALL IntDwmOnWindowIconChanged(_In_ PWND Wnd);
VOID FASTCALL IntDwmNotifyClassIconsChanged(_In_ struct _CLS *pcls);
VOID FASTCALL IntDwmNotifyWindowStyleChanged(_In_ PWND Wnd, _In_ LONG Idx, _In_ ULONG OldVal, _In_ ULONG NewVal);
VOID FASTCALL IntDwmNotifyChildParentChanged(_In_ PWND Wnd, _In_ PWND WndNewParent);

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

/* Longhorn 5112 DwmTopLevelCreate / DwmTopLevelUpdate LPC bodies (used from GreDwmStartup + runtime). */
VOID FASTCALL IntDwmTopLevelCreate(_In_ PWND Wnd, _In_opt_ PRECTL prcIn, _In_ ULONG HintFlagsAnd1);
VOID FASTCALL IntDwmTopLevelUpdate(_In_ PWND Wnd, _In_ ULONG UpdateArg, _In_opt_ PRECTL prcOptional);
VOID FASTCALL IntDwmGreStartupWalkDceList(_In_ HDEV hdev);

/* DwmNotifyChildrenAddRemove: batch child op 0x11 / 18 under the desktop (5112 sprite.c).
 * fIgnoreCompositingGate: TRUE for the pre-GreDwmStartup notify in xxxDwmStartup. */
VOID FASTCALL IntDwmNotifyDesktopChildrenAddRemove(_In_ BOOLEAN BooleanAdd,
                                                   _In_ BOOLEAN fIgnoreCompositingGate);
