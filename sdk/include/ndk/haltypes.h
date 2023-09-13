/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    haltypes.h

Abstract:

    Type definitions for the HAL.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _HALTYPES_H
#define _HALTYPES_H

//
// Dependencies
//
#include <umtypes.h>

typedef struct _LOADER_PARAMETER_BLOCK *PLOADER_PARAMETER_BLOCK;

#ifndef NTOS_MODE_USER

//
// HalShutdownSystem Types
//
typedef enum _FIRMWARE_REENTRY
{
    HalHaltRoutine,
    HalPowerDownRoutine,
    HalRestartRoutine,
    HalRebootRoutine,
    HalInteractiveModeRoutine,
    HalMaximumRoutine
} FIRMWARE_REENTRY, *PFIRMWARE_REENTRY;

//
// HAL Private function Types
//
typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForConfigSpace)(
    _In_ BUS_DATA_TYPE ConfigSpace,
    _In_ ULONG BusNumber
);

typedef
NTSTATUS
(NTAPI *PINSTALL_BUS_HANDLER)(
    _In_ PBUS_HANDLER Bus
);

typedef
NTSTATUS
(NTAPI *pHalRegisterBusHandler)(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ BUS_DATA_TYPE ConfigSpace,
    _In_ ULONG BusNumber,
    _In_ INTERFACE_TYPE ParentInterfaceType,
    _In_ ULONG ParentBusNumber,
    _In_ ULONG ContextSize,
    _In_ PINSTALL_BUS_HANDLER InstallCallback,
    _Out_ PBUS_HANDLER *BusHandler
);

typedef
VOID
(NTAPI *pHalSetWakeEnable)(
    _In_ BOOLEAN Enable
);

typedef
VOID
(NTAPI *pHalSetWakeAlarm)(
    _In_ ULONGLONG AlartTime,
    _In_ ULONGLONG TimeFields
);

typedef
VOID
(NTAPI *pHalLocateHiberRanges)(
    _In_ PVOID MemoryMap
);

typedef
NTSTATUS
(NTAPI *pHalAllocateMapRegisters)(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ ULONG Unknown,
    _In_ ULONG Unknown2,
    PMAP_REGISTER_ENTRY Registers
);

//
// HAL Bus Handler Callback Types
//
typedef
NTSTATUS
(NTAPI *PADJUSTRESOURCELIST)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _Inout_ PIO_RESOURCE_REQUIREMENTS_LIST* pResourceList
);

typedef
NTSTATUS
(NTAPI *PASSIGNSLOTRESOURCES)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ PUNICODE_STRING RegistryPath,
    _In_opt_ PUNICODE_STRING DriverClassName,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SlotNumber,
    _Inout_ PCM_RESOURCE_LIST* AllocatedResources
);

typedef
ULONG
(NTAPI *PGETSETBUSDATA)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ ULONG SlotNumber,
    _In_ PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
);

typedef
ULONG
(NTAPI *PGETINTERRUPTVECTOR)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity
);

typedef
BOOLEAN
(NTAPI *PTRANSLATEBUSADDRESS)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress
);

//Vista+ definitions

/* Move these somewhere else */

typedef struct _KAFFINITY_EX
{
    USHORT Count;
    USHORT Size;
    ULONG Reserved;
    ULONG Bitmap:20;
} KAFFINITY_EX, *PKAFFINITY_EX;

typedef struct _INTERRUPT_REMAPPING_INFO {
    ULONG IrtIndex:30;
    ULONG FlagHalInternal:1;
    ULONG FlagTranslated:1;

    union {
        ULARGE_INTEGER RemappedFormat; // generic form

        struct {
            ULONG   MessageAddressLow;
            USHORT  MessageData;
            USHORT  Reserved;
        } Msi;
    } u;
} INTERRUPT_REMAPPING_INFO, *PINTERRUPT_REMAPPING_INFO;

typedef enum _PCI_BUSMASTER_RID_TYPE {
    BusmasterRidFromDeviceRid,
    BusmasterRidFromBridgeRid,
    BusmasterRidFromMultipleBridges
} PCI_BUSMASTER_RID_TYPE, *PPCI_BUSMASTER_RID_TYPE;

typedef struct _PCI_BUSMASTER_DESCRIPTOR {

    PCI_BUSMASTER_RID_TYPE Type;

    ULONG Segment;

    union {

        struct {
            UCHAR   Bus;
            UCHAR   Device;
            UCHAR   Function;
            UCHAR   Reserved;
        } DeviceRid;

        struct {
            UCHAR   Bus;
            UCHAR   Device;
            UCHAR   Function;
            UCHAR   Reserved;
        } BridgeRid;

        struct {
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
        } MultipleBridges;

    } DUMMYSTRUCTNAME;
} PCI_BUSMASTER_DESCRIPTOR, *PPCI_BUSMASTER_DESCRIPTOR;

typedef enum {
    InterruptTypeControllerInput,
    InterruptTypeXapicMessage,
    InterruptTypeHypertransport,
    InterruptTypeMessageRequest
} INTERRUPT_CONNECTION_TYPE;

