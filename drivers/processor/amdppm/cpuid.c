/*
 * PROJECT:     ReactOS AMD Processor Power Management Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        drivers/processor/amdppm/cpuid.c
 * PURPOSE:     AMD CPUID capability detection and MSR-based P-state control.
 *
 *              This file detects which processor power management features the
 *              running AMD CPU exposes through CPUID leaves and provides
 *              wrappers for MSR-based performance-state transitions (FFH
 *              "Functional Fixed Hardware" mechanism, ACPI spec §8.4.6.1).
 *
 * REFERENCES:  AMD64 Architecture Programmer's Manual Vol. 2 & 3
 *              ACPI Specification 6.x, §8.4.6.1 (FFH execution)
 *              Windows 10 amdppm.sys (IDA decompilation reference)
 *
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 */

/* INCLUDES ******************************************************************/

#include "amdppm.h"
#include <intrin.h>     /* __cpuid, __readmsr, __writemsr            */

/* DEFINES *******************************************************************/

/*
 * CPUID leaves and sub-leaves relevant to AMD power management.
 */
#define CPUID_VENDOR_STRING_LEAF        0x00000000
#define CPUID_FEATURE_INFO_LEAF         0x00000001
#define CPUID_EXTENDED_FEATURES_LEAF    0x80000000  /* highest ext leaf      */
#define CPUID_AMD_POWER_MGMT_LEAF       0x80000007  /* advanced power mgmt   */
#define CPUID_ADDR_SIZES_LEAF           0x80000008

/*
 * CPUID AMD extended feature bits (leaf 0x80000007, EDX).
 */
#define CPUID_APM_FREQ_ID_CTLS          (1 << 1)  /* FID/VID change support */
#define CPUID_APM_VID_CTLS              (1 << 2)  /* Voltage ID control     */
#define CPUID_APM_THERMTRIP             (1 << 4)  /* Thermal trip support   */
#define CPUID_APM_HW_THERMAL_CTRL       (1 << 5)  /* HW thermal control     */
#define CPUID_APM_SW_THERMAL_CTRL       (1 << 6)  /* SW thermal control     */
#define CPUID_APM_100MHZ_STEPS          (1 << 7)  /* P-state 100 MHz steps  */
#define CPUID_APM_HW_PSTATE_CTRL        (1 << 8)  /* Hardware P-state ctrl  */
#define CPUID_APM_TSC_INVARIANT         (1 << 9)  /* TSC invariant          */
#define CPUID_APM_CPB                   (1 << 10) /* Core Performance Boost */

/*
 * CPUID feature flags (leaf 0x00000001, ECX / EDX).
 */
#define CPUID_FEATURE_MWAIT             (1 << 3)  /* ECX: MONITOR/MWAIT     */
#define CPUID_FEATURE_EST               (1 << 7)  /* ECX: Enhanced SpeedStep */

/*
 * AMD Model-Specific Registers used for FFH P-state control.
 */
#define MSR_AMD_PSTATE_CTRL             0xC0010062  /* Write: request P-state */
#define MSR_AMD_PSTATE_STATUS           0xC0010063  /* Read:  current P-state */
#define MSR_AMD_PSTATE_DEF_BASE         0xC0010064  /* P-state definition[0]  */

/* Number of AMD P-state definition MSRs */
#define AMD_PSTATE_DEF_COUNT            8

/*
 * Vendor string for AMD processors (little-endian char packing).
 */
#define AMD_VENDOR_EBX  'htuA'  /* "Auth" */
#define AMD_VENDOR_ECX  'DMAc'  /* "cAMD" */
#define AMD_VENDOR_EDX  'itne'  /* "enti" */

/* PRIVATE HELPERS ***********************************************************/

/*
 * AmdReadMsr
 *
 * Reads a 64-bit MSR.  Wrapped to allow future mocking/virtualisation.
 */
static
ULONGLONG
AmdReadMsr(
    _In_ ULONG MsrIndex)
{
    return __readmsr(MsrIndex);
}

/*
 * AmdWriteMsr
 *
 * Writes a 64-bit MSR.
 */
static
VOID
AmdWriteMsr(
    _In_ ULONG     MsrIndex,
    _In_ ULONGLONG Value)
{
    __writemsr(MsrIndex, Value);
}

/* PUBLIC: CPUID WRAPPERS ****************************************************/

/*
 * GetCpuIdInfo
 *
 * Executes the CPUID instruction for the given Function (leaf) and stores
 * the four output registers in Results[0..3] = EAX, EBX, ECX, EDX.
 */
