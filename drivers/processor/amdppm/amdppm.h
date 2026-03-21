/*
 * PROJECT:     ReactOS AMD Processor Power Management Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        drivers/processor/amdppm/amdppm.h
 * PURPOSE:     Main header for the AMD PPM (Processor Power Management) driver.
 *
 *              This driver manages C-states (idle power states), P-states
 *              (performance/frequency scaling states), and T-states (thermal
 *              throttle states) for AMD processors.  It communicates with the
 *              kernel Power Manager through the PPM_DRIVER_DISPATCH_TABLE
 *              obtained via ZwPowerInformation(ProcessorStateHandler, ...).
 *
 * REFERENCES:  Windows 10 amdppm.sys (IDA decompilation), AMD ACPI spec,
 *              ACPI 6.x specification.
 *
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 */

#pragma once

/* ---- includes ------------------------------------------------------------- */

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include <ntpoapi.h>

#define NDEBUG
#include <debug.h>

/* ---- pool tags ------------------------------------------------------------ */

#define TAG_AMDPPM_GENERIC      'mppA'   /* AmdPpm – generic allocations     */
#define TAG_AMDPPM_ACPI         'cpAA'   /* AmdPpm ACPI output buffer        */
#define TAG_AMDPPM_PSS          'sspA'   /* AmdPpm _PSS / XPSS table         */
#define TAG_AMDPPM_CST          'tscA'   /* AmdPpm _CST table                */
#define TAG_AMDPPM_TSS          'sstA'   /* AmdPpm _TSS table                */

/* ---- PPM driver dispatch table ------------------------------------------- */

/*
 * Interface version exchanged with the kernel.
 * Must match PPM_DRIVER_INTERFACE_VERSION (43) in ntoskrnl ppm.h.
 */
#define PPM_DRIVER_INTERFACE_VERSION    43

/*
 * PPM_DRIVER_DISPATCH_TABLE
 *
 * Kernel-side dispatch table that processor drivers (amdppm, intelppm, ...)
 * retrieve with:
 *     ZwPowerInformation(ProcessorStateHandler, NULL, 0,
 *                        &Table, sizeof(Table));
 *
 * Each function pointer is a kernel callback for registering various power
 * management information (P-states, C-states, performance caps, etc.).
 *
 * Layout matches the Windows kernel exactly; size is 0x60 bytes on x86-32.
 */
typedef struct _PPM_DRIVER_DISPATCH_TABLE
{
    ULONG    InterfaceVersion;
    NTSTATUS (NTAPI *RegisterPerfStates)(PVOID PerfStates);
    VOID     (NTAPI *UpdatePerfStates)(PVOID PerfStatesUpdate);
    NTSTATUS (NTAPI *RegisterPerfCap)(PVOID ProcessorCap);
    NTSTATUS (NTAPI *RegisterSpmSettings)(PVOID SpmSettings);
    NTSTATUS (NTAPI *RegisterIdleStates)(PVOID IdleStates);
    NTSTATUS (NTAPI *RegisterIdleDomains)(PVOID IdleDomains);
    NTSTATUS (NTAPI *RegisterPlatformStates)(PVOID PlatformIdleStates);
    NTSTATUS (NTAPI *RegisterCoordinatedStates)(PVOID CoordinatedIdleStates);
    NTSTATUS (NTAPI *RegisterVetoList)(PVOID VetoList);
    NTSTATUS (NTAPI *RemoveVetoBias)(VOID);
    NTSTATUS (NTAPI *UpdateProcessorIdleVeto)(PVOID ProcessorIdleVeto);
    NTSTATUS (NTAPI *UpdatePlatformIdleVeto)(PVOID PlatformIdleVeto);
    NTSTATUS (NTAPI *RegisterPerfStatesHv)(PVOID PerfStatesHv);
    NTSTATUS (NTAPI *RegisterPerfCapHv)(PVOID PerfCapHv);
    NTSTATUS (NTAPI *RegisterIdleStatesHv)(PVOID IdleStatesHv);
    NTSTATUS (NTAPI *RegisterPerfStatesCountersHv)(PVOID PerfStatesCountersHv);
    NTSTATUS (NTAPI *SetProcessorPep)(PVOID PepHandle);
    NTSTATUS (NTAPI *ParkPreferenceNotification)(PVOID PepHandle, PVOID Notification);
    NTSTATUS (NTAPI *ParkMaskNotification)(PVOID PepHandle, PVOID Notification);
    NTSTATUS (NTAPI *IdleSelectNotification)(PVOID PepHandle, PVOID Notification);
    NTSTATUS (NTAPI *QueryPlatformStateNotification)(PVOID PepHandle,
                                                     PVOID Notification,
                                                     BOOLEAN Update);
    NTSTATUS (NTAPI *QueryCoordinatedDependencyNotification)(PVOID PepHandle,
                                                              PVOID Notification);
    VOID     (NTAPI *RegisterEnergyEstimation)(PVOID ComputeEnergy,
                                                PVOID SnapCounters);
} PPM_DRIVER_DISPATCH_TABLE, *PPPM_DRIVER_DISPATCH_TABLE;

