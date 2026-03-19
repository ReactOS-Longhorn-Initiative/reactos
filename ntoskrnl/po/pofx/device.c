/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power device handling mechanism routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/**
 * @brief
 * The global list of PoFx-registered devices. Every device object that
 * calls PoFxRegisterDevice will be linked into this list.
 */
LIST_ENTRY PopFxDeviceList;

/**
 * @brief
 * The spin lock that protects the PoFx device list from concurrent access.
 */
KSPIN_LOCK PopFxDeviceLock;

/**
 * @brief
 * The global list of Platform Extension Plug-ins (PEPs) registered
 * with the Power Framework.
 */
LIST_ENTRY PopFxPluginList;

/**
 * @brief
 * The spin lock that protects the PEP plugin list from concurrent access.
 */
KSPIN_LOCK PopFxPluginLock;

/**
 * @brief
 * The PoFx trace level bitmask. When _POFX_DEBUG_ is non-zero, the
 * POFXTRACE macro uses this value to gate debug messages by category.
 */
ULONG PopFxTraceLevel = _POFX_DEBUG_;

/* PRIVATE FUNCTIONS **********************************************************/

/* EOF */
