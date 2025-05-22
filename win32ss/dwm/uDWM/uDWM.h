/*
 * PROJECT:     RWM - User ReactOS Window Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:        
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/* INCLUDES ******************************************************************/

#include <std.h>


/* This seems to be something we don't support? */
DECLARE_HANDLE(HSPRITE);

typedef struct _REMOTE_PORT_VIEW
{
    ULONG Length;
    SIZE_T ViewSize;
    PVOID ViewBase;
} REMOTE_PORT_VIEW, *PREMOTE_PORT_VIEW;

#include <rwmcmdmsgs.h>


typedef struct _IDwmRedirectionManager
{
  PVOID (__thiscall *LookupSprite)(PVOID ClassData, HSPRITE Sprite);
  PVOID (__thiscall *LookupWindow)(PVOID ClassData, HWND hWnd);
} IDwmRedirectionManager;


typedef struct _RWM_STARTUPINFO
{
    BOOLEAN AreWeRemoting;
    BOOLEAN AreWeBitmapRemoting;
    DWORD   ConnectionGeneration;
    DWORD   MaxTextureWidth;
    DWORD   MaxTextureHeight;
    BOOLEAN DoesPowerStateReccommendspaqueBlend;
    HRESULT (__fastcall *pfnNotificationCallback)(const MIL_MESSAGE*);
    PVOID* pSettingsManager;
} RWM_STARTUPINFO, *PRWM_STARTUPINFO;

typedef struct _RWM_COMPOSITIONINFO
{
    HMIL_CONNECTIONMANAGER hConnectionManager;
    HMIL_CONNECTION hConnection;
    MIL_CHANNEL hRedirectionStateChannel;
    BOOLEAN IsDesktopCompositionActive;
} RWM_COMPOSITIONINFO, *PRWM_COMPOSITIONINFO;

#include <milcoreexports.hpp>
#include "MilResource.hpp"
#include <CompositedWindow.hpp>

HRESULT
WINAPI
MilResourceCreateType(MIL_RESOURCE_TYPE type,
                      HMIL_CHANNEL MilChannel,
                      MilResource **MilResourceInstance);

HRESULT
WINAPI
MilResourceAdaptType(MIL_CHANNEL MilChannel,
                     HMIL_RESOURCE ResourceHandle,
                     MilResource **MilResourceInstance);

#include "DwmDesktop.hpp"
#include "WindowList.hpp"

HRESULT
WINAPI
CreateDwmDesktop(PRWM_STARTUPINFO StartupInfo, PRWM_COMPOSITIONINFO CompInfo);

#include "DwmVisual.hpp"
#include "WindowClientData.hpp"