C_ASSERT(sizeof(PPM_DRIVER_DISPATCH_TABLE) == 0x60);

/* ---- ACPI method argument structures ------------------------------------- */

/*
 * ACPI_CST_STATE
 * Single entry in the _CST package (ACPI 6.x §8.4.2.1).
 */
typedef struct _ACPI_CST_STATE
{
    ULONG       Type;       /* 1=C1, 2=C2, 3=C3                        */
    ULONG       Latency;    /* Worst-case entry latency in µs           */
    ULONG       Power;      /* Average power in mW                      */
} ACPI_CST_STATE, *PACPI_CST_STATE;

/*
 * ACPI_CST
 * Complete _CST return value: count followed by ACPI_CST_STATE entries.
 */
typedef struct _ACPI_CST
{
    ULONG           Count;
    ACPI_CST_STATE  States[ANYSIZE_ARRAY];
} ACPI_CST, *PACPI_CST;

/*
 * ACPI_PSS_STATE
 * Single entry in the _PSS / _XPSS package (ACPI 6.x §8.4.6).
 */
typedef struct _ACPI_PSS_STATE
{
    ULONG       CoreFrequency;  /* MHz                                  */
    ULONG       Power;          /* mW                                   */
    ULONG       TransitionLatency; /* µs                               */
    ULONG       BusMasterLatency;  /* µs                               */
    ULONG       Control;        /* Value to write to _PCT register      */
    ULONG       Status;         /* Expected value after transition      */
} ACPI_PSS_STATE, *PACPI_PSS_STATE;

/*
 * ACPI_PSS
 * Complete _PSS return value: count followed by ACPI_PSS_STATE entries.
 */
typedef struct _ACPI_PSS
{
    ULONG           Count;
    ACPI_PSS_STATE  States[ANYSIZE_ARRAY];
} ACPI_PSS, *PACPI_PSS;

/*
 * GEN_ADDR (Generic Address Structure, ACPI §5.2.3.2)
 * Used by _PCT, _PTC, _CSD, _PSD etc.
 */
typedef struct _GEN_ADDR
{
    UCHAR   AddressSpaceID;   /* 0=memory, 1=IO, 2=PCI config, 0x7F=FFHW */
    UCHAR   RegisterBitWidth;
    UCHAR   RegisterBitOffset;
    UCHAR   AccessSize;
    LARGE_INTEGER Address;
} GEN_ADDR, *PGEN_ADDR;

/*
 * ACPI_CTRL_STATUS
 * Returned by _PCT (performance control/status registers) or
 * _PTC (throttle control/status registers).
 */
typedef struct _ACPI_CTRL_STATUS
{
    GEN_ADDR    Control;
    GEN_ADDR    Status;
} ACPI_CTRL_STATUS, *PACPI_CTRL_STATUS;

/*
 * ACPI_XSD_ENTRY
 * Single entry in a _PSD / _TSD dependency package.
 */
typedef struct _ACPI_XSD_ENTRY
{
    ULONG   NumEntries;
    ULONG   Revision;
    ULONG   Domain;
    ULONG   CoordType;      /* 0xFC=SW_ALL, 0xFD=SW_ANY, 0xFE=HW_ALL   */
    ULONG   NumProcessors;
} ACPI_XSD_ENTRY, *PACPI_XSD_ENTRY;

