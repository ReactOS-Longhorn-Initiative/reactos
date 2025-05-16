

#include <LpcConnectLib.hpp>


#include <debug.h>

EXTERN_C
NTSTATUS
WINAPI
MilDwmInitialize(VOID)
{
    __debugbreak();
    return S_OK;
}

EXTERN_C
NTSTATUS
WINAPI
MilDwmShutdown(VOID)
{
    __debugbreak();
    return S_OK;
}

EXTERN_C
NTSTATUS
WINAPI
MilDwmReplySyncMessage(VOID)
{
    __debugbreak();
    return S_OK;
}

EXTERN_C
VOID
WINAPI
MilDwmAttachDesktopTarget(PVOID IMilClientChannelPublic, HANDLE RenderTargetHandle)
{
    __debugbreak();
}

VOID
DumpLpcData(PLPC_RWM_MESSAGE LpcMessage)
{
    DPRINT1("LpcDataDump for %X----------\n", LpcMessage->Message);
    /* HEX DUMP each byte of data of the total data size*/
    for (int i = 0; i < (LpcMessage->Header.u1.s1.DataLength - 2); i++)
    {
        if (i % 16 == 0)
            DPRINT1("\n");
        DPRINT1("%02X ", LpcMessage->Data[i]);
    }
}
EXTERN_C
NTSTATUS
WINAPI
MilDwmDispatchMessage(PLPC_RWM_MESSAGE LpcMessage)
{
    RWM_COMMANDS Command = (RWM_COMMANDS)LpcMessage->Message;
    switch (Command)
    {
        case RWMCMD_REDIR_SHUTDOWN:
            MilDwmShutdown();
            break;
        case RWMCMD_RX_GETSHAREDSURFACE:
            DPRINT1("RWMCMD_RX_GETSHAREDSURFACE: Called\n");
            break;
        case RWMCMD_RX_UPDATESHAREDSURF:
            DPRINT1("RWMCMD_RX_UPDATESHAREDSURF: Called\n");
            break;
        case RWMCMD_REDIR_CREATEWINDOW:
            DPRINT1("RWMCMD_REDIR_CREATEWINDOW: Called\n");
            break;
        case RWMCMD_REDIR_DESTROYWINDOW:
            DPRINT1("RWMCMD_REDIR_DESTROYWINDOW: Called\n");
            break;
        case RWMCMD_REDIR_DIRTYWINDOW:
            DPRINT1("RWMCMD_REDIR_DIRTYWINDOW: Called\n");
            break;
        case RWMCMD_REDIR_ZORDERWINDOW: 
            DPRINT1("RWMCMD_REDIR_ZORDERWINDOW: Called\n");
            break;
        case RWMCMD_REDIR_UPDATESPRITE:
            DPRINT1("RWMCMD_REDIR_UPDATESPRITE: Called\n");
            break;
        case RWMCMD_REDIR_SHOWWINDOW:
            DPRINT1("RWMCMD_REDIR_SHOWWINDOW: Called\n");
            break;
        case RWMCMD_REDIR_ICONCHANGE:
            DPRINT1("RWMCMD_REDIR_ICONCHANGE: Called\n");
            break;
        case RWMCMD_REDIR_TEXTCHANGE:
            DPRINT1("RWMCMD_REDIR_TEXTCHANGE: Called\n");
            break;
        case RWMCMD_REDIR_STYLECHANGE:
            DPRINT1("RWMCMD_REDIR_STYLECHANGE: Called\n");
            break;
        case RWMCMD_REDIR_ACTIVATIONCHANGE:
            DPRINT1("RWMCMD_REDIR_ACTIVATIONCHANGE: Called\n");
            break;
        case RWMCMD_REDIR_DESKTOPCHANGE:
            DPRINT1("RWMCMD_REDIR_DESKTOPCHANGE: Called\n");
            break;
        case RWMCMD_REDIR_SHELLWINDOWCHANGE:
            DPRINT1("RWMCMD_REDIR_SHELLWINDOWCHANGE: Called\n");
            break;
        case RWMCMD_REDIR_POWERCHANGE:
            DPRINT1("RWMCMD_REDIR_POWERCHANGE: Called\n");
            break;
        case RWMCMD_REDIR_CHILDCREATE:
            DPRINT1("RWMCMD_REDIR_CHILDCREATE: Called\n");
            break;
        case RWMCMD_REDIR_CHILDLINK:
            DPRINT1("RWMCMD_REDIR_CHILDLINK: Called\n");
            break;
        case RWMCMD_REDIR_CHILDUNLINK:
            DPRINT1("RWMCMD_REDIR_CHILDUNLINK: Called\n");
            break;
        case RWMCMD_REDIR_CHILDDESTROY: 
            DPRINT1("RWMCMD_REDIR_CHILDDESTROY: Called\n");
            break;
        case RWMCMD_REDIR_CHILDMOVESIZE:
            DPRINT1("RWMCMD_REDIR_CHILDMOVESIZE: Called\n");
            break;
        case RWMCMD_REDIR_CHILDSTYLECHANGE: 
            DPRINT1("RWMCMD_REDIR_CHILDSTYLECHANGE: Called\n");
            break;
        case RWMCMD_REDIR_CHILDCLIPRGNCHANGE:
            DPRINT1("RWMCMD_REDIR_CHILDCLIPRGNCHANGE: Called\n");
            break;
        case RWMCMD_REDIR_HITTESTQUERY:
            DPRINT1("RWMCMD_REDIR_HITTESTQUERY: Called\n");
            break;
        case RWMCMD_REDIR_MOUSELEAVEWINDOW:
            DPRINT1("RWMCMD_REDIR_MOUSELEAVEWINDOW: Called\n");
            break;
        case RWMCMD_REDIR_CHANGEWINDOWRELATIVE:
            DPRINT1("RWMCMD_REDIR_CHANGEWINDOWRELATIVE: Called\n");
            break;
        case RWMCMD_REDIR_CLIENTAREABLURCHANGEX:
            DPRINT1("RWMCMD_REDIR_CLIENTAREABLURCHANGEX: Called\n");
            break;
        case RWMCMD_REDIR_NCRENDERINGCHANGE:    
            DPRINT1("RWMCMD_REDIR_NCRENDERINGCHANGE: Called\n");
            break;
        case RWMCMD_REDIR_REGISTERTHUMBNAIL:
            DPRINT1("RWMCMD_REDIR_REGISTERTHUMBNAIL: Called\n");
            break;
        case RWMCMD_REDIR_UPDATETHUMBNAILPROPERTIES:
            DPRINT1("RWMCMD_REDIR_UPDATETHUMBNAILPROPERTIES: Called\n");
            break;
        case RWMCMD_REDIR_UNREGISTERTHUMBNAIL:
            DPRINT1("RWMCMD_REDIR_UNREGISTERTHUMBNAIL: Called\n");
            break;
        case RWMCMD_CAPTURE_SCREENBITS:
            DPRINT1("RWMCMD_CAPTURE_SCREENBITS: Called\n");
            break;
        default:
            DPRINT1("Unknown command: %d\n", Command);
            __debugbreak();
            break;
    }
    DumpLpcData(LpcMessage);
    __debugbreak();
    return S_OK;
}

