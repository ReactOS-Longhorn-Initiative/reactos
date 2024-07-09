/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpres.c
 * PURPOSE:         Resource handling code
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 *                  ReactOS Portable Systems Group
 */

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PDEVICE_NODE IopLegacyDeviceNode = NULL;

extern PPNP_RESERVED_RESOURCES_CONTEXT IopInitReservedResourceList;
extern KSEMAPHORE PpRegistrySemaphore;
extern INTERFACE_TYPE PnpDefaultInterfaceType;
extern BOOLEAN IopBootConfigsReserved;
extern LIST_ENTRY IopLegacyBusInformationTable[MaximumInterfaceType];
extern ERESOURCE PpRegistryDeviceResource;
extern PDEVICE_NODE IopRootDeviceNode;
extern PDEVICE_NODE IopInitHalDeviceNode;
extern PCM_RESOURCE_LIST IopInitHalResources;
extern PNP_ALLOCATE_RESOURCES_ROUTINE IopAllocateBootResourcesRoutine;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
IopWriteResourceList(
    _In_ HANDLE Handle,
    _In_ PUNICODE_STRING ResourceName,
    _In_ PUNICODE_STRING Description,
    _In_ PUNICODE_STRING ValueName,
    _In_ PCM_RESOURCE_LIST CmResource,
    _In_ ULONG ListSize)
{
    NTSTATUS Status;
    HANDLE ResourceHandle;
    HANDLE DescriptionHandle;

    PAGED_CODE();

    if (ResourceName)
        DPRINT("IopWriteResourceList: ResourceName - %wZ\n", ResourceName);
    if (Description)
        DPRINT("IopWriteResourceList: Description - %wZ\n", Description);
    if (ResourceName)
        DPRINT("IopWriteResourceList: ValueName - %wZ\n", ValueName);

    Status = IopCreateRegistryKeyEx(&ResourceHandle,
                                    Handle,
                                    ResourceName,
                                    KEY_READ | KEY_WRITE,
                                    REG_OPTION_VOLATILE,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = IopCreateRegistryKeyEx(&DescriptionHandle,
                                    ResourceHandle,
                                    Description,
                                    KEY_READ | KEY_WRITE,
                                    REG_OPTION_VOLATILE,
                                    NULL);

    ZwClose(ResourceHandle);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (CmResource->Count)
    {
        Status = ZwSetValueKey(DescriptionHandle,
                               ValueName,
                               0,
                               REG_RESOURCE_LIST,
                               CmResource,
                               ListSize);
    }
    else
    {
        Status = ZwDeleteValueKey(DescriptionHandle, ValueName);
    }

    ZwClose(DescriptionHandle);

    return Status;
}

NTSTATUS
NTAPI
IopFilterResourceRequirementsList(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST InIoResources,
    _In_ PCM_RESOURCE_LIST CmResources,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST * OutIoResources,
    _Out_ BOOLEAN * OutIsNoFiltered)
{
    PCM_FULL_RESOURCE_DESCRIPTOR CmFullList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    PIO_RESOURCE_REQUIREMENTS_LIST IoResources;
    PIO_RESOURCE_REQUIREMENTS_LIST NewIoResources;
    PIO_RESOURCE_LIST IoList;
    PIO_RESOURCE_LIST NewIoList;
    PIO_RESOURCE_LIST CurrentIoList = NULL;
    PIO_RESOURCE_DESCRIPTOR IoDescriptor;
    PIO_RESOURCE_DESCRIPTOR IoDescriptorsEnd;
    PIO_RESOURCE_DESCRIPTOR CurrentIoDesc;
    ULONGLONG IoMinimumValue;
    ULONGLONG IoMaximumValue;
    ULONGLONG CmMinimumValue;
    ULONGLONG CmMaximumValue;
    ULONG CmLength;
    ULONG IoLength;
    ULONG IoAlignment;
    ULONG IoAltListsCount;
    ULONG CmDescCount = 0;
    ULONG IoDescCount;
    ULONG NewIoDescCount = 0;
    ULONG DataSize;
    ULONG ListSise;
    ULONG ix;
    ULONG jx;
    ULONG kx;
    ULONG mx;
    NTSTATUS Status;
    USHORT Version;
    UCHAR IoShareDisposition;
    UCHAR CmShareDisposition;
    BOOLEAN IsNoFiltered;

    PAGED_CODE();
    DPRINT("IopFilterResourceRequirementsList: InIoResources - %p, CmResources - %p\n",
           InIoResources, CmResources);

    *OutIoResources = NULL;
    *OutIsNoFiltered = FALSE;

    if (!InIoResources || !InIoResources->AlternativeLists)
    {
        if (CmResources && CmResources->Count)
        {
            *OutIoResources = IopCmResourcesToIoResources(0,
                                                          CmResources,
                                                          LCPRI_BOOTCONFIG);
        }
        else
        {
            ASSERT(FALSE);
        }

        return STATUS_SUCCESS;
    }

    IoResources = ExAllocatePoolWithTag(PagedPool,
                                        InIoResources->ListSize,
                                        'uspP');
    if (!IoResources)
    {
        ASSERT(FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(IoResources, InIoResources, InIoResources->ListSize);

    if (!CmResources || !CmResources->Count)
    {
        ASSERT(FALSE);
        *OutIoResources = IoResources;
        return STATUS_SUCCESS;
    }

    CmFullList = &CmResources->List[0];
    for (ix = 0; ix < CmResources->Count; ix++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;

        CmDescCount += CmFullList->PartialResourceList.Count;
        CmDescriptor = CmFullList->PartialResourceList.PartialDescriptors;

        for (jx = 0; jx < CmFullList->PartialResourceList.Count; jx++)
        {
            if (CmDescriptor->Type == CmResourceTypeNull ||
                CmDescriptor->Type >= CmResourceTypeMaximum)
            {
                DPRINT("IopFilterResourceRequirementsList: CmDescriptor->Type - %X\n",
                       CmDescriptor->Type);

                CmDescCount--;
            }

            CmDescriptor = PipGetNextCmPartialDescriptor(CmDescriptor);
        }

        CmFullList = (PCM_FULL_RESOURCE_DESCRIPTOR)CmDescriptor;
    }

    if (!CmDescCount)
    {
        ASSERT(FALSE);
        *OutIoResources = IoResources;
        return STATUS_SUCCESS;
    }

    IoList = &IoResources->List[0];
    for (ix = 0; ix < IoResources->AlternativeLists; ix++)
    {
        for (jx = 0; jx < IoList->Count; jx++)
        {
            IoDescriptor = &IoList->Descriptors[jx];
            IoDescriptor->Spare1 = 0;
        }

        IoList = (PIO_RESOURCE_LIST)(IoList->Descriptors + IoList->Count);
    }

    IoList = &IoResources->List[0];
    IoAltListsCount = IoResources->AlternativeLists;

    for (ix = 0; ix < IoResources->AlternativeLists; ix++)
    {
        DPRINT("IopFilterResourceRequirementsList: ix- %X, IoAltListsCount - %X\n",
               ix, IoAltListsCount);

        if (IoList->Version == -1)
        {
            Version = 1;
        }
        else
        {
            Version = IoList->Version;
        }

        IoDescCount = IoList->Count;
        IoDescriptorsEnd = &IoList->Descriptors[IoDescCount];

        if (IoList->Descriptors == IoDescriptorsEnd)
        {
            IoList->Version = -1;
            IoResources->AlternativeLists--;
            continue;
        }
        else
        {
            IoList->Version = 0;
        }

        IsNoFiltered = TRUE;
        CmFullList = CmResources->List;

        for (jx = 0; jx < CmResources->Count; jx++)
        {
            CmDescriptor = CmFullList->PartialResourceList.PartialDescriptors;

            for (kx = 0; kx < CmFullList->PartialResourceList.Count; kx++)
            {
                if (CmDescriptor->Type == CmResourceTypeDeviceSpecific)
                {
                    DataSize = CmDescriptor->u.Generic.Start.LowPart;
                    goto NextCmDescriptor;
                }
                else
                {
                    DataSize = 0;
                }

                if (CmDescriptor->Type == CmResourceTypeNull ||
                    CmDescriptor->Type >= CmResourceTypeMaximum)
                {
                    goto NextCmDescriptor;
                }

                for (mx = 0; mx < 2; mx++)
                {
                    for (IoDescriptor = IoList->Descriptors;
                         IoDescriptor < IoDescriptorsEnd;
                         IoDescriptor++)
                    {
                        if (IoDescriptor->Type == CmDescriptor->Type && IoDescriptor->Spare1)
                        {
                            CmLength = 1;
                            IoLength = 1;

                            if (CmDescriptor->ShareDisposition == CmResourceShareDeviceExclusive ||
                                CmDescriptor->ShareDisposition == CmResourceShareDriverExclusive ||
                                CmDescriptor->ShareDisposition == CmResourceShareShared)
                            {
                                CmShareDisposition = CmDescriptor->ShareDisposition;
                            }
                            else
                            {
                                CmShareDisposition = IoDescriptor->ShareDisposition;
                            }

                            if (IoDescriptor->ShareDisposition == CmResourceShareDeviceExclusive ||
                                IoDescriptor->ShareDisposition == CmResourceShareDriverExclusive ||
                                IoDescriptor->ShareDisposition == CmResourceShareShared)
                            {
                                IoShareDisposition = IoDescriptor->ShareDisposition;
                            }
                            else
                            {
                                IoShareDisposition = CmShareDisposition;
                            }

                            IoAlignment = 1;

                            switch (CmDescriptor->Type)
                            {
                                case CmResourceTypePort:
                                case CmResourceTypeMemory:
                                {
                                    CmMinimumValue = CmDescriptor->u.Generic.Start.QuadPart;
                                    CmMaximumValue = CmDescriptor->u.Generic.Start.QuadPart +
                                                     CmDescriptor->u.Generic.Length - 1;
                                    CmLength = CmDescriptor->u.Generic.Length;

                                    IoMinimumValue = IoDescriptor->u.Generic.MinimumAddress.QuadPart;
                                    IoMaximumValue = IoDescriptor->u.Generic.MaximumAddress.QuadPart;
                                    IoAlignment = IoDescriptor->u.Generic.Alignment;
                                    IoLength = IoDescriptor->u.Generic.Length;

                                    break;
                                }
                                case CmResourceTypeInterrupt:
                                {
                                    CmMinimumValue = CmDescriptor->u.Interrupt.Vector;
                                    CmMaximumValue = CmDescriptor->u.Interrupt.Vector;

                                    IoMinimumValue = IoDescriptor->u.Interrupt.MinimumVector;
                                    IoMaximumValue = IoDescriptor->u.Interrupt.MaximumVector;

                                    break;
                                }
                                case CmResourceTypeDma:
                                {
                                    CmMaximumValue = CmDescriptor->u.Dma.Channel;
                                    CmMinimumValue = CmDescriptor->u.Dma.Channel;

                                    IoMinimumValue = IoDescriptor->u.Port.Length;
                                    IoMaximumValue = IoDescriptor->u.Dma.MaximumChannel;

                                    break;
                                }
                                case CmResourceTypeBusNumber:
                                {
                                    CmMinimumValue = CmDescriptor->u.BusNumber.Start;
                                    CmMaximumValue = CmMinimumValue +
                                                     CmDescriptor->u.BusNumber.Length - 1;
                                    CmLength = CmDescriptor->u.BusNumber.Length;

                                    IoMinimumValue = IoDescriptor->u.BusNumber.MinBusNumber;
                                    IoMaximumValue = IoDescriptor->u.BusNumber.MaxBusNumber;
                                    IoLength = IoDescriptor->u.BusNumber.Length;

                                    break;
                                }
                                default:
                                {
                                    ASSERT(FALSE);

                                    CmMinimumValue = 0;
                                    CmMaximumValue = 0;

                                    IoMinimumValue = 0;
                                    IoMaximumValue = 0;

                                    break;
                                }
                            }

                            if (mx != 0)
                            {
                                IsNoFiltered = FALSE;

                                if (IoMinimumValue <= CmMinimumValue &&
                                    IoMaximumValue >= CmMaximumValue &&
                                    IoLength >= CmLength &&
                                    ((ULONG)CmMinimumValue & (IoAlignment - 1)) == 0 &&
                                    IoShareDisposition == CmShareDisposition)
                                {
                                    switch (CmDescriptor->Type)
                                    {
                                        case CmResourceTypePort:
                                        case CmResourceTypeMemory:
                                            IoDescriptor->u.Generic.MinimumAddress.QuadPart = CmMinimumValue;
                                            IoDescriptor->u.Generic.MaximumAddress.QuadPart = CmMinimumValue + IoLength - 1;
                                            break;

                                        case CmResourceTypeInterrupt:
                                            IoDescriptor->u.Interrupt.MinimumVector = CmMinimumValue;
                                            IoDescriptor->u.Interrupt.MaximumVector = CmMaximumValue;
                                            break;

                                        case CmResourceTypeDma:
                                            IoDescriptor->u.Dma.MinimumChannel = CmMinimumValue;
                                            IoDescriptor->u.Dma.MaximumChannel = CmMaximumValue;
                                            break;

                                        case CmResourceTypeBusNumber:
                                            IoDescriptor->u.BusNumber.MinBusNumber = CmMinimumValue;
                                            IoDescriptor->u.BusNumber.MaxBusNumber = CmMinimumValue + IoLength - 1;
                                            break;

                                        default:
                                            ASSERT(FALSE);
                                            break;
                                    }

                                    IoList->Version++;

                                    IoDescriptor->Spare1 = 0x80;
                                    IoDescriptor->Flags = CmDescriptor->Flags;

                                    if (IoDescriptor->Option & IO_RESOURCE_ALTERNATIVE)
                                    {
                                        PIO_RESOURCE_DESCRIPTOR descriptor;

                                        for (descriptor = IoDescriptor - 1;
                                             descriptor >= IoList->Descriptors;
                                             descriptor--)
                                        {
                                            descriptor->Type = CmResourceTypeNull;
                                            IoList->Count--;
                                        }
                                    }

                                    IoDescriptor->Option = IO_RESOURCE_PREFERRED;

                                    while (TRUE)
                                    {
                                        IoDescriptor++;

                                        if (IoDescriptor >= IoDescriptorsEnd ||
                                            !(IoDescriptor->Option & IO_RESOURCE_ALTERNATIVE))
                                        {
                                            break;
                                        }

                                        IoDescriptor->Type = CmResourceTypeNull;
                                        IoList->Count--;
                                    }
                                    break;
                                }
                                continue;
                            }

                            /* mx == 0*/
                            if (IoMinimumValue == CmMinimumValue &&
                                IoMaximumValue >= CmMaximumValue &&
                                IoLength >= CmLength &&
                                IoShareDisposition == CmShareDisposition)
                            {
                                if (IoMaximumValue != CmMaximumValue)
                                {
                                    IsNoFiltered = FALSE;
                                }

                                IoList->Version++;
                                IoDescriptor->Spare1 = 0x80;

                                if (IoDescriptor->Option & IO_RESOURCE_ALTERNATIVE)
                                {
                                    PIO_RESOURCE_DESCRIPTOR descriptor;

                                    for (descriptor = IoDescriptor - 1;
                                         descriptor >= IoList->Descriptors;
                                         descriptor--)
                                    {
                                        IoList->Count--;
                                        descriptor->Type = CmResourceTypeNull;

                                        if (descriptor->Option == IO_RESOURCE_ALTERNATIVE)
                                        {
                                            continue;
                                        }
                                    }
                                }

                                IoDescriptor->Option = IO_RESOURCE_PREFERRED;
                                IoDescriptor->Flags = CmDescriptor->Flags;

                                switch (CmDescriptor->Type)
                                {
                                    case CmResourceTypePort:
                                    case CmResourceTypeMemory:
                                        IoDescriptor->u.Generic.MinimumAddress.QuadPart = CmMinimumValue;
                                        IoDescriptor->u.Generic.MaximumAddress.QuadPart = CmMinimumValue + IoLength - 1;
                                        IoDescriptor->u.Generic.Alignment = 1;
                                        break;

                                    case CmResourceTypeBusNumber:
                                        IoDescriptor->u.BusNumber.MinBusNumber = CmMinimumValue;
                                        IoDescriptor->u.BusNumber.MaxBusNumber = CmMinimumValue + IoLength - 1;
                                        break;

                                    default:
                                        break;
                                }

                                CurrentIoDesc = IoDescriptor + 1;

                                while (CurrentIoDesc < IoDescriptorsEnd &&
                                       CurrentIoDesc->Option & IO_RESOURCE_ALTERNATIVE)
                                {
                                    CurrentIoDesc->Type = CmResourceTypeNull;
                                    CurrentIoDesc++;
                                    IoList->Count--;
                                }

                                mx = 1;
                                break;
                            }
                        }
                    }
                }

NextCmDescriptor:
                /* Next Cm partial descriptor */
                CmDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                               ((ULONG_PTR)CmDescriptor +
                               sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + DataSize);
            }

            /* Next Cm full descriptor */
            CmFullList = (PCM_FULL_RESOURCE_DESCRIPTOR)CmDescriptor;
        }

        if (IoList->Version == (USHORT)CmDescCount)
        {
            if (IoList->Count == CmDescCount ||
                (IoList->Count == (CmDescCount + 1) &&
                 IoList->Descriptors[0].Type == CmResourceTypeConfigData))
            {
                if (CurrentIoList == NULL)
                {
                    CurrentIoList = IoList;
                    IoList->Version = Version;
                    NewIoDescCount += IoList->Count;

                    if (IsNoFiltered)
                    {
                        *OutIsNoFiltered = TRUE;
                    }
                }
                else
                {
                    IoList->Version = -1;
                    IoResources->AlternativeLists--;
                }

            }
            else
            {
                NewIoDescCount += IoList->Count;
                IoList->Version = Version;
            }
        }
        else
        {
            IoList->Version = -1;
            IoResources->AlternativeLists--;
        }

        IoList->Count = IoDescCount;

        /* Next Alternative list */
        IoList = (PIO_RESOURCE_LIST)IoDescriptorsEnd;
    }

    if (!IoResources->AlternativeLists)
    {
        *OutIoResources = IopCmResourcesToIoResources(0,
                                                      CmResources,
                                                      LCPRI_BOOTCONFIG);
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    ListSise = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
               (IoResources->AlternativeLists - 1) * sizeof(IO_RESOURCE_LIST) +
               NewIoDescCount * sizeof(IO_RESOURCE_DESCRIPTOR);

    NewIoResources = ExAllocatePoolWithTag(PagedPool, ListSise, 'uspP');

    if (!NewIoResources)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    NewIoResources->ListSize = ListSise;

    NewIoResources->InterfaceType = CmResources->List[0].InterfaceType;
    NewIoResources->BusNumber = CmResources->List[0].BusNumber;
    NewIoResources->SlotNumber = IoResources->SlotNumber;

    if (IoResources->AlternativeLists > 1)
    {
        *OutIsNoFiltered = FALSE;
    }

    NewIoResources->AlternativeLists = IoResources->AlternativeLists;

    IoList = IoResources->List;
    NewIoList = NewIoResources->List;

    for (ix = 0; ix < IoAltListsCount; ix++)
    {
        IoDescriptor = IoList->Descriptors;
        IoDescriptorsEnd = &IoList->Descriptors[IoList->Count];

        if (IoList->Version != -1)
        {
            PIO_RESOURCE_DESCRIPTOR newIoDescriptor;
            PIO_RESOURCE_DESCRIPTOR newConfigIoDescriptor;

            NewIoList->Version = IoList->Version;
            NewIoList->Revision = IoList->Revision;

            newIoDescriptor = &NewIoList->Descriptors[0];

            if (IoList->Descriptors[0].Type == CmResourceTypeConfigData)
            {
                NewIoResources->ListSize -= sizeof(IO_RESOURCE_DESCRIPTOR);
                newConfigIoDescriptor = newIoDescriptor;
            }
            else
            {
                /* First descriptor is config descriptor */
                newIoDescriptor->Option = IO_RESOURCE_PREFERRED;
                newIoDescriptor->Type = CmResourceTypeConfigData;
                newIoDescriptor->ShareDisposition = CmResourceShareShared;
                newIoDescriptor->Spare1 = 0;
                newIoDescriptor->Flags = 0;
                newIoDescriptor->Spare2 = 0;

                newIoDescriptor->u.ConfigData.Priority = LCPRI_BOOTCONFIG;

                newConfigIoDescriptor = newIoDescriptor;
                newIoDescriptor++;
            }

            for (; IoDescriptor < IoDescriptorsEnd; IoDescriptor++)
            {
                if (IoDescriptor->Type)
                {
                    RtlCopyMemory(newIoDescriptor,
                                  IoDescriptor,
                                  sizeof(IO_RESOURCE_DESCRIPTOR));

                    newIoDescriptor++;
                }
            }

            NewIoList->Count = (ULONG)(newIoDescriptor - NewIoList->Descriptors);

            newConfigIoDescriptor->u.ConfigData.Priority = LCPRI_BOOTCONFIG;

            NewIoList = (PIO_RESOURCE_LIST)newIoDescriptor;
        }

        IoList = (PIO_RESOURCE_LIST)IoDescriptorsEnd;
    }

    ASSERT((ULONG_PTR)NewIoList == (ULONG_PTR)NewIoResources +
                                              NewIoResources->ListSize);

    *OutIoResources = NewIoResources;
    Status = STATUS_SUCCESS;

Exit:

    ExFreePoolWithTag(IoResources, 'uspP');
    return Status;
}

NTSTATUS
NTAPI
IopMergeFilteredResourceRequirementsList(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST IoResources1,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST IoResources2,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *OutIoResources)
{
    PIO_RESOURCE_REQUIREMENTS_LIST IoResources;
    PIO_RESOURCE_REQUIREMENTS_LIST NewIoResources;
    SIZE_T Size;

    PAGED_CODE();
    DPRINT("IopMergeFilteredResourceRequirementsList: IoResources1 %p, IoResources2 %p\n",
           IoResources1, IoResources2);

    *OutIoResources = NULL;

    if ((!IoResources1 || !IoResources1->AlternativeLists) &&
        (!IoResources2 || !IoResources2->AlternativeLists))
    {
        return STATUS_SUCCESS;
    }

    if (IoResources1 && IoResources1->AlternativeLists)
    {
        if (IoResources2 && IoResources2->AlternativeLists)
        {
            Size = IoResources1->ListSize +
                   IoResources2->ListSize -
                   sizeof(IO_RESOURCE_DESCRIPTOR);

            NewIoResources = ExAllocatePoolWithTag(PagedPool, Size, 'uspP');

            if (NewIoResources)
            {
                RtlCopyMemory(NewIoResources,
                              IoResources1,
                              IoResources1->ListSize);

                RtlCopyMemory((PUCHAR)NewIoResources + IoResources1->ListSize,
                              IoResources2->List,
                              Size - IoResources1->ListSize);

                NewIoResources->ListSize = Size;
                NewIoResources->AlternativeLists += IoResources2->AlternativeLists;

                *OutIoResources = NewIoResources;

                return STATUS_SUCCESS;
            }
            else
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            IoResources = IoResources1;
        }
    }
    else
    {
        IoResources = IoResources2;
    }

    NewIoResources = ExAllocatePoolWithTag(PagedPool,
                                           IoResources->ListSize,
                                           'uspP');
    if (!NewIoResources)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(NewIoResources, IoResources, IoResources->ListSize);

    *OutIoResources = NewIoResources;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopQueryDeviceRequirements(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST * OutResourceList,
    _Out_ PULONG OutSize)
{
    PIO_RESOURCE_REQUIREMENTS_LIST IoResources;
    PIO_RESOURCE_REQUIREMENTS_LIST NewIoResources;
    PIO_RESOURCE_REQUIREMENTS_LIST FilteredIoResource = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST newIoResources;
    PIO_RESOURCE_REQUIREMENTS_LIST MergedIoResources;
    PCM_RESOURCE_LIST CmResource;
    UNICODE_STRING ValueName;
    PDEVICE_NODE DeviceNode;
    HANDLE Handle;
    HANDLE KeyHandle;
    PCM_RESOURCE_LIST CmResources;
    SIZE_T ResourcesSize;
    NTSTATUS Status;
    BOOLEAN IsNoFiltered;
  
    PAGED_CODE();
    DPRINT("IopQueryDeviceRequirements: [%p] *OutResourceList %X\n", DeviceObject, *OutResourceList);

    *OutResourceList = NULL;
    *OutSize = 0;

    DeviceNode = IopGetDeviceNode(DeviceObject);

    Status = IopGetDeviceResourcesFromRegistry(DeviceObject,
                                               FALSE,
                                               PIP_CONFIG_TYPE_FORCED,
                                               (PVOID *)&CmResource,
                                               &ResourcesSize);

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        //"ForcedConfig"

        ASSERT(NT_SUCCESS(Status));
        DPRINT("IopQueryDeviceRequirements: ForcedConfig. Status - %X\n", Status);

        if (!CmResource)
        {
            IoResources = NULL;
            goto Exit;
        }

        IoResources = IopCmResourcesToIoResources(0, CmResource, 0);
        ExFreePoolWithTag(CmResource, 'uspP');

        if (!IoResources)
        {
            *OutResourceList = NULL;
            *OutSize = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        *OutResourceList = IoResources;
        *OutSize = IoResources->ListSize;

        goto Exit;
    }

    DPRINT("IopQueryDeviceRequirements: STATUS_OBJECT_NAME_NOT_FOUND (%p)\n", DeviceObject);

    Status = IopGetDeviceResourcesFromRegistry(DeviceObject,
                                               TRUE,
                                               PIP_CONFIG_TYPE_OVERRIDE,
                                               (PVOID *)&IoResources,
                                               &ResourcesSize);

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ASSERT(FALSE);
    }
    else if (DeviceNode->Flags & DNF_MADEUP)
    {
        Status = IopGetDeviceResourcesFromRegistry(DeviceObject,
                                                   TRUE,
                                                   PIP_CONFIG_TYPE_BASIC,
                                                   (PVOID *)&IoResources,
                                                   &ResourcesSize);
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            IoResources = NULL;
            Status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(Status))
        {
            ASSERT(FALSE);
            return Status;
        }
    }
    else if (!DeviceNode->ResourceRequirements)
    {
        Status = PpIrpQueryResourceRequirements(DeviceObject, &IoResources);
        if (Status == STATUS_NOT_SUPPORTED)
        {
            DPRINT("IopQueryDeviceRequirements: PpIrpQueryResourceRequirements return STATUS_NOT_SUPPORTED\n");
            ASSERT(IoResources == NULL);
            IoResources = NULL;
            Status = STATUS_SUCCESS;
        }
        else if (!NT_SUCCESS(Status))
        {
            ASSERT(FALSE);
            return Status;
        }
        else
        {
            DPRINT("IopQueryDeviceRequirements: PipDumpResourceRequirementsList\n");
            PipDumpResourceRequirementsList(IoResources, 1);
        }
    }
    else
    {
        ASSERT(DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED);

        IoResources = ExAllocatePoolWithTag(PagedPool, DeviceNode->ResourceRequirements->ListSize, '  pP');
        if (!IoResources)
        {
            ASSERT(FALSE);
            return STATUS_NO_MEMORY;
        }

        DPRINT("IopQueryDeviceRequirements: PipDumpResourceRequirementsList (%p, %X)\n",
               DeviceNode->ResourceRequirements, DeviceNode->ResourceRequirements->ListSize);

        PipDumpResourceRequirementsList(DeviceNode->ResourceRequirements, 1);

        RtlCopyMemory(IoResources, DeviceNode->ResourceRequirements, DeviceNode->ResourceRequirements->ListSize);
    }

    Status = IopGetDeviceResourcesFromRegistry(DeviceObject, FALSE, PIP_CONFIG_TYPE_BOOT, (PVOID *)&CmResources, &ResourcesSize);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (CmResources && CmResources->Count && (CmResources->List[0].InterfaceType == PCIBus))
        goto Exit;

    Status = IopFilterResourceRequirementsList(IoResources, CmResources, &NewIoResources, &IsNoFiltered);

    if (CmResources)
        ExFreePoolWithTag(CmResources, 'uspP');

    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);

        if (IoResources)
            ExFreePoolWithTag(IoResources, '  pP');

        return Status;
    }

    if ((DeviceNode->Flags & DNF_MADEUP) ||
        (IsNoFiltered && (IoResources->AlternativeLists <= 1)))
    {
        if (IoResources)
            ExFreePoolWithTag(IoResources, '  pP');

        IoResources = NewIoResources;
    }
    else
    {
        Status = IopMergeFilteredResourceRequirementsList(NewIoResources, IoResources, &MergedIoResources);

        if (IoResources)
            ExFreePool(IoResources);//ExFreePoolWithTag(IoResources, '  pP');

        if (NewIoResources)
            ExFreePoolWithTag(NewIoResources, 'uspP');

        if (!NT_SUCCESS(Status))
        {
            ASSERT(FALSE);
            return Status;
        }

        IoResources = MergedIoResources;
    }

Exit:

    Status = IopFilterResourceRequirementsCall(DeviceObject, IoResources, &FilteredIoResource);
    DPRINT("IopQueryDeviceRequirements: Status %X, FilteredIoResource %p\n", Status, FilteredIoResource);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(Status == STATUS_NOT_SUPPORTED);

        *OutResourceList = IoResources;

        if (IoResources == NULL)
            *OutSize = 0;
        else
            *OutSize = IoResources->ListSize;

        return STATUS_SUCCESS;
    }

    if (!FilteredIoResource)
    {
        if (IoResources)
        {
            DPRINT("IopQueryDeviceRequirements: IoResources filtered to NULL! %p, %X\n", IoResources, IoResources->ListSize);
        }

        *OutSize = 0;
        *OutResourceList = NULL;
    }
    else
    {
        *OutSize = FilteredIoResource->ListSize;
        ASSERT(*OutSize);

        newIoResources = ExAllocatePoolWithTag(PagedPool, *OutSize, '  pP');
        *OutResourceList = newIoResources;

        if (!newIoResources)
        {
            ExFreePool(FilteredIoResource);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(newIoResources, FilteredIoResource, *OutSize);

        ExFreePool(FilteredIoResource);
    }

    Status = PnpDeviceObjectToDeviceInstance(DeviceObject, &Handle, KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
        return STATUS_SUCCESS;

    RtlInitUnicodeString(&ValueName, L"Control");

    Status = IopOpenRegistryKeyEx(&KeyHandle, Handle, &ValueName, KEY_READ);
    if (!NT_SUCCESS(Status))
        return STATUS_SUCCESS;

    RtlInitUnicodeString(&ValueName, L"FilteredConfigVector");
    ZwSetValueKey(KeyHandle, &ValueName, 0, REG_RESOURCE_REQUIREMENTS_LIST, *OutResourceList, *OutSize);

    ZwClose(KeyHandle);
    ZwClose(Handle);

    return STATUS_SUCCESS;
}

PDEVICE_NODE
NTAPI
IopFindLegacyBusDeviceNode(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG LegacyBusNumber)
{
    PDEVICE_NODE DeviceNode;
    PLIST_ENTRY Header;
    PLIST_ENTRY Entry;
    ULONG BusNumber;

    PAGED_CODE();
    DPRINT("IopFindLegacyBusDeviceNode: InterfaceType - %X\n", InterfaceType);

    DeviceNode = IopRootDeviceNode;

    if (InterfaceType >= MaximumInterfaceType ||
        InterfaceType <= InterfaceTypeUndefined ||
        InterfaceType == PNPBus)
    {
        DPRINT("IopFindLegacyBusDeviceNode: return IopRootDeviceNode\n");
        return DeviceNode;
    }

    if (InterfaceType == Eisa)
    {
        Header = &IopLegacyBusInformationTable[Isa];
    }
    else
    {
        Header = &IopLegacyBusInformationTable[InterfaceType];
        DPRINT("IopFindLegacyBusDeviceNode: Header - %p\n", Header);
    }

    for (Entry = Header->Flink;
         Entry != Header;
         Entry = Entry->Flink)
    {
        BusNumber = CONTAINING_RECORD(Entry,
                                      DEVICE_NODE,
                                      LegacyBusListEntry)->BusNumber;

        if (BusNumber == LegacyBusNumber)
        {
            DeviceNode = CONTAINING_RECORD(Entry,
                                           DEVICE_NODE,
                                           LegacyBusListEntry);
            break;
        }

        if (BusNumber > LegacyBusNumber)
        {
            break;
        }
    }

    if (DeviceNode == IopRootDeviceNode)
    {
        DPRINT("IopFindLegacyBusDeviceNode: return IopRootDeviceNode\n");
    }
    else
    {
        DPRINT("IopFindLegacyBusDeviceNode: Found - %wZ, Interface - %X, Bus - %X\n",
               &DeviceNode->InstancePath, InterfaceType, LegacyBusNumber);
    }

    return DeviceNode;
}

BOOLEAN
NTAPI
IopFindResourceHandlerInfo(
    _In_ ULONG Type,
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR IoDescriptorType,
    _In_ PVOID * OutArbEntry)
{
    PPI_RESOURCE_ARBITER_ENTRY ArbEntry;
    PLIST_ENTRY Head;
    PLIST_ENTRY Entry;
    USHORT NoBitMask;
    USHORT QueryBitMask;
    USHORT TypesBitMask;
    BOOLEAN Result;

    DPRINT("IopFindResourceHandlerInfo: Type - %X, DeviceNode - %p, IoDescriptorType - %X\n",
           Type, DeviceNode, IoDescriptorType);

    *OutArbEntry = NULL;

    if (Type == IOP_RES_HANDLER_TYPE_TRANSLATOR)
    {
        NoBitMask = DeviceNode->NoTranslatorMask;
        QueryBitMask = DeviceNode->QueryTranslatorMask;
        Head = &DeviceNode->DeviceTranslatorList;
    }
    else if (Type == IOP_RES_HANDLER_TYPE_ARBITER)
    {
        NoBitMask = DeviceNode->NoArbiterMask;
        QueryBitMask = DeviceNode->QueryArbiterMask;
        Head = &DeviceNode->DeviceArbiterList;
    }
    else
    {
        DPRINT("IopFindResourceHandlerInfo: Unknown Type - %X\n", Type);
        ASSERT(FALSE);
        return FALSE;
    }

    TypesBitMask = 1 << IoDescriptorType;

    DPRINT("IopFindResourceHandlerInfo: TypesBitMask - %04X, NoBitMask - %04X\n",
           TypesBitMask, NoBitMask);

    if (NoBitMask & TypesBitMask)
    {
        DPRINT("IopFindResourceHandlerInfo: return TRUE\n");
        Result = TRUE;
    }
    else if (QueryBitMask & TypesBitMask)
    {
        for (Entry = Head->Flink;
             Entry != Head;
             Entry = Entry->Flink)
        {
            ArbEntry = CONTAINING_RECORD(Entry,
                                         PI_RESOURCE_ARBITER_ENTRY,
                                         DeviceArbiterList);

            if (ArbEntry->ResourceType == IoDescriptorType)
            {
                break;
            }
        }

        ASSERT(Entry != Head);
        *OutArbEntry = ArbEntry;

        DPRINT("IopFindResourceHandlerInfo: return TRUE\n");
        Result = TRUE;
    }
    else
    {
        if (IoDescriptorType > IOP_MAX_MAIN_RESOURCE_TYPE)
        {
            for (Entry = Head->Flink;
                 Entry != Head;
                 Entry = Entry->Flink)
            {
                ArbEntry = CONTAINING_RECORD(Entry,
                                             PI_RESOURCE_ARBITER_ENTRY,
                                             DeviceArbiterList);

                if (ArbEntry->ResourceType == IoDescriptorType)
                {
                    break;
                }
            }

            if (ArbEntry->ArbiterInterface)
            {
                *OutArbEntry = ArbEntry;
            }

            DPRINT("IopFindResourceHandlerInfo: return TRUE\n");
            Result = TRUE;
        }
        else
        {
            DPRINT("IopFindResourceHandlerInfo: return FALSE\n");
            Result = FALSE;
        }
    }

    return Result;
}

NTSTATUS
NTAPI
IopTranslateAndAdjustReqDesc(
    _In_ PPNP_REQ_DESCRIPTOR ReqDescriptor,
    _In_ PPI_RESOURCE_TRANSLATOR_ENTRY TranslatorEntry,
    _In_ PPNP_REQ_DESCRIPTOR* OutReqDesc)
{
    PTRANSLATOR_INTERFACE TranslatorInterface;
    PPNP_REQ_DESCRIPTOR NewReqResDescs;
    PNP_REQ_RESOURCE_ENTRY* Res;
    PIO_RESOURCE_DESCRIPTOR* target;
    PIO_RESOURCE_DESCRIPTOR ioDescriptor;
    PIO_RESOURCE_DESCRIPTOR NewIoDescriptors;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PULONG NewIoDescCount;
    ULONG NumbersOfIoDescs = 0;
    ULONG ix;
    NTSTATUS OutStatus = STATUS_SUCCESS;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN IsTranslate = FALSE;
#if 1
    PDEVICE_NODE DeviceNode;

    ASSERT(ReqDescriptor->ReqEntry.PhysicalDevice);
    DeviceNode = IopGetDeviceNode(ReqDescriptor->ReqEntry.PhysicalDevice);
    ASSERT(DeviceNode);
#endif

    DPRINT("IopTranslateAndAdjustReqDesc: Descriptor %p Entry %p\n", ReqDescriptor, TranslatorEntry);

    TranslatorInterface = TranslatorEntry->TranslatorInterface;

    if (ReqDescriptor->ReqEntry.Count == 0)
    {
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    *OutReqDesc = NULL;

    target = ExAllocatePoolWithTag(PagedPool, (4 * ReqDescriptor->ReqEntry.Count), 'erpP');
    if (!target)
    {
        DPRINT1("IopTranslateAndAdjustReqDesc: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    DPRINT("IopTranslateAndAdjustReqDesc: target %p\n", target);
    RtlZeroMemory(target, (4 * ReqDescriptor->ReqEntry.Count));

    NewIoDescCount = ExAllocatePoolWithTag(PagedPool, (4 * ReqDescriptor->ReqEntry.Count), 'erpP');
    if (!NewIoDescCount)
    {
        DPRINT1("IopTranslateAndAdjustReqDesc: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(target, 'erpP');
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    DPRINT("IopTranslateAndAdjustReqDesc: NewIoDescCount %p\n", NewIoDescCount);
    RtlZeroMemory(NewIoDescCount, (4 * ReqDescriptor->ReqEntry.Count));

    ioDescriptor = ReqDescriptor->ReqEntry.IoDescriptor;

    for (ix = 0; ix < ReqDescriptor->ReqEntry.Count; ix++, ioDescriptor++)
    {
        Status = TranslatorInterface->
                 TranslateResourceRequirements(TranslatorInterface->Context,
                                               ioDescriptor,
                                               ReqDescriptor->ReqEntry.PhysicalDevice,
                                               &NewIoDescCount[ix],
                                               &target[ix]);

        if (NT_SUCCESS(Status) && NewIoDescCount[ix])
        {
            NumbersOfIoDescs += NewIoDescCount[ix];
            IsTranslate = TRUE;
            //DPRINT1("TranslateResourceRequirements ret ok\n", DeviceNode->InstancePath.Buffer);
            PipDumpIoResourceDescriptor(ioDescriptor, 0);
        }
        else
        {
            DPRINT1("Translator failed to adjust resreqlist for %S\n", DeviceNode->InstancePath.Buffer);
            DPRINT1("Status %X NewIoDescCount[ix] %p\n", Status, NewIoDescCount[ix]);
            PipDumpIoResourceDescriptor(ioDescriptor, 0);

            NewIoDescCount[ix] = 0;
            target[ix] = ioDescriptor;
            NumbersOfIoDescs++;
        }

        if (NT_SUCCESS(Status) && OutStatus != STATUS_TRANSLATION_COMPLETE)
            OutStatus = Status;
    }

    if (!IsTranslate)
    {
        DPRINT1("Failed to translate any requirement for %S!\n", DeviceNode->InstancePath.Buffer);
        OutStatus = Status;
    }

    ASSERT(NumbersOfIoDescs != 0);

    NewIoDescriptors = ExAllocatePoolWithTag(PagedPool, (NumbersOfIoDescs * sizeof(IO_RESOURCE_DESCRIPTOR)), 'erpP');
    if (!NewIoDescriptors)
    {
        DPRINT1("IopTranslateAndAdjustReqDesc: STATUS_INSUFFICIENT_RESOURCES\n");
        OutStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    DPRINT("IopTranslateAndAdjustReqDesc: NewIoDescriptors %p\n", NewIoDescriptors);

    NewReqResDescs = ExAllocatePoolWithTag(PagedPool, sizeof(PNP_REQ_DESCRIPTOR), 'erpP');
    if (!NewReqResDescs)
    {
        DPRINT1("IopTranslateAndAdjustReqDesc: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(NewIoDescriptors, 'erpP');
        OutStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    DPRINT("IopTranslateAndAdjustReqDesc: NewReqResDescs %p\n", NewReqResDescs);
    RtlCopyMemory(NewReqResDescs, ReqDescriptor, sizeof(PNP_REQ_DESCRIPTOR));

    NewReqResDescs->AltList = NULL;
    NewReqResDescs->TranslatedReqDesc = NULL;
    NewReqResDescs->TranslatorEntry = TranslatorEntry;

    Res = &NewReqResDescs->ReqEntry;
    Res->Count = NumbersOfIoDescs;
    Res->IoDescriptor = NewIoDescriptors;
    Res->pCmDescriptor = &NewReqResDescs->ReqEntry.CmDescriptor;
    InitializeListHead(&Res->Link);

    Descriptor = ReqDescriptor->ReqEntry.IoDescriptor;
    if (ReqDescriptor->ReqEntry.Count == 0)
        ASSERT(FALSE); // IoDbgBreakPointEx();

    for (ix = 0; ix < ReqDescriptor->ReqEntry.Count; ix++, NewIoDescriptors++)
    {
        if (NewIoDescCount[ix])
        {
            RtlCopyMemory(NewIoDescriptors, target[ix], (NewIoDescCount[ix] * sizeof(IO_RESOURCE_DESCRIPTOR)));
            NewIoDescriptors += NewIoDescCount[ix];
            continue;
        }

        RtlCopyMemory(NewIoDescriptors, Descriptor, sizeof(IO_RESOURCE_DESCRIPTOR));

        switch (NewIoDescriptors->Type)
        {
            case CmResourceTypePort:
                NewIoDescriptors->u.Port.MinimumAddress.LowPart = 2;
                NewIoDescriptors->u.Port.MinimumAddress.HighPart = 0;
                NewIoDescriptors->u.Port.MaximumAddress.LowPart = 1;
                NewIoDescriptors->u.Port.MaximumAddress.HighPart = 0;
                break;

            case CmResourceTypeInterrupt:
                NewIoDescriptors->u.Interrupt.MinimumVector = 2;
                NewIoDescriptors->u.Interrupt.MaximumVector = 1;
                break;

            case CmResourceTypeMemory:
                NewIoDescriptors->u.Memory.MinimumAddress.LowPart = 2;
                NewIoDescriptors->u.Memory.MinimumAddress.HighPart = 0;
                NewIoDescriptors->u.Memory.MaximumAddress.LowPart = 1;
                NewIoDescriptors->u.Memory.MaximumAddress.HighPart = 0;
                break;

            case CmResourceTypeDma:
                NewIoDescriptors->u.Dma.MinimumChannel = 2;
                NewIoDescriptors->u.Dma.MaximumChannel = 1;
                break;

            case CmResourceTypeBusNumber:
                NewIoDescriptors->u.BusNumber.MinBusNumber = 2;
                NewIoDescriptors->u.BusNumber.MaxBusNumber = 1;
                break;

            default:
                DPRINT1("IopTranslateAndAdjustReqDesc: unknown Type %X\n", NewIoDescriptors->Type);
                ASSERT(FALSE); // IoDbgBreakPointEx();
                break;
        }
    }

    Descriptor = NewReqResDescs->ReqEntry.IoDescriptor;

    if (Descriptor->Option & IO_RESOURCE_ALTERNATIVE)
    {
        PDEVICE_NODE DeviceNode = NULL;

        if (ReqDescriptor->ReqEntry.PhysicalDevice)
        {
            DeviceNode = IopGetDeviceNode(ReqDescriptor->ReqEntry.PhysicalDevice);
            ASSERT(DeviceNode);
        }

        DPRINT1("IopTranslateAndAdjustReqDesc: Pdo %X, Node %X\n", ReqDescriptor->ReqEntry.PhysicalDevice, DeviceNode);
        DPRINT1("IopTranslateAndAdjustReqDesc: Descriptor %X\n", Descriptor);
        PipDumpIoResourceDescriptor(Descriptor, 0);

        if (DeviceNode)
        {
          #if DBG
            DPRINT1("Dumping Node:\n");
            PipDumpDeviceNodes(NULL, 1+2+4+8, 0); // !devnode from WinDbg
            DPRINT1("\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();
          #endif
        }
        else
        {
            ASSERT(FALSE); // IoDbgBreakPointEx();
        }

        ASSERT(FALSE); // IoDbgBreakPointEx(); // ASSERT(!(Descriptor->Option & IO_RESOURCE_ALTERNATIVE));
    }

    Descriptor++;

    for (ix = 1; ix < NumbersOfIoDescs; ix++, Descriptor++)
    {
        PDEVICE_NODE DeviceNode = NULL;

        if ((Descriptor->Option & IO_RESOURCE_ALTERNATIVE) != 0)
            continue;

        if (ReqDescriptor->ReqEntry.PhysicalDevice)
        {
            DeviceNode = IopGetDeviceNode(ReqDescriptor->ReqEntry.PhysicalDevice);
            ASSERT(DeviceNode);
        }

        DPRINT1("IopTranslateAndAdjustReqDesc: Pdo %X, Node %X\n", ReqDescriptor->ReqEntry.PhysicalDevice, DeviceNode);
        DPRINT1("IopTranslateAndAdjustReqDesc: Descriptor %Xn", Descriptor);
        PipDumpIoResourceDescriptor(Descriptor, 0);

        if (DeviceNode)
        {
          #if DBG
            DPRINT1("Dumping Node:\n");
            PipDumpDeviceNodes(NULL, 1+2+4+8, 0);
            DPRINT1("\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();
          #endif
        }
        else
        {
            ASSERT(FALSE); // IoDbgBreakPointEx(); // ASSERT(DeviceNode);
        }

        ASSERT(FALSE); // IoDbgBreakPointEx(); // ASSERT(Descriptor->Option & IO_RESOURCE_ALTERNATIVE);
    }

    *OutReqDesc = NewReqResDescs;

Exit:

    for (ix = 0; ix < ReqDescriptor->ReqEntry.Count; ix++)
    {
        if (NewIoDescCount[ix])
        {
            ASSERT(target[ix]);
            ExFreePool(target[ix]);
        }
    }

    if (target)
        ExFreePoolWithTag(target, 'erpP');

    if (NewIoDescCount)
        ExFreePoolWithTag(NewIoDescCount, 'erpP');

    return OutStatus;
}

NTSTATUS
NTAPI
IopSetupArbiterAndTranslators(
    _In_ PPNP_REQ_DESCRIPTOR ReqDescriptor)
{
    PDEVICE_OBJECT PhysicalDevice;
    ARBITER_REQUEST_SOURCE AllocationType;
    PDEVICE_NODE DeviceNode;
    PPI_RESOURCE_ARBITER_ENTRY ResArbiterEntry;
    PPI_RESOURCE_TRANSLATOR_ENTRY TranslatorEntry;
    PPNP_REQ_DESCRIPTOR TranslatedReqDesc;
    USHORT TypesBitMask;
    PVOID Interface;
    NTSTATUS Status;
    UCHAR IoDescriptorType;
    BOOLEAN IsFindTranslator = TRUE;
    BOOLEAN IsTranslatorFound = FALSE;
    BOOLEAN IsArbiterFound = FALSE;
    BOOLEAN IsFindBus;
    BOOLEAN Result;

    PhysicalDevice = ReqDescriptor->ReqEntry.PhysicalDevice;
    AllocationType = ReqDescriptor->ReqEntry.AllocationType;
    IoDescriptorType = ReqDescriptor->TranslatedReqDesc->ReqEntry.IoDescriptor->Type;

    DPRINT("IopSetupArbiterAndTranslators: ReqDesc %p Pdo %p Type %X\n", ReqDescriptor, PhysicalDevice, IoDescriptorType);

    if ((AllocationType == ArbiterRequestHalReported) && (ReqDescriptor->InterfaceType == Internal))
        IsFindBus = FALSE;
    else
        IsFindBus = TRUE;

    if (PhysicalDevice && (AllocationType != ArbiterRequestHalReported))
    {
        DeviceNode = IopGetDeviceNode(PhysicalDevice);
        ASSERT(DeviceNode);
    }
    else
    {
        DeviceNode = IopRootDeviceNode;
    }

    while (DeviceNode != NULL)
    {
        if ((DeviceNode == IopRootDeviceNode) && !IsTranslatorFound && IsFindBus)
        {
            IsFindBus = FALSE;

            DeviceNode = IopFindLegacyBusDeviceNode(ReqDescriptor->InterfaceType, ReqDescriptor->BusNumber);

            if (DeviceNode == IopRootDeviceNode &&
                ReqDescriptor->AltList->ReqList->InterfaceType == Internal)
            {
                DeviceNode = IopFindLegacyBusDeviceNode(Isa, 0);
            }

            continue;
        }

        if (!IsArbiterFound && DeviceNode->PhysicalDeviceObject != PhysicalDevice)
        {
            Result = IopFindResourceHandlerInfo(IOP_RES_HANDLER_TYPE_ARBITER,
                                                DeviceNode,
                                                IoDescriptorType,
                                                (PVOID *)&ResArbiterEntry);
            if (!Result)
            {
                if (IoDescriptorType <= IOP_MAX_MAIN_RESOURCE_TYPE)
                    TypesBitMask = (1 << IoDescriptorType);
                else
                    TypesBitMask = 0;

                DeviceNode->QueryArbiterMask |= TypesBitMask;

                Status = IopQueryResourceHandlerInterface(IOP_RES_HANDLER_TYPE_ARBITER,
                                                          DeviceNode->PhysicalDeviceObject,
                                                          IoDescriptorType,
                                                          &Interface);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("IopSetupArbiterAndTranslators: Status %X\n", Status);
                    DeviceNode->NoArbiterMask |= TypesBitMask;

                    if (IoDescriptorType <= IOP_MAX_MAIN_RESOURCE_TYPE)
                    {
                        ASSERT(ResArbiterEntry == NULL);
                        goto FindTranslator;
                    }

                    Interface = NULL;
                }

                ResArbiterEntry = ExAllocatePoolWithTag(PagedPool, sizeof(PI_RESOURCE_ARBITER_ENTRY), 'erpP');
                if (!ResArbiterEntry)
                {
                    DPRINT1("IopSetupArbiterAndTranslators: STATUS_INSUFFICIENT_RESOURCES\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                DPRINT("IopSetupArbiterAndTranslators: ResArbiterEntry %p\n", ResArbiterEntry);

                ResArbiterEntry->ResourcesChanged = 0;
                ResArbiterEntry->State = 0;

                InitializeListHead(&ResArbiterEntry->ActiveArbiterList);
                InitializeListHead(&ResArbiterEntry->BestConfig);
                InitializeListHead(&ResArbiterEntry->ResourceList);
                InitializeListHead(&ResArbiterEntry->BestResourceList);
                InitializeListHead(&ResArbiterEntry->DeviceArbiterList);

                InsertTailList(&DeviceNode->DeviceArbiterList, &ResArbiterEntry->DeviceArbiterList);

                ResArbiterEntry->ResourceType = IoDescriptorType;
                ResArbiterEntry->Level = DeviceNode->Level;
                ResArbiterEntry->ArbiterInterface = Interface;

                if (!Interface)
                    ResArbiterEntry = NULL;
            }

            if (ResArbiterEntry)
            {
                if (ResArbiterEntry->ArbiterInterface->Flags & 1) // FIXME
                {
                    ASSERT(FALSE);Status = STATUS_NOT_IMPLEMENTED;
                    //Status = IopCallArbiter(ResArbiterEntry, ArbiterActionQueryArbitrate, ReqDescriptor->TranslatedReqDesc, NULL, NULL);
                    if (!NT_SUCCESS(Status))
                        IsArbiterFound = FALSE;
                }
                else
                {
                    IsArbiterFound = TRUE;

                    ReqDescriptor->ArbiterEntry = ResArbiterEntry;
                    ResArbiterEntry->State = 0;
                    ResArbiterEntry->ResourcesChanged = 0;
                }
            }
        }

FindTranslator:

        if (!IsFindTranslator)
            goto Next;

        Result = IopFindResourceHandlerInfo(IOP_RES_HANDLER_TYPE_TRANSLATOR,
                                            DeviceNode,
                                            IoDescriptorType,
                                            (PVOID *)&TranslatorEntry);
        if (!Result)
        {
            BOOLEAN IsFind = FALSE;

            if (IoDescriptorType <= IOP_MAX_MAIN_RESOURCE_TYPE)
                TypesBitMask = (1 << IoDescriptorType);
            else
                TypesBitMask = 0;

            Status = IopQueryResourceHandlerInterface(IOP_RES_HANDLER_TYPE_TRANSLATOR,
                                                      DeviceNode->PhysicalDeviceObject,
                                                      IoDescriptorType,
                                                      &Interface);

            DeviceNode->QueryTranslatorMask |= TypesBitMask;

            if (!NT_SUCCESS(Status))
            {
                DPRINT("IopSetupArbiterAndTranslators: Status %X\n", Status);
                DeviceNode->NoTranslatorMask |= TypesBitMask;

                if (IoDescriptorType > IOP_MAX_MAIN_RESOURCE_TYPE)
                    Interface = NULL;
                else
                    IsFind = TRUE;
            }

            if (!IsFind)
            {
                TranslatorEntry = ExAllocatePoolWithTag(PagedPool, sizeof(PI_RESOURCE_TRANSLATOR_ENTRY), 'erpP');
                if (!TranslatorEntry)
                {
                    DPRINT1("IopSetupArbiterAndTranslators: STATUS_INSUFFICIENT_RESOURCES\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                DPRINT("IopSetupArbiterAndTranslators: TranslatorEntry %p\n", TranslatorEntry);

                TranslatorEntry->ResourceType = IoDescriptorType;
                TranslatorEntry->TranslatorInterface = Interface;
                TranslatorEntry->DeviceNode = DeviceNode;

                InitializeListHead(&TranslatorEntry->DeviceTranslatorList);
                InsertTailList(&DeviceNode->DeviceTranslatorList, &TranslatorEntry->DeviceTranslatorList);

                DPRINT("IopSetupArbiterAndTranslators: (%p, %p) (%p, %p)\n",
                       DeviceNode, &DeviceNode->DeviceTranslatorList, TranslatorEntry, &TranslatorEntry->DeviceTranslatorList);

                if (!Interface)
                {
                    TranslatorEntry = NULL;
                    DPRINT("SetupArbiterAndTranslators: Iface NULL\n");
                }
                else
                {
                    DPRINT("SetupArbiterAndTranslators: Iface %p\n", Interface);
                }
            }
        }

        if (TranslatorEntry)
            IsTranslatorFound = TRUE;

        if (IsArbiterFound || !TranslatorEntry)
            goto Next;

        Status = IopTranslateAndAdjustReqDesc(ReqDescriptor->TranslatedReqDesc, TranslatorEntry, &TranslatedReqDesc);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopSetupArbiterAndTranslators: TranslationAndAdjusted failed (%X)\n", Status);
            return Status;
        }

        ASSERT(TranslatedReqDesc);

        IoDescriptorType = TranslatedReqDesc->ReqEntry.IoDescriptor->Type;
        TranslatedReqDesc->TranslatedReqDesc = ReqDescriptor->TranslatedReqDesc;
        ReqDescriptor->TranslatedReqDesc = TranslatedReqDesc;

        if (Status == STATUS_TRANSLATION_COMPLETE)
            IsFindTranslator = FALSE;
Next:
        DeviceNode = DeviceNode->Parent;
    }

    if (IsArbiterFound)
        return STATUS_SUCCESS;

    DPRINT("IopSetupArbiterAndTranslators: no arbiter for resource type %X \n", IoDescriptorType);

    ASSERT(IsArbiterFound);

    return STATUS_RESOURCE_TYPE_NOT_FOUND;
}

VOID
NTAPI
IopFreeReqAlternative(
    _In_ PPNP_REQ_ALT_LIST AltList)
{
    PPNP_REQ_DESCRIPTOR * ReqDescriptors;
    PPNP_REQ_DESCRIPTOR Current;
    PPNP_REQ_DESCRIPTOR reqDesc;
    PIO_RESOURCE_DESCRIPTOR IoDescriptor;
    ULONG ix;

    PAGED_CODE();
    DPRINT("IopFreeReqAlternative: AltList - %p\n", AltList);

    if (!AltList)
    {
        return;
    }

    ReqDescriptors = AltList->ReqDescriptors;

    for (ix = 0; ix < AltList->CountDescriptors; ix++)
    {
        Current = (*ReqDescriptors)->TranslatedReqDesc;

        while (Current && !Current->AltList)
        {
            IoDescriptor = Current->ReqEntry.IoDescriptor;

            if (IoDescriptor)
            {
                DPRINT("IopFreeReqAlternative: Free IoDescriptor - %p\n", IoDescriptor);
                ExFreePool(IoDescriptor);
            }

            reqDesc = Current;
            Current = Current->TranslatedReqDesc;

            DPRINT("IopFreeReqAlternative: Free reqDesc - %p\n", reqDesc);
            ExFreePool(reqDesc);
        }

        ReqDescriptors++;
    }
}

VOID
NTAPI
IopFreeReqList(
    _In_ PPNP_REQ_LIST ReqList)
{
    PPNP_REQ_ALT_LIST * AltList;
    ULONG ix;

    PAGED_CODE();
    DPRINT("IopFreeReqList: ReqList - %p\n", ReqList);

    if (!ReqList)
    {
        return;
    }

    AltList = ReqList->AltLists;

    for (ix = 0; ix < ReqList->Count; ix++)
    {
        IopFreeReqAlternative(*AltList);
        AltList++;
    }

    DPRINT("IopFreeReqList: Free ReqList - %p\n", ReqList);
    ExFreePoolWithTag(ReqList, 'erpP');
}

VOID
NTAPI
IopFreeResourceRequirementsForAssignTable(
    _In_ PPNP_RESOURCE_REQUEST requestTable,
    _In_ PPNP_RESOURCE_REQUEST requestTableEnd)
{
    ULONG Count;
    ULONG ix;

    PAGED_CODE();
    DPRINT("IopFreeResourceRequirementsForAssignTable: requestTable - %p, requestTableEnd - %p\n",
           requestTable, requestTableEnd);

    Count = ((ULONG_PTR)requestTableEnd - (ULONG_PTR)requestTable - 1) /
            sizeof(PNP_RESOURCE_REQUEST) + 1;

    for (ix = 0; ix < Count; ix++)
    {
        IopFreeReqList(requestTable[ix].ReqList);
        requestTable[ix].ReqList = NULL;

        if (requestTable[ix].Flags & 2)
        {
            if (requestTable[ix].ResourceRequirements)
            {
                ExFreePool(requestTable[ix].ResourceRequirements);
                requestTable[ix].ResourceRequirements = NULL;
            }
        }
    }
}

NTSTATUS
NTAPI
IopResourceRequirementsListToReqList(
    _In_ PPNP_RESOURCE_REQUEST ResRequest,
    _Out_ PPNP_REQ_LIST * OutReqList)
{
    PIO_RESOURCE_REQUIREMENTS_LIST IoResources;
    ULONG_PTR IoResourcesEnd;
    PIO_RESOURCE_LIST IoList;
    ULONG ListsCount;
    PIO_RESOURCE_DESCRIPTOR IoDescriptor;
    PIO_RESOURCE_DESCRIPTOR IoDescriptorEnd;
    PIO_RESOURCE_DESCRIPTOR FirstIoDescriptor;
    ULONG IoDescCount;
    ULONG AltIoDescCount;
    ULONG PrefIoDescCount;
    ULONG ListPoolSize;
    PPNP_REQ_ALT_LIST AltListsPool;
    ULONG AltListsPoolSize;
    PPNP_REQ_DESCRIPTOR ReqDescsPool;
    ULONG ReqDescsPoolSize;
    PPNP_REQ_LIST ReqList;
    ULONG ReqListSize;
    PPNP_REQ_ALT_LIST * pAltLists; // pointer to array AlternativeLists pointers
    PPNP_REQ_ALT_LIST CurrentAltList;
    PPNP_REQ_DESCRIPTOR * pReqDescs; // pointer to array ReqDescriptors pointers
    PPNP_REQ_DESCRIPTOR ReqDesc;
    PPNP_REQ_RESOURCE_ENTRY ReqEntry;
    PPNP_REQ_DESCRIPTOR CurrentReqDesc;
    ULONG_PTR EndPtr;
    ULONG BusNumber;
    ULONG CountAlts;
    INTERFACE_TYPE InterfaceType;
    ULONG IoDescriptorsCount;
    ULONG ix;
    ULONG jx;
    ULONG kx;
    NTSTATUS Status;
    BOOLEAN NoDefaultOrPreferredDescs;
    UCHAR Type;

    PAGED_CODE();

    *OutReqList = NULL;
    IoResources = ResRequest->ResourceRequirements;

    DPRINT("IopResourceRequirementsListToReqList: ResRequest - %p, AlternativeLists - %X\n",
           ResRequest, IoResources->AlternativeLists);

    if (!IoResources->AlternativeLists)
    {
        DPRINT("IopResourceRequirementsListToReqList: AlternativeLists == 0\n");
        return STATUS_SUCCESS;
    }

    IoResourcesEnd = (ULONG_PTR)IoResources + IoResources->ListSize;
    IoList = &IoResources->List[0];

    IoDescCount = 0;
    AltIoDescCount = 0;

    for (ix = 0; ix < IoResources->AlternativeLists; ix++)
    {
        if (IoList->Count == 0)
        {
            DPRINT("IopResourceRequirementsListToReqList: IoList->Count == 0\n");
            return STATUS_SUCCESS;
        }

        IoDescriptor = &IoList->Descriptors[0];

        IoDescriptorEnd = (PIO_RESOURCE_DESCRIPTOR)
                          ((ULONG_PTR)IoList + sizeof(IO_RESOURCE_LIST) +
                           (IoList->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR));

        if (IoDescriptor > IoDescriptorEnd ||
            (ULONG_PTR)IoDescriptor > IoResourcesEnd ||
            (ULONG_PTR)IoDescriptorEnd > IoResourcesEnd)
        {
            DPRINT("IopResourceRequirementsListToReqList: Invalid ResReqList\n");
            return STATUS_INVALID_PARAMETER;
        }

        /* ConfigData descriptors are per-LogConf
           and should be at the beginning of an AlternativeList */

        if (IoDescriptor->Type == CmResourceTypeConfigData)
        {
            DPRINT("IopResourceRequirementsListToReqList: ConfigData descriptor\n");
            IoDescriptor++;
        }

        FirstIoDescriptor = IoDescriptor;

        NoDefaultOrPreferredDescs = TRUE;

        while (IoDescriptor < IoDescriptorEnd)
        {
            Type = IoDescriptor->Type;

            if (Type == CmResourceTypeConfigData)
            {
                DPRINT("IopResourceRequirementsListToReqList: Invalid ResReq list!\n");
                return STATUS_INVALID_PARAMETER;
            }

            if ( Type == CmResourceTypeDevicePrivate )
            {
                while (IoDescriptor < IoDescriptorEnd &&
                       IoDescriptor->Type == CmResourceTypeDevicePrivate)
                {
                    if ( IoDescriptor == FirstIoDescriptor )
                    {
                        DPRINT("IopResourceRequirementsListToReqList: FirstIoDescriptor can not be a DevicePrivate descriptor.\n");
                        return STATUS_INVALID_PARAMETER;
                    }

                    IoDescCount++;
                    IoDescriptor++;
                }

                NoDefaultOrPreferredDescs = TRUE;
                continue;
            }

            IoDescCount++;

            if (Type & CmResourceTypeConfigData || Type == CmResourceTypeNull)
            {
                if (Type == 0xF0)
                {
                    DPRINT("IopResourceRequirementsListToReqList: Type == 0xF0\n");
                    IoDescCount--;
                }

                IoDescriptor->Option = IO_RESOURCE_PREFERRED;
                IoDescriptor++;

                NoDefaultOrPreferredDescs = TRUE;
                continue;
            }

            if (IoDescriptor->Option & IO_RESOURCE_ALTERNATIVE)
            {
                if (NoDefaultOrPreferredDescs)
                {
                    DPRINT("IopResourceRequirementsListToReqList: Alternative without Default or Preferred!\n");
                    return STATUS_INVALID_PARAMETER;
                }

                AltIoDescCount++;
                DPRINT("IopResourceRequirementsListToReqList: AltIoDescCount - %X\n",
                       AltIoDescCount);
            }
            else
            {
                NoDefaultOrPreferredDescs = FALSE;
            }

            IoDescriptor++;
        }

        ASSERT(IoDescriptor == IoDescriptorEnd);
        IoList = (PIO_RESOURCE_LIST)IoDescriptorEnd;
    }

    Status = STATUS_UNSUCCESSFUL;

    ListsCount = IoResources->AlternativeLists;
    PrefIoDescCount = IoDescCount - AltIoDescCount;

    ListPoolSize = FIELD_OFFSET(PNP_REQ_LIST, AltLists) +
                   ListsCount * sizeof(PPNP_REQ_ALT_LIST);
    DPRINT("IopResourceRequirementsListToReqList: ListsCount - %X, ListPoolSize - %X\n",
           ListsCount, ListPoolSize);

    AltListsPoolSize = ListsCount * (FIELD_OFFSET(PNP_REQ_ALT_LIST, ReqDescriptors) +
                       PrefIoDescCount * sizeof(PPNP_REQ_DESCRIPTOR));
    DPRINT("IopResourceRequirementsListToReqList: AltListsPoolSize  - %X\n",
           AltListsPoolSize);

    ReqDescsPoolSize = PrefIoDescCount * sizeof(PNP_REQ_DESCRIPTOR);
    DPRINT("IopResourceRequirementsListToReqList: PrefIoDescCount - %X, ReqDescsPoolSize - %X\n",
           PrefIoDescCount, ReqDescsPoolSize);

    ReqListSize = ListPoolSize + AltListsPoolSize + ReqDescsPoolSize;

    ReqList = ExAllocatePoolWithTag(PagedPool, ReqListSize, 'erpP');

    if (!ReqList)
    {
        ASSERT(FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(ReqList, ReqListSize);
    DPRINT("IopResourceRequirementsListToReqList: ReqList - %X, ReqListSize - %X\n",
           ReqList, ReqListSize);

    pAltLists = &ReqList->AltLists[0];
    DPRINT("IopResourceRequirementsListToReqList: pAltLists - %X, ListsCount - %X\n",
           pAltLists, ListsCount);

    AltListsPool = (PPNP_REQ_ALT_LIST)((ULONG_PTR)ReqList + ListPoolSize);
    DPRINT("IopResourceRequirementsListToReqList: AltListsPool - %X, AltListsPoolSize - %X\n",
           AltListsPool, AltListsPoolSize);

    ReqDescsPool = (PPNP_REQ_DESCRIPTOR)((ULONG_PTR)AltListsPool + AltListsPoolSize);
    DPRINT("IopResourceRequirementsListToReqList: ReqDescsPool - %X, ReqDescsPoolSize - %X\n",
           ReqDescsPool, ReqDescsPoolSize);

    InterfaceType = IoResources->InterfaceType;

    if (InterfaceType == InterfaceTypeUndefined)
    {
        InterfaceType = PnpDefaultInterfaceType;
    }

    ReqList->InterfaceType = InterfaceType;
    ReqList->BusNumber = IoResources->BusNumber;
    ReqList->ResRequest = ResRequest;
    ReqList->AltList1 = NULL;
    ReqList->Count = ListsCount;

    IoList = &IoResources->List[0];

    InterfaceType = IoResources->InterfaceType;
    BusNumber = IoResources->BusNumber;

    CountAlts = 0;
    CurrentReqDesc = ReqDescsPool;

    EndPtr = (ULONG_PTR)AltListsPool;

    for (ix = 0; ix < ListsCount; ix++)
    {
        DPRINT("IopResourceRequirementsListToReqList: ix - %X\n", ix);

        IoDescriptorsCount = IoList->Count;
        IoDescriptor = &IoList->Descriptors[0];
        IoDescriptorEnd = &IoDescriptor[IoDescriptorsCount];

        CurrentAltList = (PPNP_REQ_ALT_LIST)EndPtr;
        *pAltLists = CurrentAltList;
        pAltLists++;

        CurrentAltList->ListNumber = CountAlts++;
        CurrentAltList->CountDescriptors = 0;
        CurrentAltList->ReqList = ReqList;

        if (IoDescriptor->Type == CmResourceTypeConfigData)
        {
            CurrentAltList->ConfigPriority = IoDescriptor->u.ConfigData.Priority;
            IoDescriptor++;
        }
        else
        {
            CurrentAltList->ConfigPriority = LCPRI_NORMAL;
        }

        pReqDescs = &CurrentAltList->ReqDescriptors[0];
        EndPtr = (ULONG_PTR)pReqDescs;

        if (IoDescriptor >= IoDescriptorEnd)
        {
            goto NextList;
        }

        for (jx = 0; IoDescriptor < IoDescriptorEnd; jx++)
        {
            if (IoDescriptor->Type == 0xF0)
            {
                InterfaceType = IoDescriptor->u.DevicePrivate.Data[0];

                if (InterfaceType == InterfaceTypeUndefined)
                {
                    InterfaceType = PnpDefaultInterfaceType;
                }

                BusNumber = IoDescriptor->u.DevicePrivate.Data[1];

                IoDescriptor++;
                continue;
            }

            DPRINT("IopResourceRequirementsListToReqList: jx - %X\n", jx);

            ReqDesc = CurrentReqDesc;
            ReqDesc->IsArbitrated = (IoDescriptor->Type != CmResourceTypeNull);
            ReqDesc->AltList = CurrentAltList;
            ReqDesc->InterfaceType = InterfaceType;
            ReqDesc->TranslatedReqDesc = ReqDesc;
            ReqDesc->BusNumber = BusNumber;
            ReqDesc->DescriptorsCount = 0;
            ReqDesc->DevicePrivateIoDesc = NULL;
            ReqDesc->DescNumber = jx;

            ReqEntry = &ReqDesc->ReqEntry;
            ReqEntry->InterfaceType = InterfaceType;
            ReqEntry->SlotNumber = IoResources->SlotNumber;
            ReqEntry->BusNumber = IoResources->BusNumber;
            ReqEntry->PhysicalDevice = ResRequest->PhysicalDevice;
            ReqEntry->AllocationType = ResRequest->AllocationType;
            ReqEntry->IoDescriptor = IoDescriptor;
            ReqEntry->pCmDescriptor = &ReqDesc->ReqEntry.CmDescriptor;
            ReqEntry->Count = 0;
            ReqEntry->Reserved1 = CurrentAltList->ConfigPriority == LCPRI_BOOTCONFIG;
            ReqEntry->Reserved2 = 0;
            ReqEntry->Reserved4 = -1;

            InitializeListHead(&ReqEntry->Link);

            CurrentAltList->CountDescriptors++;
            *pReqDescs = ReqDesc;
            pReqDescs++;
            CurrentReqDesc++;
            EndPtr = (ULONG_PTR)pReqDescs;

            if (ReqDesc->IsArbitrated)
            {
                NTSTATUS status;

                ASSERT(!(IoDescriptor->Option & IO_RESOURCE_ALTERNATIVE));

                ReqDesc->ReqEntry.CmDescriptor.Type = 7; // ?
                ReqDesc->ReqEntry.Count++;

                for (kx = ReqDesc->ReqEntry.Count; ; ReqDesc->ReqEntry.Count = kx)
                {
                    IoDescriptor++;

                    if (IoDescriptor >= IoDescriptorEnd)
                    {
                        break;
                    }

                    if (IoDescriptor->Type == CmResourceTypeDevicePrivate)
                    {
                        DPRINT("IopResourceRequirementsListToReqList: kx - %X\n", kx);
                        ReqDesc->DevicePrivateIoDesc = IoDescriptor;

                        while (IoDescriptor < IoDescriptorEnd &&
                               IoDescriptor->Type == CmResourceTypeDevicePrivate)
                        {
                            ReqDesc->DescriptorsCount++;
                            IoDescriptor++;
                        }

                        break;
                    }

                    if (!(IoDescriptor->Option & IO_RESOURCE_ALTERNATIVE))
                    {
                        break;
                    }

                    kx++;
                }

                IopDumpReqDescriptor(ReqDesc, jx+1);
                status = IopSetupArbiterAndTranslators(ReqDesc);
                IopDumpReqDescriptor(ReqDesc, jx+1);

                if (!NT_SUCCESS(status))
                {
                    DPRINT("IopResourceRequirementsListToReqList: Unable to setup Arbiter and Translators\n");

                    CountAlts--;
                    pAltLists--;
                    ReqList->Count--;

                    ASSERT(FALSE);
                    //IopFreeReqAlternative(CurrentAltList);

                    Status = status;
                    break;
                }
            }
            else
            {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;

                CmDescriptor = &ReqDesc->ReqEntry.CmDescriptor;
                CmDescriptor->Type = IoDescriptor->Type;
                CmDescriptor->ShareDisposition = IoDescriptor->ShareDisposition;
                CmDescriptor->Flags = IoDescriptor->Flags;

                CmDescriptor->u.DevicePrivate.Data[0] = IoDescriptor->u.DevicePrivate.Data[0];
                CmDescriptor->u.DevicePrivate.Data[1] = IoDescriptor->u.DevicePrivate.Data[1];
                CmDescriptor->u.DevicePrivate.Data[2] = IoDescriptor->u.DevicePrivate.Data[2];

                IoDescriptor++;
            }
        }

NextList:
        IoList = (PIO_RESOURCE_LIST)IoDescriptorEnd;
    }

    if (CountAlts != 0)
    {
        *OutReqList = ReqList;
        return STATUS_SUCCESS;
    }

    IopFreeReqList(ReqList);

    if (Status != STATUS_SUCCESS)
    {
        return Status;
    }

    *OutReqList = ReqList;

    return STATUS_SUCCESS;
}

int
__cdecl 
IopCompareReqAlternativePriority(
    const void * x,
    const void * y)
{
    PPNP_REQ_ALT_LIST * pAltList1;
    PPNP_REQ_ALT_LIST * pAltList2;
    int Result;

    pAltList1 = (PPNP_REQ_ALT_LIST *)x;
    pAltList2 = (PPNP_REQ_ALT_LIST *)y;

    PAGED_CODE();
    DPRINT("IopCompareReqAlternativePriority: x - %p, y - %p\n", x, y);

    if ((*pAltList2)->ConfigPriority != (*pAltList1)->ConfigPriority)
    {
        if ((*pAltList2)->ConfigPriority < (*pAltList1)->ConfigPriority)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    if ((*pAltList1)->Priority > (*pAltList2)->Priority)
    {
        return 1;
    }

    if ((*pAltList1)->Priority < (*pAltList2)->Priority)
    {
        return -1;
    }

    ASSERT(FALSE);

    if ((*pAltList1) < (*pAltList2))
    {
        Result = -1;
    }
    else
    {
        Result = 1;
    }

    return Result;
}

VOID
NTAPI
IopRearrangeReqList(
    _Inout_ PPNP_REQ_LIST ReqList)
{
    PPNP_REQ_ALT_LIST * AltLists;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    ULONG ix;

    PAGED_CODE();
    DPRINT("IopRearrangeReqList: ReqList - %p, ReqList->Count - %X\n",
           ReqList, ReqList->Count);

    if (ReqList->Count > 1)
    {
        AltLists = ReqList->AltLists;

        for (ix = 0; ix < ReqList->Count; ix++, AltLists++)
        {
            (*AltLists)->Priority = ix;
        }

        qsort(ReqList->AltLists,
              ReqList->Count,
              sizeof(PPNP_REQ_ALT_LIST),
              IopCompareReqAlternativePriority);
    }

    for (AltLists = ReqList->AltLists;
         AltLists < &ReqList->AltLists[ReqList->Count];
         AltLists++)
    {
        if ((*AltLists)->ConfigPriority > LCPRI_LASTSOFTCONFIG)
        {
            break;
        }
    }

    DeviceObject = ReqList->ResRequest->PhysicalDevice;
    ASSERT(DeviceObject);
    DeviceNode = IopGetDeviceNode(DeviceObject);

    if (AltLists == ReqList->AltLists)
    {
        DPRINT("IopRearrangeReqList: Invalid priorities for %wZ\n",
               &DeviceNode->InstancePath);

        ReqList->AltList2 = NULL;

        DPRINT("IopRearrangeReqList: DeviceNode - %p, DeviceNode->BootResources - %p, AltList2 - NULL\n",
               DeviceNode, DeviceNode->BootResources);
    }
    else
    {
        ReqList->AltList2 = AltLists;

        DPRINT("IopRearrangeReqList: DeviceNode - %p, DeviceNode->BootResources - %p, AltList2 - %p\n",
               DeviceNode, DeviceNode->BootResources, ReqList->AltList2);
    }
}

NTSTATUS
NTAPI
IopGetResourceRequirementsForAssignTable(
    _In_ PPNP_RESOURCE_REQUEST RequestTable,
    _In_ PPNP_RESOURCE_REQUEST RequestTableEnd,
    _Out_ PULONG OutDeviceCount)
{
    PPNP_RESOURCE_REQUEST ResRequest;
    PDEVICE_OBJECT PhysicalDevice;
    PDEVICE_NODE DeviceNode;
    PPNP_REQ_LIST ReqList;
    ULONG ListSize;
    ULONG Count;
    ULONG ix;
    NTSTATUS Status;

    PAGED_CODE();

    *OutDeviceCount = 0;

    if (RequestTable >= RequestTableEnd)
    {
        ASSERT(FALSE);

        if (*OutDeviceCount != 0)
        {
            return STATUS_SUCCESS;
        }

        return STATUS_UNSUCCESSFUL;
    }

    Count = ((ULONG_PTR)RequestTableEnd - (ULONG_PTR)RequestTable - 1) /
            sizeof(PNP_RESOURCE_REQUEST) + 1;

    DPRINT("IopGetResourceRequirementsForAssignTable: RequestTable - %p, RequestTableEnd - %p, Count - %X\n",
           RequestTable, RequestTableEnd, Count);

    for (ix = 0; ix < Count; ix++)
    {
        ResRequest = &RequestTable[ix];

        DPRINT("IopGetResourceRequirementsForAssignTable: [%X] ResRequest - %p\n",
               ix, ResRequest);

        DeviceNode = NULL;
        ResRequest->ReqList = NULL;

        if ((ResRequest->Flags & 0x20) == 0)
        {
              PhysicalDevice = ResRequest->PhysicalDevice;
              ASSERT(PhysicalDevice);

              ResRequest->ResourceAssignment = NULL;
              ResRequest->TranslatedResourceAssignment = NULL;

              DeviceNode = IopGetDeviceNode(PhysicalDevice);
              ASSERT(DeviceNode);

              if (DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED)
              {
                  if (DeviceNode->ResourceRequirements)
                  {
                      ExFreePool(DeviceNode->ResourceRequirements);

                      DeviceNode->ResourceRequirements = NULL;
                      DeviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;

                      ResRequest->Flags |= 0x400;
                  }
              }

              if (!ResRequest->ResourceRequirements)
              {
                  if (!DeviceNode->ResourceRequirements ||
                      (DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED))
                  {
                      DPRINT("IopGetResourceRequirementsForAssignTable: Query ResourceRequirements for %wZ\n",
                             &DeviceNode->InstancePath);

                      Status = IopQueryDeviceRequirements(ResRequest->PhysicalDevice,
                                                          &ResRequest->ResourceRequirements,
                                                          &ListSize);
                      DPRINT("IopGetResourceRequirementsForAssignTable: List size - %X\n",
                             ListSize);

                      PipDumpResourceRequirementsList(ResRequest->ResourceRequirements, 1);

                      if (!NT_SUCCESS(Status) ||
                          !ResRequest->ResourceRequirements)
                      {
                          DPRINT("IopGetResourceRequirementsForAssignTable: Status - %X\n",
                                 Status);

                          ResRequest->Status = Status;
                          ResRequest->Flags |= 0x20;

                          continue;
                      }

                      if (DeviceNode->ResourceRequirements)
                      {
                          ExFreePool(DeviceNode->ResourceRequirements);
                          DeviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
                      }

                      DeviceNode->ResourceRequirements = ResRequest->ResourceRequirements;
                  }
                  else
                  {
                      DPRINT("IopGetResourceRequirementsForAssignTable: ResourceRequirements already exists for %wZ\n",
                             &DeviceNode->InstancePath);

                      ResRequest->ResourceRequirements = DeviceNode->ResourceRequirements;
                      ResRequest->AllocationType = ArbiterRequestPnpEnumerated;
                  }
              }

              if (ResRequest->Flags & 0x200)
              {
                  ASSERT(DeviceNode->ResourceRequirements == ResRequest->ResourceRequirements);

                  DPRINT("IopGetResourceRequirementsForAssignTable: FIXME IopFilterResourceRequirementsList()\n");
                  ASSERT(FALSE);
              }

              Status = IopResourceRequirementsListToReqList(ResRequest,
                                                            &ResRequest->ReqList);
              if (!NT_SUCCESS(Status))
              {
                  ASSERT(FALSE);

                  ResRequest->Status = Status;
                  ResRequest->Flags |= 0x20;

                  continue;
              }

              ReqList = ResRequest->ReqList;

              if (!ReqList)
              {
                  ASSERT(FALSE);

                  ResRequest->Status = Status;
                  ResRequest->Flags |= 0x20;

                  continue;
              }

              IopRearrangeReqList(ReqList);

              if (!ReqList->AltList2)
              {
                  IopFreeResourceRequirementsForAssignTable(ResRequest, &ResRequest[1]);

                  Status = STATUS_DEVICE_CONFIGURATION_ERROR;
                  ResRequest->Status = Status;
                  ResRequest->Flags |= 0x20;

                  continue;
              }

              if (ReqList->Count >= 3)
              {
                  ResRequest->Priority = ReqList->Count;
              }
              else
              {
                  ResRequest->Priority = 0;
              }

              ResRequest->Status = Status;

              (*OutDeviceCount)++;
        }
    }

    if (*OutDeviceCount != 0)
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        DPRINT("IopGetResourceRequirementsForAssignTable: return STATUS_UNSUCCESSFUL\n");
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

VOID
NTAPI
IopAddRemoveReqDescs(
    _In_ PPNP_REQ_DESCRIPTOR * ResDescriptor,
    _In_ ULONG Count,
    _Out_ PLIST_ENTRY ConfigurationList,
    _In_ BOOLEAN AddOrRemove)
{
    PPNP_REQ_LIST ResList;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    PPNP_REQ_DESCRIPTOR Descriptor;
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry;
    PLIST_ENTRY Entry;
    ULONG ix;

    PAGED_CODE();
    DPRINT("IopAddRemoveReqDescs: ResDescriptor - %p, Count - %X, AddOrRemove - %X\n",
           ResDescriptor, Count, AddOrRemove);

    if (!Count)
    {
        ASSERT(FALSE);
        return;
    }

    ResList = (*ResDescriptor)->AltList->ReqList;

    DeviceObject = ResList->ResRequest->PhysicalDevice;
    ASSERT(DeviceObject);

    DeviceNode = IopGetDeviceNode(DeviceObject);

    if (AddOrRemove)
    {
        DPRINT("IopAddRemoveReqDescs: Adding %X/%X req alt to the arbiters for %wZ\n",
               ResList->AltList1[0]->ListNumber + 1,
               ResList->Count,
               &DeviceNode->InstancePath);
    }
    else
    {
        DPRINT("IopAddRemoveReqDescs: Removing %X/%X req alt from the arbiters for %wZ\n",
               ResList->AltList1[0]->ListNumber + 1,
               ResList->Count,
               &DeviceNode->InstancePath);
    }

    for (ix = 0; ix < Count; ix++)
    {
        DPRINT("IopAddRemoveReqDescs: ix - %X\n", ix);
        Descriptor = ResDescriptor[ix];
        //PipDumpDescriptors(Descriptor, ix);

        if (!Descriptor->IsArbitrated)
        {
            DPRINT("IopAddRemoveReqDescs: ix - %X\n", ix);
            continue;
        }

        ArbiterEntry = Descriptor->ArbiterEntry;
        ASSERT(ArbiterEntry);

        if (ArbiterEntry->State & 1)
        {
            ArbiterEntry->State &= ~1;
            ASSERT(FALSE);
            ArbiterEntry->ArbiterInterface->
                ArbiterHandler(ArbiterEntry->ArbiterInterface->Context,
                               ArbiterActionRollbackAllocation,
                               NULL);
        }

        ArbiterEntry->ResourcesChanged = 1;

        if (AddOrRemove == 1)
        {
            InitializeListHead(&Descriptor->TranslatedReqDesc->ReqEntry.Link);
            InsertTailList(&ArbiterEntry->ResourceList,
                           &Descriptor->TranslatedReqDesc->ReqEntry.Link);

            if (IsListEmpty(&ArbiterEntry->ActiveArbiterList))
            {
                PPI_RESOURCE_ARBITER_ENTRY arbEntry;

                for (Entry = ConfigurationList->Flink;
                     Entry != ConfigurationList;
                     Entry = Entry->Flink)
                {
                    arbEntry = CONTAINING_RECORD(Entry, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);
                    if (arbEntry->Level >= ArbiterEntry->Level)
                    {
                        break;
                    }
                }

                InsertTailList(Entry, &ArbiterEntry->ActiveArbiterList);
            }
        }
        else
        {
            ASSERT(FALSE);
            ASSERT(!IsListEmpty(&ArbiterEntry->ResourceList));

            RemoveEntryList(&Descriptor->TranslatedReqDesc->ReqEntry.Link);
            InitializeListHead(&Descriptor->TranslatedReqDesc->ReqEntry.Link);

            if (IsListEmpty(&ArbiterEntry->ResourceList))
            {
                RemoveEntryList(&ArbiterEntry->ActiveArbiterList);
                InitializeListHead(&ArbiterEntry->ActiveArbiterList);
            }
        }
    }
}

VOID
NTAPI
IopSelectFirstConfiguration(
    _In_ PPNP_RESOURCE_REQUEST ResRequest,
    _In_ ULONG DeviceCount,
    _In_ PLIST_ENTRY ConfigurationList)
{
    PPNP_REQ_LIST ReqList;
    ULONG ix;

    PAGED_CODE();
    DPRINT("IopSelectFirstConfiguration: ResRequest - %p, DeviceCount - %X, ConfigurationList - %p\n",
           ResRequest, DeviceCount, ConfigurationList);

    for (ix = 0; ix < DeviceCount; ix++)
    {
        ReqList = ResRequest[ix].ReqList;
        ReqList->AltList1 = ReqList->AltLists;

        IopAddRemoveReqDescs(ReqList->AltLists[0]->ReqDescriptors,
                             ReqList->AltLists[0]->CountDescriptors,
                             ConfigurationList,
                             TRUE);
    }
}

NTSTATUS
NTAPI
IopTestConfiguration(
    _In_ PLIST_ENTRY ConfigurationList)
{
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry;
    PARBITER_INTERFACE ArbInterface;
    ARBITER_PARAMETERS Params;
    PLIST_ENTRY Entry;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopTestConfiguration: ConfigurationList - %p\n", ConfigurationList);

    Status = STATUS_SUCCESS;

    for (Entry = ConfigurationList->Flink;
         Entry != ConfigurationList;
         Entry = Entry->Flink)
    {
        ArbiterEntry = CONTAINING_RECORD(Entry,
                                         PI_RESOURCE_ARBITER_ENTRY,
                                         ActiveArbiterList);

        ASSERT(IsListEmpty(&ArbiterEntry->ResourceList) == FALSE);

        if (!ArbiterEntry->ResourcesChanged)
        {
            if (ArbiterEntry->State & 2)
            {
                ASSERT(FALSE);
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            continue;
        }

        ArbInterface = ArbiterEntry->ArbiterInterface;
        ASSERT(ArbInterface);

        Params.Parameters.TestAllocation.ArbitrationList = &ArbiterEntry->ResourceList;
        Params.Parameters.TestAllocation.AllocateFromCount = 0;
        Params.Parameters.TestAllocation.AllocateFrom = NULL;

        Status = ArbInterface->ArbiterHandler(ArbInterface->Context,
                                              ArbiterActionTestAllocation,
                                              &Params);
        if (!NT_SUCCESS(Status))
        {
            ArbiterEntry->State |= 2;
            break;
        }

        ArbiterEntry->State = (ArbiterEntry->State & ~2) | 1;
        ArbiterEntry->ResourcesChanged = 0;
    }

    return Status;
}

NTSTATUS
NTAPI
IopFindBestConfiguration(
    _In_ PPNP_RESOURCE_REQUEST ResRequest,
    _In_ ULONG DeviceCount,
    _In_ PLIST_ENTRY ConfigurationList)
{
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopFindBestConfiguration: ResRequest - %p, DeviceCount - %X, ConfigurationList - %X, \n",
           ResRequest, DeviceCount, ConfigurationList);
  
    InitializeListHead(ConfigurationList);

    IopSelectFirstConfiguration(ResRequest, DeviceCount, ConfigurationList);

    while (TRUE)
    {
        Status = IopTestConfiguration(ConfigurationList);

        if (NT_SUCCESS(Status))
        {
            if (DeviceCount == 1)
            {
                break;
            }
            else
            {
                DPRINT("IopFindBestConfiguration: FIXME IopComputeConfigurationPriority()\n");
                ASSERT(FALSE);
            }
        }

        DPRINT("IopFindBestConfiguration: FIXME IopSelectNextConfiguration()\n");
        ASSERT(FALSE);
        break;
    }

    if (IsListEmpty(ConfigurationList))
    {
        DPRINT("IopFindBestConfiguration: STATUS_UNSUCCESSFUL\n");
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (DeviceCount == 1)
    {
        DPRINT("IopFindBestConfiguration: STATUS_SUCCESS\n");
        return STATUS_SUCCESS;
    }

    DPRINT("IopFindBestConfiguration: FIXME IopSaveRestoreConfiguration()\n");
    ASSERT(FALSE);
    return Status;
}

VOID
NTAPI
IopCheckDataStructuresWorker(
    _In_ PDEVICE_NODE DeviceNode)
{
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry;
    PLIST_ENTRY Entry;

    PAGED_CODE();

    for (Entry = DeviceNode->DeviceArbiterList.Flink;
         Entry != &DeviceNode->DeviceArbiterList;
         Entry = Entry->Flink)
    {
        ArbiterEntry = CONTAINING_RECORD(Entry,
                                         PI_RESOURCE_ARBITER_ENTRY,
                                         DeviceArbiterList);

        if (ArbiterEntry->ArbiterInterface)
        {
            ASSERT(IsListEmpty(&ArbiterEntry->ResourceList));
            ASSERT(IsListEmpty(&ArbiterEntry->ActiveArbiterList));
        }
    }
}

VOID
NTAPI
IopCheckDataStructures(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    for (deviceNode = DeviceNode;
         deviceNode;
         deviceNode = deviceNode->Sibling)
    {
        IopCheckDataStructuresWorker(deviceNode);
    }

    for (deviceNode = DeviceNode;
         deviceNode;
         deviceNode = deviceNode->Sibling)
    {
        if (deviceNode->Child)
        {
            IopCheckDataStructures(deviceNode->Child);
        }
    }
}

NTSTATUS
NTAPI
IopCommitConfiguration(
    _In_ PLIST_ENTRY ConfigurationList)
{
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry;
    PLIST_ENTRY Entry;
    NTSTATUS commitStatus;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopCommitConfiguration: ConfigurationList - %p\n", ConfigurationList);

    Status = STATUS_SUCCESS;

    for (Entry = ConfigurationList->Flink;
         Entry != ConfigurationList;
         )
    {
        ArbiterEntry = CONTAINING_RECORD(Entry,
                                         PI_RESOURCE_ARBITER_ENTRY,
                                         ActiveArbiterList);
        Entry = Entry->Flink;

        ASSERT(!IsListEmpty(&ArbiterEntry->ResourceList));

        commitStatus = ArbiterEntry->ArbiterInterface->
                       ArbiterHandler(ArbiterEntry->ArbiterInterface->Context,
                                      ArbiterActionCommitAllocation,
                                      NULL);

        ArbiterEntry->ResourcesChanged = 0;
        ArbiterEntry->State = 0;

        Status = commitStatus;

        InitializeListHead(&ArbiterEntry->ResourceList);
        InitializeListHead(&ArbiterEntry->BestResourceList);
        InitializeListHead(&ArbiterEntry->BestConfig);
        InitializeListHead(&ArbiterEntry->ActiveArbiterList);

        if (!NT_SUCCESS(commitStatus))
        {
            ASSERT(NT_SUCCESS(commitStatus));
            break;
        }
    }

    IopCheckDataStructures(IopRootDeviceNode);
    return Status;
}

NTSTATUS
NTAPI
IopWriteAllocatedResourcesToRegistry(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PCM_RESOURCE_LIST CmResource,
    _In_ ULONG DataSize)
{
    UNICODE_STRING AllocConfigName = RTL_CONSTANT_STRING(L"AllocConfig");
    UNICODE_STRING ControlName = RTL_CONSTANT_STRING(L"Control");
    HANDLE InstanceHandle;
    HANDLE KeyHandle;
    NTSTATUS Status;

    DPRINT("IopWriteAllocatedResourcesToRegistry: [%p] CmResource %p, DataSize %X\n", DeviceNode, CmResource, DataSize);

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    Status = PnpDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject, &InstanceHandle, KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = IopCreateRegistryKeyEx(&KeyHandle,
                                    InstanceHandle,
                                    &ControlName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_VOLATILE,
                                    NULL);
    ZwClose(InstanceHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopWriteAllocatedResourcesToRegistry: Status %X\n", Status);
        goto Exit;
    }

    if (CmResource)
        Status = ZwSetValueKey(KeyHandle, &AllocConfigName, 0, REG_RESOURCE_LIST, CmResource, DataSize);
    else
        Status = ZwDeleteValueKey(KeyHandle, &AllocConfigName);

    ZwClose(KeyHandle);

Exit:

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    return Status;
}

NTSTATUS
NTAPI
IopParentToRawTranslation(
    _In_ PPNP_REQ_DESCRIPTOR ReqDesc)
{
    PPNP_REQ_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    DPRINT("IopParentToRawTranslation: ReqDesc %p\n", ReqDesc);

    Status = STATUS_SUCCESS;

    if (ReqDesc->ReqEntry.Count &&
        ReqDesc->ReqEntry.CmDescriptor.Type == 7)
    {
        DPRINT("Invalid ReqDesc for parent-to-raw translation\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (ReqDesc->AltList)
        return Status;

    Descriptor = ReqDesc->TranslatedReqDesc;

    Status = ReqDesc->TranslatorEntry->TranslatorInterface->
             TranslateResources(ReqDesc->ArbiterEntry->ArbiterInterface->Context,
                                ReqDesc->ReqEntry.pCmDescriptor,
                                ArbiterActionRetestAllocation,
                                Descriptor->ReqEntry.Count,
                                Descriptor->ReqEntry.IoDescriptor,
                                Descriptor->ReqEntry.PhysicalDevice,
                                Descriptor->ReqEntry.pCmDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopParentToRawTranslation: Status %X\n", Status);
        return Status;
    }

    ASSERT(Status != STATUS_TRANSLATION_COMPLETE);

    Status = IopParentToRawTranslation(Descriptor);
    return Status;
}

NTSTATUS
NTAPI 
IopChildToRootTranslation(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR InCmDesc,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR *OutAssigedCmDesc)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR AssignedCmDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR TempCmDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedCmDesc;
    PPI_RESOURCE_TRANSLATOR_ENTRY TranslatorEntry;
    PTRANSLATOR_INTERFACE TranslatorInterface;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PLIST_ENTRY HeadTranslators;
    PLIST_ENTRY Entry;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN IsComlete = FALSE;
    BOOLEAN IsNotFoundLegacyBus;

    IsNotFoundLegacyBus = AllocationType == ArbiterRequestHalReported;
    DPRINT("IopChildToRootTranslation: [%p] Iface %X Bus %X AllocType %X InCmDesc %p, HalRequest %X\n",
           DeviceNode, InterfaceType, BusNumber, AllocationType, InCmDesc, IsNotFoundLegacyBus);

    AssignedCmDesc = ExAllocatePoolWithTag(PagedPool, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR), 'erpP');
    if (!AssignedCmDesc)
    {
        DPRINT1("IopChildToRootTranslation: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("IopChildToRootTranslation: AssignedCmDesc %p\n", AssignedCmDesc);

    TranslatedCmDesc = ExAllocatePoolWithTag(PagedPool, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR), 'erpP');
    if (!TranslatedCmDesc)
    {
        DPRINT1("IopChildToRootTranslation: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(AssignedCmDesc, 'erpP');
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("IopChildToRootTranslation: TranslatedCmDesc %p\n", TranslatedCmDesc);

    RtlCopyMemory(AssignedCmDesc, InCmDesc, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    if (DeviceNode)
    {
        PhysicalDeviceObject = DeviceNode->PhysicalDeviceObject;
    }
    else
    {
        PhysicalDeviceObject = NULL;
        DeviceNode = IopFindLegacyBusDeviceNode(InterfaceType, BusNumber);
    }

    while (DeviceNode && !IsComlete)
    {
        if (DeviceNode == IopRootDeviceNode && !IsNotFoundLegacyBus)
        {
            DPRINT("Node == IopRootDeviceNode && IsNotFoundLegacyBus == FALSE\n");

            IsNotFoundLegacyBus = TRUE;

            DeviceNode = IopFindLegacyBusDeviceNode(InterfaceType, BusNumber);

            if ((DeviceNode == IopRootDeviceNode) && (InterfaceType == Internal))
                DeviceNode = IopFindLegacyBusDeviceNode(Isa, 0);

            DPRINT("[%p] Service '%wZ', Instance '%wZ'\n", DeviceNode, &DeviceNode->ServiceName, &DeviceNode->InstancePath);
            continue;
        }

        DPRINT("[%p] Service '%wZ', Instance '%wZ'\n", DeviceNode, &DeviceNode->ServiceName, &DeviceNode->InstancePath);

        if (IsListEmpty(&DeviceNode->DeviceTranslatorList))
            DPRINT("empty DeviceTranslatorList(%X)\n", &DeviceNode->DeviceTranslatorList);

        HeadTranslators = &DeviceNode->DeviceTranslatorList;
        DPRINT("IopChildToRootTranslation: HeadTranslators %X\n", HeadTranslators);

        for (Entry = HeadTranslators->Flink; (Entry != HeadTranslators); Entry = Entry->Flink)
        {
            TranslatorEntry = CONTAINING_RECORD(Entry, PI_RESOURCE_TRANSLATOR_ENTRY, DeviceTranslatorList);

            if (TranslatorEntry->ResourceType != InCmDesc->Type)
            {
                DPRINT("Entry %X TranslatorEntry %p, Type %X\n", Entry, TranslatorEntry, TranslatorEntry->ResourceType);
                continue;
            }

            if (!TranslatorEntry->TranslatorInterface)
            {
                DPRINT1("No TranslatorInterface for this entry\n");
                break;
            }

            TranslatorInterface = TranslatorEntry->TranslatorInterface;

            DPRINT("Found TranslatorEntry %p (Type %X)\n", TranslatorEntry, TranslatorEntry->ResourceType);
            DPRINT("TranslatorInterface %p, PDO %p, Context %X\n", TranslatorInterface, PhysicalDeviceObject, TranslatorInterface->Context);

            Status = TranslatorInterface->TranslateResources(TranslatorInterface->Context,
                                                             AssignedCmDesc,
                                                             0,
                                                             0,
                                                             NULL,
                                                             PhysicalDeviceObject,
                                                             TranslatedCmDesc);
            if (NT_SUCCESS(Status))
            {
                DPRINT("IopChildToRootTranslation: Translation ok. (%X, %X)\n", DeviceNode, DeviceNode->PhysicalDeviceObject);

                DPRINT("IopChildToRootTranslation: [Assigned]   Resource Type %X Data %X %X %X\n",
                       AssignedCmDesc->Type, AssignedCmDesc->u.Generic.Start.LowPart, AssignedCmDesc->u.Generic.Start.HighPart,
                       AssignedCmDesc->u.Generic.Length);

                DPRINT("IopChildToRootTranslation: [Translated] Resource Type %X Data %X %X %X\n",
                       TranslatedCmDesc->Type, TranslatedCmDesc->u.Generic.Start.LowPart, TranslatedCmDesc->u.Generic.Start.HighPart,
                       TranslatedCmDesc->u.Generic.Length);

                TempCmDesc = AssignedCmDesc;
                AssignedCmDesc = TranslatedCmDesc;
                TranslatedCmDesc = TempCmDesc;

                if (Status == STATUS_TRANSLATION_COMPLETE)
                    IsComlete = TRUE;

                break;
            }

            DPRINT("Child to Root Translation failed. Node %X PDO %X Type %X Data %X %X %X\n",
                   DeviceNode, DeviceNode->PhysicalDeviceObject, AssignedCmDesc->Type, AssignedCmDesc->u.Generic.Start.LowPart,
                   AssignedCmDesc->u.Generic.Start.HighPart, AssignedCmDesc->u.Generic.Length);

            ASSERT(FALSE); // IoDbgBreakPointEx();

            ExFreePoolWithTag(AssignedCmDesc, 'erpP');
            goto Exit;
        }

        DeviceNode = DeviceNode->Parent;
    }

    *OutAssigedCmDesc = AssignedCmDesc;

Exit:

    ExFreePoolWithTag(TranslatedCmDesc, 'erpP');

    DPRINT("IopChildToRootTranslation: return %X\n", Status);
    return Status;
}

NTSTATUS
NTAPI
IopBuildCmResourceList(
    _In_ PPNP_RESOURCE_REQUEST RequestTable)
{
    UNICODE_STRING ResourceMapKeyName = RTL_CONSTANT_STRING(IO_REG_KEY_RESOURCEMAP);
    UNICODE_STRING Pnp_ManagerName = RTL_CONSTANT_STRING(L"PnP Manager");
    UNICODE_STRING PnpManagerName = RTL_CONSTANT_STRING(L"PnpManager");
    UNICODE_STRING Destination;
    OBJECT_NAME_INFORMATION_EX ObjNameInfo;
    PPNP_REQ_LIST ReqList;
    PPNP_REQ_ALT_LIST AltList;
    PCM_RESOURCE_LIST CmTranslated;
    PCM_RESOURCE_LIST CmRaw;
    PIO_RESOURCE_DESCRIPTOR DevicePrivateIoDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR RawDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR AssignlDesc;
    ULONG_PTR RawDescEnd;
    ULONG_PTR TranslatedDescEnd;
    PPNP_REQ_DESCRIPTOR * AltListDescriptors;
    PPNP_REQ_DESCRIPTOR Descriptor;
    PPNP_REQ_DESCRIPTOR TranslatedReqDesc;
    PDEVICE_NODE DeviceNode;
    HANDLE Handle;
    ULONG CountDescriptors;
    ULONG DescriptorsCount;
    ULONG DescCount = 0;
    ULONG DescSize;
    SIZE_T DataSize;
    ULONG Dummy = 0;
    ULONG ix;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    ReqList = RequestTable->ReqList;
    AltList = *(PPNP_REQ_ALT_LIST *)ReqList->AltList1;

    CountDescriptors = AltList->CountDescriptors;

    for (ix = 0, AltListDescriptors = AltList->ReqDescriptors;
         ix < CountDescriptors;
         ix++, AltListDescriptors++)
    {
        DPRINT("IopBuildCmResourceList: (*AltListDescriptors)->DescriptorsCount %X\n", (*AltListDescriptors)->DescriptorsCount);
        DescCount += (*AltListDescriptors)->DescriptorsCount + 1;
    }

    DataSize = (sizeof(CM_RESOURCE_LIST) + (DescCount - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
    DescSize = (DescCount * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
    DPRINT("IopBuildCmResourceList: RequestTable %p, DescCount %X DataSize %X\n", RequestTable, DescCount, DataSize);

    CmTranslated = ExAllocatePoolWithTag(PagedPool, DataSize, 'erpP');
    if (!CmTranslated)
    {
        DPRINT1("IopBuildCmResourceList: STATUS_INSUFFICIENT_RESOURCES\n");
        RequestTable->ResourceAssignment = NULL;
        RequestTable->TranslatedResourceAssignment = NULL;
        RequestTable->Status = STATUS_INSUFFICIENT_RESOURCES;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("IopBuildCmResourceList: CmTranslated %p\n", CmTranslated);

    CmRaw = ExAllocatePoolWithTag(PagedPool, DataSize, 'erpP');
    if (!CmRaw)
    {
        DPRINT1("IopBuildCmResourceList: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(CmTranslated, 'erpP');
        RequestTable->Status = STATUS_INSUFFICIENT_RESOURCES;
        RequestTable->ResourceAssignment = NULL;
        RequestTable->TranslatedResourceAssignment = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("IopBuildCmResourceList: CmRaw %p\n", CmRaw);

    CmTranslated->Count = 1;
    CmTranslated->List[0].InterfaceType = ReqList->InterfaceType;
    CmTranslated->List[0].BusNumber = ReqList->BusNumber;
    CmTranslated->List[0].PartialResourceList.Count = DescCount;
    CmTranslated->List[0].PartialResourceList.Version = 1;
    CmTranslated->List[0].PartialResourceList.Revision = 1;

    TranslatedDesc = CmTranslated->List[0].PartialResourceList.PartialDescriptors;
    TranslatedDescEnd = ((ULONG_PTR)CmTranslated->List[0].PartialResourceList.PartialDescriptors + DescSize);

    CmRaw->Count = 1;
    CmRaw->List[0].InterfaceType = ReqList->InterfaceType;
    CmRaw->List[0].BusNumber = ReqList->BusNumber;
    CmRaw->List[0].PartialResourceList.Count = DescCount;
    CmRaw->List[0].PartialResourceList.Version = 1;
    CmRaw->List[0].PartialResourceList.Revision = 1;

    RawDesc = CmRaw->List[0].PartialResourceList.PartialDescriptors;
    RawDescEnd = ((ULONG_PTR)CmRaw->List[0].PartialResourceList.PartialDescriptors + DescSize);

    if (AltList->CountDescriptors == 0)
    {
        DPRINT1("IopBuildCmResourceList: AltList->CountDescriptors is 0\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    for (ix = 0; ix < CountDescriptors; ix++)
    {
        Descriptor = AltList->ReqDescriptors[ix];
        TranslatedReqDesc = Descriptor->TranslatedReqDesc;

        if (TranslatedReqDesc->ReqEntry.Reserved4 == 2)
        {
            RtlCopyMemory(RawDesc, TranslatedReqDesc->ReqEntry.pCmDescriptor, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
        }
        else
        {
            Status = IopParentToRawTranslation(TranslatedReqDesc);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Parent To Raw translation failed. Status %X\n", Status);
                ExFreePoolWithTag(CmTranslated, 'erpP');
                ExFreePoolWithTag(CmRaw, 'erpP');
                RequestTable->Status = STATUS_INSUFFICIENT_RESOURCES;
                RequestTable->ResourceAssignment = NULL;
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(RawDesc, Descriptor->ReqEntry.pCmDescriptor, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
        }

        RawDesc++;

        if (TranslatedReqDesc->ReqEntry.Reserved4 == 2)
        {
            RtlCopyMemory(TranslatedDesc, TranslatedReqDesc->ReqEntry.pCmDescriptor, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
        }
        else
        {
            if (!Descriptor->ReqEntry.PhysicalDevice)
            {
                DPRINT("IopBuildCmResourceList: Descriptor->ReqEntry.PhysicalDevice == NULL\n");
                ASSERT(Descriptor->ReqEntry.PhysicalDevice);

                ExFreePoolWithTag(CmTranslated, 'erpP');
                ExFreePoolWithTag(CmRaw, 'erpP');
                RequestTable->Status = STATUS_INSUFFICIENT_RESOURCES;
                RequestTable->ResourceAssignment = NULL;
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            DeviceNode = IopGetDeviceNode(Descriptor->ReqEntry.PhysicalDevice);
            ASSERT(DeviceNode);

            Status = IopChildToRootTranslation(DeviceNode,
                                               Descriptor->InterfaceType,
                                               Descriptor->BusNumber,
                                               Descriptor->ReqEntry.AllocationType,
                                               &Descriptor->ReqEntry.CmDescriptor,
                                               &AssignlDesc);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IopBuildCmResourceList: Child to Root translation failed\n");
                ExFreePoolWithTag(CmTranslated, 'erpP');
                ExFreePoolWithTag(CmRaw, 'erpP');
                RequestTable->Status = STATUS_INSUFFICIENT_RESOURCES;
                RequestTable->ResourceAssignment = NULL;
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(TranslatedDesc, AssignlDesc, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
            ExFreePoolWithTag(AssignlDesc, 'erpP');
        }

        TranslatedDesc++;

        DevicePrivateIoDesc = Descriptor->DevicePrivateIoDesc;

        for (DescriptorsCount = Descriptor->DescriptorsCount;
             DescriptorsCount != 0;
             DescriptorsCount--)
        {
            RawDesc->Type = 0x81;
            RawDesc->ShareDisposition = 1;

            TranslatedDesc->Type = 0x81;
            TranslatedDesc->ShareDisposition = 1;

            RawDesc->Flags = DevicePrivateIoDesc->Flags;
            TranslatedDesc->Flags = DevicePrivateIoDesc->Flags;

            RawDesc->u.DevicePrivate.Data[0] = DevicePrivateIoDesc->u.DevicePrivate.Data[0];
            RawDesc->u.DevicePrivate.Data[1] = DevicePrivateIoDesc->u.DevicePrivate.Data[1];
            RawDesc->u.DevicePrivate.Data[2] = DevicePrivateIoDesc->u.DevicePrivate.Data[2];
            RawDesc++;

            TranslatedDesc->u.DevicePrivate.Data[0] = DevicePrivateIoDesc->u.DevicePrivate.Data[0];
            TranslatedDesc->u.DevicePrivate.Data[1] = DevicePrivateIoDesc->u.DevicePrivate.Data[1];
            TranslatedDesc->u.DevicePrivate.Data[2] = DevicePrivateIoDesc->u.DevicePrivate.Data[2];
            TranslatedDesc++;

            DevicePrivateIoDesc++;

            ASSERT((ULONG_PTR)RawDesc <= (ULONG_PTR)RawDescEnd);
            ASSERT((ULONG_PTR)TranslatedDesc <= (ULONG_PTR)TranslatedDescEnd);
        }

        ASSERT((ULONG_PTR)RawDesc <= (ULONG_PTR)RawDescEnd);
        ASSERT((ULONG_PTR)TranslatedDesc <= (ULONG_PTR)TranslatedDescEnd);
    }

    Status = IopCreateRegistryKeyEx(&Handle,
                                    NULL,
                                    &ResourceMapKeyName,
                                    (KEY_READ | KEY_WRITE),
                                    REG_OPTION_VOLATILE,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopBuildCmResourceList: Status %X\n", Status);
        goto Exit;
    }

    Status = ObQueryNameString(RequestTable->PhysicalDevice,
                               (POBJECT_NAME_INFORMATION)&ObjNameInfo,
                               sizeof(ObjNameInfo),
                               &Dummy);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(Handle);
        goto Exit;
    }

    ObjNameInfo.Name.MaximumLength = sizeof(ObjNameInfo.Buffer);
    if (ObjNameInfo.Name.Length == 0)
    {
        DPRINT1("IopBuildCmResourceList: ObjNameInfo.Name.Length is 0\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        ObjNameInfo.Name.Buffer = ObjNameInfo.Buffer;
    }

    Destination = ObjNameInfo.Name;
    RtlAppendUnicodeToString(&Destination, L".Raw");

    Status = IopWriteResourceList(Handle, &Pnp_ManagerName, &PnpManagerName, &Destination, CmRaw, DataSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopBuildCmResourceList: Status %X\n", Status);
        ZwClose(Handle);
        goto Exit;
    }

    Destination = ObjNameInfo.Name;
    RtlAppendUnicodeToString(&Destination, L".Translated");

    IopWriteResourceList(Handle, &Pnp_ManagerName, &PnpManagerName, &Destination, CmTranslated, DataSize);
    ZwClose(Handle);

Exit:

    RequestTable->ResourceAssignment = CmRaw;
    RequestTable->TranslatedResourceAssignment = CmTranslated;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopBuildCmResourceLists(
    _In_ PPNP_RESOURCE_REQUEST RequestTable,
    _In_ PPNP_RESOURCE_REQUEST RequestTableEnd)
{
    PDEVICE_NODE DeviceNode;
    ULONG TableSize;
    ULONG ListSize;
    ULONG Count;
    ULONG ix;

    PAGED_CODE();

    if (RequestTable >= RequestTableEnd)
    {
        DPRINT1("IopBuildCmResourceLists: Table %p, TableEnd %p\n", RequestTable, RequestTableEnd);
        return STATUS_SUCCESS;
    }

    TableSize = ((ULONG_PTR)RequestTableEnd - (ULONG_PTR)RequestTable);
    Count = ((TableSize - 1) / sizeof(PNP_RESOURCE_REQUEST) + 1);

    DPRINT("IopBuildCmResourceLists: Table %p, TableEnd %p, Count %X\n", RequestTable, RequestTableEnd, Count);

    for (ix = 0; ix < Count; ix++)
    {
        DPRINT("IopBuildCmResourceLists: RequestTable[%X].ResourceAssignment %p\n", ix, RequestTable[ix].ResourceAssignment);
        RequestTable[ix].ResourceAssignment = 0;

        if ((RequestTable[ix].Flags & (0x20 | 0x08)))
            continue;

        if (RequestTable[ix].Flags & 0x10)
        {
            RequestTable[ix].Status = STATUS_UNSUCCESSFUL;
            continue;
        }

        RequestTable[ix].Status = STATUS_SUCCESS;

        IopBuildCmResourceList(&RequestTable[ix]);

        if (!RequestTable[ix].ResourceAssignment)
            continue;

        if (!RequestTable[ix].PhysicalDevice)
        {
            ASSERT(RequestTable[ix].PhysicalDevice);
            continue;
        }

        DeviceNode = IopGetDeviceNode(RequestTable[ix].PhysicalDevice);
        ASSERT(DeviceNode);

        ListSize = PnpDetermineResourceListSize(RequestTable[ix].ResourceAssignment);
        IopWriteAllocatedResourcesToRegistry(DeviceNode, RequestTable[ix].ResourceAssignment, ListSize);
    }

    DPRINT("IopBuildCmResourceLists: return STATUS_SUCCESS\n");
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
IopNeedToReleaseBootResources(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PCM_RESOURCE_LIST CmResources)
{
    PCM_RESOURCE_LIST BootResources;
    PCM_FULL_RESOURCE_DESCRIPTOR BootResourcesList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmBootDescriptor;
    ULONG BootDescCount;
    ULONG SpecificDataSize;
    ULONG ix;
    ULONG jx;
    ULONG kx;
    BOOLEAN IsFind = FALSE;
    BOOLEAN Result = FALSE;

    PAGED_CODE();

    if (CmResources->Count != 1)
    {
        DPRINT1("IopNeedToReleaseBootResources: [%p] Instance '%wZ', Service '%wZ'\n", DeviceNode, &DeviceNode->InstancePath, &DeviceNode->ServiceName);
        return FALSE;
    }

    BootResources = DeviceNode->BootResources;
    if (!BootResources)
    {
        DPRINT1("IopNeedToReleaseBootResources: [%p] Instance '%wZ', Service '%wZ'\n", DeviceNode, &DeviceNode->InstancePath, &DeviceNode->ServiceName);
        return FALSE;
    }

    if (!BootResources->Count)
    {
        DPRINT1("IopNeedToReleaseBootResources: [%p] Instance '%wZ', Service '%wZ'\n", DeviceNode, &DeviceNode->InstancePath, &DeviceNode->ServiceName);
        return FALSE;
    }

    DPRINT("IopNeedToReleaseBootResources: [%p], CmResources %p, BootResources %p\n", DeviceNode, CmResources, BootResources);

    BootResourcesList = &BootResources->List[0];
    DPRINT("IopNeedToReleaseBootResources: BootResourcesList %p\n", BootResourcesList);

    for (ix = 0; ix < BootResources->Count; ix++)
    {
        BootDescCount = BootResourcesList->PartialResourceList.Count;
        CmBootDescriptor = &BootResourcesList->PartialResourceList.PartialDescriptors[0];

        DPRINT("IopNeedToReleaseBootResources: ix-[%X] CmBootDescriptor %p\n", ix, CmBootDescriptor);

        for (jx = 0; jx < BootDescCount; jx++)
        {
            DPRINT("IopNeedToReleaseBootResources: jx-[%X] CmBootDescriptor %p, Type %X\n", jx, CmBootDescriptor, CmBootDescriptor->Type);

            SpecificDataSize = 0;

            switch (CmBootDescriptor->Type)
            {
                case CmResourceTypeNull:
                    DPRINT("IopNeedToReleaseBootResources: CmResourceTypeNull\n");
                    break;

                case CmResourceTypeDeviceSpecific:
                    SpecificDataSize = CmBootDescriptor->u.DeviceSpecificData.DataSize;
                    DPRINT("IopNeedToReleaseBootResources: SpecificDataSize %X\n", SpecificDataSize);
                    break;

                default:
                {
                    if (CmBootDescriptor->Type >= 7)
                    {
                        DPRINT("IopNeedToReleaseBootResources: CmBootDescriptor->Type %X\n", CmBootDescriptor->Type);
                        break;
                    }

                    CmDescriptor = &CmResources->List[0].PartialResourceList.PartialDescriptors[0];

                    for (kx = 0; kx < CmResources->List[0].PartialResourceList.Count; kx++)
                    {
                        ULONG DataSize = 0;

                        DPRINT("IopNeedToReleaseBootResources: kx-[%X] CmDescriptor %p, Type %X\n", kx, CmDescriptor, CmDescriptor->Type);

                        if (CmDescriptor->Type == CmResourceTypeDeviceSpecific)
                        {
                            DataSize = CmDescriptor->u.DeviceSpecificData.DataSize;
                        }
                        else if (CmDescriptor->Type == CmBootDescriptor->Type)
                        {
                            IsFind = TRUE;
                            break;
                        }

                        CmDescriptor++;
                        CmDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)((ULONG_PTR)CmDescriptor + DataSize);
                    }

                    if (IsFind == FALSE)
                    {
                        Result = TRUE;
                        goto Exit;
                    }

                    break;
                }
            }

            CmBootDescriptor++;
            CmBootDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)((ULONG_PTR)CmBootDescriptor + SpecificDataSize);
        }

        BootResourcesList = (PCM_FULL_RESOURCE_DESCRIPTOR)CmBootDescriptor;
        DPRINT("IopNeedToReleaseBootResources: BootResourcesList %p\n", BootResourcesList);
    }

Exit:

    DPRINT("IopNeedToReleaseBootResources: return %X\n", Result);
    return Result;
}

NTSTATUS
NTAPI
IopRestoreResourcesInternal(
    _In_ PDEVICE_NODE DeviceNode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopReleaseResourcesInternal(
    _In_ PDEVICE_NODE DeviceNode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
IopReleaseFilteredBootResources(
    _In_ PPNP_RESOURCE_REQUEST RequestTable,
    _In_ PPNP_RESOURCE_REQUEST RequestTableEnd)
{
    PDEVICE_NODE DeviceNode;
    ULONG Count;
    ULONG Size;
    ULONG ix;
    NTSTATUS Status;
    BOOLEAN Result;

    PAGED_CODE();

    if (RequestTable >= RequestTableEnd)
    {
        DPRINT1("IopReleaseFilteredBootResources: Table %p, TableEnd %p\n", RequestTable, RequestTableEnd);
        return;
    }

    DPRINT("IopReleaseFilteredBootResources: Table %p, TableEnd %p\n", RequestTable, RequestTableEnd);

    Size = ((ULONG_PTR)RequestTableEnd - (ULONG_PTR)RequestTable);
    Count = ((Size - 1) / sizeof(PNP_RESOURCE_REQUEST)) + 1;

    for (ix = 0; ix < Count; ix++)
    {
        if (!RequestTable[ix].ResourceAssignment)
            continue;

        ASSERT(RequestTable[ix].PhysicalDevice);
        DeviceNode = IopGetDeviceNode(RequestTable[ix].PhysicalDevice);
        ASSERT(DeviceNode);

        Result = IopNeedToReleaseBootResources(DeviceNode, RequestTable[ix].ResourceAssignment);
        if (Result == FALSE)
            continue;

        IopReleaseResourcesInternal(DeviceNode);
        PipRequestDeviceAction(NULL, PipEnumAssignResources, 0, 0, NULL, NULL);

        IopAllocateBootResourcesInternal(ArbiterRequestPnpEnumerated,
                                         RequestTable[ix].PhysicalDevice,
                                         RequestTable[ix].ResourceAssignment);

        DeviceNode->Flags &= ~DNF_BOOT_CONFIG_RESERVED;
        DeviceNode->ResourceList = RequestTable[ix].ResourceAssignment;

        DPRINT1("IopReleaseFilteredBootResources: FIXME IopRestoreResourcesInternal\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();

        Status = 0;//IopRestoreResourcesInternal(DeviceNode);
        if (NT_SUCCESS(Status))
        {
            DeviceNode->ResourceList = NULL;
            continue;
        }

        DPRINT1("IopReleaseFilteredBootResources: Possible boot conflict on '%ws'\n", DeviceNode->InstancePath.Buffer);
        ASSERT(Status == STATUS_SUCCESS);

        RequestTable[ix].Flags = 0x10;
        RequestTable[ix].Status = Status;

        ExFreePoolWithTag(RequestTable[ix].ResourceAssignment, 0);

        RequestTable[ix].ResourceAssignment = NULL;
        DeviceNode->ResourceList = NULL;
    }
}

NTSTATUS
NTAPI
IopReleaseDeviceResources(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN IsAllocateBootResources)
{
    UNICODE_STRING BootConfigName = RTL_CONSTANT_STRING(L"BootConfig");
    UNICODE_STRING LogConfName = RTL_CONSTANT_STRING(L"LogConf");
    PCM_RESOURCE_LIST ResourceList;
    HANDLE Handle;
    HANDLE KeyHandle;
    ULONG ResourceListSize;
    NTSTATUS Status;

    DPRINT("IopReleaseDeviceResources: [%p] IsAllocateBootResources %X\n", DeviceNode, IsAllocateBootResources);
    PAGED_CODE();

    if (!DeviceNode->ResourceList && !(DeviceNode->Flags & DNF_BOOT_CONFIG_RESERVED))
        goto Exit;

    ResourceList = NULL;
    ResourceListSize = 0;

    if (IsAllocateBootResources && !(DeviceNode->Flags & DNF_MADEUP))
    {
        Status = PpIrpQueryResources(DeviceNode->PhysicalDeviceObject, &ResourceList, &ResourceListSize);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopReleaseDeviceResources: Status %X\n", Status);
            ResourceList = NULL;
            ResourceListSize = 0;
        }
    }

    Status = IopLegacyResourceAllocation(ArbiterRequestUndefined,
                                         IopRootDriverObject,
                                         DeviceNode->PhysicalDeviceObject,
                                         NULL,
                                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopReleaseDeviceResources: Status %X\n", Status);
        return Status;
    }

    PipRequestDeviceAction(NULL, PipEnumAssignResources, 0, 0, NULL, NULL);

    if (!IsAllocateBootResources || (DeviceNode->Flags & DNF_MADEUP))
        goto Exit;

    ASSERT(DeviceNode->BootResources == NULL);

    KeyHandle = NULL;

    Status = PnpDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject, &Handle, KEY_ALL_ACCESS);
    if (NT_SUCCESS(Status))
    {
        Status = IopCreateRegistryKeyEx(&KeyHandle, Handle, &LogConfName, KEY_ALL_ACCESS, 0, NULL);
        ZwClose(Handle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopReleaseDeviceResources: Status %X\n", Status);
            KeyHandle = NULL;
        }
    }

    if (KeyHandle)
    {
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

        if (ResourceList)
            ZwSetValueKey(KeyHandle, &BootConfigName, 0, REG_RESOURCE_LIST, ResourceList, ResourceListSize);
        else
            ZwDeleteValueKey(KeyHandle, &BootConfigName);

        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();

        ZwClose(KeyHandle);
    }

    if (!ResourceList)
        goto Exit;

    DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
    DeviceNode->BootResources = ResourceList;

    IopAllocateBootResourcesRoutine(ArbiterRequestPnpEnumerated, DeviceNode->PhysicalDeviceObject, ResourceList);

Exit:

    return STATUS_SUCCESS;
}

VOID
NTAPI
IopTestForReconfiguration(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG RebalancePhase,
    _Inout_ ULONG* OutCount,
    _Inout_ PDEVICE_OBJECT** ppDevice)
{
    PDEVICE_NODE child;
    BOOLEAN IsResourceChanged = FALSE;
    NTSTATUS Status;

    DPRINT("IopTestForReconfiguration: [%X] Phase %X Count %X\n", DeviceNode, RebalancePhase, *OutCount);

    if (RebalancePhase == 0)
    {
        if ((DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED) &&
           !(DeviceNode->Flags & DNF_NON_STOPPED_REBALANCE))
        {
            **ppDevice = DeviceNode->PhysicalDeviceObject;

            (*ppDevice)++;
            (*OutCount)++;

            return;
        }

        if (DeviceNode->State == DeviceNodeStarted)
        {
            Status = IopQueryReconfiguration(IRP_MN_QUERY_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);

            if (NT_SUCCESS(Status) && (Status == STATUS_RESOURCE_REQUIREMENTS_CHANGED))
            {
                DeviceNode->Flags |= DNF_RESOURCE_REQUIREMENTS_CHANGED;
                IsResourceChanged = TRUE;
            }

            IopQueryReconfiguration(IRP_MN_CANCEL_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);

            if (IsResourceChanged)
            {
                **ppDevice = DeviceNode->PhysicalDeviceObject;
                (*ppDevice)++;
                (*OutCount)++;
            }
        }

        return;
    }

    /* RebalancePhase != 0 */ 

    if (DeviceNode->State == DeviceNodeStarted)
    {
        for (child = DeviceNode->Child;
             child;
             child = child->Sibling)
        {
            if (child->State != DeviceNodeUninitialized &&
                child->State != DeviceNodeInitialized &&
                child->State != DeviceNodeDriversAdded &&
                child->State != DeviceNodeQueryStopped &&
                child->State != DeviceNodeRemovePendingCloses &&
                child->State != DeviceNodeRemoved &&
                !(child->Flags & DNF_NEEDS_REBALANCE))
            {
                break;
            }
        }

        if (child)
        {
            DPRINT("IopTestForReconfiguration: Child %ws not stopped for %ws\n", child->InstancePath.Buffer, DeviceNode->InstancePath.Buffer);
            return;
        }
    }
    else
    {
        if (DeviceNode->State != DeviceNodeDriversAdded)
            return;

        if (!(DeviceNode->Flags & DNF_HAS_BOOT_CONFIG))
            return;

        if (DeviceNode->Flags & DNF_MADEUP)
            return;
    }

    Status = IopQueryReconfiguration(IRP_MN_QUERY_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        IopQueryReconfiguration(IRP_MN_CANCEL_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
        return;
    }

    DPRINT("IopTestForReconfiguration: %ws succeeded QueryStop\n", DeviceNode->InstancePath.Buffer);

    if (DeviceNode->State == DeviceNodeStarted)
    {
        PipSetDevNodeState(DeviceNode, DeviceNodeQueryStopped, NULL);

        **ppDevice = DeviceNode->PhysicalDeviceObject;
        ObReferenceObject(DeviceNode->PhysicalDeviceObject);

        (*ppDevice)++;
        (*OutCount)++;

        return;
    }

    ASSERT(!(DeviceNode->Flags & DNF_HAS_BOOT_CONFIG));

    Status = IopQueryReconfiguration(IRP_MN_STOP_DEVICE, DeviceNode->PhysicalDeviceObject);
    ASSERT(NT_SUCCESS(Status));

    ASSERT((DeviceNode->Flags & DNF_MADEUP) == 0);

    IopReleaseResourcesInternal(DeviceNode);
    DeviceNode->Flags &= ~(DNF_BOOT_CONFIG_RESERVED | DNF_HAS_BOOT_CONFIG);

    if (DeviceNode->BootResources)
    {
        ExFreePoolWithTag(DeviceNode->BootResources, 0);
        DeviceNode->BootResources = 0;
    }

    DeviceNode->Flags &= ~(DNF_BOOT_CONFIG_RESERVED | DNF_HAS_BOOT_CONFIG);
}

VOID
NTAPI
IopQueryRebalanceWorker(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG RebalancePhase,
    _Inout_ ULONG* OutCount,
    _Inout_ PDEVICE_OBJECT** ppDevice)
{
    PDEVICE_NODE node;

    DPRINT("IopQueryRebalanceWorker: [%X] Phase %X Count %X\n", DeviceNode, RebalancePhase, *OutCount);

    ASSERT(DeviceNode);

    if (DeviceNode->State != DeviceNodeStarted)
        return;

    if (DeviceNode->Flags & (DNF_LEGACY_DRIVER | DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM))
        return;

    for (node = DeviceNode->Child;
         node;
         node = node->Sibling)
    {
        IopQueryRebalanceWorker(node, RebalancePhase, OutCount, ppDevice);
    }

    IopTestForReconfiguration(DeviceNode, RebalancePhase, OutCount, ppDevice);
}

VOID
NTAPI
IopQueryRebalance(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG RebalancePhase,
    _Inout_ ULONG* OutCount,
    _Inout_ PDEVICE_OBJECT** ppDevice)
{
    PDEVICE_NODE node;
    PDEVICE_OBJECT* DevicesArrayN;
    PDEVICE_OBJECT* NewDevicesArray;
    PDEVICE_OBJECT* NewDevicesArrayEnd;
    PDEVICE_OBJECT* array;
    ULONG Count;
    ULONG Size;

    DPRINT("IopQueryRebalance: Phase %X Count %X\n", RebalancePhase, (OutCount?*OutCount:0));

    ASSERT(OutCount);

    DevicesArrayN = *ppDevice;
    IopQueryRebalanceWorker(DeviceNode, RebalancePhase, OutCount, ppDevice);

    Count = *OutCount;
    if (!Count)
        return;

    if (RebalancePhase != 0)
        return;

    Size = (Count * sizeof(PDEVICE_OBJECT));

    NewDevicesArray = ExAllocatePoolWithTag(PagedPool, Size, 'erpP');
    if (!NewDevicesArray)
    {
        DPRINT1("IopQueryRebalance: Not enough memory\n");
        *OutCount = 0;
        return;
    }

    DPRINT("IopQueryRebalance: NewDevicesArray %p\n", NewDevicesArray);

    RtlCopyMemory(NewDevicesArray, DevicesArrayN, Size);

    *OutCount = 0;
    *ppDevice = DevicesArrayN;

    NewDevicesArrayEnd = &NewDevicesArray[Count];

    for (array = NewDevicesArray; array < NewDevicesArrayEnd; array++)
    {
        ASSERT(*array);
        node = IopGetDeviceNode(*array);
        ASSERT(node);

        IopQueryRebalanceWorker(node, 1, OutCount, ppDevice);
    }

    ExFreePoolWithTag(NewDevicesArray, 'erpP');
}

NTSTATUS
NTAPI
IopRebalance(
    _In_ ULONG InDevicesCount,
    _In_ PPNP_RESOURCE_REQUEST InRequestTable)
{
    PPNP_RESOURCE_REQUEST RequestTable = NULL;
    PPNP_RESOURCE_REQUEST RebalanceEntry;
    PPNP_RESOURCE_REQUEST RebalanceEntryEnd;
    PPNP_RESOURCE_REQUEST AssignedTable = NULL;
    PPNP_RESOURCE_REQUEST AssignedTableEnd;
    PPNP_RESOURCE_REQUEST Entry;
    PDEVICE_OBJECT * DeviceList;
    PDEVICE_OBJECT * pDevice;
    PDEVICE_NODE DeviceNode;
    LIST_ENTRY ConfigList;
    ULONG RebalancePhase;
    ULONG SizeOfDevicePart;
    ULONG SizeOfRebalancePart;
    ULONG RebalanceCount;
    ULONG AssignedCount;
    ULONG CountForFind;
    ULONG Count;
    ULONG ix;
    ULONG jx;
    NTSTATUS Status;

    DPRINT1("IopRebalance: Count %X Table [%p-%p]\n", InDevicesCount, InRequestTable, (InRequestTable + InDevicesCount));

    DeviceList = ExAllocatePoolWithTag(PagedPool, (IopNumberDeviceNodes * sizeof(PDEVICE_OBJECT)), 'erpP');
    if (!DeviceList)
    {
        DPRINT1("IopRebalance: Not enough memory to perform rebalance\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RebalancePhase = 0;
    RebalanceCount = 0;
    Count = 0;

    while (TRUE)
    {
        while (TRUE)
        {
            pDevice = &DeviceList[Count];

            IopQueryRebalance(IopRootDeviceNode, RebalancePhase, &RebalanceCount, &pDevice);
            if (RebalanceCount)
                break;

            if (RebalancePhase == 1 || InDevicesCount == 0)
            {
                DPRINT1("IopRebalance: No device participates in rebalance phase %X\n", RebalancePhase);
                ExFreePoolWithTag(DeviceList, 'erpP');
                return STATUS_UNSUCCESSFUL;
            }

            RebalancePhase = 1;
        }

        if (RebalanceCount == Count)
        {
            DPRINT1("IopRebalance: RebalanceCount == Count (%X)\n", Count);
            Status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }

        if (!RebalancePhase)
            Count = RebalanceCount;

        SizeOfDevicePart = (InDevicesCount * sizeof(PNP_RESOURCE_REQUEST));
        SizeOfRebalancePart = (RebalanceCount * sizeof(PNP_RESOURCE_REQUEST));

        RequestTable = ExAllocatePoolWithTag(PagedPool, (SizeOfDevicePart + SizeOfRebalancePart), 'erpP');
        if (!RequestTable)
        {
            DPRINT1("IopRebalance: Not enough memory to perform rebalance\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        RebalanceEntry = &RequestTable[InDevicesCount];
        RebalanceEntryEnd = &RequestTable[InDevicesCount + RebalanceCount];


        if (InDevicesCount)
            RtlCopyMemory(RequestTable, InRequestTable, ((SizeOfDevicePart * 4) / 4));

        RtlZeroMemory(RebalanceEntry, (SizeOfRebalancePart * 4) / 4);

        for (jx = 0; jx < RebalanceCount; jx++)
        {
            RebalanceEntry[jx].AllocationType = ArbiterRequestPnpEnumerated;
            RebalanceEntry[jx].PhysicalDevice = DeviceList[jx];
        }

        Status = IopGetResourceRequirementsForAssignTable(RebalanceEntry, RebalanceEntryEnd, &AssignedCount);
        if (!AssignedCount)
        {
            DPRINT1("IopRebalance: AssignedCount - 0\n");
            goto Exit;
        }

        CountForFind = (InDevicesCount + AssignedCount);

        if (AssignedCount == RebalanceCount)
        {
            AssignedTable = RequestTable;
            AssignedTableEnd = RebalanceEntryEnd;
        }
        else
        {
            PPNP_RESOURCE_REQUEST newEntry;

            AssignedTable = ExAllocatePoolWithTag(PagedPool, (CountForFind * sizeof(PNP_RESOURCE_REQUEST)), 'erpP');
            if (!AssignedTable)
            {
                IopFreeResourceRequirementsForAssignTable(RebalanceEntry, RebalanceEntryEnd);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            newEntry = AssignedTable;

            for (Entry = RequestTable; Entry < RebalanceEntryEnd; Entry++)
            {
                if ((Entry->Flags & 0x20) != 0)
                {
                    ASSERT(Entry >= RebalanceEntry);
                }
                else
                {
                    RtlCopyMemory(newEntry, Entry, sizeof(PNP_RESOURCE_REQUEST));
                    newEntry++;
                }
            }

            AssignedTableEnd = &AssignedTable[CountForFind];
        }

        Status = IopFindBestConfiguration(AssignedTable, CountForFind, &ConfigList);

        if (NT_SUCCESS(Status))
        {
            IopBuildCmResourceLists(AssignedTable, AssignedTableEnd);

            if (InDevicesCount != 0)
                RtlCopyMemory(InRequestTable, AssignedTable, ((SizeOfDevicePart * 4) / 4));

            IopFreeResourceRequirementsForAssignTable(&AssignedTable[InDevicesCount], AssignedTableEnd);

            Count = (ULONG)(AssignedTableEnd - AssignedTable);

            if (RequestTable != AssignedTable)
            {
                for (ix = 0; ix < Count; ix++)
                {
                    if (!(RequestTable[ix].Flags & 0x20))
                    {
                        RtlCopyMemory(&RequestTable[ix], &AssignedTable[ix], sizeof(PNP_RESOURCE_REQUEST));

                        if (AssignedTable[ix].Flags & 0x10)
                            RequestTable[ix].Status = STATUS_CONFLICTING_ADDRESSES;
                    }
                }
            }

            for (Entry = RebalanceEntry; Entry < RebalanceEntryEnd; Entry++)
            {
                ASSERT(Entry->PhysicalDevice);
                DeviceNode = IopGetDeviceNode(Entry->PhysicalDevice);
                ASSERT(DeviceNode);

                if (!NT_SUCCESS(Entry->Status))
                {
                    IopQueryReconfiguration(IRP_MN_CANCEL_STOP_DEVICE, Entry->PhysicalDevice);
                    PipRestoreDevNodeState(DeviceNode);
                }
                else
                {
                    DPRINT1("STOPPING %wZ during REBALANCE\n", &DeviceNode->InstancePath);
                    IopQueryReconfiguration(IRP_MN_STOP_DEVICE, Entry->PhysicalDevice);
                    PipSetDevNodeState(DeviceNode, DeviceNodeStopped, NULL);
                }
            }

            IopCommitConfiguration(&ConfigList);

            for (Entry = (RebalanceEntryEnd - 1); Entry >= RebalanceEntry; Entry--)
            {
                ASSERT(Entry->PhysicalDevice);
                DeviceNode = IopGetDeviceNode(Entry->PhysicalDevice);
                ASSERT(DeviceNode);

                if (NT_SUCCESS(Entry->Status))
                {
                    if (DeviceNode->ResourceList)
                        ExFreePoolWithTag(DeviceNode->ResourceList, 0);

                    if (DeviceNode->ResourceListTranslated)
                        ExFreePoolWithTag(DeviceNode->ResourceListTranslated, 0);

                    DeviceNode->ResourceList = Entry->ResourceAssignment;
                    DeviceNode->ResourceListTranslated = Entry->TranslatedResourceAssignment;

                    if (DeviceNode->ResourceList == NULL)
                        DeviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;

                    if ((Entry->Flags & 0x0400) != 0)
                        DeviceNode->Flags &= ~(DNF_NON_STOPPED_REBALANCE | DNF_RESOURCE_REQUIREMENTS_CHANGED);
                }
            }

            Status = STATUS_SUCCESS;
            break;
        }

        IopFreeResourceRequirementsForAssignTable((PPNP_RESOURCE_REQUEST )((PCHAR)AssignedTable + SizeOfDevicePart), AssignedTableEnd);

        if (RebalancePhase == 1)
        {
            for (Entry = RebalanceEntry; Entry < RebalanceEntryEnd; Entry++)
            {
                IopQueryReconfiguration(IRP_MN_CANCEL_STOP_DEVICE, Entry->PhysicalDevice);
                ASSERT(Entry->PhysicalDevice);

                DeviceNode = IopGetDeviceNode(Entry->PhysicalDevice);
                ASSERT(DeviceNode);

                PipRestoreDevNodeState(DeviceNode);
            }

            break;
        }

        RebalancePhase = 1;

        if (AssignedTable)
            ExFreePoolWithTag(AssignedTable, 'erpP');

        if (RequestTable != AssignedTable)
            ExFreePoolWithTag(RequestTable, 'erpP');

        AssignedTable = NULL;
        RequestTable = NULL;
    }

    for (pDevice = &DeviceList[RebalanceCount - 1];
         pDevice >= DeviceList;
         pDevice--)
    {
        ObDereferenceObject(*pDevice);
    }

    ExFreePoolWithTag(DeviceList, 'erpP');
    DeviceList = NULL;

Exit:

    if (!NT_SUCCESS(Status) && (DeviceList != NULL))
    {
        DPRINT1("IopRebalance: Rebalance failed\n");

        for (pDevice = &DeviceList[RebalanceCount - 1];
             pDevice >= DeviceList;
             pDevice--)
        {
            ASSERT(*pDevice);
            DeviceNode = IopGetDeviceNode(*pDevice);
            ASSERT(DeviceNode);

            IopQueryReconfiguration(IRP_MN_CANCEL_STOP_DEVICE, *pDevice);
            PipRestoreDevNodeState(DeviceNode);
            ObDereferenceObject(*pDevice);
        }
    }

    if (DeviceList)
        ExFreePoolWithTag(DeviceList, 'erpP');

    if (AssignedTable)
        ExFreePoolWithTag(AssignedTable, 'erpP');

    if (RequestTable && (RequestTable != AssignedTable))
        ExFreePoolWithTag(RequestTable, 'erpP');

    return Status;
}

NTSTATUS
NTAPI
IopAllocateResources(
    _Inout_ PULONG OutDeviceCount,
    _In_ PPNP_RESOURCE_REQUEST * ResContext,
    _In_ BOOLEAN IsLocked,
    _In_ BOOLEAN IsBootConfig,
    _Out_ BOOLEAN * OutIsAssigned)
{
    PPNP_RESOURCE_REQUEST RequestTable;
    PPNP_RESOURCE_REQUEST RequestTableEnd;
    PPNP_RESOURCE_REQUEST Current;
    LIST_ENTRY List;
    ULONG DeviceCount;
    ULONG Count;
    ULONG ix;
    NTSTATUS Status;
    BOOLEAN IsMultiAlloc;

    PAGED_CODE();
    DPRINT("IopAllocateResources: Count %X, Context %p, Locked %X, BootConfig %X\n", *OutDeviceCount, *ResContext, IsLocked, IsBootConfig);

    if (!IsLocked)
    {
        KeEnterCriticalRegion();
        KeWaitForSingleObject(&PpRegistrySemaphore, DelayExecution, KernelMode, FALSE, NULL);
    }

    DeviceCount = *OutDeviceCount;
    RequestTable = *ResContext;
    RequestTableEnd = &RequestTable[DeviceCount];

    Status = IopGetResourceRequirementsForAssignTable(RequestTable, RequestTableEnd, &DeviceCount);

    DPRINT("IopAllocateResources: DeviceCount %X, Status %X\n", DeviceCount, Status);

    if (!DeviceCount)
    {
        DPRINT("IopAllocateResources: DeviceCount is 0\n");
        goto Exit;
    }

    if (*OutDeviceCount == 1 && (RequestTable->Flags & 0x80)) // ?reallocated Resources?
        IsMultiAlloc = FALSE;
    else
        IsMultiAlloc = TRUE;

    if (IsBootConfig == FALSE)
    {
        for (Current = RequestTable; Current < RequestTableEnd; Current++)
        {
            PDEVICE_NODE DeviceNode;

            ASSERT(Current->PhysicalDevice);
            DeviceNode = IopGetDeviceNode(Current->PhysicalDevice);

            if (NT_SUCCESS(Current->Status) && !Current->ResourceRequirements)
            {
                DPRINT("IopAllocateResources: Processing device '%wZ'\n", &DeviceNode->InstancePath);
            }
            else
            {
                DPRINT("IopAllocateResources: Ignoring device '%wZ'\n", &DeviceNode->InstancePath);

                Current->Flags |= 0x20;
                Current->Status = STATUS_RETRY;
            }

            DPRINT("IopAllocateResources: [%p] BootResources %p\n", DeviceNode, DeviceNode->BootResources);
        }

        IopFreeResourceRequirementsForAssignTable(RequestTable, RequestTableEnd);
        goto Exit;
    }

    if (!IopBootConfigsReserved)
    {
        for (Current = RequestTable; Current < RequestTableEnd; Current++)
        {
            PDEVICE_NODE DeviceNode;

            ASSERT(Current->PhysicalDevice);
            DeviceNode = IopGetDeviceNode(Current->PhysicalDevice);

            DPRINT("IopAllocateResources: IopBootConfigsReserved is NULL for '%wZ'\n", &DeviceNode->InstancePath);
            DPRINT("IopAllocateResources: [%p] BootResources %p\n", DeviceNode, DeviceNode->BootResources);

            if (DeviceNode->Flags & DNF_HAS_BOOT_CONFIG)
                break;
        }

        if (Current != RequestTableEnd)
        {
            for (Current = RequestTable; Current < RequestTableEnd; Current++)
            {
                PDEVICE_NODE DeviceNode;

                ASSERT(Current->PhysicalDevice);
                DeviceNode = IopGetDeviceNode(Current->PhysicalDevice);

                if (!(Current->Flags & 0x20) &&
                    !(DeviceNode->Flags & DNF_HAS_BOOT_CONFIG) &&
                    Current->ResourceRequirements)
                {
                    DPRINT("IopAllocateResources: Delaying non BOOT config device '%wZ'\n", &DeviceNode->InstancePath);

                    ASSERT(FALSE);

                    Current->Flags |= 0x20;
                    Current->Status = STATUS_RETRY;
                    DeviceCount--;
                }
            }
        }
    }

    if (!DeviceCount)
    {
        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
        IopFreeResourceRequirementsForAssignTable(RequestTable, RequestTableEnd);
        goto Exit;
    }

    if (DeviceCount != *OutDeviceCount)
    {
        PNP_RESOURCE_REQUEST TempResRequest;
        PPNP_RESOURCE_REQUEST LastResRequest;
        PPNP_RESOURCE_REQUEST CurrentResRequest;

        DPRINT("IopAllocateResources: Count %X, *OutDeviceCount %X\n", DeviceCount, *OutDeviceCount);

        LastResRequest = (RequestTableEnd - 1);

        for (CurrentResRequest = RequestTable;
             CurrentResRequest < RequestTableEnd;
             )
        {
            if (!(CurrentResRequest->Flags & 0x20))
            {
                CurrentResRequest++;
                continue;
            }

            RtlCopyMemory(&TempResRequest, CurrentResRequest, sizeof(TempResRequest));
            RtlCopyMemory(CurrentResRequest, LastResRequest, sizeof(PNP_RESOURCE_REQUEST));
            RtlCopyMemory(LastResRequest, &TempResRequest, sizeof(TempResRequest));

            RequestTableEnd--;
            LastResRequest--;
        }
    }

    Count = (ULONG)(RequestTableEnd - RequestTable);
    ASSERT(Count == DeviceCount);

    if (DeviceCount > 1)
    {
        DPRINT("IopAllocateResources: FIXME IopRearrangeAssignTable. Count %X\n", Count);
        //IopRearrangeAssignTable(RequestTable, DeviceCount);
    }

    for (Current = RequestTable; Current < RequestTableEnd; Current++)
    {
        PDEVICE_NODE DeviceNode;

        ASSERT(Current->PhysicalDevice);
        DeviceNode = IopGetDeviceNode(Current->PhysicalDevice);

        DPRINT("IopAllocateResources: Trying alloc for '%S'\n", DeviceNode->InstancePath.Buffer);
        DPRINT("IopAllocateResources: [%p] BootResources %p\n", DeviceNode, DeviceNode->BootResources);

        DPRINT("\n");
        PipDumpResourceRequirementsList(DeviceNode->ResourceRequirements, 1);
        DPRINT("\n");
        PipDumpCmResourceList(DeviceNode->BootResources, 1);
        DPRINT("\n");

        Status = IopFindBestConfiguration(Current, 1, &List);
        DPRINT("IopAllocateResources: Status %X\n", Status);
    
        if (NT_SUCCESS(Status))
        {
            Status = IopCommitConfiguration(&List);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IopAllocateResources: Status %X\n", Status);
                ASSERT(FALSE);

                Current->Status = STATUS_CONFLICTING_ADDRESSES;
                break;
            }

            IopBuildCmResourceLists(Current, &Current[1]);
            DPRINT("IopAllocateResources: [%p] BootResources %p\n", DeviceNode, DeviceNode->BootResources);

            break;
        }

        if (Status == STATUS_INSUFFICIENT_RESOURCES)
        {
            DPRINT("IopAllocateResources: Failed to allocate Pool.\n");
            ASSERT(FALSE);
            break;
        }

        if (!IsMultiAlloc)
        {
            ASSERT(FALSE);
            Current->Status = STATUS_CONFLICTING_ADDRESSES;
            break;
        }

        DeviceNode->Flags |= DNF_NEEDS_REBALANCE;

        Status = IopRebalance(1, Current);

        DeviceNode->Flags &= ~DNF_NEEDS_REBALANCE;

        if (OutIsAssigned)
        {
            *OutIsAssigned = TRUE;
            break;
        }
    }

    Current++;

    for (; Current < RequestTableEnd; Current++)
    {
        DPRINT("IopAllocateResources: Current %p\n", Current);

        if (Status == STATUS_INSUFFICIENT_RESOURCES)
        {
            DPRINT("IopAllocateResources: STATUS_INSUFFICIENT_RESOURCES\n");
            ASSERT(FALSE);
            Current->Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Current->Flags |= 0x20;
            Current->Status = STATUS_RETRY;
        }
    }

    if (RequestTable >= RequestTableEnd)
    {
        IopFreeResourceRequirementsForAssignTable(RequestTable, RequestTableEnd);
        goto Exit;
    }

    for (ix = 0; ix < Count; ix++)
    {
        DPRINT("IopAllocateResources: [%X] Flags %X, ResourceAssignment %X\n", ix, RequestTable[ix].Flags, RequestTable[ix].ResourceAssignment);

        if (RequestTable[ix].Flags & (0x20|0x08))
            continue;

        if (!RequestTable[ix].Status &&
            RequestTable[ix].AllocationType == ArbiterRequestPnpEnumerated)
        {
            IopReleaseFilteredBootResources(&RequestTable[ix], &RequestTable[ix + 1]);
        }

        if (!(RequestTable[ix].Flags & 0x10) && RequestTable[ix].ResourceAssignment)
            continue;

        DPRINT("IopAllocateResources: Flags %X, ResourceAssignment %X\n", RequestTable[ix].Flags, RequestTable[ix].ResourceAssignment);

        ASSERT(FALSE);
        RequestTable[ix].Status = STATUS_CONFLICTING_ADDRESSES;
    }

    IopFreeResourceRequirementsForAssignTable(RequestTable, RequestTableEnd);

Exit:

    if (!IsLocked)
    {
        KeReleaseSemaphore(&PpRegistrySemaphore, IO_NO_INCREMENT, 1, FALSE);
        KeLeaveCriticalRegion();
    }

    return Status;
}

VOID
NTAPI
IopReleaseResources(
    _In_ PDEVICE_NODE DeviceNode)
{
    DPRINT1("IopReleaseResources: [%X]\n", DeviceNode);

    IopReleaseResourcesInternal(DeviceNode);

    if (DeviceNode->PrevCmResource)
    {
        ExFreePool(DeviceNode->PrevCmResource);
        DeviceNode->PrevCmResource = NULL;
    }

  #if DBG
    if (DeviceNode->DbgParam2)
    {
        ExFreePool(DeviceNode->DbgParam2);
        DeviceNode->DbgParam2 = NULL;
    }
  #endif

    if (DeviceNode->ResourceList)
    {
        if (NT_SUCCESS(DeviceNode->DebugStatus))
            ExFreePool(DeviceNode->ResourceList);
        else
            DeviceNode->PrevCmResource = DeviceNode->ResourceList;

        DeviceNode->ResourceList = NULL;
    }

    if (DeviceNode->ResourceListTranslated)
    {
        ExFreePool(DeviceNode->ResourceListTranslated);
        DeviceNode->ResourceListTranslated = NULL;
    }

    if ((DeviceNode->Flags & (DNF_MADEUP + DNF_DEVICE_GONE)) != DNF_MADEUP)
    {
        DeviceNode->Flags &= ~(DNF_BOOT_CONFIG_RESERVED | DNF_HAS_BOOT_CONFIG);

        if (DeviceNode->BootResources) {
            ExFreePool(DeviceNode->BootResources);
            DeviceNode->BootResources = NULL;
        }

        return;
    }

    if (!(DeviceNode->Flags & DNF_HAS_BOOT_CONFIG))
        return;

    if (!DeviceNode->BootResources)
        return;

    IopAllocateBootResourcesInternal(ArbiterRequestPnpEnumerated,
                                     DeviceNode->PhysicalDeviceObject,
                                     DeviceNode->BootResources);
}

VOID
NTAPI
IopRemoveLegacyDeviceNode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDEVICE_NODE LegacyDeviceNode)
{
    PDEVICE_NODE Child;
    PDEVICE_NODE node;
    PDEVICE_OBJECT PhysicalDeviceObject;
  
    DPRINT1("IopRemoveLegacyDeviceNode: [%X]:[%X]\n", DeviceObject, LegacyDeviceNode);
    ASSERT(LegacyDeviceNode);

    if (DeviceObject)
    {
        if (DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE)
            return;
    }
    else
    {
        if (!LegacyDeviceNode->DuplicatePDO)
        {
            DPRINT1("IopRemoveLegacyDeviceNode: No duplicate PDO for '%S'\n", LegacyDeviceNode->InstancePath.Buffer);
            ASSERT(LegacyDeviceNode->DuplicatePDO);
            return;
        }

        Child = LegacyDeviceNode->Child;
        LegacyDeviceNode->DuplicatePDO = NULL;

        if (Child)
            Child->Sibling = LegacyDeviceNode->Sibling;

        if (LegacyDeviceNode->Sibling)
            LegacyDeviceNode->Sibling->Child = LegacyDeviceNode->Child;

        if (IopLegacyDeviceNode == LegacyDeviceNode)
            IopLegacyDeviceNode = LegacyDeviceNode->Sibling;
    }

    for (node = LegacyDeviceNode->OverUsed1.LegacyDeviceNode;
         node;
         node = node->OverUsed2.NextResourceDeviceNode)
    {
        if (node->OverUsed2.NextResourceDeviceNode == LegacyDeviceNode)
        {
            node->OverUsed2.NextResourceDeviceNode = LegacyDeviceNode->OverUsed2.NextResourceDeviceNode;
            break;
        }
    }

    LegacyDeviceNode->Flags &= ~DNF_LEGACY_RESOURCE_DEVICENODE;

    PhysicalDeviceObject = LegacyDeviceNode->PhysicalDeviceObject;

    LegacyDeviceNode->LastChild = NULL;
    LegacyDeviceNode->Child = NULL;
    LegacyDeviceNode->Sibling = NULL;
    LegacyDeviceNode->Parent = NULL;

    IopDestroyDeviceNode(LegacyDeviceNode);

    if (DeviceObject)
        return;

    PhysicalDeviceObject->DriverObject = IopRootDriverObject;
    IoDeleteDevice(PhysicalDeviceObject);
}

PCM_RESOURCE_LIST
NTAPI
IopCombineLegacyResources(
    _In_ PDEVICE_NODE LegacyDeviceNode)
{
    PCM_FULL_RESOURCE_DESCRIPTOR PrevIoList;
    PCM_FULL_RESOURCE_DESCRIPTOR IoList;
    PCM_RESOURCE_LIST CmResources = NULL;
    PDEVICE_NODE DeviceNode;
    SIZE_T SizeCmResources = 0;
    ULONG SizeIoList;

    PAGED_CODE()
    DPRINT1("IopCombineLegacyResources: [%X]\n", LegacyDeviceNode);

    if (!LegacyDeviceNode)
        return NULL;

    for (DeviceNode = LegacyDeviceNode;
         DeviceNode;
         DeviceNode = DeviceNode->OverUsed2.NextResourceDeviceNode)
    {
        if (DeviceNode->ResourceList)
            SizeCmResources += PnpDetermineResourceListSize(DeviceNode->ResourceList);
    }

    if (!SizeCmResources)
        return NULL;

    CmResources = ExAllocatePoolWithTag(PagedPool, SizeCmResources, 'erpP');
    if (!CmResources)
    {
        DPRINT1("IopCombineLegacyResources: Not enough memory\n");
        return NULL;
    }

    DPRINT("IopCombineLegacyResources: CmResources %p\n", CmResources);

    CmResources->Count = 0;
    IoList = CmResources->List;

    for (DeviceNode = LegacyDeviceNode;
         DeviceNode != NULL;
         DeviceNode = DeviceNode->OverUsed2.NextResourceDeviceNode)
    {
        if (!DeviceNode->ResourceList)
            continue;

        SizeCmResources = PnpDetermineResourceListSize(DeviceNode->ResourceList);
        if (!SizeCmResources)
            continue;

        PrevIoList = IoList;

        SizeIoList = (SizeCmResources - FIELD_OFFSET(CM_RESOURCE_LIST, List));
        IoList = (PCM_FULL_RESOURCE_DESCRIPTOR)((ULONG_PTR)IoList + SizeIoList);

        RtlCopyMemory(PrevIoList, DeviceNode->ResourceList->List, SizeIoList);

        CmResources->Count += DeviceNode->ResourceList->Count;
    }

    return CmResources;
}

VOID
NTAPI
IopSetLegacyResourcesFlag(
    _In_ PDRIVER_OBJECT DriverObject)
{
    KIRQL OldIrql;

    DPRINT1("IopSetLegacyResourcesFlag: DriverObject %X\n", DriverObject);

    OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);

    DriverObject->Flags |= DRVO_LEGACY_RESOURCES;

    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);
}

NTSTATUS
NTAPI
IopSetLegacyDeviceInstance(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_NODE DeviceNode)
{
    UNICODE_STRING RootLegacyName = RTL_CONSTANT_STRING(L"ROOT\\LEGACY");
    UNICODE_STRING InstanceStr;
    PDEVICE_OBJECT LegacyDeviceObject;
    PDEVICE_NODE LegacyDeviceNode;
    PUNICODE_STRING ServiceStr;
    HANDLE Handle;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT1("IopSetLegacyDeviceInstance: [%X]:[%X]\n", DriverObject, DeviceNode);

    DeviceNode->OverUsed1.LegacyDeviceNode = NULL;
    ServiceStr = (PUNICODE_STRING)&DriverObject->DriverExtension->ServiceKeyName;

    InstanceStr.Length = 0;
    InstanceStr.Buffer = NULL;

    Status = PipServiceInstanceToDeviceInstance(NULL, ServiceStr, 0, &InstanceStr, &Handle, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopSetLegacyDeviceInstance: Status %X\n", Status);
        return Status;
    }

    if (!InstanceStr.Length)
    {
        DPRINT1("IopSetLegacyDeviceInstance: InstanceStr.Length is 0\n");
        return Status;
    }

    if (!RtlPrefixUnicodeString(&RootLegacyName, &InstanceStr, TRUE))
    {
        RtlFreeUnicodeString(&InstanceStr);
        goto Exit;
    }

    DeviceNode->InstancePath = InstanceStr;
    LegacyDeviceObject = IopDeviceObjectFromDeviceInstance(&InstanceStr);

    if (LegacyDeviceObject)
        goto Exit;

    LegacyDeviceNode = IopGetDeviceNode(LegacyDeviceObject);
    ASSERT(LegacyDeviceNode);

    DeviceNode->OverUsed2.NextResourceDeviceNode = LegacyDeviceNode->OverUsed2.NextResourceDeviceNode;
    LegacyDeviceNode->OverUsed2.NextResourceDeviceNode = DeviceNode;
    DeviceNode->OverUsed1.LegacyDeviceNode = LegacyDeviceNode;

Exit:
    Status = ZwClose(Handle);
    return Status;
}

NTSTATUS
NTAPI
IopFindLegacyDeviceNode(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PDEVICE_NODE * OutLegacyDeviceNode,
    _Out_ PDEVICE_OBJECT * OutLegacyPDO)
{
    PDEVICE_NODE LegacyDeviceNode;
    PDEVICE_OBJECT LegacyPDO;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    DPRINT1("IopFindLegacyDeviceNode: [%X]:[%X]\n", DriverObject, DeviceObject);
    ASSERT(OutLegacyDeviceNode && OutLegacyPDO);

    if (DeviceObject)
    {
        LegacyDeviceNode = IopGetDeviceNode(DeviceObject);
        if (LegacyDeviceNode)
        {
            *OutLegacyPDO = DeviceObject;
            *OutLegacyDeviceNode = LegacyDeviceNode;

            return STATUS_SUCCESS;
        }

        if (DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE)
        {
            DPRINT1("IopFindLegacyDeviceNode: No node for PDO %p\n", DeviceObject);
            ASSERT(IopGetDeviceNode(DeviceObject));
            return Status;
        }

        /*
            PipAllocateDeviceNode() return Status in NT:
        
            Status = PipAllocateDeviceNode(DeviceObject, &LegacyDeviceNode);
            if (Status == STATUS_SYSTEM_HIVE_TOO_LARGE || !LegacyDeviceNode)
                return STATUS_INSUFFICIENT_RESOURCES;
        */
        LegacyDeviceNode = PipAllocateDeviceNode(DeviceObject);
        if (!LegacyDeviceNode)
        {
            DPRINT1("IopFindLegacyDeviceNode: STATUS_INSUFFICIENT_RESOURCES for PDO %p\n", DeviceObject);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

      #if 0
        if (Status == STATUS_SYSTEM_HIVE_TOO_LARGE)
        {
            IopDestroyDeviceNode(LegacyDeviceNode);
            return Status;
        }
      #endif

        LegacyDeviceNode->Flags |= DNF_LEGACY_RESOURCE_DEVICENODE;
        IopSetLegacyDeviceInstance(DriverObject, LegacyDeviceNode);

        *OutLegacyPDO = DeviceObject;
        *OutLegacyDeviceNode = LegacyDeviceNode;

        return STATUS_SUCCESS;
    }

    LegacyDeviceNode = IopLegacyDeviceNode;
    if (LegacyDeviceNode)
    {
        do
        {
            if ((PDRIVER_OBJECT)LegacyDeviceNode->DuplicatePDO == DriverObject)
                break;

            LegacyDeviceNode = LegacyDeviceNode->Sibling;
        }
        while (LegacyDeviceNode);

        if (LegacyDeviceNode)
        {
            *OutLegacyPDO = LegacyDeviceNode->PhysicalDeviceObject;
            *OutLegacyDeviceNode = LegacyDeviceNode;

            return STATUS_SUCCESS;
        }
    }

    Status = IoCreateDevice(IopRootDriverObject,
                            sizeof(IOPNP_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &LegacyPDO);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopFindLegacyDeviceNode: Status %X\n", Status);
        return Status;
    }

    LegacyPDO->Flags |= DO_BUS_ENUMERATED_DEVICE;

    /*
        PipAllocateDeviceNode() return Status in NT:

        Status = PipAllocateDeviceNode(LegacyPDO, &LegacyDeviceNode);
        if (Status == STATUS_SYSTEM_HIVE_TOO_LARGE || !LegacyDeviceNode)
            return STATUS_INSUFFICIENT_RESOURCES;
    */
    LegacyDeviceNode = PipAllocateDeviceNode(LegacyPDO);
    if (!LegacyDeviceNode)
    {
        DPRINT1("IopFindLegacyDeviceNode: STATUS_INSUFFICIENT_RESOURCES for PDO %p\n", DeviceObject);
        IoDeleteDevice(LegacyPDO);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LegacyPDO->DriverObject = DriverObject;
    LegacyDeviceNode->Flags = (DNF_LEGACY_RESOURCE_DEVICENODE + DNF_MADEUP);

    PipSetDevNodeState(LegacyDeviceNode, DeviceNodeInitialized, NULL);
    LegacyDeviceNode->DuplicatePDO = (PDEVICE_OBJECT)DriverObject;

    IopSetLegacyDeviceInstance(DriverObject, LegacyDeviceNode);
    LegacyDeviceNode->Sibling = IopLegacyDeviceNode;

    if (IopLegacyDeviceNode)
        IopLegacyDeviceNode->Child = LegacyDeviceNode;

    *OutLegacyPDO = LegacyPDO;
    *OutLegacyDeviceNode = LegacyDeviceNode;

    return Status;
}

NTSTATUS
NTAPI 
IopLegacyResourceAllocation(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT DeviceObject OPTIONAL,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    _Inout_ PCM_RESOURCE_LIST * AllocatedResources)
{
    PPNP_RESOURCE_REQUEST RequestTable;
    PCM_RESOURCE_LIST LegacyCmResources;
    PNP_RESOURCE_REQUEST RequestEntry;
    PCM_RESOURCE_LIST CmResources;
    PDEVICE_NODE LegacyDeviceNode;
    PDEVICE_OBJECT LegacyPDO;
    PDEVICE_NODE DeviceNode;
    ULONG DeviceCount;
    ULONG ListSize;
    SIZE_T Size;
    NTSTATUS Status;

    DPRINT1("IopLegacyResourceAllocation: AllocType %X [%X]:[%X] \n", AllocationType, DriverObject, DeviceObject);

    ASSERT(DriverObject);
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&PpRegistrySemaphore, DelayExecution, KernelMode, FALSE, NULL);

    Status = IopFindLegacyDeviceNode(DriverObject, DeviceObject, &DeviceNode, &LegacyPDO);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopLegacyResourceAllocation: Status %X\n", Status);
        goto Exit;
    }

    LegacyDeviceNode = NULL;

    if (!DeviceNode->Parent && IoResources)
    {
        if (IoResources->InterfaceType == InterfaceTypeUndefined)
            IoResources->InterfaceType = PnpDefaultInterfaceType;

        DeviceNode->Parent = IopRootDeviceNode;
    }

    if ((DeviceNode->Parent && !IoResources) ||
        DeviceNode->ResourceList ||
        DeviceNode->BootResources)
    {
        IopReleaseResources(DeviceNode);
    }

    if (!IoResources)
    {
        LegacyDeviceNode = DeviceNode->OverUsed1.LegacyDeviceNode;
        IopRemoveLegacyDeviceNode(DeviceObject, DeviceNode);
        goto Finish;
    }

    RtlZeroMemory(&RequestEntry, sizeof(RequestEntry));

    RequestEntry.ResourceRequirements = IoResources;
    RequestEntry.PhysicalDevice = LegacyPDO;
    RequestEntry.AllocationType = AllocationType;
    RequestEntry.Flags = 0x80;

    RequestTable = &RequestEntry;
    DeviceCount = 1;

    IopAllocateResources(&DeviceCount, &RequestTable, TRUE, TRUE, NULL);
    Status = RequestEntry.Status;
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLegacyResourceAllocation: Status %X\n", Status);
        IopRemoveLegacyDeviceNode(DeviceObject, DeviceNode);
        goto Exit;
    }

    DeviceNode->ResourceListTranslated = RequestEntry.TranslatedResourceAssignment;

    CmResources = *AllocatedResources;
    if (!CmResources)
        CmResources = RequestEntry.ResourceAssignment;

    Size = PnpDetermineResourceListSize(CmResources);

    DeviceNode->ResourceList = ExAllocatePoolWithTag(PagedPool, Size, 'erpP');
    if (!DeviceNode->ResourceList)
    {
        DPRINT1("IopLegacyResourceAllocation: STATUS_INSUFFICIENT_RESOURCES\n");

        DeviceNode->ResourceList = RequestEntry.ResourceAssignment;

        IopReleaseResources(DeviceNode);
        IopRemoveLegacyDeviceNode(DeviceObject, DeviceNode);

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    DPRINT("IopLegacyResourceAllocation: DeviceNode->ResourceList %p\n", DeviceNode->ResourceList);

    if (*AllocatedResources)
    {
        ASSERT(RequestEntry.ResourceAssignment);
        ExFreePool(RequestEntry.ResourceAssignment);
    }
    else
    {
        *AllocatedResources = RequestEntry.ResourceAssignment;
    }

    RtlCopyMemory(DeviceNode->ResourceList, *AllocatedResources, Size);
    LegacyDeviceNode = DeviceNode->OverUsed1.LegacyDeviceNode;

Finish:

    if (LegacyDeviceNode)
    {
        LegacyCmResources = IopCombineLegacyResources(LegacyDeviceNode);
        if (LegacyCmResources)
        {
            ListSize = PnpDetermineResourceListSize(LegacyCmResources);
            IopWriteAllocatedResourcesToRegistry(LegacyDeviceNode, LegacyCmResources, ListSize);
            ExFreePool(LegacyCmResources);
        }
    }

    if ((AllocationType != ArbiterRequestPnpDetected) && IoResources)
        IopSetLegacyResourcesFlag(DriverObject);

Exit:

    KeReleaseSemaphore(&PpRegistrySemaphore, IO_NO_INCREMENT, 1, FALSE);
    KeLeaveCriticalRegion();

    DPRINT1("IopLegacyResourceAllocation: return Status %X\n", Status);
    return Status;
}

NTSTATUS
NTAPI
IopAssignResourcesToDevices(
    _In_ ULONG DeviceCount,
    _In_ PPNP_RESOURCE_REQUEST ResContext,
    _In_ BOOLEAN Config,
    _Out_ BOOLEAN * OutIsAssigned)
{
    KEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    PDEVICE_NODE DeviceNode;
    ULONG DeviceReported;
    UNICODE_STRING ValueName;
    ULONG ResultLength;
    HANDLE KeyHandle;
    ULONG Idx;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopAssignResourcesToDevices: ResContext - %p, DeviceCount - %X\n",
           ResContext, DeviceCount);

    ASSERT(DeviceCount);

    for (Idx = 0; Idx < DeviceCount; Idx++)
    {
        ResContext[Idx].Flags = 0;
        ResContext[Idx].AllocationType = ArbiterRequestPnpEnumerated;
        ResContext[Idx].ResourceAssignment = NULL;
        ResContext[Idx].Status = STATUS_SUCCESS;

        DeviceNode = IopGetDeviceNode(ResContext[Idx].PhysicalDevice);

        DPRINT("IopAssignResourcesToDevices: Idx - %X, DeviceNode - %p, Flags - %X\n",
               Idx, DeviceNode, DeviceNode->Flags);

        if (!(DeviceNode->Flags & DNF_MADEUP))
        {
            goto Next;
        }

        DeviceReported = 0;

        Status = PnpDeviceObjectToDeviceInstance(ResContext[Idx].PhysicalDevice,
                                                 &KeyHandle,
                                                 KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            goto Next;
        }

        ResultLength = 0;
        RtlInitUnicodeString(&ValueName, L"DeviceReported");

        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 &KeyValueInformation,
                                 sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                   sizeof(DeviceReported),
                                 &ResultLength);

        if (NT_SUCCESS(Status))
        {
            DeviceReported = *(PULONG)&KeyValueInformation.Data[0];
        }

        ZwClose(KeyHandle);

        if (DeviceReported != 0)
        {
            ASSERT(FALSE);
            ResContext[Idx].AllocationType = ArbiterRequestLegacyReported;
        }

Next:
        ResContext[Idx].ResourceRequirements = NULL;
        IopDumpResRequest(&ResContext[Idx]);
    }

    Status = IopAllocateResources(&DeviceCount,
                                  &ResContext,
                                  FALSE,
                                  Config,
                                  OutIsAssigned);
    return Status;
}

NTSTATUS
IopProcessAssignResourcesWorker(
    _In_ PDEVICE_NODE DeviceNode,
    _Inout_ PVOID Context)
{
    PPIP_ASSIGN_RESOURCES_CONTEXT AssignContext = Context;

    PAGED_CODE();

    if (AssignContext->IncludeFailedDevices)
    {
        if ((DeviceNode->Flags & DNF_HAS_PROBLEM) && 
            ((DeviceNode->Problem == CM_PROB_NORMAL_CONFLICT) ||
             (DeviceNode->Problem == CM_PROB_TRANSLATION_FAILED) ||
             (DeviceNode->Problem == CM_PROB_IRQ_TRANSLATION_FAILED)))
        {
            PipClearDevNodeProblem(DeviceNode);
        }
    }

    if ((DeviceNode->Flags & DNF_HAS_PROBLEM)||
        (DeviceNode->Flags & DNF_HAS_PRIVATE_PROBLEM))
    {
        DPRINT("IopProcessAssignResourcesWorker: [%p][%p] Flags %X\n", DeviceNode->PhysicalDeviceObject, DeviceNode, DeviceNode->Flags);
        return STATUS_SUCCESS;
    }

    if (DeviceNode->State == DeviceNodeDriversAdded)
    {
        AssignContext->DeviceList[AssignContext->DeviceCount] = DeviceNode->PhysicalDeviceObject;
        DPRINT("IopProcessAssignResourcesWorker: PDO %p\n", DeviceNode->PhysicalDeviceObject);

        AssignContext->DeviceCount++;
        DPRINT("IopProcessAssignResourcesWorker: DeviceCount %X\n", AssignContext->DeviceCount);
    }

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
IopProcessAssignResources(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN IncludeFailedDevices,
    _Inout_ BOOLEAN * OutIsAssigned)
{
    PPIP_ASSIGN_RESOURCES_CONTEXT AssignContext;
    DEVICETREE_TRAVERSE_CONTEXT Context;
    PPNP_RESOURCE_REQUEST ResRequest;
    PDEVICE_NODE node;
    ULONG AssignContextSize;
    ULONG DeviceCount;
    ULONG MaxConfigs;
    ULONG ConfigNum;
    ULONG ix;
    NTSTATUS Status;
    BOOLEAN IsRetry = TRUE;
    BOOLEAN IsAssignBootConfig;
    BOOLEAN Result = FALSE;

    PAGED_CODE();

    if (IopBootConfigsReserved)
         MaxConfigs = 1;
    else
         MaxConfigs = 2;

    DPRINT("IopProcessAssignResources: [%p] %X, %X\n", DeviceNode, IncludeFailedDevices, MaxConfigs);

    for (ConfigNum = 0; ; ConfigNum++)
    {
        DPRINT("IopProcessAssignResources: ConfigNum %X\n", ConfigNum);

        if (IsRetry == FALSE || ConfigNum >= MaxConfigs)
        {
            DPRINT("IopProcessAssignResources: IsRetry %X\n", IsRetry);
            Result = FALSE;
            break;
        }

        IsRetry = FALSE;

        AssignContextSize = sizeof(PNP_RESOURCE_REQUEST) + (IopNumberDeviceNodes * sizeof(PDEVICE_OBJECT));

        AssignContext = ExAllocatePoolWithTag(PagedPool, AssignContextSize, 'ddpP');
        if (!AssignContext)
        {
            DPRINT1("IopProcessAssignResources: Not enough memory\n");
            Result = FALSE;
            break;
        }

        AssignContext->DeviceCount = 0;
        AssignContext->IncludeFailedDevices = IncludeFailedDevices;

        IopInitDeviceTreeTraverseContext(&Context, DeviceNode, IopProcessAssignResourcesWorker, AssignContext);
        Status = IopTraverseDeviceTree(&Context);

        DeviceCount = AssignContext->DeviceCount;
        if (!DeviceCount)
        {
            DPRINT("IopProcessAssignResources: DeviceCount is 0\n");
            ExFreePoolWithTag(AssignContext, 'ddpP');
            Result = FALSE;
            break;
        }

        DPRINT("IopProcessAssignResources: DeviceCount %x\n", DeviceCount);

        ResRequest = ExAllocatePoolWithTag(PagedPool, (DeviceCount * sizeof(PNP_RESOURCE_REQUEST)), 'ddpP');
        if (!ResRequest)
        {
            DPRINT1("IopProcessAssignResources: Not enough memory\n");
            goto Next;
        }

        RtlZeroMemory(ResRequest, (DeviceCount * sizeof(PNP_RESOURCE_REQUEST)));//need?

        for (ix = 0; ix < DeviceCount; ix++)
        {
            ResRequest[ix].PhysicalDevice = AssignContext->DeviceList[ix];
            ResRequest[ix].ReqList = NULL;
            ResRequest[ix].Priority = 0;
        }

        if (ConfigNum == 0)
            IsAssignBootConfig = IopBootConfigsReserved;
        else
            IsAssignBootConfig = TRUE;

        IopAssignResourcesToDevices(DeviceCount, ResRequest, IsAssignBootConfig, OutIsAssigned);

#if 0
        for (ix = 0; ix < DeviceCount; ix++)
            IopDumpResRequest(&ResRequest[ix]);
#endif

        for (ix = 0; ix < DeviceCount; ix++)
        {
            node = IopGetDeviceNode(ResRequest[ix].PhysicalDevice);
            Status = ResRequest[ix].Status;

            DPRINT("IopProcessAssignResources: (%X) Status[%X] %X\n", ConfigNum, ix, Status);

            if (!NT_SUCCESS(Status))
            {
                switch (Status)
                {
                    case STATUS_RESOURCE_TYPE_NOT_FOUND:
                        ASSERT(FALSE);
                        PipSetDevNodeProblem(node, CM_PROB_UNKNOWN_RESOURCE);
                        break;

                    case STATUS_DEVICE_CONFIGURATION_ERROR:
                        ASSERT(FALSE);
                        PipSetDevNodeProblem(node, CM_PROB_NO_SOFTCONFIG);
                        break;

                    case STATUS_RETRY:
                        DPRINT("IopProcessAssignResources: STATUS_RETRY\n");
                        IsRetry = TRUE;
                        break;

                    case STATUS_PNP_BAD_MPS_TABLE:
                        ASSERT(FALSE);
                        PipSetDevNodeProblem(node, CM_PROB_BIOS_TABLE);
                        break;

                    case STATUS_PNP_TRANSLATION_FAILED:
                        ASSERT(FALSE);
                        PipSetDevNodeProblem(node, CM_PROB_TRANSLATION_FAILED);
                        break;

                    case STATUS_PNP_IRQ_TRANSLATION_FAILED:
                        ASSERT(FALSE);
                        PipSetDevNodeProblem(node, CM_PROB_IRQ_TRANSLATION_FAILED);
                        break;

                    default:
                        ASSERT(FALSE);
                        PipSetDevNodeProblem(node, CM_PROB_NORMAL_CONFLICT);
                        break;
                }
            }
            else
            {
                if (ResRequest[ix].ResourceAssignment)
                {
                    node->ResourceList = ResRequest[ix].ResourceAssignment;
                    node->ResourceListTranslated = ResRequest[ix].TranslatedResourceAssignment;
                }
                else
                {
                    node->Flags |= DNF_NO_RESOURCE_REQUIRED;
                }

                PipSetDevNodeState(node, DeviceNodeResourcesAssigned, FALSE);
                node->UserFlags &= ~4;

                Result = TRUE;
            }
        }

        ExFreePoolWithTag(ResRequest, 'ddpP');

Next:
        ExFreePoolWithTag(AssignContext, 'ddpP');

        if (Result)
           break;
    }

    return Result;
}

NTSTATUS
NTAPI
PipReadDeviceConfiguration(
    _In_ HANDLE KeyHandle,
    _In_ ULONG ConfigType,
    _Out_ PCM_RESOURCE_LIST * OutCmResource,
    _Out_ SIZE_T * OutSize)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    PCM_RESOURCE_LIST CmResource;
    PCM_FULL_RESOURCE_DESCRIPTOR FullList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    PWCHAR ValueName;
    SIZE_T Length;
    ULONG ix;
    ULONG jx;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PipReadDeviceConfiguration: Handle %p, Type %X\n", KeyHandle, ConfigType);

    *OutCmResource = NULL;
    *OutSize = 0;

    if (ConfigType == PIP_CONFIG_TYPE_ALLOC)
        ValueName = L"AllocConfig";
    else if (ConfigType == PIP_CONFIG_TYPE_FORCED)
        ValueName = L"ForcedConfig";
    else if (ConfigType == PIP_CONFIG_TYPE_BOOT)
        ValueName = L"BootConfig";
    else
    {
        DPRINT("PipReadDeviceConfiguration: Unknown ConfigType %X\n", ConfigType);
        return STATUS_INVALID_PARAMETER_2;
    }

    Status = IopGetRegistryValue(KeyHandle, ValueName, &ValueInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipReadDeviceConfiguration: Status %X\n", Status);
        return Status;
    }

    if (ValueInfo->Type != REG_RESOURCE_LIST)
    {
        DPRINT("PipReadDeviceConfiguration: ValueInfo->Type != REG_RESOURCE_LIST\n");
        Status = STATUS_UNSUCCESSFUL;
        ExFreePoolWithTag(ValueInfo, 'uspP');
        return Status;
    }

    Length = ValueInfo->DataLength;
    if (!Length)
    {
        DPRINT("PipReadDeviceConfiguration: Length is 0\n");
        ExFreePoolWithTag(ValueInfo, 'uspP');
        return Status;
    }

    *OutCmResource = ExAllocatePoolWithTag(PagedPool, Length, 'uspP');
    if (!*OutCmResource)
    {
        DPRINT1("PipReadDeviceConfiguration: STATUS_INSUFFICIENT_RESOURCES\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePoolWithTag(ValueInfo, 'uspP');
        return Status;
    }

    *OutSize = ValueInfo->DataLength;

    CmResource = *OutCmResource;

    RtlCopyMemory(CmResource, ((PUCHAR)ValueInfo + ValueInfo->DataOffset), ValueInfo->DataLength);

    FullList = CmResource->List;

    for (ix = 0; ix < CmResource->Count; ix++)
    {
        DPRINT("PipReadDeviceConfiguration: ix %X\n", ix);

        if (FullList->InterfaceType == InterfaceTypeUndefined)
        {
            FullList->BusNumber = 0;
            FullList->InterfaceType = PnpDefaultInterfaceType;
        }

        CmDescriptor = FullList->PartialResourceList.PartialDescriptors;

        for (jx = 0; jx < FullList->PartialResourceList.Count; jx++)
        {
            CmDescriptor = PipGetNextCmPartialDescriptor(CmDescriptor);
        }

        FullList = (PCM_FULL_RESOURCE_DESCRIPTOR)CmDescriptor;
    }

    ExFreePoolWithTag(ValueInfo, 'uspP');

    return Status;
}

NTSTATUS
NTAPI
IopGetDeviceResourcesFromRegistry(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN ResourcesType,
    _In_ ULONG ConfigTypes,
    _Out_ PVOID * OutResource,
    _Out_ SIZE_T * OutSize)
{
    PIO_RESOURCE_REQUIREMENTS_LIST IoResource;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    UNICODE_STRING ValueName;
    HANDLE InstanceKeyHandle = NULL;
    HANDLE KeyHandle = NULL;
    PWCHAR ConfigVectorName;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopGetDeviceResourcesFromRegistry: %p, %X, %X\n", DeviceObject, ResourcesType, ConfigTypes);

    *OutResource = NULL;
    *OutSize = 0;

    Status = PnpDeviceObjectToDeviceInstance(DeviceObject, &InstanceKeyHandle, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopGetDeviceResourcesFromRegistry: Status %X\n", Status);
        return Status;
    }

    if (ResourcesType)
    {
        /* ResourcesType == TRUE (PIO_RESOURCE_REQUIREMENTS_LIST) */

        RtlInitUnicodeString(&ValueName, L"LogConf");

        Status = IopOpenRegistryKeyEx(&KeyHandle, InstanceKeyHandle, &ValueName, KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IopGetDeviceResourcesFromRegistry: Status %X\n", Status);
            ZwClose(InstanceKeyHandle);
            return Status;
        }

        if (ConfigTypes & PIP_CONFIG_TYPE_OVERRIDE)
            ConfigVectorName = L"OverrideConfigVector";
        else if (ConfigTypes & PIP_CONFIG_TYPE_BASIC)
            ConfigVectorName = L"BasicConfigVector";
        else
            goto Exit;

        Status = IopGetRegistryValue(KeyHandle, ConfigVectorName, &ValueInfo);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IopGetDeviceResourcesFromRegistry: Status %X\n", Status);
            goto Exit;
        }

        if (ValueInfo->Type != REG_RESOURCE_REQUIREMENTS_LIST ||
            ValueInfo->DataLength == 0)
        {
            ExFreePoolWithTag(ValueInfo, 'uspP');
            goto Exit;
        }

        IoResource = ExAllocatePoolWithTag(PagedPool, ValueInfo->DataLength, 'uspP');
        *OutResource = IoResource;

        if (!IoResource)
        {
            DPRINT1("IopGetDeviceResourcesFromRegistry: STATUS_INSUFFICIENT_RESOURCES\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ExFreePoolWithTag(ValueInfo, 'uspP');
            goto Exit;
        }

        *OutSize = ValueInfo->DataLength;

        RtlCopyMemory(IoResource, ((PUCHAR)ValueInfo + ValueInfo->DataOffset), ValueInfo->DataLength);

        if (IoResource->InterfaceType == InterfaceTypeUndefined)
        {
            IoResource->BusNumber = 0;
            IoResource->InterfaceType = PnpDefaultInterfaceType;
        }

        ExFreePoolWithTag(ValueInfo, 'uspP');
        goto Exit;
    }

    /* ResourcesType == FALSE (PCM_RESOURCE_LIST) */

    if (ConfigTypes & PIP_CONFIG_TYPE_ALLOC)
    {
        RtlInitUnicodeString(&ValueName, L"Control");

        Status = IopOpenRegistryKeyEx(&KeyHandle, InstanceKeyHandle, &ValueName, KEY_READ);
        if (NT_SUCCESS(Status))
        {
            Status = PipReadDeviceConfiguration(KeyHandle,
                                                PIP_CONFIG_TYPE_ALLOC,
                                                (PCM_RESOURCE_LIST *)OutResource,
                                                OutSize);
            ZwClose(KeyHandle);

            if (NT_SUCCESS(Status))
                goto Exit;

            DPRINT("IopGetDeviceResourcesFromRegistry: Status %X\n", Status);
        }
        else
        {
            DPRINT("IopGetDeviceResourcesFromRegistry: Status %X\n", Status);
        }
    }

    KeyHandle = NULL;

    if (ConfigTypes & PIP_CONFIG_TYPE_FORCED)
    {
        RtlInitUnicodeString(&ValueName, L"LogConf");

        Status = IopOpenRegistryKeyEx(&KeyHandle, InstanceKeyHandle, &ValueName, KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IopGetDeviceResourcesFromRegistry: Status %X\n", Status);
            ZwClose(InstanceKeyHandle);
            return Status;
        }

        Status = PipReadDeviceConfiguration(KeyHandle,
                                            PIP_CONFIG_TYPE_FORCED,
                                            (PCM_RESOURCE_LIST *)OutResource,
                                            OutSize);
        if (NT_SUCCESS(Status))
            goto Exit;

        DPRINT("IopGetDeviceResourcesFromRegistry: Status %X\n", Status);
    }

    if (ConfigTypes & PIP_CONFIG_TYPE_BOOT)
    {
        if (!KeyHandle)
        {
            RtlInitUnicodeString(&ValueName, L"LogConf");

            Status = IopOpenRegistryKeyEx(&KeyHandle, InstanceKeyHandle, &ValueName, KEY_READ);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IopGetDeviceResourcesFromRegistry: Status %X\n", Status);
                goto Exit;
            }
        }

        Status = PipReadDeviceConfiguration(KeyHandle,
                                            PIP_CONFIG_TYPE_BOOT,
                                            (PCM_RESOURCE_LIST *)OutResource,
                                            OutSize);
    }

Exit:

    if (!KeyHandle)
        ZwClose(KeyHandle);

    if (!InstanceKeyHandle)
        ZwClose(InstanceKeyHandle);

    return Status;
}

NTSTATUS
NTAPI
IopBootAllocation(
    _In_ PPNP_REQ_LIST ReqList)
{
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry;
    PPNP_REQ_RESOURCE_ENTRY ReqResDesc;
    ARBITER_PARAMETERS ArbiterParams;
    PLIST_ENTRY Entry;
    PLIST_ENTRY NextEntry;
    LIST_ENTRY List;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("IopBootAllocation: ReqList - %p\n", ReqList);

    InitializeListHead(&List);

    ReqList->AltList1 = ReqList->AltLists;

    IopAddRemoveReqDescs(ReqList->AltLists[0]->ReqDescriptors,
                         ReqList->AltLists[0]->CountDescriptors,
                         &List,
                         TRUE);

    for (Entry = List.Flink; !IsListEmpty(&List); Entry = NextEntry)
    {
        ArbiterEntry = CONTAINING_RECORD(Entry,
                                         PI_RESOURCE_ARBITER_ENTRY,
                                         ActiveArbiterList);
        NextEntry = Entry->Flink;

        if (ArbiterEntry->ResourcesChanged)
        {
            ASSERT(IsListEmpty(&ArbiterEntry->ResourceList) == FALSE);

            ArbiterParams.Parameters.BootAllocation.ArbitrationList =
                                     &ArbiterEntry->ResourceList;

            Status = ArbiterEntry->ArbiterInterface->
                     ArbiterHandler(ArbiterEntry->ArbiterInterface->Context,
                                    ArbiterActionBootAllocation,
                                    &ArbiterParams);

            if (!NT_SUCCESS(Status))
            {
                ReqResDesc = CONTAINING_RECORD(ArbiterEntry,
                                               PNP_REQ_RESOURCE_ENTRY,
                                               Link);
                DPRINT("IopBootAllocation: Failed. Count - %X, PDO - %X\n",
                       ReqResDesc->Count, ReqResDesc->PhysicalDevice);

                ASSERT(FALSE);
                PipDumpIoResourceDescriptor(ReqResDesc->IoDescriptor, 0);
            }

            ArbiterEntry->ResourcesChanged = 0; // FIXME UCHAR ==>> BOOLEAN
            ArbiterEntry->State = 0;

            InitializeListHead(&ArbiterEntry->ActiveArbiterList);
            InitializeListHead(&ArbiterEntry->BestConfig);
            InitializeListHead(&ArbiterEntry->ResourceList);
            InitializeListHead(&ArbiterEntry->BestResourceList);
        }

        if (NextEntry == &List)
        {
            break;
        }
    }

    IopCheckDataStructures(IopRootDeviceNode);

    return Status;
}

NTSTATUS
NTAPI
IopAllocateBootResourcesInternal(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST CmResource)
{
    PDEVICE_NODE DeviceNode = NULL;
    PIO_RESOURCE_REQUIREMENTS_LIST IoResources;
    NTSTATUS Status;
    SIZE_T ListSize;
    PCM_RESOURCE_LIST NewList;
    PNP_RESOURCE_REQUEST ResRequest;
    PPNP_REQ_LIST ReqList;

    PAGED_CODE();
    DPRINT("IopAllocateBootResourcesInternal: AllocationType - %X, DeviceObject - %p\n",
           AllocationType, DeviceObject);

    IoResources = IopCmResourcesToIoResources(0, CmResource, LCPRI_BOOTCONFIG);

    if (!IoResources)
    {
        ASSERT(FALSE);
        DPRINT("IopAllocateBootResourcesInternal: STATUS_UNSUCCESSFUL\n");
        return STATUS_UNSUCCESSFUL;
    }

    DeviceNode = IopGetDeviceNode(DeviceObject);

    DPRINT("IopAllocateBootResourcesInternal: IoResources->AlternativeLists - %X, DeviceNode->BootResources - %p\n",
           IoResources->AlternativeLists, DeviceNode->BootResources);

    DPRINT("\n");
    DPRINT("=== BootResourceRequirementsList =======================\n");
    PipDumpResourceRequirementsList(IoResources, 1);
    DPRINT("=== BootResourceRequirementsList end ===================\n");

    ResRequest.AllocationType = AllocationType;
    ResRequest.ResourceRequirements = IoResources;
    ResRequest.PhysicalDevice = DeviceObject;

    DPRINT("\n");
    DPRINT("==IopResourceRequirementsListToReqList()=================================\n");
    Status = IopResourceRequirementsListToReqList(&ResRequest, &ReqList);
    DPRINT("==IopResourceRequirementsListToReqList() end=============================\n");
    DPRINT("\n");

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopAllocateBootResourcesInternal: Status - %X\n", Status);
        ASSERT(FALSE);
        goto Exit;
    }

    if (!ReqList)
    {
        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    Status = IopBootAllocation(ReqList);

    if (!NT_SUCCESS(Status))
    {
        IopFreeReqList(ReqList);
        goto Exit;
    }

    if (!DeviceNode)
    {
        IopFreeReqList(ReqList);
        goto Exit;
    }

    DeviceNode->Flags |= DNF_BOOT_CONFIG_RESERVED;

    if (DeviceNode->BootResources)
    {
        IopFreeReqList(ReqList);
        goto Exit;
    }

    ListSize = PnpDetermineResourceListSize(CmResource);

    NewList = ExAllocatePoolWithTag(PagedPool, ListSize, 'erpP');
    DeviceNode->BootResources = NewList;

    if (!NewList)
    {
        DPRINT1("IopAllocateBootResourcesInternal: STATUS_INSUFFICIENT_RESOURCES\n");
        ASSERT(FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(NewList, CmResource, ListSize);

    IopFreeReqList(ReqList);

Exit:

    ExFreePoolWithTag(IoResources, 'uspP');

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopAllocateBootResourcesInternal: Status %X\n", Status);
    }

    return Status;
}

NTSTATUS
NTAPI
IopAllocateBootResources(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST CmResource)
{
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopAllocateBootResources: DeviceObject %p\n", DeviceObject);

    KeEnterCriticalRegion();
    KeWaitForSingleObject(&PpRegistrySemaphore, DelayExecution, KernelMode, FALSE, NULL);

    Status = IopAllocateBootResourcesInternal(AllocationType, DeviceObject, CmResource);

    KeReleaseSemaphore(&PpRegistrySemaphore, IO_NO_INCREMENT, 1, FALSE);
    KeLeaveCriticalRegion();

    return Status;
}

NTSTATUS
NTAPI
IopReportBootResources(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST CmResource)
{
    PPNP_RESERVED_RESOURCES_CONTEXT ReservedContext;
    PCM_RESOURCE_LIST NewList;
    PCM_RESOURCE_LIST ReservedResource;
    PDEVICE_NODE DeviceNode;
    SIZE_T ListSize;

    DPRINT("IopReportBootResources: DeviceObject %p, AllocationType %X\n", DeviceObject, AllocationType);

    ListSize = PnpDetermineResourceListSize(CmResource);
    if (!ListSize)
    {
        DPRINT1("IopReportBootResources: ListSize is 0\n");
        ASSERT(FALSE);
        return STATUS_SUCCESS;
    }

    if (DeviceObject)
    {
        DeviceNode = IopGetDeviceNode(DeviceObject);
        ASSERT(DeviceNode);

        if (!(DeviceNode->Flags & DNF_MADEUP))
            return IopAllocateBootResources(AllocationType, DeviceObject, CmResource);

        if (DeviceNode->BootResources == NULL)
        {
            NewList = ExAllocatePoolWithTag(PagedPool, ListSize, 'erpP');

            DeviceNode->BootResources = NewList;
            if (!NewList)
            {
                DPRINT1("IopReportBootResources: STATUS_INSUFFICIENT_RESOURCES\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(NewList, CmResource, ListSize);
        }
    }
    else
    {
        DPRINT1("IopReportBootResources: DeviceObject is NULL\n");
        ASSERT(FALSE);
        DeviceNode = NULL;
    }

    ReservedContext = ExAllocatePoolWithTag(PagedPool, sizeof(PNP_RESERVED_RESOURCES_CONTEXT), 'erpP');
    if (!ReservedContext)
    {
        ASSERT(FALSE);

        if (!DeviceNode)
        {
            DPRINT1("IopReportBootResources: STATUS_INSUFFICIENT_RESOURCES\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (DeviceNode->BootResources)
            ExFreePoolWithTag(DeviceNode->BootResources, 0);
    }

    if (DeviceNode)
    {
        ReservedResource = DeviceNode->BootResources;
        DPRINT("IopReportBootResources: (%p) '%wZ', '%wZ'\n", DeviceNode, &DeviceNode->InstancePath, &DeviceNode->ServiceName);
    }
    else
    {
        ReservedResource = CmResource;
    }

    ReservedContext->ReservedResource = ReservedResource;
    ReservedContext->DeviceObject = DeviceObject;
    ReservedContext->NextReservedContext = IopInitReservedResourceList;

    IopInitReservedResourceList = ReservedContext;

    DPRINT("IopReportBootResources: List %p\n", IopInitReservedResourceList);

    return STATUS_SUCCESS;
}

PCM_RESOURCE_LIST
NTAPI
IopCreateCmResourceList(
    _In_ PCM_RESOURCE_LIST CmResourceList,
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _Out_ PCM_RESOURCE_LIST* OutResourceList)
{
    PCM_RESOURCE_LIST NewCmResourceList;
    PCM_RESOURCE_LIST RemainingCmList;
    PCM_FULL_RESOURCE_DESCRIPTOR CmFullDesc;
    PCM_FULL_RESOURCE_DESCRIPTOR RemainFullDesc;
    PCM_FULL_RESOURCE_DESCRIPTOR NewFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmPartialDesc;
    ULONG SizePartialDesc = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    ULONG AllListsLenght = sizeof(ULONG);
    ULONG FullDescSize;
    ULONG Lenght = 0;
    ULONG DescSize;
    ULONG ix;
    ULONG jx;

    PAGED_CODE();
    DPRINT("IopCreateCmResourceList: %p, Iface %X, Bus %X\n", CmResourceList, InterfaceType, BusNumber);

    if (CmResourceList->Count == 0)
    {
        ASSERT(FALSE); // IoDbgBreakPointEx();
        *OutResourceList = CmResourceList;
        return NULL;
    }

    CmFullDesc = &CmResourceList->List[0];

    for (ix = 0; ix < CmResourceList->Count; ix++)
    {
        DPRINT("IopCreateCmResourceList: [%X] %X, %X, %X, %X, %X\n", ix,
               CmFullDesc->InterfaceType, CmFullDesc->BusNumber, CmFullDesc->PartialResourceList.Version,
               CmFullDesc->PartialResourceList.Revision, CmFullDesc->PartialResourceList.Count);

        FullDescSize = SizePartialDesc;
        CmPartialDesc = CmFullDesc->PartialResourceList.PartialDescriptors;

        for (jx = 0; jx < CmFullDesc->PartialResourceList.Count; jx++)
        {
            if (CmPartialDesc->Type == CmResourceTypeDeviceSpecific)
                DescSize = (SizePartialDesc + CmPartialDesc->u.DeviceSpecificData.DataSize);
            else
                DescSize = SizePartialDesc;

            FullDescSize += DescSize;
            CmPartialDesc = PipGetNextCmPartialDescriptor(CmPartialDesc);
        }

        AllListsLenght += FullDescSize;

        if (CmFullDesc->InterfaceType == InterfaceType &&
            CmFullDesc->BusNumber == BusNumber)
        {
            Lenght += FullDescSize;
        }

        CmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)CmPartialDesc;
    }

    if (Lenght == 0)
    {
        DPRINT("IopCreateCmResourceList: Lenght == 0\n");
        *OutResourceList = CmResourceList;
        return NULL;
    }

    DPRINT("IopCreateCmResourceList: AllLen %X, Len %X, (AllLen-Len) %X\n", AllListsLenght, Lenght, (AllListsLenght - Lenght));

    if (AllListsLenght == (Lenght + sizeof(ULONG)))
    {
        *OutResourceList = NULL;
        return CmResourceList;
    }

    NewCmResourceList = ExAllocatePoolWithTag(PagedPool, (Lenght + sizeof(ULONG)), 'erpP');
    if (!NewCmResourceList)
    {
        DPRINT1("IopCreateCmResourceList: Not enough memory\n");
        *OutResourceList = NULL;
        return NULL;
    }

    DPRINT("IopCreateCmResourceList: NewCmResourceList %p\n", NewCmResourceList);

    ASSERT((AllListsLenght - Lenght) != 0);

    RemainingCmList = ExAllocatePoolWithTag(PagedPool, AllListsLenght - Lenght, 'erpP');
    *OutResourceList = RemainingCmList;
    if (!RemainingCmList)
    {
        DPRINT1("IopCreateCmResourceList: Not enough memory\n");
        ExFreePoolWithTag(NewCmResourceList, 0);
        return NULL;
    }

    DPRINT("IopCreateCmResourceList: *OutResourceList %p\n", *OutResourceList);

    CmFullDesc = &CmResourceList->List[0];

    NewCmResourceList->Count = 0;
    NewFullDesc = &NewCmResourceList->List[0];

    RemainingCmList->Count = 0;
    RemainFullDesc = &RemainingCmList->List[0];

    for (ix = 0; ix < CmResourceList->Count; ix++)
    {
        FullDescSize = SizePartialDesc;
        CmPartialDesc = CmFullDesc->PartialResourceList.PartialDescriptors;

        for (jx = 0; jx < CmFullDesc->PartialResourceList.Count; jx++)
        {
            if (CmPartialDesc->Type == CmResourceTypeDeviceSpecific)
                DescSize = (SizePartialDesc + CmPartialDesc->u.DeviceSpecificData.DataSize);
            else
                DescSize = SizePartialDesc;

            FullDescSize += DescSize;
            CmPartialDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)((ULONG_PTR)CmPartialDesc + DescSize);
        }

        if (CmFullDesc->InterfaceType == InterfaceType &&
            CmFullDesc->BusNumber == BusNumber)
        {
            RtlCopyMemory(NewFullDesc, CmFullDesc, FullDescSize);

            NewFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)((ULONG_PTR)NewFullDesc + FullDescSize);
            NewCmResourceList->Count++;
        }
        else
        {
            RtlCopyMemory(RemainFullDesc, CmFullDesc, FullDescSize);

            RemainFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)((ULONG_PTR)RemainFullDesc + FullDescSize);
            RemainingCmList->Count++;
        }

        CmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)CmPartialDesc;
    }

    return NewCmResourceList;
}

PCM_RESOURCE_LIST
NTAPI
IopCombineCmResourceList(
    _In_ PCM_RESOURCE_LIST BootList,
    _In_ PCM_RESOURCE_LIST AddList)
{
    PCM_RESOURCE_LIST NewList = NULL;
    ULONG BootListSize;
    ULONG AddListSize;
    ULONG Size;

    PAGED_CODE();
    DPRINT("IopCombineCmResourceList: BootList %p, AddList %p\n", BootList, AddList);

    if (!BootList)
        return AddList;

    if (!AddList)
        return BootList;

    BootListSize = PnpDetermineResourceListSize(BootList);
    AddListSize = PnpDetermineResourceListSize(AddList);

    if (!BootListSize)
        return NULL;

    if (!AddListSize)
        return NULL;

    Size = (AddListSize - sizeof(ULONG));
    ASSERT((BootListSize + Size) != 0);

    NewList = ExAllocatePoolWithTag(PagedPool, (BootListSize + Size), 'erpP');
    if (!NewList)
    {
        DPRINT1("IopCombineCmResourceList: NewList is NULL!\n");
        return NULL;
    }

    DPRINT("IopCombineCmResourceList: NewList %p\n", NewList);

    RtlCopyMemory(NewList, BootList, BootListSize);
    RtlCopyMemory((PVOID)((ULONG_PTR)NewList + BootListSize), AddList->List, Size);

    NewList->Count += AddList->Count;

    return NewList;
}

VOID
NTAPI
IopAllocateLegacyBootResources(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber)
{
    PCM_RESOURCE_LIST RemainList;
    PCM_RESOURCE_LIST NewCmList;
    PCM_RESOURCE_LIST PrevBootResources;
    PCM_RESOURCE_LIST CmList;
    PPNP_RESERVED_RESOURCES_CONTEXT Entry = NULL;
    PPNP_RESERVED_RESOURCES_CONTEXT NextEntry;

    DPRINT("IopAllocateLegacyBootResources: %X, %X, %X, %X\n", InterfaceType, BusNumber, IopInitHalDeviceNode, IopInitHalResources);

    if (!IopInitHalDeviceNode)
        goto PnpEnum;

    if (!IopInitHalResources)
        goto PnpEnum;

    RemainList = NULL;

    NewCmList = IopCreateCmResourceList(IopInitHalResources, InterfaceType, BusNumber, &RemainList);
    if (!NewCmList)
    {
        ASSERT(RemainList && RemainList == IopInitHalResources);
        goto PnpEnum;
    }

    if (RemainList)
    {
        ASSERT(IopInitHalResources != NewCmList);
        ASSERT(IopInitHalResources != RemainList);
    }
    else
    {
        ASSERT(NewCmList == IopInitHalResources);
    }

    if (RemainList)
        ExFreePoolWithTag(IopInitHalResources, 0);

    IopInitHalResources = RemainList;
    PrevBootResources = IopInitHalDeviceNode->BootResources;

    IopInitHalDeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;

    DPRINT("IopAllocateLegacyBootResources: Alloc HAL reported res. Iface %X, Bus %X\n", InterfaceType, BusNumber);

    IopAllocateBootResources(ArbiterRequestHalReported, IopInitHalDeviceNode->PhysicalDeviceObject, NewCmList);

    IopInitHalDeviceNode->BootResources = IopCombineCmResourceList(PrevBootResources, NewCmList);
    ASSERT(IopInitHalDeviceNode->BootResources);

    if (PrevBootResources)
        ExFreePoolWithTag(PrevBootResources, 0);

PnpEnum:

    for (NextEntry = IopInitReservedResourceList;
         NextEntry != NULL;
        )
    {
        CmList = NextEntry->ReservedResource;

        DPRINT("IopAllocateLegacyBootResources: Next %X, Iface %X, Bus %X\n", NextEntry, CmList->List[0].InterfaceType, CmList->List[0].BusNumber);
        DPRINT("IopAllocateLegacyBootResources: [%X] ReservedRes %X NextReservedCtx %X\n", NextEntry->DeviceObject, NextEntry->ReservedResource, NextEntry->NextReservedContext);

        if (CmList->List[0].InterfaceType != InterfaceType ||
            CmList->List[0].BusNumber != BusNumber)
        {
            Entry = NextEntry;
            NextEntry = NextEntry->NextReservedContext;
            continue;
        }

        DPRINT("IopAllocateLegacyBootResources: Alloc boot cfg. (made-up device) Iface %X, Bus %X\n", InterfaceType, BusNumber);
        IopAllocateBootResources(ArbiterRequestPnpEnumerated, NextEntry->DeviceObject, CmList);

        if (!NextEntry->DeviceObject)
            ExFreePoolWithTag(CmList, 0);

        if (Entry)
            Entry->NextReservedContext = NextEntry->NextReservedContext;
        else
            IopInitReservedResourceList = NextEntry->NextReservedContext;

        ExFreePoolWithTag(NextEntry, 0);

        if (Entry)
            NextEntry = Entry->NextReservedContext;
        else
            NextEntry = IopInitReservedResourceList;
    }

    DPRINT("IopAllocateLegacyBootResources exit\n");
}

VOID
NTAPI
IopReallocateResources(
    _In_ PDEVICE_NODE DeviceNode)
{
    PNP_RESOURCE_REQUEST RequestTable;
    PPNP_RESOURCE_REQUEST RequestTableEnd = (&RequestTable + 1);
    LIST_ENTRY ConfigList;
    ULONG NoResourceRequiredFlag;
    ULONG DeviceCount;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT1("IopReallocateResources: DeviceNode %p, Flags %X\n", DeviceNode, DeviceNode->Flags);

    KeEnterCriticalRegion();
    KeWaitForSingleObject(&PpRegistrySemaphore, DelayExecution, KernelMode, FALSE, NULL);

    if (!(DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_CHANGED))
    {
        DPRINT1("Resource requirements not changed in IopReallocateResources, returning error!\n");
        KeReleaseSemaphore(&PpRegistrySemaphore, IO_NO_INCREMENT, 1, FALSE);
        KeLeaveCriticalRegion();
        return;
    }

    NoResourceRequiredFlag = (DeviceNode->Flags & DNF_NO_RESOURCE_REQUIRED);
    DeviceNode->Flags = (DeviceNode->Flags & ~DNF_NO_RESOURCE_REQUIRED);

    if (!(DeviceNode->Flags & DNF_NON_STOPPED_REBALANCE))
    {
        Status = IopRebalance(0, NULL);
        goto Exit;
    }

    RtlZeroMemory(&RequestTable, sizeof(RequestTable));

    RequestTable.Flags |= (0x200 | 0x80);
    RequestTable.PhysicalDevice = DeviceNode->PhysicalDeviceObject;

    Status = IopGetResourceRequirementsForAssignTable(&RequestTable, RequestTableEnd, &DeviceCount);
    if (DeviceCount == 0)
        goto Exit;

    if (DeviceNode->ResourceList)
        IopReleaseResourcesInternal(DeviceNode);

    Status = IopFindBestConfiguration(&RequestTable, DeviceCount, &ConfigList);
    if (NT_SUCCESS(Status))
        Status = IopCommitConfiguration(&ConfigList);

    if (!NT_SUCCESS(Status))
    {
        NTSTATUS RestoreResourcesStatus;

        DPRINT1("IopReallocateResources: Status %p\n", Status);

        RestoreResourcesStatus = IopRestoreResourcesInternal(DeviceNode);
        if (!NT_SUCCESS(RestoreResourcesStatus))
        {
            ASSERT(NT_SUCCESS(RestoreResourcesStatus));
            PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_NEED_RESTART);
        }

        IopFreeResourceRequirementsForAssignTable(&RequestTable, RequestTableEnd);

        goto Exit;
    }

    DeviceNode->Flags &= ~(DNF_NON_STOPPED_REBALANCE | DNF_RESOURCE_REQUIREMENTS_CHANGED);

    IopBuildCmResourceLists(&RequestTable, RequestTableEnd);

    if (DeviceNode->ResourceList)
        ExFreePoolWithTag(DeviceNode->ResourceList, 0);

    if (DeviceNode->ResourceListTranslated)
        ExFreePoolWithTag(DeviceNode->ResourceListTranslated, 0);

    DeviceNode->ResourceList = RequestTable.ResourceAssignment;
    DeviceNode->ResourceListTranslated = RequestTable.TranslatedResourceAssignment;

    ASSERT(DeviceNode->State == DeviceNodeStarted);

    Status = IopStartDevice(DeviceNode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopReallocateResources: Status %p\n", Status);
        PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_NORMAL_CONFLICT);
    }

    IopFreeResourceRequirementsForAssignTable(&RequestTable, RequestTableEnd);

Exit:

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopReallocateResources: Status %p\n", Status);
        DeviceNode->Flags = (NoResourceRequiredFlag | (DeviceNode->Flags & ~DNF_NO_RESOURCE_REQUIRED));
    }

    KeReleaseSemaphore(&PpRegistrySemaphore, IO_NO_INCREMENT, 1, FALSE);
    KeLeaveCriticalRegion();
}

/* EOF */
