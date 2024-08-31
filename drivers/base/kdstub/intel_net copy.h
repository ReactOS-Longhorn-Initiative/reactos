#pragma once

#define NOEXTAPI
#include <ntifs.h>
#include "kdnetshareddata.h"
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
(*KDNET_GET_PHYSICAL_ADDRESS) (
    PVOID Va
    );

typedef
VOID
(*KDNET_STALL_EXECUTION_PROCESSOR) (
    ULONG Microseconds
    );

typedef
UCHAR
(*KDNET_READ_REGISTER_UCHAR) (
    PUCHAR Register
    );

typedef
USHORT
(*KDNET_READ_REGISTER_USHORT) (
    PUSHORT Register
    );

typedef
ULONG
(*KDNET_READ_REGISTER_ULONG) (
    PULONG Register
    );

typedef
ULONG64
(*KDNET_READ_REGISTER_ULONG64) (
    PULONG64 Register
    );

typedef
VOID
(*KDNET_WRITE_REGISTER_UCHAR) (
    PUCHAR Register,
    UCHAR Value
    );

typedef
VOID
(*KDNET_WRITE_REGISTER_USHORT) (
    PUSHORT Register,
    USHORT Value
    );

typedef
VOID
(*KDNET_WRITE_REGISTER_ULONG) (
    PULONG Register,
    ULONG Value
    );

typedef
VOID
(*KDNET_WRITE_REGISTER_ULONG64) (
    PULONG64 Register,
    ULONG64 Value
    );

typedef
UCHAR
(*KDNET_READ_PORT_UCHAR) (
    PUCHAR Port
    );

typedef
USHORT
(*KDNET_READ_PORT_USHORT) (
    PUSHORT Port
    );

typedef
ULONG
(*KDNET_READ_PORT_ULONG) (
    PULONG Port
    );

typedef
ULONG
(*KDNET_READ_PORT_ULONG64) (
    PULONG64 Port
    );

typedef
VOID
(*KDNET_WRITE_PORT_UCHAR) (
    PUCHAR Port,
    UCHAR Value
    );

typedef
VOID
(*KDNET_WRITE_PORT_USHORT) (
    PUSHORT Port,
    USHORT Value
    );

typedef
VOID
(*KDNET_WRITE_PORT_ULONG) (
    PULONG Port,
    ULONG Value
    );

typedef
VOID
(*KDNET_WRITE_PORT_ULONG64) (
    PULONG Port,
    ULONG64 Value
    );

typedef
ULONG
(*KDNET_GET_PCI_DATA_BY_OFFSET) (
    ULONG BusNumber,
    ULONG SlotNumber,
    __out_bcount(Length) PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

typedef
ULONG
(*KDNET_SET_PCI_DATA_BY_OFFSET) (
    ULONG BusNumber,
    ULONG SlotNumber,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

typedef
VOID
(*KDNET_SET_DEBUGGER_NOT_PRESENT) (
    BOOLEAN NotPresent
    );

typedef
VOID
(*KDNET_SET_HIBER_RANGE) (
    PVOID MemoryMap,
    ULONG     Flags,
    PVOID     Address,
    ULONG_PTR Length,
    ULONG     Tag
    );

typedef
VOID
(*KDNET_BUGCHECK_EX) (
    ULONG BugCheckCode,
    ULONG_PTR BugCheckParameter1,
    ULONG_PTR BugCheckParameter2,
    ULONG_PTR BugCheckParameter3,
    ULONG_PTR BugCheckParameter4
    );

typedef
PVOID
(*KDNET_MAP_PHYSICAL_MEMORY_64) (
    PHYSICAL_ADDRESS PhysicalAddress,
    ULONG NumberPages,
    BOOLEAN FlushCurrentTLB
    );

typedef
VOID
(*KDNET_UNMAP_VIRTUAL_ADDRESS)(
    PVOID VirtualAddress,
    ULONG NumberPages,
    BOOLEAN FlushCurrentTLB
    );

typedef
ULONG64
(*KDNET_READ_CYCLE_COUNTER) (
    PULONG64 Frequency
);

typedef
VOID
(*KDNET_DBGPRINT)(
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

#define KdGetPhysicalAddress KdNetExtensibilityImports->GetPhysicalAddress 
#define READ_REGISTER_UCHAR KdNetExtensibilityImports->ReadRegisterUChar
#define READ_REGISTER_USHORT KdNetExtensibilityImports->ReadRegisterUShort
#define READ_REGISTER_ULONG KdNetExtensibilityImports->ReadRegisterULong
#define WRITE_REGISTER_UCHAR KdNetExtensibilityImports->WriteRegisterUChar
#define WRITE_REGISTER_USHORT KdNetExtensibilityImports->WriteRegisterUShort
#define WRITE_REGISTER_ULONG KdNetExtensibilityImports->WriteRegisterULong
#define WRITE_PORT_UCHAR KdNetExtensibilityImports->WritePortUChar
#define WRITE_PORT_USHORT KdNetExtensibilityImports->WritePortUShort
#define WRITE_PORT_ULONG KdNetExtensibilityImports->WritePortULong
#define READ_PORT_UCHAR KdNetExtensibilityImports->ReadPortUChar
#define READ_PORT_USHORT KdNetExtensibilityImports->ReadPortUShort
#define READ_PORT_ULONG KdNetExtensibilityImports->ReadPortULong

#undef READ_REGISTER_ULONG64
#undef WRITE_REGISTER_ULONG64

#if defined(_WIN64)

#define READ_REGISTER_ULONG64 KdNetExtensibilityImports->ReadRegisterULong64
#define WRITE_REGISTER_ULONG64 KdNetExtensibilityImports->WriteRegisterULong64

#endif

#undef KdGetPciDataByOffset
#define KdGetPciDataByOffset KdNetExtensibilityImports->GetPciDataByOffset

#undef KdSetPciDataByOffset
#define KdSetPciDataByOffset KdNetExtensibilityImports->SetPciDataByOffset

#undef KeStallExecutionProcessor
#define KeStallExecutionProcessor KdNetExtensibilityImports->StallExecutionProcessor

#undef PoSetHiberRange
#define PoSetHiberRange KdNetExtensibilityImports->SetHiberRange

#undef _KeBugCheckEx
#define _KeBugCheckEx KdNetExtensibilityImports->BugCheckEx

#undef KdMapPhysicalMemory64
#define KdMapPhysicalMemory64 KdNetExtensibilityImports->MapPhysicalMemory64

#undef KdUnmapVirtualAddress
#define KdUnmapVirtualAddress KdNetExtensibilityImports->UnmapVirtualAddress

#undef KdReadCycleCounter
#define KdReadCycleCounter KdNetExtensibilityImports->ReadCycleCounter

#undef KdDbgPrintf
#define KdDbgPrintf KdNetExtensibilityImports->KdNetDbgPrintf

#define KdNetErrorStatus *(KdNetExtensibilityImports->KdNetErrorStatus)
#define KdNetErrorString *(KdNetExtensibilityImports->KdNetErrorString)
#define KdNetHardwareID *(KdNetExtensibilityImports->KdNetHardwareID)

//
// For ARM platform force strictly aligned copy as unaligned access 
// isn't supported by hardware on stricty ordered memory used for
// DMA transfers (as it is today)
//

#if (_ARM_) || (_ARM64_)
#undef RtlCopyMemory
#define RtlCopyMemory(d, s, l)  _memcpy_strict_align(d, s, l)
#endif

ULONG
KdStubDbgPrint(const char *Format, ...);