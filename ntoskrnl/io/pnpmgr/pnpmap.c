/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpreport.c
 * PURPOSE:         PNP Mapper Functions
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ULONG KeI386MachineType;

/* TYPES *******************************************************************/

typedef struct _PNP_MAPPER_DEVICE_ID
{ 
    PWCHAR TypeName;
    PWCHAR PnPId;
} PNP_MAPPER_DEVICE_ID, *PPNP_MAPPER_DEVICE_ID; 

typedef struct _PNP_MAPPER_INFORMATION
{ 
    struct _PNP_MAPPER_INFORMATION * NextInfo;
    INTERFACE_TYPE BusType;
    ULONG BusNumber;
    CONFIGURATION_TYPE ControllerType;
    ULONG ControllerNumber;
    CONFIGURATION_TYPE PeripheralType;
    ULONG PeripheralNumber;
    ULONG CmFullDescriptorSize;
    PVOID CmFullDescriptor;
    ULONG IdentifierSize;
    ULONG IdentifierType;
    PVOID Identifier;
    PWCHAR PnPId;
    BOOLEAN IsCreatedNewKey;
    UCHAR Padded[3];
} PNP_MAPPER_INFORMATION, *PPNP_MAPPER_INFORMATION; 

typedef struct _PNP_MAPPER_DEVICE_EXTENSION
{ 
    PPNP_MAPPER_INFORMATION  MapperInfo;
} PNP_MAPPER_DEVICE_EXTENSION, *PPNP_MAPPER_DEVICE_EXTENSION; 

/* DATA **********************************************************************/

PNP_MAPPER_DEVICE_EXTENSION MapperDeviceExtension;

static
CONFIGURATION_TYPE TypeArray[] =
{
    PointerController,
    KeyboardController,
    ParallelController,
    DiskController,
    FloppyDiskPeripheral,
    SerialController // shoul be last !
};

static
PNP_MAPPER_DEVICE_ID PointerMap[] =
{
    { L"PS2 MOUSE", L"*PNP0F0E" },
    { L"SERIAL MOUSE", L"*PNP0F0C" },
    { L"MICROSOFT PS2 MOUSE", L"*PNP0F03" },
    { L"LOGITECH PS2 MOUSE", L"*PNP0F12" },
    { L"MICROSOFT INPORT MOUSE", L"*PNP0F02" },
    { L"MICROSOFT SERIAL MOUSE", L"*PNP0F01" },
    { L"MICROSOFT BALLPOINT SERIAL MOUSE", L"*PNP0F09" },
    { L"LOGITECH SERIAL MOUSE", L"*PNP0F08" },
    { L"MICROSOFT BUS MOUSE", L"*PNP0F00" },
    { L"NEC PC-9800 BUS MOUSE", L"*nEC1F00" },
    { NULL, NULL }
};

static
PNP_MAPPER_DEVICE_ID KeyboardMap[] =
{
    { L"XT_83KEY", L"*PNP0300" },
    { L"PCAT_86KEY", L"*PNP0301" },
    { L"PCXT_84KEY", L"*PNP0302" },
    { L"XT_84KEY", L"*PNP0302" },
    { L"101-KEY", L"*PNP0303" },
    { L"OLI_83KEY", L"*PNP0304" },
    { L"ATT_301", L"*PNP0304" },
    { L"OLI_102KEY", L"*PNP0305" },
    { L"OLI_86KEY", L"*PNP0306" },
    { L"OLI_A101_102KEY", L"*PNP0309" },
    { L"ATT_302", L"*PNP030a" },
    { L"PCAT_ENHANCED", L"*PNP030b" },
    { L"PC98_106KEY", L"*nEC1300" },
    { L"PC98_LaptopKEY", L"*nEC1300" },
    { L"PC98_N106KEY", L"*PNP0303" },
    { NULL, NULL }
};

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
MapperFreeList(VOID)
{
    PPNP_MAPPER_INFORMATION MapperInfo;
    PPNP_MAPPER_INFORMATION NextInfo;

    for (MapperInfo = MapperDeviceExtension.MapperInfo;
         MapperInfo;
         MapperInfo = NextInfo)
    {
        if (MapperInfo->CmFullDescriptor)
            ExFreePoolWithTag(MapperInfo->CmFullDescriptor, 'rpaM');

        if (MapperInfo->Identifier)
            ExFreePoolWithTag(MapperInfo->Identifier, 'rpaM');

        NextInfo = MapperInfo->NextInfo;
        ExFreePoolWithTag(MapperInfo, 'rpaM');
    }
}