HRESULT WINAPI
MilCompositionEngine_DeinitializePartitionManager();

EXTERN_C
NTSTATUS
WINAPI
MilTransport_ShutDownTransport()
{
    return MilCompositionEngine_DeinitializePartitionManager();
}

HRESULT WINAPI
MilCompositionEngine_InitializePartitionManager(
    int nPriority
    );

EXTERN_C
NTSTATUS
WINAPI
MilTransport_InitializeTransport(int PriorityLevel, HANDLE EventHandle)
{
    return MilCompositionEngine_InitializePartitionManager(PriorityLevel);
}

/* Stubs */
VOID WINAPI
MilTSConnector_Create(VOID)
{
    
}
VOID WINAPI
MilTSConnector_Destroy(VOID)
{

}
VOID WINAPI
MilTSConnector_GetHDCForRemoteWindow(VOID)
{

}
VOID WINAPI
MilTSConnector_ProcessPacket(VOID)
{

}
VOID WINAPI
MilTSConnector_SetScale(VOID)
{

}
VOID WINAPI
MilTSConnector_SetViewport(VOID)
{

}

VOID WINAPI
MilTS_CreateVCThread(VOID)
{

}
VOID WINAPI
MilTS_DestroyVCThread(VOID)
{

}
VOID WINAPI
MilSetMilWindowData(PVOID test,HWND hwnd)
{
    __debugbreak();
}
VOID WINAPI
MilSyncCompositionDevice_Present(VOID)
{
    __debugbreak();
}