typedef struct _INTERRUPT_HT_INTR_INFO {

    union {
        struct {
            ULONG Mask:1;             // bit 0
            ULONG Polarity:1;         // bit 1
            ULONG MessageType:3;      // bits [4:2]
            ULONG RequestEOI:1;       // bit 5
            ULONG DestinationMode:1;  // bit 6
            ULONG MessageType3:1;     // bit 7
            ULONG Destination:8;      // bits [15:8]
            ULONG Vector:8;           // bits [23:16]
            ULONG ExtendedAddress:8;  // bits [31:24] -- always set to 0xF8
        } bits;

        ULONG AsULONG;

    } LowPart;

    union {
        struct {
            ULONG ExtendedDestination:24;
            ULONG Reserved:6;
            ULONG PassPW:1;
            ULONG WaitingForEOI:1;
        } bits;

        ULONG AsULONG;

    } HighPart;

} INTERRUPT_HT_INTR_INFO, *PINTERRUPT_HT_INTR_INFO;

typedef struct _INTERRUPT_VECTOR_DATA {

    INTERRUPT_CONNECTION_TYPE Type;
    ULONG Vector;
    KIRQL Irql;

    //
    // N.B. If Polarity is InterruptActiveBothTriggerLow or
    //      InterruptActiveBothTriggerHigh, Mode will be Latched.
    //
    //      If the interrupt controller will be programmed to emulate
    //      ActiveBoth using levels, the KINTERRUPT object used in interrupt
    //      dispatching must be passed LevelSensitive instead of this Mode.
    //

    KINTERRUPT_POLARITY Polarity;
    KINTERRUPT_MODE Mode;
    GROUP_AFFINITY TargetProcessors;
    INTERRUPT_REMAPPING_INFO IntRemapInfo;

    struct {
        ULONG Gsiv;
        ULONG WakeInterrupt:1;
        ULONG ReservedFlags:31;
    } ControllerInput;

    ULONGLONG HvDeviceId;

    union {
        struct {
            PHYSICAL_ADDRESS Address;
            ULONG DataPayload;
        } XapicMessage;

        struct {
            INTERRUPT_HT_INTR_INFO IntrInfo;
        } Hypertransport;

        struct {
            PHYSICAL_ADDRESS Address;
            ULONG DataPayload;
        } GenericMessage;

        struct {
            HAL_APIC_DESTINATION_MODE DestinationMode;
        } MessageRequest;
    };
} INTERRUPT_VECTOR_DATA, *PINTERRUPT_VECTOR_DATA;

typedef
VOID
(*PHAL_LOG_ROUTINE) (
    _In_ ULONG EventId,
    _In_ PVOID Buffer,
    _In_ ULONG Size
    );

typedef struct _HAL_LOG_REGISTER_CONTEXT {
    PHAL_LOG_ROUTINE LogRoutine;
    ULONG Flag;
} HAL_LOG_REGISTER_CONTEXT, *PHAL_LOG_REGISTER_CONTEXT;

/******************************/

typedef
ULONG
(NTAPI *pHalGetInterruptVector)(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity
);

typedef
NTSTATUS
(NTAPI *pHalGetVectorInput)(
    _In_ ULONG Vector,
    _In_ PGROUP_AFFINITY Affinity,
    _Out_ PULONG Input,
    _Out_ PKINTERRUPT_POLARITY Polarity,
    _Out_ PINTERRUPT_REMAPPING_INFO IntRemapInfo
);

typedef
NTSTATUS
(NTAPI *pHalLoadMicrocode)(
    _In_ PVOID ImageHandle
);

typedef
NTSTATUS
(NTAPI *pHalUnloadMicrocode)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalPostMicrocodeUpdate)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalAllocateMessageTarget)(
    _In_ PDEVICE_OBJECT Owner,
    _In_ PGROUP_AFFINITY ProcessorSet,
    _In_ ULONG NumberOfIdtEntries,
    _In_ KINTERRUPT_MODE Mode,
    _In_ BOOLEAN ShareVector,
    _Out_ PULONG Vector,
    _Out_ PKIRQL Irql,
    _Out_ PULONG IdtEntry
);

typedef
VOID
(NTAPI *pHalFreeMessageTarget)(
    _In_ PDEVICE_OBJECT Owner,
    _In_ ULONG Vector,
    _In_ PGROUP_AFFINITY ProcessorSet
);

//
// Dynamic partitioning hot-replace HAL interface function.
//

#define HAL_DP_REPLACE_PHASE_QUIESCE    0
#define HAL_DP_REPLACE_PHASE_SWAP       1
#define HAL_DP_REPLACE_PHASE_WAKE       2
#define HAL_DP_REPLACE_PHASE_CANCEL     3

typedef struct _PNP_REPLACE_PROCESSOR_LIST *PPNP_REPLACE_PROCESSOR_LIST;

