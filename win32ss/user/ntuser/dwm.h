/*
 * PROJECT:     ReactOS Win32k subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Desktop composition redirection - win32k -> DWM producer side
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/*
 * Phase 1 of the DWM bring-up: SPRITE GEOMETRY ONLY.
 *
 * win32k tells DWM where every top-level window is, how big it is, whether it
 * is visible and how it is stacked. It does NOT yet hand over window CONTENT --
 * that needs redirection surfaces (WS_EX_REDIRECTED backed by a real bitmap
 * instead of a clipped display DC), which is a separate piece of work.
 *
 * This is not a shortcut we invented. Vista has the same split: dwm.exe calls
 * DwmStartRedirection(!fStructuralMode), and "structural mode" is exactly this
 * -- geometry without content. The sprite commands carry no surface handle
 * (see MILCMD_DWM_REDIRECTION_CREATESPRITE); content arrives separately and
 * pull-based through CompositedWindow::GetGDISurface.
 *
 * With geometry alone the compositor can build its visual tree and draw window
 * CHROME -- frames, glass, captions, shadows -- because uDWM sources those from
 * DWMWindow.msstyles, not from the window. Interiors stay empty until phase 2.
 */

/* ---------------------------------------------------------------------------
 * Wire format
 *
 * Recovered from dwmredir's consumer side; see the reference notes in
 * DarkFiresReactOSModules/dwm/dwmredir/{RedirCommands.hpp,stubs.cpp}.
 *
 *   payload      = (BYTE*)PORT_MESSAGE + sizeof(PORT_MESSAGE)
 *   payload[0]   = the RWM opcode (dword)
 *   DataLength  >= the per-opcode required size, or dwmredir drops the message
 *   MessageId    = sequence number, echoed into its traces
 *   u2.s2.Type   = kind | kernel-only bit:
 *                    kind       = Type & 0x7FFF   (1 = sync request, 3 = async)
 *                    kernelOnly = Type & 0xFFFF8000
 * ------------------------------------------------------------------------- */

#define DWM_MSG_KIND_SYNC       1
#define DWM_MSG_KIND_ASYNC      3
#define DWM_MSG_KERNELONLY      0x8000

/* RWM redirection opcodes. 1:1 with _RWM_REDIR_COMMANDS in
 * DarkFiresReactOSModules/dwm/dwmredir/CWin32Redirection.hpp, which recovered
 * them from the third argument of each CRedirTracer::TraceDwmUpdate call. */
#define RWMCMD_REDIR_CREATESPRITE           0x40000002
#define RWMCMD_REDIR_DESTROYSPRITE          0x40000003
#define RWMCMD_REDIR_DIRTYSPRITE            0x40000004
#define RWMCMD_REDIR_ZORDERSPRITE           0x40000005
#define RWMCMD_REDIR_UPDATESPRITE           0x40000006
#define RWMCMD_REDIR_SHOWSPRITE             0x40000007
#define RWMCMD_REDIR_NOTIFYACTIVATIONCHANGE 0x4000000B

/*
 * REGISTERS A WINDOW, and it must precede that window's CREATESPRITE.
 *
 * dwmredir's CMilWindowManager::NotifyChildCreate (dwmredir.dll.c:17594) is
 * the only caller of AddWindowContextForWindow in the whole binary -- nothing
 * else puts an hwnd into the context map. CreateSprite (:17021) then requires
 * that entry to exist:
 *
 *     if (hwnd)  LookupContext(hwnd, &pwnd);   // must ALREADY be there
 *     else       CMilWindowContext::Create(&pwnd);
 *
 * so sending CREATESPRITE for an unregistered window fails at the lookup with
 * E_HANDLE, which is what every sprite did before this opcode existed.
 *
 * The name says "child" and it means "window": a NULL hwndParent is the
 * TOP-LEVEL case, and it is the branch that flags the context top-level and
 * links it under the root.
 */
#define RWMCMD_REDIR_NOTIFYCHILDCREATE      0x4000000F

/*
 * tagMINIWINDOWINFO - 12 dwords. UpdateSprite carries only the first ten;
 * CreateSprite additionally supplies fDpiAware at mini-info slot 10.
 */
typedef struct _DWM_MINIWINDOWINFO
{
    RECT   rcWindow;
    RECT   rcClient;
    UINT32 dwStyle;
    UINT32 dwExStyle;
    UINT32 fDpiAware;
    UINT32 Unknown11;
} DWM_MINIWINDOWINFO, *PDWM_MINIWINDOWINFO;

#include <pshpack1.h>

typedef struct _DWM_CMD_CREATESPRITE
{
    UINT32             Type;
    UINT32             hSprite;
    UINT32             hwnd;
    RECT               rcWindow;
    UINT32             fVisible;
    DWM_MINIWINDOWINFO MiniInfo;
} DWM_CMD_CREATESPRITE;

