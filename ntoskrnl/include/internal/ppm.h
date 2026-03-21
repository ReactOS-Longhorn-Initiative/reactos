/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Internal header for the Processor Power Management (PPM)
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

#pragma once

//
// Define this if you want debugging support
//
#define _PPM_DEBUG_                                     0x00

//
// These define the Debug Masks Supported
//
#define PPM_VETO_DEBUG                                  0x01
#define PPM_HETERO_DEBUG                                0x02
#define PPM_CORE_PARK_DEBUG                             0x04
#define PPM_PERF_DEBUG                                  0x06
#define PPM_INIT_SUBSYSTEM_DEBUG                        0x08

//
// Debug/Tracing support
//
#if _PPM_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define PPMTRACE DbgPrintEx
#else
#define PPMTRACE(x, ...)                                 \
    if (x & PpmTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define PPMTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// Processor C state types
//
typedef enum _PPM_PROCESSOR_CSTATE_TYPES
{
    ProcessorC0State,
    ProcessorC1State,
    ProcessorC2State,
    ProcessorC3State
} PPM_PROCESSOR_CSTATE_TYPES;

//
// Idle synchronization state
//
typedef union _PPM_IDLE_SYNCHRONIZATION_STATE
{
    struct
    {
        LONG Value;
        LONG Value2;
    };
    ULONG RefCount:30;
    ULONG Idling:1;
    struct
    {
        ULONG Active:1;
        ULONG CriticalIdleOverride:1;
        ULONG ResidentOverride:1;
        ULONG CompleteIdleStatePending:1;
    };
    ULONG Reserved:29;
} PPM_IDLE_SYNCHRONIZATION_STATE, *PPPM_IDLE_SYNCHRONIZATION_STATE;

//
// Idle bucket time type
//
typedef enum _PPM_IDLE_BUCKET_TIME_TYPE
{
    PpmIdleBucketTimeInQpc,
    PpmIdleBucketTimeIn100ns,
    PpmIdleBucketTimeMaximum
} PPM_IDLE_BUCKET_TIME_TYPE;

//
// Performance state selection
//
typedef struct _PERFINFO_PPM_STATE_SELECTION
{
    ULONG SelectedState;
    ULONG VetoedStates;
    ULONG VetoReason[1];
} PERFINFO_PPM_STATE_SELECTION, *PPERFINFO_PPM_STATE_SELECTION;

//
// Concurrency accounting
//
typedef struct _PPM_CONCURRENCY_ACCOUNTING
{
    ULONG Lock;
    ULONG Processors;
    ULONG ActiveProcessors;
    ULONGLONG LastUpdateTime;
    ULONGLONG TotalTime;
    ULONGLONG AccumulatedTime[1];
} PPM_CONCURRENCY_ACCOUNTING, *PPPM_CONCURRENCY_ACCOUNTING;

//
// Fixed Function Hardware (FFH) throttle state info
//
typedef struct _PPM_FFH_THROTTLE_STATE_INFO
{
    BOOLEAN EnableLogging;
    ULONG MismatchCount;
    UCHAR Initialized;
    ULONGLONG LastValue;
    LARGE_INTEGER LastLogTickCount;
} PPM_FFH_THROTTLE_STATE_INFO, *PPPM_FFH_THROTTLE_STATE_INFO;

//
// Selection statistics
//
typedef struct _PPM_SELECTION_STATISTICS
{
    ULONGLONG PlatformOnlyCount;
    ULONGLONG PreVetoCount;
    ULONGLONG VetoCount;
    ULONGLONG IdleDurationCount;
    ULONGLONG LatencyCount;
    ULONGLONG InterruptibleCount;
    ULONGLONG DeviceDependencyCount;
    ULONGLONG ProcessorDependencyCount;
    ULONGLONG WrongProcessorCount;
    ULONGLONG LegacyOverrideCount;
    ULONGLONG CstateCheckCount;
    ULONGLONG NoCStateCount;
    ULONGLONG SelectedCount;
} PPM_SELECTION_STATISTICS, *PPPM_SELECTION_STATISTICS;

//
// Veto accounting
//
typedef struct _PPM_VETO_ACCOUNTING
{
    volatile LONG VetoPresent;
    LIST_ENTRY VetoListHead;
} PPM_VETO_ACCOUNTING, *PPPM_VETO_ACCOUNTING;

//
// Power processor idle state
//
typedef struct _PPM_IDLE_STATE
{
    KAFFINITY_EX DomainMembers;
    ULONG Latency;
    ULONG BreakEvenDuration;
    ULONG Power;
    ULONG StateFlags;
    PPM_VETO_ACCOUNTING VetoAccounting;
    UCHAR StateType;
    BOOLEAN InterruptsEnabled;
    UCHAR Interruptible;
    BOOLEAN ContextRetained;
    BOOLEAN CacheCoherent;
    BOOLEAN WakesSpuriously;
    BOOLEAN PlatformOnly;
    BOOLEAN NoCState;
} PPM_IDLE_STATE, *PPPM_IDLE_STATE;

//
// PPM Driver Interface Version exchanged via ProcessorStateHandler
//
#define PPM_DRIVER_INTERFACE_VERSION    43

//
// PPM Driver Dispatch Table
// (returned by NtPowerInformation(ProcessorStateHandler, ...) to processor drivers)
//
// The layout matches Windows exactly to allow binary-compatible processor drivers.
// Total size is 0x60 (96) bytes on 32-bit x86.
//
typedef struct _PPM_DRIVER_DISPATCH_TABLE
{
    //
    // Interface version. Processor drivers reject non-PPM_DRIVER_INTERFACE_VERSION.
    //
    ULONG InterfaceVersion;

    //
    // Register ACPI/PEP-provided performance (P-state) table for this processor.
    // Input: PROCESSOR_PERF_STATES*
    //
    NTSTATUS (NTAPI *RegisterPerfStates)(_In_ PVOID PerfStates);

    //
    // Notify the kernel about a performance-state update.
    // Input: PROCESSOR_PERF_STATES_UPDATE*
    //
    VOID    (NTAPI *UpdatePerfStates)(_In_ PVOID PerfStatesUpdate);

    //
    // Register performance cap (thermal/platform limit) for this processor.
    // Input: PROCESSOR_CAP*
    //
    NTSTATUS (NTAPI *RegisterPerfCap)(_In_ PVOID ProcessorCap);

    //
    // Register SPM (Shared PM) settings for this processor.
    //
    NTSTATUS (NTAPI *RegisterSpmSettings)(_In_ PVOID SpmSettings);

    //
    // Register ACPI/PEP idle states (C-states) for this processor.
    // Input: PROCESSOR_IDLE_STATES_EX*
    //
    NTSTATUS (NTAPI *RegisterIdleStates)(_In_ PVOID IdleStates);

    //
    // Register idle domains (C-state coordination) for this processor.
    // Input: PROCESSOR_IDLE_DOMAINS*
    //
    NTSTATUS (NTAPI *RegisterIdleDomains)(_In_ PVOID IdleDomains);

    //
    // Register platform (package/ACPI S-state aware) idle states.
    // Input: PLATFORM_IDLE_STATES*
    //
    NTSTATUS (NTAPI *RegisterPlatformStates)(_In_ PVOID PlatformIdleStates);

    //
    // Register coordinated (multi-processor) idle states.
    // Input: COORDINATED_IDLE_STATES*
    //
    NTSTATUS (NTAPI *RegisterCoordinatedStates)(_In_ PVOID CoordinatedIdleStates);

    //
    // Register pre-registered veto list.
    // Input: PREREGISTERED_VETO_LIST*
    //
    NTSTATUS (NTAPI *RegisterVetoList)(_In_ PVOID VetoList);

    //
    // Remove all veto biases accumulated for this processor.
    //
    NTSTATUS (NTAPI *RemoveVetoBias)(VOID);

    //
    // Update a processor idle veto entry.
    // Input: PROCESSOR_IDLE_VETO*
    //
    NTSTATUS (NTAPI *UpdateProcessorIdleVeto)(_In_ PVOID ProcessorIdleVeto);

    //
    // Update a platform idle veto entry.
    // Input: PLATFORM_IDLE_VETO*
    //
    NTSTATUS (NTAPI *UpdatePlatformIdleVeto)(_In_ PVOID PlatformIdleVeto);

    //
    // Hyper-V performance state registration.
    //
    NTSTATUS (NTAPI *RegisterPerfStatesHv)(_In_ PVOID PerfStatesHv);

    //
    // Hyper-V performance cap registration.
    //
    NTSTATUS (NTAPI *RegisterPerfCapHv)(_In_ PVOID PerfCapHv);

    //
    // Hyper-V idle state registration.
    //
    NTSTATUS (NTAPI *RegisterIdleStatesHv)(_In_ PVOID IdleStatesHv);

    //
    // Hyper-V performance state counters registration.
    //
    NTSTATUS (NTAPI *RegisterPerfStatesCountersHv)(_In_ PVOID PerfStatesCountersHv);

    //
    // Bind this processor to a PEP-managed processor handle.
    // Input: PEP handle (PVOID)
    //
    NTSTATUS (NTAPI *SetProcessorPep)(_In_ PVOID PepHandle);

    //
    // PEP park preference notification.
    //
    NTSTATUS (NTAPI *ParkPreferenceNotification)(_In_ PVOID PepHandle,
                                                  _In_ PVOID Notification);

    //
    // PEP park mask notification.
    //
    NTSTATUS (NTAPI *ParkMaskNotification)(_In_ PVOID PepHandle,
                                            _In_ PVOID Notification);

    //
    // PEP idle select notification.
    //
    NTSTATUS (NTAPI *IdleSelectNotification)(_In_ PVOID PepHandle,
                                              _In_ PVOID Notification);

    //
    // PEP platform state query notification.
    //
    NTSTATUS (NTAPI *QueryPlatformStateNotification)(_In_ PVOID PepHandle,
                                                      _In_ PVOID Notification,
                                                      _In_ BOOLEAN Update);

    //
    // PEP coordinated dependency query notification.
    //
    NTSTATUS (NTAPI *QueryCoordinatedDependencyNotification)(
        _In_ PVOID PepHandle,
        _In_ PVOID Notification);

    //
    // Register energy estimation callbacks.
    //
    VOID    (NTAPI *RegisterEnergyEstimation)(_In_ PVOID ComputeEnergy,
                                               _In_ PVOID SnapCounters);

} PPM_DRIVER_DISPATCH_TABLE, *PPPM_DRIVER_DISPATCH_TABLE;

C_ASSERT(sizeof(PPM_DRIVER_DISPATCH_TABLE) == 0x60);

//
// Global kernel-side PPM driver dispatch table (exported via ProcessorStateHandler).
// Initialised once during PpmInitialize; read-only afterwards.
//
extern PPM_DRIVER_DISPATCH_TABLE PpmKernelDispatchTable;

//
// Period of the per-processor performance-monitoring DPC timer: 20 ms.
// Expressed as a negative (relative) 100-ns interval for KeSetTimerEx.
//
#define PPM_PERF_DPC_PERIOD     200000LL    /* 20 ms in 100-ns units    */
#define PPM_PERF_DPC_PERIOD_MS  20          /* Same period in ms        */

/* -------------------------------------------------------------------------
 * Kernel-side views of structures passed by processor drivers via the
 * PPM_DRIVER_DISPATCH_TABLE callbacks.  These layouts must match the
 * corresponding structures in the processor driver headers exactly.
 * -------------------------------------------------------------------------*/

//
// Forward-declare opaque constraint / prepare types so that the idle-state
// function pointer signatures can reference them without a full definition.
// A processor driver that needs the fields can include its own definition.
//
typedef struct _PPM_IDLE_PREPARE_INFO  PPM_IDLE_PREPARE_INFO,  *PPPM_IDLE_PREPARE_INFO;
typedef struct _PPM_IDLE_CONSTRAINTS   PPM_IDLE_CONSTRAINTS,   *PPPM_IDLE_CONSTRAINTS;

//
// PPM_IDLE_STATES_EX
//
// Kernel-side view of the PROCESSOR_IDLE_STATES_EX structure registered by
// a processor driver via RegisterIdleStates().  The pointer is stored in
// Prcb->PowerState.IdleState; PpmIdle() casts it to this type.
//
typedef struct _PPM_IDLE_STATES_EX
{
    ULONG               Version;            /* Must be 1                        */
    PROCESSOR_NUMBER    Processor;          /* Logical processor identification */
    PVOID               Context;            /* Driver context for callbacks     */
    BOOLEAN             EstimateIdleDuration;
    BOOLEAN             Update;
    UCHAR               InterfaceVersion;

    /* Driver-provided C-state callbacks */
    VOID    (FASTCALL *IdlePrepare)       (_In_  PPPM_IDLE_PREPARE_INFO Info);
    VOID    (FASTCALL *IdleCancel)        (_In_  PVOID Context,
                                           _In_  ULONG StateIndex);
    ULONG   (FASTCALL *IdlePreselect)     (_In_  PVOID Context,
                                           _In_  PPPM_IDLE_CONSTRAINTS Constraints);
    ULONG   (FASTCALL *IdleTest)          (_In_  PVOID Context,
                                           _In_  ULONG StateIndex,
                                           _In_  ULONG Duration);
    ULONG   (FASTCALL *IdleAvailabilityCheck)
                                          (_In_  PVOID Context,
                                           _In_  ULONG StateIndex);
    NTSTATUS(FASTCALL *IdlePreExecute)    (_In_  PVOID Context,
                                           _In_  ULONG StateIndex,
                                           _In_  ULONG ProcessorIndex,
                                           _In_  ULONG Flags,
                                           _Out_ PULONG Hint);
    NTSTATUS(FASTCALL *IdleExecute)       (_In_  PVOID Context,
                                           _In_  ULONG StateIndex,
                                           _In_  ULONG ProcessorIndex,
                                           _In_  ULONG Flags,
                                           _Out_ PULONG Hint);
    VOID    (FASTCALL *IdleComplete)      (_In_  PVOID Context,
                                           _In_  ULONG StateIndex,
                                           _In_  ULONG ProcessorIndex,
                                           _In_  ULONG Flags,
                                           _In_  PULONG Hint);
    BOOLEAN (FASTCALL *IdleIsHalted)      (_In_  PVOID Context);
    BOOLEAN (FASTCALL *IdleInitiateWake)  (_In_  PVOID Context);

    ULONG               MaximumDependencies;
    ULONG               ProcessorIdleCount;
    /* Variable-length State[] follows — see PPM_PROCESSOR_IDLE_STATE_EX */
} PPM_IDLE_STATES_EX, *PPPM_IDLE_STATES_EX;

//
// One C-state entry in the State[] tail of PPM_IDLE_STATES_EX /
// PROCESSOR_IDLE_STATES_EX.  Layout must match the processor drivers'
// PROCESSOR_IDLE_STATE_EX (e.g. amdppm.h).
//
typedef struct _PPM_PROCESSOR_IDLE_STATE_EX
{
    ULONG           FlagsAsUlong;
    ULONG           Latency;             /* µs (ACPI latency guidance)       */
    ULONG           BreakEvenDuration;   /* µs — min idle worth entering     */
    ULONG           Power;               /* mW (informational)               */
    UNICODE_STRING  Name;
} PPM_PROCESSOR_IDLE_STATE_EX, *PPPM_PROCESSOR_IDLE_STATE_EX;

//
// PPM_PERF_STATES_EX
//
// Kernel-side view of the PROCESSOR_PERF_STATES structure registered by
// a processor driver via RegisterPerfStates().  Only the fields used by
// the kernel policy engine are listed; remaining fields are opaque.
//
// NOTE: The pointer is stored in Prcb->PowerState.IdleHandlers (a PVOID)
// as a temporary measure until a dedicated PRCB field is available.
//
typedef struct _PPM_PERF_STATES_EX
{
    ULONG   Version;            /* Must be 1                                    */
    USHORT  Type;               /* PPM_PERF_STATE_TYPE_*                        */
    BOOLEAN HardPlatformCap;    /* TRUE = BIOS performance cap active           */
    BOOLEAN AffinitizeControl;
    BOOLEAN EfficientThrottle;
    ULONG   ProcessorCount;
    ULONG   NominalFrequency;   /* Nominal MHz (100 % P-state)                  */
    ULONG   MaxPerfPercent;     /* Platform-allowed max performance (0-100)     */
    ULONG   MinPerfPercent;     /* Minimum performance level (0-100)            */
    ULONG   MinThrottlePercent; /* Minimum throttle level (0-100)               */
    /* ... remaining fields opaque ... */
} PPM_PERF_STATES_EX, *PPPM_PERF_STATES_EX;

//
// PPM_PERF_CAP
//
// Kernel-side view of the PROCESSOR_CAP structure registered by a processor
// driver via RegisterPerfCap().  Limits the highest P-state the policy engine
// may select.
//
typedef struct _PPM_PERF_CAP
{
    ULONG            Version;          /* Must be 1                             */
    PROCESSOR_NUMBER ProcessorNumber;
    ULONG            PlatformCap;      /* Max platform-allowed perf (0-100 %)   */
    ULONG            ThermalCap;       /* Max thermally-allowed perf (0-100 %)  */
    ULONG            LimitReasons;     /* Bitmask of active limit reasons       */
} PPM_PERF_CAP, *PPPM_PERF_CAP;

/* -------------------------------------------------------------------------
 * Global PPM policy parameters
 *
 * These variables are updated by the power setting workers (posett.c) when
 * the system power policy changes (AC/DC switch, user scheme change, etc.).
 * The PPM engine (policy.c, eng.c) reads them on every DPC period.
 * Defaults reflect Windows "Balanced" profile behaviour.
 * -------------------------------------------------------------------------*/

//
// Maximum and minimum processor throttle percentages.
// Updated by GUID_PROCESSOR_THROTTLE_MAXIMUM / _MINIMUM workers.
//
extern volatile UCHAR PpmPolicyMaxThrottle;
extern volatile UCHAR PpmPolicyMinThrottle;

//
// Busy-percentage thresholds that trigger a P-state increase or decrease.
// Updated by GUID_PROCESSOR_PERF_INCREASE_THRESHOLD / _DECREASE_THRESHOLD.
// Default: promote at 60 %, demote at 40 %.
//
extern volatile UCHAR PpmPolicyPerfIncreaseThreshold;
extern volatile UCHAR PpmPolicyPerfDecreaseThreshold;

//
// Number of consecutive DPC periods the CPU must remain above/below the
// threshold before the P-state is actually changed (hysteresis).
// Updated by GUID_PROCESSOR_PERF_INCREASE_TIME / _DECREASE_TIME.
// Default: promote after 1 period (fast), demote after 5 periods (slow).
//
extern volatile ULONG PpmPolicyPerfIncreaseTime;
extern volatile ULONG PpmPolicyPerfDecreaseTime;

//
// C-state idle thresholds.
// Updated by GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD / _PROMOTE_THRESHOLD.
// Default: demote at 10 % idle, promote at 20 % idle.
//
extern volatile UCHAR PpmPolicyIdleDemoteThreshold;
extern volatile UCHAR PpmPolicyIdlePromoteThreshold;

//
// Core parking (park.c) — logical CPUs masked out of idle-aware scheduling.
//
extern volatile KAFFINITY PpmCoreParkingParkMask;
extern volatile ULONG PpmCoreParkingMinCores;
extern volatile ULONG PpmCoreParkingMaxCores;
extern volatile UCHAR PpmCoreParkingBusyIncreaseThreshold;
extern volatile UCHAR PpmCoreParkingBusyDecreaseThreshold;
extern volatile ULONG PpmCoreParkingIncreaseTime;
extern volatile ULONG PpmCoreParkingDecreaseTime;
extern volatile BOOLEAN PpmCoreParkingPepOverrideActive;
extern volatile KAFFINITY PpmCoreParkingPepParkMask;
extern volatile ULONG PpmCoreParkingPepPreferencePercent;

/* -------------------------------------------------------------------------
 * Function prototypes
 * -------------------------------------------------------------------------*/

//
// Initialization routines
//
CODE_SEG("INIT")
VOID
NTAPI
PpmInitDispatchTable(VOID);

CODE_SEG("INIT")
NTSTATUS
NTAPI
PpmInitialize(
    _In_ BOOLEAN EarlyPhase);

//
// Processor idle functions
//
VOID
FASTCALL
PpmIdle(
    _In_ PPROCESSOR_POWER_STATE PowerState);

//
// Processor performance functions
//
VOID
NTAPI
PpmPerfIdleDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2);