#define HAL_DP_REPLACE_PROCESSOR_ID_RECONFIGURE     0x01
#define HAL_DP_REPLACE_HARDWARE_QUIESCE             0x02

typedef struct _HAL_DP_REPLACE_PARAMETERS {
    ULONG Flags;
    PPNP_REPLACE_PROCESSOR_LIST TargetProcessors;
    PPNP_REPLACE_PROCESSOR_LIST SpareProcessors;
} HAL_DP_REPLACE_PARAMETERS, *PHAL_DP_REPLACE_PARAMETERS;

typedef
NTSTATUS
(NTAPI *pHalDpReplaceBegin)(
    _In_ PHAL_DP_REPLACE_PARAMETERS Parameters,
    _Outptr_ PVOID *ReplaceContext
);

typedef
VOID
(NTAPI *pHalDpReplaceTarget)(
    _In_ PVOID ReplaceContext
);

typedef
NTSTATUS
(NTAPI *pHalDpReplaceControl)(
    _In_ ULONG Phase,
    _In_ PVOID ReplaceContext
);

typedef
VOID
(NTAPI *pHalDpReplaceEnd)(
    _In_ PVOID ReplaceContext
);

typedef
NTSTATUS
(NTAPI *pHalDpMaskLevelTriggeredInterrupts)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalDpUnmaskLevelTriggeredInterrupts)(
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalDpGetInterruptReplayState)(
    _In_ PVOID ReplaceContext,
    _Outptr_ PVOID *Buffer
);

typedef
NTSTATUS
(NTAPI *pHalDpReplayInterrupts)(
    _In_ PVOID InterruptState
);

#define HAL_PREPARE_NMI_IN_PROGRESS     0x00000001UL

typedef
VOID
(NTAPI *pHalPrepareForBugcheck)(
    _In_ ULONG Flags
);

typedef
BOOLEAN
(NTAPI *pHalQueryWakeTime)(
    _Out_ PULONGLONG WakeTime,
    _Out_opt_ PULONGLONG TscOffset
);

typedef
BOOLEAN
(NTAPI *pHalQueryIoPortAccessSupported)(
    VOID
);

typedef
VOID
(NTAPI *pHalReportIdleStateUsage)(
    _In_ UCHAR DeepestHardwareIdleState,
    _In_ PKAFFINITY_EX TargetSet
);

typedef
VOID
(NTAPI *pHalTscSynchronization)(
    _In_ BOOLEAN ForcedSynchronization,
    _In_opt_ PULONG TargetProcessor
);

typedef struct _WHEA_ERROR_RECORD_SECTION_DESCRIPTOR
    *PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR;

typedef struct _WHEA_PROCESSOR_GENERIC_ERROR_SECTION
    *PWHEA_PROCESSOR_GENERIC_ERROR_SECTION;

typedef
NTSTATUS
(NTAPI *pHalWheaInitProcessorGenericSection)(
    _Out_ PWHEA_ERROR_RECORD_SECTION_DESCRIPTOR Descriptor,
    _Out_ PWHEA_PROCESSOR_GENERIC_ERROR_SECTION Section
);

typedef
VOID
(NTAPI *pHalStopLegacyUsbInterrupts)(
    _In_ SYSTEM_POWER_STATE LastSystemState
);

typedef
NTSTATUS
(NTAPI *pHalReadWheaPhysicalMemory)(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG Length,
    _Out_writes_bytes_(Length) PVOID Data
);

typedef
NTSTATUS
(NTAPI *pHalWriteWheaPhysicalMemory)(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PVOID Data
);


#if !defined(_ARM64_) && !defined(_ARM_)

#define _HAL_ENLIGHTENMENT_INFORMATION _HAL_INTEL_ENLIGHTENMENT_INFORMATION
#define  HAL_ENLIGHTENMENT_INFORMATION  HAL_INTEL_ENLIGHTENMENT_INFORMATION
#define PHAL_ENLIGHTENMENT_INFORMATION PHAL_INTEL_ENLIGHTENMENT_INFORMATION

#else

#define _HAL_ENLIGHTENMENT_INFORMATION _HAL_ARM_ENLIGHTENMENT_INFORMATION
#define  HAL_ENLIGHTENMENT_INFORMATION  HAL_ARM_ENLIGHTENMENT_INFORMATION
#define PHAL_ENLIGHTENMENT_INFORMATION PHAL_ARM_ENLIGHTENMENT_INFORMATION

#endif

typedef struct _HAL_ENLIGHTENMENT_INFORMATION
    HAL_ENLIGHTENMENT_INFORMATION, *PHAL_ENLIGHTENMENT_INFORMATION;

typedef
VOID
(NTAPI *pHalGetEnlightenmentInformation)(
    PHAL_ENLIGHTENMENT_INFORMATION EnlightenmentInformation
);

typedef
PVOID
(NTAPI *pHalAllocateEarlyPages)(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ ULONG PageCount,
    _Out_ PULONG64 PhysicalAddress,
    _In_ ULONG Protection
);

