/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * FILE:        ntoskrnl/po/ppm/dispatch.c
 * PURPOSE:     Kernel-side PPM driver dispatch table
 *
 *              This table is exported to user-space processor drivers (intelppm,
 *              amdppm, processr) via NtPowerInformation(ProcessorStateHandler).
 *              Each entry provides a kernel callback that processor drivers can
 *              invoke to register their P-state / C-state / performance-cap
 *              information with the OS power manager.
 *
 * COPYRIGHT:   Copyright 2024 ReactOS Contributors
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include "internal/ppm.h"

/* GLOBALS *******************************************************************/

//
// The global kernel-side dispatch table returned to processor drivers that call
// ZwPowerInformation(ProcessorStateHandler, NULL, 0, &Table, sizeof(Table)).
//
// The table is populated in PpmInitDispatchTable() which is called from
// PpmInitialize() during the early-phase kernel power-manager initialisation.
//
PPM_DRIVER_DISPATCH_TABLE PpmKernelDispatchTable;

//
// Global PPM policy parameters.
//
// These are the writable policy knobs that the power setting workers
// (posett.c) update when the system power policy changes.  The PPM engine
// (eng.c, policy.c) reads them on every DPC period.  They are declared as
// volatile so that compiler re-ordering does not obscure an update that
// arrives from a different thread.
//
// Default values match Windows "Balanced" profile behaviour.
//
volatile UCHAR PpmPolicyMaxThrottle           = 100; /* GUID_PROCESSOR_THROTTLE_MAXIMUM        */
volatile UCHAR PpmPolicyMinThrottle           = 0;   /* GUID_PROCESSOR_THROTTLE_MINIMUM        */
volatile UCHAR PpmPolicyPerfIncreaseThreshold = 60;  /* GUID_PROCESSOR_PERF_INCREASE_THRESHOLD */
volatile UCHAR PpmPolicyPerfDecreaseThreshold = 40;  /* GUID_PROCESSOR_PERF_DECREASE_THRESHOLD */
volatile ULONG PpmPolicyPerfIncreaseTime      = 1;   /* GUID_PROCESSOR_PERF_INCREASE_TIME      */
volatile ULONG PpmPolicyPerfDecreaseTime      = 5;   /* GUID_PROCESSOR_PERF_DECREASE_TIME      */
volatile UCHAR PpmPolicyIdleDemoteThreshold   = 10;  /* GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD   */
volatile UCHAR PpmPolicyIdlePromoteThreshold  = 20;  /* GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD  */

/* PROTOTYPES ****************************************************************/

/*
 * Forward declarations for the dispatch-table entries defined below.
 * All functions run at PASSIVE_LEVEL (called from PASSIVE context in
 * EvtDevicePrepareHardware / EvtDeviceD0Entry of processor drivers).
 */

static NTSTATUS NTAPI PpmDispRegisterPerfStates(_In_ PVOID PerfStates);
static VOID     NTAPI PpmDispUpdatePerfStates(_In_ PVOID PerfStatesUpdate);
static NTSTATUS NTAPI PpmDispRegisterPerfCap(_In_ PVOID ProcessorCap);
static NTSTATUS NTAPI PpmDispRegisterSpmSettings(_In_ PVOID SpmSettings);
static NTSTATUS NTAPI PpmDispRegisterIdleStates(_In_ PVOID IdleStates);
static NTSTATUS NTAPI PpmDispRegisterIdleDomains(_In_ PVOID IdleDomains);
static NTSTATUS NTAPI PpmDispRegisterPlatformStates(_In_ PVOID PlatformIdleStates);
static NTSTATUS NTAPI PpmDispRegisterCoordinatedStates(_In_ PVOID CoordinatedIdleStates);
static NTSTATUS NTAPI PpmDispRegisterVetoList(_In_ PVOID VetoList);
static NTSTATUS NTAPI PpmDispRemoveVetoBias(VOID);
static NTSTATUS NTAPI PpmDispUpdateProcessorIdleVeto(_In_ PVOID ProcessorIdleVeto);
static NTSTATUS NTAPI PpmDispUpdatePlatformIdleVeto(_In_ PVOID PlatformIdleVeto);
static NTSTATUS NTAPI PpmDispRegisterPerfStatesHv(_In_ PVOID PerfStatesHv);
static NTSTATUS NTAPI PpmDispRegisterPerfCapHv(_In_ PVOID PerfCapHv);
static NTSTATUS NTAPI PpmDispRegisterIdleStatesHv(_In_ PVOID IdleStatesHv);
static NTSTATUS NTAPI PpmDispRegisterPerfStatesCountersHv(_In_ PVOID PerfStatesCountersHv);
static NTSTATUS NTAPI PpmDispSetProcessorPep(_In_ PVOID PepHandle);
static NTSTATUS NTAPI PpmDispParkPreferenceNotification(_In_ PVOID PepHandle,
                                                         _In_ PVOID Notification);
