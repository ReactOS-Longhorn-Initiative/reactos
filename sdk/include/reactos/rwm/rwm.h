/*
 * PROJECT:     ReactOS Window Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     RWM Shared Headers
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */
#pragma once

/* PSDK/NDK Headers */
#include <stdio.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#define NTOS_MODE_USER
#include <ndk/obtypes.h>
#include <ndk/obfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/lpcfuncs.h>
#include <ndk/umfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#define RWMUXSMS_NAME               L"UxSms"
#define RWMUXSMS_APIPORTDESCRIPTION L"User Experience Session Management Service API Port"
#define RWMUXSMS_APIPORTDESCRIPTIONLEN (sizeof(RWMUXSMS_APIPORTDESCRIPTION)/sizeof(WCHAR))

#define RWMUXSMS_APIPORTNAME        L"\\UxSmsApiPort"
#define RWMUXSMS_APIPORTNAMELEN     (sizeof(RWMUXSMS_APIPORTNAME)/sizeof(WCHAR))

#define RWMAPP_NAME             L"uxss.exe"
#define RWMAPP_NAMELEN          (sizeof(RWM_APPNAME)/sizeof(WCHAR))
#define RWMAPP_WINDOWDESC       L"UXSS Notification Window"
#define RWMAPP_WINDOWNAMELEN    (sizeof(RWM_APPWINDOWDESC)/sizeof(WCHAR))
#define RWMAPP_WINDOWCLASS      L"UxSsWindowClass"


#define RWM_MAX_MESSAGE_DATA   (0x130)

typedef struct _LPC_RWM_MESSAGE
{
    PORT_MESSAGE Header;
    ULONG        Message;
    ULONG        Status;                     /* - Message  -       Status */
    BYTE         Data[RWM_MAX_MESSAGE_DATA - sizeof(ULONG) - sizeof(ULONG)];
} LPC_RWM_MESSAGE, *PLPC_RWM_MESSAGE;

/*
 * These operations currently target Longhorn 5112
 */
#include <rwmcmdmsgs.h>
#include <rwmservicemsgs.h>
