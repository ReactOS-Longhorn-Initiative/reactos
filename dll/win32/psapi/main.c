/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/misc/win32.c
 * PURPOSE:     Win32 interfaces for PSAPI
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 *              Thomas Weidenmueller <w3seek@reactos.com>
 *              Pierre Schweitzer <pierre@reactos.org>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 */

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <psapi.h>

#include <pseh/pseh2.h>

#define NDEBUG
#include <debug.h>

#define MAX_MODULES 0x2710      // Matches 10.000 modules
#define INIT_MEMORY_SIZE 0x1000 // Matches 4kB

/*
 * @unimplemented
 */
static VOID NTAPI
PsParseCommandLine(VOID)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
static VOID NTAPI
PsInitializeAndStartProfile(VOID)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
static VOID NTAPI
PsStopAndAnalyzeProfile(VOID)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN
WINAPI
DllMain(HINSTANCE hDllHandle,
        DWORD nReason,
        LPVOID Reserved)
{
    switch(nReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hDllHandle);
            if (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_PROFILE_USER)
            {
                PsParseCommandLine();
                PsInitializeAndStartProfile();
            }
            break;

        case DLL_PROCESS_DETACH:
            if (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_PROFILE_USER)
            {
                PsStopAndAnalyzeProfile();
            }
            break;
    }

    return TRUE;
}