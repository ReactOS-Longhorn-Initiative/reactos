#pragma once

#include <windows.h>

#define DWMAPP_NAME L"uxss.exe" //Longhorn 5048-5112

/* Main serice entry */
HRESULT WINAPI
ServiceStartup();

NTSTATUS
WINAPI
SessionBypassInitializeDWM();
