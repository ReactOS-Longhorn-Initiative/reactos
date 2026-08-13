/*
 * PROJECT:     ReactOS Win32k subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Desktop composition redirection - win32k -> DWM producer side
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * See dwm.h for the phase-1 scope statement and the wire format.
 *
 * The LPC mechanics here are modelled directly on csr.c, which has been doing
 * kernel-mode LPC to CSRSS for years: reference the caller's port handle with
 * LpcPortObjectType, keep the object, and LpcRequestPort datagrams at it. Vista
 * hands win32k a port HANDLE too -- dwm.exe's call site is literally
 * `RegisterSessionPort(m_hPort)` (dwm.exe.c:9451) -- so the shape matches 1:1.
 */

#include <win32k.h>
#include "dwm.h"

DBG_DEFAULT_CHANNEL(UserDwm);

/* The DWM session port, referenced from the handle dwm.exe passed us. */
PVOID gpDwmApiPort = NULL;

/* Set by NtUserDwmStartRedirection / cleared by NtUserDwmStopRedirection. */
BOOL gbDwmRedirectionActive = FALSE;

/*
 * Vista: DwmStartRedirection(!fStructuralMode). FALSE means structural mode --
 * geometry only, no content redirection. Phase 1 can only honour FALSE; we
 * accept TRUE but do not yet act on it, because the redirection surfaces it
 * asks for do not exist. Recorded rather than rejected so the caller sees the
 * same success Vista gives it.
 */
static BOOL gbDwmRedirectContent = FALSE;

/* Sequence number stamped into PORT_MESSAGE.MessageId; dwmredir echoes it. */
static LONG glDwmSequence = 0;

/* ---------------------------------------------------------------------------
 * Command queue + posting thread. See the rationale in dwm.h -- the short
 * version is that LPC only stamps LPC_KERNELMODE_MESSAGE when the SENDING
 * thread is in kernel mode, and our emitters run on application threads.
 * ------------------------------------------------------------------------- */

typedef struct _DWM_QUEUE_ENTRY
{
    USHORT cbPayload;
    BYTE   Payload[DWM_QUEUE_ENTRYSIZE];
} DWM_QUEUE_ENTRY;

static DWM_QUEUE_ENTRY gDwmQueue[DWM_QUEUE_ENTRIES];
static ULONG           gDwmQueueHead = 0;   /* producer writes here */
static ULONG           gDwmQueueTail = 0;   /* worker reads here    */
static KSPIN_LOCK      gDwmQueueLock;
static KEVENT          gDwmQueueEvent;
static PETHREAD        gpDwmWorkerThread = NULL;
static volatile LONG   glDwmWorkerStop   = 0;
static BOOL            gbDwmQueueInit    = FALSE;

/* Count of commands dropped because the queue was full. Reported rather than
 * hidden: a silent drop here looks exactly like a missing hook. */
static LONG glDwmDropped = 0;

static NTSTATUS IntDwmStartWorker(VOID);
static VOID     IntDwmStopWorker(VOID);

/*
 * HSPRITE allocation.
 *
 * Vista keeps a real sprite object per redirected window. Phase 1 needs only a
 * stable non-zero token that round-trips through dwmredir's sprite lookup
 * table, so a monotonic counter stored on the window is enough. It deliberately
 * never reuses a value: a stale DestroySprite arriving after a new window took
 * the same id would tear down the wrong visual, and the counter is cheaper than
 * proving that cannot happen.
 */
static LONG glDwmNextSprite = 0;

/* ---------------------------------------------------------------------------
 * Registration
 * ------------------------------------------------------------------------- */

