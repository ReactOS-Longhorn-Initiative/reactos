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
 * LINKS A REGISTERED WINDOW INTO THE COMPOSITOR'S TREE.
 *
 * NOTIFYCHILDCREATE registers a window; this is what gives it a position.
 * Vista's LinkWindow (win32k.sys.c:152594) emits it whenever a window is
 * linked into the sibling list:
 *
 *     v13 = HWInsertAfter(...);
 *     DwmChildLink(pwnd->head.h, parent, v13);
 *
 * and _DwmStartRedirection's enumeration sends one per window in a second
 * pass, AFTER every window has been created:
 *
 *     DwmNotifyChildrenAddRemove(1):
 *         DwmNotifyChildrenCreateDestroy(1);   // register everything
 *         DwmNotifyChildrenLinkUnlink(1);      // then link everything
 *
 * -- the two passes are separate so a link can always resolve both of its
 * endpoints, which a single interleaved pass cannot guarantee.
 */
#define RWMCMD_REDIR_NOTIFYCHILDLINK        0x40000010

/*
 * UNREGISTERS a window -- the counterpart to NOTIFYCHILDCREATE.
 *
 * Vista sends it from xxxFreeWindow (win32k.sys.c:174296) per window, and
 * from the teardown pass of DwmNotifyChildrenAddRemove(0), which runs the
 * enumeration in REVERSE: unlink everything, then destroy everything.
 *
 * Destroying the sprite is not enough on its own. A sprite is a composition
 * object; the CONTEXT is what holds the hwnd registration, and dwmredir keeps
 * it until told otherwise. Leaving contexts behind across a dwm.exe restart
 * means the next registration pass finds every window "already registered"
 * and keeps a context built around a window that may no longer exist.
 */
#define RWMCMD_REDIR_NOTIFYCHILDDESTROY     0x40000012

/*
 * MILCMD_DWM_REDIRECTION_NOTIFYCHILDMOVESIZE, 60 bytes.
 *
 * THIS IS WHAT GIVES A WINDOW ITS CHROME. dwmredir's handler
 * (CMilWindowManager::NotifyChildMoveSize, dwmredir.dll.c:18087) derives the
 * content margins from it as rcContent minus rcWindow -- the non-client
 * thickness -- and feeds them to UpdateContentMargins. uDWM's
 * CTopLevelWindow::GetCurrentStyle then tests that top margin against the
 * minimum caption height, and only sets the 0x2/0x4 style bits (the ones
 * fChromeWanted reads) when the margins are non-zero.
 *
 * Never emitting it left every window with {0,0,0,0} margins, derived style
 * 0x1, and all 17 mesh-image slots NULL -- no frames, no caption, no buttons,
 * for the whole session, with the entire art pipeline loaded and idle behind
 * the gate.
 */
#define RWMCMD_REDIR_NOTIFYCHILDMOVESIZE    0x40000013

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

/*
 * MILCMD_DWM_REDIRECTION_NOTIFYCHILDLINK, 16 bytes.
 * dwmredir.dll.c:16111, and the argument list of Vista's
 * DwmChildLink(hwnd, hwndParent, hwndInsertAfter).
 *
 * These are HWNDs, not sprite handles -- unlike ZORDERSPRITE, which carries
 * sprite ids. The two describe the same ordering from different sides.
 */
/*
 * The 15 slots dwmredir's NotifyChildMoveSize reads; see the opcode note.
 *
 * rcContent is a THIRD rect, distinct from rcClient in Vista: the handler
 * stores rcWindow and rcClient onto the context but computes the margins from
 * rcContent. We send the client rect for both, because win32k has exactly one
 * client rect to offer and the margin subtraction is the consumer that
 * matters. Stated as a deviation rather than hidden: if a case turns up where
 * Vista's content rect differs from its client rect, this is where it lands.
 */
typedef struct _DWM_CMD_NOTIFYCHILDMOVESIZE
{
    UINT32 Type;        /* slot 0      */
    UINT32 hwnd;        /* slot 1      */
    RECT   rcWindow;    /* slots 2-5   */
    RECT   rcClient;    /* slots 6-9   */
    RECT   rcContent;   /* slots 10-13 */
    UINT32 cBorders;    /* slot 14     */
} DWM_CMD_NOTIFYCHILDMOVESIZE;

typedef struct _DWM_CMD_NOTIFYCHILDLINK
{
    UINT32 Type;
    UINT32 hwnd;
    UINT32 hwndParent;
    UINT32 hwndInsertAfter;   /* 0 == front of the sibling list */
} DWM_CMD_NOTIFYCHILDLINK;

/* MILCMD_DWM_REDIRECTION_NOTIFYCHILDDESTROY, 8 bytes (dwmredir dispatches
 * opcode 0x40000012 with cb 8). Keyed on the HWND, not a sprite. */
typedef struct _DWM_CMD_NOTIFYCHILDDESTROY
{
    UINT32 Type;
    UINT32 hwnd;
} DWM_CMD_NOTIFYCHILDDESTROY;

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

/*
 * Raised from 256 when the startup enumeration grew from "top-level windows"
 * to "every window on the desktop, registered then linked" -- two passes over
 * a whole desktop, so several commands per window rather than one.
 *
 * The queue DROPS THE NEWEST when full (IntDwmPost) rather than overwriting,
 * because it is an ordered command stream; a drop is counted and reported, so
 * an undersized queue shows up as a number instead of as windows that
 * silently never appear. If that counter is ever non-zero, this is the knob.
 */
