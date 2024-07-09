/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnproot.c
 * PURPOSE:         PnP manager root device
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Copyright 2007 Herv? Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define ENUM_NAME_ROOT L"Root"

extern ERESOURCE PpRegistryDeviceResource;

extern PNP_ALLOCATE_RESOURCES_ROUTINE IopAllocateBootResourcesRoutine;

extern ARBITER_INSTANCE IopRootBusNumberArbiter;
extern ARBITER_INSTANCE IopRootIrqArbiter;
extern ARBITER_INSTANCE IopRootDmaArbiter;
extern ARBITER_INSTANCE IopRootMemArbiter;
extern ARBITER_INSTANCE IopRootPortArbiter;

/* DATA **********************************************************************/

typedef struct _PNPROOT_DEVICE
{
    // Entry on device list
    LIST_ENTRY ListEntry;
    // Physical Device Object of device
    PDEVICE_OBJECT Pdo;
    // Device ID
    UNICODE_STRING DeviceID;
    // Instance ID
    UNICODE_STRING InstanceID;
    // Device description
    UNICODE_STRING DeviceDescription;
    // Resource requirement list
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList;
    // Associated resource list
    PCM_RESOURCE_LIST ResourceList;
    ULONG ResourceListSize;
} PNPROOT_DEVICE, *PPNPROOT_DEVICE;

/* Physical Device Object device extension for a child device */
typedef struct _PNPROOT_PDO_DEVICE_EXTENSION
{
    // Informations about the device
    PPNPROOT_DEVICE DeviceInfo;
} PNPROOT_PDO_DEVICE_EXTENSION, *PPNPROOT_PDO_DEVICE_EXTENSION;

typedef struct _PNP_ROOT_RELATIONS_CONTEXT {
    NTSTATUS Status;
    PUNICODE_STRING RootEnumString;
    ULONG MaxDevices;
    ULONG Count;
    PVOID Objects;
} PNP_ROOT_RELATIONS_CONTEXT, *PPNP_ROOT_RELATIONS_CONTEXT;

#define PNP_MAX_ROOT_DEVICES 256

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
IopGetServiceType(
    _In_ PUNICODE_STRING ServiceName,
    _Out_ PULONG OutServiceType)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    HANDLE Handle;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopGetServiceType: ServiceName '%wZ'\n", ServiceName);

    *OutServiceType = -1;

    Status = PipOpenServiceEnumKeys(ServiceName, KEY_READ, &Handle, NULL, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IopGetRegistryValue(Handle, L"Type", &ValueInfo);

    if (!NT_SUCCESS(Status))
    {
        ZwClose(Handle);
        return Status;
    }

    if (ValueInfo->Type == REG_DWORD &&
        ValueInfo->DataLength >= sizeof(ULONG))
    {
        *OutServiceType = *(PULONG)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
    }

    ExFreePoolWithTag(ValueInfo, 'uspP');
    ZwClose(Handle);

    return Status;
}

BOOLEAN
NTAPI
PipIsFirmwareMapperDevicePresent(
    _In_ HANDLE KeyHandle)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    UNICODE_STRING ControlName;
    ULONG FirmwareIdentified = 0;
    ULONG FirmwareMember;
    HANDLE Handle;
    NTSTATUS Status;
    BOOLEAN Result;

    PAGED_CODE();
    DPRINT("PipIsFirmwareMapperDevicePresent()\n");

    Status = IopGetRegistryValue(KeyHandle, L"FirmwareIdentified", &ValueInfo);
    if (!NT_SUCCESS(Status))
        return TRUE;

    if (ValueInfo->Type == REG_DWORD &&
        ValueInfo->DataLength == sizeof(ULONG))
    {
        FirmwareIdentified = *(PULONG)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
    }

    ExFreePoolWithTag(ValueInfo, 'uspP');

    if (!FirmwareIdentified)
        return TRUE;

    RtlInitUnicodeString(&ControlName, L"Control");

    Status = IopOpenRegistryKeyEx(&Handle, KeyHandle, &ControlName, KEY_READ);
    if (!NT_SUCCESS(Status))
        return FALSE;

    Status = IopGetRegistryValue(Handle, L"FirmwareMember", &ValueInfo);
    ZwClose(Handle);

    if (!NT_SUCCESS(Status))
        return FALSE;

    FirmwareMember = 0;

    if (ValueInfo->Type == REG_DWORD &&
        ValueInfo->DataLength == sizeof(ULONG))
    {
        FirmwareMember = *(PULONG)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
    }

    ExFreePoolWithTag(ValueInfo, 'uspP');

    if (!FirmwareMember)
        Result = FALSE;
    else
        Result = TRUE;

    return Result;
}

