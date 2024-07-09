/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpreport.c
 * PURPOSE:         Device Changes Reporting Functions
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 *                  Pierre Schweitzer
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ERESOURCE PpRegistryDeviceResource;

/* TYPES *******************************************************************/

typedef struct _INTERNAL_WORK_QUEUE_ITEM
{
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_CHANGE_COMPLETE_CALLBACK Callback;
    PVOID Context;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure;
} INTERNAL_WORK_QUEUE_ITEM, *PINTERNAL_WORK_QUEUE_ITEM;

NTSTATUS
PpSetCustomTargetEvent(IN PDEVICE_OBJECT DeviceObject,
                       IN OUT PKEVENT SyncEvent OPTIONAL,
                       IN OUT PNTSTATUS SyncStatus OPTIONAL,
                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback OPTIONAL,
                       IN PVOID Context OPTIONAL,
                       IN PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure);

/* PRIVATE FUNCTIONS *********************************************************/

PWCHAR
IopGetInterfaceTypeString(INTERFACE_TYPE IfType)
{
    switch (IfType)
    {
        case Internal:
            return L"Internal";

        case Isa:
            return L"Isa";

        case Eisa:
            return L"Eisa";

        case MicroChannel:
            return L"MicroChannel";

        case TurboChannel:
            return L"TurboChannel";

        case PCIBus:
            return L"PCIBus";

        case VMEBus:
            return L"VMEBus";

        case NuBus:
            return L"NuBus";

        case PCMCIABus:
            return L"PCMCIABus";

        case CBus:
            return L"CBus";

        case MPIBus:
            return L"MPIBus";

        case MPSABus:
            return L"MPSABus";

        case ProcessorInternal:
            return L"ProcessorInternal";

        case PNPISABus:
            return L"PNPISABus";

        case PNPBus:
            return L"PNPBus";

        case Vmcs:
            return L"Vmcs";

        default:
            DPRINT1("Invalid bus type: %d\n", IfType);
            return NULL;
    }
}

VOID
NTAPI
IopReportTargetDeviceChangeAsyncWorker(PVOID Context)
{
    PINTERNAL_WORK_QUEUE_ITEM Item;

    Item = (PINTERNAL_WORK_QUEUE_ITEM)Context;
    PpSetCustomTargetEvent(Item->PhysicalDeviceObject, NULL, NULL, Item->Callback, Item->Context, Item->NotificationStructure);
    ObDereferenceObject(Item->PhysicalDeviceObject);
    ExFreePoolWithTag(Context, '  pP');
}