/*
 * ACPI_XSD
 * Complete _PSD / _TSD return: count followed by entries.
 */
typedef struct _ACPI_XSD
{
    ULONG           Count;
    ACPI_XSD_ENTRY  Entries[ANYSIZE_ARRAY];
} ACPI_XSD, *PACPI_XSD;

/*
 * ACPI_TSS_STATE
 * Single throttle state entry from _TSS.
 */
typedef struct _ACPI_TSS_STATE
{
    ULONG   Percent;
    ULONG   Power;
    ULONG   TransitionLatency;
    ULONG   Control;
    ULONG   Status;
} ACPI_TSS_STATE, *PACPI_TSS_STATE;

/*
 * ACPI_TSS
 * Complete _TSS return.
 */
typedef struct _ACPI_TSS
{
    ULONG           Count;
    ACPI_TSS_STATE  States[ANYSIZE_ARRAY];
} ACPI_TSS, *PACPI_TSS;

/*
 * OSC_INPUT_BUFFER
 * _OSC input: UUID + revision + count of capabilities DWORDs + capabilities.
 */
typedef struct _OSC_INPUT_BUFFER
{
    UCHAR   Uuid[16];
    ULONG   Revision;
    ULONG   Count;
    ULONG   Capabilities[2];
} OSC_INPUT_BUFFER, *POSC_INPUT_BUFFER;

/*
 * OSC_OUTPUT_BUFFER
 * _OSC output: status DWORD + capabilities DWORDs.
 */
typedef struct _OSC_OUTPUT_BUFFER
{
    ULONG   Status;
    ULONG   Capabilities[2];
} OSC_OUTPUT_BUFFER, *POSC_OUTPUT_BUFFER;

/*
 * PDC_INPUT_BUFFER
 * _PDC (processor driver capabilities) input: revision + count + caps.
 */
typedef struct _PDC_INPUT_BUFFER
{
    ULONG   Revision;
    ULONG   Count;
    ULONG   Capabilities[ANYSIZE_ARRAY];
} PDC_INPUT_BUFFER, *PPDC_INPUT_BUFFER;

/* ---- ACPI interface ------------------------------------------------------- */

/*
 * ACPI method evaluation is performed via synchronous IOCTL_ACPI_EVAL_METHOD
 * requests sent to the processor PDO (DevExt->Pdo).  There is no separate
 * "ACPI interface" to acquire – the PDO is obtained from WDF during
 * EvtDevicePrepareHardware and is valid for the lifetime of the device.
 *
 * AcquireAcpiInterfaces() takes an object-manager reference on the PDO and
 * stores the flag in DevExt->AcpiPdoReferenced.
 * ReleaseAcpiInterfaces() drops that reference.
 */

/* ---- Kernel PPM structures (passed to dispatch-table callbacks) ----------- */

/*
 * PROCESSOR_IDLE_STATE_EX
 *
 * Describes a single processor idle (C-state) state entry.
 * Passed inside PROCESSOR_IDLE_STATES_EX to RegisterIdleStates().
 *
 * Layout matches Windows 10 (processr.sys/amdppm.sys reference).
 */
typedef union _PROCESSOR_IDLE_STATE_EX_FLAGS
{
    struct
    {
        ULONG Interruptible         : 1;  /* C-state may be interrupted        */
        ULONG CacheCoherent         : 1;  /* Cache coherency maintained        */
        ULONG ThreadContextRetained : 1;  /* Thread context retained on wake   */
        ULONG CStateType            : 4;  /* ACPI C-state type (1=C1, 2=C2...) */
        ULONG WakesSpuriously       : 1;  /* May generate spurious wakeups     */
        ULONG PlatformOnly          : 1;  /* Platform-level state, not per-CPU */
        ULONG Reserved              : 21;
        ULONG NoCState              : 1;  /* Not a real C-state entry          */
        ULONG InterruptsEnabled     : 1;  /* Interrupts enabled in this state  */
    };
    ULONG AsUlong;
} PROCESSOR_IDLE_STATE_EX_FLAGS;

