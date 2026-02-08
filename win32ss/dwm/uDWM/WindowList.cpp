#include "uDWM.h"
#include <debug.h>
#include "RwmVistaSp1MilCmd.h"
extern DwmDesktop* DwmDesktopInstance;

static LIST_ENTRY g_RwmWindowListHead;
static BOOLEAN g_RwmWindowListInitialized = FALSE;
static UINT g_RwmWindowCount = 0;
static ULONG g_uDwmUpdateSceneCount = 0;

#pragma pack(push, 4)
typedef struct _MILCMD_WINDOWNODE_SETBOUNDS_VSP1
{
    MILCMD Type;          /* 58 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    RECT WindowRect;
    RECT ClientRect;
    RECT ContentRect;
} MILCMD_WINDOWNODE_SETBOUNDS_VSP1;

typedef struct _MILCMD_WINDOWNODE_UPDATESPRITEHANDLE_VSP1
{
    MILCMD Type;          /* 60 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    HSPRITE hSprite;
    UINT32 Unknown0;      /* always 0 in dwmredir */
} MILCMD_WINDOWNODE_UPDATESPRITEHANDLE_VSP1;

typedef struct _MILCMD_WINDOWNODE_SETALPHAMARGINS_VSP1
{
    MILCMD Type;          /* 67 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    MARGINS Margins;
} MILCMD_WINDOWNODE_SETALPHAMARGINS_VSP1;

typedef struct _MILCMD_WINDOWNODE_CREATE_VSP1
{
    MILCMD Type;          /* 54 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    UINT32 Sprite;        /* HSPRITE (DWORD) */
    UINT32 Unknown0;      /* always 0 in dwmredir */
    UINT32 Hwnd;          /* HWND (DWORD) */
    UINT32 Unknown1;      /* always 0 in dwmredir */
    UINT32 CachingMode;   /* MILWindowTargetCachingMode (DWORD) */
} MILCMD_WINDOWNODE_CREATE_VSP1;

typedef struct _MILCMD_TARGET_SETROOT_VSP1
{
    MILCMD Type;          /* 77 */
    HMIL_RESOURCE Handle; /* target */
    HMIL_RESOURCE hRoot;  /* root node */
} MILCMD_TARGET_SETROOT_VSP1;

typedef struct _MILCMD_WINDOWNODE_SETSPRITEIMAGE_VSP1
{
    MILCMD Type;              /* 62 */
    HMIL_RESOURCE Handle;     /* WindowNode handle */
    HMIL_RESOURCE hSpriteImage; /* typically a redirection/GDI surface handle */
} MILCMD_WINDOWNODE_SETSPRITEIMAGE_VSP1;
#pragma pack(pop)

static __forceinline VOID uDwmInitializeWindowTracking(VOID)
{
    if (!g_RwmWindowListInitialized)
    {
        InitializeListHead(&g_RwmWindowListHead);
        g_RwmWindowListInitialized = TRUE;
        g_RwmWindowCount = 0;
    }
}

static UINT uDwmGetInsertedChildCount(VOID)
{
    if (!g_RwmWindowListInitialized)
        return 0;

    UINT c = 0;
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (pData && (pData->Flags & RWM_WD_VISUAL_INSERTED))
            c++;
    }
    return c;
}

static UINT uDwmGetInsertedIndexForWindow(_In_ const RWM_WINDOWDATA_VISTA_SP1* pTarget)
{
    if (!g_RwmWindowListInitialized || !pTarget)
        return 0;

    UINT idx = 0;
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        const RWM_WINDOWDATA_VISTA_SP1* pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (pData == pTarget)
            break;
        if (pData && (pData->Flags & RWM_WD_VISUAL_INSERTED))
            idx++;
    }
    return idx;
}

static VOID uDwmSetDesiredVisible(PRWM_WINDOWDATA_VISTA_SP1 pData, BOOLEAN visible)
{
    if (!pData)
        return;

    pData->Flags |= RWM_WD_HAS_DESIRED_VISIBLE;
    if (visible)
        pData->Flags |= RWM_WD_DESIRED_VISIBLE;
    else
        pData->Flags &= ~RWM_WD_DESIRED_VISIBLE;
}

