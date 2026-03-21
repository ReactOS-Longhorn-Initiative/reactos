/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor core parking — affinity mask vs scheduler (SMP)
 *
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <internal/ppm.h>
#include <internal/po.h>

/*
 * Bits set in PpmCoreParkingParkMask = logical processors treated as “parked”
 * for *new* scheduling decisions (KiSelectNextProcessor / KiFindIdealProcessor).
 * If masking would eliminate every CPU in the candidate set, the mask is ignored.
 */
volatile KAFFINITY PpmCoreParkingParkMask = 0;

volatile ULONG PpmCoreParkingMinCores = 1;
volatile ULONG PpmCoreParkingMaxCores = 0; /* 0 = use all logical processors */

volatile UCHAR PpmCoreParkingBusyIncreaseThreshold = 85;
volatile UCHAR PpmCoreParkingBusyDecreaseThreshold = 45;
volatile ULONG PpmCoreParkingIncreaseTime = 4;
volatile ULONG PpmCoreParkingDecreaseTime = 4;

/*
 * Optional override from PEP ParkMaskNotification (Version 1 struct below).
 * When PpmCoreParkingPepOverrideActive is TRUE, PpmCoreParkingPepParkMask
 * replaces the policy-derived park mask in the scheduler hook.
 */
volatile BOOLEAN PpmCoreParkingPepOverrideActive = FALSE;
volatile KAFFINITY PpmCoreParkingPepParkMask = 0;
volatile ULONG PpmCoreParkingPepPreferencePercent = 100;

static ULONG PpmCoreParkingTargetActive;
static ULONG PpmCoreParkingBusyHighStreak;
static ULONG PpmCoreParkingBusyLowStreak;

static VOID
PpmCoreParkingRecomputeMaskLocked(VOID)
{
    ULONG n = (ULONG)KeNumberProcessors;
    ULONG maxC = PpmCoreParkingMaxCores;
    ULONG minC = PpmCoreParkingMinCores;
    ULONG active;
    KAFFINITY unparked = 0;
    ULONG i;

    if (n == 0)
    {
        PpmCoreParkingParkMask = 0;
        return;
    }

    if (maxC == 0 || maxC > n)
        maxC = n;
    if (minC < 1)
        minC = 1;
    if (minC > n)
        minC = n;
    if (maxC < minC)
        maxC = minC;

    active = PpmCoreParkingTargetActive;
    if (active < minC)
        active = minC;
    if (active > maxC)
        active = maxC;

    for (i = 0; i < active && i < n; i++)
        unparked |= (KAFFINITY)1 << i;

    PpmCoreParkingParkMask = (KeActiveProcessors & ~unparked);
}

VOID
NTAPI
PpmCoreParkingInitialize(VOID)
{
    ULONG n = (ULONG)KeNumberProcessors;

    if (n == 0)
        n = 1;

    PpmCoreParkingTargetActive = n;
    PpmCoreParkingBusyHighStreak = 0;
    PpmCoreParkingBusyLowStreak = 0;
    PpmCoreParkingRecomputeMaskLocked();
}

VOID
NTAPI
PpmCoreParkingSetMinMaxCores(
    _In_ ULONG MinCores,
    _In_ ULONG MaxCores)
{
    if (MinCores >= 1)
        PpmCoreParkingMinCores = MinCores;
    PpmCoreParkingMaxCores = MaxCores;
    PpmCoreParkingRecomputeMaskLocked();
}

VOID
NTAPI
PpmCoreParkingSetPolicyDword(
    _In_ LPCGUID SettingGuid,
    _In_ ULONG Value)
{
    if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_CORE_PARKING_MAX_CORES))
    {
        PpmCoreParkingMaxCores = Value;
    }
    else if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_CORE_PARKING_MIN_CORES))
    {
        PpmCoreParkingMinCores = (Value < 1) ? 1 : Value;
    }
    else if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD))
    {
        if (Value <= 100)
            PpmCoreParkingBusyIncreaseThreshold = (UCHAR)Value;
    }
    else if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD))
    {
        if (Value <= 100)
            PpmCoreParkingBusyDecreaseThreshold = (UCHAR)Value;
    }
    else if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME))
    {
        PpmCoreParkingIncreaseTime = max(Value, 1);
    }
    else if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME))
    {
        PpmCoreParkingDecreaseTime = max(Value, 1);
    }

    PpmCoreParkingRecomputeMaskLocked();
}

VOID
NTAPI
PpmCoreParkingRefreshMask(VOID)
{
    PpmCoreParkingRecomputeMaskLocked();
}

/*
 * Called once per perf-DPC period on processor 0.  Adjusts how many logical
 * processors stay unparked using average busy % and hysteresis counters.
 */
VOID
NTAPI
PpmCoreParkingPeriodicRebalance(VOID)
{
    ULONG n = (ULONG)KeNumberProcessors;
    ULONG i;
    ULONG sum = 0;
    ULONG avg;
    ULONG maxC, minC, active;
    UCHAR hi, lo;

    if (n <= 1)
        return;

    for (i = 0; i < n; i++)
    {
        PKPRCB Prcb = KiProcessorBlock[i];
        if (Prcb != NULL)
            sum += (ULONG)Prcb->PowerState.LastBusyPercentage;
    }
    avg = sum / n;

    hi = PpmCoreParkingBusyIncreaseThreshold;
    lo = PpmCoreParkingBusyDecreaseThreshold;
    if (lo >= hi)
        lo = (hi > 0) ? (UCHAR)(hi - 1) : 0;

    maxC = PpmCoreParkingMaxCores;
    minC = PpmCoreParkingMinCores;
    if (maxC == 0 || maxC > n)
        maxC = n;
    if (minC < 1)
        minC = 1;
    if (minC > n)
        minC = n;
    if (maxC < minC)
        maxC = minC;

    active = PpmCoreParkingTargetActive;
    if (active < minC)
        active = minC;
    if (active > maxC)
        active = maxC;

    if ((UCHAR)avg >= hi)
    {
        PpmCoreParkingBusyHighStreak++;
        PpmCoreParkingBusyLowStreak = 0;
        if (PpmCoreParkingBusyHighStreak >= PpmCoreParkingIncreaseTime &&
            active < maxC)
        {
            active++;
            PpmCoreParkingBusyHighStreak = 0;
        }
    }
    else if ((UCHAR)avg <= lo)
    {
        PpmCoreParkingBusyLowStreak++;
        PpmCoreParkingBusyHighStreak = 0;
        if (PpmCoreParkingBusyLowStreak >= PpmCoreParkingDecreaseTime &&
            active > minC)
        {
            active--;
            PpmCoreParkingBusyLowStreak = 0;
        }
    }
    else
    {
        PpmCoreParkingBusyHighStreak = 0;
        PpmCoreParkingBusyLowStreak = 0;
    }

    PpmCoreParkingTargetActive = active;
    PpmCoreParkingRecomputeMaskLocked();
}

KAFFINITY
NTAPI
PpmCoreParkingApplySchedulerMask(
    _In_ KAFFINITY CandidateSet)
{
    KAFFINITY park;
    KAFFINITY filtered;

    if (CandidateSet == 0)
        return CandidateSet;

    if (PpmCoreParkingPepOverrideActive)
        park = PpmCoreParkingPepParkMask;
    else
        park = PpmCoreParkingParkMask;

    if (park == 0)
        return CandidateSet;

    filtered = CandidateSet & ~park;
    if (filtered == 0)
        return CandidateSet;

    return filtered;
}
