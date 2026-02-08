#include "uDWM.h"
#include <debug.h>
#include "RwmVistaSp1MilCmd.h"
extern DwmDesktop* DwmDesktopInstance;

static LIST_ENTRY g_RwmWindowListHead;
static BOOLEAN g_RwmWindowListInitialized = FALSE;
static UINT g_RwmWindowCount = 0;
static ULONG g_uDwmUpdateSceneCount = 0;
static BOOLEAN g_RwmRebuildChildren = TRUE;

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

/*
 * Vista SP1 translate transform command as used by dwmredir:
 * DuceHelper::CreateTranslateTransform:
 * - CreateResource(TYPE_TRANSLATETRANSFORM=68)
 * - Send cmd.Type=163, sizeof=0x20
 * Layout (from dwmredir.dll.c):
 *   UINT32 Type(163), UINT32 Handle, double X, double Y, UINT32 unk0, UINT32 unk1
 */
typedef struct _MILCMD_TRANSLATETRANSFORM_VSP1
{
    MILCMD Type;              /* 163 */
    HMIL_RESOURCE Handle;     /* transform handle */
    double X;
    double Y;
    UINT32 Unknown0;
    UINT32 Unknown1;
} MILCMD_TRANSLATETRANSFORM_VSP1;

/* Visual_SetTransform (also used on WindowNode handles in Vista) */
typedef struct _MILCMD_VISUAL_SETTRANSFORM_VSP1
{
    MILCMD Type;              /* 35 */
    HMIL_RESOURCE Handle;     /* visual/window node */
    HMIL_RESOURCE hTransform; /* transform handle or 0 */
} MILCMD_VISUAL_SETTRANSFORM_VSP1;
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

static __forceinline BOOLEAN uDwmComputeVisibleFromHwnd(_In_opt_ HWND hWnd)
{
    if (!hWnd)
        return FALSE;

    /*
     * Visibility is surprisingly tricky during startup:
     * - IsWindowVisible() can transiently return FALSE during reparenting/show sequences.
     * - GetWindowLongPtrW() can fail (returns 0) and must be error-checked.
     *
     * We bias toward "visible" when queries are unreliable to avoid the classic symptom:
     * "renders briefly, then disappears forever".
     */
    if (!IsWindow(hWnd))
        return FALSE;

    BOOLEAN visWin32 = IsWindowVisible(hWnd) ? TRUE : FALSE;

    SetLastError(0);
    const LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
    const DWORD gle = GetLastError();

    BOOLEAN visStyle = FALSE;
    if (!(style == 0 && gle != 0))
        visStyle = (style & WS_VISIBLE) ? TRUE : FALSE;

    /* If either method says visible, treat as visible (unless minimized). */
    BOOLEAN vis = (visWin32 || visStyle) ? TRUE : FALSE;
    if (vis && IsIconic(hWnd))
        vis = FALSE;

    /*
     * If both checks say "not visible" but style query failed, keep it visible.
     * This avoids incorrect hides when GetWindowLongPtrW() is temporarily failing.
     */
    if (!vis && (style == 0 && gle != 0))
        vis = TRUE;

    return vis;
}

static __forceinline BOOLEAN uDwmIsTopmostWindow(_In_opt_ HWND hWnd)
{
    if (!hWnd || !IsWindow(hWnd))
        return FALSE;

    /* Shell tray should behave topmost for our purposes. */
    WCHAR cls[64] = {0};
    if (GetClassNameW(hWnd, cls, ARRAYSIZE(cls)) > 0)
    {
        if (_wcsicmp(cls, L"Shell_TrayWnd") == 0)
            return TRUE;
    }

    SetLastError(0);
    const LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    const DWORD gle = GetLastError();
    if (exStyle == 0 && gle != 0)
        return FALSE; /* unknown -> not topmost */
    return (exStyle & WS_EX_TOPMOST) ? TRUE : FALSE;
}

