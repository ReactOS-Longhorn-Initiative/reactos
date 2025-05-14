#pragma once


#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/lpcfuncs.h>
#include "../shared/LpcConnectLib/LpcConnectLib.hpp"
#include "../shared/LpcCreateLib/LpcCreateLib.hpp"
#define DWMAPP_NAME L"uxss.exe" //Longhorn 5048-5112

/* Main serice entry */
HRESULT WINAPI
ServiceStartup();

NTSTATUS
WINAPI
SessionBypassInitializeDWM();

BOOL PullPortAPIs(void);


VOID
WINAPI
InitializeServicePort();
