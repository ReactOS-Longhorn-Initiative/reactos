
#include "dwr.hpp"
#include <ndk/lpcfuncs.h>
#include "../shared/LpcConnectLib/LpcConnectLib.hpp"
#include <strsafe.h>
LpcConnectLib* lpcConnectLib;
extern WCHAR PortName[MAX_PATH];

#define DWMUXSMS_APIPORTDESCRIPTION L"User Experience SubSystem API Port"
static WCHAR PortNameUxSms[] = L"\\UxSmsApiPort";
#define DWM_CONNECT_TO_SERVICE 0x00000001

typedef struct _SESSION_INIT
{
    ULONG SessionId;
    ULONG ProcessId;
} SESSION_INIT, *PSESSION_INIT;

typedef struct  _SESSION_PORTPATH
{
    WCHAR PortPathStr[MAX_PATH];
} SESSION_PORTPATH, *PSESSION_PORTPATH;

VOID
WINAPI
RWMConnectToUxServ()
{
    SESSION_PORTPATH PortPath = {0};
    wcscpy_s(PortPath.PortPathStr, _countof(PortName), PortName);
    SESSION_INIT SessionInit;
    HRESULT ReturnHr;
    /* Initlaize the LpcConnectLib class*/
    lpcConnectLib = new LpcConnectLib();
    /* Initialize the connection to the service */
    lpcConnectLib->ConnectToPortString(PortNameUxSms, DWMUXSMS_APIPORTDESCRIPTION);
    lpcConnectLib->SendComplexSyncRequest(
        DWM_CONNECT_TO_SERVICE,
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