static NTSTATUS NTAPI PpmDispParkMaskNotification(_In_ PVOID PepHandle,
                                                   _In_ PVOID Notification);
static NTSTATUS NTAPI PpmDispIdleSelectNotification(_In_ PVOID PepHandle,
                                                     _In_ PVOID Notification);
static NTSTATUS NTAPI PpmDispQueryPlatformStateNotification(_In_ PVOID PepHandle,
                                                             _In_ PVOID Notification,
                                                             _In_ BOOLEAN Update);
static NTSTATUS NTAPI PpmDispQueryCoordinatedDependencyNotification(
    _In_ PVOID PepHandle,
    _In_ PVOID Notification);
static VOID     NTAPI PpmDispRegisterEnergyEstimation(_In_ PVOID ComputeEnergy,
                                                       _In_ PVOID SnapCounters);

/*
 * PEP notification layouts (subset of Windows PPM PEP contracts).  Drivers
 * must set Version to 1 for the fields below to be interpreted.
 */
typedef struct _PPM_PEP_PARK_MASK_NOTIFICATION
{
    ULONG Version;
    KAFFINITY ParkMask;
} PPM_PEP_PARK_MASK_NOTIFICATION, *PPPM_PEP_PARK_MASK_NOTIFICATION;

typedef struct _PPM_PEP_PARK_PREFERENCE_NOTIFICATION
{
    ULONG Version;
    ULONG PreferencePercent; /* 0–100, higher = keep more cores active */
} PPM_PEP_PARK_PREFERENCE_NOTIFICATION, *PPPM_PEP_PARK_PREFERENCE_NOTIFICATION;

/* DISPATCH TABLE ENTRIES ****************************************************/

/*
 * PpmDispRegisterPerfStates
 *
 * Called by a processor driver to register its ACPI or PEP-supplied
 * performance (P-state) table with the OS power manager.
 *
 * The incoming PerfStates pointer points to a PROCESSOR_PERF_STATES structure
 * (defined in the processor driver's header).  ReactOS stores the pointer in a
 * per-logical-processor slot and enables the performance policy engine.
 *
 * Note: Full P-state transition support (writing MSRs / IO ports to change
 * frequency) is deferred to the processor driver; the kernel only tracks the
 * available states and selects among them via the policy engine.
 */