typedef struct _PROCESSOR_IDLE_STATE_EX
{
    PROCESSOR_IDLE_STATE_EX_FLAGS Flags;
    ULONG                         Latency;           /* Entry latency in µs  */
    ULONG                         BreakEvenDuration; /* Minimum idle time µs */
    ULONG                         Power;             /* Average power in mW  */
    UNICODE_STRING                Name;              /* Optional state name  */
} PROCESSOR_IDLE_STATE_EX, *PPROCESSOR_IDLE_STATE_EX;

/*
 * PROCESSOR_IDLE_STATES_EX
 *
 * Container for all C-state entries plus the driver callbacks the kernel
 * invokes when entering/exiting idle states.
 *
 * Layout matches Windows 10 (processr.sys/amdppm.sys reference).
 * The structure is variable-length: State[ProcessorIdleCount].
 */
typedef struct _PROCESSOR_IDLE_PREPARE_INFO  *PPROCESSOR_IDLE_PREPARE_INFO;
typedef struct _PROCESSOR_IDLE_CONSTRAINTS   *PPROCESSOR_IDLE_CONSTRAINTS;

typedef struct _PROCESSOR_IDLE_STATES_EX
{
    ULONG               Version;           /* Must be 1                        */
    PROCESSOR_NUMBER    Processor;         /* Logical processor index          */
    PVOID               Context;           /* Driver context for callbacks     */
    BOOLEAN             EstimateIdleDuration; /* Driver estimates duration     */
    BOOLEAN             Update;            /* TRUE = update existing states    */
    UCHAR               InterfaceVersion;  /* Internal version tag             */

    /* Driver-provided callbacks invoked by the kernel idle path */
    VOID    (FASTCALL *IdlePrepare)       (_In_  PPROCESSOR_IDLE_PREPARE_INFO Info);
    VOID    (FASTCALL *IdleCancel)        (_In_  PVOID Context, _In_ ULONG StateIndex);
    ULONG   (FASTCALL *IdlePreselect)     (_In_  PVOID Context,
                                           _In_  PPROCESSOR_IDLE_CONSTRAINTS Constraints);
    ULONG   (FASTCALL *IdleTest)          (_In_  PVOID Context,
                                           _In_  ULONG StateIndex,
                                           _In_  ULONG Duration);
    ULONG   (FASTCALL *IdleAvailabilityCheck)(_In_  PVOID Context,
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

    ULONG                   MaximumDependencies; /* Max coordinator states    */
    ULONG                   ProcessorIdleCount;  /* Number of C-state entries */
    PROCESSOR_IDLE_STATE_EX State[ANYSIZE_ARRAY];/* Variable-length C-states  */
} PROCESSOR_IDLE_STATES_EX, *PPROCESSOR_IDLE_STATES_EX;

/*
 * PROCESSOR_PERF_INFO
 *
 * Per-processor identity record inside PROCESSOR_PERF_STATES.
 */
typedef struct _PROCESSOR_PERF_INFO
{
    ULONG   InitialApicId;   /* APIC ID from CPUID leaf 1 EBX[31:24]          */
    ULONG   ProcessorIndex;  /* Logical processor index                       */
} PROCESSOR_PERF_INFO, *PPROCESSOR_PERF_INFO;

/*
 * PROCESSOR_PERF_STATES
 *
 * Describes the full P-state configuration for a logical processor (or a
 * group of processors that share a performance domain).
 *
 * Version, Type, and the function pointers are the fields the kernel
 * scheduler uses; the remaining fields are informational.  On ReactOS the
 * kernel dispatch stubs store the pointer for future use.
 *
 * Simplified from the Windows 10 internal structure; function pointers
 * that ReactOS has not yet implemented are included as PVOID placeholders
 * to preserve the binary layout.
 */
#define PROCESSOR_PERF_STATES_VERSION   1

/* Type field values */
#define PPM_PERF_STATE_TYPE_ACPI_IO     0   /* Legacy ACPI throttle I/O port  */
#define PPM_PERF_STATE_TYPE_ACPI_MSR    1   /* AMD/Intel MSR-based P-states   */
#define PPM_PERF_STATE_TYPE_ACPI_FFH    2   /* Functional Fixed Hardware      */

typedef struct _PROCESSOR_PERF_STATES
{
    /* ---- Common header --------------------------------------------------- */
    ULONG           Version;            /* PROCESSOR_PERF_STATES_VERSION (1)  */
    USHORT          Type;               /* PPM_PERF_STATE_TYPE_*              */
    BOOLEAN         HardPlatformCap;    /* TRUE = BIOS caps are in effect     */
    BOOLEAN         AffinitizeControl;  /* TRUE = all CPUs in domain together */
    BOOLEAN         EfficientThrottle;  /* TRUE = use efficient throttle      */
    ULONG           ProcessorCount;     /* Number of processors in domain     */
    ULONG           NominalFrequency;   /* MHz at 100% perf                   */
    ULONG           MaxPerfPercent;     /* 0-100 platform-allowed maximum     */
    ULONG           MinPerfPercent;     /* 0-100 minimum performance          */
    ULONG           MinThrottlePercent; /* 0-100 minimum throttle level       */

    /* ---- Processor domain ------------------------------------------------ */
    ULONG           GlobalContext;      /* Driver-private domain context      */
    KAFFINITY_EX    TargetProcessors;   /* Processors in the P-state domain   */

    /* ---- Driver callbacks (PVOID placeholders) --------------------------- */
    PVOID           GetFFHThrottleState;     /* Retrieve current FFH state    */
    PVOID           TimeWindowHandler;       /* Notify: time window change    */
    PVOID           BoostPolicyHandler;      /* Notify: boost policy change   */
    PVOID           BoostModeHandler;        /* Notify: boost mode change     */
    PVOID           EnergyPerfPreferenceHandler;
    PVOID           AutonomousActivityWindowHandler;
    PVOID           AutonomousModeHandler;
    PVOID           StartPolicyUpdate;
    PVOID           CompletePolicyUpdate;
    PVOID           ReinitializeHandler;
    PVOID           PerfSelectionHandler;   /* Select P-state for given load  */
    PVOID           PerfControlHandler;     /* Write control register         */
    PVOID           PerfControlReadFeedback;
    PVOID           PerfControlAcquirePerformance;
    PVOID           PerfControlCommitPerformance;
    PVOID           ParkPreference;         /* Core park preference           */
    PVOID           ParkMask;               /* Core park mask                 */
    PVOID           PerfCheckComplete;      /* Perf-check epoch complete      */

    /* ---- Per-processor info array ---------------------------------------- */
    PPROCESSOR_PERF_INFO Processors;       /* Array of ProcessorCount entries */

} PROCESSOR_PERF_STATES, *PPROCESSOR_PERF_STATES;

/*
 * PROCESSOR_CAP
 *
 * Performance capability cap reported to the kernel on D0 entry or on receipt
 * of ACPI notification 0x80 (PPC) and 0x81 (TPC).  Passed to RegisterPerfCap.
 *
 * Layout matches Windows 10 (processr.sys reference, struct at offset 7424).
 */
#define PROCESSOR_CAP_VERSION   1

typedef struct _PROCESSOR_CAP
{
    ULONG            Version;          /* PROCESSOR_CAP_VERSION (1)            */
    PROCESSOR_NUMBER ProcessorNumber;  /* Logical processor number             */
    ULONG            PlatformCap;      /* Highest P-state allowed by platform  */
    ULONG            ThermalCap;       /* Highest P-state allowed thermally    */
    ULONG            LimitReasons;     /* Bitmask of reasons for cap           */
} PROCESSOR_CAP, *PPROCESSOR_CAP;

/* ---- AMD-specific CPUID feature flags ------------------------------------ */

/*
 * Bit flags set in FdoData->DrvCapabilities after CPUID analysis.
 * Mirror the Windows driver's internal capability enum.
 */
#define AMD_CAP_PSS              0x0000000000000001ULL  /* _PSS  supported      */
#define AMD_CAP_XPSS             0x0000000000000002ULL  /* _XPSS supported      */
#define AMD_CAP_CST              0x0000000000000004ULL  /* _CST  supported      */
#define AMD_CAP_PCT              0x0000000000000008ULL  /* _PCT  supported      */
#define AMD_CAP_TSS              0x0000000000000010ULL  /* _TSS  supported      */
#define AMD_CAP_PSD              0x0000000000000020ULL  /* _PSD  supported      */
#define AMD_CAP_TSD              0x0000000000000040ULL  /* _TSD  supported      */
#define AMD_CAP_PPC              0x0000000000000080ULL  /* _PPC  supported      */
#define AMD_CAP_TPC              0x0000000000000100ULL  /* _TPC  supported      */
#define AMD_CAP_OSC              0x0000000000000200ULL  /* _OSC  supported      */
#define AMD_CAP_PDC              0x0000000000000400ULL  /* _PDC  supported      */
#define AMD_CAP_FFH              0x0000000000000800ULL  /* FFH IO/MSR P-state   */
#define AMD_CAP_CPPC             0x0000000000001000ULL  /* HW Collaborative PPC */
#define AMD_CAP_HWFB             0x0000000000002000ULL  /* HW feedback MSR      */
#define AMD_CAP_BOOST            0x0000000000004000ULL  /* Core-boost control   */

/* ---- FDO_DATA (device extension) ----------------------------------------- */

/*
 * FDO_DATA
 *
 * Per-processor device extension allocated by WDF as context data on the
 * WDFDEVICE object.  One FDO_DATA exists per logical processor.
 */
typedef struct _FDO_DATA
{
    /* ---- WDM objects ---------------------------------------------------- */
    PDEVICE_OBJECT          Self;           /* FDO device object              */
    PDEVICE_OBJECT          Pdo;            /* Underlying PDO                 */
    WDFIOTARGET             DefaultTarget;  /* Default WDF IO target (= PDO)  */
    PFILE_OBJECT            FileObject;     /* File object to the ACPI device */

    /* ---- List linkage ---------------------------------------------------- */
    LIST_ENTRY              DeviceLink;     /* Entry in Globals.DeviceHead    */

    /* ---- Processor identity --------------------------------------------- */
    ULONG                   InitialApicId;  /* APIC ID from CPUID leaf 1      */
    ULONG                   AcpiId;         /* _UID / MADT ACPI processor ID  */
    ULONG                   NtNumber;       /* Kernel logical CPU number      */
    ULONG                   LpIndex;        /* HAL logical processor index    */

    /* ---- Power state flags ---------------------------------------------- */
    BOOLEAN                 ResumeFromSleep;/* TRUE after S1-S4 resume        */
    BOOLEAN                 Spare[3];

    /* ---- ACPI state ---------------------------------------------------- */
    BOOLEAN                 AcpiPdoReferenced; /* TRUE if ObRef taken on Pdo */
    UCHAR                   AcpiSpare[3];
    OSC_OUTPUT_BUFFER      *OscOutput;      /* _OSC output (heap-allocated)   */

    /* ---- P-state tables ------------------------------------------------- */
    ACPI_CTRL_STATUS        PCT;            /* Performance control/status reg  */
    ACPI_PSS               *PSS;            /* _PSS table (heap-allocated)     */
    ACPI_PSS               *XPSS;           /* _XPSS table (heap-allocated)    */
    ULONG                   PPC;            /* _PPC – highest allowed P-state  */
    ACPI_XSD               *PSD;            /* _PSD – P-state domain           */

    /* ---- T-state tables ------------------------------------------------- */
    ACPI_CTRL_STATUS        PTC;            /* Throttle control/status reg     */
    ACPI_TSS               *TSS;            /* _TSS table (heap-allocated)     */
    ULONG                   TPC;            /* _TPC – highest allowed T-state  */
    ACPI_XSD               *TSD;            /* _TSD – T-state domain           */

    /* ---- C-state tables ------------------------------------------------- */
    ACPI_CST               *CST;            /* _CST table (heap-allocated)     */

    /* ---- Capability & feature bits -------------------------------------- */
    ULONGLONG               DrvCapabilities;  /* AMD_CAP_* flags (CPUID)      */
    ULONGLONG               PPMFound;         /* methods found during enum    */
    ULONGLONG               PPMEnabled;       /* methods successfully enabled  */
    ULONG                   PPMFeatureFlags;  /* platform/OS flags            */

    /* ---- Function pointers set at device start -------------------------- */
    NTSTATUS (FASTCALL *SetPState)(ULONG Context,
                                   ULONGLONG ControlValue,
                                   ULONGLONG StatusValue);
    NTSTATUS (FASTCALL *SetTState)(ULONG Context,
                                   ULONGLONG ControlValue,
                                   ULONGLONG StatusValue);

    /* ---- Performance capability caps (read from ACPI notifications) ----- */
    ULONG                   PPC_Cap;        /* Current PPC cap value           */
    ULONG                   TPC_Cap;        /* Current TPC cap value           */
    ULONG                   PccCap;         /* PCC cap (if PCC P-states used)  */

    /* ---- PoFx handle ---------------------------------------------------- */
    PVOID                   FxHandle;       /* PoFx device handle (POHANDLE)   */

} FDO_DATA, *PFDO_DATA;

/*
 * WDF context type registration helpers.
 * WDF_DECLARE_CONTEXT_TYPE_WITH_NAME declares the accessor macro
 * AmdPpmGetFdoData(Device) for retrieving the FDO_DATA from a WDFDEVICE.
 */
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, AmdPpmGetFdoData)