NTSTATUS
IntDwmRegisterSessionPort(HANDLE hPort)
{
    NTSTATUS Status;
    PVOID PortObject;

    if (gpDwmApiPort != NULL)
    {
        ERR("DWM session port already registered\n");
        return STATUS_PORT_ALREADY_SET;
    }

    Status = ObReferenceObjectByHandle(hPort,
                                       0,
                                       LpcPortObjectType,
                                       UserMode,
                                       &PortObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to reference DWM session port: 0x%lx\n", Status);
        return Status;
    }

    gpDwmApiPort = PortObject;

    Status = IntDwmStartWorker();
    if (!NT_SUCCESS(Status))
    {
        /* Without a worker nothing can ever be posted. Fail loudly here rather
         * than accept the port and then silently emit nothing. */
        ObDereferenceObject(gpDwmApiPort);
        gpDwmApiPort = NULL;
        return Status;
    }

    TRACE("DWM session port registered (%p)\n", gpDwmApiPort);
    return STATUS_SUCCESS;
}

VOID
IntDwmUnregisterSessionPort(VOID)
{
    /*
     * Full stop, not just a flag clear: this has to drop every sprite id too,
     * or a later dwm.exe re-registering finds every window already "having" a
     * sprite and creates none. Safe if the port is already dead -- IntDwmPost
     * fails harmlessly and latches itself off.
     */
    if (gbDwmRedirectionActive)
        IntDwmStopRedirection();

    gbDwmRedirectionActive = FALSE;

    /* Before the deref: a worker still inside LpcRequestPort would be holding
     * a pointer to an object we are about to release. */
    IntDwmStopWorker();

    if (gpDwmApiPort)
    {
        ObDereferenceObject(gpDwmApiPort);
        gpDwmApiPort = NULL;
    }
}

/*
 * Walk every top-level window, front to back.
 *
 * The desktop's child list IS the top-level window list, and it is ordered
 * front-to-back -- which is also the order sprites have to be created in for
 * ZorderSprite's "insert after" to name a sprite that already exists.
 *
 * Callers hold the USER lock exclusively (both reach here from a syscall that
 * took it), so the list is stable for the duration of the walk.
 */
static VOID
IntDwmForEachTopLevel(VOID (*pfn)(PWND))
{
    PWND Desktop = UserGetDesktopWindow();
    PWND Wnd;

    if (Desktop == NULL)
    {
        ERR("DWM sweep: no desktop window\n");
        return;
    }

    for (Wnd = Desktop->spwndChild; Wnd != NULL; Wnd = Wnd->spwndNext)
        pfn(Wnd);
}

static VOID
IntDwmSweepCreate(PWND Wnd)
{
    IntDwmCreateSprite(Wnd);

    /* Establish stacking as we go. Front-to-back means the window we name as
     * "insert after" was created on a previous iteration. */
    IntDwmZorderSprite(Wnd);
}

NTSTATUS
IntDwmStartRedirection(BOOL fRedirectContent)
{
    if (gpDwmApiPort == NULL)
        return STATUS_INVALID_PORT_HANDLE;

    gbDwmRedirectContent = fRedirectContent;
    gbDwmRedirectionActive = TRUE;

    TRACE("DWM redirection started (content=%d)\n", fRedirectContent);

    /*
     * Tell DWM about the windows that ALREADY EXIST.
     *
     * Without this the compositor only ever learns about windows created after
     * this instant -- and dwm.exe starts after the desktop, explorer and
     * everything they own, so that is almost nothing. The scene would come up
     * near-empty and read as "the protocol does not work" rather than "nobody
     * ever told it about anything".
     *
     * Must run AFTER gbDwmRedirectionActive is set: every emitter gates on
     * IntDwmIsActive() and would no-op otherwise.
     */
    IntDwmForEachTopLevel(IntDwmSweepCreate);

    return STATUS_SUCCESS;
}

VOID
IntDwmStopRedirection(VOID)
{
    /*
     * Tear the sprites down BEFORE clearing the active flag -- IntDwmPost
     * no-ops once it is clear, so the order here is what decides whether DWM
     * is told at all.
     *
     * Clearing WND::DwmSprite is not optional bookkeeping. IntDwmCreateSprite
     * opens with `if (Wnd->DwmSprite != 0) return;`, so leaving stale ids
     * behind means the next StartRedirection creates NOTHING -- and since
     * every dwm.exe restart is a stop/start cycle, the second run of any test
     * would silently produce an empty scene and look like a regression in
     * whatever changed between runs.
     */
    IntDwmForEachTopLevel(IntDwmDestroySprite);

    gbDwmRedirectionActive = FALSE;
    TRACE("DWM redirection stopped\n");
}