//
// CPU statistics (cpustat.c)
//
VOID
NTAPI
PpmUpdateIdleAccounting(
    _In_ PPROCESSOR_POWER_STATE PowerState,
    _In_ ULONG                  StateIndex,
    _In_ ULONGLONG              Duration);

UCHAR
NTAPI
PpmComputeBusyPercentage(
    _In_ PKPRCB Prcb);

VOID
NTAPI
PpmResetIdleAccounting(
    _In_ PPROCESSOR_POWER_STATE PowerState);

VOID
NTAPI
PpmResetIdleAccountingAllProcessors(VOID);

//
// Core engine (eng.c)
//
ULONG
NTAPI
PpmSelectIdleState(
    _In_ PPROCESSOR_POWER_STATE PowerState,
    _In_ ULONGLONG              IdleDuration);

UCHAR
NTAPI
PpmSelectPerfState(
    _In_ PKPRCB Prcb,
    _In_ UCHAR  BusyPercentage);

VOID
NTAPI
PpmApplyThrottle(
    _In_ PPROCESSOR_POWER_STATE PowerState,
    _In_ UCHAR                  ThrottlePercent);

//
// Processor performance policy (policy.c)
//
VOID
NTAPI
PpmEvaluatePerfPolicy(
    _In_ PKPRCB Prcb);

VOID
NTAPI
PpmCoreParkingInitialize(VOID);

VOID
NTAPI
PpmCoreParkingSetMinMaxCores(
    _In_ ULONG MinCores,
    _In_ ULONG MaxCores);

VOID
NTAPI
PpmCoreParkingSetPolicyDword(
    _In_ LPCGUID SettingGuid,
    _In_ ULONG Value);

VOID
NTAPI
PpmCoreParkingRefreshMask(VOID);

VOID
NTAPI
PpmCoreParkingPeriodicRebalance(VOID);

KAFFINITY
NTAPI
PpmCoreParkingApplySchedulerMask(
    _In_ KAFFINITY CandidateSet);

/* EOF */