static BOOLEAN uDwmGetDesiredVisible(const RWM_WINDOWDATA_VISTA_SP1* pData)
{
    if (!pData)
        return FALSE;
    if (!(pData->Flags & RWM_WD_HAS_DESIRED_VISIBLE))
        return TRUE; /* default: visible until told otherwise */
    return (pData->Flags & RWM_WD_DESIRED_VISIBLE) ? TRUE : FALSE;
}

static HRESULT uDwmWindowNode_SetBounds(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode || !pData->hWnd)
        return E_UNEXPECTED;

    RECT rcWindow = {};
    RECT rcClient = {};
    RECT rcContent = {};

    if (!GetWindowRect(pData->hWnd, &rcWindow))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!GetClientRect(pData->hWnd, &rcClient))
        return HRESULT_FROM_WIN32(GetLastError());

    POINT pt = { rcClient.left, rcClient.top };
    (void)ClientToScreen(pData->hWnd, &pt);
    const LONG cx = rcClient.right - rcClient.left;
    const LONG cy = rcClient.bottom - rcClient.top;
    rcClient.left = pt.x;
    rcClient.top = pt.y;
    rcClient.right = pt.x + cx;
    rcClient.bottom = pt.y + cy;

    /* Until we implement real GetContentRect semantics, treat content==client. */
    rcContent = rcClient;

    pData->WindowRect = rcWindow;
    pData->ClientMarginsRect = rcClient;

    MILCMD_WINDOWNODE_SETBOUNDS_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETBOUNDS;
    cmd.Handle = (HMIL_RESOURCE)pData->ClientNode;
    cmd.WindowRect = rcWindow;
    cmd.ClientRect = rcClient;
    cmd.ContentRect = rcContent;

    HRESULT hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
    if (FAILED(hr))
        DPRINT1("uDwmWindowNode_SetBounds failed hr=0x%08lx node=0x%lx hwnd=0x%p\n", hr, pData->ClientNode, pData->hWnd);
    return hr;
}

static HRESULT uDwmWindowNode_UpdateSpriteHandle(PRWM_WINDOWDATA_VISTA_SP1 pData, HSPRITE hSprite)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    MILCMD_WINDOWNODE_UPDATESPRITEHANDLE_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_UPDATESPRITE;
    cmd.Handle = (HMIL_RESOURCE)pData->ClientNode;
    cmd.hSprite = hSprite;
    cmd.Unknown0 = 0;

    HRESULT hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
    if (FAILED(hr))
        DPRINT1("uDwmWindowNode_UpdateSpriteHandle failed hr=0x%08lx node=0x%lx\n", hr, pData->ClientNode);
    return hr;
}

static HRESULT uDwmWindowNode_SetAlphaMargins(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode || !pData->Window)
        return E_UNEXPECTED;

    MARGINS m = {};
    pData->Window->GetClientMargins(&m);

    MILCMD_WINDOWNODE_SETALPHAMARGINS_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETALPHAMARGINS;
    cmd.Handle = (HMIL_RESOURCE)pData->ClientNode;
    cmd.Margins = m;

    HRESULT hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
    if (FAILED(hr))
        DPRINT1("uDwmWindowNode_SetAlphaMargins failed hr=0x%08lx node=0x%lx\n", hr, pData->ClientNode);
    return hr;
}

static HRESULT uDwmWindowNode_SetSpriteImage(PRWM_WINDOWDATA_VISTA_SP1 pData, UINT32 hSurface)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    /* Reuse ClientNodeClone as "last sprite image surface" cache (UINT32 handle). */
    if (pData->ClientNodeClone == hSurface)
        return S_OK;

    MILCMD_WINDOWNODE_SETSPRITEIMAGE_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETSPRITEIMAGE;
    cmd.Handle = (HMIL_RESOURCE)pData->ClientNode;
    cmd.hSpriteImage = (HMIL_RESOURCE)hSurface;

    HRESULT hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
    if (SUCCEEDED(hr))
        pData->ClientNodeClone = hSurface;
    else
        DPRINT1("uDwmWindowNode_SetSpriteImage failed hr=0x%08lx node=0x%lx surf=0x%lx\n",
                hr, (ULONG)pData->ClientNode, (ULONG)hSurface);
    return hr;
}