PPNP_MAPPER_DEVICE_ID
NTAPI
MapperFindIdentMatch(
    _In_ PPNP_MAPPER_DEVICE_ID MapperId,
    _In_ PWSTR TypeString)
{
    PPNP_MAPPER_DEVICE_ID Id;

    for (Id = MapperId; ; ++Id)
    {
        if (!Id->TypeName)
        {
            return NULL;
        }

        if (!wcscmp(TypeString, Id->TypeName))
        {
            break;
        }
    }

    return Id;
}

PWSTR
NTAPI
MapperTranslatePnPId(
    _In_ CONFIGURATION_TYPE ControllerType,
    _In_ PKEY_VALUE_FULL_INFORMATION PeripheralValueInfo)
{
    PPNP_MAPPER_DEVICE_ID KeyboardId;
    PPNP_MAPPER_DEVICE_ID PointerId;
    PWCHAR Identifier = NULL;

    if (PeripheralValueInfo)
    {
        Identifier = (PWCHAR)((ULONG_PTR)PeripheralValueInfo +
                              PeripheralValueInfo->DataOffset);

        DPRINT("MapperTranslatePnPId: Identifier - %S\n", Identifier);
    }

    switch (ControllerType)
    {
        case DiskController:
            DPRINT("MapperTranslatePnPId: %s (%d) - %s\n",
                   "DiskController", DiskController, "*PNP0700");
            return L"*PNP0700";

        case SerialController:
            DPRINT("MapperTranslatePnPId: %s (%d) - %s\n",
                   "SerialController", SerialController, "*PNP0501");
            return L"*PNP0501";

        case ParallelController:
            DPRINT("MapperTranslatePnPId: %s (%d) - %s\n",
                   "ParallelController", ParallelController, "*PNP0400");
            return L"*PNP0400";

        case PointerController:
            DPRINT("MapperTranslatePnPId: %s (%d) - %s\n",
                   "PointerController", PointerController, "*PNP0F0E");
            return L"*PNP0F0E";

        case KeyboardController:
            DPRINT("MapperTranslatePnPId: %s (%d) - %s\n",
                   "KeyboardController", KeyboardController, "*PNP0300");
            return L"*PNP0300";

        case DiskPeripheral:
            DPRINT("MapperTranslatePnPId: %s (%d) - %s\n",
                   "DiskPeripheral", DiskPeripheral, "NULL");
            return NULL;

        case FloppyDiskPeripheral:
            DPRINT("MapperTranslatePnPId: %s (%d) - %s\n",
                   "FloppyDiskPeripheral", FloppyDiskPeripheral, "*PNP0700");
            return L"*PNP0700";

        case PointerPeripheral:
            if (!Identifier)
            {
                DPRINT("MapperTranslatePnPId: Identifier == NULL\n");
                return NULL;
            }

            PointerId = MapperFindIdentMatch(PointerMap, Identifier);

            if (!PointerId)
            {
                DPRINT("MapperTranslatePnPId: No PointerId for %S\n",
                       Identifier);
                return NULL;
            }

            DPRINT("MapperTranslatePnPId: PointerId->PnPId - %S\n",
                   PointerId->PnPId);
            return PointerId->PnPId;

        case KeyboardPeripheral:
            if (!Identifier)
            {
                DPRINT("MapperTranslatePnPId: Identifier == NULL\n");
                return NULL;
            }

            KeyboardId = MapperFindIdentMatch(KeyboardMap, Identifier);

            if (!KeyboardId)
            {
                DPRINT("MapperTranslatePnPId: No KeyboardId for %S\n",
                       Identifier);
                return NULL;
            }

            DPRINT("MapperTranslatePnPId: KeyboardId->PnPId - %S\n",
                   KeyboardId->PnPId);
            return KeyboardId->PnPId;

        default:
            DPRINT("MapperTranslatePnPId: Unknown ControllerType - %X\n",
                   ControllerType);
            return NULL;
    }
}