/* ---------------------------------------------------------------------------
 * Transport
 * ------------------------------------------------------------------------- */

/*
 * Post one redirection command as an LPC datagram.
 *
 * cbPayload is the size dwmredir will validate against
 * (RedirVerifyPayloadSize reads u1.s1.DataLength), so it must be the real
 * command size -- undersizing it makes dwmredir drop the message silently.
 */
static VOID
IntDwmPost(PVOID pPayload, USHORT cbPayload)
{
    KIRQL OldIrql;
    ULONG Next;

    ASSERT(cbPayload <= DWM_QUEUE_ENTRYSIZE);

    if (!IntDwmIsActive() || !gbDwmQueueInit)
        return;

    /*
     * ENQUEUE ONLY. The actual LpcRequestPort happens on the worker thread --
     * see the long note in dwm.h. Posting from here would strip the
     * kernel-only bit and dwmredir would discard every command in silence.
     *
     * A spinlock rather than the USER lock: the worker must never take the
     * USER lock (it would deadlock against a window-manager thread that is
     * mid-emit waiting on the queue), so the queue needs its own.
     */
    KeAcquireSpinLock(&gDwmQueueLock, &OldIrql);

    Next = (gDwmQueueHead + 1) % DWM_QUEUE_ENTRIES;
    if (Next == gDwmQueueTail)
    {
        /*
         * Full. Drop the NEWEST rather than overwrite the oldest: the queue is
         * an ordered command stream, and overwriting the tail would reorder
         * a CreateSprite behind its own UpdateSprite. Counted so it shows up
         * as a number instead of as inexplicably missing windows.
         */
        KeReleaseSpinLock(&gDwmQueueLock, OldIrql);
        if (InterlockedIncrement(&glDwmDropped) == 1)
            ERR("DWM queue full -- dropping commands (opcode 0x%lx)\n",
                *(UINT32*)pPayload);
        return;
    }

    gDwmQueue[gDwmQueueHead].cbPayload = cbPayload;
    RtlCopyMemory(gDwmQueue[gDwmQueueHead].Payload, pPayload, cbPayload);
    gDwmQueueHead = Next;

    KeReleaseSpinLock(&gDwmQueueLock, OldIrql);

    KeSetEvent(&gDwmQueueEvent, IO_NO_INCREMENT, FALSE);
}

/*
 * The posting thread. PsCreateSystemThread gives it PreviousMode == KernelMode
 * for its whole life, which is the only reason LPC will stamp
 * LPC_KERNELMODE_MESSAGE on what it sends.
 */
