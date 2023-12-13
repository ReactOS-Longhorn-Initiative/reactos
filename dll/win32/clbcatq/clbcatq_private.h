/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * PURPOSE:         ReactOS Desktop Window Manager API header
 * COPYRIGHT:       Copyright 2021 Justin Miller (justinmiller100@gmail.com)
 */

#ifndef __clbcatq_H
#define __clbcatq_H

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

/* Public Win32K Headers */
#include <ntuser.h>

#endif /* __clbcatq_H */
