/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * PURPOSE:
 * COPYRIGHT:       Copyright 2021 Justin Miller (justinmiller100@gmail.com)
 */

#pragma once

/* INCLUDES ******************************************************************/
#include <std.h>


#include "MilResource.hpp"

HRESULT
WINAPI
MilResourceCreateType(MIL_RESOURCE_TYPE type,
                      HMIL_CHANNEL MilChannel,
                      MilResource **MilResourceInstance);