static VOID NTAPI
IntDwmWorkerThread(PVOID Context)
{
    struct
    {
        PORT_MESSAGE Header;
        BYTE         Data[DWM_QUEUE_ENTRYSIZE];
    } Msg;
    DWM_QUEUE_ENTRY Entry;
    KIRQL    OldIrql;
    NTSTATUS Status;
    BOOL     fHave;

    UNREFERENCED_PARAMETER(Context);

    TRACE("DWM worker thread running\n");

    while (InterlockedCompareExchange(&glDwmWorkerStop, 0, 0) == 0)
    {
        KeWaitForSingleObject(&gDwmQueueEvent, Executive, KernelMode, FALSE, NULL);

        for (;;)
        {
            KeAcquireSpinLock(&gDwmQueueLock, &OldIrql);
            fHave = (gDwmQueueTail != gDwmQueueHead);
            if (fHave)
            {
                Entry = gDwmQueue[gDwmQueueTail];
                gDwmQueueTail = (gDwmQueueTail + 1) % DWM_QUEUE_ENTRIES;
            }
            KeReleaseSpinLock(&gDwmQueueLock, OldIrql);

            if (!fHave)
                break;

            if (gpDwmApiPort == NULL)
                continue;

            RtlZeroMemory(&Msg, sizeof(Msg));
            RtlCopyMemory(Msg.Data, Entry.Payload, Entry.cbPayload);

            Msg.Header.u1.s1.DataLength  = Entry.cbPayload;
            Msg.Header.u1.s1.TotalLength =
                (CSHORT)(sizeof(PORT_MESSAGE) + Entry.cbPayload);
            Msg.Header.MessageId = InterlockedIncrement(&glDwmSequence);

            /*
             * kind = 3 (async datagram) | the kernel-only bit.
             *
             * ntoskrnl validates the BASE type only -- LpcpGetMessageType
             * masks LPC_KERNELMODE_MESSAGE off first (lpc_x.h:13) -- so
             * 3|0x8000 passes as LPC_DATAGRAM and the flag survives into the
             * copied message, because this thread is kernel mode.
             */
            Msg.Header.u2.s2.Type = DWM_MSG_KIND_ASYNC | DWM_MSG_KERNELONLY;

            /*
             * ONE-SHOT: does LPC actually stamp the kernel-only bit?
             *
             * dwm.exe reports every sprite arriving with Type=0x0003 -- the
             * bit gone -- which routes all thirty redirection opcodes to
             * dwmredir's USER-mode dispatcher, where none of them have an arm.
             *
             * Reading ntoskrnl says it should survive: LpcRequestPort ORs
             * LPC_KERNELMODE_MESSAGE in when PreviousMode == KernelMode
             * (lpc/send.c:47), LpcpMoveMessage writes `MessageType & 0xFFFF`
             * so 0x8000 is kept (lpc/reply.c:156), and the receive side passes
             * MessageType = 0 so the type copies verbatim (reply.c:686).
             * PreviousMode is the only input that is not visible from the
             * source, so print it rather than assume it.
             */
            {
                static LONG lProbed = 0;
                if (InterlockedCompareExchange(&lProbed, 1, 0) == 0)
                {
                    ERR("DWM probe: PreviousMode=%d (KernelMode=%d) pid=%p TypeOut=0x%04x\n",
                        (int)ExGetPreviousMode(),
                        (int)KernelMode,
                        PsGetCurrentProcessId(),
                        (unsigned)(USHORT)Msg.Header.u2.s2.Type);
                }
            }

            Status = LpcRequestPort(gpDwmApiPort, &Msg.Header);

            if (!NT_SUCCESS(Status))
            {
                ERR("DWM LPC post failed: 0x%lx (opcode 0x%lx)\n",
                    Status, *(UINT32*)Entry.Payload);

                /*
                 * A dead port means the DWM process is gone. Latch redirection
                 * off rather than failing once per window event forever;
                 * dwm.exe re-registers on restart.
                 */
                if (Status == STATUS_PORT_DISCONNECTED ||
                    Status == STATUS_INVALID_PORT_HANDLE)
                {
                    gbDwmRedirectionActive = FALSE;
                }
            }
        }
    }

    TRACE("DWM worker thread exiting (%ld commands dropped)\n", glDwmDropped);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static NTSTATUS
IntDwmStartWorker(VOID)
{
    NTSTATUS Status;
    HANDLE   hThread;

    if (gpDwmWorkerThread != NULL)
        return STATUS_SUCCESS;

    if (!gbDwmQueueInit)
    {
        KeInitializeSpinLock(&gDwmQueueLock);
        KeInitializeEvent(&gDwmQueueEvent, SynchronizationEvent, FALSE);
        gDwmQueueHead = gDwmQueueTail = 0;
        gbDwmQueueInit = TRUE;
    }

    InterlockedExchange(&glDwmWorkerStop, 0);

    Status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                  IntDwmWorkerThread, NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("DWM: failed to create worker thread: 0x%lx\n", Status);
        return Status;
    }

    /* Keep a referenced pointer so shutdown can wait on it; the handle itself
     * is not needed past this point. */
    Status = ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType,
                                       KernelMode, (PVOID*)&gpDwmWorkerThread,
                                       NULL);
    ZwClose(hThread);

    if (!NT_SUCCESS(Status))
    {
        ERR("DWM: failed to reference worker thread: 0x%lx\n", Status);
        gpDwmWorkerThread = NULL;
        return Status;
    }

    TRACE("DWM worker thread started\n");
    return STATUS_SUCCESS;
}

