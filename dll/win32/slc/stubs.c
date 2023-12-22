/*
 * PROJECT:     ReactOS DWM Compatibility Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Stub file for slc
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include "slc.h"
//#define NDEBUG
#include <debug.h>

HRESULT WINAPI SLGetWindowsInformationDWORD(
  PCWSTR pwszValueName,
  DWORD  *pdwValue
)
{
    *pdwValue = 0;
    return 0;
}