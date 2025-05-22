#include "uDWM.h"
#include <debug.h>
extern DwmDesktop* DwmDesktopInstance;
extern DwmVisual* GlobalRootVisual;

HRESULT
WINAPI
uDwmCreateWindow(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmCreateWindow Called:\n");
    __debugbreak();
    if (DwmWindowInterface->GetClientData())
    {
        DPRINT1("Window already initialized!\n");
         __debugbreak();
    }
    WindowClientData* WindowClientDataLoc;
    WindowClientDataLoc = new WindowClientData();
    DwmWindowInterface->SetClientData((PVOID)WindowClientDataLoc);
    WindowClientDataLoc->hWnd = DwmWindowInterface->GetWindowHandle();
    return S_OK;
}

HRESULT WINAPI uDwmDestroyWindow(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDestroyWindow Called:\n");
    return 0;
}

HRESULT WINAPI uDwmCreateSprite(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmCreateSprite Called:\n");
    return 0;
}

HRESULT WINAPI uDwmDestroySprite(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDestroySprite Called:\n");
    return 0;
}

HRESULT WINAPI uDwmShowHide(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmShowHide Called:\n");
    return 0;
}

HRESULT WINAPI uDwmMoveSize(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmMoveSize Called:\n");
    return 0;
}

HRESULT WINAPI uDwmZOrder(CompositedWindow* DwmWindowInterface, CompositedWindow* DwmWindowToInsertAfter)
{
    DPRINT1("uDwmZOrder Called:\n");
    return 0;
}

HRESULT WINAPI uDwmStyleChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmStyleChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmOwnerChange(CompositedWindow* DwmWindowInterface, CompositedWindow* NewDwmWindowInterfac)
{
    DPRINT1("uDwmOwnerChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmClientMarginsChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmClientMarginsChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmClientGlassChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmClientGlassChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmActivationChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmActivationChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmAlphaChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmAlphaChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmBlurBehindChange(CompositedWindow* DwmWindowInterface, DWM_BLURBEHIND * BlurBehind)
{
    DPRINT1("uDwmBlurBehindChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmClipChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmClipChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmDXContentChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDXContentChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmGDISurfaceChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmGDISurfaceChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmGhostChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmGhostChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmForceIconicRepresentationChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmForceIconicRepresentationChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmFlip3DWindowPolicyChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmFlip3DWindowPolicyChange Called:\n");
    return 0;
}

HRESULT WINAPI uDwmForceDisconnectClientNode(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmForceDisconnectClientNode Called:\n");
    return 0;
}

HRESULT WINAPI uDwmGetWindowBounds(HWND hWnd, RECT* rect)
{
    DPRINT1("uDwmGetWindowBounds Called:\n");
    return 0;
}

HRESULT
WINAPI
uDwmProcessAsyncDwmMessage(RWM_COMMANDS Command, PVOID CommandData, UINT32 CommandDataSize, BOOLEAN IsKernelMessage)
{
    DPRINT1("uDwmProcessAsyncDwmMessage Called with command 0x%X:\n", Command);
    switch(Command)
    {
        case RWMCMD_REDIR_STARTUP:
        {
            DPRINT1("uDwmProcessAsyncDwmMessage: RWMCMD_REDIR_STARTUP\n");
            DPRINT1("Mark Composition ready for start\n");
            /* Nothing else for now! */
            break;
        }
        
        case RWMCMD_REDIR_CHANGESETTINGS:
        {
            DPRINT1("uDwmProcessAsyncDwmMessage: RWMCMD_REDIR_CHANGESETTINGS\n");
            DPRINT1("This should likely load the aero theme information?\n");
            break;
        }
        default:
        {
            DPRINT1("uDwmProcessAsyncDwmMessage: Unimplemented Command 0x%X\n", Command);
            break;
        }
    }
    return 0;
}

HRESULT WINAPI uDwmProcessSyncDwmMessage(RWM_COMMANDS Command, PVOID CommandData, UINT32 CommandDataSize, BOOLEAN IsKernelMessage, UINT32 ProcessId, REMOTE_PORT_VIEW* RemotePortView, HRESULT* ReplyHr, UINT32* ReplySize)
{
    DPRINT1("uDwmProcessSyncDwmMessage Calledwith command 0x%X:\n", Command);
    return 0;
}

HRESULT WINAPI uDwmProcessBackChannelMessage(MIL_MESSAGE* Message)
{
    DPRINT1("uDwmProcessBackChannelMessage Called:\n");
    return 0;
}

HRESULT WINAPI uDwmUpdateScene()
{
    DPRINT1("uDwmUpdateScene Called:\n");
    MilChannel_CommitChannel(DwmDesktopInstance->GlobalChannel);
    return 0;
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