typedef
PVOID
(NTAPI *pHalMapEarlyPages)(
    _In_ ULONG64 PhysicalAddress,
    _In_ ULONG PageCount,
    _In_ ULONG Protection
);

//
// Flags to prepare processor for idle.
//

#define HAL_PREPARE_FOR_IDLE_INTERRUPTIBLE   (1 << 0)

typedef
NTSTATUS
(NTAPI *pHalPrepareProcessorForIdle) (
    _In_ ULONG Flags
);

typedef
VOID
(NTAPI *pHalResumeProcessorFromIdle) (
   VOID
   );

typedef
VOID
(NTAPI *pHalNotifyProcessorFreeze) (
    _In_ BOOLEAN Freezing,
    _In_ BOOLEAN ThawingToSpinLoop
);

typedef
VOID
(NTAPI *pHalRegisterLogRoutine) (
    _In_ PHAL_LOG_REGISTER_CONTEXT Context
);

//
// Modes of clock timer operation.
//

typedef enum _HAL_CLOCK_TIMER_MODE {
    HalClockTimerModePeriodic,
    HalClockTimerModeOneShot,
    HalClockTimerModeMax
} HAL_CLOCK_TIMER_MODE, *PHAL_CLOCK_TIMER_MODE;

//
// Clock timer configuration.
//

typedef struct _HAL_CLOCK_TIMER_CONFIGURATION {
    union {
        BOOLEAN Flags;
        struct {
            BOOLEAN AlwaysOnTimer: 1;
            BOOLEAN HighLatency: 1;
            BOOLEAN PerCpuTimer: 1;
            BOOLEAN DynamicTickSupported: 1;
        };
    };

    ULONG KnownType;
    ULONG Capabilities;
    ULONG64 MaxIncrement;
    ULONG MinIncrement;
} HAL_CLOCK_TIMER_CONFIGURATION, *PHAL_CLOCK_TIMER_CONFIGURATION;

typedef
VOID
(NTAPI *pHalGetClockConfiguration) (
    _Out_ PHAL_CLOCK_TIMER_CONFIGURATION Configuration
);

typedef
VOID
(NTAPI *pHalClockTimerActivate) (
    _In_ BOOLEAN ClockOwner
);

typedef
VOID
(NTAPI *pHalClockTimerInitialize) (
    VOID
);

typedef
VOID
 (NTAPI *pHalClockTimerStop) (
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalClockTimerArm) (
    _In_ HAL_CLOCK_TIMER_MODE Mode,
    _In_ ULONG64 RequestedInteval,
    _Out_ PULONG64 ActualInterval
);

typedef
BOOLEAN
(NTAPI *pHalTimerOnlyClockInterruptPending) (
    VOID
);

typedef struct _HAL_IOMMU_DISPATCH HAL_IOMMU_DISPATCH, *PHAL_IOMMU_DISPATCH;

typedef
VOID
(NTAPI *pHalIommuRegisterDispatchTable) (
    _Inout_ PHAL_IOMMU_DISPATCH DispatchTable
);

//
// Secondary interrupt services related routines.
//

//
// The difference between pHalVectorToIDTEntry and pHalVectorToIDTEntryEx is
// the return type. The old routine assumes that IDT entries can only range from
// 0 - 0xFF. The new routine supports IDT ranges beyond (0x0 - 0xFF). This is
// possible for entries within the secondary IDT, the range for which starts
// after the primary IDT range.
//

typedef
ULONG
(NTAPI *pHalVectorToIDTEntryEx) (
    ULONG Vector
);

typedef
NTSTATUS
(NTAPI *pHalSecondaryInterruptQueryPrimaryInformation) (
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _Out_ PULONG PrimaryGsiv
);

typedef
BOOLEAN
(NTAPI *pHalIsInterruptTypeSecondary) (
    _In_ ULONG Type,                        // INTERRUPT_CONNECTION_TYPE
    _In_ ULONG InputGsiv
);

// begin_wdm

#define HAL_MASK_UNMASK_FLAGS_NONE (0x0)
#define HAL_MASK_UNMASK_FLAGS_SERVICING_DEFERRED (0x1)
#define HAL_MASK_UNMASK_FLAGS_SERVICING_COMPLETE (0x2)

// end_wdm

typedef
NTSTATUS
(NTAPI *pHalMaskInterrupt) (
    _In_ ULONG InputGsiv,
    _In_ ULONG Flags
);

typedef
NTSTATUS
(NTAPI *pHalUnmaskInterrupt) (
    _In_ ULONG InputGsiv,
    _In_ ULONG Flags
);

typedef
NTSTATUS
(NTAPI *pHalAllocateGsivForSecondaryInterrupt) (
    _In_reads_bytes_(OwnerNameLength) PCCHAR OwnerName,
    _In_ USHORT OwnerNameLength,
    _Out_ PULONG Gsiv
);

typedef
NTSTATUS
(NTAPI *pHalInterruptVectorDataToGsiv) (
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _Out_ PULONG Gsiv
);

