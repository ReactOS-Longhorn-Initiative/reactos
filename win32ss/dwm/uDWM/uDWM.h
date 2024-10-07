/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * PURPOSE:
 * COPYRIGHT:       Copyright 2021 Justin Miller (justinmiller100@gmail.com)
 */

#pragma once

/* INCLUDES ******************************************************************/

#include <stdio.h>
#include <stdlib.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <winnls32.h>
#include <winver.h>

#include <imm.h>
#include <ddk/immdev.h>

#define NTOS_MODE_USER
#include <ndk/umtypes.h>
#include <ndk/pstypes.h>
#include <ndk/rtlfuncs.h>

typedef void IDwmRedirectionClient;
typedef void IDwmRedirectionManage;
typedef void DwmRedirectionManager;
typedef long MilMessageClass;
    //
struct MIL_MESSAGE
{
    MilMessageClass type;
    DWORD dwReserved;
};


#include <dwmapi.h>
#include "dwmredir.h"
#include "../shared/milk.h" 

typedef struct _StartupInfo
{
    DWORD Var_14;
    DWORD LpCrtiicalSection;
    DWORD var_C;
    DWORD var_8;
    DWORD arg_0;
    DWORD arg_4;
} StartupInfo;

typedef struct _CompositionInfo
{
    DWORD Var_14;
    HMIL_CONNECTION hConnection;
    MIL_CHANNEL hSourceChannel;
// HMIL_CONNECTION__ ** MillConnection;;
// MIL_CHANNEL__ ** Millchannel;
    DWORD LpCrtiicalSection;
    DWORD var_C;
    DWORD var_8;
    DWORD arg_0;
    DWORD arg_4;
} CompositionInfo;

EXTERN_C
HRESULT WINAPI
DwmRedirectionManagerInitialize(
           CompositionInfo * CompInfo,
           IDwmRedirectionClient* RedirClient,
           struct MIL_MESSAGE* MilMsg,
           DwmRedirectionManager** Out_RedirMgr);


EXTERN_C
HRESULT WINAPI MilConnection_CreateChannel(
    HMIL_CONNECTION hConnection,
    MIL_CHANNEL hSourceChannel,
    MIL_CHANNEL *phChannel
    );
    
EXTERN_C 
HRESULT WINAPI
MilChannel_GetMarshalType(
    MIL_CHANNEL hChannel,
    UINT32 *pMarshalType
    );
EXTERN_C
    VOID WINAPI
DwmRedirectionManagerSetClientChannel(MIL_CHANNEL MilCoreHandle);

EXTERN_C
HRESULT WINAPI MilChannel_CommitChannel(
    MIL_CHANNEL hChannel
    );