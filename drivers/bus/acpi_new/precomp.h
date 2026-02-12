#pragma once 

#include <stdio.h>
#include <string.h>
#include <wchar.h>

/*
 * Use repo-relative includes so both the build system and IDE tooling can
 * consistently resolve uACPI headers.
 */
#include "uacpi/include/uacpi/acpi.h"
#include "uacpi/include/uacpi/kernel_api.h"
#include "uacpi/include/uacpi/uacpi.h"
#include "uacpi/include/uacpi/event.h"
#include "uacpi/include/uacpi/utilities.h"

#include <initguid.h>
#include <wdmguid.h>
#include <ntddk.h>
#include <ntifs.h>
#include <mountdev.h>
#include <mountmgr.h>
#include <ketypes.h>
#include <iotypes.h>
#include <rtlfuncs.h>
#include <arc/arc.h>

UINT32
ACPIInitUACPI(void);
