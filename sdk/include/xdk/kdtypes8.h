/******************************************************************************
 *                          Kernel Debugger Types                             *
 ******************************************************************************/

$if (_NTDDK_)
typedef struct _DEBUG_DEVICE_ADDRESS {
  UCHAR Type;
  BOOLEAN Valid;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)
  union {
#endif
    UCHAR Reserved[2];
#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)
    struct {
      UCHAR BitWidth;
      UCHAR AccessSize;
    };
  };
#endif
  PUCHAR TranslatedAddress;
  ULONG Length;
} DEBUG_DEVICE_ADDRESS, *PDEBUG_DEVICE_ADDRESS;

typedef struct _DEBUG_MEMORY_REQUIREMENTS {
  PHYSICAL_ADDRESS Start;
  PHYSICAL_ADDRESS MaxEnd;
  PVOID VirtualAddress;
  ULONG Length;
  BOOLEAN Cached;
  BOOLEAN Aligned;
} DEBUG_MEMORY_REQUIREMENTS, *PDEBUG_MEMORY_REQUIREMENTS;

typedef enum {
  KdNameSpacePCI,
  KdNameSpaceACPI,
  KdNameSpaceAny,
  KdNameSpaceNone,
  KdNameSpaceMax, /* Maximum namespace enumerator */
} KD_NAMESPACE_ENUM, *PKD_NAMESPACE_ENUM;

#if (NTDDI_VERSION >= NTDDI_WIN10)
typedef struct _DEBUG_TRANSPORT_DATA {
  ULONG HwContextSize;
  BOOLEAN UseSerialFraming;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
  BOOLEAN ValidUSBCoreId;
  UCHAR USBCoreId;
#endif
} DEBUG_TRANSPORT_DATA, *PDEBUG_TRANSPORT_DATA;
#endif

typedef enum {
    KdConfigureDeviceAndContinue,
    KdSkipDeviceAndContinue,
    KdConfigureDeviceAndStop,
    KdSkipDeviceAndStop,
} KD_CALLBACK_ACTION, *PKD_CALLBACK_ACTION;

#define MAXIMUM_DEBUG_BARS 6

#if (NTDDI_VERSION >= NTDDI_WIN10)
#define DBG_DEVICE_FLAG_HAL_SCRATCH_ALLOCATED 0x01
#define DBG_DEVICE_FLAG_BARS_MAPPED           0x02
#define DBG_DEVICE_FLAG_SCRATCH_ALLOCATED     0x04
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)
#define DBG_DEVICE_FLAG_UNCACHED_MEMORY       0x08
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)
#define DBG_DEVICE_FLAG_SYNTHETIC             0x10
#endif

typedef struct _DEBUG_DEVICE_DESCRIPTOR {
  ULONG Bus;
  ULONG Slot;
  USHORT Segment;
  USHORT VendorID;
  USHORT DeviceID;
  UCHAR BaseClass;
  UCHAR SubClass;
  UCHAR ProgIf;
#if (NTDDI_VERSION >= NTDDI_WIN10)
  union {
#endif
    UCHAR Flags;
#if (NTDDI_VERSION >= NTDDI_WIN10)
    struct {
      UCHAR DbgHalScratchAllocated : 1;
      UCHAR DbgBarsMapped : 1;
      UCHAR DbgScratchAllocated : 1;
    };
  };
#endif
  BOOLEAN Initialized;
  BOOLEAN Configured;
  DEBUG_DEVICE_ADDRESS BaseAddress[MAXIMUM_DEBUG_BARS];
  DEBUG_MEMORY_REQUIREMENTS Memory;
#if (NTDDI_VERSION >= NTDDI_WIN10_19H1)
  ULONG Dbg2TableIndex;
#endif
  USHORT PortType;
  USHORT PortSubtype;
  PVOID OemData;
  ULONG OemDataLength;
  KD_NAMESPACE_ENUM NameSpace;
  PWCHAR NameSpacePath;
  ULONG NameSpacePathLength;
#if (NTDDI_VERSION >= NTDDI_WIN10)
  ULONG TransportType;
  DEBUG_TRANSPORT_DATA TransportData;
#endif
} DEBUG_DEVICE_DESCRIPTOR, *PDEBUG_DEVICE_DESCRIPTOR;

typedef
KD_CALLBACK_ACTION
(NTAPI *PDEBUG_DEVICE_FOUND_FUNCTION) (
    PDEBUG_DEVICE_DESCRIPTOR Device);

typedef NTSTATUS
(NTAPI *pKdSetupPciDeviceForDebugging)(
  _In_opt_ PVOID LoaderBlock,
  _Inout_ PDEBUG_DEVICE_DESCRIPTOR PciDevice);

typedef NTSTATUS
(NTAPI *pKdReleasePciDeviceForDebugging)(
  _Inout_ PDEBUG_DEVICE_DESCRIPTOR PciDevice);

typedef PVOID
(NTAPI *pKdGetAcpiTablePhase0)(
  _In_ struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
  _In_ ULONG Signature);

typedef VOID
(NTAPI *pKdCheckPowerButton)(VOID);

typedef
NTSTATUS
(NTAPI *pKdEnumerateDebuggingDevices) (
    _In_ PVOID LoaderBlock,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device,
    _In_ PDEBUG_DEVICE_FOUND_FUNCTION Callback);

typedef PVOID
(NTAPI *pKdMapPhysicalMemory64)(
  _In_ PHYSICAL_ADDRESS PhysicalAddress,
  _In_ ULONG NumberPages,
  _In_ BOOLEAN FlushCurrentTLB);

typedef VOID
(NTAPI *pKdUnmapVirtualAddress)(
  _In_ PVOID VirtualAddress,
  _In_ ULONG NumberPages,
  _In_ BOOLEAN FlushCurrentTLB);

typedef ULONG
(NTAPI *pKdGetPciDataByOffset)(
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Offset,
  _In_ ULONG Length);

typedef ULONG
(NTAPI *pKdSetPciDataByOffset)(
  _In_ ULONG BusNumber,
  _In_ ULONG SlotNumber,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Offset,
  _In_ ULONG Length);

typedef NTSTATUS
(NTAPI *pKdSetupIntegratedDeviceForDebugging)(
    _In_opt_ PVOID LoaderBlock,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR IntegratedDevice);

typedef NTSTATUS
(NTAPI *pKdReleaseIntegratedDeviceForDebugging)(
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR  IntegratedDevice);
$endif (_NTDDK_)