static HRESULT uDwmTryAttachClientSurface(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !pData->Window || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    UINT32 hSurface = 0;
    HRESULT hrSurf = pData->Window->GetGDISurface(DwmDesktopInstance->GlobalChannel, &hSurface);
    if (FAILED(hrSurf) || !hSurface)
        return FAILED(hrSurf) ? hrSurf : S_FALSE;

    return uDwmWindowNode_SetSpriteImage(pData, hSurface);
}

static HRESULT uDwmEnsureWindowVisual(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel)
        return E_UNEXPECTED;

    HRESULT hr = S_OK;
    const BOOL LogClientNodeOnce = (pData->Flags & RWM_WD_LOGGED_CLIENTNODE) ? FALSE : TRUE;

    /*
     * On Vista, dwmredir may call CreateWindow before the per-window client
     * composition node has been fully established. Keep probing until we get a
     * non-zero node handle.
     */
    if (!pData->ClientNode && pData->Window)
    {
        UINT32 node = 0;
        HRESULT hrNode = pData->Window->GetClientNode(DwmDesktopInstance->GlobalChannel, &node);
        if (SUCCEEDED(hrNode) && node)
        {
            pData->ClientNode = node;
        }
        else
        {
            /*
             * Vista dwmredir's GetClientNode duplicates a pre-existing WindowRootNode
             * handle from its source channel. If that node isn't ready yet, it can
             * fail or return 0.
             *
             * Try GetClientNodeClone as a more self-contained fallback; it is known
             * to create a usable node graph in dwmredir.
             */
            UINT32 clone = 0;
            HRESULT hrClone = pData->Window->GetClientNodeClone(DwmDesktopInstance->GlobalChannel, &clone);
            if (SUCCEEDED(hrClone) && clone)
            {
                pData->ClientNode = clone;
                if (LogClientNodeOnce)
                    DPRINT1("uDwmEnsureWindowVisual: got ClientNodeClone=0x%lx for hwnd=0x%p\n",
                            (ULONG)pData->ClientNode, pData->hWnd);
            }
            else if (LogClientNodeOnce)
            {
                DPRINT1("uDwmEnsureWindowVisual: GetClientNode failed/empty hr=0x%08lx node=0x%lx; clone hr=0x%08lx clone=0x%lx hwnd=0x%p\n",
                        hrNode, (ULONG)node, hrClone, (ULONG)clone, pData->hWnd);
            }
        }
    }
    if (LogClientNodeOnce && pData->ClientNode)
        pData->Flags |= RWM_WD_LOGGED_CLIENTNODE;

    /*
     * If this is the DesktopWindow, switch the desktop render target root over
     * to its node. This matches Vista's "root at desktop window context" model
     * much better than keeping an empty fallback root.
     */
    if (pData->ClientNode &&
        DwmDesktopInstance &&
        DwmDesktopInstance->hDesktopTarget &&
        pData->hWnd == GetDesktopWindow() &&
        (HMIL_RESOURCE)pData->ClientNode != DwmDesktopInstance->hRootNode)
    {
        HMIL_RESOURCE oldRoot = DwmDesktopInstance->hRootNode;
        const BOOLEAN oldWasDesktop = DwmDesktopInstance->RootIsDesktopClone;

        DwmDesktopInstance->hRootNode = (HMIL_RESOURCE)pData->ClientNode;
        DwmDesktopInstance->RootIsDesktopClone = TRUE;

        MILCMD_TARGET_SETROOT_VSP1 setRoot = {};
        setRoot.Type = (MILCMD)RWM_MILCMD_VSP1_TARGET_SETROOT;
        setRoot.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hDesktopTarget;
        setRoot.hRoot = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
        (void)MilResource_SendCommand(&setRoot, sizeof(setRoot), DwmDesktopInstance->GlobalChannel);

        /*
         * Critical: ensure the new root has non-empty bounds.
         * We always set bounds on the initial fallback root in EnsureDesktopTargetAndRoot(),
         * but when we switch the root to the DesktopWindow's node we must do it again,
         * otherwise milcore may clip the entire subtree to empty and everything "disappears"
         * after the root swap.
         */
        (void)uDwmWindowNode_SetBounds(pData);

        /* Force a reinsert pass for all windows against the new root. */
        if (g_RwmWindowListInitialized)
        {
            for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
            {
                PRWM_WINDOWDATA_VISTA_SP1 wd = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
                if (wd && wd->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
                    wd->Flags &= ~RWM_WD_VISUAL_INSERTED;
            }
        }

        DPRINT1("uDwmEnsureWindowVisual: switched root to DesktopWindow node=0x%lx (old root=0x%lx)\n",
                (ULONG)DwmDesktopInstance->hRootNode, (ULONG)oldRoot);

        /* Release the old fallback root if we owned it. */
        if (oldRoot && !oldWasDesktop)
            (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, oldRoot, NULL);

        /* Don't parent the desktop root into itself. */
        return S_OK;
    }

    /*
     * Vista dwmredir does NOT use Visual_SetContent(WindowNode) in the desktop tree.
     * It builds a graph of WindowNodes and uses Visual_InsertChildAt/RemoveChild
     * on these graph nodes. Treat the ClientNode as the child to parent directly.
     */

    /* Insert client WindowNode into root WindowNode once. */
    if (uDwmGetDesiredVisible(pData) &&
        !(pData->Flags & RWM_WD_VISUAL_INSERTED) &&
        DwmDesktopInstance->hRootNode &&
        pData->ClientNode)
    {
        /* If this node is the desktop root, don't parent it into itself. */
        if ((HMIL_RESOURCE)pData->ClientNode == (HMIL_RESOURCE)DwmDesktopInstance->hRootNode)
            return S_OK;

        static BOOLEAN s_loggedFirstInsert = FALSE;

        MILCMD_VISUAL_INSERTCHILDAT insertChild = {};
        insertChild.Type =
#if UDWM_TARGET_VISTA_SP1_MILCORE
            (MILCMD)RWM_MILCMD_VSP1_VISUAL_INSERTCHILDAT;
#else
            MilCmdVisualInsertChildAt;
#endif
        insertChild.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
        insertChild.hChild = (HMIL_RESOURCE)pData->ClientNode;
        /* Must be <= current child count on the root visual. */
        insertChild.index = uDwmGetInsertedChildCount();
        hr = MilResource_SendCommand(&insertChild, sizeof(insertChild), DwmDesktopInstance->GlobalChannel);
        if (FAILED(hr))
            DPRINT1("uDwmEnsureWindowVisual: InsertChildAt failed hr=0x%08lx\n", hr);
        if (SUCCEEDED(hr))
        {
            pData->Flags |= RWM_WD_VISUAL_INSERTED;
            if (!s_loggedFirstInsert)
            {
                s_loggedFirstInsert = TRUE;
                DPRINT1("uDwmEnsureWindowVisual: first child inserted root=0x%lx child=0x%lx idx=%u\n",
                        (ULONG)DwmDesktopInstance->hRootNode,
                        (ULONG)pData->ClientNode,
                        insertChild.index);
            }
        }
    }

    if (pData->ClientNode)
    {
        (void)uDwmWindowNode_SetBounds(pData);
        (void)uDwmWindowNode_SetAlphaMargins(pData);
        if (pData->hSprite)
            (void)uDwmWindowNode_UpdateSpriteHandle(pData, pData->hSprite);

        /* Best-effort: bind the current client surface for visible pixels. */
        (void)uDwmTryAttachClientSurface(pData);
    }

    return hr;
}