/* ---- Driver globals ------------------------------------------------------- */

/*
 * AMDPPM_GLOBALS
 *
 * Single instance of per-driver global state.  Only one AMD PPM driver is
 * loaded, but it handles one WDFDEVICE per logical processor.
 */
typedef struct _AMDPPM_GLOBALS
{
    /* PPM kernel dispatch table (from ZwPowerInformation) */
    PPM_DRIVER_DISPATCH_TABLE   PpmDispatchTable;

    /* WDF synchronisation objects */
    WDFWAITLOCK                 Mutex;          /* Serialises cap changes      */

    /* Device list (one entry per logical CPU) */
    LIST_ENTRY                  DeviceHead;
    ULONG                       ProcessorCount;

    /* Global registration callbacks (set in ProcLibGlobalInit) */
    NTSTATUS (STDCALL *RegisterIdleStates)(PFDO_DATA DevExt);
    NTSTATUS (STDCALL *RegisterPStates)(PFDO_DATA DevExt);
    NTSTATUS (STDCALL *RegisterPerfCap)(PFDO_DATA DevExt);

    /* AMD-specific flags */
    ULONG                       PPMOverrideFlags;
    BOOLEAN                     AssertsDisabled;

} AMDPPM_GLOBALS, *PAMDPPM_GLOBALS;

