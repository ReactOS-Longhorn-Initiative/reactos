/*
 * PROJECT:     RWM - User ReactOS Window Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:        
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

#include <uDWM.h>
#include <debug.h>
#include "RwmVistaSp1MilCmd.h"
IDwmRedirectionManager* m_pIDwmRedirectionManager = NULL;
DwmDesktop *DwmDesktopInstance;
DWORD WINAPI
uDWMWndProc(LPVOID lpThreadParameter)
{
    while(1)
    {
        Sleep(10000);
       // DPRINT1("uDWMWndProc Called:\n");
    }
    return 0;
}

HRESULT
WINAPI
CreateDwmDesktop(PRWM_STARTUPINFO StartupInfo, PRWM_COMPOSITIONINFO CompInfo)
{
    DwmDesktopInstance = new DwmDesktop();
    if (DwmDesktopInstance == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = DwmDesktopInstance->Initialize(StartupInfo, CompInfo);
    if (FAILED(hr))
    {
        delete DwmDesktopInstance;
        return hr;
    }

    return S_OK;
}

DwmDesktop::DwmDesktop()
{
    GlobalChannel = NULL;
    hDesktopTarget = 0;
    hRootNode = 0;
    RootIsDesktopClone = FALSE;
    InitializeCriticalSection(&CsDwmInstance);
}

DwmDesktop::~DwmDesktop()
{
    if (GlobalChannel && hDesktopTarget)
    {
        (void)MilResource_ReleaseOnChannel(GlobalChannel, hDesktopTarget, NULL);
        hDesktopTarget = 0;
    }
    if (GlobalChannel && hRootNode)
    {
        (void)MilResource_ReleaseOnChannel(GlobalChannel, hRootNode, NULL);
        hRootNode = 0;
    }
    DeleteCriticalSection(&CsDwmInstance);
}

/*
 * Vista SP1: dwmredir ensures a render target and binds a root node.
 *
 * In the normal (non cross-machine) path, it uses:
 * - CreateResource(TYPE_WINDOWRENDERTARGET=50)
 * - Send cmd.Type=73 (size 0x5c) to "create" the target (HWND + dimensions)
 * - Send cmd.Type=77 (size 0x0c) to set the root
 *
 * In the cross-machine path it uses TYPE_DESKTOPRENDERTARGET=48 with the same
 * cmd.Type=73, but a different payload.
 */
#pragma pack(push, 4)
typedef struct _MILCMD_TARGET_CREATE_VSP1
{
    MILCMD Type;          /* 73 */
    HMIL_RESOURCE Handle; /* target handle */
    UINT32 Data[21];      /* opaque payload, mostly zeros */
} MILCMD_TARGET_CREATE_VSP1;

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

