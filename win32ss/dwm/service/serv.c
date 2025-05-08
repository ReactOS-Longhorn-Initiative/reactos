/*
 * PROJECT:     ReactOS DWM Service
 * LICENSE:     MIT (https://opensource.org/licenses/MIT)
 * PURPOSE:     Entry point for the DWM service
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "uxsms.h"
#include <debug.h>

/* GLOBALS ******************************************************************/

static WCHAR ServiceName[] = L"UxSms";

SERVICE_STATUS_HANDLE ServiceStatusHandle;
SERVICE_STATUS ServiceStatus;

#ifndef SERVICE_CONTROL_SESSIONCHANGE
#define SERVICE_CONTROL_SESSIONCHANGE          0x0000000E
#endif

#ifndef WTS_CONSOLE_CONNECT
#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#endif

/* FUNCTIONS *****************************************************************/

static VOID
UpdateServiceStatus(DWORD dwState)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwCurrentState = dwState;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SESSIONCHANGE |
                                       SERVICE_ACCEPT_STOP |
                                       SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(ServiceStatusHandle,
                     &ServiceStatus);
}

HRESULT WINAPI
ServiceShutdown()
{
    DPRINT1("ServiceShutdown() called\n");
    UpdateServiceStatus(SERVICE_STOP_PENDING);
    // Perform any necessary cleanup here
    // This is where you would free resources or save state

    /* Disconnect LPC port */
    // Set the service status to stopped
    UpdateServiceStatus(SERVICE_STOPPED);
    return S_OK;
}

static DWORD WINAPI
ServiceHandleSessionEvents(DWORD dwEventType,
                           LPVOID lpEventData,
                           LPVOID lpContext)
{
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

static DWORD WINAPI
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
        /* This service aspect of this dll is primarily to track session changes */
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
    DPRINT1("ServiceStartup() called\n");
    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        DPRINT1("RegisterServiceCtrlHandlerExW failed\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }
    DPRINT1("RegisterServiceCtrlHandlerExW succeeded\n");
    
    UpdateServiceStatus(SERVICE_START_PENDING);
    /* Start port here */
    UpdateServiceStatus(SERVICE_RUNNING);
    return S_OK;
}