extern AMDPPM_GLOBALS AmdPpmGlobals;

/* ---- function prototypes -------------------------------------------------- */

/*
 * amdppm.c – driver entry & WDF callbacks
 */

/*
 * Registration helper prototypes.
 * Called from ProcLibDeviceStart and EvtDeviceD0Entry.
 */
NTSTATUS
RegisterKernelIdleStates(
    _In_ PFDO_DATA DevExt);

NTSTATUS
RegisterKernelPerfStates(
    _In_ PFDO_DATA DevExt);

NTSTATUS
RegisterKernelPerfCap(
    _In_ PFDO_DATA DevExt);
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY EvtDeviceD0Entry;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

NTSTATUS
ProcLibGlobalInit(
    _In_ PDRIVER_OBJECT DriverObject);

NTSTATUS
ProcLibDriverCleanup(
    VOID);

NTSTATUS
ProcLibDeviceCreate(
    _In_ PFDO_DATA DevExt);

NTSTATUS
ProcLibDeviceStart(
    _In_ PFDO_DATA DevExt);

/*
 * acpi.c – ACPI method evaluation (IOCTL-based)
 */
NTSTATUS
AcquireAcpiInterfaces(
    _In_ PFDO_DATA DevExt);

VOID
ReleaseAcpiInterfaces(
    _In_ PFDO_DATA DevExt);