typedef struct _MILCMD_WINDOWNODE_SETBOUNDS_VSP1
{
    MILCMD Type;          /* 58 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    RECT WindowRect;
    RECT ClientRect;
    RECT ContentRect;
} MILCMD_WINDOWNODE_SETBOUNDS_VSP1;
#pragma pack(pop)

static HRESULT uDwmEnsureDesktopTargetAndRoot(MIL_CHANNEL hChannel)
{
    if (!DwmDesktopInstance || !hChannel)
        return E_UNEXPECTED;

    HRESULT hr = S_OK;

    /*
     * Vista SP1 dwmredir uses a WindowNode (TYPE_WINDOWNODE=42) as the desktop
     * root that is bound to the desktop/window render target via SetRoot.
     * It then uses Visual_* child commands on these graph nodes.
     */
    if (!DwmDesktopInstance->hRootNode)
    {
        /*
         * Vista uDWM binds the desktop render target root to the *desktop window context*
         * WindowRootNode (created/owned by dwmredir), not to an arbitrary empty node.
         *
         * Try to obtain the desktop window's node clone via the redirection manager.
         */
        DwmDesktopInstance->RootIsDesktopClone = FALSE;
        if (m_pIDwmRedirectionManager && m_pIDwmRedirectionManager->LookupWindow)
        {
            const HWND hDesk = GetDesktopWindow();
            CompositedWindow* DeskWnd =
                (CompositedWindow*)m_pIDwmRedirectionManager->LookupWindow(m_pIDwmRedirectionManager, hDesk);
            if (DeskWnd)
            {
                UINT32 hRootClone = 0;
                HRESULT hrClone = DeskWnd->GetClientNodeClone(hChannel, &hRootClone);
                if (SUCCEEDED(hrClone) && hRootClone)
                {
                    DwmDesktopInstance->hRootNode = (HMIL_RESOURCE)hRootClone;
                    DwmDesktopInstance->RootIsDesktopClone = TRUE;
                    DPRINT1("uDwmEnsureDesktopTargetAndRoot: using desktop ClientNodeClone root=0x%lx hwnd=%p\n",
                            (ULONG)DwmDesktopInstance->hRootNode, hDesk);
                }
            }
        }

        /* Fallback: create an empty root WindowNode if we couldn't get the desktop clone. */
        if (!DwmDesktopInstance->hRootNode)
        {
            hr = MilResource_CreateOrAddRefOnChannel(hChannel,
                                                     (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_WINDOWNODE,
                                                     &DwmDesktopInstance->hRootNode);
            if (FAILED(hr) || !DwmDesktopInstance->hRootNode)
                return FAILED(hr) ? hr : E_FAIL;

            /* Mirror DuceHelper::WindowNode_Create: cmd.Type=54, sizeof=0x1C. */
            MILCMD_WINDOWNODE_CREATE_VSP1 nodeCreate = {};
            nodeCreate.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_CREATE;
            nodeCreate.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
            nodeCreate.Sprite = 0;
            nodeCreate.Unknown0 = 0;
            nodeCreate.Hwnd = 1;
            nodeCreate.Unknown1 = 0;
            nodeCreate.CachingMode = 0;

            hr = MilResource_SendCommand(&nodeCreate, sizeof(nodeCreate), hChannel);
            if (FAILED(hr))
            {
                DPRINT1("uDwmEnsureDesktopTargetAndRoot: WindowNode_Create failed hr=0x%08lx root=0x%lx\n",
                        hr, (ULONG)DwmDesktopInstance->hRootNode);
                return hr;
            }

            DPRINT1("uDwmEnsureDesktopTargetAndRoot: created fallback root windownode=0x%lx\n",
                    (ULONG)DwmDesktopInstance->hRootNode);
        }
    }

    /*
     * Critical: give the root a non-empty bounds rect.
     * Without bounds, milcore can clip the entire subtree to empty, resulting
     * in a permanent black screen even if children are inserted.
     */
    {
        RECT rcDesktop = {};
        const HWND hDesktopWnd = GetDesktopWindow();
        if (hDesktopWnd)
            (void)GetWindowRect(hDesktopWnd, &rcDesktop);

        if ((rcDesktop.right - rcDesktop.left) <= 0 || (rcDesktop.bottom - rcDesktop.top) <= 0)
        {
            rcDesktop.left = 0;
            rcDesktop.top = 0;
            rcDesktop.right = GetSystemMetrics(SM_CXSCREEN);
            rcDesktop.bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        MILCMD_WINDOWNODE_SETBOUNDS_VSP1 bounds = {};
        bounds.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETBOUNDS;
        bounds.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
        bounds.WindowRect = rcDesktop;
        bounds.ClientRect = rcDesktop;
        bounds.ContentRect = rcDesktop;
        (void)MilResource_SendCommand(&bounds, sizeof(bounds), hChannel);
    }

    if (!DwmDesktopInstance->hDesktopTarget)
    {
        /*
         * Match Vista SP1 uDWM (CDesktopManager::EnableRenderTargetImpl):
         * it creates a TYPE_DESKTOPRENDERTARGET (48) and sends cmd.Type=73 (0x5c bytes).
         *
         * NOTE:
         * We experimented with creating a TYPE_WINDOWRENDERTARGET (50) to force explicit sizing,
         * but while the command was accepted it caused presentation/clearing to stop in practice.
         * Keep the desktop target path as the known-good baseline; we'll fix viewport/offset issues
         * without changing the target resource type.
         */
        hr = MilResource_CreateOrAddRefOnChannel(hChannel,
                                                 (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_DESKTOPRENDERTARGET,
                                                 &DwmDesktopInstance->hDesktopTarget);
        if (FAILED(hr) || !DwmDesktopInstance->hDesktopTarget)
            return FAILED(hr) ? hr : E_FAIL;

        MILCMD_TARGET_CREATE_VSP1 cmd = {};
        cmd.Type = (MILCMD)RWM_MILCMD_VSP1_DESKTOPTARGET_CREATE; /* 73 */
        cmd.Handle = DwmDesktopInstance->hDesktopTarget;

        /*
         * Vista fills a mostly-zero payload with a couple of fixed-point scale values and flags.
         * Keep it conservative and zero-initialized, but set the observed fields.
         */
        cmd.Data[6]  = 0x00010000; /* scaleX (16.16) */
        cmd.Data[7]  = 0x00010000; /* scaleY (16.16) */
        cmd.Data[12] = 66846;      /* 66842 | 4 in Vista when enabled */
        cmd.Data[13] = 1;

        hr = MilResource_SendCommand(&cmd, sizeof(cmd), hChannel);
        if (FAILED(hr))
            return hr;
    }

    /* Bind root visual to the desktop target (Vista cmd.Type = 77). */
    MILCMD_TARGET_SETROOT setRoot = {};
    setRoot.Type = (MILCMD)RWM_MILCMD_VSP1_TARGET_SETROOT;
    setRoot.Handle = DwmDesktopInstance->hDesktopTarget;
    setRoot.hRoot = DwmDesktopInstance->hRootNode;

    hr = MilResource_SendCommand(&setRoot, sizeof(setRoot), hChannel);
    if (SUCCEEDED(hr))
        DPRINT1("uDwmEnsureDesktopTargetAndRoot: bound target=0x%lx to root=0x%lx\n",
                (ULONG)DwmDesktopInstance->hDesktopTarget, (ULONG)DwmDesktopInstance->hRootNode);
    else
        DPRINT1("uDwmEnsureDesktopTargetAndRoot: SetRoot failed hr=0x%08lx target=0x%lx root=0x%lx\n",
                hr, (ULONG)DwmDesktopInstance->hDesktopTarget, (ULONG)DwmDesktopInstance->hRootNode);

    return hr;
}

HRESULT DwmDesktop::EnsureDesktopTargetAndRoot()
{
    if (!GlobalChannel)
        return E_UNEXPECTED;
    return uDwmEnsureDesktopTargetAndRoot(GlobalChannel);
}

HRESULT (__fastcall *pfnNotificationCallbackTest)(const MIL_MESSAGE*);

HRESULT
__fastcall
NoteCallback(const MIL_MESSAGE* Message)
{
    DPRINT1("NoteCallback Called:\n");
    return 0;
}
EXTERN_C
VOID
WINAPI 
UpdateWindowList( IDwmWindowList* WindowListInstance);
EXTERN_C
HRESULT
WINAPI
DwmRedirectionManagerInitialize(PRWM_COMPOSITIONINFO Compinfo ,
                                IDwmWindowList* Interface ,
                                HRESULT (__fastcall *pfnNotificationCallback)(const MIL_MESSAGE*),
                                IDwmRedirectionManager ** Redir);

EXTERN_C
VOID WINAPI
DwmRedirectionManagerSetClientChannel(MIL_CHANNEL MilCoreHandle);

HRESULT DwmDesktop::Initialize(PRWM_STARTUPINFO StartupInfo, PRWM_COMPOSITIONINFO CompInfo)
{
    HRESULT hr = S_OK;
    DPRINT1("DwmDesktop::Initialize Called:\n");
    IDwmWindowList* WindowListInstance = (IDwmWindowList*)malloc(sizeof(IDwmWindowList));
    WindowListInstance->lpVtbl = (WindowListVtbl*)malloc(sizeof(WindowListVtbl));
    UpdateWindowList(WindowListInstance);
    hr = DwmRedirectionManagerInitialize(CompInfo, (IDwmWindowList*)WindowListInstance,
                                        StartupInfo->pfnNotificationCallback,
                                        &m_pIDwmRedirectionManager);
    if (SUCCEEDED(hr))
    {
        DPRINT1("DwmRedirectionManagerInitialize Succeeded:\n");
    }
    else
    {
        DPRINT1("DwmRedirectionManagerInitialize Failed:\n");
    }
    hr = MilConnection_CreateChannel(CompInfo->hConnection,
                                    CompInfo->hRedirectionStateChannel,
                                    &GlobalChannel);
    if (SUCCEEDED(hr))
    {
        DPRINT1("MilConnection_CreateChannel Succeeded:\n");
    }
    else
    {
        DPRINT1("MilConnection_CreateChannel Failed:\n");
    }

    hr = MilChannel_GetMarshalType(GlobalChannel, (MilMarshalType::Enum *)&MarshalType);
    if (SUCCEEDED(hr))
    {
        DPRINT1("MilChannel_GetMarshalType Succeeded:\n");
    }
    else
    {
        DPRINT1("MilChannel_GetMarshalType Failed:\n");
    }

    DwmRedirectionManagerSetClientChannel(GlobalChannel);
    /* Vista SP1 path creates a TYPE_VISUAL root in uDwmEnsureDesktopTargetAndRoot. */

     HANDLE EventStarted = CreateEventW(0, 1, 0, 0);
     CreateThread(0,
                 0,
                 uDWMWndProc,
                 EventStarted,
                 0,
                 0);

    hr = EnsureDesktopTargetAndRoot();
    if (FAILED(hr))
    {
        DPRINT1("uDwmEnsureDesktopTargetAndRoot Failed: 0x%08lx\n", hr);
    }

    MilChannel_CommitChannel(GlobalChannel);
    DPRINT1("DwmDesktop::Initialize Succeeded:\n");
    return hr;
}   

WindowClientData::WindowClientData()
{
    pszTitle = NULL;
    CompositedWindow = NULL;
    hWnd = NULL;
    SpriteHandle = NULL;
}

WindowClientData::~WindowClientData()
{
    if (pszTitle)
    {
        free(pszTitle);
        pszTitle = NULL;
    }
    if (CompositedWindow)
    {
        CompositedWindow = NULL;
    }
}