typedef
NTSTATUS
(NTAPI *pHalAddInterruptRemapping) (
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_ PPCI_BUSMASTER_DESCRIPTOR BusMasterDescriptor,
    _In_range_(0, 3) UCHAR PhantomBits,
    _Inout_updates_(VectorCount) PINTERRUPT_VECTOR_DATA VectorData,
    ULONG VectorCount
);

typedef
VOID
(NTAPI *pHalRemoveInterruptRemapping) (
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_ PPCI_BUSMASTER_DESCRIPTOR BusMasterDescriptor,
    _In_range_(0, 3) UCHAR PhantomBits,
    _Inout_updates_(VectorCount) PINTERRUPT_VECTOR_DATA VectorData,
    ULONG VectorCount
);

typedef
VOID
(NTAPI *pHalSaveAndDisableHvEnlightenment) (
    VOID
);

typedef
VOID
(NTAPI *pHalRestoreHvEnlightenment) (
    VOID
);

//
// Flush external cache if present
//

typedef
VOID
(NTAPI *pHalFlushIoBuffersExternalCache) (
    IN PMDL  Mdl,
    IN BOOLEAN  ReadOperation
);

typedef
VOID
(NTAPI *pHalFlushIoRectangleExternalCache) (
    IN PMDL Mdl,
    IN ULONG StartOffset,
    IN ULONG Width,
    IN ULONG Height,
    IN ULONG Stride,
    IN BOOLEAN ReadOperation
);

typedef
VOID
(NTAPI *pHalFlushExternalCache) (
    IN BOOLEAN Invalidate
);

typedef
VOID
(NTAPI *pHalFlushAndInvalidatePageExternalCache) (
    IN PHYSICAL_ADDRESS PhysicalAddress
);

typedef
NTSTATUS
(NTAPI *pHalPciEarlyRestore) (
    _In_ SYSTEM_POWER_STATE SleepState
);

typedef
VOID
(NTAPI *pHalPciLateRestore) (
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalGetProcessorId) (
    _In_ ULONG ProcessorIndex,
    _Out_ ULONG *Identifier
);

//
// Pmc counter profiling interfaces
//

typedef struct _HAL_PMC_COUNTERS *PMC_HANDLE;

typedef
NTSTATUS
(NTAPI *pHalAllocatePmcCounterSet) (
    _In_ ULONG ProcessorIndex,
    _In_reads_(SourceCount) KPROFILE_SOURCE *SourceList,
    _In_ ULONG SourceCount,
    _Out_ PMC_HANDLE *Handle
);

typedef
VOID
(NTAPI *pHalFreePmcCounterSet) (
    _In_ PMC_HANDLE Handle
);

typedef
VOID
(NTAPI *pHalCollectPmcCounters) (
    _In_ PMC_HANDLE Handle,
    _Out_ PULONG64 Data
);

typedef
ULONGLONG
(NTAPI *pHalTimerQueryCycleCounter) (
    _Out_opt_ PULONGLONG CycleCounterFrequency
);

// begin_wdm

//
// Processor driver halt routine.
//

typedef
NTSTATUS
PROCESSOR_HALT_ROUTINE (
    _Inout_opt_ PVOID Context
);

typedef PROCESSOR_HALT_ROUTINE *PPROCESSOR_HALT_ROUTINE;

// end_wdm

typedef
NTSTATUS
(NTAPI *pHalProcessorHalt) (
    _In_ ULONG Flags,
    _Inout_opt_ PVOID Context,
    _In_ PPROCESSOR_HALT_ROUTINE Halt
);

typedef
VOID
(NTAPI *pHalPciMarkHiberPhase) (
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalQueryProcessorRestartEntryPoint) (
    _Out_ PPHYSICAL_ADDRESS EntryPoint
);

typedef
NTSTATUS
(NTAPI *pHalRequestInterrupt) (
    _In_ ULONG Gsiv
);

typedef
VOID
(NTAPI *pHalPowerEarlyRestore) (
    ULONG Phase
);

typedef
NTSTATUS
(NTAPI *pHalUpdateCapsule) (
    _In_ PVOID CapsuleHeaderArray,
    _In_ ULONG CapsuleCount,
    _In_opt_ PHYSICAL_ADDRESS ScatterGatherList
);

typedef
NTSTATUS
(NTAPI *pHalQueryCapsuleCapabilities) (
    _In_ PVOID CapsuleHeaderArray,
    _In_ ULONG CapsuleCount,
    _Out_ PULONGLONG MaximumCapsuleSize,
    _Out_ PULONG ResetType
);

typedef
BOOLEAN
(NTAPI *pHalPciMultiStageResumeCapable) (
    VOID
);

typedef
VOID
(NTAPI *pHalDmaFreeCrashDumpRegisters) (
    _In_ ULONG Phase
);

typedef
BOOLEAN
(NTAPI *pHalAcpiAoacCapable) (
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalInterruptSetDestination) (
    __in ULONG Gsiv,
    __in PINTERRUPT_VECTOR_DATA VectorData,
    __in PGROUP_AFFINITY TargetProcessors
);

