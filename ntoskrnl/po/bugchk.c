/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager graceful bug check support for PoShutdownBugCheck
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Crashes down the system and performs a power action.
 * This is typically used for debugging purposes on forced
 * shutdowns to test the power rundown states.
 *
 * @param[in] LogError
 * If set to TRUE, the function will poke the I/O manager
 * to write a specific log dump describing the reason of the crash.
 *
 * @param[in] BugCheckCode
 * The main bugcheck value that indicates the reason of the crash.
 *
 * @param[in] BugCheckParameter1
 * The additional parameter for the bugcheck indicating the reason
 * of the crash.
 *
 * @param[in] BugCheckParameter2
 * The additional 2nd parameter for the bugcheck indicating the
 * reason of the crash.
 *
 * @param[in] BugCheckParameter3
 * The additional 3rd parameter for the bugcheck indicating the
 * reason of the crash.
 *
 * @param[in] BugCheckParameter4
 * The additional 4th parameter for the bugcheck indicating the
 * reason of the crash.
 */
VOID
NTAPI
PoShutdownBugCheck(
    _In_ BOOLEAN LogError,
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1,
    _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3,
    _In_ ULONG_PTR BugCheckParameter4)
{
    POP_SHUTDOWN_BUG_CHECK BugCode;

    /*
     * If a crash dump is not to be allowed for this bugcheck, disable it first
     * before crashing the system.
     *
     * FIXME: On Windows this calls IoConfigureCrashDump(CrashDumpDisable, NULL)
     * to suppress the crash dump. ReactOS does not yet implement this internal
     * I/O Manager interface, so the dump suppression is skipped for now.
     */
    UNREFERENCED_PARAMETER(LogError);

    /* Capture the thread and process identity that submitted this bugcheck request */
    BugCode.ThreadId = PsGetCurrentThreadId();
    BugCode.ProcessId = PsGetCurrentProcessId();

    /* Capture the bugcheck code and its extra parameters */
    BugCode.Code = BugCheckCode;
    BugCode.Parameter1 = BugCheckParameter1;
    BugCode.Parameter2 = BugCheckParameter2;
    BugCode.Parameter3 = BugCheckParameter3;
    BugCode.Parameter4 = BugCheckParameter4;

    /*
     * Cache the bugcheck reasoning into the global power action structure
     * so that, once the system initiates the power action to shut it down,
     * it knows it was because of this bugcheck.
     */
    PopAction.ShutdownBugCode = &BugCode;

    /*
     * Initiate a graceful power-down shutdown due to this bugcheck.
     * Use POWER_ACTION_CRITICAL so that this action is prioritized above
     * anything else and cannot be blocked.
     */
    ZwInitiatePowerAction(PowerActionShutdown,
                          PowerSystemSleeping3,
                          POWER_ACTION_CRITICAL,
                          FALSE);

    /* We should not be alive by this point. If we somehow are, crash hard */
    KeBugCheckEx(BugCheckCode,
                 BugCheckParameter1,
                 BugCheckParameter2,
                 BugCheckParameter3,
                 BugCheckParameter4);
}

/* EOF */
