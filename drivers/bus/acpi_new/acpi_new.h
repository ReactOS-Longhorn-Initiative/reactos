#pragma once

#include <initguid.h>
#include <ntddk.h>
#include <ntifs.h>

//#define NDEBUG
#include <debug.h>

#include <uacpi/internal/opregion.h>
#include <uacpi/notify.h>

typedef enum _ACPI_NEW_DEVTYPE
{
    AcpiNewDeviceFdo,
    AcpiNewDevicePdo,
} ACPI_NEW_DEVTYPE;

typedef struct _ACPI_NEW_COMMON_EXTENSION
{
    ACPI_NEW_DEVTYPE Type;
    PDEVICE_OBJECT Self;
} ACPI_NEW_COMMON_EXTENSION, *PACPI_NEW_COMMON_EXTENSION;

typedef struct _ACPI_NEW_PDO_EXTENSION
{
    ACPI_NEW_COMMON_EXTENSION Common;

    PDEVICE_OBJECT ParentFdo;
    LIST_ENTRY Link;

    uacpi_namespace_node *Node;
    uacpi_u32 StaFlags;

    PWSTR DeviceId;
    PWSTR HardwareIds;
    PWSTR CompatibleIds;
    PWSTR InstanceId;

    FAST_MUTEX NotifyLock;
    LIST_ENTRY NotifyList;

    BOOLEAN Present;
} ACPI_NEW_PDO_EXTENSION, *PACPI_NEW_PDO_EXTENSION;

typedef struct _ACPI_NEW_FDO_EXTENSION
{
    ACPI_NEW_COMMON_EXTENSION Common;

    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT LowerDevice;
    FAST_MUTEX Mutex;
    LIST_ENTRY PdoList;

    BOOLEAN Started;
    BOOLEAN Enumerated;

    volatile LONG EnumerationDirty;
    volatile LONG RefreshInProgress;

    volatile LONG NotifyWorkQueued;
    WORK_QUEUE_ITEM NotifyWorkItem;

    BOOLEAN Removed;
} ACPI_NEW_FDO_EXTENSION, *PACPI_NEW_FDO_EXTENSION;

static __forceinline BOOLEAN AcpiNewIsFdo(_In_ PDEVICE_OBJECT DeviceObject)
{
    PACPI_NEW_COMMON_EXTENSION common = (PACPI_NEW_COMMON_EXTENSION)DeviceObject->DeviceExtension;
    return common && (common->Type == AcpiNewDeviceFdo);
}

static __forceinline BOOLEAN AcpiNewIsPdo(_In_ PDEVICE_OBJECT DeviceObject)
{
    PACPI_NEW_COMMON_EXTENSION common = (PACPI_NEW_COMMON_EXTENSION)DeviceObject->DeviceExtension;
    return common && (common->Type == AcpiNewDevicePdo);
}

// driver.c
DRIVER_INITIALIZE DriverEntry;

// adddev.c
DRIVER_ADD_DEVICE AcpiNewAddDevice;
DRIVER_UNLOAD AcpiNewUnload;

// dispatch.c
DRIVER_DISPATCH AcpiNewDispatchPower;
DRIVER_DISPATCH AcpiNewDispatchCreateClose;
DRIVER_DISPATCH AcpiNewDispatchDefault;
DRIVER_DISPATCH AcpiNewDispatchDeviceControl;

// pnp.c
DRIVER_DISPATCH AcpiNewDispatchPnp;

// util.c
NTSTATUS AcpiNewForwardIrpSynchronously(_In_ PACPI_NEW_FDO_EXTENSION FdoExt, _In_ PIRP Irp);
PWSTR AcpiNewDupUnicodeString(_In_reads_(Chars) const WCHAR *Src, _In_ SIZE_T Chars);
PWSTR AcpiNewDupMultiSz(_In_z_ const WCHAR *Src);

// ids.c
NTSTATUS AcpiNewBuildIdsFromHid(
    _In_z_ const uacpi_char *Hid,
    _Outptr_result_nullonfailure_ PWSTR *OutDeviceId,
    _Outptr_result_nullonfailure_ PWSTR *OutHardwareIds
);
NTSTATUS AcpiNewBuildCompatibleIdsFromCid(
    _In_ uacpi_namespace_node *Node,
    _Outptr_result_maybenull_ PWSTR *OutCompatibleIds
);

// enum.c
VOID AcpiNewRefreshEnumeration(_In_ PACPI_NEW_FDO_EXTENSION FdoExt);
VOID AcpiNewEnumerateNamespace(_In_ PACPI_NEW_FDO_EXTENSION FdoExt);

// uacpi_init.c
NTSTATUS AcpiNewStartUacpiAndEnumerate(_In_ PACPI_NEW_FDO_EXTENSION FdoExt);

// notify.c
uacpi_status AcpiNewNotifyHandler(
    _In_ uacpi_handle context,
    _In_ uacpi_namespace_node *node,
    _In_ uacpi_u64 value
);

// interface.c
NTSTATUS AcpiNewPdoQueryInterface(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ PIRP Irp);

// resources.c
NTSTATUS AcpiNewPdoQueryResources(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ PIRP Irp);
NTSTATUS AcpiNewPdoQueryResourceRequirements(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ PIRP Irp);