static VOID
IntDwmStopWorker(VOID)
{
    if (gpDwmWorkerThread == NULL)
        return;

    InterlockedExchange(&glDwmWorkerStop, 1);
    KeSetEvent(&gDwmQueueEvent, IO_NO_INCREMENT, FALSE);

    /*
     * Wait for it to actually leave. The port object is dereferenced right
     * after this returns, and a worker still inside LpcRequestPort would be
     * holding a pointer to freed memory.
     */
    KeWaitForSingleObject(gpDwmWorkerThread, Executive, KernelMode, FALSE, NULL);

    ObDereferenceObject(gpDwmWorkerThread);
    gpDwmWorkerThread = NULL;
}

/* ---------------------------------------------------------------------------
 * Window -> command packing
 * ------------------------------------------------------------------------- */

/*
 * Which windows get a sprite.
 *
 * Vista gates the compositor's own side on sprite-live plus WS_VISIBLE, and the
 * uDWM notes record what happens without an equivalent gate here: every
 * invisible helper window in the session (PNIHiddenWnd, WorkerW,
 * MS_WebcheckMonitor) gets chromed. Filter at the producer so those never reach
 * the wire in the first place.
 */
static BOOL
IntDwmShouldRedirect(PWND Wnd)
{
    if (Wnd == NULL || Wnd->head.h == NULL)
        return FALSE;

    /* Top-level only. Child windows have their own NotifyChild* opcodes,
     * which phase 1 does not emit. */
    if (Wnd->style & WS_CHILD)
        return FALSE;

    /* The desktop itself is the compositor's root, not a composed window. */
    if (Wnd == UserGetDesktopWindow())
        return FALSE;

    return TRUE;
}

static VOID
IntDwmFillMiniInfo(PWND Wnd, PDWM_MINIWINDOWINFO pInfo)
{
    RtlZeroMemory(pInfo, sizeof(*pInfo));
    pInfo->rcWindow  = Wnd->rcWindow;
    pInfo->rcClient  = Wnd->rcClient;
    pInfo->dwStyle   = Wnd->style;
    pInfo->dwExStyle = Wnd->ExStyle;
    pInfo->fDpiAware = 0;
}

/*
 * Registers the window with dwmredir. MUST precede its CREATESPRITE.
 *
 * dwmredir's CMilWindowManager::NotifyChildCreate is the only thing that puts
 * an hwnd into the context map, and CreateSprite looks the hwnd up in that map
 * rather than creating on demand -- see the comment on
 * RWMCMD_REDIR_NOTIFYCHILDCREATE in dwm.h. Without this every sprite we sent
 * was answered with E_HANDLE and dropped.
 *
 * hwndParent is 0 for the top-level windows phase 1 redirects, which is the
 * case dwmredir flags as top level.
 */
static VOID
IntDwmNotifyChildCreate(PWND Wnd)
{
    DWM_CMD_NOTIFYCHILDCREATE Cmd;

    RtlZeroMemory(&Cmd, sizeof(Cmd));
    Cmd.Type       = RWMCMD_REDIR_NOTIFYCHILDCREATE;
    Cmd.hwnd       = HandleToUlong(Wnd->head.h);
    Cmd.hwndParent = 0;
    Cmd.dwStyle    = Wnd->style;
    Cmd.dwExStyle  = Wnd->ExStyle;
    Cmd.rcWindow   = Wnd->rcWindow;
    Cmd.dwClsStyle = (Wnd->pcls != NULL) ? Wnd->pcls->style : 0;

    IntDwmPost(&Cmd, sizeof(Cmd));
}

