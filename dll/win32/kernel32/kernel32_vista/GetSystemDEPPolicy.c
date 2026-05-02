/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of GetSystemDEPPolicy
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "k32_vista.h"

DEP_SYSTEM_POLICY_TYPE
WINAPI
GetSystemDEPPolicy(VOID)
{
    return (DEP_SYSTEM_POLICY_TYPE)SharedUserData->NXSupportPolicy;
}