static
NTSTATUS
NTAPI
PpmDispRegisterPerfStates(
    _In_ PVOID PerfStates)
{
    PKPRCB           Prcb;
    PPPM_PERF_STATES_EX PerfStatesEx;
    PPROCESSOR_POWER_STATE PowerState;

    /*
     * NULL clears the PROCESSOR_PERF_STATES pointer for the *current*
     * processor (used when unloading amdppm on that CPU via KeIpiGenericCall).
     * This path must not use PAGED_CODE — the broadcast runs at high IRQL.
     */
    if (!PerfStates)
    {
        Prcb = KeGetCurrentPrcb();
        PowerState = &Prcb->PowerState;
        PowerState->PerfStates = NULL;
        PowerState->ProcessorMaxThrottle = 100;
        PowerState->ProcessorMinThrottle = 0;
        if (PowerState->CurrentThrottle > 100)
            PowerState->CurrentThrottle = 100;
        return STATUS_SUCCESS;
    }

    PAGED_CODE();

    PerfStatesEx = (PPPM_PERF_STATES_EX)PerfStates;

    //
    // Validate the version field before touching anything else.
    //
    if (PerfStatesEx->Version != 1)
    {
        DPRINT1("PpmDispRegisterPerfStates: unsupported version %lu\n",
                PerfStatesEx->Version);
        return STATUS_REVISION_MISMATCH;
    }

    Prcb      = KeGetCurrentPrcb();
    PowerState = &Prcb->PowerState;

    /* Store the driver-provided PROCESSOR_PERF_STATES pointer (see potypes.h). */
    PowerState->PerfStates = PerfStates;

    //
    // Extract the key policy limits and store them in the PRCB fields the
    // policy engine already reads (PpmSelectPerfState, PpmEvaluatePerfPolicy).
    //
    if (PerfStatesEx->MaxPerfPercent <= 100)
        PowerState->ProcessorMaxThrottle = (UCHAR)PerfStatesEx->MaxPerfPercent;
    else
        PowerState->ProcessorMaxThrottle = 100;

    if (PerfStatesEx->MinPerfPercent <= 100)
        PowerState->ProcessorMinThrottle = (UCHAR)PerfStatesEx->MinPerfPercent;
    else
        PowerState->ProcessorMinThrottle = 0;

    //
    // Initialise CurrentThrottle to the platform maximum if this is the
    // first registration (or if a re-registration brings a higher ceiling).
    //
    if (PowerState->CurrentThrottle < PowerState->ProcessorMinThrottle)
        PowerState->CurrentThrottle = PowerState->ProcessorMinThrottle;
    if (PowerState->CurrentThrottle > PowerState->ProcessorMaxThrottle)
        PowerState->CurrentThrottle = PowerState->ProcessorMaxThrottle;

    DPRINT("PpmDispRegisterPerfStates: CPU %u P-states @ %p "
           "(min=%u%%, max=%u%%)\n",
           Prcb->Number, PerfStates,
           PowerState->ProcessorMinThrottle,
           PowerState->ProcessorMaxThrottle);

    return STATUS_SUCCESS;
}

/*
 * PpmDispUpdatePerfStates
 *
 * Notifies the kernel that performance-state parameters (such as TPC or PPC
 * caps read from ACPI notifications 0x80/0x81) have changed.
 */
static
VOID
NTAPI
PpmDispUpdatePerfStates(
    _In_ PVOID PerfStatesUpdate)
{
    PAGED_CODE();

    if (!PerfStatesUpdate)
        return;

    DPRINT("PpmDispUpdatePerfStates: update @ %p on CPU %u\n",
           PerfStatesUpdate, KeGetCurrentProcessorNumber());

    /*
     * An update to the P-state configuration (e.g. from a PPC/TPC ACPI
     * notification) is treated as a full re-registration: route through
     * PpmDispRegisterPerfStates so the throttle limits are refreshed.
     */
    PpmDispRegisterPerfStates(PerfStatesUpdate);
}

/*
 * PpmDispRegisterPerfCap
 *
 * Registers the current hardware and thermal performance caps for this
 * processor.  Caps limit the highest P-state the policy engine may select.
 *
 * Input: pointer to a PROCESSOR_CAP structure:
 *   { Version, ProcessorNumber, PlatformCap, ThermalCap, LimitReasons }
 */
static
NTSTATUS
NTAPI
PpmDispRegisterPerfCap(
    _In_ PVOID ProcessorCap)
{
    PKPRCB          Prcb;
    PPPM_PERF_CAP    Cap;
    PPROCESSOR_POWER_STATE PowerState;
    UCHAR            NewMax;

    PAGED_CODE();

    if (!ProcessorCap)
        return STATUS_INVALID_PARAMETER;

    Cap = (PPPM_PERF_CAP)ProcessorCap;

    if (Cap->Version != 1)
    {
        DPRINT1("PpmDispRegisterPerfCap: unsupported version %lu\n",
                Cap->Version);
        return STATUS_REVISION_MISMATCH;
    }

    Prcb       = KeGetCurrentPrcb();
    PowerState = &Prcb->PowerState;

    //
    // The effective ceiling is the minimum of the platform cap and the
    // thermal cap; a lower value means more throttling is required.
    //
    NewMax = 100;
    if (Cap->PlatformCap <= 100 && (UCHAR)Cap->PlatformCap < NewMax)
        NewMax = (UCHAR)Cap->PlatformCap;
    if (Cap->ThermalCap  <= 100 && (UCHAR)Cap->ThermalCap  < NewMax)
        NewMax = (UCHAR)Cap->ThermalCap;

    //
    // Apply thermal ceiling to the dedicated thermal limit field and to
    // the platform maximum throttle.  ProcessorMaxThrottle is the harder
    // ceiling: neither the driver nor the policy engine may exceed it.
    //
    PowerState->ThermalThrottleLimit = NewMax;
    PowerState->ProcessorMaxThrottle = NewMax;

    //
    // If the processor is currently running above the new ceiling, pull
    // it down immediately rather than waiting for the next DPC period.
    //
    if (PowerState->CurrentThrottle > NewMax)
        PpmApplyThrottle(PowerState, NewMax);

    DPRINT("PpmDispRegisterPerfCap: CPU %u platform=%lu%% thermal=%lu%%"
           " → max=%u%%\n",
           Prcb->Number,
           Cap->PlatformCap, Cap->ThermalCap, NewMax);

    return STATUS_SUCCESS;
}