static VOID uDwmLogWindowBrief(_In_ const char* tag, _In_opt_ HWND hWnd)
{
    if (!hWnd || !IsWindow(hWnd))
        return;

    WCHAR cls[64] = {0};
    WCHAR text[96] = {0};
    (void)GetClassNameW(hWnd, cls, ARRAYSIZE(cls));
    (void)GetWindowTextW(hWnd, text, ARRAYSIZE(text));

    /* Only spam for key shell windows (tray/taskmgr). */
    if (_wcsicmp(cls, L"Shell_TrayWnd") != 0 &&
        _wcsicmp(cls, L"TaskManagerWindow") != 0)
        return;

    DWORD pid = 0;
    (void)GetWindowThreadProcessId(hWnd, &pid);
    SetLastError(0);
    const LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
    const DWORD gleStyle = GetLastError();
    SetLastError(0);
    const LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    const DWORD gleEx = GetLastError();

    DPRINT1("%s: %s hwnd=0x%p cls=%S text=%S pid=%lu vis=%u top=%u style=0x%Ix%s ex=0x%Ix%s\n",
            tag,
            "uDWM",
            hWnd,
            cls,
            text,
            (ULONG)pid,
            (ULONG)(IsWindowVisible(hWnd) ? 1 : 0),
            (ULONG)(uDwmIsTopmostWindow(hWnd) ? 1 : 0),
            (ULONG_PTR)style,
            (style == 0 && gleStyle != 0) ? "(!)" : "",
            (ULONG_PTR)exStyle,
            (exStyle == 0 && gleEx != 0) ? "(!)" : "");
}

static __forceinline PRWM_WINDOWDATA_VISTA_SP1 uDwmFindWindowDataByHwnd(_In_opt_ HWND hWnd)
{
    if (!g_RwmWindowListInitialized || !hWnd)
        return NULL;

    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE && pData->hWnd == hWnd)
            return pData;
    }
    return NULL;
}

static HRESULT uDwmInsertChildAtRoot(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData, _Inout_ UINT* pIndex)
{
    if (!pData || !pIndex || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !DwmDesktopInstance->hRootNode)
        return E_UNEXPECTED;

    if (!pData->ClientNode)
        return E_INVALIDARG;
    if ((HMIL_RESOURCE)pData->ClientNode == (HMIL_RESOURCE)DwmDesktopInstance->hRootNode)
        return S_FALSE;

    MILCMD_VISUAL_INSERTCHILDAT insertChild = {};
    insertChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_INSERTCHILDAT;
    insertChild.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
    insertChild.hChild = (HMIL_RESOURCE)pData->ClientNode;
    insertChild.index = *pIndex;

    HRESULT hrIns = MilResource_SendCommand(&insertChild, sizeof(insertChild), DwmDesktopInstance->GlobalChannel);
    if (SUCCEEDED(hrIns))
    {
        pData->Flags |= RWM_WD_VISUAL_INSERTED;
        (*pIndex)++;
    }
    else
    {
        DPRINT1("uDwmRebuildRootChildren: InsertChildAt failed hr=0x%08lx root=0x%lx child=0x%lx idx=%u hwnd=0x%p\n",
                hrIns,
                (ULONG)DwmDesktopInstance->hRootNode,
                (ULONG)pData->ClientNode,
                insertChild.index,
                pData->hWnd);
    }
    return hrIns;
}

/*
 * Reference behavior (Vista uDWM): Z-order and show/hide are modeled on a higher-level
 * VisualCollection and become MIL commands during RenderRecursive().
 *
 * Our uDWM implementation talks directly to the MIL channel. To avoid "remove without
 * successful reinsert" scenarios (which present as "shows briefly then disappears forever"),
 * we rebuild the root's child list in UpdateScene whenever the logical order/visibility changes.
 */
static __forceinline VOID uDwmMarkRebuildNeeded(VOID)
{
    g_RwmRebuildChildren = TRUE;
}