VOID
IntDwmCreateSprite(PWND Wnd)
{
    DWM_CMD_CREATESPRITE Cmd;

    if (!IntDwmIsActive() || !IntDwmShouldRedirect(Wnd))
        return;

    if (Wnd->DwmSprite != 0)
        return;

    /*
     * Registration first, and in that order on the wire: both commands go
     * through the same queue and the same worker, so the ordering here is the
     * ordering dwmredir sees.
     */
    IntDwmNotifyChildCreate(Wnd);

    Wnd->DwmSprite = (UINT32)InterlockedIncrement(&glDwmNextSprite);

    RtlZeroMemory(&Cmd, sizeof(Cmd));
    Cmd.Type     = RWMCMD_REDIR_CREATESPRITE;
    Cmd.hSprite  = Wnd->DwmSprite;
    Cmd.hwnd     = HandleToUlong(Wnd->head.h);
    Cmd.rcWindow = Wnd->rcWindow;
    Cmd.fVisible = (Wnd->style & WS_VISIBLE) ? TRUE : FALSE;
    IntDwmFillMiniInfo(Wnd, &Cmd.MiniInfo);

    TRACE("CreateSprite hwnd=%p sprite=%lu %dx%d\n",
          Wnd->head.h, Cmd.hSprite,
          Wnd->rcWindow.right - Wnd->rcWindow.left,
          Wnd->rcWindow.bottom - Wnd->rcWindow.top);

    IntDwmPost(&Cmd, sizeof(Cmd));
}

VOID
IntDwmDestroySprite(PWND Wnd)
{
    DWM_CMD_DESTROYSPRITE Cmd;

    /*
     * Deliberately NOT gated on IntDwmShouldRedirect: a window's style can
     * change between create and destroy, and a sprite we created must be torn
     * down on the strength of having a sprite id, not on it still qualifying.
     */
    if (!IntDwmIsActive() || Wnd == NULL || Wnd->DwmSprite == 0)
        return;

    Cmd.Type    = RWMCMD_REDIR_DESTROYSPRITE;
    Cmd.hSprite = Wnd->DwmSprite;

    TRACE("DestroySprite hwnd=%p sprite=%lu\n", Wnd->head.h, Cmd.hSprite);
    IntDwmPost(&Cmd, sizeof(Cmd));

    Wnd->DwmSprite = 0;
}

VOID
IntDwmShowSprite(PWND Wnd, BOOL fShow)
{
    DWM_CMD_SHOWSPRITE Cmd;

    if (!IntDwmIsActive() || Wnd == NULL || Wnd->DwmSprite == 0)
        return;

    Cmd.Type    = RWMCMD_REDIR_SHOWSPRITE;
    Cmd.hSprite = Wnd->DwmSprite;
    Cmd.fShow   = fShow ? TRUE : FALSE;

    TRACE("ShowSprite hwnd=%p sprite=%lu show=%d\n",
          Wnd->head.h, Cmd.hSprite, fShow);
    IntDwmPost(&Cmd, sizeof(Cmd));
}

VOID
IntDwmUpdateSprite(PWND Wnd)
{
    DWM_CMD_UPDATESPRITE Cmd;
    DWM_MINIWINDOWINFO   Info;

    if (!IntDwmIsActive() || Wnd == NULL || Wnd->DwmSprite == 0)
        return;

    IntDwmFillMiniInfo(Wnd, &Info);

    RtlZeroMemory(&Cmd, sizeof(Cmd));
    Cmd.Type         = RWMCMD_REDIR_UPDATESPRITE;
    Cmd.hSprite      = Wnd->DwmSprite;
    Cmd.Flags        = 0;
    Cmd.fHasMiniInfo = TRUE;

    /* Only the first ten mini-info dwords ride along; dwmredir zeroes the
     * remaining two itself (CWin32Redirection::UpdateSprite). */
    RtlCopyMemory(Cmd.MiniInfo, &Info, sizeof(Cmd.MiniInfo));

    TRACE("UpdateSprite hwnd=%p sprite=%lu (%d,%d)-(%d,%d)\n",
          Wnd->head.h, Cmd.hSprite,
          Wnd->rcWindow.left, Wnd->rcWindow.top,
          Wnd->rcWindow.right, Wnd->rcWindow.bottom);
    IntDwmPost(&Cmd, sizeof(Cmd));
}

