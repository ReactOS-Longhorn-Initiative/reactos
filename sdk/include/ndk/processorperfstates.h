/*++
 *
 * NDK: PROCESSOR_PERF_STATES layout (Windows 10–compatible).
 *
 * Matches the public order from the Windows 10 processor driver reference
 * (e.g. processr.sys / amdppm.sys headers). Kernel PPM and processor drivers
 * must agree on this layout.
 *
 * Copyright (c) ReactOS Contributors
 * SPDX-License-Identifier: MIT
 *
--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NTOS_MODE_USER

#include <ketypes.h>

struct _PROCESSOR_FEEDBACK_COUNTER;
typedef struct _PROCESSOR_FEEDBACK_COUNTER PROCESSOR_FEEDBACK_COUNTER, *PPROCESSOR_FEEDBACK_COUNTER;

typedef ULONG (FASTCALL *PPM_PERF_SELECTION_HANDLER)(
    _In_ ULONG Context,
    _In_ ULONG TargetPercent,
    _In_ ULONG MinPercent,
    _In_ ULONG MaxPercent,
    _In_ ULONG Flags,
    _Out_ PULONG Frequency,
    _Out_ PULONGLONG Selection);

typedef VOID (FASTCALL *PPM_PERF_CONTROL_HANDLER)(
    _In_ ULONG Context,
    _In_ ULONGLONG SelectedState,
    _In_ ULONG MinPercent,
    _In_ ULONG MaxPercent,
    _In_ ULONG TolerancePercent,
    _In_ UCHAR Autonomous,
    _In_ UCHAR Initiate,
    _In_ UCHAR Force);

#define PROCESSOR_PERF_STATES_VERSION           1

#define PPM_PERF_STATE_TYPE_ACPI_IO             0
#define PPM_PERF_STATE_TYPE_ACPI_MSR            1
#define PPM_PERF_STATE_TYPE_ACPI_FFH            2

#if defined(_MSC_VER)
#define _PPM_PROCESSOR_PERF_STATES_ALIGN __declspec(align(8))
#elif defined(__GNUC__)
#define _PPM_PROCESSOR_PERF_STATES_ALIGN __attribute__((aligned(8)))
#else
#define _PPM_PROCESSOR_PERF_STATES_ALIGN
#endif

typedef struct _PROCESSOR_PERF_INFO
{
    PROCESSOR_NUMBER Number;
    ULONG PerfContext;
    ULONG PlatformCap;
    ULONG ThermalCap;
    ULONG LimitReasons;
} PROCESSOR_PERF_INFO, *PPROCESSOR_PERF_INFO;

typedef _PPM_PROCESSOR_PERF_STATES_ALIGN struct _PROCESSOR_PERF_STATES
{
    ULONG Version;
    USHORT Type;
    BOOLEAN HardPlatformCap;
    BOOLEAN AffinitizeControl;
    BOOLEAN EfficientThrottle;
    ULONG ProcessorCount;
    ULONG NominalFrequency;
    ULONG MaxPerfPercent;
    ULONG MinPerfPercent;
    ULONG MinThrottlePercent;
    ULONG FeedbackCounterCount;
    ULONG MinimumPerfCheckPeriod;
    UCHAR AutonomousMode;
    ULONGLONG MinimumRelativePerformance;
    ULONGLONG NominalRelativePerformance;
    ULONG GlobalContext;
    KAFFINITY_EX TargetProcessors;
    VOID (FASTCALL *GetFFHThrottleState)(_Out_ PULONGLONG State);
    VOID (FASTCALL *TimeWindowHandler)(_In_ ULONG A, _In_ ULONG B);
    VOID (FASTCALL *BoostPolicyHandler)(_In_ ULONG A, _In_ ULONG B);
    VOID (FASTCALL *BoostModeHandler)(_In_ ULONG A, _In_ ULONG B);
    VOID (FASTCALL *EnergyPerfPreferenceHandler)(_In_ ULONG A, _In_ ULONG B);
    VOID (FASTCALL *AutonomousActivityWindowHandler)(_In_ ULONG A, _In_ ULONG B);
    VOID (FASTCALL *AutonomousModeHandler)(_In_ ULONG A, _In_ ULONG B);
    LONG (FASTCALL *StartPolicyUpdate)(VOID);
    LONG (FASTCALL *CompletePolicyUpdate)(VOID);
    VOID (FASTCALL *ReinitializeHandler)(_In_ ULONG A);
    PPM_PERF_SELECTION_HANDLER PerfSelectionHandler;
    PPM_PERF_CONTROL_HANDLER PerfControlHandler;
    VOID (FASTCALL *PerfControlReadFeedback)(_In_ VOID (FASTCALL *Callback)(VOID));
    VOID (FASTCALL *PerfControlAcquirePerformance)(_In_ VOID (FASTCALL *Callback)(VOID));
    VOID (FASTCALL *PerfControlCommitPerformance)(_In_ VOID (FASTCALL *Callback)(VOID));
    VOID (NTAPI *ParkPreference)(
        _In_ UCHAR A,
        _In_ ULONGLONG B,
        _In_ ULONG C,
        _Inout_ PKAFFINITY_EX D,
        _Inout_ PKAFFINITY_EX E,
        _Inout_ PKAFFINITY_EX F,
        _Inout_ PKAFFINITY_EX G);
    VOID (NTAPI *ParkMask)(_In_ ULONGLONG A, _Inout_ PKAFFINITY_EX B);
    VOID (NTAPI *PerfCheckComplete)(_In_ ULONG A, _In_ ULONGLONG B);
    PPROCESSOR_FEEDBACK_COUNTER FeedbackCounters;
    PPROCESSOR_PERF_INFO Processors;
    PULONG CounterContexts;
} PROCESSOR_PERF_STATES, *PPROCESSOR_PERF_STATES;

#endif /* !NTOS_MODE_USER */

#ifdef __cplusplus
}
#endif