static VOID uDwmRebuildRootChildren(VOID)
{
    if (!g_RwmWindowListInitialized || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !DwmDesktopInstance->hRootNode)
        return;

    if (!g_RwmRebuildChildren)
        return;

    UINT cVisible = 0;
    const UINT cTotal = g_RwmWindowCount;

    /* Pass 1: remove everything we believe is currently inserted. */
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
            continue;

        if ((pData->Flags & RWM_WD_VISUAL_INSERTED) && pData->ClientNode &&
            (HMIL_RESOURCE)pData->ClientNode != (HMIL_RESOURCE)DwmDesktopInstance->hRootNode)
        {
            MILCMD_VISUAL_REMOVECHILD removeChild = {};
            removeChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_REMOVECHILD;
            removeChild.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
            removeChild.hChild = (HMIL_RESOURCE)pData->ClientNode;
            (void)MilResource_SendCommand(&removeChild, sizeof(removeChild), DwmDesktopInstance->GlobalChannel);
            pData->Flags &= ~RWM_WD_VISUAL_INSERTED;
        }
    }

    /*
     * Pass 2: refresh visibility + insert visible windows.
     *
     * Vista reference:
     * - `CWindowList::ZOrder` maintains an internal list and applies `ZOrderAfter` to the root visual's
     *   VisualCollection (see `DwmReversing\\Vista\\uDWM.dll.c`, `CWindowList::ZOrder`).
     *
     * Our uDWM talks to milcore directly (InsertChildAt/RemoveChild), which makes incremental reorder fragile
     * early-on. For correctness, rebuild the root's children in *actual Win32 z-order* so that topmost/bands
     * are honored deterministically (this matches what Vista ultimately composes from).
     */
    UINT idx = 0;

    /* Refresh desired visibility for all tracked windows up-front. */
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
            continue;
        if (pData->hWnd)
        {
            uDwmSetDesiredVisible(pData, uDwmComputeVisibleFromHwnd(pData->hWnd));
            uDwmLogWindowBrief("rebuild", pData->hWnd);
        }
    }

    /* Insert tracked windows in actual Win32 z-order (bottom -> top). */
    UINT cZ = 0;
    for (HWND w = GetTopWindow(NULL); w; w = GetWindow(w, GW_HWNDNEXT))
        cZ++;

    if (cZ)
    {
        HWND* z = (HWND*)HeapAlloc(GetProcessHeap(), 0, sizeof(HWND) * cZ);
        if (z)
        {
            UINT k = 0;
            for (HWND w = GetTopWindow(NULL); w && k < cZ; w = GetWindow(w, GW_HWNDNEXT))
                z[k++] = w;

            while (k)
            {
                HWND w = z[--k]; /* bottom -> top */
                PRWM_WINDOWDATA_VISTA_SP1 pData = uDwmFindWindowDataByHwnd(w);
                if (!pData)
                    continue;
                if (!uDwmGetDesiredVisible(pData))
                    continue;
                if (!pData->ClientNode)
                    continue;
                if (pData->Flags & RWM_WD_VISUAL_INSERTED)
                    continue;

                cVisible++;
                (void)uDwmInsertChildAtRoot(pData, &idx);
            }

            HeapFree(GetProcessHeap(), 0, z);
        }
    }

    /* Fallback: insert any remaining visible tracked windows not present in z-order enumeration. */
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
            continue;
        if (!uDwmGetDesiredVisible(pData) || !pData->ClientNode)
            continue;
        if (pData->Flags & RWM_WD_VISUAL_INSERTED)
            continue;

        cVisible++;
        (void)uDwmInsertChildAtRoot(pData, &idx);
    }

    /*
     * Breadcrumb (rate limited): if everything disappears, this tells us whether
     * we're rebuilding to zero children due to visibility state.
     */
    static ULONG s_rebuildPrint = 0;
    if ((++s_rebuildPrint % 60) == 1 || idx == 0)
    {
        DPRINT1("uDwmRebuildRootChildren: root=0x%lx inserted=%u visible=%u total=%u\n",
                (ULONG)DwmDesktopInstance->hRootNode, idx, cVisible, cTotal);
    }

    g_RwmRebuildChildren = FALSE;
}

static HRESULT uDwmWindowNode_SetBounds(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode || !pData->hWnd)
        return E_UNEXPECTED;

    RECT rcWindow = {};
    RECT rcClient = {};
    RECT rcContent = {};

    /*
     * Root safety:
     * When the render-target root is the DesktopWindow clone, never allow an empty/odd
     * DesktopWindow rect to clip the entire subtree (classic "everything appears briefly then vanishes").
     */
    if (DwmDesktopInstance->RootIsDesktopClone &&
        pData->hWnd == GetDesktopWindow() &&
        (HMIL_RESOURCE)pData->ClientNode == (HMIL_RESOURCE)DwmDesktopInstance->hRootNode)
    {
        rcWindow.left = 0;
        rcWindow.top = 0;
        rcWindow.right = GetSystemMetrics(SM_CXSCREEN);
        rcWindow.bottom = GetSystemMetrics(SM_CYSCREEN);
        rcClient = rcWindow;
        rcContent = rcWindow;
    }
    else
    {
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
    }

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