NTSTATUS
AcpiEvaluateMethod(
    _In_  PFDO_DATA  DevExt,
    _In_  ULONG      MethodName,
    _In_opt_ PVOID   InputBuffer,
    _Out_ PVOID     *OutputBuffer,
    _Out_ PULONG     OutputBufferReturned);

NTSTATUS
AcpiEval_OSC(
    _In_  PFDO_DATA         DevExt,
    _In_  POSC_INPUT_BUFFER OscInput,
    _In_  USHORT            OscInputSize,
    _Out_ OSC_OUTPUT_BUFFER **OutBuffer);

NTSTATUS
AcpiEval_PDC(
    _In_ PFDO_DATA          DevExt,
    _In_ PPDC_INPUT_BUFFER  InBuffer,
    _In_ USHORT             InBufferSize);

NTSTATUS
AcpiEval_CST(
    _In_  PFDO_DATA   DevExt,
    _Out_ ACPI_CST  **CStates);

NTSTATUS
AcpiEval_PSS(
    _In_  PFDO_DATA   DevExt,
    _Out_ ACPI_PSS  **Address);

NTSTATUS
AcpiEval_XPSS(
    _In_  PFDO_DATA   DevExt,
    _Out_ ACPI_PSS  **Address);

NTSTATUS
AcpiEval_PPC(
    _In_  PFDO_DATA   DevExt,
    _Out_ PULONG      PPC);

