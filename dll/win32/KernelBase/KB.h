/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/win32/kernel32/k32.h
 * PURPOSE:         Win32 Kernel Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef __KB_H
#define __KB_H

/* INCLUDES ******************************************************************/

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <wincon.h>
#include <winuser.h>

/* Redefine NTDDI_VERSION to 2K3 SP1 to get correct NDK definitions */
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1

#define NTOS_MODE_USER
#include <ndk/kefuncs.h>


#endif /* __KB_H */