static HRESULT uDwmUpdateWindowTransform(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    /*
     * Reference (Vista dwmredir CMilWindowContext::RefreshTransform):
     * compute an (x,y) offset and apply it via:
     * - CreateTranslateTransform(x,y) -> hTransform (TYPE_TRANSLATETRANSFORM=68, cmd 163)
     * - Visual_SetTransform(m_hWindowRootNode, hTransform) (cmd 35)
     *
     * We don't yet have the full CMilWindowContext content-rect model; approximate
     * using WindowRect relative to the DesktopWindow origin.
     */
    RECT rcDesk = {};
    const HWND hDesk = GetDesktopWindow();
    if (hDesk)
        (void)GetWindowRect(hDesk, &rcDesk);

    const double xOffset = (double)((LONG)pData->WindowRect.left - (LONG)rcDesk.left);
    const double yOffset = (double)((LONG)pData->WindowRect.top - (LONG)rcDesk.top);

    /* Cache to avoid churning transform resources. */
    if (pData->hWindowTransform && pData->OffsetX == xOffset && pData->OffsetY == yOffset)
        return S_OK;

    HMIL_RESOURCE hNew = 0;
    HRESULT hr = S_OK;

    if (xOffset != 0.0 || yOffset != 0.0)
    {
        hr = MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                 (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_TRANSLATETRANSFORM,
                                                 &hNew);
        if (FAILED(hr) || !hNew)
            return FAILED(hr) ? hr : E_FAIL;

        MILCMD_TRANSLATETRANSFORM_VSP1 cmd = {};
        cmd.Type = (MILCMD)RWM_MILCMD_VSP1_TRANSLATETRANSFORM;
        cmd.Handle = hNew;
        cmd.X = xOffset;
        cmd.Y = yOffset;
        cmd.Unknown0 = 0;
        cmd.Unknown1 = 0;

        hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
        if (FAILED(hr))
        {
            (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, hNew, NULL);
            return hr;
        }
    }

    MILCMD_VISUAL_SETTRANSFORM_VSP1 set = {};
    set.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_SETTRANSFORM;
    set.Handle = (HMIL_RESOURCE)pData->ClientNode;
    set.hTransform = (HMIL_RESOURCE)hNew;
    hr = MilResource_SendCommand(&set, sizeof(set), DwmDesktopInstance->GlobalChannel);

    if (SUCCEEDED(hr))
    {
        if (pData->hWindowTransform)
            (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hWindowTransform, NULL);

        pData->hWindowTransform = hNew;
        pData->OffsetX = xOffset;
        pData->OffsetY = yOffset;
    }
    else if (hNew)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, hNew, NULL);
    }

    return hr;
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
        uDwmMarkRebuildNeeded();

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

    /* Parenting into the root is handled in uDwmRebuildRootChildren() during UpdateScene. */

    if (pData->ClientNode)
    {
        (void)uDwmWindowNode_SetBounds(pData);
        (void)uDwmWindowNode_SetAlphaMargins(pData);
        (void)uDwmUpdateWindowTransform(pData);
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
    uDwmLogWindowBrief("create", pData->hWnd);
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
    uDwmMarkRebuildNeeded();
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
    uDwmLogWindowBrief("destroy", pData->hWnd);
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
    uDwmMarkRebuildNeeded();

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
        uDwmLogWindowBrief("showhide", pData->hWnd);
        /*
         * Reference behavior: ShowHide reflects actual HWND visibility.
         * Our CompositedWindow::IsVisible() implementation may be stubbed early-on,
         * which would incorrectly hide everything (classic "shows briefly then disappears").
         *
         * Prefer user32's IsWindowVisible on the HWND when available.
         */
        BOOLEAN visibleIface = DwmWindowInterface->IsVisible();
        BOOLEAN visibleHwnd = visibleIface;
        if (pData->hWnd)
            visibleHwnd = IsWindowVisible(pData->hWnd) ? TRUE : FALSE;
        const BOOLEAN visibleStyle = uDwmComputeVisibleFromHwnd(pData->hWnd);

        static ULONG s_visLog = 0;
        if ((++s_visLog % 120) == 1 && pData->hWnd)
        {
            DPRINT1("uDwmShowHide: hwnd=0x%p iface=%u win32=%u style=%u\n",
                    pData->hWnd, (ULONG)visibleIface, (ULONG)visibleHwnd, (ULONG)visibleStyle);
        }

        const BOOLEAN visible = visibleStyle;
        uDwmSetDesiredVisible(pData, visible);
        (void)uDwmEnsureWindowVisual(pData);
        uDwmMarkRebuildNeeded();
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
        uDwmMarkRebuildNeeded();
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

    /* Apply visibility + z-order changes in one deterministic rebuild pass. */
    uDwmRebuildRootChildren();

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