//
// N.B. This must match its PO counterpart (PEP_UNMASKED_INTERRUPT_INFORMATION).
//

#define HAL_UNMASKED_INTERRUPT_INFORMATION_V1 0x00000001
#define HAL_UNMASKED_INTERRUPT_INFORMATION_MINIMUM_SIZE \
    RTL_SIZEOF_THROUGH_FIELD(HAL_UNMASKED_INTERRUPT_INFORMATION, DeviceHandle)

typedef union _HAL_UNMASKED_INTERRUPT_FLAGS {
    struct {
        USHORT SecondaryInterrupt: 1;
        USHORT Reserved: 15;
    };

    USHORT AsUSHORT;

} HAL_UNMASKED_INTERRUPT_FLAGS, *PHAL_UNMASKED_INTERRUPT_FLAGS;

typedef struct _HAL_UNMASKED_INTERRUPT_INFORMATION {
    USHORT Version;
    USHORT Size;
    HAL_UNMASKED_INTERRUPT_FLAGS Flags;
    KINTERRUPT_MODE Mode;
    KINTERRUPT_POLARITY Polarity;
    ULONG Gsiv;
    USHORT PinNumber;
    PVOID DeviceHandle;
} HAL_UNMASKED_INTERRUPT_INFORMATION, *PHAL_UNMASKED_INTERRUPT_INFORMATION;

typedef
BOOLEAN
(HAL_ENUMERATE_INTERRUPT_SOURCE_CALLBACK)(
    _In_ PVOID Context,
    _In_ PHAL_UNMASKED_INTERRUPT_INFORMATION InterruptInformation
);

typedef HAL_ENUMERATE_INTERRUPT_SOURCE_CALLBACK
    *PHAL_ENUMERATE_INTERRUPT_SOURCE_CALLBACK;

typedef
NTSTATUS
(NTAPI *pHalEnumerateUnmaskedInterrupts) (
    _In_ PHAL_ENUMERATE_INTERRUPT_SOURCE_CALLBACK Callback,
    _In_ PVOID Context,
    _Out_ PHAL_UNMASKED_INTERRUPT_INFORMATION InterruptInformation
);

typedef
PVOID
(NTAPI *pHalAcpiGetMultiNode) (
    VOID
);

typedef
void
(HALREBOOTHANDLER)(
    _In_ ULONG ProcessorNumber,
    _Inout_opt_ volatile LONG* ProcessorsStarted
);

typedef HALREBOOTHANDLER *PHALREBOOTHANDLER;

typedef
PHALREBOOTHANDLER
(NTAPI *pHalPowerSetRebootHandler) (
    _In_opt_ PHALREBOOTHANDLER NewHandler
);

typedef
NTSTATUS
(NTAPI *pHalTimerWatchdogStart) (
    VOID
);

typedef
VOID
(NTAPI *pHalTimerWatchdogResetCountdown) (
    _In_ LOGICAL SetWakeTimer
);

typedef
NTSTATUS
(NTAPI *pHalTimerWatchdogStop) (
    VOID
);

typedef
BOOLEAN
(NTAPI *pHalTimerWatchdogGeneratedLastReset) (
    VOID
);

typedef
NTSTATUS
(NTAPI *pHalTimerWatchdogTriggerSystemReset) (
    BOOLEAN ResetViaClockInterrupt
);

//
// HAL Private dispatch Table
//
// See Version table at:
// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/hal/hal_private_dispatch.htm
//
#define HAL_PRIVATE_DISPATCH_VERSION        23

