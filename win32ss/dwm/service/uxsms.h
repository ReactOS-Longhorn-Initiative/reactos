/*
 * PROJECT:     RWM UxSms Service
 * LICENSE:     MIT (https://opensource.org/licenses/MIT)
 * PURPOSE:     UxSms Unified Service Header
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#define WIN32_NO_STATUS
#include <windows.h>
#include <versionhelpers.h>
#include <rwm.h>
#include <LpcConnectLib.hpp>
#include <LpcCreateLib.hpp>

extern WCHAR* DwmSessionPort;

/* Main serice entry */
HRESULT
WINAPI
ServiceStartup();

VOID
WINAPI
InitializeServicePort();

VOID
WINAPI
DestroyServicePort();

NTSTATUS
WINAPI
SessionBypassInitializeDWM();

