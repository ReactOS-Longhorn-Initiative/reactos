
/* INCLUDES *****************************************************************/
#include "uxsms.h"
#include <strsafe.h>
#include <wtsapi32.h>

#include <debug.h>

/* GLOBALS ******************************************************************/

WCHAR ProcessPath[MAX_PATH];
HANDLE GlobalDwmWaitHandle;
/* FUNCTIONS *****************************************************************/

static
VOID
InitializeDwmProcessPath(VOID)
{
    WCHAR szSysDir[MAX_PATH];

    GetSystemDirectoryW(szSysDir, _countof(szSysDir));
    StringCchPrintfW(ProcessPath, _countof(ProcessPath), L"%s\\%s", szSysDir, DWMAPP_NAME);
    DPRINT1("InitializeDwmProcessPath: Path is %ls\n", ProcessPath);
}

static
void
CALLBACK
SessionBypassHandleDwmExit(void *p, BOOLEAN timeout)
{
    DPRINT1("SessionBypassHandleDwmExit: Normally, DWM would reset. Here it won't\n");
}

/*
 * This is a massive hack..
 * This should happen
 */
NTSTATUS
WINAPI
SessionBypassInitializeDWM()
{
    STARTUPINFOW StartupInfo = {0};
    PROCESS_INFORMATION ProcessInfo = {0};

    HANDLE TokenHandle;
    InitializeDwmProcessPath();

    if (!WTSQueryUserToken(0, &TokenHandle))
    {
        DPRINT1("Failed to get UserToken\n");
        return 1;
    }

    StartupInfo.cb = sizeof(STARTUPINFOW);
    StartupInfo.lpDesktop = (LPWSTR)L"WinSta0\\Default";
    if (!CreateProcessW(ProcessPath,
                        NULL,
                        NULL,
                        NULL,
                        FALSE,
                        CREATE_DEFAULT_ERROR_MODE | CREATE_UNICODE_ENVIRONMENT
                        | NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInfo))
    {
        DPRINT1("Failed to create process. Error: %lu\n", GetLastError());
        return 1;
    }

    DPRINT1("Process created successfully. PID: %lu\n", ProcessInfo.dwProcessId);
    RegisterWaitForSingleObject(&GlobalDwmWaitHandle,
                                 ProcessInfo.hProcess,
                                 SessionBypassHandleDwmExit,
                                 NULL,
                                 INFINITE,
                                 WT_EXECUTEONLYONCE);
    DPRINT1("Now starting DWM.EXE/UXSS.EXE\n");
    ResumeThread(ProcessInfo.hThread);
    return 0;
}