BOOLEAN
NTAPI 
IopInitializeDeviceInstanceKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING KeyName,
    _In_ PVOID Context)
{
    PPNP_ROOT_RELATIONS_CONTEXT RelationContext = Context;
    PEXTENDED_DEVOBJ_EXTENSION DeviceObjectExtension;
    PKEY_VALUE_FULL_INFORMATION ServiceValueInfo;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT * pDevices;
    PDEVICE_NODE DeviceNode = NULL;
    PCM_RESOURCE_LIST CmResouce;
    PUNICODE_STRING EnumString;
    UNICODE_STRING Name;
    DEVICE_CAPABILITIES_FLAGS CapFlags;
    ULONG Legacy;
    ULONG Problem;
    ULONG ConfigFlags;
    ULONG ServiceType;
    NTSTATUS Status;
    BOOLEAN DuplicateOf = FALSE;

    PAGED_CODE();
    DPRINT("IopInitializeDeviceInstanceKey: KeyHandle - %p, KeyName - %wZ, RelationContext->Objects - %p\n",
           KeyHandle, KeyName, RelationContext->Objects);

    Status = IopGetRegistryValue(KeyHandle, L"Phantom", &ValueInfo);

    if (NT_SUCCESS(Status))
    {
        ULONG Phantom;

        if ((ValueInfo->Type == REG_DWORD) &&
            (ValueInfo->DataLength >= sizeof(ULONG)))
        {
            Phantom = *(PULONG)((ULONG_PTR)ValueInfo +
                                ValueInfo->DataOffset);
        }
        else
        {
            Phantom = 0;
        }

        ExFreePoolWithTag(ValueInfo, 'uspP');

        if (Phantom)
        {
            DPRINT("IopInitializeDeviceInstanceKey: return TRUE\n");
            return TRUE;
        }
    }

    if (RelationContext->Count == RelationContext->MaxDevices)
    {
        PVOID NewRelationContext;
        ULONG NewSize;

        NewSize = (PNP_MAX_ROOT_DEVICES +
                   RelationContext->Count) * sizeof(PDEVICE_OBJECT);

        NewRelationContext = ExAllocatePoolWithTag(PagedPool, NewSize, 'ddpP');

        if (!NewRelationContext)
        {
            RelationContext->Status = STATUS_INSUFFICIENT_RESOURCES;
            DPRINT("IopInitializeDeviceInstanceKey: STATUS_INSUFFICIENT_RESOURCES, return TRUE\n");
            return FALSE;
        }

        RtlCopyMemory(NewRelationContext,
                      RelationContext->Objects,
                      RelationContext->Count * sizeof(PDEVICE_OBJECT));

        ExFreePoolWithTag(RelationContext->Objects, 'ddpP');

        RelationContext->Objects = NewRelationContext;
        RelationContext->MaxDevices = NewSize / sizeof(PDEVICE_OBJECT);
    }

    EnumString = RelationContext->RootEnumString;

    if (EnumString->Buffer[EnumString->Length / sizeof(WCHAR) - 1] != '\\')
    {
        EnumString->Buffer[EnumString->Length / sizeof(WCHAR)] = '\\';
        EnumString->Length += sizeof(WCHAR);
    }

    RtlAppendUnicodeStringToString(EnumString, KeyName);
    DeviceObject = IopDeviceObjectFromDeviceInstance(EnumString);

    if (DeviceObject)
    {
        pDevices = RelationContext->Objects;
        pDevices[RelationContext->Count] = DeviceObject;
        RelationContext->Count++;
        DPRINT("IopInitializeDeviceInstanceKey: return TRUE\n");
        return TRUE;
    }

    if (!PipIsFirmwareMapperDevicePresent(KeyHandle))
    {
        DPRINT("IopInitializeDeviceInstanceKey: return TRUE\n");
        return TRUE;
    }

    Status = IopGetRegistryValue(KeyHandle,
                                 L"DuplicateOf",
                                 &ValueInfo);

    if (NT_SUCCESS(Status))
    {
        if (ValueInfo->Type == REG_SZ &&
            ValueInfo->DataLength > 0)
        {
            DuplicateOf = TRUE;
        }

        ExFreePoolWithTag(ValueInfo, 'uspP');
    }

    ServiceValueInfo = NULL;
    RtlZeroMemory(&Name, sizeof(Name));

    Status = IopGetRegistryValue(KeyHandle, L"Service", &ServiceValueInfo);

    if (NT_SUCCESS(Status) &&
        ServiceValueInfo->Type == REG_SZ &&
        ServiceValueInfo->DataLength)
    {
        PWCHAR Buffer;

        Buffer = (PWCHAR)((ULONG_PTR)ServiceValueInfo +
                          ServiceValueInfo->DataOffset);

        PnpRegSzToString(Buffer,
                         ServiceValueInfo->DataLength,
                         &Name.Length);

        Name.MaximumLength = (USHORT)ServiceValueInfo->DataLength;
        Name.Buffer = Buffer;
    }

    Status = IopGetDeviceInstanceCsConfigFlags(EnumString, &ConfigFlags);

    if (NT_SUCCESS(Status) && ConfigFlags & 2) // ?
    {
        ExFreePoolWithTag(ServiceValueInfo, 'uspP');
        DPRINT("IopInitializeDeviceInstanceKey: return TRUE\n");
        return TRUE;
    }

    Legacy = 0;

    Status = IopGetRegistryValue(KeyHandle, L"Legacy", &ValueInfo);

    if (NT_SUCCESS(Status))
    {
        if (ValueInfo->Type == REG_DWORD &&
            ValueInfo->DataLength >= sizeof(ULONG))
        {
            Legacy = *(PULONG)((ULONG_PTR)ValueInfo +
                               ValueInfo->DataOffset);
        }

        ExFreePoolWithTag(ValueInfo, 'uspP');

        if (Legacy)
        {
            Status = IopGetServiceType(&Name, &ServiceType);

            if (Name.Length == 0 ||
                !NT_SUCCESS(Status) ||
                ServiceType != SERVICE_KERNEL_DRIVER)
            {
                PpDeviceRegistration(EnumString, TRUE, NULL);

                if (ServiceValueInfo)
                {
                    ExFreePoolWithTag(ServiceValueInfo, 'uspP');
                }

                DPRINT("IopInitializeDeviceInstanceKey: return TRUE\n");
                return TRUE;
            }
        }
    }

    if (ServiceValueInfo)
    {
        ExFreePoolWithTag(ServiceValueInfo, 'uspP');
    }

    Status = IoCreateDevice(IopRootDriverObject,
                            sizeof(IOPNP_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitializeDeviceInstanceKey: Status - %X\n", Status);
        goto Exit;
    }

    DeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;

    DeviceObjectExtension = IoGetDevObjExtension(DeviceObject);
    DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;

    DeviceNode = PipAllocateDeviceNode(DeviceObject);

    if (DeviceNode == NULL) //|| PpSystemHiveTooLarge)
    {
        IoDeleteDevice(DeviceObject);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DeviceObject = NULL;
        DPRINT("IopInitializeDeviceInstanceKey: DeviceNode == NULL\n");
        goto Exit;
    }

    Status = PnpConcatenateUnicodeStrings(&DeviceNode->InstancePath,
                                          EnumString,
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
        DeviceObject = NULL;
        DPRINT("IopInitializeDeviceInstanceKey: Status - %X\n", Status);
        goto Exit;
    }

    DeviceNode->Flags = (DNF_MADEUP | DNF_ENUMERATED);
    PipSetDevNodeState(DeviceNode, DeviceNodeInitialized, NULL);
    PpDevNodeInsertIntoTree(IopRootDeviceNode, DeviceNode);

    if (Legacy)
    {
        DeviceNode->Flags |= (DNF_LEGACY_DRIVER | DNF_NO_RESOURCE_REQUIRED);
        PipSetDevNodeState(DeviceNode, DeviceNodeStarted, NULL);
    }
    else
    {
        ConfigFlags = 0;

        Status = IopGetRegistryValue(KeyHandle,
                                     L"ConfigFlags",
                                     &ValueInfo);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IopInitializeDeviceInstanceKey: Status - %X\n", Status);

            if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
                Status == STATUS_OBJECT_PATH_NOT_FOUND)
            {
                PipSetDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED);
            }
        }
        else
        {
            if (ValueInfo->Type == REG_DWORD &&
                ValueInfo->DataLength >= sizeof(ULONG))
            {
                ConfigFlags = *(PULONG)((ULONG_PTR)ValueInfo +
                                        ValueInfo->DataOffset);
            }

            ExFreePoolWithTag(ValueInfo, 'uspP');

            if (ConfigFlags & 0x20) // ?
            {
                DPRINT("IopInitializeDeviceInstanceKey: CM_PROB_REINSTALL\n");
                PipSetDevNodeProblem(DeviceNode, CM_PROB_REINSTALL);
            }
            else if (ConfigFlags & 0x2000) // ?
            {
                DPRINT("IopInitializeDeviceInstanceKey: CM_PROB_PARTIAL_LOG_CONF\n");
                PipSetDevNodeProblem(DeviceNode, CM_PROB_PARTIAL_LOG_CONF);
            }
            else if (ConfigFlags & 0x40) // ?
            {
                DPRINT("IopInitializeDeviceInstanceKey: CM_PROB_FAILED_INSTALL\n");
                PipSetDevNodeProblem(DeviceNode, CM_PROB_FAILED_INSTALL);
            }
        }
    }

    if (DuplicateOf)
    {
        DeviceNode->Flags |= DNF_DUPLICATE;
        DPRINT("IopInitializeDeviceInstanceKey: DNF_DUPLICATE\n");
    }

    Status = IopGetRegistryValue(KeyHandle,
                                 L"NoResourceAtInitTime",
                                 &ValueInfo);
    if (NT_SUCCESS(Status))
    {
        if (ValueInfo->Type == REG_DWORD &&
            ValueInfo->DataLength >= sizeof(ULONG))
        {
            if (*(PULONG)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset))
            {
                DeviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
            }
        }
        ExFreePoolWithTag(ValueInfo, 'uspP');
    }

    IopQueryAndSaveDeviceNodeCapabilities(DeviceNode);

    CapFlags.AsULONG = DeviceNode->CapabilityFlags;

    if ((CapFlags.HardwareDisabled) &&
        (!(DeviceNode->Flags & DNF_HAS_PROBLEM) ||
         DeviceNode->Problem != CM_PROB_NOT_CONFIGURED))
    {
        PipClearDevNodeProblem(DeviceNode);
        PipSetDevNodeProblem(DeviceNode, CM_PROB_HARDWARE_DISABLED);
        DPRINT("IopInitializeDeviceInstanceKey: CM_PROB_HARDWARE_DISABLED\n");
    }


    if (DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM) &&
       !CapFlags.HardwareDisabled)
    {
        PpCriticalProcessCriticalDevice(DeviceNode);
    }

    if (DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM))
    {
        Problem = DeviceNode->Flags & DNF_HAS_PROBLEM;
        if ((!Problem || DeviceNode->Problem != CM_PROB_NOT_CONFIGURED) && 
            (!Problem || DeviceNode->Problem != CM_PROB_REINSTALL) &&
            (!Problem || DeviceNode->Problem != CM_PROB_FAILED_INSTALL) &&
            (!Problem || DeviceNode->Problem != CM_PROB_HARDWARE_DISABLED) &&
            (!Problem || DeviceNode->Problem != CM_PROB_PARTIAL_LOG_CONF))
        {
            DPRINT("IopInitializeDeviceInstanceKey: DeviceNode->Flags - %X, DeviceNode->Problem - %X\n",
                   DeviceNode->Flags, DeviceNode->Problem);
            ASSERT(FALSE);
        }
    }

    if ((!(DeviceNode->Flags & DNF_HAS_PROBLEM) ||
         DeviceNode->Problem != CM_PROB_DISABLED) &&
        (!(DeviceNode->Flags & DNF_HAS_PROBLEM) ||
         DeviceNode->Problem != CM_PROB_HARDWARE_DISABLED))
    {
        if (!IopIsDeviceInstanceEnabled(KeyHandle,
                                        &DeviceNode->InstancePath,
                                        TRUE))
        {
            PipClearDevNodeProblem(DeviceNode);
            PipSetDevNodeProblem(DeviceNode, CM_PROB_DISABLED);
        }
    }

    /* Report the device's enumeration to umpnpmgr */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ENUMERATED,
                              &DeviceNode->InstancePath);

    /* Report the device's arrival to umpnpmgr */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL,
                              &DeviceNode->InstancePath);

    Status = PpDeviceRegistration(&DeviceNode->InstancePath,
                                  TRUE,
                                  &DeviceNode->ServiceName);

    if (NT_SUCCESS(Status))
    {
        if ((DeviceNode->Flags & DNF_HAS_PROBLEM) &&
            (DeviceNode->Problem == CM_PROB_NOT_CONFIGURED))
        {
            PipClearDevNodeProblem(DeviceNode);
        }
    }

    Status = IopMapDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject,
                                                &DeviceNode->InstancePath);
    ASSERT(NT_SUCCESS(Status));
    ObReferenceObject(DeviceObject);

    CmResouce = NULL;

    Status = IopGetDeviceResourcesFromRegistry(DeviceObject,
                                               FALSE,
                                               PIP_CONFIG_TYPE_BOOT,
                                               (PVOID *)&CmResouce,
                                               &ServiceType);
    if (NT_SUCCESS(Status) && CmResouce)
    {
        Status = IopAllocateBootResourcesRoutine(ArbiterRequestPnpEnumerated,
                                                 DeviceNode->PhysicalDeviceObject,
                                                 CmResouce);
        if (NT_SUCCESS(Status))
        {
            DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
        }

        ExFreePool(CmResouce);
    }

    Status = STATUS_SUCCESS;
    ObReferenceObject(DeviceObject);