typedef struct _HAL_PRIVATE_DISPATCH
{
    ULONG Version;
    pHalHandlerForBus HalHandlerForBus;
    pHalHandlerForConfigSpace HalHandlerForConfigSpace;
    pHalLocateHiberRanges HalLocateHiberRanges;
    pHalRegisterBusHandler HalRegisterBusHandler;
    pHalSetWakeEnable HalSetWakeEnable;
    pHalSetWakeAlarm HalSetWakeAlarm;
    pHalTranslateBusAddress HalPciTranslateBusAddress;
    pHalAssignSlotResources HalPciAssignSlotResources;
    pHalHaltSystem HalHaltSystem;
    pHalFindBusAddressTranslation HalFindBusAddressTranslation;
    pHalResetDisplay HalResetDisplay;
    pHalAllocateMapRegisters HalAllocateMapRegisters;
    pKdSetupPciDeviceForDebugging KdSetupPciDeviceForDebugging;
    pKdReleasePciDeviceForDebugging KdReleasePciDeviceforDebugging;
    pKdGetAcpiTablePhase0 KdGetAcpiTablePhase0;
    pKdCheckPowerButton KdCheckPowerButton;
    pHalVectorToIDTEntry HalVectorToIDTEntry;
    pKdMapPhysicalMemory64 KdMapPhysicalMemory64;
    pKdUnmapVirtualAddress KdUnmapVirtualAddress;
    pKdGetPciDataByOffset KdGetPciDataByOffset;
    pKdSetPciDataByOffset KdSetPciDataByOffset;
    pHalGetInterruptVector HalGetInterruptVectorOverride;
    pHalGetVectorInput HalGetVectorInputOverride;
    pHalLoadMicrocode HalLoadMicrocode;
    pHalUnloadMicrocode HalUnloadMicrocode;
    pHalPostMicrocodeUpdate HalPostMicrocodeUpdate;
    pHalAllocateMessageTarget HalAllocateMessageTargetOverride;
    pHalFreeMessageTarget HalFreeMessageTargetOverride;
    pHalDpReplaceBegin HalDpReplaceBegin;
    pHalDpReplaceTarget HalDpReplaceTarget;
    pHalDpReplaceControl HalDpReplaceControl;
    pHalDpReplaceEnd HalDpReplaceEnd;
    pHalPrepareForBugcheck HalPrepareForBugcheck;
    pHalQueryWakeTime HalQueryWakeTime;
    pHalReportIdleStateUsage HalReportIdleStateUsage;
    pHalTscSynchronization HalTscSynchronization;
    pHalWheaInitProcessorGenericSection HalWheaInitProcessorGenericSection;
    pHalStopLegacyUsbInterrupts HalStopLegacyUsbInterrupts;
    pHalReadWheaPhysicalMemory HalReadWheaPhysicalMemory;
    pHalWriteWheaPhysicalMemory HalWriteWheaPhysicalMemory;
    pHalDpMaskLevelTriggeredInterrupts HalDpMaskLevelTriggeredInterrupts;
    pHalDpUnmaskLevelTriggeredInterrupts HalDpUnmaskLevelTriggeredInterrupts;
    pHalDpGetInterruptReplayState HalDpGetInterruptReplayState;
    pHalDpReplayInterrupts HalDpReplayInterrupts;
    pHalQueryIoPortAccessSupported HalQueryIoPortAccessSupported;
    pKdSetupIntegratedDeviceForDebugging KdSetupIntegratedDeviceForDebugging;
    pKdReleaseIntegratedDeviceForDebugging KdReleaseIntegratedDeviceForDebugging;
    pHalGetEnlightenmentInformation HalGetEnlightenmentInformation;
    pHalAllocateEarlyPages HalAllocateEarlyPages;
    pHalMapEarlyPages HalMapEarlyPages;
    PVOID Dummy1;
    PVOID Dummy2;
    pHalNotifyProcessorFreeze HalNotifyProcessorFreeze;
    pHalPrepareProcessorForIdle HalPrepareProcessorForIdle;
    pHalRegisterLogRoutine HalRegisterLogRoutine;
    pHalResumeProcessorFromIdle HalResumeProcessorFromIdle;
    PVOID Dummy;
    pHalVectorToIDTEntryEx HalVectorToIDTEntryEx;
    pHalSecondaryInterruptQueryPrimaryInformation HalSecondaryInterruptQueryPrimaryInformation;
    pHalMaskInterrupt HalMaskInterrupt;
    pHalUnmaskInterrupt HalUnmaskInterrupt;
    pHalIsInterruptTypeSecondary HalIsInterruptTypeSecondary;
    pHalAllocateGsivForSecondaryInterrupt HalAllocateGsivForSecondaryInterrupt;
    pHalAddInterruptRemapping HalAddInterruptRemapping;
    pHalRemoveInterruptRemapping HalRemoveInterruptRemapping;
    pHalSaveAndDisableHvEnlightenment HalSaveAndDisableHvEnlightenment;
    pHalRestoreHvEnlightenment HalRestoreHvEnlightenment;
    pHalFlushIoBuffersExternalCache HalFlushIoBuffersExternalCache;
    pHalFlushExternalCache HalFlushExternalCache;
    pHalPciEarlyRestore HalPciEarlyRestore;
    pHalGetProcessorId HalGetProcessorId;
    pHalAllocatePmcCounterSet HalAllocatePmcCounterSet;
    pHalCollectPmcCounters HalCollectPmcCounters;
    pHalFreePmcCounterSet HalFreePmcCounterSet;
    pHalProcessorHalt HalProcessorHalt;
    pHalTimerQueryCycleCounter HalTimerQueryCycleCounter;
    PVOID Dummy3;
    pHalPciMarkHiberPhase HalPciMarkHiberPhase;
    pHalQueryProcessorRestartEntryPoint HalQueryProcessorRestartEntryPoint;
    pHalRequestInterrupt HalRequestInterrupt;
    pHalEnumerateUnmaskedInterrupts HalEnumerateUnmaskedInterrupts;
    pHalFlushAndInvalidatePageExternalCache HalFlushAndInvalidatePageExternalCache;
    pKdEnumerateDebuggingDevices KdEnumerateDebuggingDevices;
    pHalFlushIoRectangleExternalCache HalFlushIoRectangleExternalCache;
    pHalPowerEarlyRestore HalPowerEarlyRestore;
    pHalQueryCapsuleCapabilities HalQueryCapsuleCapabilities;
    pHalUpdateCapsule HalUpdateCapsule;
    pHalPciMultiStageResumeCapable HalPciMultiStageResumeCapable;
    pHalDmaFreeCrashDumpRegisters HalDmaFreeCrashDumpRegisters;
    pHalAcpiAoacCapable HalAcpiAoacCapable;
    pHalInterruptSetDestination HalInterruptSetDestination;
    pHalGetClockConfiguration HalGetClockConfiguration;
    pHalClockTimerActivate HalClockTimerActivate;
    pHalClockTimerInitialize HalClockTimerInitialize;
    pHalClockTimerStop HalClockTimerStop;
    pHalClockTimerArm HalClockTimerArm;
    pHalTimerOnlyClockInterruptPending HalTimerOnlyClockInterruptPending;
    pHalAcpiGetMultiNode HalAcpiGetMultiNode;
    pHalPowerSetRebootHandler HalPowerSetRebootHandler;
    pHalIommuRegisterDispatchTable HalIommuRegisterDispatchTable;
    pHalTimerWatchdogStart HalTimerWatchdogStart;
    pHalTimerWatchdogResetCountdown HalTimerWatchdogResetCountdown;
    pHalTimerWatchdogStop HalTimerWatchdogStop;
    pHalTimerWatchdogGeneratedLastReset HalTimerWatchdogGeneratedLastReset;
    pHalTimerWatchdogTriggerSystemReset HalTimerWatchdogTriggerSystemReset;
} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;

