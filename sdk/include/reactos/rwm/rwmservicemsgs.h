/*
 * PROJECT:     ReactOS Window Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     RWM DWM Service Compatible LPC Messages
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

typedef enum _RWM_SERIVCE_MSGS
{
    RWM_SERVICE_CONNECT =          0x1,
    RWM_SERVICE_QUERY_PORTNAME =   0x2,
    RWM_CHECK_IF_SESSION_HAS_DWM = 0x40000028,
} RWM_SERIVCE_MSGS, *PRWM_SERIVCE_MSGS;

/* 
 * RWM_SERVICE_CONNECT:
 * _In_  RWMSERVCMD_CONNECT_SESSION_PORT
 * _Out_ RWMSERVCMD_CONNECT_SESSIONINFO
 * Sent by the DWM to the service to establish a connection.
 */
typedef struct  _RWMSERVCMD_CONNECT_SESSION_PORT
{
    WCHAR PortPathStr[MAX_PATH];
} RWMSERVCMD_CONNECT_SESSION_PORT, *PRWMSERVCMD_CONNECT_SESSION_PORT;

typedef struct _RWMSERVCMD_CONNECT_SESSIONINFO
{
    ULONG SessionId;
    ULONG ProcessId;
} RWMSERVCMD_CONNECT_SESSIONINFO, *PRWMSERVCMD_CONNECT_SESSIONINFO;

/* 
 * RWM_SERVICE_PUSH_OBJ:
 * TODO:
 */

/* 
 * RWM_SERVICE_POP_OBJ:
 * TODO:
 */

/* 
 * RWM_SERVICE_QUERY_PORTNAME:
 * _Out_ RWMSERVCMD_CONNECT_SESSION_PORT
 * Sent to the service to get Dwm APIPort info
 * Reuses RWMSERVCMD_CONNECT_SESSION_PORT.
 */