Exit:

    EnumString->Length = RelationContext->RootEnumString->Length;

    if (NT_SUCCESS(Status))
    {
        ASSERT(DeviceObject);

        pDevices = RelationContext->Objects;
        pDevices[RelationContext->Count] = DeviceObject;
        RelationContext->Count++;

        DPRINT("IopInitializeDeviceInstanceKey: [1] RelationContext->Objects - %p, RelationContext->Status - %X\n",
               RelationContext->Objects, RelationContext->Status);

        return TRUE;
    }

    RelationContext->Status = Status;

    DPRINT("IopInitializeDeviceInstanceKey: [0] RelationContext->Objects - %p, RelationContext->Status - %X\n",
           RelationContext->Objects, RelationContext->Status);

    return FALSE;
}

BOOLEAN
NTAPI 
IopInitializeDeviceKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING KeyName,
    _In_ PVOID Context)
{
    PPNP_ROOT_RELATIONS_CONTEXT RelationContext;
    PUNICODE_STRING EnumString;
    USHORT Length;
    NTSTATUS Status;

    RelationContext = Context;

    DPRINT("IopInitializeDeviceKey: '%wZ', Objects %p\n", KeyName, RelationContext->Objects);

    EnumString = RelationContext->RootEnumString;
    Length = EnumString->Length;

    if (Length / sizeof(WCHAR))
    {
        EnumString->Buffer[Length / sizeof(WCHAR)] = '\\';
        EnumString->Length += sizeof(WCHAR);
    }

    RtlAppendUnicodeStringToString(EnumString, KeyName);

    Status = PipApplyFunctionToSubKeys(KeyHandle,
                                       NULL,
                                       KEY_ALL_ACCESS,
                                       PIP_SUBKEY_FLAG_SKIP_ERROR,
                                       IopInitializeDeviceInstanceKey,
                                       RelationContext);

    DPRINT("IopInitializeDeviceKey: Objects %p, Status %X\n", RelationContext->Objects, RelationContext->Status);

    EnumString->Length = Length;

    if (!NT_SUCCESS(Status))
        RelationContext->Status = Status;

    return NT_SUCCESS(RelationContext->Status);
}