/*
 * PpmDispRegisterSpmSettings
 *
 * Registers Shared PM (SPM) settings for this processor.  SPM is a
 * collaborative power-limit mechanism between the OS and firmware; we
 * stub this out since ReactOS does not yet support SPM.
 */
static
NTSTATUS
NTAPI
PpmDispRegisterSpmSettings(
    _In_ PVOID SpmSettings)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(SpmSettings);

    /* SPM not supported – silently succeed so the driver continues loading. */
    return STATUS_SUCCESS;
}

/*
 * PpmDispRegisterIdleStates
 *
 * Registers ACPI/PEP-supplied idle (C-state) handlers for the current
 * processor.  The OS power-idle path will call the callbacks provided in the
 * PROCESSOR_IDLE_STATES_EX structure to enter deep idle states.
 */
static
NTSTATUS
NTAPI
PpmDispRegisterIdleStates(
    _In_ PVOID IdleStates)
{
    PKPRCB Prcb;

    PAGED_CODE();

    if (!IdleStates)
        return STATUS_INVALID_PARAMETER;

    Prcb = KeGetCurrentPrcb();

    //
    // Store the driver-provided PROCESSOR_IDLE_STATES_EX pointer in
    // Prcb->PowerState.IdleState.  PpmIdle() (idle.c) reads this pointer
    // and casts it to PPPM_IDLE_STATES_EX to invoke the driver callbacks.
    //
    Prcb->PowerState.IdleState = IdleStates;

    DPRINT("PpmDispRegisterIdleStates: CPU %u registered %lu C-state(s) @ %p\n",
           Prcb->Number,
           ((PPPM_IDLE_STATES_EX)IdleStates)->ProcessorIdleCount,
           IdleStates);

    return STATUS_SUCCESS;
}

/*
 * PpmDispRegisterIdleDomains
 *
 * Registers the C-state coordination domain topology for this processor
 * (which other processors must be idle before this one can enter a given
 * C-state).  Not yet acted upon; stored for future use.
 */
static
NTSTATUS
NTAPI
PpmDispRegisterIdleDomains(
    _In_ PVOID IdleDomains)
{
    PKPRCB Prcb;

    PAGED_CODE();

    if (!IdleDomains)
        return STATUS_INVALID_PARAMETER;

    Prcb = KeGetCurrentPrcb();
    Prcb->PowerState.PpmIdleDomains = IdleDomains;

    DPRINT("PpmDispRegisterIdleDomains: CPU %u @ %p\n",
           Prcb->Number, IdleDomains);

    return STATUS_SUCCESS;
}

/*
 * PpmDispRegisterPlatformStates
 *
 * Registers platform-level idle states (package C-states) exposed by the
 * firmware.  ReactOS does not yet implement platform idle; accept and ignore.
 */
static
NTSTATUS
NTAPI
PpmDispRegisterPlatformStates(
    _In_ PVOID PlatformIdleStates)
{
    PKPRCB Prcb;

    PAGED_CODE();

    if (!PlatformIdleStates)
        return STATUS_INVALID_PARAMETER;

    Prcb = KeGetCurrentPrcb();
    Prcb->PowerState.PpmPlatformIdleStates = PlatformIdleStates;

    DPRINT("PpmDispRegisterPlatformStates: CPU %u @ %p\n",
           Prcb->Number, PlatformIdleStates);

    return STATUS_SUCCESS;
}

/*
 * PpmDispRegisterCoordinatedStates
 *
 * Registers coordinated (multi-processor) idle states.  Not yet supported.
 */