NTSTATUS
PpSetCustomTargetEvent(IN PDEVICE_OBJECT DeviceObject,
                       IN OUT PKEVENT SyncEvent OPTIONAL,
                       IN OUT PNTSTATUS SyncStatus OPTIONAL,
                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback OPTIONAL,
                       IN PVOID Context OPTIONAL,
                       IN PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);//IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PpCreateLegacyDeviceIds(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PUNICODE_STRING ServiceName,
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    PIOPNP_DEVICE_EXTENSION DeviceExtension;
    INTERFACE_TYPE InterfaceType;
    PWCHAR BusName;
    WCHAR Buffer[200];
    size_t Remaining = sizeof(Buffer);
    PWCHAR EndBuffer;
    PWCHAR Id = NULL;
    ULONG Length;

    DPRINT("PpCreateLegacyDeviceIds: DeviceObject - %p, ServiceName - %wZ\n",
           DeviceObject, ServiceName);

    if (ResourceList)
    {
        InterfaceType = ResourceList->List[0].InterfaceType;

        if (InterfaceType > MaximumInterfaceType ||
            InterfaceType < InterfaceTypeUndefined)
        {
            InterfaceType = MaximumInterfaceType;
        }
    }
    else
    {
        InterfaceType = Internal;
    }

    BusName = IopGetBusName(InterfaceType);
    DPRINT("PpCreateLegacyDeviceIds: InterfaceType - %S\n", BusName);

    RtlZeroMemory(Buffer, sizeof(Buffer));
    RtlStringCbPrintfExW(Buffer,
                         Remaining,
                         &EndBuffer,
                         &Remaining,
                         0,
                         L"%ws%ws\\%wZ",
                         L"DETECTED",
                         BusName,
                         ServiceName);
    DPRINT("PpCreateLegacyDeviceIds: Buffer - %S\n", Buffer);

    EndBuffer++;
    Remaining -= sizeof(UNICODE_NULL);

    RtlStringCbPrintfExW(EndBuffer,
                         Remaining,
                         NULL,
                         &Remaining,
                         0,
                         L"%ws\\%wZ",
                         L"DETECTED",
                         ServiceName);
    DPRINT("PpCreateLegacyDeviceIds: EndBuffer - %S\n", EndBuffer);

    Length = sizeof(Buffer) - (Remaining - 2 * sizeof(UNICODE_NULL));

    Id = ExAllocatePoolWithTag(PagedPool, Length, 'oipP');
    if (!Id)
    {
        DPRINT("PpCreateLegacyDeviceIds: error\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Id, Buffer, Length);

    DeviceExtension = DeviceObject->DeviceExtension;
    DeviceExtension->CompatibleIdList = Id;
    DeviceExtension->CompatibleIdListSize = Length;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IoReportResourceUsageInternal(_In_ ARBITER_REQUEST_SOURCE AllocationType,
                              _In_ PUNICODE_STRING DriverClassName,
                              _In_ PDRIVER_OBJECT DriverObject,
                              _In_ PCM_RESOURCE_LIST DriverList,
                              _In_ ULONG DriverListSize,
                              _In_ PDEVICE_OBJECT DeviceObject,
                              _In_ PCM_RESOURCE_LIST DeviceList,
                              _In_ ULONG DeviceListSize,
                              _In_ BOOLEAN OverrideConflict,
                              _Out_ PBOOLEAN OutConflictDetected)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);//IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopDuplicateDetection(_In_ INTERFACE_TYPE LegacyBusType,
                      _In_ ULONG BusNumber,
                      _In_ ULONG SlotNumber,
                      _Out_ PDEVICE_NODE * OutDeviceNode)
{
    PLEGACY_DEVICE_DETECTION_INTERFACE LegacyDetectInterface;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_NODE LegacyNode;
    NTSTATUS Status;

    DPRINT("IopDuplicateDetection: LegacyBusType %X BusNumber %X SlotNumber %X\n", LegacyBusType, BusNumber, SlotNumber);

    *OutDeviceNode = NULL;

    LegacyNode = IopFindLegacyBusDeviceNode(LegacyBusType, BusNumber);
    if (!LegacyNode)
    {
        DPRINT1("IopDuplicateDetection: STATUS_INVALID_DEVICE_REQUEST\n");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Status = IopQueryResourceHandlerInterface(IOP_RES_HANDLER_TYPE_LEGACY,
                                              LegacyNode->PhysicalDeviceObject,
                                              0,
                                              (PVOID *)&LegacyDetectInterface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDuplicateDetection: STATUS_INVALID_DEVICE_REQUEST\n");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (!LegacyDetectInterface)
    {
        DPRINT1("IopDuplicateDetection: STATUS_INVALID_DEVICE_REQUEST\n");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Status = LegacyDetectInterface->LegacyDeviceDetection(LegacyDetectInterface->Context,
                                                          LegacyBusType,
                                                          BusNumber,
                                                          SlotNumber,
                                                          &PhysicalDeviceObject);
    if (NT_SUCCESS(Status) && PhysicalDeviceObject)
    {
        *OutDeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
        Status = STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("IopDuplicateDetection: STATUS_INVALID_DEVICE_REQUEST\n");
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    LegacyDetectInterface->InterfaceDereference(LegacyDetectInterface->Context);
    ExFreePool(LegacyDetectInterface);

    return Status;
}

static
BOOLEAN
PipIsDuplicatedDevices(
    _In_ PCM_RESOURCE_LIST CmResource1,
    _In_ PCM_RESOURCE_LIST CmResource2)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);//IoDbgBreakPointEx();
    return FALSE;
}

static
BOOLEAN
IopIsReportedAlready(
    _In_ HANDLE handle,
    _In_ PCUNICODE_STRING ServiceKeyName,
    _In_ PCM_RESOURCE_LIST InCmResource,
    _Out_ PBOOLEAN OutMatchingKey)
{
    UNICODE_STRING DeviceReportedName = RTL_CONSTANT_STRING(L"DeviceReported");
    UNICODE_STRING LogConfName = RTL_CONSTANT_STRING(L"LogConf");
    UNICODE_STRING ControlName = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING KeyValueName;
    PKEY_VALUE_FULL_INFORMATION DeviceReportedInfo = NULL;
    PKEY_VALUE_FULL_INFORMATION BootConfigInfo = NULL;
    PKEY_VALUE_FULL_INFORMATION ServiceInfo = NULL;
    PCM_RESOURCE_LIST BootCmResource = NULL;
    HANDLE KeyHandle = NULL;
    HANDLE Handle;
    ULONG Data;
    NTSTATUS Status;
    USHORT Size;
    BOOLEAN Result = FALSE;

    PAGED_CODE();
    DPRINT("IopIsReportedAlready: Service '%wZ', InCmResource %p\n", ServiceKeyName, InCmResource);

    *OutMatchingKey = FALSE;

    Status = IopGetRegistryValue(handle, L"Service", &ServiceInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopIsReportedAlready: Status %X\n", Status);
        goto OpenControlKey;
    }

    if (ServiceInfo->Type != REG_SZ)
    {
        DPRINT1("IopIsReportedAlready: Status %X\n", Status);
        goto OpenControlKey;
    }

    if (!ServiceInfo->DataLength)
    {
        DPRINT1("IopIsReportedAlready: Status %X\n", Status);
        goto OpenControlKey;
    }

    Size = (USHORT)ServiceInfo->DataLength;

    KeyValueName.Length = Size;
    KeyValueName.MaximumLength = Size;
    KeyValueName.Buffer = (PWSTR)((ULONG_PTR)ServiceInfo + ServiceInfo->DataOffset);

    if (KeyValueName.Buffer[(Size / sizeof(WCHAR)) - 1] == 0)
        KeyValueName.Length = (Size - sizeof(WCHAR));

    if (!RtlEqualUnicodeString(ServiceKeyName, &KeyValueName, TRUE))
        goto OpenControlKey;

    Status = IopOpenRegistryKeyEx(&Handle, handle, &LogConfName, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopIsReportedAlready: Status %X\n", Status);

        if (!InCmResource)
            *OutMatchingKey = TRUE;

        goto OpenControlKey;
    }

    Status = IopGetRegistryValue(Handle, L"BootConfig", &BootConfigInfo);
    ZwClose(Handle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopIsReportedAlready: Status %X\n", Status);

        if (!InCmResource)
            *OutMatchingKey = TRUE;

        goto OpenControlKey;
    }

    if (BootConfigInfo->Type == REG_RESOURCE_LIST && BootConfigInfo->DataLength)
    {
        BootCmResource = (PCM_RESOURCE_LIST)((ULONG_PTR)BootConfigInfo + BootConfigInfo->DataOffset);

        if (InCmResource && BootCmResource)
        {
            if (PipIsDuplicatedDevices(InCmResource, BootCmResource))
                *OutMatchingKey = TRUE;
        }
    }

    if (!InCmResource && !BootCmResource)
        *OutMatchingKey = TRUE;

OpenControlKey:

    Status = IopOpenRegistryKeyEx(&KeyHandle, handle, &ControlName, KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopIsReportedAlready: Status %X\n", Status);
        goto Exit;
    }

    Status = IopGetRegistryValue(KeyHandle, L"DeviceReported", &DeviceReportedInfo);
    if (NT_SUCCESS(Status))
        goto Exit;

    if (*OutMatchingKey != TRUE)
        goto Exit;

     Result = TRUE;

     Data = 1;
     Status = ZwSetValueKey(KeyHandle, &DeviceReportedName, 0, REG_DWORD, &Data, sizeof(Data));
     if (!NT_SUCCESS(Status))
     {
         DPRINT1("IopIsReportedAlready: Status %X\n", Status);
         Result = FALSE;
     }

Exit:

    if (KeyHandle)
        ZwClose(KeyHandle);

    if (DeviceReportedInfo)
        ExFreePool(DeviceReportedInfo);

    if (BootConfigInfo)
        ExFreePool(BootConfigInfo);

    if (ServiceInfo)
        ExFreePool(ServiceInfo);

    return Result;
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
IoReportDetectedDevice(IN PDRIVER_OBJECT DriverObject,
                       IN INTERFACE_TYPE LegacyBusType,
                       IN ULONG BusNumber,
                       IN ULONG SlotNumber,
                       IN PCM_RESOURCE_LIST ResourceList,
                       IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements OPTIONAL,
                       IN BOOLEAN ResourceAssigned,
                       IN OUT PDEVICE_OBJECT* DeviceObject OPTIONAL)
{
    UNICODE_STRING EnumKeyName = RTL_CONSTANT_STRING(IO_REG_KEY_ENUM);
    UNICODE_STRING NoResourceAtInitTimeName = RTL_CONSTANT_STRING(L"NoResourceAtInitTime");
    UNICODE_STRING BasicConfigVectorName = RTL_CONSTANT_STRING(L"BasicConfigVector");
    UNICODE_STRING DeviceReportedName = RTL_CONSTANT_STRING(L"DeviceReported");
    UNICODE_STRING ConfigFlagsName = RTL_CONSTANT_STRING(L"ConfigFlags");
    UNICODE_STRING BootConfigName = RTL_CONSTANT_STRING(L"BootConfig");
    UNICODE_STRING PnP_Manager = RTL_CONSTANT_STRING(L"PnP Manager");
    UNICODE_STRING MigratedName = RTL_CONSTANT_STRING(L"Migrated");
    UNICODE_STRING LogConfName = RTL_CONSTANT_STRING(L"LogConf");
    UNICODE_STRING ControlName = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING ServiceName = RTL_CONSTANT_STRING(L"Service");
    UNICODE_STRING LegacyName = RTL_CONSTANT_STRING(L"Legacy");
    UNICODE_STRING EnumSubKeyName;
    UNICODE_STRING InstancePath;
    UNICODE_STRING DriverName;
    PUNICODE_STRING LegacyServiceName;
    PUNICODE_STRING ServiceKeyName;
    WCHAR Buffer[MAX_DEVICE_ID_LEN];
    PWCHAR EndBuffer;
    PWCHAR EndServiceName;
    PWSTR ServiceKeyString;
    PWSTR NameStr;
    PDEVICE_OBJECT Pdo;
    PDEVICE_NODE DeviceNode;
    HANDLE EnumKeyHandle;
    HANDLE KeyHandle;
    HANDLE Handle;
    HANDLE ControlKeyHandle;
    SIZE_T ResourceListSize = 0;
    ULONG Disposition;
    ULONG Data;
    ULONG Length;
    BOOLEAN IsNeedCleanup = FALSE;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IoReportDetectedDevice: Device %X, *Device %X\n", DeviceObject, (DeviceObject ? *DeviceObject : NULL));

    if (*DeviceObject)
    {
        Pdo = *DeviceObject;
        DeviceNode = IopGetDeviceNode(Pdo);

        if (!DeviceNode) {
            DPRINT1("STATUS_NO_SUCH_DEVICE DeviceNode %X\n", DeviceNode);
            return STATUS_NO_SUCH_DEVICE;
        }

        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&PpRegistryDeviceResource, 1);

        Status = PnpDeviceObjectToDeviceInstance(*DeviceObject, &KeyHandle, KEY_ALL_ACCESS);
        DPRINT("IoReportDetectedDevice: KeyHandle %X\n", KeyHandle);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoReportDetectedDevice: Status %X\n", Status);
            ExReleaseResourceLite(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();
            return Status;
        }

        if (ResourceAssigned)
        {
            Data = 1;
            ZwSetValueKey(KeyHandle, &NoResourceAtInitTimeName, 0, REG_DWORD, &Data, 4);
        }

        Status = IopCreateRegistryKeyEx(&Handle,
                                        KeyHandle,
                                        &LogConfName,
                                        KEY_ALL_ACCESS,
                                        REG_OPTION_NON_VOLATILE,
                                        NULL);
        ZwClose(KeyHandle);

        if (NT_SUCCESS(Status))
        {
            if (ResourceList)
            {
                ResourceListSize = PnpDetermineResourceListSize(ResourceList);
                ZwSetValueKey(Handle, &BootConfigName, 0, REG_RESOURCE_LIST, ResourceList, ResourceListSize);
            }

            if (ResourceRequirements)
            {
                ZwSetValueKey(Handle,
                              &BasicConfigVectorName,
                              0,
                              REG_RESOURCE_REQUIREMENTS_LIST,
                              ResourceRequirements,
                              ResourceRequirements->ListSize);
            }

            ZwClose(Handle);
        }

        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoReportDetectedDevice: Status %X\n", Status);
            return Status;
        }

        DPRINT("IoReportDetectedDevice: Status %X\n", Status);
        goto ResourcesAssign;
    }

    *DeviceObject = NULL;
    ServiceKeyName = &DriverObject->DriverExtension->ServiceKeyName;
    DPRINT("IoReportDetectedDevice: ServiceKey %wZ\n", ServiceKeyName);

    if (DriverObject->Flags & DRVO_BUILTIN_DRIVER)
    {
        EndServiceName = &ServiceKeyName->Buffer[(ServiceKeyName->Length / sizeof(WCHAR)) - 1];
        DriverName.Length = 0;

        while (*EndServiceName != '\\')
        {
            if (EndServiceName == ServiceKeyName->Buffer)
            {
                DPRINT("IoReportDetectedDevice: error\n");
                return STATUS_UNSUCCESSFUL;
            }

            EndServiceName--;
            DriverName.Length += sizeof(WCHAR);
        }

        if (EndServiceName == ServiceKeyName->Buffer)
        {
            DPRINT1("IoReportDetectedDevice: error\n");
            return STATUS_UNSUCCESSFUL;
        }

        DriverName.Buffer = (EndServiceName + 1);
        DriverName.MaximumLength = DriverName.Length + sizeof(WCHAR);
    }
    else
    {
        Status = IopDuplicateDetection(LegacyBusType, BusNumber, SlotNumber, &DeviceNode);
        if (NT_SUCCESS(Status) && DeviceNode)
        {
            Pdo = DeviceNode->PhysicalDeviceObject;

            if (PipAreDriversLoaded(DeviceNode))
            {
                ObDereferenceObject(Pdo);
                return STATUS_NO_SUCH_DEVICE;
            }

            if (DeviceNode->Flags & (DNF_HAS_PROBLEM + DNF_HAS_PRIVATE_PROBLEM))
            {
                if (DeviceNode->Problem != CM_PROB_NOT_CONFIGURED &&
                    DeviceNode->Problem != CM_PROB_REINSTALL &&
                    DeviceNode->Problem != CM_PROB_FAILED_INSTALL)
                {
                    ObDereferenceObject(Pdo);
                    return STATUS_NO_SUCH_DEVICE;
                }
            }

            DeviceNode->Flags &= ~DNF_HAS_PROBLEM;
            DeviceNode->Problem = 0;

            IopDeleteLegacyKey(DriverObject);
            goto ResourcesAssign;
        }
        else
        {
            DPRINT("Status %X DeviceNode %X\n", Status, DeviceNode);
            DriverName.Buffer = NULL;
        }
    }

    Status = IoCreateDevice(IopRootDriverObject,
                            sizeof(IOPNP_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &Pdo);
    DPRINT("IoReportDetectedDevice: Pdo %X\n", Pdo);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoReportDetectedDevice: Status %X\n", Status);
        return Status;
    }

    Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
    DeviceNode = PipAllocateDeviceNode(Pdo);

    if (/*Status == STATUS_SYSTEM_HIVE_TOO_LARGE  ||*/ !DeviceNode)
    {
        DPRINT("IoReportDetectedDevice: error\n");
        IoDeleteDevice(Pdo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!(DriverObject->Flags & DRVO_BUILTIN_DRIVER))
    {
        IopDeleteLegacyKey(DriverObject);
        LegacyServiceName = &DriverObject->DriverExtension->ServiceKeyName;
    }
    else
    {
        LegacyServiceName = &DriverName;
    }

    DPRINT("IoReportDetectedDevice: LegacyServiceName %wZ\n", LegacyServiceName);

    Status = PpCreateLegacyDeviceIds(Pdo, LegacyServiceName, ResourceList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoReportDetectedDevice: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();// IoDeleteDevice(*DeviceObject);
        return Status;
    }

    if (DriverObject->Flags & DRVO_BUILTIN_DRIVER)
        NameStr = DriverName.Buffer;
    else
        NameStr = ServiceKeyName->Buffer;

    DPRINT("IoReportDetectedDevice: NameStr %S\n", NameStr);

    RtlZeroMemory(Buffer, sizeof(Buffer));

    RtlStringCbPrintfExW(Buffer,
                         sizeof(Buffer) / sizeof(WCHAR),
                         &EndBuffer,
                         NULL,
                         0,
                         L"Root\\%ws", // ROOT
                         NameStr);
    DPRINT("IoReportDetectedDevice: Buffer %S\n", Buffer);

    Length = (ULONG_PTR)EndBuffer - (ULONG_PTR)Buffer;
    ASSERT(Length <= (sizeof(Buffer) - sizeof(L"Root\\")));

    InstancePath.MaximumLength = sizeof(Buffer);
    InstancePath.Length = Length;
    InstancePath.Buffer = Buffer;

    DPRINT("Instance %wZ\n", &InstancePath);

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    Status = IopOpenRegistryKeyEx(&EnumKeyHandle, 0, &EnumKeyName, KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoReportDetectedDevice: Status %X\n", Status);
        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();
        ASSERT(FALSE); // IoDbgBreakPointEx();// IoDeleteDevice(*DeviceObject);
        return Status;
    }

    Status = IopCreateRegistryKeyEx(&Handle,
                                    EnumKeyHandle,
                                    &InstancePath,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    &Disposition);
    DPRINT("IoReportDetectedDevice: Disposition %X\n", Disposition);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoReportDetectedDevice: Status %X\n", Status);
        ZwClose(EnumKeyHandle);
        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();
        ASSERT(FALSE); // IoDbgBreakPointEx();// IoDeleteDevice(*DeviceObject);
        return Status;
    }

    InstancePath.Buffer[InstancePath.Length / sizeof(WCHAR)] = '\\';
    InstancePath.Length += sizeof(WCHAR);
    Length += sizeof(WCHAR);

    DPRINT("Instance %wZ\n", &InstancePath);

    if (Disposition != REG_CREATED_NEW_KEY)
    {
        BOOLEAN MatchingKey = FALSE;
        UNICODE_STRING KeyName;
        PWCHAR endBuffer;
        ULONG size;
        LONG len;
        ULONG ix;

        for (ix = 0; ; ix++)
        {
            InstancePath.Length = Length;
            RtlStringCchPrintfExW(Buffer + (Length / sizeof(WCHAR)),
                                  (400 - Length) / sizeof(WCHAR),
                                  &endBuffer,
                                  0,
                                  0,
                                  L"%04u",
                                  ix);
            DPRINT("IoReportDetectedDevice: Buffer '%S'\n", Buffer);

            len = &endBuffer[-InstancePath.Length / sizeof(WCHAR)] - Buffer;

            if (len != -1)
                size = (len * sizeof(WCHAR));
            else
                size = (400 - InstancePath.Length);

            KeyName.MaximumLength = 400 - InstancePath.Length;
            KeyName.Buffer = (PWCHAR)((ULONG_PTR)Buffer + InstancePath.Length);
            KeyName.Length = size;
            DPRINT("IoReportDetectedDevice: KeyName '%wZ'\n", &KeyName);

            InstancePath.Length += size;

            Status = IopCreateRegistryKeyEx(&KeyHandle,
                                            Handle,
                                            &KeyName,
                                            KEY_ALL_ACCESS,
                                            REG_OPTION_NON_VOLATILE,
                                            &Disposition);
            if (!NT_SUCCESS(Status))
            {
                ZwClose(Handle);
                ZwClose(EnumKeyHandle);
                ExReleaseResourceLite(&PpRegistryDeviceResource);
                KeLeaveCriticalRegion();
                ASSERT(FALSE); // IoDbgBreakPointEx();// IoDeleteDevice(*DeviceObject);
                return Status;
            }

            if (Disposition != 1)
            {
                PKEY_VALUE_FULL_INFORMATION FullKeyValue = NULL;
                BOOLEAN IsMigrated = FALSE;
                NTSTATUS status;

                status = IopGetRegistryValue(KeyHandle, L"Migrated", &FullKeyValue);
                if (NT_SUCCESS(status))
                {
                    if (FullKeyValue->Type == 4 &&
                        FullKeyValue->DataLength == sizeof(ULONG) &&
                        *(PULONG)((ULONG_PTR)FullKeyValue + FullKeyValue->DataOffset))
                    {
                        IsMigrated = TRUE;
                    }

                    ExFreePoolWithTag(FullKeyValue, 0);

                    DPRINT("IoReportDetectedDevice: Delete <Migrated> value key\n");
                    ZwDeleteValueKey(KeyHandle, &MigratedName);
                }

                if (IopIsReportedAlready(KeyHandle, ServiceKeyName, ResourceList, &MatchingKey))
                {
                    DPRINT1("IoReportDetectedDevice: IsReportedAlready - TRUE\n");
                    break;
                }

                DPRINT("MatchingKey %X, IsMigrated %X\n", MatchingKey, IsMigrated);

                if (!MatchingKey || !IsMigrated)
                {
                    ZwClose(KeyHandle);
                    continue;
                }

                Disposition = 1;
            }

            ZwClose(Handle);
            ASSERT(Disposition == REG_CREATED_NEW_KEY);
            goto Label0;
        }

        ASSERT(MatchingKey == TRUE);

        Status = IopCreateRegistryKeyEx(&Handle,
                                        KeyHandle,
                                        &LogConfName,
                                        KEY_ALL_ACCESS,
                                        REG_OPTION_NON_VOLATILE,
                                        NULL);
        DPRINT("IoReportDetectedDevice: Status %X\n", Status);

        if (NT_SUCCESS(Status))
        {
            if (ResourceList)
            {
                ULONG NumberOfBytes;
                NumberOfBytes = PnpDetermineResourceListSize(ResourceList);
                ZwSetValueKey(Handle, &BootConfigName, 0, 8, ResourceList, NumberOfBytes);
                DPRINT1("CmResource %X Size %X\n", ResourceList, NumberOfBytes);
                PipDumpCmResourceList(ResourceList, 1);
                ASSERT(FALSE); // IoDbgBreakPointEx();
            }

            if (ResourceRequirements)
            {
                ZwSetValueKey(Handle, &BasicConfigVectorName, 0, 10, ResourceRequirements, ResourceRequirements->ListSize);
                DPRINT1("IoResource %X Size %X\n", ResourceRequirements, ResourceRequirements->ListSize);
                PipDumpResourceRequirementsList(ResourceRequirements, 1);
                ASSERT(FALSE); // IoDbgBreakPointEx();
            }

            ZwClose(Handle);
        }

        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();

        IoDeleteDevice(Pdo);
        ZwClose(Handle);

        Pdo = IopDeviceObjectFromDeviceInstance(&InstancePath);
        DPRINT("IoReportDetectedDevice: Pdo %X\n", Pdo);

        ZwClose(KeyHandle);
        ZwClose(EnumKeyHandle);

        if (Pdo == NULL)
        {
            DPRINT1("IoReportDetectedDevice: ASSERT\n");
            ASSERT(Pdo);
            ASSERT(FALSE); // IoDbgBreakPointEx();
            return STATUS_UNSUCCESSFUL;
        }

        DeviceNode = IopGetDeviceNode(Pdo);

        if (PipAreDriversLoaded(DeviceNode))
        {
            ASSERT(FALSE); // IoDbgBreakPointEx();
            ObDereferenceObject(Pdo);
            return STATUS_NO_SUCH_DEVICE;
        }

        if ((DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM)) &&
            DeviceNode->Problem != CM_PROB_NOT_CONFIGURED &&
            DeviceNode->Problem != CM_PROB_REINSTALL &&
            DeviceNode->Problem != CM_PROB_FAILED_INSTALL)
        {
            ASSERT(FALSE); // IoDbgBreakPointEx();
            ObDereferenceObject(Pdo);
            return STATUS_NO_SUCH_DEVICE;
        }

        goto ResourcesAssign;
    }
    else
    {
        RtlStringCbPrintfExW(Buffer + (InstancePath.Length / sizeof(WCHAR)),
                             sizeof(Buffer) - InstancePath.Length,
                             &EndBuffer,
                             NULL,
                             0,
                             L"%04u",
                             0);

        RtlInitUnicodeString(&EnumSubKeyName, Buffer + (InstancePath.Length / sizeof(WCHAR)));
        InstancePath.Length = (ULONG)(EndBuffer - Buffer) * sizeof(WCHAR);

        DPRINT("IoReportDetectedDevice: EnumSubKeyName %wZ\n", &EnumSubKeyName);

        Status = IopCreateRegistryKeyEx(&KeyHandle,
                                        Handle,
                                        &EnumSubKeyName,
                                        KEY_ALL_ACCESS,
                                        REG_OPTION_NON_VOLATILE,
                                        &Disposition);
        DPRINT("IoReportDetectedDevice: Disposition %d\n", Disposition);

        ZwClose(Handle);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoReportDetectedDevice: Status %X\n", Status);
            ZwClose(EnumKeyHandle);
            ExReleaseResourceLite(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();
            ASSERT(FALSE); // IoDbgBreakPointEx();// IoDeleteDevice(*DeviceObject);
            return Status;
        }

        ASSERT(Disposition == REG_CREATED_NEW_KEY);

Label0:
        IsNeedCleanup = TRUE;

        if (ResourceAssigned)
        {
            Data = 1;
            ZwSetValueKey(KeyHandle, &NoResourceAtInitTimeName, 0, REG_DWORD, &Data, 4);
        }

        Handle = NULL;
        Status = IopCreateRegistryKeyEx(&Handle,
                                        KeyHandle,
                                        &LogConfName,
                                        KEY_ALL_ACCESS,
                                        REG_OPTION_NON_VOLATILE,
                                        NULL);
        ASSERT(Status == STATUS_SUCCESS);

        if (NT_SUCCESS(Status))
        {
            if (ResourceList)
            {
                ResourceListSize = PnpDetermineResourceListSize(ResourceList);
                ZwSetValueKey(Handle, &BootConfigName, 0, REG_RESOURCE_LIST, ResourceList, ResourceListSize);
            }

            if (ResourceRequirements)
            {
                ZwSetValueKey(Handle,
                              &BasicConfigVectorName,
                              0,
                              REG_RESOURCE_REQUIREMENTS_LIST,
                              ResourceRequirements,
                              ResourceRequirements->ListSize);
            }
        }

        Data = 0x400;
        ZwSetValueKey(KeyHandle, &ConfigFlagsName, 0, REG_DWORD, &Data, 4);

        Data = 0;
        ZwSetValueKey(KeyHandle, &LegacyName, 0, REG_DWORD, &Data, 4);

        ControlKeyHandle = NULL;
        IopCreateRegistryKeyEx(&ControlKeyHandle,
                               KeyHandle,
                               &ControlName,
                               KEY_ALL_ACCESS,
                               REG_OPTION_VOLATILE,
                               NULL);
        ASSERT(Status == STATUS_SUCCESS);

        Data = 1;
        ZwSetValueKey(ControlKeyHandle, &DeviceReportedName, 0, REG_DWORD, &Data, 4);
        Status = ZwSetValueKey(KeyHandle, &DeviceReportedName, 0, REG_DWORD, &Data, 4);

        ZwClose(EnumKeyHandle);

        ServiceKeyString = ExAllocatePoolWithTag(PagedPool, ServiceKeyName->Length + sizeof(WCHAR), '  pP');
        if (!ServiceKeyString) {
            DPRINT1("Allocate failed\n");
            ExReleaseResourceLite(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();
            goto ErrorExit;
        }

        RtlMoveMemory(ServiceKeyString, ServiceKeyName->Buffer, ServiceKeyName->Length);
        ServiceKeyString[ServiceKeyName->Length / sizeof(WCHAR)] = UNICODE_NULL;

        ZwSetValueKey(KeyHandle,
                      &ServiceName,
                      0,
                      REG_SZ,
                      ServiceKeyString,
                      (ServiceKeyName->Length + sizeof(WCHAR)));

        if (DriverObject->Flags & DRVO_BUILTIN_DRIVER)
        {
            //DeviceNode->ServiceName = *ServiceKeyName;
            //RtlCopyUnicodeString(&DeviceNode->ServiceName, ServiceKeyName);
            DeviceNode->ServiceName.Length = ServiceKeyName->Length;
            DeviceNode->ServiceName.MaximumLength = ServiceKeyName->MaximumLength;
            DeviceNode->ServiceName.Buffer = ServiceKeyName->Buffer;
            DPRINT("ServiceKey %wZ\n", &DeviceNode->ServiceName);
        }
        else
        {
            ExFreePoolWithTag(ServiceKeyString, 0);
        }

        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();

        if (!(DriverObject->Flags & DRVO_BUILTIN_DRIVER))
            PpDeviceRegistration(&InstancePath, 1, &DeviceNode->ServiceName);

        Status = PnpConcatenateUnicodeStrings(&DeviceNode->InstancePath, &InstancePath, NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IoReportDetectedDevice: Status %X\n", Status);
            DeviceNode->InstancePath.Length = 0;
            DeviceNode->InstancePath.MaximumLength = 0;
            DPRINT("IoReportDetectedDevice: error\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();// IoDeleteDevice(Pdo);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        DPRINT("Instance '%wZ'\n", &DeviceNode->InstancePath);

        DeviceNode->Flags = (DNF_MADEUP + DNF_ENUMERATED);
        PipSetDevNodeState(DeviceNode, DeviceNodeInitialized, NULL);
        PpDevNodeInsertIntoTree(IopRootDeviceNode, DeviceNode);

        Status = IopMapDeviceObjectToDeviceInstance(Pdo, &DeviceNode->InstancePath);
        ASSERT(Status == STATUS_SUCCESS);
        ObReferenceObject(Pdo);

        //IopNotifySetupDeviceArrival(Pdo, 0, 0);

   /* Report the device's enumeration to umpnpmgr */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ENUMERATED, &DeviceNode->InstancePath);
    /* Report the device's arrival to umpnpmgr */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL, &DeviceNode->InstancePath);
    DPRINT1("Reported device '%wZ'\n", &DeviceNode->InstancePath);

ResourcesAssign:

        DPRINT("ResourcesAssign: Status %X\n", Status);

        if (ResourceAssigned)
        {
            DeviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;

            if (ResourceList)
            {
                ResourceListSize = PnpDetermineResourceListSize(ResourceList);
                IopWriteAllocatedResourcesToRegistry(DeviceNode, ResourceList, ResourceListSize);
            }
        }
        else if (ResourceList &&
                 ResourceList->Count &&
                 ResourceList->List[0].PartialResourceList.Count)
        {
            PCM_RESOURCE_LIST IntCmResource;
            BOOLEAN ConflictDetected;

            DPRINT1("IoReportDetectedDevice: ResourceAssigned is FALSE\n");

            if (!ResourceListSize)
                ResourceListSize = PnpDetermineResourceListSize(ResourceList);

            IntCmResource = ExAllocatePoolWithTag(PagedPool, ResourceListSize, 'oipP');
            if (!IntCmResource)
            {
                DPRINT1("IoReportDetectedDevice: STATUS_INSUFFICIENT_RESOURCES\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                PipSetDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY);
                goto ErrorExit;
            }

            RtlCopyMemory(IntCmResource, ResourceList, ResourceListSize);

            Status = IoReportResourceUsageInternal(ArbiterRequestLegacyReported,
                                                   &PnP_Manager,
                                                   Pdo->DriverObject,
                                                   NULL,
                                                   0,
                                                   DeviceNode->PhysicalDeviceObject,
                                                   IntCmResource,
                                                   ResourceListSize,
                                                   FALSE,
                                                   &ConflictDetected);
            ExFreePoolWithTag(IntCmResource, 0);

            if (!NT_SUCCESS(Status) || ConflictDetected)
            {
                DPRINT1("IoReportDetectedDevice: STATUS_CONFLICTING_ADDRESSES\n");
                Status = STATUS_CONFLICTING_ADDRESSES;
                PipSetDevNodeProblem(DeviceNode, CM_PROB_NORMAL_CONFLICT);
            }
        }
        else
        {
            if (DriverObject)
                ASSERT(ResourceRequirements == NULL);

            DeviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
        }

        if (NT_SUCCESS(Status))
        {
            IopDoDeferredSetInterfaceState(DeviceNode);
            PipSetDevNodeState(DeviceNode, DeviceNodeStartPostWork, NULL);

            *DeviceObject = Pdo;

            if (IsNeedCleanup)
            {
                if (ControlKeyHandle)
                    ZwClose(ControlKeyHandle);

                if (Handle)
                    ZwClose(Handle);

                ZwClose(KeyHandle);
            }

            PipRequestDeviceAction(Pdo, PipEnumDeviceOnly, 0, 0, NULL, NULL);
            return Status;
        }
        else
        {
            DPRINT1("IoReportDetectedDevice: Status %X\n", Status);
            ASSERT(FALSE); // IoDbgBreakPointEx();
        }

ErrorExit:

        IopReleaseDeviceResources(DeviceNode, FALSE);

        if (IsNeedCleanup)
        {
            IoDeleteDevice(Pdo);

            if (ControlKeyHandle)
                ZwDeleteKey(ControlKeyHandle);

            if (Handle)
                ZwDeleteKey(Handle);

            if (KeyHandle)
                ZwDeleteKey(KeyHandle);
        }

        DPRINT1("IoReportDetectedDevice: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    return STATUS_SUCCESS;
}

/*
 * @halfplemented
 */
NTSTATUS
NTAPI
IoReportResourceForDetection(IN PDRIVER_OBJECT DriverObject,
                             IN PCM_RESOURCE_LIST DriverList OPTIONAL,
                             IN ULONG DriverListSize OPTIONAL,
                             IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                             IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
                             IN ULONG DeviceListSize OPTIONAL,
                             OUT PBOOLEAN ConflictDetected)
{
    PCM_RESOURCE_LIST ResourceList;
    NTSTATUS Status;

    *ConflictDetected = FALSE;

    if (!DriverList && !DeviceList)
        return STATUS_INVALID_PARAMETER;

    /* Find the real list */
    if (!DriverList)
        ResourceList = DeviceList;
    else
        ResourceList = DriverList;

    /* Look for a resource conflict */
    Status = IopDetectResourceConflict(ResourceList, FALSE, NULL);
    if (Status == STATUS_CONFLICTING_ADDRESSES)
    {
        /* Oh noes */
        *ConflictDetected = TRUE;
    }
    else if (NT_SUCCESS(Status))
    {
        /* Looks like we're good to go */

        /* TODO: Claim the resources in the ResourceMap */
    }

    return Status;
}

VOID
NTAPI
IopSetEvent(IN PVOID Context)
{
    PKEVENT Event = Context;

    /* Set the event */
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReportTargetDeviceChange(IN PDEVICE_OBJECT PhysicalDeviceObject,
                           IN PVOID NotificationStructure)
{
    KEVENT NotifyEvent;
    NTSTATUS Status, NotifyStatus;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

    ASSERT(notifyStruct);

    /* Check for valid PDO */
    if (!IopIsValidPhysicalDeviceObject(PhysicalDeviceObject))
    {
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG_PTR)PhysicalDeviceObject, 0, 0);
    }

    /* FileObject must be null. PnP will fill in it */
    ASSERT(notifyStruct->FileObject == NULL);

    /* Do not handle system PnP events */
    if ((RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_QUERY_REMOVE), sizeof(GUID)) != sizeof(GUID)) ||
        (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_CANCELLED), sizeof(GUID)) != sizeof(GUID)) ||
        (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_COMPLETE), sizeof(GUID)) != sizeof(GUID)))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (notifyStruct->Version != 1)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Initialize even that will let us know when PnP will have finished notify */
    KeInitializeEvent(&NotifyEvent, NotificationEvent, FALSE);

    Status = PpSetCustomTargetEvent(PhysicalDeviceObject, &NotifyEvent, &NotifyStatus, NULL, NULL, notifyStruct);
    /* If no error, wait for the notify to end and return the status of the notify and not of the event */
    if (NT_SUCCESS(Status))
    {
        KeWaitForSingleObject(&NotifyEvent, Executive, KernelMode, FALSE, NULL);
        Status = NotifyStatus;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReportTargetDeviceChangeAsynchronous(IN PDEVICE_OBJECT PhysicalDeviceObject,
                                       IN PVOID NotificationStructure,
                                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback OPTIONAL,
                                       IN PVOID Context OPTIONAL)
{
    PINTERNAL_WORK_QUEUE_ITEM Item = NULL;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

    ASSERT(notifyStruct);

    /* Check for valid PDO */
    if (!IopIsValidPhysicalDeviceObject(PhysicalDeviceObject))
    {
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG_PTR)PhysicalDeviceObject, 0, 0);
    }

    /* FileObject must be null. PnP will fill in it */
    ASSERT(notifyStruct->FileObject == NULL);

    /* Do not handle system PnP events */
    if ((RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_QUERY_REMOVE), sizeof(GUID)) != sizeof(GUID)) ||
        (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_CANCELLED), sizeof(GUID)) != sizeof(GUID)) ||
        (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_COMPLETE), sizeof(GUID)) != sizeof(GUID)))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (notifyStruct->Version != 1)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* We need to store all the data given by the caller with the WorkItem, so use our own struct */
    Item = ExAllocatePoolWithTag(NonPagedPool, sizeof(INTERNAL_WORK_QUEUE_ITEM), '  pP');
    if (!Item) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize all stuff */
    ObReferenceObject(PhysicalDeviceObject);
    Item->NotificationStructure = notifyStruct;
    Item->PhysicalDeviceObject = PhysicalDeviceObject;
    Item->Callback = Callback;
    Item->Context = Context;
    ExInitializeWorkItem(&(Item->WorkItem), IopReportTargetDeviceChangeAsyncWorker, Item);

    /* Finally, queue the item, our work here is done */
    ExQueueWorkItem(&(Item->WorkItem), DelayedWorkQueue);

    return STATUS_PENDING;
}
