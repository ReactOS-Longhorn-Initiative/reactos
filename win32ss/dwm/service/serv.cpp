/*
 * PROJECT:     RWM UxSms Service
 * LICENSE:     MIT (https://opensource.org/licenses/MIT)
 * PURPOSE:     UxSms service side logic
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <uxsms.h>
#include <winsvc.h>
#include <wtsapi32.h>
#include <svc.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

SERVICE_STATUS_HANDLE GlobalServiceStatusHandle;
SERVICE_STATUS GlobalServiceStatus = {0};
PSVCHOST_GLOBAL_DATA lpServiceGlobals;

/* FUNCTIONS *****************************************************************/

EXTERN_C
VOID
WINAPI
SvchostPushServiceGlobals(
    _In_ PSVCHOST_GLOBAL_DATA lpGlobals)
{
    DPRINT1("SvchostPushServiceGlobals(%p)\n", lpGlobals);
    lpServiceGlobals = lpGlobals;
}

static
VOID
UpdateServiceStatus(DWORD dwState)
{
    GlobalServiceStatus = {0};

    GlobalServiceStatus.dwServiceType = SERVICE_WIN32;
    GlobalServiceStatus.dwCurrentState = dwState;
    GlobalServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SESSIONCHANGE |
                                       SERVICE_ACCEPT_STOP |
                                       SERVICE_ACCEPT_SHUTDOWN;

    SetServiceStatus(GlobalServiceStatusHandle,
                     &GlobalServiceStatus);
}

HRESULT
WINAPI
ServiceShutdown()
{
    DPRINT("ServiceShutdown() called\n");
    UpdateServiceStatus(SERVICE_STOP_PENDING);
    // Perform any necessary cleanup here
    // This is where you would free resources or save state

    /* Disconnect LPC port */
    // Set the service status to stopped
    UpdateServiceStatus(SERVICE_STOPPED);
    return S_OK;
}

static
DWORD
WINAPI
ServiceHandleSessionEvents(DWORD dwEventType,
                           LPVOID lpEventData,
                           LPVOID lpContext)
{
    /*
     * ReactOS win32ss doesn't support this behavior
     * A huge part of what this file does is deal with the events below.
     * Realistically these won't matter for ReactOS till we have Vista+ style
     * Multisession support. Sadly in order to Really get this file working on
     * Vista we need a impl.
     */

    switch (dwEventType)
    {
        case WTS_CONSOLE_CONNECT:
            DPRINT1("Console connect event received\n");
            break;
        case WTS_CONSOLE_DISCONNECT:
            DPRINT1("Console disconnect event received\n");
            break;
        case WTS_REMOTE_CONNECT:
            DPRINT1("Remote connect event received\n");
            break;
        case WTS_REMOTE_DISCONNECT: 
            DPRINT1("Remote disconnect event received\n");
            break;
        case WTS_SESSION_LOGON:
            DPRINT1("Session logon event received\n");
            SessionBypassInitializeDWM();
            break;
        case WTS_SESSION_LOGOFF:
            DPRINT1("Session logoff event received\n");
            break;
        default:
            DPRINT1("Unknown session event received: %lu\n", dwEventType);
            break;
    }

    return ERROR_SUCCESS;
}

static
DWORD
WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    DPRINT1("ServiceControlHandler() called\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
        {
            DPRINT1("ServiceControlHandler: Control to Stop/Shutdown received\n");
            ServiceShutdown();
            return ERROR_SUCCESS;
        }
        case SERVICE_CONTROL_SESSIONCHANGE:
        {
            DPRINT1("ServiceControlHandler: Control to SessionChange received\n");
            // Handle session change events here
            // This is where you would respond to session changes, like logon/logoff
            return ServiceHandleSessionEvents(dwEventType,
                                              lpEventData,
                                              lpContext);
        }
        default:
        {
            DPRINT1("ServiceControlHandler:  Control %lu received\n");
            return ERROR_SUCCESS;
        }
    }
}

HRESULT WINAPI
ServiceStartup()
{
    DPRINT("ServiceStartup() called\n");
    GlobalServiceStatusHandle = RegisterServiceCtrlHandlerExW(RWMUXSMS_NAME,
                                                              ServiceControlHandler,
                                                              NULL);
    if (!GlobalServiceStatusHandle)
    {
        DPRINT("RegisterServiceCtrlHandlerExW failed\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }
    DPRINT("RegisterServiceCtrlHandlerExW succeeded\n");

    UpdateServiceStatus(SERVICE_START_PENDING);

    /* Start port here (UxSmsApiPort)*/
    InitializeServicePort();

    UpdateServiceStatus(SERVICE_RUNNING);
    // TODO: HACK: ReactOS has no support for Multisession, so let's manually start 
    /* Start UX.SS/DWM.EXE
     * This is actually NOT correct. 
     * The DWM service should be started  at the logon event
     * Here we are just starting it as a hack.
     * The act of declerating the service running SHOULD result in the thing begin done at logon
     * 
     * 
     * Some weird observations:
     * -> This is probably the result of the black fade to desktop functionality. and is why
     * this is even possible. 
     * -> Logonui, putting the passwords in, then right when logon is accepted is likely
     * when this is fired.
     * -> So DWM startsup at this point if all lights are green behind the screen and fade in 
     * only occurs after it finishes.
     */
    if (IsReactOS())
    {
        SessionBypassInitializeDWM();
    }

    return S_OK;
}