//
// HAL Supported Range
//
#define HAL_SUPPORTED_RANGE_VERSION 1
typedef struct _SUPPORTED_RANGE
{
    struct _SUPPORTED_RANGE *Next;
    ULONG SystemAddressSpace;
    LONGLONG SystemBase;
    LONGLONG Base;
    LONGLONG Limit;
} SUPPORTED_RANGE, *PSUPPORTED_RANGE;

typedef struct _SUPPORTED_RANGES
{
    USHORT Version;
    BOOLEAN Sorted;
    UCHAR Reserved;
    ULONG NoIO;
    SUPPORTED_RANGE IO;
    ULONG NoMemory;
    SUPPORTED_RANGE Memory;
    ULONG NoPrefetchMemory;
    SUPPORTED_RANGE PrefetchMemory;
    ULONG NoDma;
    SUPPORTED_RANGE Dma;
} SUPPORTED_RANGES, *PSUPPORTED_RANGES;

//
// HAL Bus Handler
//
#define HAL_BUS_HANDLER_VERSION 1
typedef struct _BUS_HANDLER
{
    ULONG Version;
    INTERFACE_TYPE InterfaceType;
    BUS_DATA_TYPE ConfigurationType;
    ULONG BusNumber;
    PDEVICE_OBJECT DeviceObject;
    struct _BUS_HANDLER *ParentHandler;
    PVOID BusData;
    ULONG DeviceControlExtensionSize;
    PSUPPORTED_RANGES BusAddresses;
    ULONG Reserved[4];
    PGETSETBUSDATA GetBusData;
    PGETSETBUSDATA SetBusData;
    PADJUSTRESOURCELIST AdjustResourceList;
    PASSIGNSLOTRESOURCES AssignSlotResources;
    PGETINTERRUPTVECTOR GetInterruptVector;
    PTRANSLATEBUSADDRESS TranslateBusAddress;
    PVOID Spare1;
    PVOID Spare2;
    PVOID Spare3;
    PVOID Spare4;
    PVOID Spare5;
    PVOID Spare6;
    PVOID Spare7;
    PVOID Spare8;
} BUS_HANDLER;

//
// HAL Chip Hacks
//
#define HAL_PCI_CHIP_HACK_BROKEN_ACPI_TIMER        0x01
#define HAL_PCI_CHIP_HACK_DISABLE_HIBERNATE        0x02
#define HAL_PCI_CHIP_HACK_DISABLE_ACPI_IRQ_ROUTING 0x04
#define HAL_PCI_CHIP_HACK_USB_SMI_DISABLE          0x08

//
// Kernel Exports
//
#if !defined(_NTSYSTEM_) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_))
extern NTSYSAPI PHAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH ((PHAL_PRIVATE_DISPATCH)&HalPrivateDispatchTable)
#else
extern NTSYSAPI HAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH (&HalPrivateDispatchTable)
#endif

//
// HAL Exports
//
extern NTHALAPI PUCHAR KdComPortInUse;

//
// HAL Constants
//
#define HAL_IRQ_TRANSLATOR_VERSION 0x0

//
// BIOS call structure
//
typedef struct _X86_BIOS_REGISTERS
{
    ULONG Eax;
    ULONG Ecx;
    ULONG Edx;
    ULONG Ebx;
    ULONG Ebp;
    ULONG Esi;
    ULONG Edi;
    USHORT SegDs;
    USHORT SegEs;
} X86_BIOS_REGISTERS, *PX86_BIOS_REGISTERS;

#endif
#endif



