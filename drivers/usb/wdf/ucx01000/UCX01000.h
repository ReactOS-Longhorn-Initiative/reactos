/*
 * PROJECT:         ReactOS WDF Modern USB Stack
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         xHCI Driver header
 * COPYRIGHT:       Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <ntddk.h>
#include <windef.h>
#include <hubbusif.h>
//#include <drivers/usbport/usbmport.h>

#include "ntddk.h"
#include "ntstrsafe.h"
#include "wdfcore.h"
#include "wdf.h"
#include <wdfldr.h>
#include "specstrings.h"