NTSTATUS
NTAPI
MapperPeripheralCallback(
    _In_ PVOID Context,
    _In_ PUNICODE_STRING PathName,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG BusNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION * BusInformation,
    _In_ CONFIGURATION_TYPE ControllerType,
    _In_ ULONG ControllerNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION * ControllerInformation,
    _In_ CONFIGURATION_TYPE PeripheralType,
    _In_ ULONG PeripheralNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION * PeripheralInformation)
{
    PPNP_MAPPER_INFORMATION MapperInfo = Context;
    PKEY_VALUE_FULL_INFORMATION IdentInfo;
    SIZE_T IdentInfoLength;
    PWCHAR Identifier;

    DPRINT("MapperPeripheralCallback: PathName - %S\n", PathName->Buffer);

    if (!ControllerInformation)
    {
        DPRINT("MapperPeripheralCallback: ControllerInformation == NULL\n");
    }

    if (!PeripheralInformation)
    {
        DPRINT("MapperPeripheralCallback: PeripheralInformation == NULL\n");
        return STATUS_SUCCESS;
    }

    IdentInfo = PeripheralInformation[0];

    if (!IdentInfo)
    {
        DPRINT("MapperPeripheralCallback: IdentInfo == NULL\n");
        goto Exit;
    }

    MapperInfo->PnPId = MapperTranslatePnPId(PeripheralType,
                                             PeripheralInformation[0]);
    if (!MapperInfo->PnPId)
    {
        DPRINT("MapperPeripheralCallback: MapperInfo->PnPId == NULL\n");
        goto Exit;
    }

    IdentInfoLength = IdentInfo->DataLength;

    if (IdentInfoLength <= sizeof(WCHAR) || IdentInfo->Type != REG_SZ)
    {
        DPRINT("MapperPeripheralCallback: IdentInfoLength - %S, IdentInfo->Type - %X\n",
               IdentInfoLength, IdentInfo->Type);
        goto Exit;
    }

    Identifier = (PWCHAR)((ULONG_PTR)IdentInfo + IdentInfo->DataOffset);

    if (*Identifier == UNICODE_NULL)
    {
        DPRINT("MapperPeripheralCallback: *IdentInfo == NULL\n");
        goto Exit;
    }

    if (MapperInfo->Identifier)
    {
        ExFreePoolWithTag(MapperInfo->Identifier, 'rpaM');
    }

    MapperInfo->Identifier = ExAllocatePoolWithTag(NonPagedPool,
                                                   IdentInfoLength,
                                                   'rpaM');
    if (!Identifier)
    {
        DPRINT1("MapperPeripheralCallback: STATUS_INSUFFICIENT_RESOURCES\n");
        goto Exit;
    }

    MapperInfo->IdentifierType = IdentInfo->Type;
    MapperInfo->IdentifierSize = IdentInfoLength;

    RtlCopyMemory(MapperInfo->Identifier, Identifier, IdentInfoLength);

Exit:

    MapperInfo->PeripheralType = PeripheralType;
    MapperInfo->PeripheralNumber = PeripheralNumber;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MapperCallback(
    _In_ PVOID Context,
    _In_ PUNICODE_STRING PathName,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG BusNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION * BusInformation,
    _In_ CONFIGURATION_TYPE ControllerType,
    _In_ ULONG ControllerNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION * ControllerInformation,
    _In_ CONFIGURATION_TYPE PeripheralType,
    _In_ ULONG PeripheralNumber,
    _In_ PKEY_VALUE_FULL_INFORMATION * PeripheralInformation)
{
    PPNP_MAPPER_DEVICE_EXTENSION MapperContext = Context;
    PKEY_VALUE_FULL_INFORMATION DataInfo;
    PKEY_VALUE_FULL_INFORMATION IdentInfo;
    PPNP_MAPPER_INFORMATION MapperInfo;
    CONFIGURATION_TYPE peripheralType;
    PCM_FULL_RESOURCE_DESCRIPTOR CmFullDescriptor;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDescriptor;
    PWCHAR Identifier;
    SIZE_T DataInfoLength;
    SIZE_T IdentInfoLength;

    DPRINT("MapperCallback: PathName - %wZ, ControllerType - %X\n",
           PathName, ControllerType);

    DataInfo = ControllerInformation[1];
    if (!DataInfo)
    {
        DPRINT("MapperCallback: DataInfo == NULL\n");
        return STATUS_SUCCESS;
    }

    DataInfoLength = DataInfo->DataLength;
    if (!DataInfoLength)
    {
        DPRINT("MapperCallback: DataInfoLength == 0\n");
        return STATUS_SUCCESS;
    }

    MapperInfo = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(PNP_MAPPER_INFORMATION),
                                       'rpaM');
    if (!MapperInfo)
    {
        DPRINT1("MapperCallback: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(MapperInfo, sizeof(PNP_MAPPER_INFORMATION));

    MapperInfo->ControllerType = ControllerType;
    MapperInfo->ControllerNumber = ControllerNumber;

    MapperInfo->BusNumber = BusNumber;
    MapperInfo->BusType = BusType;

    CmFullDescriptor = ExAllocatePoolWithTag(NonPagedPool,
                                             DataInfoLength,
                                             'rpaM');
    if (!CmFullDescriptor)
    {
        DPRINT1("MapperCallback: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(MapperInfo, 'rpaM');
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    cmFullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
                       ((ULONG_PTR)DataInfo + DataInfo->DataOffset);

    RtlCopyMemory(CmFullDescriptor, cmFullDescriptor, DataInfoLength);

    MapperInfo->CmFullDescriptor = CmFullDescriptor;
    MapperInfo->CmFullDescriptorSize = DataInfoLength;

    IdentInfo = ControllerInformation[0];

    if (IdentInfo)
    {
        IdentInfoLength = IdentInfo->DataLength;

        if (IdentInfoLength)
        {
            Identifier = (PWCHAR)((ULONG_PTR)IdentInfo + IdentInfo->DataOffset);

            if (ControllerType == ParallelController)
            {
                DPRINT("MapperCallback: FIXME ControllerType - ParallelController\n");
                ASSERT(FALSE);
            }
            else
            {
                if (IdentInfoLength)
                {
                    MapperInfo->Identifier = ExAllocatePoolWithTag(NonPagedPool,
                                                                   IdentInfoLength,
                                                                   'rpaM');
                    if (MapperInfo->Identifier)
                    {
                        MapperInfo->IdentifierType = IdentInfo->Type;
                        MapperInfo->IdentifierSize = IdentInfoLength;

                        RtlCopyMemory(MapperInfo->Identifier,
                                      Identifier,
                                      IdentInfoLength);
                    }
                    else
                    {
                        DPRINT1("MapperCallback: STATUS_INSUFFICIENT_RESOURCES\n");
                    }
                }
            }
        }
    }

    switch (ControllerType)
    {
        case DiskController:
            peripheralType = FloppyDiskPeripheral;
            break;

        case SerialController:
        case ParallelController:
            peripheralType = ArcSystem;
            break;

        case PointerController:
            peripheralType = PointerPeripheral;
            break;

        case KeyboardController:
            peripheralType = KeyboardPeripheral;
            break;

        default:
            peripheralType = ArcSystem;
            break;
    }

    DPRINT("MapperCallback: PathName - %S, Ident[0] - %X, Data[1] - %X, Information[2] - %X\n",
           PathName->Buffer,
           ControllerInformation[0],
           ControllerInformation[1],
           ControllerInformation[2]);

    if (peripheralType != ArcSystem)
    {
        DPRINT("MapperCallback: peripheralType - %d\n", peripheralType);

        IoQueryDeviceDescription(&BusType,
                                 &BusNumber,
                                 &ControllerType,
                                 &ControllerNumber,
                                 &peripheralType,
                                 0,
                                 MapperPeripheralCallback,
                                 MapperInfo);
    }

    if (!MapperInfo->PnPId && !MapperInfo->PeripheralType)
    {
        MapperInfo->PnPId = MapperTranslatePnPId(ControllerType, NULL);

        if (!MapperInfo->PnPId)
        {
            DPRINT("MapperCallback: No PnPId for %S !\n", PathName->Buffer);
        }
    }

    DPRINT("MapperCallback: Constructed name - %d_%d_%d_%d_%d_%d\n",
           MapperInfo->BusType,
           MapperInfo->BusNumber,
           MapperInfo->ControllerType,
           MapperInfo->ControllerNumber,
           MapperInfo->PeripheralType,
           MapperInfo->PeripheralNumber);

    if (MapperInfo->PnPId)
    {
        MapperInfo->NextInfo = MapperContext->MapperInfo;
        MapperContext->MapperInfo = MapperInfo;
    }
    else
    {
        ExFreePoolWithTag(CmFullDescriptor, 'rpaM');

        if (MapperInfo->Identifier)
        {
            ExFreePoolWithTag(MapperInfo->Identifier, 'rpaM');
        }

        ExFreePoolWithTag(MapperInfo, 'rpaM');
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MapperProcessFirmwareTree(
    _In_ BOOLEAN IsDisableMapper)
{
    CONFIGURATION_TYPE ControllerType;
    INTERFACE_TYPE Interface;
    ULONG Index;
    ULONG ArraySize; 
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("MapperProcessFirmwareTree: IsDisableMapper %X\n", IsDisableMapper);

    ArraySize = (*(&TypeArray + 1) - TypeArray);

    for (Interface = Internal;
         Interface < MaximumInterfaceType;
         Interface++)
    {
        if (IsDisableMapper)
            /* Only SerialController */
            Index = (ArraySize - 1);
        else
            Index = 0;

        for (; Index < ArraySize; Index++)
        {
            ControllerType = TypeArray[Index];

            Status = IoQueryDeviceDescription(&Interface,
                                              NULL,
                                              &ControllerType,
                                              NULL,
                                              NULL,
                                              NULL,
                                              MapperCallback,
                                              &MapperDeviceExtension);
        }
    }

    return Status;
}

PCM_RESOURCE_LIST
NTAPI
MapperAdjustResourceList(
    _In_ PCM_RESOURCE_LIST CmResource,
    _In_ PWCHAR PnPId,
    _Inout_ PULONG OutListSize)
{
    PCM_RESOURCE_LIST NewCmResource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR BadCmDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR NewCmDescriptor;
    ULONG ix;

    DPRINT("MapperAdjustResourceList: CmResource %p, PnPId '%S'\n", CmResource, PnPId);

    if (KeI386MachineType == MACHINE_TYPE_EISA)
    {
        DPRINT1("MapperAdjustResourceList: FIXME. KeI386MachineType == MACHINE_TYPE_EISA\n");
        ASSERT(FALSE);
    }

    if (wcscmp(PnPId, L"*PNP0700") != 0) // Floppy Id
        return CmResource;

    DPRINT("MapperAdjustResourceList: Floppy\n");

    if (CmResource->Count != 1)
    {
        DPRINT1("MapperAdjustResourceList: CmResource->Count %X\n", CmResource->Count);
        return CmResource;
    }

    CmDescriptor = CmResource->List[0].PartialResourceList.PartialDescriptors;
    BadCmDescriptor = NULL;

    if (CmResource->List[0].PartialResourceList.Count == 0)
    {
        DPRINT1("MapperAdjustResourceList: CmResource->List[0].PartialResourceList.Count = 0\n");
        return CmResource;
    }

    for (ix = 0; ix < CmResource->List[0].PartialResourceList.Count; ix++)
    {
        if (CmDescriptor->Type == CmResourceTypePort &&
            CmDescriptor->u.Port.Length == 8)
        {
            if (BadCmDescriptor)
            {
                BadCmDescriptor = NULL;
                break;
            }
            else
            {
                BadCmDescriptor = CmDescriptor;
            }
        }

        CmDescriptor++;
    }

    if (!BadCmDescriptor)
        return CmResource;

    DPRINT("MapperAdjustResourceList: BadCmDescriptor %p, Port.Length %X\n", BadCmDescriptor, BadCmDescriptor->u.Port.Length);

    BadCmDescriptor->u.Port.Length = 6;

    NewCmResource = ExAllocatePoolWithTag(NonPagedPool, (*OutListSize + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)), 'rpaM');
    if (!NewCmResource)
    {
        DPRINT1("MapperAdjustResourceList: STATUS_INSUFFICIENT_RESOURCES\n");
        return CmResource;
    }

    RtlCopyMemory(NewCmResource, CmResource, *OutListSize);

    NewCmDescriptor = &NewCmResource->List[0].PartialResourceList.PartialDescriptors[0] +
                      NewCmResource->List[0].PartialResourceList.Count;

    RtlMoveMemory(NewCmDescriptor, BadCmDescriptor, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    NewCmDescriptor->u.Port.Start.QuadPart += 7;
    NewCmDescriptor->u.Port.Length = 1;

    NewCmResource->List[0].PartialResourceList.Count++;
    *OutListSize += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    ExFreePoolWithTag(CmResource, 'rpaM');

    return NewCmResource;
}

VOID
NTAPI
MapperMarkKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING KeyName,
    _In_ PPNP_MAPPER_INFORMATION MapperInfo)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    PCM_RESOURCE_LIST CmResource;
    PCM_RESOURCE_LIST CmBootConfigList;
    UNICODE_STRING ValueName;
    ULONG KeyNameLength;
    PWCHAR BufferEnd;
    ULONG Disposition;
    ULONG Data;
    ULONG Length;
    NTSTATUS Status;

    DPRINT("MapperMarkKey: KeyName '%wZ'\n", KeyName);

    KeyNameLength = KeyName->Length;

    Data = 1;
    RtlInitUnicodeString(&ValueName, L"FirmwareIdentified");

    ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Data, sizeof(ULONG));

    BufferEnd = &KeyName->Buffer[KeyName->Length / sizeof(WCHAR) + 1];

    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
                               NULL,
                               NULL);

    RtlAppendUnicodeToString(KeyName, L"\\Control");

    Status = ZwCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);

    if (NT_SUCCESS(Status))
    {
        Data = 1;
        RtlInitUnicodeString(&ValueName, L"FirmwareMember");
        ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Data, sizeof(ULONG));
        ZwClose(KeyHandle);
    }
    else
    {
        DPRINT("MapperMarkKey: Status %X\n", Status);
    }

    if (!MapperInfo->CmFullDescriptor)
        goto Exit;

    KeyName->Length = KeyNameLength;
    *BufferEnd = UNICODE_NULL;

    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
                               NULL,
                               NULL);

    RtlAppendUnicodeToString(KeyName, L"\\LogConf");

    Status = ZwCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("MapperMarkKey: Status %X\n", Status);
        goto Exit;
    }

    Length = MapperInfo->CmFullDescriptorSize +
             (sizeof(CM_RESOURCE_LIST) - sizeof(CM_FULL_RESOURCE_DESCRIPTOR));

    CmResource = ExAllocatePoolWithTag(NonPagedPool, Length, 'rpaM');

    if (!CmResource)
    {
        DPRINT1("MapperMarkKey: STATUS_INSUFFICIENT_RESOURCES\n");
        ZwClose(KeyHandle);
        goto Exit;
    }

    CmResource->Count = 1;

    RtlCopyMemory(CmResource->List, MapperInfo->CmFullDescriptor, MapperInfo->CmFullDescriptorSize);

    CmBootConfigList = MapperAdjustResourceList(CmResource, MapperInfo->PnPId, &Length);

    RtlInitUnicodeString(&ValueName, L"BootConfig");
    ZwSetValueKey(KeyHandle, &ValueName, 0, REG_RESOURCE_LIST, CmBootConfigList, Length);

    ExFreePoolWithTag(CmBootConfigList, 'rpaM');
    ZwClose(KeyHandle);