NTSTATUS
AcpiEval_TSS(
    _In_  PFDO_DATA   DevExt,
    _Out_ ACPI_TSS  **Address);

NTSTATUS
AcpiEval_TPC(
    _In_  PFDO_DATA   DevExt,
    _Out_ PULONG      TPC);

NTSTATUS
AcpiEval_PCT_PTC(
    _In_  PFDO_DATA          DevExt,
    _In_  ULONG              ObjectName,
    _Out_ ACPI_CTRL_STATUS  *Address);

NTSTATUS
AcpiEval_PSD_TSD(
    _In_  PFDO_DATA   DevExt,
    _In_  ULONG       ObjectName,
    _Out_ ACPI_XSD  **Address);

NTSTATUS
EnumerateControlMethods(
    _In_  PFDO_DATA   DevExt,
    _Out_ PULONG      FeaturesPresent);

/*
 * cpuid.c – AMD CPUID capability detection and MSR control
 */
VOID
GetCpuIdInfo(
    _In_  ULONG     Function,
    _Out_ PULONG    Results);   /* Results[0..3] = EAX,EBX,ECX,EDX */

ULONG
AmdPpmDeviceStart(
    _In_ ULONG PlatformIndex);

NTSTATUS
ValidatePStateCapability(
    _In_  PACPI_CTRL_STATUS  PCT,
    _In_  PACPI_PSS          PSS,
    _Out_ PULONG             ValidationErrors);

NTSTATUS
SetFFHPState(
    _In_ ULONGLONG ControlValue,
    _In_ ULONGLONG StatusValue);

VOID
SetPerformanceBoostMode(
    _In_ ULONG Context,
    _In_ ULONG Policy);

BOOLEAN
IsAmdProcessor(VOID);

/* ---- ACPI method name helpers -------------------------------------------- */

/*
 * Pack a 4-character ACPI method name into a ULONG used by AcpiEvaluateMethod.
 * Names are stored little-endian (e.g. '_CST' → 'TSC_').
 */
#define ACPI_METHOD_NAME_4(a,b,c,d) \
    ((ULONG)(UCHAR)(a) | ((ULONG)(UCHAR)(b) << 8) | \
     ((ULONG)(UCHAR)(c) << 16) | ((ULONG)(UCHAR)(d) << 24))

#define ACPI_METHOD_OSC  ACPI_METHOD_NAME_4('_','O','S','C')
#define ACPI_METHOD_PDC  ACPI_METHOD_NAME_4('_','P','D','C')
#define ACPI_METHOD_CST  ACPI_METHOD_NAME_4('_','C','S','T')
#define ACPI_METHOD_PCT  ACPI_METHOD_NAME_4('_','P','C','T')
#define ACPI_METHOD_PSD  ACPI_METHOD_NAME_4('_','P','S','D')
#define ACPI_METHOD_PSS  ACPI_METHOD_NAME_4('_','P','S','S')
#define ACPI_METHOD_XPSS ACPI_METHOD_NAME_4('X','P','S','S')
#define ACPI_METHOD_PPC  ACPI_METHOD_NAME_4('_','P','P','C')
#define ACPI_METHOD_PTC  ACPI_METHOD_NAME_4('_','P','T','C')
#define ACPI_METHOD_TSD  ACPI_METHOD_NAME_4('_','T','S','D')
#define ACPI_METHOD_TSS  ACPI_METHOD_NAME_4('_','T','S','S')
#define ACPI_METHOD_TPC  ACPI_METHOD_NAME_4('_','T','P','C')

/* EOF */