static
NTSTATUS
NTAPI
PpmDispRegisterCoordinatedStates(
    _In_ PVOID CoordinatedIdleStates)
{
    PKPRCB Prcb;

    PAGED_CODE();

    if (!CoordinatedIdleStates)
        return STATUS_INVALID_PARAMETER;

    Prcb = KeGetCurrentPrcb();
    Prcb->PowerState.PpmCoordinatedIdleStates = CoordinatedIdleStates;

    DPRINT("PpmDispRegisterCoordinatedStates: CPU %u @ %p\n",
           Prcb->Number, CoordinatedIdleStates);

    return STATUS_SUCCESS;
}

/*
 * PpmDispRegisterVetoList
 *
 * Registers the pre-registered veto list used to prevent specific idle state
 * transitions under certain conditions (e.g. debug-halt active).
 */
static
NTSTATUS
NTAPI
PpmDispRegisterVetoList(
    _In_ PVOID VetoList)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(VetoList);

    /* FIXME: Wire up veto list to the idle selection engine. */
    return STATUS_SUCCESS;
}

/*
 * PpmDispRemoveVetoBias
 *
 * Removes all accumulated veto biases for the current processor.
 */
static
NTSTATUS
NTAPI
PpmDispRemoveVetoBias(VOID)
{
    PAGED_CODE();

    /* FIXME: Reset veto counters in PPM_VETO_ACCOUNTING for this CPU. */
    return STATUS_SUCCESS;
}

/*
 * PpmDispUpdateProcessorIdleVeto
 *
 * Increments or decrements a specific idle-state veto counter for this
 * processor.
 */
static
NTSTATUS
NTAPI
PpmDispUpdateProcessorIdleVeto(
    _In_ PVOID ProcessorIdleVeto)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(ProcessorIdleVeto);

    /* FIXME: Update per-CPU veto accounting. */
    return STATUS_SUCCESS;
}

/*
 * PpmDispUpdatePlatformIdleVeto
 *
 * Updates veto counters for a platform-level idle state.
 */
static
NTSTATUS
NTAPI
PpmDispUpdatePlatformIdleVeto(
    _In_ PVOID PlatformIdleVeto)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PlatformIdleVeto);

    /* FIXME: Update platform veto accounting. */
    return STATUS_SUCCESS;
}

/* Hyper-V stubs – ReactOS does not run under Hyper-V as a partition manager. */

static
NTSTATUS
NTAPI
PpmDispRegisterPerfStatesHv(
    _In_ PVOID PerfStatesHv)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PerfStatesHv);
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
NTAPI
PpmDispRegisterPerfCapHv(
    _In_ PVOID PerfCapHv)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PerfCapHv);
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
NTAPI
PpmDispRegisterIdleStatesHv(
    _In_ PVOID IdleStatesHv)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(IdleStatesHv);
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
NTAPI
PpmDispRegisterPerfStatesCountersHv(
    _In_ PVOID PerfStatesCountersHv)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PerfStatesCountersHv);
    return STATUS_NOT_SUPPORTED;
}

/* PEP (Platform Extension Plugin) stubs – PEP infrastructure is separate. */

static
NTSTATUS
NTAPI
PpmDispSetProcessorPep(
    _In_ PVOID PepHandle)
{
    PKPRCB Prcb;

    PAGED_CODE();

    Prcb = KeGetCurrentPrcb();
    Prcb->PowerState.PpmPepHandle = PepHandle;

    DPRINT("PpmDispSetProcessorPep: CPU %u PEP @ %p\n", Prcb->Number, PepHandle);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
PpmDispParkPreferenceNotification(
    _In_ PVOID PepHandle,
    _In_ PVOID Notification)
{
    PPPM_PEP_PARK_PREFERENCE_NOTIFICATION Pref;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(PepHandle);

    if (Notification == NULL)
        return STATUS_INVALID_PARAMETER;

    Pref = (PPPM_PEP_PARK_PREFERENCE_NOTIFICATION)Notification;
    if (Pref->Version != 1)
        return STATUS_INVALID_PARAMETER;

    PpmCoreParkingPepPreferencePercent = min(Pref->PreferencePercent, 100);

    DPRINT("PpmDispParkPreferenceNotification: preference=%lu%%\n",
           PpmCoreParkingPepPreferencePercent);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
PpmDispParkMaskNotification(
    _In_ PVOID PepHandle,
    _In_ PVOID Notification)
{
    PPPM_PEP_PARK_MASK_NOTIFICATION MaskNotif;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(PepHandle);

    if (Notification == NULL)
    {
        PpmCoreParkingPepOverrideActive = FALSE;
        return STATUS_SUCCESS;
    }

    MaskNotif = (PPPM_PEP_PARK_MASK_NOTIFICATION)Notification;
    if (MaskNotif->Version != 1)
        return STATUS_INVALID_PARAMETER;

    PpmCoreParkingPepParkMask = MaskNotif->ParkMask;
    PpmCoreParkingPepOverrideActive = TRUE;

    DPRINT("PpmDispParkMaskNotification: ParkMask=%Ix\n",
           (ULONG_PTR)PpmCoreParkingPepParkMask);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
PpmDispIdleSelectNotification(
    _In_ PVOID PepHandle,
    _In_ PVOID Notification)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(PepHandle);
    UNREFERENCED_PARAMETER(Notification);

    /* Reserved for coordinated idle / PEP selection overrides. */
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
PpmDispQueryPlatformStateNotification(
    _In_ PVOID PepHandle,
    _In_ PVOID Notification,
    _In_ BOOLEAN Update)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PepHandle);
    UNREFERENCED_PARAMETER(Notification);
    UNREFERENCED_PARAMETER(Update);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
PpmDispQueryCoordinatedDependencyNotification(
    _In_ PVOID PepHandle,
    _In_ PVOID Notification)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PepHandle);
    UNREFERENCED_PARAMETER(Notification);

    return STATUS_SUCCESS;
}