Exit:
    KeyName->Length = KeyNameLength;
    *BufferEnd = UNICODE_NULL;
}

#define PNP_MAPPER_SEED_BUFFER_SIZE 0x400

VOID
NTAPI
MapperSeedKey(
    _In_ HANDLE Handle,
    _In_ PUNICODE_STRING KeyName,
    _In_ PPNP_MAPPER_INFORMATION MapperInfo,
    _In_ BOOLEAN IsDisableMapper)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    CONFIGURATION_TYPE ControllerType;
    UNICODE_STRING ValueName;
    PWCHAR Buffer;
    PWCHAR BufferEnd;
    HANDLE KeyHandle;
    ULONG Disposition;
    ULONG IdentifierSize;
    ULONG Data;
    NTSTATUS Status;
    USHORT KeyNameLength;

    DPRINT("MapperSeedKey: KeyName '%wZ'\n", KeyName);

    Buffer = ExAllocatePoolWithTag(NonPagedPool, PNP_MAPPER_SEED_BUFFER_SIZE, 'rpaM');
    if (!Buffer)
    {
        DPRINT1("MapperSeedKey: STATUS_INSUFFICIENT_RESOURCES\n");
        return;
    }

    RtlZeroMemory(Buffer, PNP_MAPPER_SEED_BUFFER_SIZE);

    KeyNameLength = KeyName->Length;
    
    BufferEnd = (PWCHAR)((ULONG_PTR)KeyName->Buffer + KeyName->Length);
    *BufferEnd = UNICODE_NULL;

    RtlAppendUnicodeToString(KeyName, L"\\Control");

    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
                               NULL,
                               NULL);

    Status = ZwCreateKey(&KeyHandle,
                         (KEY_READ | KEY_WRITE),
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_SZ,
                         &Disposition);

    if (NT_SUCCESS(Status))
    {
        ZwClose(KeyHandle);
    }
    else
    {
        DPRINT("MapperSeedKey: Status %X\n", Status);
    }

    KeyName->Length = KeyNameLength;

    RtlAppendUnicodeToString(KeyName, L"\\LogConf");

    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
                               NULL,
                               NULL);

    Status = ZwCreateKey(&KeyHandle,
                         (KEY_READ | KEY_WRITE),
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_NONE,
                         &Disposition);

    if (NT_SUCCESS(Status))
    {
        ZwClose(KeyHandle);
    }
    else
    {
        DPRINT("MapperSeedKey: Status %X\n", Status);
    }

    KeyName->Length = KeyNameLength;

    ControllerType = MapperInfo->ControllerType;

    if ((ControllerType == SerialController &&
         ControllerType == ParallelController) ||
        MapperInfo->Identifier != NULL)
    {
        Status = IopOpenDeviceParametersSubkey(&KeyHandle, NULL, KeyName, (KEY_READ | KEY_WRITE));

        if (NT_SUCCESS(Status))
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            DPRINT("MapperSeedKey: Status %X\n", Status);
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    IdentifierSize = (wcslen(MapperInfo->PnPId) + 2) * sizeof(WCHAR);

    if (MapperInfo->BusType == Eisa)
    {
        ASSERT(FALSE);
    }
    else
    {
        RtlCopyMemory(Buffer, MapperInfo->PnPId, (IdentifierSize - sizeof(WCHAR)));
        Buffer[IdentifierSize / sizeof(WCHAR) - 1] = UNICODE_NULL;
    }

    RtlInitUnicodeString(&ValueName, L"HardwareID");
    ZwSetValueKey(Handle,
                  &ValueName,
                  0,
                  REG_MULTI_SZ,
                  Buffer,
                  IdentifierSize);

    if (MapperInfo->PeripheralType == KeyboardPeripheral)
    {
        ULONG Len = sizeof(L"PS2_KEYBOARD");
        RtlMoveMemory(Buffer, L"PS2_KEYBOARD", Len);
        IdentifierSize = (Len + sizeof(WCHAR));
    }
    else if (MapperInfo->PeripheralType == PointerPeripheral &&
             (!wcscmp(MapperInfo->PnPId, L"*PNP0F0E") ||
              !wcscmp(MapperInfo->PnPId, L"*PNP0F03") ||
              !wcscmp(MapperInfo->PnPId, L"*PNP0F12")))
    {
        ULONG Len = sizeof(L"PS2_MOUSE");
        RtlMoveMemory(Buffer, L"PS2_MOUSE", Len);
        IdentifierSize = (Len + sizeof(WCHAR));
    }
    else
    {
        goto Next;
    }

    Buffer[IdentifierSize / sizeof(WCHAR)] = UNICODE_NULL;
    IdentifierSize += sizeof(WCHAR);

    RtlInitUnicodeString(&ValueName, L"CompatibleIDs");
    ZwSetValueKey(Handle, &ValueName, 0, REG_MULTI_SZ, Buffer, IdentifierSize);

Next:

    Data = 1;
    RtlInitUnicodeString(&ValueName, L"FirmwareIdentified");
    ZwSetValueKey(Handle, &ValueName, 0, REG_DWORD, &Data, sizeof(ULONG));

    RtlMoveMemory(Buffer, MapperInfo->Identifier, MapperInfo->IdentifierSize);

    RtlInitUnicodeString(&ValueName, L"DeviceDesc");
    ZwSetValueKey(Handle, &ValueName, 0, REG_SZ, Buffer, MapperInfo->IdentifierSize);

    if (IsDisableMapper)
    {
        Data = 1;
        RtlInitUnicodeString(&ValueName, L"Phantom");
        ZwSetValueKey(Handle, &ValueName, 0, REG_DWORD, &Data, sizeof(ULONG));
    }

    ExFreePoolWithTag(Buffer, 'rpaM');
}