NTSTATUS
NTAPI
IopGetRootDevices(
    _Out_ ULONG_PTR * OutInformation)
{
    PDEVICE_RELATIONS Relations;
    PWCHAR Buffer;
    UNICODE_STRING RootName;
    UNICODE_STRING EnumString;
    HANDLE Handle;
    ULONG RelationsSize;
    ULONG ix;
    NTSTATUS Status;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\Root");
    PNP_ROOT_RELATIONS_CONTEXT RelationContext;
  
    PAGED_CODE();
    DPRINT("IopGetRootDevices: *OutInformation - %p\n", *OutInformation);

    *OutInformation = 0;

    Buffer = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, 'ddpP');
    if (!Buffer)
    {
        DPRINT1("IopGetRootDevices: return STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RelationContext.Objects = ExAllocatePoolWithTag(PagedPool,
                                                    PNP_MAX_ROOT_DEVICES * sizeof(PDEVICE_OBJECT),
                                                    'ddpP');
    if (!RelationContext.Objects)
    {
        ExFreePoolWithTag(Buffer, 'ddpP');
        DPRINT1("IopGetRootDevices: return STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RelationContext.Count = 0;
    RelationContext.MaxDevices = PNP_MAX_ROOT_DEVICES;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PpRegistryDeviceResource, TRUE);

    Status = IopCreateRegistryKeyEx(&Handle,
                                    NULL,
                                    &KeyName,
                                    KEY_READ,
                                    REG_OPTION_NON_VOLATILE,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopGetRootDevices: Status - %X\n", Status);
        ASSERT(FALSE);
        goto Exit;
    }

    EnumString.Length = 0;
    EnumString.MaximumLength = PAGE_SIZE;
    EnumString.Buffer = Buffer;
    RtlZeroMemory(EnumString.Buffer, PAGE_SIZE);

    RtlInitUnicodeString(&RootName, ENUM_NAME_ROOT);
    RtlAppendUnicodeStringToString(&EnumString, &RootName);

    RelationContext.RootEnumString = &EnumString;
    RelationContext.Status = STATUS_SUCCESS;

    PipApplyFunctionToSubKeys(Handle,
                              NULL,
                              KEY_ALL_ACCESS,
                              PIP_SUBKEY_FLAG_SKIP_ERROR,
                              IopInitializeDeviceKey,
                              &RelationContext);
    ZwClose(Handle);
    Status = RelationContext.Status;
    DPRINT("IopGetRootDevices: RelationContext.Count - %p, Status - %X\n", RelationContext.Count, Status);

    if (!RelationContext.Count)
    {
        DPRINT1("IopGetRootDevices: Status - STATUS_UNSUCCESSFUL\n");
        Status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopGetRootDevices: Status - %X\n", Status);
        goto ErrorExit;
    }

    RelationsSize = sizeof(DEVICE_RELATIONS) +
                    RelationContext.Count * sizeof(PDEVICE_OBJECT);

    Relations = ExAllocatePoolWithTag(PagedPool, RelationsSize, 'ddpP');
    if (!Relations)
    {
        DPRINT1("IopGetRootDevices: Status - STATUS_INSUFFICIENT_RESOURCES\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    Relations->Count = RelationContext.Count;

    RtlCopyMemory(Relations->Objects,
                  RelationContext.Objects,
                  RelationContext.Count * sizeof(PDEVICE_OBJECT));

    *OutInformation = (ULONG_PTR)Relations;

ErrorExit:

    if (NT_SUCCESS(Status))
    {
        goto Exit;
    }

    for (ix = 0; ix < RelationContext.Count; ix++)
    {
        DPRINT("IopGetRootDevices: Relations->Objects[%X] - %p\n",
               ix, Relations->Objects[ix]);
        ObDereferenceObject(Relations->Objects[ix]);
    }

Exit:

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    ExFreePoolWithTag(Buffer, 'ddpP');
    ExFreePoolWithTag(RelationContext.Objects, 'ddpP');

    DPRINT("IopGetRootDevices: return Status - %X\n", Status);
    return Status;
}

static NTSTATUS
PdoQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_RELATIONS Relations;
    DEVICE_RELATION_TYPE Type;
    ULONG_PTR Information = 0;
    NTSTATUS Status;

    Type = IrpSp->Parameters.QueryDeviceRelations.Type;

    if (DeviceObject == IopRootDeviceNode->PhysicalDeviceObject &&
        Type == BusRelations)
    {
        Status = IopGetRootDevices(&Information);
        Irp->IoStatus.Information = Information;
    }
    else if (Type == TargetDeviceRelation)
    {
        DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / TargetDeviceRelation\n");
        Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
        if (!Relations)
        {
            DPRINT("ExAllocatePoolWithTag() failed\n");
            Status = STATUS_NO_MEMORY;
        }
        else
        {
            ObReferenceObject(DeviceObject);
            Relations->Count = 1;
            Relations->Objects[0] = DeviceObject;
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR)Relations;
        }
    }
    else
    {
        Status = Irp->IoStatus.Status;
    }

    return Status;
}

BOOLEAN
NTAPI
IopIsFirmwareDisabled(
    _In_ PDEVICE_NODE DeviceNode)
{
    KEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    UNICODE_STRING ValueName;
    HANDLE Handle;
    HANDLE KeyHandle;
    ULONG ResultLength;
    NTSTATUS Status;
    BOOLEAN IsFirmwareDisabled = FALSE;

    DPRINT("IopIsFirmwareDisabled: DeviceNode - %p\n", DeviceNode);

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    Status = PnpDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject,
                                             &Handle,
                                             KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopIsFirmwareDisabled: Status - %p\n", Status);
        goto Exit;
    }

    RtlInitUnicodeString(&ValueName, L"Control");
    Status = IopCreateRegistryKeyEx(&KeyHandle,
                                    Handle,
                                    &ValueName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_VOLATILE,
                                    NULL);
    ZwClose(Handle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopIsFirmwareDisabled: Status - %p\n", Status);
        goto Exit;
    }

    ResultLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG);
    RtlInitUnicodeString(&ValueName, L"FirmwareDisabled");

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             &ValueInfo,
                             sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG),
                             &ResultLength);
    ZwClose(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopIsFirmwareDisabled: Status - %p\n", Status);
        goto Exit;
    }

    if (ValueInfo.Type == REG_DWORD &&
        ValueInfo.DataLength == sizeof(ULONG))
    {
        if (*(PULONG)ValueInfo.Data)
        {
            IsFirmwareDisabled = TRUE;
        }
    }

