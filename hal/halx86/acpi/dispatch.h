
#pragma once

typedef struct _HALP_STATE_DATA {
    UCHAR Data0;
    UCHAR Data1;
    UCHAR Data2;
} HALP_STATE_DATA, *PHALP_STATE_DATA;

typedef union _HALP_STATE_CONTEXT {
  struct {
    ULONG Data1  :4;  // 0 BitPosition
    ULONG Data2  :4;  // 4 BitPosition
    ULONG Flags  :8;  // 8 BitPosition
    ULONG Pad    :16; // 16 BitPosition
  };
  ULONG AsULONG;
} HALP_STATE_CONTEXT, *PHALP_STATE_CONTEXT;

typedef VOID
(NTAPI * PHAL_ACPI_TIMER_INIT)(
    _In_ PULONG TimerPort,
    _In_ BOOLEAN TimerValExt
);

typedef VOID
(NTAPI * PHAL_ACPI_MACHINE_STATE_INIT)(
    _In_ ULONG Par1,
    _In_ PHALP_STATE_DATA StateData,
    _Out_ ULONG* OutInterruptModel
);

typedef ULONG
(NTAPI * PHAL_ACPI_QUERY_FLAGS)(
    VOID
);

typedef UCHAR
(NTAPI * PHAL_ACPI_PIC_STATE_INTACT)(
    VOID
);

typedef VOID
(NTAPI * PHAL_RESTORE_INT_CONTROLLER_STATE)(
    VOID
);

typedef ULONG
(NTAPI * PHAL_PCI_INTERFACE_READ_CONFIG)(
    _In_ PBUS_HANDLER RootBusHandler,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER SlotNumber,
    _In_ PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
);

typedef ULONG
(NTAPI * PHAL_PCI_INTERFACE_WRITE_CONFIG)(
    _In_ PBUS_HANDLER RootBusHandler,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER SlotNumber,
    _In_ PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
);

typedef VOID
(NTAPI * PHAL_SET_VECTOR_STATE)(
    _In_ ULONG Vector,
    _In_ ULONG Par2
);

typedef ULONG
(NTAPI * PHAL_GET_APIC_VERSION)(
    _In_ ULONG Par1
);

typedef VOID
(NTAPI * PHAL_SET_MAX_LEGACY_PCI_BUS_NUMBER)(
    _In_ ULONG Par1
);

typedef BOOLEAN
(NTAPI * PHAL_IS_VECTOR_VALID)(
    _In_ ULONG Vector
);

typedef struct _ACPI_PM_DISPATCH_TABLE
{
    ULONG Signature;
    ULONG Version;
    PHAL_ACPI_TIMER_INIT HalAcpiTimerInit;                          // HaliAcpiTimerInit
    PVOID HalAcpiTimerInterrupt;
    PHAL_ACPI_MACHINE_STATE_INIT HalAcpiMachineStateInit;           // HaliAcpiMachineStateInit
    PHAL_ACPI_QUERY_FLAGS HalAcpiQueryFlags;                        // HaliAcpiQueryFlags
    PHAL_ACPI_PIC_STATE_INTACT HalAcpiPicStateIntact;               // HalpAcpiPicStateIntact
    PHAL_RESTORE_INT_CONTROLLER_STATE HalRestoreIntControllerState; // HalpRestoreInterruptControllerState
    PHAL_PCI_INTERFACE_READ_CONFIG HalPciInterfaceReadConfig;       // HaliPciInterfaceReadConfig
    PHAL_PCI_INTERFACE_WRITE_CONFIG HalPciInterfaceWriteConfig;     // HaliPciInterfaceWriteConfig
    PHAL_SET_VECTOR_STATE HalSetVectorState;                        // HaliSetVectorState
    PHAL_GET_APIC_VERSION HalGetApicVersion;                        // HalpGetApicVersion
    PHAL_SET_MAX_LEGACY_PCI_BUS_NUMBER HalSetMaxLegacyPciBusNumber; // HaliSetMaxLegacyPciBusNumber
    PHAL_IS_VECTOR_VALID HalIsVectorValid;                          // HaliIsVectorValid
} ACPI_PM_DISPATCH_TABLE, *PACPI_PM_DISPATCH_TABLE;

VOID
NTAPI
HaliAcpiMachineStateInit(
    _In_ ULONG Par1,
    _In_ PHALP_STATE_DATA StateData,
    _Out_ ULONG* OutInterruptModel
);

ULONG
NTAPI
HaliAcpiQueryFlags(
    VOID
);

UCHAR
NTAPI
HalpAcpiPicStateIntact(
    VOID
);

VOID
NTAPI
HalpRestoreInterruptControllerState(
    VOID
);

VOID
NTAPI
HaliSetVectorState(
    _In_ ULONG Vector,
    _In_ ULONG Par2
);

ULONG
NTAPI
HalpGetApicVersion(
    _In_ ULONG Par1
);

VOID
NTAPI
HaliSetMaxLegacyPciBusNumber(
    _In_ ULONG MaxLegacyPciBusNumber
);

BOOLEAN
NTAPI
HaliIsVectorValid(
    _In_ ULONG Vector
);

/* EOF */