HRESULT
WINAPI
uDwmCreateWindow(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmCreateWindow Called\n");
    if (!DwmWindowInterface)
        return E_INVALIDARG;
    if (!DwmDesktopInstance)
        return E_UNEXPECTED;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    uDwmInitializeWindowTracking();

    if (DwmWindowInterface->GetClientData())
    {
        LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    }

    PRWM_WINDOWDATA_VISTA_SP1 pData =
        (PRWM_WINDOWDATA_VISTA_SP1)HeapAlloc(GetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             sizeof(RWM_WINDOWDATA_VISTA_SP1));
    if (!pData)
    {
        LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
        return E_OUTOFMEMORY;
    }

    pData->cbSize = sizeof(RWM_WINDOWDATA_VISTA_SP1);
    pData->Signature = RWM_WINDOWDATA_VISTA_SP1_SIGNATURE;
    pData->Window = DwmWindowInterface;
    pData->hWnd = DwmWindowInterface->GetWindowHandle();
    pData->hSprite = DwmWindowInterface->GetSpriteHandle();
    /* Default visible until ShowHide tells us otherwise. */

    /* Best-effort: capture the window's MIL node (Vista uses this heavily). */
    if (DwmDesktopInstance->GlobalChannel)
    {
        UINT32 node = 0;
        HRESULT hrNode = DwmWindowInterface->GetClientNode(DwmDesktopInstance->GlobalChannel, &node);
        if (SUCCEEDED(hrNode))
            pData->ClientNode = node;
    }

    /*
     * Attach the per-window data to the window object. This must happen before
     * dwmredir starts issuing further per-window callbacks.
     */
    DwmWindowInterface->SetClientData((PVOID)pData);

    /* Track this window internally so UpdateScene can walk it. */
    InsertTailList(&g_RwmWindowListHead, &pData->ListEntry);
    /* Create per-window visual and hook up content. */
    (void)uDwmEnsureWindowVisual(pData);
    g_RwmWindowCount++;

    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmDestroyWindow(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDestroyWindow Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);

    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
        return E_INVALIDARG;
    }

    if (g_RwmWindowListInitialized)
    {
        RemoveEntryList(&pData->ListEntry);
        if (g_RwmWindowCount)
            g_RwmWindowCount--;
    }

    if ((pData->Flags & RWM_WD_VISUAL_INSERTED) && DwmDesktopInstance->hRootNode && pData->ClientNode)
    {
        MILCMD_VISUAL_REMOVECHILD removeChild = {};
        removeChild.Type =
#if UDWM_TARGET_VISTA_SP1_MILCORE
            (MILCMD)RWM_MILCMD_VSP1_VISUAL_REMOVECHILD;
#else
            MilCmdVisualRemoveChild;
#endif
        removeChild.Handle = DwmDesktopInstance->hRootNode;
        removeChild.hChild = (HMIL_RESOURCE)pData->ClientNode;
        (void)MilResource_SendCommand(&removeChild, sizeof(removeChild), DwmDesktopInstance->GlobalChannel);
        pData->Flags &= ~RWM_WD_VISUAL_INSERTED;
    }

    if (pData->hWindowTransform)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hWindowTransform, NULL);
        pData->hWindowTransform = 0;
    }

    DwmWindowInterface->SetClientData(NULL);
    HeapFree(GetProcessHeap(), 0, pData);

    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmCreateSprite(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmCreateSprite Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        pData->hSprite = DwmWindowInterface->GetSpriteHandle();
        (void)uDwmEnsureWindowVisual(pData);
        (void)uDwmWindowNode_UpdateSpriteHandle(pData, pData->hSprite);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmDestroySprite(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDestroySprite Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        pData->hSprite = 0;
        (void)uDwmWindowNode_UpdateSpriteHandle(pData, 0);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmShowHide(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmShowHide Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        const BOOLEAN visible = DwmWindowInterface->IsVisible();
        uDwmSetDesiredVisible(pData, visible);
        (void)uDwmEnsureWindowVisual(pData);

        if (!visible && (pData->Flags & RWM_WD_VISUAL_INSERTED) &&
            DwmDesktopInstance->hRootNode && pData->ClientNode)
        {
            MILCMD_VISUAL_REMOVECHILD removeChild = {};
            removeChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_REMOVECHILD;
            removeChild.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
            removeChild.hChild = (HMIL_RESOURCE)pData->ClientNode;
            HRESULT hrRm = MilResource_SendCommand(&removeChild, sizeof(removeChild), DwmDesktopInstance->GlobalChannel);
            if (FAILED(hrRm))
            {
                DPRINT1("uDwmShowHide: RemoveChild failed hr=0x%08lx root=0x%lx child=0x%lx\n",
                        hrRm, (ULONG)DwmDesktopInstance->hRootNode, (ULONG)pData->ClientNode);
            }
            else
            {
                pData->Flags &= ~RWM_WD_VISUAL_INSERTED;
            }
        }
        else if (visible && !(pData->Flags & RWM_WD_VISUAL_INSERTED) &&
                 DwmDesktopInstance->hRootNode && pData->ClientNode)
        {
            MILCMD_VISUAL_INSERTCHILDAT insertChild = {};
            insertChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_INSERTCHILDAT;
            insertChild.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
            insertChild.hChild = (HMIL_RESOURCE)pData->ClientNode;
            insertChild.index = uDwmGetInsertedIndexForWindow(pData);
            HRESULT hrIns = MilResource_SendCommand(&insertChild, sizeof(insertChild), DwmDesktopInstance->GlobalChannel);
            if (FAILED(hrIns))
            {
                /* Fallback: append, in case our index math got out of sync. */
                insertChild.index = uDwmGetInsertedChildCount();
                hrIns = MilResource_SendCommand(&insertChild, sizeof(insertChild), DwmDesktopInstance->GlobalChannel);
            }

            if (FAILED(hrIns))
            {
                DPRINT1("uDwmShowHide: InsertChildAt failed hr=0x%08lx root=0x%lx child=0x%lx idx=%u\n",
                        hrIns, (ULONG)DwmDesktopInstance->hRootNode, (ULONG)pData->ClientNode, insertChild.index);
            }
            else
            {
                pData->Flags |= RWM_WD_VISUAL_INSERTED;
            }
        }
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmMoveSize(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmMoveSize Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);
        (void)uDwmWindowNode_SetBounds(pData);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmZOrder(CompositedWindow* DwmWindowInterface, CompositedWindow* DwmWindowToInsertAfter)
{
    DPRINT1("uDwmZOrder Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        /* Reorder our internal list to reflect the requested z-order. */
        RemoveEntryList(&pData->ListEntry);

        if (DwmWindowToInsertAfter)
        {
            PRWM_WINDOWDATA_VISTA_SP1 pAfter = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowToInsertAfter->GetClientData();
            if (pAfter && pAfter->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
            {
                InsertHeadList(&pAfter->ListEntry, &pData->ListEntry);
            }
            else
            {
                InsertTailList(&g_RwmWindowListHead, &pData->ListEntry);
            }
        }
        else
        {
            InsertTailList(&g_RwmWindowListHead, &pData->ListEntry);
        }

        /* If currently parented, remove and reinsert at the new index. */
        if ((pData->Flags & RWM_WD_VISUAL_INSERTED) && DwmDesktopInstance->hRootNode && pData->ClientNode)
        {
            MILCMD_VISUAL_REMOVECHILD removeChild = {};
            removeChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_REMOVECHILD;
            removeChild.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
            removeChild.hChild = (HMIL_RESOURCE)pData->ClientNode;
            HRESULT hrRm = MilResource_SendCommand(&removeChild, sizeof(removeChild), DwmDesktopInstance->GlobalChannel);
            if (FAILED(hrRm))
            {
                DPRINT1("uDwmZOrder: RemoveChild failed hr=0x%08lx root=0x%lx child=0x%lx\n",
                        hrRm, (ULONG)DwmDesktopInstance->hRootNode, (ULONG)pData->ClientNode);
                /* Don't clear the flag if we didn't actually remove it. */
            }
            else
            {
                pData->Flags &= ~RWM_WD_VISUAL_INSERTED;
            }

            MILCMD_VISUAL_INSERTCHILDAT insertChild = {};
            insertChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_INSERTCHILDAT;
            insertChild.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
            insertChild.hChild = (HMIL_RESOURCE)pData->ClientNode;
            insertChild.index = uDwmGetInsertedIndexForWindow(pData);
            HRESULT hrIns = MilResource_SendCommand(&insertChild, sizeof(insertChild), DwmDesktopInstance->GlobalChannel);
            if (FAILED(hrIns))
            {
                insertChild.index = uDwmGetInsertedChildCount();
                hrIns = MilResource_SendCommand(&insertChild, sizeof(insertChild), DwmDesktopInstance->GlobalChannel);
            }

            if (FAILED(hrIns))
            {
                DPRINT1("uDwmZOrder: InsertChildAt failed hr=0x%08lx root=0x%lx child=0x%lx idx=%u\n",
                        hrIns, (ULONG)DwmDesktopInstance->hRootNode, (ULONG)pData->ClientNode, insertChild.index);
            }
            else
            {
                pData->Flags |= RWM_WD_VISUAL_INSERTED;
            }
        }
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmStyleChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmOwnerChange(CompositedWindow* DwmWindowInterface, CompositedWindow* NewDwmWindowInterfac)
{
    return 0;
}

HRESULT WINAPI uDwmClientMarginsChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmClientMarginsChange Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);
        (void)uDwmWindowNode_SetAlphaMargins(pData);
        (void)uDwmWindowNode_SetBounds(pData);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmClientGlassChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmActivationChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmAlphaChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmBlurBehindChange(CompositedWindow* DwmWindowInterface, DWM_BLURBEHIND * BlurBehind)
{
    return 0;
}

HRESULT WINAPI uDwmClipChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmDXContentChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDXContentChange Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);
        (void)uDwmTryAttachClientSurface(pData);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmGDISurfaceChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmGDISurfaceChange Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);

        (void)uDwmTryAttachClientSurface(pData);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmGhostChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmForceIconicRepresentationChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmFlip3DWindowPolicyChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmForceDisconnectClientNode(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmGetWindowBounds(HWND hWnd, RECT* rect)
{
    return 0;
}

HRESULT
WINAPI
uDwmProcessAsyncDwmMessage(RWM_COMMANDS Command, PVOID CommandData, UINT32 CommandDataSize, BOOLEAN IsKernelMessage)
{
    DPRINT1("uDwmProcessAsyncDwmMessage Called cmd=0x%X size=%u kernel=%u\n",
            Command, CommandDataSize, IsKernelMessage);
    switch(Command)
    {
        case RWMCMD_REDIR_STARTUPBEGIN:
        {
            /* Vista: StartupBegin just flips internal state under a critical section. */
            DPRINT1("RWMCMD_REDIR_STARTUPBEGIN\n");
            break;
        }
        case RWMCMD_REDIR_STARTUP:
        {
            DPRINT1("RWMCMD_REDIR_STARTUP\n");
            /*
             * Vista: StartupEnd enables the desktop render target.
             * Our equivalent: ensure the desktop target/root are created using
             * the uDWM (desktop target) path, and commit.
             */
            if (DwmDesktopInstance)
            {
                (void)DwmDesktopInstance->EnsureDesktopTargetAndRoot();
                if (DwmDesktopInstance->GlobalChannel)
                    (void)MilChannel_CommitChannel(DwmDesktopInstance->GlobalChannel);
            }
            break;
        }
        
        case RWMCMD_REDIR_CHANGESETTINGS:
        {
            DPRINT1("RWMCMD_REDIR_CHANGESETTINGS\n");
            break;
        }
        default:
        {
            break;
        }
    }
    return 0;
}

HRESULT WINAPI uDwmProcessSyncDwmMessage(RWM_COMMANDS Command, PVOID CommandData, UINT32 CommandDataSize, BOOLEAN IsKernelMessage, UINT32 ProcessId, REMOTE_PORT_VIEW* RemotePortView, HRESULT* ReplyHr, UINT32* ReplySize)
{
    return 0;
}

HRESULT WINAPI uDwmProcessBackChannelMessage(MIL_MESSAGE* Message)
{
    return 0;
}

HRESULT WINAPI uDwmUpdateScene()
{
    static ULONG s_scenePrint = 0;
    if ((++s_scenePrint % 60) == 1)
        DPRINT1("uDwmUpdateScene Called (%lu)\n", s_scenePrint);

    g_uDwmUpdateSceneCount++;
    if (!DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel)
        return E_UNEXPECTED;

    /* Minimal breadcrumb: verify DWM is driving us. */
    if (g_uDwmUpdateSceneCount == 1)
        DPRINT1("uDwmUpdateScene: first call, target=0x%lx root=0x%lx\n",
                (ULONG)DwmDesktopInstance->hDesktopTarget,
                (ULONG)DwmDesktopInstance->hRootNode);

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);

    /*
     * Vista routes UpdateScene through the desktop manager which renders the
     * root visual tree, destroys pending sprites, then commits and sync-flushes
     * the MIL channel. We don't have the full scene graph yet; still, we keep
     * the same commit/flush semantics so the redirection pipeline makes forward
     * progress.
     */
    /* Minimal Vista-ish walk: ensure nodes exist and bounds/transform are up to date. */
    if (g_RwmWindowListInitialized)
    {
        for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
        {
            PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
            if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
                continue;

            (void)uDwmEnsureWindowVisual(pData);
        }
    }

    /*
     * Do not emit arbitrary test commands here.
     * In particular, DwmVisual::DrawBullshit was sending a ColorResource update
     * against a Visual handle, which can poison a Vista milcore command stream.
     */

    /*
     * IMPORTANT:
     * Do NOT send TargetInvalidate with a guessed command ID. On Vista SP1 this
     * can crash the milcore render thread (batch processing error 0x8000ffff),
     * killing all rendering.
     *
     * For now, we only commit + syncflush; once we confirm the correct Vista
     * numeric ID and packet shape for TargetInvalidate, we can reintroduce it.
     */
    /*
     * IMPORTANT (Vista behavior):
     * uDWM does NOT call MilComposition_SyncFlush. It sends a transport command
     * MILCMD_TRANSPORT_SYNCFLUSH (Type=3, sizeof=4) and then commits.
     *
     * Calling MilComposition_SyncFlush against Vista milcore has been observed
     * to crash the render thread with 0x8000ffff (batch processing error).
     */
    MILCMD_TRANSPORT_SYNCFLUSH tf = {};
    tf.Type = (MILCMD)RWM_MILCMD_VSP1_TRANSPORT_SYNCFLUSH;
    (void)MilResource_SendCommand(&tf, sizeof(tf), DwmDesktopInstance->GlobalChannel);

    HRESULT hr = MilChannel_CommitChannel(DwmDesktopInstance->GlobalChannel);

    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return hr;
}


EXTERN_C
VOID
WINAPI 
UpdateWindowList( IDwmWindowList* WindowListInstance)
{
    WindowListInstance->lpVtbl->WindowListCreateWindow = uDwmCreateWindow;
    WindowListInstance->lpVtbl->WindowListDestroyWindow = uDwmDestroyWindow;
    WindowListInstance->lpVtbl->WindowListCreateSprite = uDwmCreateSprite;
    WindowListInstance->lpVtbl->WindowListDestroySprite = uDwmDestroySprite;
    WindowListInstance->lpVtbl->WindowListShowHide = uDwmShowHide;
    WindowListInstance->lpVtbl->WindowListMoveSize = uDwmMoveSize;
    WindowListInstance->lpVtbl->WindowListZOrder = uDwmZOrder;
    WindowListInstance->lpVtbl->WindowListStyleChange = uDwmStyleChange;
    WindowListInstance->lpVtbl->WindowListOwnerChange = uDwmOwnerChange;
    WindowListInstance->lpVtbl->WindowListClientMarginsChange = uDwmClientMarginsChange;
    WindowListInstance->lpVtbl->WindowListClientGlassChange = uDwmClientGlassChange;
    WindowListInstance->lpVtbl->WindowListActivationChange = uDwmActivationChange;
    WindowListInstance->lpVtbl->WindowListAlphaChange = uDwmAlphaChange;
    WindowListInstance->lpVtbl->WindowListBlurBehindChange = uDwmBlurBehindChange;
    WindowListInstance->lpVtbl->WindowListClipChange = uDwmClipChange;
    WindowListInstance->lpVtbl->WindowListDXContentChange = uDwmDXContentChange;
    WindowListInstance->lpVtbl->WindowListGDISurfaceChange = uDwmGDISurfaceChange;
    WindowListInstance->lpVtbl->WindowListGhostChange = uDwmGhostChange;
    WindowListInstance->lpVtbl->WindowListForceIconicRepresentationChange = uDwmForceIconicRepresentationChange;
    WindowListInstance->lpVtbl->WindowListFlip3DWindowPolicyChange = uDwmFlip3DWindowPolicyChange;
    WindowListInstance->lpVtbl->WindowListForceDisconnectClientNode = uDwmForceDisconnectClientNode;
    WindowListInstance->lpVtbl->WindowListGetWindowBounds = uDwmGetWindowBounds;
    WindowListInstance->lpVtbl->WindowListProcessAsyncDwmMessage = uDwmProcessAsyncDwmMessage;
    WindowListInstance->lpVtbl->WindowListProcessSyncDwmMessage = uDwmProcessSyncDwmMessage;
    WindowListInstance->lpVtbl->WindowListProcessBackChannelMessage = uDwmProcessBackChannelMessage;
    WindowListInstance->lpVtbl->WindowListUpdateScene = uDwmUpdateScene;
}