Exit:

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    return IsFirmwareDisabled;
}

NTSTATUS
NTAPI
PdoQueryCapabilities(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_CAPABILITIES DeviceCapabilities;
    PDEVICE_NODE DeviceNode;
    ULONG NumberOfBytes;
    ULONG UINumber;
    NTSTATUS Status;

    DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

    DPRINT("PdoQueryCapabilities: DeviceCapabilities - %p\n",
           DeviceCapabilities);

    DeviceCapabilities->Size = sizeof(*DeviceCapabilities);
    DeviceCapabilities->Version = 1;

    DeviceCapabilities->DeviceState[PowerSystemUnspecified] = PowerDeviceUnspecified;
    DeviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    DeviceCapabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
    DeviceCapabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
    DeviceCapabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
    DeviceCapabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    DeviceCapabilities->DeviceState[PowerSystemShutdown] = PowerDeviceD3;

    DeviceNode = IopGetDeviceNode(DeviceObject);

    if (IopIsFirmwareDisabled(DeviceNode))
    {
        DeviceCapabilities->HardwareDisabled = 1;
    }

    if (DeviceCapabilities->UINumber != -1)
    {
        return STATUS_SUCCESS;
    }

    NumberOfBytes = sizeof(UINumber);

    Status = PiGetDeviceRegistryProperty(DeviceObject,
                                         REG_DWORD,
                                         L"UINumber",
                                         NULL,
                                         &UINumber,
                                         &NumberOfBytes);
    if (NT_SUCCESS(Status))
    {
        DeviceCapabilities->UINumber = UINumber;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PdoQueryResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    PCM_RESOURCE_LIST ResourceList;

    DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceExtension->DeviceInfo->ResourceList)
    {
        /* Copy existing resource requirement list */
        ResourceList = ExAllocatePool(
            PagedPool,
            DeviceExtension->DeviceInfo->ResourceListSize);
        if (!ResourceList)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(
            ResourceList,
            DeviceExtension->DeviceInfo->ResourceList,
            DeviceExtension->DeviceInfo->ResourceListSize);

        Irp->IoStatus.Information = (ULONG_PTR)ResourceList;

        return STATUS_SUCCESS;
    }
    else
    {
        /* No resources so just return without changing the status */
        return Irp->IoStatus.Status;
    }
}

