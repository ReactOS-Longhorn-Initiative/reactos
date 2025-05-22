/*
 * PROJECT:     ReactOS DWM Compatibility Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Stub file for dwmredir
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include "dwmredir.h"
//#define NDEBUG
#include <debug.h>

typedef void IDwmRedirectionClient;
typedef void IDwmRedirectionManage;
typedef void DwmRedirectionManager;
typedef void CompositionInfo;

EXTERN_C
HRESULT
WINAPI
DwmVersionCheck(UINT32 Version)
{
    return 0;
}

EXTERN_C
HRESULT
WINAPI
DwmRedirectionManagerInitialize(struct _CompositionInfo * Compinfo ,
                                PVOID Startupinfo ,
                                MIL_MESSAGE * Message,
                                PVOID* Redir)
{
    return 0;
}

EXTERN_C
VOID WINAPI
DwmRedirectionManagerSetClientChannel(MIL_CHANNEL MilCoreHandle)
{

}

