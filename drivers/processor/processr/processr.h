/*
 * PROJECT:        ReactOS Generic CPU Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/processor/processr/processr.h
 * PURPOSE:        Common header file
 * PROGRAMMERS:    Eric Kohl <eric.kohl@reactos.org>
 */

#ifndef _PROCESSR_PCH_
#define _PROCESSR_PCH_

#include <ntddk.h>
#include <ndk/processorperfstates.h>

/*
 * Kernel PPM dispatch (must match ntoskrnl ppm.h / amdppm).
 */
#define PROCESSR_PPM_DRIVER_INTERFACE_VERSION    43

typedef struct _PROCESSR_PPM_DISPATCH_TABLE
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
    VOID     (NTAPI *RemoveVetoBias)(VOID);
    VOID     (NTAPI *UpdateProcessorIdleVeto)(PVOID ProcessorIdleVeto);
    VOID     (NTAPI *UpdatePlatformIdleVeto)(PVOID PlatformIdleVeto);
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
} PROCESSR_PPM_DISPATCH_TABLE, *PPROCESSR_PPM_DISPATCH_TABLE;

C_ASSERT(sizeof(PROCESSR_PPM_DISPATCH_TABLE) == 0x60);

/*
 * ACPI _PSS / _PCT (same layout as ACPI 6.x / amdppm).
 */
typedef struct _PROCESSR_GEN_ADDR
{
    UCHAR AddressSpaceID;
    UCHAR RegisterBitWidth;
    UCHAR RegisterBitOffset;
    UCHAR AccessSize;
    LARGE_INTEGER Address;
} PROCESSR_GEN_ADDR, *PPROCESSR_GEN_ADDR;

typedef struct _PROCESSR_ACPI_CTRL_STATUS
{
    PROCESSR_GEN_ADDR Control;
    PROCESSR_GEN_ADDR Status;
} PROCESSR_ACPI_CTRL_STATUS, *PPROCESSR_ACPI_CTRL_STATUS;

typedef struct _PROCESSR_PSS_STATE
{
    ULONG CoreFrequency;
    ULONG Power;
    ULONG TransitionLatency;
    ULONG BusMasterLatency;
    ULONG Control;
    ULONG Status;
} PROCESSR_PSS_STATE, *PPROCESSR_PSS_STATE;

typedef struct _PROCESSR_PSS
{
    ULONG Count;
    PROCESSR_PSS_STATE States[ANYSIZE_ARRAY];
} PROCESSR_PSS, *PPROCESSR_PSS;

typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDevice;

    ULONG ProcessorIndex;
    BOOLEAN PpmRegistered;
    PPROCESSOR_PERF_STATES KernelPerfStates;
    PPROCESSOR_PERF_INFO KernelPerfProcInfo;
    UCHAR IntelPerfStateCount;
    UCHAR IntelPerfRatio[32];
    ULONG IntelNominalMhz;
    ULONG IntelMaxRatio;

    PPROCESSR_PSS AcpiPss;
    PROCESSR_ACPI_CTRL_STATUS AcpiPct;
    BOOLEAN AcpiPctValid;
    PVOID AcpiPctMemoryVirt;
    SIZE_T AcpiPctMemoryBytes;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* misc.c */

NTSTATUS
NTAPI
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);


/* pnp.c */

NTSTATUS
NTAPI
ProcessorPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
ProcessorAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo);

NTSTATUS
NTAPI
ProcessrInitPpmDispatch(
    VOID);

VOID
ProcessrRegisterIntelPerfIfNeeded(
    _Inout_ PDEVICE_EXTENSION DevExt);

VOID
ProcessrUnregisterIntelPerf(
    _Inout_ PDEVICE_EXTENSION DevExt);

NTSTATUS
NTAPI
ProcessrCollectAcpiPerfStates(
    _In_ PDEVICE_OBJECT ProcessorPdo,
    _Inout_ PDEVICE_EXTENSION DevExt);

VOID
ProcessrReleaseAcpiPerfData(
    _Inout_ PDEVICE_EXTENSION DevExt);

#endif /* _PROCESSR_PCH_ */