VOID
GetCpuIdInfo(
    _In_  ULONG   Function,
    _Out_ PULONG  Results)
{
    int CpuidResult[4];

    __cpuid(CpuidResult, (int)Function);

    Results[0] = (ULONG)CpuidResult[0]; /* EAX */
    Results[1] = (ULONG)CpuidResult[1]; /* EBX */
    Results[2] = (ULONG)CpuidResult[2]; /* ECX */
    Results[3] = (ULONG)CpuidResult[3]; /* EDX */
}

/* PUBLIC: PROCESSOR IDENTIFICATION ******************************************/

/*
 * IsAmdProcessor
 *
 * Returns TRUE if CPUID leaf 0 identifies the processor as AMD.
 */
BOOLEAN
IsAmdProcessor(VOID)
{
    ULONG Regs[4];

    GetCpuIdInfo(CPUID_VENDOR_STRING_LEAF, Regs);

    return (Regs[1] == AMD_VENDOR_EBX &&
            Regs[2] == AMD_VENDOR_ECX &&
            Regs[3] == AMD_VENDOR_EDX);
}

/*
 * AmdPpmDeviceStart
 *
 * Detects which AMD CPU power management features are available via CPUID
 * and returns a bitmask of AMD_CAP_* flags.
 *
 * PlatformIndex: logical processor index (used to set affinity when probing).
 * Returns 0 if the processor is not AMD or does not expose advanced PM.
 */
ULONG
AmdPpmDeviceStart(
    _In_ ULONG PlatformIndex)
{
    ULONG Regs[4];
    ULONG MaxExtendedLeaf;
    ULONG Caps = 0;
    KAFFINITY OldAffinity;
    KAFFINITY TargetAffinity;

    PAGED_CODE();

    /* Pin to the target processor so CPUID reflects the right CPU */
    TargetAffinity = (KAFFINITY)1 << PlatformIndex;
    OldAffinity = KeSetSystemAffinityThreadEx(TargetAffinity);

    __try
    {
        /* Verify AMD vendor */
        if (!IsAmdProcessor())
        {
            DPRINT1("AmdPpm: AmdPpmDeviceStart – not AMD on CPU %lu\n",
                    PlatformIndex);
            __leave;
        }

        /* Discover highest extended leaf */
        GetCpuIdInfo(CPUID_EXTENDED_FEATURES_LEAF, Regs);
        MaxExtendedLeaf = Regs[0];

        if (MaxExtendedLeaf < CPUID_AMD_POWER_MGMT_LEAF)
        {
            DPRINT("AmdPpm: CPU %lu: no extended APM leaf\n", PlatformIndex);
            __leave;
        }

        /* Read the advanced power management information leaf */
        GetCpuIdInfo(CPUID_AMD_POWER_MGMT_LEAF, Regs);

        /* EAX: thermal & power management feature flags for the platform */
        /* ECX: number of effective frequency ID values                   */
        /* EDX: advanced PM feature identifiers                           */

        if (Regs[3] & CPUID_APM_HW_PSTATE_CTRL)
        {
            /*
             * Hardware P-state control via PSTATE_CTRL MSR.
             * This is the AMD-specific "FFH" mechanism.
             */
            Caps |= AMD_CAP_FFH;
            DPRINT("AmdPpm: CPU %lu: HW P-state control (FFH) supported\n",
                   PlatformIndex);
        }

        if (Regs[3] & CPUID_APM_CPB)
        {
            Caps |= AMD_CAP_BOOST;
            DPRINT("AmdPpm: CPU %lu: Core Performance Boost supported\n",
                   PlatformIndex);
        }

        if (Regs[3] & CPUID_APM_FREQ_ID_CTLS)
        {
            /* Legacy FID/VID change support – implies P-states available */
            Caps |= AMD_CAP_PSS;
        }

        /* MONITOR/MWAIT → C-state support */
        GetCpuIdInfo(CPUID_FEATURE_INFO_LEAF, Regs);
        if (Regs[2] & CPUID_FEATURE_MWAIT)
        {
            Caps |= AMD_CAP_CST;
            DPRINT("AmdPpm: CPU %lu: MONITOR/MWAIT (C-states) supported\n",
                   PlatformIndex);
        }
    }
    __finally
    {
        KeRevertToUserAffinityThreadEx(OldAffinity);
    }

    return Caps;
}

/* PUBLIC: P-STATE VALIDATION ************************************************/