VOID
IntDwmActivationChange(PWND Wnd, BOOL fActive)
{
    DWM_CMD_ACTIVATIONCHANGE Cmd;

    /*
     * Gated on having a sprite, but keyed on the HWND -- this command does not
     * carry a sprite id (see DWM_CMD_ACTIVATIONCHANGE). The sprite check is
     * just "is this a window DWM knows about"; sending activation for a window
     * it never got a CreateSprite for would name nothing on its side.
     */
    if (!IntDwmIsActive() || Wnd == NULL || Wnd->DwmSprite == 0)
        return;

    Cmd.Type    = RWMCMD_REDIR_NOTIFYACTIVATIONCHANGE;
    Cmd.hwnd    = HandleToUlong(Wnd->head.h);
    Cmd.fActive = fActive ? TRUE : FALSE;

    TRACE("ActivationChange hwnd=%p active=%d\n", Wnd->head.h, fActive);
    IntDwmPost(&Cmd, sizeof(Cmd));
}

VOID
IntDwmZorderSprite(PWND Wnd)
{
    DWM_CMD_ZORDERSPRITE Cmd;
    PWND Prev;

    if (!IntDwmIsActive() || Wnd == NULL || Wnd->DwmSprite == 0)
        return;

    /*
     * "Insert after" means the sprite this one sits behind. win32k's sibling
     * list runs front-to-back, so the window ahead of us in the list is the one
     * we are inserted after. Walk back past siblings that carry no sprite --
     * they are not in the compositor's tree, so naming one would be a dangling
     * reference.
     */
    Prev = Wnd->spwndPrev;
    while (Prev != NULL && Prev->DwmSprite == 0)
        Prev = Prev->spwndPrev;

    Cmd.Type               = RWMCMD_REDIR_ZORDERSPRITE;
    Cmd.hSprite            = Wnd->DwmSprite;
    Cmd.hSpriteInsertAfter = Prev ? Prev->DwmSprite : 0;

    TRACE("ZorderSprite hwnd=%p sprite=%lu after=%lu\n",
          Wnd->head.h, Cmd.hSprite, Cmd.hSpriteInsertAfter);
    IntDwmPost(&Cmd, sizeof(Cmd));
}

/* ---------------------------------------------------------------------------
 * Syscalls
 *
 * DEVIATION FROM VISTA, STATED DELIBERATELY:
 * Vista places these at syscall 0x1270-0x1274, contiguous. ReactOS's service
 * table follows Windows 2003 SP2 ordering (win32ss/w32ksvc32.h), where those
 * slots hold unrelated 2003 calls. Matching Vista's NUMBERS would mean
 * reordering all 739 entries to Vista's layout and breaking every existing
 * user32/gdi32 call -- an enormous change unrelated to composition. The names,
 * signatures and argument counts ARE 1:1; only the ordinals differ, and nothing
 * binds these by ordinal (win32u.spec resolves them by name).
 * ------------------------------------------------------------------------- */

BOOL
APIENTRY
NtUserRegisterSessionPort(HANDLE hPort)
{
    NTSTATUS Status;

    UserEnterExclusive();
    Status = IntDwmRegisterSessionPort(hPort);
    UserLeave();

    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

BOOL
APIENTRY
NtUserUnregisterSessionPort(VOID)
{
    UserEnterExclusive();
    IntDwmUnregisterSessionPort();
    UserLeave();
    return TRUE;
}

BOOL
APIENTRY
NtUserDwmStartRedirection(BOOL fRedirectContent)
{
    NTSTATUS Status;

    UserEnterExclusive();
    Status = IntDwmStartRedirection(fRedirectContent);
    UserLeave();

    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

BOOL
APIENTRY
NtUserDwmStopRedirection(VOID)
{
    UserEnterExclusive();
    IntDwmStopRedirection();
    UserLeave();
    return TRUE;
}

/* EOF */