#define PNP_MAPPER_REGISTRY_BUFFER_SIZE 0x800
#define PNP_MAPPER_INSTANCE_BUFFER_SIZE 0x200

VOID
NTAPI
MapperConstructRootEnumTree(
    _In_ BOOLEAN IsDisableMapper)
{
    PPNP_MAPPER_INFORMATION MapperInfo;
    PKEY_VALUE_FULL_INFORMATION KeyInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ValueName;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    PWCHAR RegistryBuffer;
    PWSTR InstanceBuffer;
    ULONG Disposition;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("MapperConstructRootEnumTree: IsDisableMapper %X\n", IsDisableMapper);

    RegistryBuffer = ExAllocatePoolWithTag(NonPagedPool, PNP_MAPPER_REGISTRY_BUFFER_SIZE, 'rpaM');
    if (!RegistryBuffer)
    {
        DPRINT1("MapperConstructRootEnumTree: STATUS_INSUFFICIENT_RESOURCES\n");
        MapperFreeList();
        return;
    }

    InstanceBuffer = ExAllocatePoolWithTag(NonPagedPool, PNP_MAPPER_INSTANCE_BUFFER_SIZE, 'rpaM');
    if (!InstanceBuffer)
    {
        DPRINT1("MapperConstructRootEnumTree: STATUS_INSUFFICIENT_RESOURCES\n");
        MapperFreeList();
        ExFreePoolWithTag(RegistryBuffer, 'rpaM');
        return;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
                               NULL,
                               NULL);

    for (MapperInfo = MapperDeviceExtension.MapperInfo;
         MapperInfo;
         MapperInfo = MapperInfo->NextInfo)
    {
        KeyName.Length = 0;
        KeyName.MaximumLength = PNP_MAPPER_REGISTRY_BUFFER_SIZE;
        KeyName.Buffer = RegistryBuffer;

        RtlZeroMemory(RegistryBuffer, PNP_MAPPER_REGISTRY_BUFFER_SIZE);

        RtlAppendUnicodeToString(&KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\Root\\");
        RtlAppendUnicodeToString(&KeyName, MapperInfo->PnPId);

        Status = ZwCreateKey(&KeyHandle,
                             (KEY_READ | KEY_WRITE),
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &Disposition);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("MapperConstructRootEnumTree: Status %X\n", Status);
            continue;
        }

        ZwClose(KeyHandle);

        RtlZeroMemory(InstanceBuffer, PNP_MAPPER_INSTANCE_BUFFER_SIZE);

        RtlStringCbPrintfW(InstanceBuffer,
                           PNP_MAPPER_INSTANCE_BUFFER_SIZE,
                           L"\\%d_%d_%d_%d_%d_%d",
                           MapperInfo->BusType,
                           MapperInfo->BusNumber,
                           MapperInfo->ControllerType,
                           MapperInfo->ControllerNumber,
                           MapperInfo->PeripheralType,
                           MapperInfo->PeripheralNumber);

        RtlAppendUnicodeToString(&KeyName, InstanceBuffer);

        Status = ZwCreateKey(&KeyHandle,
                             (KEY_READ | KEY_WRITE),
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &Disposition);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("MapperConstructRootEnumTree: Status %X\n", Status);
            continue;
        }

        if (Disposition != REG_CREATED_NEW_KEY)
        {
            Status = IopGetRegistryValue(KeyHandle, L"Migrated", &KeyInfo);

            if (NT_SUCCESS(Status))
            {
                if (KeyInfo->Type == REG_DWORD &&
                    KeyInfo->DataLength == sizeof(ULONG))
                {
                    if (*(PULONG)((ULONG_PTR)KeyInfo + KeyInfo->DataOffset) != 0)
                    {
                        Disposition = REG_CREATED_NEW_KEY;
                    }
                }

                ExFreePoolWithTag(KeyInfo, 'uspP');

                RtlInitUnicodeString(&ValueName, L"Migrated");
                ZwDeleteValueKey(KeyHandle, &ValueName);
            }
            else
            {
                DPRINT("MapperConstructRootEnumTree: Status %X\n", Status);
            }
        }

        if (Disposition == REG_CREATED_NEW_KEY)
        {
            MapperInfo->IsCreatedNewKey = TRUE;
            MapperSeedKey(KeyHandle, &KeyName, MapperInfo, IsDisableMapper);
        }

        MapperMarkKey(KeyHandle, &KeyName, MapperInfo);
        ZwClose(KeyHandle);
    }

    ExFreePoolWithTag(InstanceBuffer, 'rpaM');
}

/* EOF */