static NTSTATUS
PdoQueryResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceList;

    DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceExtension->DeviceInfo->ResourceRequirementsList)
    {
        /* Copy existing resource requirement list */
        ResourceList = ExAllocatePool(PagedPool, DeviceExtension->DeviceInfo->ResourceRequirementsList->ListSize);
        if (!ResourceList)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(
            ResourceList,
            DeviceExtension->DeviceInfo->ResourceRequirementsList,
            DeviceExtension->DeviceInfo->ResourceRequirementsList->ListSize);

        Irp->IoStatus.Information = (ULONG_PTR)ResourceList;

        return STATUS_SUCCESS;
    }
    else
    {
        /* No resource requirements so just return without changing the status */
        return Irp->IoStatus.Status;
    }
}

static NTSTATUS
PdoQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    DEVICE_TEXT_TYPE DeviceTextType;
    NTSTATUS Status = Irp->IoStatus.Status;

    DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DeviceTextType = IrpSp->Parameters.QueryDeviceText.DeviceTextType;

    switch (DeviceTextType)
    {
        case DeviceTextDescription:
        {
            UNICODE_STRING String;
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");

            if (DeviceExtension->DeviceInfo->DeviceDescription.Buffer != NULL)
            {
                Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                                   &DeviceExtension->DeviceInfo->DeviceDescription,
                                                   &String);
                Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            }
            break;
        }

        case DeviceTextLocationInformation:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextLocationInformation\n");
            break;
        }

        default:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown query id type 0x%lx\n", DeviceTextType);
        }
    }

    return Status;
}

