/*
 * PROJECT:     ReactOS DWM Compatibility Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Stub file for dwmredir
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include "dwmredir.h"
//#define NDEBUG
#include <debug.h>

HRESULT
WINAPI
DwmVersionCheck(UINT32 Version);
{
    return 0;
}


typedef void IDwmRedirectionClient;
typedef void IDwmRedirectionManage;
typedef void DwmRedirectionManager;
typedef long MilMessageClass;
typedef void CompositionInfo;
    //
struct MIL_MESSAGE
{
    MilMessageClass type;
    DWORD dwReserved;
};

long WINAPI DwmRedirectionManagerInitialize(struct _CompositionInfo *,void *,struct MIL_MESSAGE *,void * *)
{
    return 0;
}

typedef PVOID MIL_CHANNEL;

VOID WINAPI
DwmRedirectionManagerSetClientChannel(MIL_CHANNEL MilCoreHandle)
{

}