/*
 * ValidatePStateCapability
 *
 * Validates that the _PCT register descriptors and _PSS table are
 * compatible.  Checks:
 *   1. _PCT Control register has a non-zero address.
 *   2. _PSS has at least one entry.
 *   3. Each P-state entry has a non-zero CoreFrequency.
 *
 * ValidationErrors receives a bitmask of error codes:
 *   bit 0: PCT control address is zero
 *   bit 1: PSS is empty
 *   bit 2: One or more PSS entries have zero frequency
 */
NTSTATUS
ValidatePStateCapability(
    _In_  PACPI_CTRL_STATUS  PCT,
    _In_  PACPI_PSS          PSS,
    _Out_ PULONG             ValidationErrors)
{
    ULONG Errors = 0;
    ULONG i;

    PAGED_CODE();

    *ValidationErrors = 0;

    if (!PCT || !PSS)
        return STATUS_INVALID_PARAMETER;

    /* 1. PCT control register address must be non-zero */
    if (PCT->Control.Address.QuadPart == 0)
    {
        DPRINT1("AmdPpm: ValidatePStateCapability: PCT control address is 0\n");
        Errors |= 0x1;
    }

    /* 2. PSS must have at least one entry */
    if (PSS->Count == 0)
    {
        DPRINT1("AmdPpm: ValidatePStateCapability: PSS is empty\n");
        Errors |= 0x2;
    }

    /* 3. Each PSS entry must have a non-zero frequency */
    for (i = 0; i < PSS->Count; i++)
    {
        if (PSS->States[i].CoreFrequency == 0)
        {
            DPRINT1("AmdPpm: ValidatePStateCapability: PSS[%lu] has zero "
                    "frequency\n", i);
            Errors |= 0x4;
            break;
        }
    }

    *ValidationErrors = Errors;

    return (Errors == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/* PUBLIC: FFH P-STATE CONTROL ***********************************************/

/*
 * SetFFHPState
 *
 * Performs a Functional Fixed Hardware (FFH) P-state transition by writing
 * the target P-state index to the AMD PSTATE_CTRL MSR (0xC0010062) and
 * verifying it via PSTATE_STATUS (0xC0010063).
 *
 * ControlValue: P-state index to request.
 * StatusValue:  Expected P-state index in the status register.
 *
 * Must be called on the target logical processor (caller sets affinity).
 */
NTSTATUS
SetFFHPState(
    _In_ ULONGLONG ControlValue,
    _In_ ULONGLONG StatusValue)
{
    ULONGLONG PStateStatus;
    ULONG Retries = 100;

    /* Write the desired P-state index to PSTATE_CTRL */
    AmdWriteMsr(MSR_AMD_PSTATE_CTRL, ControlValue & 0x7);

    /*
     * Poll PSTATE_STATUS until the hardware acknowledges the transition
     * or we time out.  Each iteration represents approximately 1 µs.
     */
    do
    {
        PStateStatus = AmdReadMsr(MSR_AMD_PSTATE_STATUS) & 0x7;
        if (PStateStatus == (StatusValue & 0x7))
            return STATUS_SUCCESS;

        KeStallExecutionProcessor(1);

    } while (--Retries > 0);

    DPRINT1("AmdPpm: SetFFHPState: timeout waiting for P-state %llu "
            "(current %llu)\n", StatusValue & 0x7, PStateStatus);

    return STATUS_IO_TIMEOUT;
}

/* PUBLIC: BOOST POLICY ******************************************************/

/*
 * SetPerformanceBoostMode
 *
 * Enables or disables AMD Core Performance Boost (CPB) for the current
 * processor by writing to the PSTATE_CTRL MSR.
 *
 * Context: opaque context value (logical processor index).
 * Policy:  0 = disable boost, non-zero = enable boost.
 *
 * Note: CPB control via MSR is model-specific.  This implementation
 * targets Family 10h / 14h and later AMD processors.
 */
VOID
SetPerformanceBoostMode(
    _In_ ULONG Context,
    _In_ ULONG Policy)
{
    /*
     * Boost disable is controlled via HWCR MSR (0xC0010015) bit 25
     * on Family 10h/12h/14h/15h/16h AMD processors.
     */
#define MSR_AMD_HWCR            0xC0010015
#define HWCR_CPB_DISABLE        (1ULL << 25)

    ULONGLONG HwCr;

    UNREFERENCED_PARAMETER(Context);

    HwCr = AmdReadMsr(MSR_AMD_HWCR);

    if (Policy == 0)
    {
        /* Disable boost */
        HwCr |= HWCR_CPB_DISABLE;
    }
    else
    {
        /* Enable boost */
        HwCr &= ~HWCR_CPB_DISABLE;
    }

    AmdWriteMsr(MSR_AMD_HWCR, HwCr);
}