NTSTATUS
NTAPI
PdoQueryId(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    BUS_QUERY_ID_TYPE IdType;
    NTSTATUS Status;

    IdType = IrpSp->Parameters.QueryId.IdType;

    DPRINT("PdoQueryId: IRP_MJ_PNP / IRP_MN_QUERY_ID / IdType - %X\n", IdType);

    if (DeviceObject == IopRootDeviceNode->PhysicalDeviceObject)
        return Irp->IoStatus.Status;

    if (NT_SUCCESS(Irp->IoStatus.Status) && Irp->IoStatus.Information)
        return Irp->IoStatus.Status;

    switch (IdType)
    {
        case BusQueryDeviceID:
        case BusQueryInstanceID:
        {
            PDEVICE_NODE DeviceNode;
            PWCHAR Buffer;
            PWCHAR PtrChar;
            ULONG ix;

            DeviceNode = IopGetDeviceNode(DeviceObject);

            ASSERT(DeviceNode);
            ASSERT(DeviceNode->InstancePath.Buffer);
            ASSERT(DeviceNode->InstancePath.Length);

            Buffer = ExAllocatePoolWithTag(PagedPool, DeviceNode->InstancePath.Length, 'ddpP');
            if (!Buffer)
            {
                DPRINT("PdoQueryId: STATUS_INSUFFICIENT_RESOURCES\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlZeroMemory(Buffer, DeviceNode->InstancePath.Length);

            Irp->IoStatus.Information = (ULONG_PTR)Buffer;
            Status = STATUS_SUCCESS;

            PtrChar = DeviceNode->InstancePath.Buffer;

            ix = 0;

            if (IdType == BusQueryInstanceID)
            {
                if (*PtrChar)
                {
                    do
                    {
                        if (*PtrChar == '\\')
                        {
                            ix++;
                            if (ix == 2)
                                break;
                        }

                        PtrChar++;
                    }
                    while (*PtrChar);

                    for (PtrChar += 1; *PtrChar; PtrChar++, Buffer++)
                    {
                        *Buffer = *PtrChar;
                    }
                }

                DPRINT("PdoQueryId: Irp->IoStatus.Information - %p, Id - %S\n", Irp->IoStatus.Information, (PWSTR)Irp->IoStatus.Information);
                break;
            }

            ASSERT(IrpSp->Parameters.QueryId.IdType == BusQueryDeviceID);

            if (*PtrChar)
            {
                do
                {
                    if (*PtrChar == '\\')
                    {
                        ix++;
                        if (ix == 2)
                            break;
                    }

                    *Buffer = *PtrChar;
                    Buffer++;

                    PtrChar++;
                }
                while (*PtrChar);
            }

            DPRINT("PdoQueryId: Irp->IoStatus.Information - %p, Id - %S\n", Irp->IoStatus.Information, (PWSTR)Irp->IoStatus.Information);
            break;
        }
        case BusQueryCompatibleIDs:
        {
            PIOPNP_DEVICE_EXTENSION DeviceExtension;
            PWCHAR Buffer;

            DeviceExtension = DeviceObject->DeviceExtension;

            if (Irp->IoStatus.Status != STATUS_NOT_SUPPORTED ||
                !DeviceExtension ||
                !DeviceExtension->CompatibleIdListSize)
            {
                Status = Irp->IoStatus.Status;
                break;
            }

            Buffer = ExAllocatePoolWithTag(PagedPool, DeviceExtension->CompatibleIdListSize, 'ddpP');
            if (!Buffer)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlCopyMemory(Buffer, DeviceExtension->CompatibleIdList, DeviceExtension->CompatibleIdListSize);

            Irp->IoStatus.Information = (ULONG_PTR)Buffer;
            Status = STATUS_SUCCESS;

            break;
        }
        case BusQueryHardwareIDs:
        case BusQueryDeviceSerialNumber:
        {
            Status = Irp->IoStatus.Status;
            break;
        }
        default:
        {
            ASSERT(FALSE);
            break;
        }
    }

    return Status;
}

static NTSTATUS
PdoQueryBusInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNP_BUS_INFORMATION BusInfo;
    NTSTATUS Status;

    BusInfo = (PPNP_BUS_INFORMATION)ExAllocatePoolWithTag(PagedPool, sizeof(PNP_BUS_INFORMATION), TAG_PNP_ROOT);
    if (!BusInfo)
        Status = STATUS_NO_MEMORY;
    else
    {
        RtlCopyMemory(
            &BusInfo->BusTypeGuid,
            &GUID_BUS_TYPE_INTERNAL,
            sizeof(BusInfo->BusTypeGuid));
        BusInfo->LegacyBusType = PNPBus;
        /* We're the only root bus enumerator on the computer */
        BusInfo->BusNumber = 0;
        Irp->IoStatus.Information = (ULONG_PTR)BusInfo;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
NTAPI
IopTranslatorHandlerCm(
    _Inout_opt_ PVOID Context,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    _In_ RESOURCE_TRANSLATION_DIRECTION Direction,
    _In_opt_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) PIO_RESOURCE_DESCRIPTOR Alternatives,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Target)
{
    PAGED_CODE();
    DPRINT("IopTranslatorHandlerCm(Source - %p, Target - %p)\n", Source, Target);

    Target->Type = Source->Type;
    Target->ShareDisposition = Source->ShareDisposition;
    Target->Flags = Source->Flags;

    Target->u.Generic.Start.QuadPart = Source->u.Generic.Start.QuadPart;
    Target->u.Generic.Length = Source->u.Generic.Length;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopTranslatorHandlerIo(
    _Inout_opt_ PVOID Context,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PULONG TargetCount,
    _Out_writes_(*TargetCount) PIO_RESOURCE_DESCRIPTOR *Target)
{
    DPRINT("IopTranslatorHandlerIo: ... \n");
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Handle Plug and Play IRPs for Root device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
NTSTATUS
NTAPI
PnpRootPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    PAGED_CODE();

    DeviceExtension = DeviceObject->DeviceExtension;
    Status = Irp->IoStatus.Status;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("PnpRootPdoPnpControl: DeviceObject - %p, Irp - %p\n", DeviceObject, Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE: /* 0x00 */
            DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS: /* 0x07 */
            Status = PdoQueryDeviceRelations(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_CAPABILITIES: /* 0x09 */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");
            Status = PdoQueryCapabilities(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_RESOURCES: /* 0x0a */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
            Status = PdoQueryResources(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: /* 0x0b */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Status = PdoQueryResourceRequirements(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT: /* 0x0c */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Status = PdoQueryDeviceText(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* 0x0d */
            DPRINT("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            break;

        case IRP_MN_REMOVE_DEVICE:
            /* Remove the device from the device list and decrement the device count*/
            RemoveEntryList(&DeviceExtension->DeviceInfo->ListEntry);

            /* Free some strings we created */
            RtlFreeUnicodeString(&DeviceExtension->DeviceInfo->DeviceDescription);
            RtlFreeUnicodeString(&DeviceExtension->DeviceInfo->DeviceID);
            RtlFreeUnicodeString(&DeviceExtension->DeviceInfo->InstanceID);

            /* Free the resource requirements list */
            if (DeviceExtension->DeviceInfo->ResourceRequirementsList != NULL)
            ExFreePool(DeviceExtension->DeviceInfo->ResourceRequirementsList);

            /* Free the boot resources list */
            if (DeviceExtension->DeviceInfo->ResourceList != NULL)
            ExFreePool(DeviceExtension->DeviceInfo->ResourceList);

            /* Free the device info */
            ExFreePool(DeviceExtension->DeviceInfo);

            /* Finally, delete the device object */
            IoDeleteDevice(DeviceObject);

            /* Return success */
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_ID: /* 0x13 */
            Status = PdoQueryId(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE: /* 0x14 */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_PNP_DEVICE_STATE\n");
            break;

        case IRP_MN_QUERY_BUS_INFORMATION: /* 0x15 */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_BUS_INFORMATION\n");
            Status = PdoQueryBusInformation(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_INTERFACE:
        {
            const GUID * InterfaceType;

            DPRINT("IopPnPDispatch: IRP_MJ_PNP / IRP_MN_QUERY_INTERFACE\n");

            Status = Irp->IoStatus.Status;

            if (IopGetDeviceNode(DeviceObject) != IopRootDeviceNode)
            {
                DPRINT("IopPnPDispatch: DevNode(%p) != IopRootDeviceNode (%p)\n",
                       IopGetDeviceNode(DeviceObject), IopRootDeviceNode);
                break;
            }

            InterfaceType = IrpSp->Parameters.QueryInterface.InterfaceType;

            if (InterfaceType == &GUID_ARBITER_INTERFACE_STANDARD ||
                (RtlCompareMemory(InterfaceType, &GUID_ARBITER_INTERFACE_STANDARD, sizeof(GUID)) == sizeof(GUID)))
            {
                PARBITER_INTERFACE ArbiterInterface;
                Status = STATUS_SUCCESS;

                ArbiterInterface = (PARBITER_INTERFACE)IrpSp->Parameters.QueryInterface.Interface;
                ArbiterInterface->ArbiterHandler = ArbArbiterHandler;

                switch ((ULONG)IrpSp->Parameters.QueryInterface.InterfaceSpecificData)
                {
                    case CmResourceTypePort:
                        ArbiterInterface->Context = &IopRootPortArbiter;
                        break;

                    case CmResourceTypeInterrupt:
                        ArbiterInterface->Context = &IopRootIrqArbiter;
                        break;

                    case CmResourceTypeMemory:
                        ArbiterInterface->Context = &IopRootMemArbiter;
                        break;

                    case CmResourceTypeDma:
                        ArbiterInterface->Context = &IopRootDmaArbiter;
                        break;

                    case CmResourceTypeBusNumber:
                        ArbiterInterface->Context = &IopRootBusNumberArbiter;
                        break;

                    default:
                        ASSERT(FALSE);
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                }
            }
            else if (InterfaceType == &GUID_TRANSLATOR_INTERFACE_STANDARD ||
                     (RtlCompareMemory(InterfaceType, &GUID_TRANSLATOR_INTERFACE_STANDARD, sizeof(GUID)) == sizeof(GUID)))
            {
                PTRANSLATOR_INTERFACE TranslatorInterface;

                TranslatorInterface = (PTRANSLATOR_INTERFACE)IrpSp->Parameters.QueryInterface.Interface;
                TranslatorInterface->TranslateResources = IopTranslatorHandlerCm;
                TranslatorInterface->TranslateResourceRequirements = IopTranslatorHandlerIo;

                Status = STATUS_SUCCESS;
                break;
            }

            break;
        }
        default:
            DPRINT1("IRP_MJ_PNP / Unknown minor function 0x%lx\n", IrpSp->MinorFunction);
            break;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        ASSERT(FALSE);
    }

    return Status;
}

/*
 * FUNCTION: Handle Power IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS NTAPI
PnpRootPowerControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    Status = Irp->IoStatus.Status;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_QUERY_POWER:
        case IRP_MN_SET_POWER:
            Status = STATUS_SUCCESS;
            break;
    }
    Irp->IoStatus.Status = Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static NTSTATUS NTAPI
PnpRootSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    DPRINT1("PnpRootSystemControl(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);
    ASSERT(FALSE);
    return Status = STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PnpRootAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    DPRINT("PnpRootAddDevice(DriverObject %p, Pdo %p)\n", DriverObject, PhysicalDeviceObject);
    ASSERT(FALSE);
    
    return STATUS_SUCCESS;
}

#if MI_TRACE_PFNS
PDEVICE_OBJECT IopPfnDumpDeviceObject;

NTSTATUS NTAPI
PnpRootCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    if (DeviceObject != IopPfnDumpDeviceObject)
    {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->MajorFunction == IRP_MJ_CREATE)
    {
        MmDumpArmPfnDatabase(TRUE);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
#endif

NTSTATUS NTAPI
PnpRootDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
#if MI_TRACE_PFNS
    NTSTATUS Status;
    UNICODE_STRING PfnDumpDeviceName = RTL_CONSTANT_STRING(L"\\Device\\PfnDump");
#endif

    DPRINT("PnpRootDriverEntry(%p %wZ)\n", DriverObject, RegistryPath);

    IopRootDriverObject = DriverObject;

    DriverObject->DriverExtension->AddDevice = PnpRootAddDevice;

#if MI_TRACE_PFNS
    DriverObject->MajorFunction[IRP_MJ_CREATE] = PnpRootCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = PnpRootCreateClose;
#endif
    DriverObject->MajorFunction[IRP_MJ_PNP] = PnpRootPnpControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PnpRootPowerControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PnpRootSystemControl;

#if MI_TRACE_PFNS
    Status = IoCreateDevice(DriverObject,
                            0,
                            &PfnDumpDeviceName,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &IopPfnDumpDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Creating PFN Dump device failed with %lx\n", Status);
    }
    else
    {
        IopPfnDumpDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
PnpRootRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject)
{
    ASSERT(FALSE);
    return STATUS_UNSUCCESSFUL;
}