/*
 * MILCMD_DWM_REDIRECTION_NOTIFYCHILDCREATE, 40 bytes.
 *
 * Layout from dwmredir's own RedirCommands.hpp, which recovered it from the
 * Vista read sites: {Type, hwnd, hwndParent, dwStyle, dwExStyle, RECT, dwClsStyle}.
 * The dispatcher validates cb >= 40 before casting, so the size is load-bearing.
 */
typedef struct _DWM_CMD_NOTIFYCHILDCREATE
{
    UINT32 Type;
    UINT32 hwnd;
    UINT32 hwndParent;      /* 0 == top level, and that is the branch that matters */
    UINT32 dwStyle;
    UINT32 dwExStyle;
    RECT   rcWindow;
    UINT32 dwClsStyle;
} DWM_CMD_NOTIFYCHILDCREATE;

typedef struct _DWM_CMD_DESTROYSPRITE
{
    UINT32 Type;
    UINT32 hSprite;
} DWM_CMD_DESTROYSPRITE;

typedef struct _DWM_CMD_SHOWSPRITE
{
    UINT32 Type;
    UINT32 hSprite;
    UINT32 fShow;
} DWM_CMD_SHOWSPRITE;

/*
 * NOTE: keyed on HWND, not hSprite -- unlike every sprite command above.
 * dwmredir looks the window up in its own table (CWin32Redirection::
 * NotifyActivationChange), so sending a sprite id here silently matches
 * nothing.
 */
typedef struct _DWM_CMD_ACTIVATIONCHANGE
{
    UINT32 Type;
    UINT32 hwnd;
    UINT32 fActive;
} DWM_CMD_ACTIVATIONCHANGE;

typedef struct _DWM_CMD_ZORDERSPRITE
{
    UINT32 Type;
    UINT32 hSprite;
    UINT32 hSpriteInsertAfter;
} DWM_CMD_ZORDERSPRITE;

typedef struct _DWM_CMD_UPDATESPRITE
{
    UINT32 Type;
    UINT32 hSprite;
    UINT32 Flags;
    UINT32 fHasMiniInfo;
    UINT32 MiniInfo[10];   /* tagMINIWINDOWINFO[0..9] */
    UINT32 Attributes[8];  /* _SpriteCachedAttributes */
} DWM_CMD_UPDATESPRITE;

#include <poppack.h>

/* ---------------------------------------------------------------------------
 * Why a worker thread carries these, and not the calling thread
 *
 * dwmredir refuses any redirection command that did not come from kernel mode:
 * it routes on `kernelOnly = Type & 0xFFFF8000`, and that bit is NT's standard
 * LPC_KERNELMODE_MESSAGE (0x8000, xdk/ntifs.template.h). ntoskrnl only sets it
 * for us if the SENDING THREAD is in kernel mode -- ntoskrnl/lpc/send.c:48:
 *
 *     if ((PreviousMode == KernelMode) &&
 *         (LpcMessage->u2.s2.Type & LPC_KERNELMODE_MESSAGE))
 *         MessageType |= LPC_KERNELMODE_MESSAGE;
 *
 * and _KeGetPreviousMode() is the THREAD's PreviousMode. Our emitters run
 * inside win32k during a syscall made by an ordinary application thread, so
 * PreviousMode is UserMode and the bit is stripped. dwmredir then hands the
 * message to its user-mode dispatcher, which has no arm for the sprite
 * opcodes, returns E_NOTIMPL, and drops it -- silently, with nothing logged
 * anywhere. Identical in appearance to the hooks never firing at all.
 *
 * So commands are queued on the caller's thread and posted by a system thread
 * created with PsCreateSystemThread, which is in kernel mode by construction.
 * That also moves any blocking on a full port queue off the application
 * thread, which is worth having on its own.
 * ------------------------------------------------------------------------- */

#define DWM_QUEUE_ENTRIES   256
#define DWM_QUEUE_ENTRYSIZE 96      /* >= sizeof(DWM_CMD_UPDATESPRITE) */

/* ------------------------------------------------------------------------- */

extern PVOID gpDwmApiPort;
extern BOOL  gbDwmRedirectionActive;

/* Cheap gate for the lifecycle hooks: no port, no work. */
FORCEINLINE BOOL IntDwmIsActive(VOID)
{
    return (gpDwmApiPort != NULL) && gbDwmRedirectionActive;
}

NTSTATUS IntDwmRegisterSessionPort(HANDLE hPort);
VOID     IntDwmUnregisterSessionPort(VOID);
NTSTATUS IntDwmStartRedirection(BOOL fRedirectContent);
VOID     IntDwmStopRedirection(VOID);

/* Lifecycle emitters. Each is a no-op unless IntDwmIsActive(). */
VOID IntDwmCreateSprite(PWND Wnd);
VOID IntDwmDestroySprite(PWND Wnd);
VOID IntDwmShowSprite(PWND Wnd, BOOL fShow);
VOID IntDwmUpdateSprite(PWND Wnd);
VOID IntDwmZorderSprite(PWND Wnd);
VOID IntDwmActivationChange(PWND Wnd, BOOL fActive);

/* EOF */
