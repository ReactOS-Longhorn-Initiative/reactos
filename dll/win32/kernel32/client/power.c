/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/power.c
 * PURPOSE:         Power Management Functions
 * PROGRAMMER:      Dmitry Chapyshev <dmitry@reactos.org>
 *
 * UPDATE HISTORY:
 *                  01/15/2009 Created
 */

#include <k32.h>

#include <ndk/pofuncs.h>

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/
typedef struct _REASON_CONTEXT
{
    ULONG Version;
    DWORD Flags;
    union
    {
        struct
        {
            HMODULE LocalizedReasonModule;
            ULONG LocalizedReasonId;
            ULONG ReasonStringCount;
            LPWSTR *ReasonStrings;
        } Detailed;
        LPWSTR SimpleReasonString;
    } Reason;
} REASON_CONTEXT, *PREASON_CONTEXT;

typedef enum _POWER_REQUEST_TYPE
{
    PowerRequestDisplayRequired,
    PowerRequestSystemRequired,
    PowerRequestAwayModeRequired
} POWER_REQUEST_TYPE, *PPOWER_REQUEST_TYPE;
DEBUG_CHANNEL(wine);
/***********************************************************************
 *           GetSystemPowerStatus      (KERNEL32.@)
 */
BOOL WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS ps)
{
    SYSTEM_BATTERY_STATE bs;
    NTSTATUS status;

    TRACE("(%p)\n", ps);

    ps->ACLineStatus        = AC_LINE_UNKNOWN;
    ps->BatteryFlag         = BATTERY_FLAG_UNKNOWN;
    ps->BatteryLifePercent  = BATTERY_PERCENTAGE_UNKNOWN;
    ps->SystemStatusFlag    = 0;
    ps->BatteryLifeTime     = BATTERY_LIFE_UNKNOWN;
    ps->BatteryFullLifeTime = BATTERY_LIFE_UNKNOWN;

    status = NtPowerInformation(SystemBatteryState, NULL, 0, &bs, sizeof(bs));
    if (status == STATUS_NOT_IMPLEMENTED) return TRUE;
    if (FAILED(status)) return FALSE;

    ps->ACLineStatus = bs.AcOnLine;

    if (bs.BatteryPresent)
    {
        ps->BatteryLifePercent = bs.MaxCapacity ? 100 * bs.RemainingCapacity / bs.MaxCapacity : 100;
        ps->BatteryLifeTime = bs.EstimatedTime;
        if (!bs.Charging && (LONG)bs.Rate < 0)
            ps->BatteryFullLifeTime = 3600 * bs.MaxCapacity / -(LONG)bs.Rate;

        ps->BatteryFlag = 0;
        if (bs.Charging)
            ps->BatteryFlag |= BATTERY_FLAG_CHARGING;
        if (ps->BatteryLifePercent > 66)
            ps->BatteryFlag |= BATTERY_FLAG_HIGH;
        if (ps->BatteryLifePercent < 33)
            ps->BatteryFlag |= BATTERY_FLAG_LOW;
        if (ps->BatteryLifePercent < 5)
            ps->BatteryFlag |= BATTERY_FLAG_CRITICAL;
    }
    else
    {
        ps->BatteryFlag = BATTERY_FLAG_NO_BATTERY;
    }

    return TRUE;
}

/***********************************************************************
 *           PowerCreateRequest      (KERNEL32.@)
 */
HANDLE WINAPI PowerCreateRequest(REASON_CONTEXT *context)
{
    FIXME("(%p): stub\n", context);

    return CreateEventW(NULL, TRUE, FALSE, NULL);
}

/***********************************************************************
 *           PowerSetRequest      (KERNEL32.@)
 */
BOOL WINAPI PowerSetRequest(HANDLE request, POWER_REQUEST_TYPE type)
{
    FIXME("(%p, %u): stub\n", request, type);

    return TRUE;
}

/***********************************************************************
 *           PowerClearRequest      (KERNEL32.@)
 */
BOOL WINAPI PowerClearRequest(HANDLE request, POWER_REQUEST_TYPE type)
{
    FIXME("(%p, %u): stub\n", request, type);

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetSystemPowerState(IN BOOL fSuspend,
                    IN BOOL fForce)
{
    NTSTATUS Status;

    Status = NtInitiatePowerAction((fSuspend != FALSE) ? PowerActionSleep     : PowerActionHibernate,
                                   (fSuspend != FALSE) ? PowerSystemSleeping1 : PowerSystemHibernate,
                                   (fForce == FALSE),
                                   FALSE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDevicePowerState(IN HANDLE hDevice,
                    OUT BOOL *pfOn)
{
    DEVICE_POWER_STATE DevicePowerState;
    NTSTATUS Status;

    Status = NtGetDevicePowerState(hDevice, &DevicePowerState);
    if (NT_SUCCESS(Status))
    {
        *pfOn = (DevicePowerState == PowerDeviceUnspecified) ||
                (DevicePowerState == PowerDeviceD0);
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RequestDeviceWakeup(IN HANDLE hDevice)
{
    NTSTATUS Status;

    Status = NtRequestDeviceWakeup(hDevice);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RequestWakeupLatency(IN LATENCY_TIME latency)
{
    NTSTATUS Status;

    Status = NtRequestWakeupLatency(latency);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CancelDeviceWakeupRequest(IN HANDLE hDevice)
{
    NTSTATUS Status;

    Status = NtCancelDeviceWakeupRequest(hDevice);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsSystemResumeAutomatic(VOID)
{
    return (BOOL)NtIsSystemResumeAutomatic();
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMessageWaitingIndicator(IN HANDLE hMsgIndicator,
                           IN ULONG ulMsgCount)
{
    /* This is the correct Windows implementation */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
EXECUTION_STATE
WINAPI
SetThreadExecutionState(EXECUTION_STATE esFlags)
{
    NTSTATUS Status;

    Status = NtSetThreadExecutionState(esFlags, &esFlags);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return esFlags;
}