#define DWM_QUEUE_ENTRIES   1024
#define DWM_QUEUE_ENTRYSIZE 96      /* >= sizeof(DWM_CMD_UPDATESPRITE) */

/* ------------------------------------------------------------------------- */

extern PVOID gpDwmApiPort;

/* The compositor's process, captured at port registration. Weak pointer, used
 * only so IntDwmSetRedirectedWindow can refuse to redirect DWM's own windows;
 * see the note beside its definition in dwm.c. */
extern PEPROCESS gpepDwm;
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
/*
 * Registers a window with dwmredir. Separate from IntDwmCreateSprite: every
 * window on the desktop is registered, only top-level ones get sprites.
 * MUST be called before the window is linked -- see co_UserCreateWindowEx.
 */
VOID IntDwmNotifyChildCreate(PWND Wnd);

VOID IntDwmCreateSprite(PWND Wnd);
VOID IntDwmDestroySprite(PWND Wnd);

/* Unregisters a window. Counterpart to IntDwmNotifyChildCreate; call it
 * wherever the sprite is torn down. */
VOID IntDwmNotifyChildDestroy(PWND Wnd);
VOID IntDwmShowSprite(PWND Wnd, BOOL fShow);
VOID IntDwmUpdateSprite(PWND Wnd);

/* 0x40000013. Delivers the non-client thickness, which is what opens the
 * chrome gate in uDWM -- see the opcode note above. */
VOID IntDwmNotifyChildMoveSize(PWND Wnd);
VOID IntDwmZorderSprite(PWND Wnd);
VOID IntDwmActivationChange(PWND Wnd, BOOL fActive);

/* Enumerate every window on the active desktop, parents before children.
 * Shared with dwmredir.c, which needs the same walk for the bitmap pass. */
VOID IntDwmForEachOnDesktop(VOID (*pfn)(PWND));

/* ------------------------------------------------------------------------- */
/*  Content redirection -- see ntuser/dwmredir.c                              */
/* ------------------------------------------------------------------------- */

/* WND::DwmRedirFlags. Vista reads the same word through _GetRedirectionFlags. */
#define DWM_REDIRF_REDIRECTED   0x00000001

/* Vista _gfStructuralRedirection: TRUE = geometry only, no content capture. */
extern BOOL gfStructuralRedirection;

HBITMAP FASTCALL UserGetRedirectionBitmap(PWND Wnd);
VOID    FASTCALL UserSetRedirectionBitmap(PWND Wnd, HBITMAP hbm);
HBITMAP FASTCALL UserGetOldRedirectionBitmap(PWND Wnd);
VOID    FASTCALL UserSetOldRedirectionBitmap(PWND Wnd, HBITMAP hbm);
UINT32  FASTCALL UserGetRedirectionFlags(PWND Wnd);

/* TRUE when this window's DCs must be pointed at its redirection bitmap. */
BOOL    FASTCALL UserIsWindowRedirected(PWND Wnd);
PWND    FASTCALL UserGetRedirectionTarget(PWND Wnd);
VOID    FASTCALL UserGetRedirectedWindowOrigin(PWND Wnd, PPOINT ppt);

HBITMAP FASTCALL IntDwmCreateRedirectionBitmap(PWND Wnd);
HBITMAP FASTCALL IntDwmRecreateRedirectionBitmap(PWND Wnd);
VOID    FASTCALL IntDwmRemoveRedirectionBitmap(PWND Wnd);

BOOL    FASTCALL IntDwmSetRedirectedWindow(PWND Wnd);
VOID    FASTCALL IntDwmUnsetRedirectedWindow(PWND Wnd);
VOID    FASTCALL IntDwmResetRedirectedWindows(VOID);
VOID    FASTCALL IntDwmResetRedirectedWindowSurfaces(VOID);

VOID    FASTCALL IntDwmRedirOnWindowCreated(PWND Wnd);
VOID    FASTCALL IntDwmRedirOnWindowDestroyed(PWND Wnd);
VOID    FASTCALL IntDwmRedirOnWindowSized(PWND Wnd);

/*
 * tagDWMSURFACEDATA -- the win32k -> dwmredir contract, seven DWORDs.
 * Consumed by CMilWindowContext::GetNewSurfaceData; the authoritative
 * commentary on each slot is in dwmredir/RedirCommands.hpp, which recovered
 * it from Vista's read sites. Keep the two in step.
 */
typedef struct _DWM_SURFACE_DATA
{
    HANDLE hSection;        /* slot 0 -- mapped then closed by dwmredir */
    UINT32 nWidth;          /* slot 1 */
    UINT32 nHeight;         /* slot 2 */
    UINT32 dwGdiFormat;     /* slot 3 -- BI_* compression */
    UINT32 dwGdiFormatAux;  /* slot 4 -- bits per pixel */
    UINT32 dwStride;        /* slot 5 */
    UINT32 dwFlags;         /* slot 6 -- high byte == 1 means "has alpha" */
} DWM_SURFACE_DATA, *PDWM_SURFACE_DATA;

/* windc.c: retarget DCs that were handed out before the mode changed. */
VOID    FASTCALL IntDwmUpdateRedirectedDCs(VOID);

/* EOF */
