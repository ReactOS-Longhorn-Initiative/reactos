#pragma once

typedef NTSTATUS
(NTAPI *KD_GET_RX_PACKET)(
    _In_ PVOID Adapter,
    _Out_ PULONG Handle,
    _Out_ PVOID *Packet,
    _Out_ PULONG Length);

typedef VOID
(NTAPI *KD_RELEASE_RX_PACKET)(
    _In_ PVOID Adapter,
    _In_ ULONG Handle);

typedef NTSTATUS
(NTAPI *KD_GET_TX_PACKET)(
    _In_ PVOID Adapter,
    _Out_ PULONG Handle);

typedef NTSTATUS
(NTAPI *KD_SEND_TX_PACKET)(
    _In_ PVOID Adapter,
    _In_ ULONG Handle,
    _In_ ULONG Length);

typedef PVOID
(NTAPI *KD_GET_PACKET_ADDRESS)(
    _In_ PVOID Adapter,
    _In_ ULONG Handle);

typedef ULONG
(NTAPI *KD_GET_PACKET_LENGTH)(
    _In_ PVOID Adapter,
    _In_ ULONG Handle);

#define KDNET_EXT_EXPORTS 13

typedef struct _KDNET_EXTENSIBILITY_EXPORTS
{
    ULONG FunctionCount;
    PVOID KdInitializeController;
    PVOID KdShutdownController;
    PVOID KdSetHibernateRange;
    KD_GET_RX_PACKET KdGetRxPacket;
    KD_RELEASE_RX_PACKET KdReleaseRxPacket;
    KD_GET_TX_PACKET KdGetTxPacket;
    KD_SEND_TX_PACKET KdSendTxPacket;
    KD_GET_PACKET_ADDRESS KdGetPacketAddress;
    KD_GET_PACKET_LENGTH KdGetPacketLength;
    PVOID KdGetHardwareContextSize;
    PVOID KdDeviceControl;
    PVOID KdReadSerialByte;
    PVOID KdWriteSerialByte;
    PVOID DebugSerialOutputInit;
    PVOID DebugSerialOutputByte;
} KDNET_EXTENSIBILITY_EXPORTS, *PKDNET_EXTENSIBILITY_EXPORTS;

#define KDNET_EXT_IMPORTS 30


typedef
PHYSICAL_ADDRESS
(NTAPI *KDNET_GET_PHYSICAL_ADDRESS) (
    PVOID Va
    );

typedef
VOID
(NTAPI *KDNET_STALL_EXECUTION_PROCESSOR) (
    ULONG Microseconds
    );

typedef
UCHAR
(NTAPI *KDNET_READ_REGISTER_UCHAR) (
    PUCHAR Register
    );

typedef
USHORT
(NTAPI *KDNET_READ_REGISTER_USHORT) (
    PUSHORT Register
    );

typedef
ULONG
(NTAPI *KDNET_READ_REGISTER_ULONG) (
    PULONG Register
    );

typedef
ULONG64
(NTAPI *KDNET_READ_REGISTER_ULONG64) (
    PULONG64 Register
    );

typedef
VOID
(NTAPI *KDNET_WRITE_REGISTER_UCHAR) (
    PUCHAR Register,
    UCHAR Value
    );

typedef
VOID
(NTAPI *KDNET_WRITE_REGISTER_USHORT) (
    PUSHORT Register,
    USHORT Value
    );

typedef
VOID
(NTAPI *KDNET_WRITE_REGISTER_ULONG) (
    PULONG Register,
    ULONG Value
    );

typedef
VOID
(NTAPI *KDNET_WRITE_REGISTER_ULONG64) (
    PULONG64 Register,
    ULONG64 Value
    );

typedef
UCHAR
(NTAPI *KDNET_READ_PORT_UCHAR) (
    PUCHAR Port
    );

typedef
USHORT
(NTAPI *KDNET_READ_PORT_USHORT) (
    PUSHORT Port
    );

typedef
ULONG
(NTAPI *KDNET_READ_PORT_ULONG) (
    PULONG Port
    );

typedef
ULONG
(NTAPI *KDNET_READ_PORT_ULONG64) (
    PULONG64 Port
    );

typedef
VOID
(NTAPI *KDNET_WRITE_PORT_UCHAR) (
    PUCHAR Port,
    UCHAR Value
    );

typedef
VOID
(NTAPI *KDNET_WRITE_PORT_USHORT) (
    PUSHORT Port,
    USHORT Value
    );