static
VOID
NTAPI
PpmDispRegisterEnergyEstimation(
    _In_ PVOID ComputeEnergy,
    _In_ PVOID SnapCounters)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(ComputeEnergy);
    UNREFERENCED_PARAMETER(SnapCounters);

    /* FIXME: Wire energy estimation callbacks into the PPM policy engine. */
}

/* INIT *********************************************************************/

/*
 * PpmInitDispatchTable
 *
 * Populates the global PpmKernelDispatchTable with the function pointers
 * defined above.  Must be called during power manager initialisation
 * (PpmInitialize early phase) before any processor driver is loaded.
 */
CODE_SEG("INIT")
VOID
NTAPI
PpmInitDispatchTable(VOID)
{
    PPM_DRIVER_DISPATCH_TABLE *Table = &PpmKernelDispatchTable;

    Table->InterfaceVersion                      = PPM_DRIVER_INTERFACE_VERSION;
    Table->RegisterPerfStates                    = PpmDispRegisterPerfStates;
    Table->UpdatePerfStates                      = PpmDispUpdatePerfStates;
    Table->RegisterPerfCap                       = PpmDispRegisterPerfCap;
    Table->RegisterSpmSettings                   = PpmDispRegisterSpmSettings;
    Table->RegisterIdleStates                    = PpmDispRegisterIdleStates;
    Table->RegisterIdleDomains                   = PpmDispRegisterIdleDomains;
    Table->RegisterPlatformStates                = PpmDispRegisterPlatformStates;
    Table->RegisterCoordinatedStates             = PpmDispRegisterCoordinatedStates;
    Table->RegisterVetoList                      = PpmDispRegisterVetoList;
    Table->RemoveVetoBias                        = PpmDispRemoveVetoBias;
    Table->UpdateProcessorIdleVeto               = PpmDispUpdateProcessorIdleVeto;
    Table->UpdatePlatformIdleVeto                = PpmDispUpdatePlatformIdleVeto;
    Table->RegisterPerfStatesHv                  = PpmDispRegisterPerfStatesHv;
    Table->RegisterPerfCapHv                     = PpmDispRegisterPerfCapHv;
    Table->RegisterIdleStatesHv                  = PpmDispRegisterIdleStatesHv;
    Table->RegisterPerfStatesCountersHv          = PpmDispRegisterPerfStatesCountersHv;
    Table->SetProcessorPep                       = PpmDispSetProcessorPep;
    Table->ParkPreferenceNotification            = PpmDispParkPreferenceNotification;
    Table->ParkMaskNotification                  = PpmDispParkMaskNotification;
    Table->IdleSelectNotification                = PpmDispIdleSelectNotification;
    Table->QueryPlatformStateNotification        = PpmDispQueryPlatformStateNotification;
    Table->QueryCoordinatedDependencyNotification= PpmDispQueryCoordinatedDependencyNotification;
    Table->RegisterEnergyEstimation              = PpmDispRegisterEnergyEstimation;

    PpmCoreParkingInitialize();
}
