
#include "rwm.hpp"

#include <strsafe.h>
LpcConnectLib* lpcConnectLib;
extern WCHAR PortName[MAX_PATH];

VOID
WINAPI
RWMConnectToUxServ()
{
    RWMSERVCMD_CONNECT_SESSION_PORT PortPath = {0};
    wcscpy(PortPath.PortPathStr, PortName);
    RWMSERVCMD_CONNECT_SESSIONINFO SessionInit;
    HRESULT ReturnHr;
    /* Initlaize the LpcConnectLib class*/
    lpcConnectLib = new LpcConnectLib();
    /* Initialize the connection to the service */
    lpcConnectLib->ConnectToPortString(RWMUXSMS_APIPORTNAME, RWMUXSMS_APIPORTDESCRIPTION);
    lpcConnectLib->SendComplexSyncRequest(
        RWM_SERVICE_CONNECT,
        &PortPath,
        sizeof(PortPath),
        &SessionInit,
        sizeof(SessionInit),
        &ReturnHr);

    if (FAILED(ReturnHr))
    {
        DbgPrint("Failed to connect to UxSms service: %lx\n", ReturnHr);
        return;
    }

    DbgPrint("Connected to UxSms service, SessionId: %d, ProcessId: %d\n", SessionInit.SessionId, SessionInit.ProcessId);
    __debugbreak();
}