typedef
VOID
(NTAPI *KDNET_WRITE_PORT_ULONG) (
    PULONG Port,
    ULONG Value
    );

typedef
VOID
(NTAPI *KDNET_WRITE_PORT_ULONG64) (
    PULONG Port,
    ULONG64 Value
    );

typedef
ULONG
(NTAPI *KDNET_GET_PCI_DATA_BY_OFFSET) (
    ULONG BusNumber,
    ULONG SlotNumber,
    __out_bcount(Length) PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

typedef
ULONG
(NTAPI *KDNET_SET_PCI_DATA_BY_OFFSET) (
    ULONG BusNumber,
    ULONG SlotNumber,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

typedef
VOID
(NTAPI *KDNET_SET_DEBUGGER_NOT_PRESENT) (
    BOOLEAN NotPresent
    );

typedef
VOID
(NTAPI *KDNET_SET_HIBER_RANGE) (
    PVOID MemoryMap,
    ULONG     Flags,
    PVOID     Address,
    ULONG_PTR Length,
    ULONG     Tag
    );

typedef
VOID
(NTAPI *KDNET_BUGCHECK_EX) (
    ULONG BugCheckCode,
    ULONG_PTR BugCheckParameter1,
    ULONG_PTR BugCheckParameter2,
    ULONG_PTR BugCheckParameter3,
    ULONG_PTR BugCheckParameter4
    );

typedef
PVOID
(NTAPI *KDNET_MAP_PHYSICAL_MEMORY_64) (
    PHYSICAL_ADDRESS PhysicalAddress,
    ULONG NumberPages,
    BOOLEAN FlushCurrentTLB
    );

typedef
VOID
(NTAPI *KDNET_UNMAP_VIRTUAL_ADDRESS)(
    PVOID VirtualAddress,
    ULONG NumberPages,
    BOOLEAN FlushCurrentTLB
    );

typedef
ULONG64
(NTAPI *KDNET_READ_CYCLE_COUNTER) (
    PULONG64 Frequency
);

typedef
VOID
(NTAPI *KDNET_DBGPRINT)(
     PCHAR pFmt,
     ...
   );



#pragma pack(push,1)
typedef struct _KDNET_EXTENSIBILITY_IMPORTS
{
    ULONG FunctionCount;
    UINT32 Padding;
    KDNET_GET_PCI_DATA_BY_OFFSET GetPciDataByOffset;
    KDNET_SET_PCI_DATA_BY_OFFSET SetPciDataByOffset;
    KDNET_GET_PHYSICAL_ADDRESS GetPhysicalAddress;
    KDNET_STALL_EXECUTION_PROCESSOR StallExecutionProcessor;
    KDNET_READ_REGISTER_UCHAR ReadRegisterUChar;
    KDNET_READ_REGISTER_USHORT ReadRegisterUShort;
    KDNET_READ_REGISTER_ULONG ReadRegisterULong;
    KDNET_READ_REGISTER_ULONG64 ReadRegisterULong64;
    KDNET_WRITE_REGISTER_UCHAR WriteRegisterUChar;
    KDNET_WRITE_REGISTER_USHORT WriteRegisterUShort;
    KDNET_WRITE_REGISTER_ULONG WriteRegisterULong;
    KDNET_WRITE_REGISTER_ULONG64 WriteRegisterULong64;
    KDNET_READ_PORT_UCHAR ReadPortUChar;
    KDNET_READ_PORT_USHORT ReadPortUShort;
    KDNET_READ_PORT_ULONG ReadPortULong;
    KDNET_READ_PORT_ULONG64 ReadPortULong64;
    KDNET_WRITE_PORT_UCHAR WritePortUChar;
    KDNET_WRITE_PORT_USHORT WritePortUShort;
    KDNET_WRITE_PORT_ULONG WritePortULong;
    KDNET_WRITE_PORT_ULONG64 WritePortULong64;
    KDNET_SET_HIBER_RANGE SetHiberRange;
    NTSTATUS* KdNetErrorStatus;
    PUCHAR** KdNetErrorString;
    UINT32* KdNetHardwareID;
} KDNET_EXTENSIBILITY_IMPORTS, *PKDNET_EXTENSIBILITY_IMPORTS;

#pragma pack(pop)
extern KDNET_EXTENSIBILITY_IMPORTS* KdNetExtensibilityImports;
