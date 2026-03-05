#include "precomp.h"
#include "acpi_new.h"

#include <uacpi/resources.h>

static
BOOLEAN
AcpiNewIsPciRootBridge(_In_ const PACPI_NEW_PDO_EXTENSION PdoExt)
{
    if (!PdoExt || !PdoExt->HardwareIds)
        return FALSE;
    return (wcsstr(PdoExt->HardwareIds, L"PNP0A03") != NULL ||
            wcsstr(PdoExt->HardwareIds, L"PNP0A08") != NULL);
}

static
ULONG
AcpiNewCountCmDescriptors(_In_ uacpi_resources *Resources)
{
    ULONG count = 0;
    uacpi_resource *res;

    if (!Resources || !Resources->entries)
        return 0;

    res = Resources->entries;
    while (res->type != UACPI_RESOURCE_TYPE_END_TAG)
    {
        switch (res->type)
        {
        case UACPI_RESOURCE_TYPE_IRQ:
            count += res->irq.num_irqs;
            break;
        case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            if (res->extended_irq.direction == UACPI_PRODUCER)
                break;
            count += res->extended_irq.num_irqs;
            break;
        case UACPI_RESOURCE_TYPE_DMA:
            count += res->dma.num_channels;
            break;
        case UACPI_RESOURCE_TYPE_FIXED_DMA:
        case UACPI_RESOURCE_TYPE_IO:
        case UACPI_RESOURCE_TYPE_FIXED_IO:
        case UACPI_RESOURCE_TYPE_MEMORY24:
        case UACPI_RESOURCE_TYPE_MEMORY32:
        case UACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            count++;
            break;
        case UACPI_RESOURCE_TYPE_ADDRESS16:
            if (res->address16.common.direction == UACPI_PRODUCER)
                break;
            count++;
            break;
        case UACPI_RESOURCE_TYPE_ADDRESS32:
            if (res->address32.common.direction == UACPI_PRODUCER)
                break;
            count++;
            break;
        case UACPI_RESOURCE_TYPE_ADDRESS64:
            if (res->address64.common.direction == UACPI_PRODUCER)
                break;
            count++;
            break;
        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED:
            if (res->address64_extended.common.direction == UACPI_PRODUCER)
                break;
            count++;
            break;
        default:
            break;
        }

        res = UACPI_NEXT_RESOURCE(res);
    }

    return count;
}

static
USHORT
AcpiNewInterruptFlags(_In_ UINT8 Triggering)
{
    return (Triggering == UACPI_TRIGGERING_LEVEL) ?
           CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE :
           CM_RESOURCE_INTERRUPT_LATCHED;
}

static
ULONG
AcpiNewMemoryFlags(_In_ UINT8 WriteStatus, _In_ UINT8 Caching)
{
    ULONG flags = 0;
    flags |= (WriteStatus == UACPI_NON_WRITABLE) ?
             CM_RESOURCE_MEMORY_READ_ONLY :
             CM_RESOURCE_MEMORY_READ_WRITE;

    switch (Caching)
    {
    case UACPI_CACHEABLE:
        flags |= CM_RESOURCE_MEMORY_CACHEABLE;
        break;
    case UACPI_CACHEABLE_WRITE_COMBINING:
        flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE;
        break;
    case UACPI_PREFETCHABLE:
        flags |= CM_RESOURCE_MEMORY_PREFETCHABLE;
        break;
    default:
        break;
    }

    return flags;
}

static
NTSTATUS
AcpiNewBuildCmResourceListFromCrs(
    _In_ PACPI_NEW_PDO_EXTENSION PdoExt,
    _Outptr_ PCM_RESOURCE_LIST *OutList)
{
    uacpi_status st;
    uacpi_resources *resources = NULL;
    ULONG count, size;
    PCM_RESOURCE_LIST list;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    uacpi_resource *res;
    ULONG i;

    *OutList = NULL;

    if (!PdoExt || !PdoExt->Node)
        return STATUS_NOT_SUPPORTED;

    st = uacpi_get_current_resources(PdoExt->Node, &resources);
    if (uacpi_unlikely_error(st) || !resources)
        return STATUS_NOT_SUPPORTED;

    count = AcpiNewCountCmDescriptors(resources);
    if (count == 0)
    {
        uacpi_free_resources(resources);
        return STATUS_NOT_SUPPORTED;
    }

    size = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (count - 1);
    list = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(PagedPool, size, 'RpcA');
    if (!list)
    {
        uacpi_free_resources(resources);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(list, size);
    list->Count = 1;
    list->List[0].InterfaceType = Internal;
    list->List[0].BusNumber = 0;
    list->List[0].PartialResourceList.Version = 1;
    list->List[0].PartialResourceList.Revision = 1;
    list->List[0].PartialResourceList.Count = count;
    desc = list->List[0].PartialResourceList.PartialDescriptors;

    res = resources->entries;
    while (res->type != UACPI_RESOURCE_TYPE_END_TAG)
    {
        switch (res->type)
        {
        case UACPI_RESOURCE_TYPE_IRQ:
            for (i = 0; i < res->irq.num_irqs; i++)
            {
                desc->Type = CmResourceTypeInterrupt;
                desc->ShareDisposition = (res->irq.sharing == UACPI_SHARED) ?
                                         CmResourceShareShared :
                                         CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewInterruptFlags(res->irq.triggering);
                desc->u.Interrupt.Level = desc->u.Interrupt.Vector = res->irq.irqs[i];
                desc->u.Interrupt.Affinity = (KAFFINITY)(-1);
                desc++;
            }
            break;

        case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            if (res->extended_irq.direction == UACPI_PRODUCER)
                break;
            for (i = 0; i < res->extended_irq.num_irqs; i++)
            {
                desc->Type = CmResourceTypeInterrupt;
                desc->ShareDisposition = (res->extended_irq.sharing == UACPI_SHARED) ?
                                         CmResourceShareShared :
                                         CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewInterruptFlags(res->extended_irq.triggering);
                desc->u.Interrupt.Level = desc->u.Interrupt.Vector = (ULONG)res->extended_irq.irqs[i];
                desc->u.Interrupt.Affinity = (KAFFINITY)(-1);
                desc++;
            }
            break;

        case UACPI_RESOURCE_TYPE_IO:
            desc->Type = CmResourceTypePort;
            desc->ShareDisposition = CmResourceShareDriverExclusive;
            desc->Flags = CM_RESOURCE_PORT_IO;
            desc->Flags |= (res->io.decode_type == UACPI_DECODE_16) ?
                          CM_RESOURCE_PORT_16_BIT_DECODE :
                          CM_RESOURCE_PORT_10_BIT_DECODE;
            desc->u.Port.Start.QuadPart = res->io.minimum;
            desc->u.Port.Length = res->io.length;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_FIXED_IO:
            desc->Type = CmResourceTypePort;
            desc->ShareDisposition = CmResourceShareDriverExclusive;
            desc->Flags = CM_RESOURCE_PORT_IO;
            desc->u.Port.Start.QuadPart = res->fixed_io.address;
            desc->u.Port.Length = res->fixed_io.length;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_MEMORY24:
            desc->Type = CmResourceTypeMemory;
            desc->ShareDisposition = CmResourceShareDeviceExclusive;
            desc->Flags = AcpiNewMemoryFlags(res->memory24.write_status, UACPI_NON_CACHEABLE);
            desc->u.Memory.Start.QuadPart = res->memory24.minimum;
            desc->u.Memory.Length = res->memory24.length;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_MEMORY32:
            desc->Type = CmResourceTypeMemory;
            desc->ShareDisposition = CmResourceShareDeviceExclusive;
            desc->Flags = AcpiNewMemoryFlags(res->memory32.write_status, UACPI_NON_CACHEABLE);
            desc->u.Memory.Start.QuadPart = res->memory32.minimum;
            desc->u.Memory.Length = (ULONG)res->memory32.length;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            desc->Type = CmResourceTypeMemory;
            desc->ShareDisposition = CmResourceShareDeviceExclusive;
            desc->Flags = AcpiNewMemoryFlags(res->fixed_memory32.write_status, UACPI_NON_CACHEABLE);
            desc->u.Memory.Start.QuadPart = res->fixed_memory32.address;
            desc->u.Memory.Length = (ULONG)res->fixed_memory32.length;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS16:
            if (res->address16.common.direction == UACPI_PRODUCER)
                break;
            if (res->address16.common.type == UACPI_RANGE_BUS)
            {
                desc->Type = CmResourceTypeBusNumber;
                desc->ShareDisposition = CmResourceShareShared;
                desc->Flags = 0;
                desc->u.BusNumber.Start = res->address16.minimum;
                desc->u.BusNumber.Length = res->address16.address_length;
            }
            else if (res->address16.common.type == UACPI_RANGE_IO)
            {
                desc->Type = CmResourceTypePort;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = CM_RESOURCE_PORT_IO;
                if (res->address16.common.decode_type == UACPI_POISITIVE_DECODE)
                    desc->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                desc->u.Port.Start.QuadPart = res->address16.minimum;
                desc->u.Port.Length = res->address16.address_length;
            }
            else
            {
                desc->Type = CmResourceTypeMemory;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewMemoryFlags(res->address16.common.attribute.memory.write_status,
                                                 res->address16.common.attribute.memory.caching);
                desc->u.Memory.Start.QuadPart = res->address16.minimum;
                desc->u.Memory.Length = res->address16.address_length;
            }
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS32:
            if (res->address32.common.direction == UACPI_PRODUCER)
                break;
            if (res->address32.common.type == UACPI_RANGE_BUS)
            {
                desc->Type = CmResourceTypeBusNumber;
                desc->ShareDisposition = CmResourceShareShared;
                desc->Flags = 0;
                desc->u.BusNumber.Start = (ULONG)res->address32.minimum;
                desc->u.BusNumber.Length = (ULONG)res->address32.address_length;
            }
            else if (res->address32.common.type == UACPI_RANGE_IO)
            {
                desc->Type = CmResourceTypePort;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = CM_RESOURCE_PORT_IO;
                if (res->address32.common.decode_type == UACPI_POISITIVE_DECODE)
                    desc->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                desc->u.Port.Start.QuadPart = res->address32.minimum;
                desc->u.Port.Length = (ULONG)res->address32.address_length;
            }
            else
            {
                desc->Type = CmResourceTypeMemory;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewMemoryFlags(res->address32.common.attribute.memory.write_status,
                                                 res->address32.common.attribute.memory.caching);
                desc->u.Memory.Start.QuadPart = res->address32.minimum;
                desc->u.Memory.Length = (ULONG)res->address32.address_length;
            }
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS64:
            if (res->address64.common.direction == UACPI_PRODUCER)
                break;
            if (res->address64.common.type == UACPI_RANGE_BUS)
            {
                desc->Type = CmResourceTypeBusNumber;
                desc->ShareDisposition = CmResourceShareShared;
                desc->Flags = 0;
                desc->u.BusNumber.Start = (ULONG)res->address64.minimum;
                desc->u.BusNumber.Length = (ULONG)res->address64.address_length;
            }
            else if (res->address64.common.type == UACPI_RANGE_IO)
            {
                desc->Type = CmResourceTypePort;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = CM_RESOURCE_PORT_IO;
                if (res->address64.common.decode_type == UACPI_POISITIVE_DECODE)
                    desc->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                desc->u.Port.Start.QuadPart = (LONGLONG)res->address64.minimum;
                desc->u.Port.Length = (ULONG)res->address64.address_length;
            }
            else
            {
                desc->Type = CmResourceTypeMemory;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewMemoryFlags(res->address64.common.attribute.memory.write_status,
                                                 res->address64.common.attribute.memory.caching);
                desc->u.Memory.Start.QuadPart = (LONGLONG)res->address64.minimum;
                desc->u.Memory.Length = (ULONG)res->address64.address_length;
            }
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED:
            if (res->address64_extended.common.direction == UACPI_PRODUCER)
                break;
            if (res->address64_extended.common.type == UACPI_RANGE_BUS)
            {
                desc->Type = CmResourceTypeBusNumber;
                desc->ShareDisposition = CmResourceShareShared;
                desc->Flags = 0;
                desc->u.BusNumber.Start = (ULONG)res->address64_extended.minimum;
                desc->u.BusNumber.Length = (ULONG)res->address64_extended.address_length;
            }
            else if (res->address64_extended.common.type == UACPI_RANGE_IO)
            {
                desc->Type = CmResourceTypePort;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = CM_RESOURCE_PORT_IO;
                if (res->address64_extended.common.decode_type == UACPI_POISITIVE_DECODE)
                    desc->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                desc->u.Port.Start.QuadPart = (LONGLONG)res->address64_extended.minimum;
                desc->u.Port.Length = (ULONG)res->address64_extended.address_length;
            }
            else
            {
                desc->Type = CmResourceTypeMemory;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewMemoryFlags(res->address64_extended.common.attribute.memory.write_status,
                                                 res->address64_extended.common.attribute.memory.caching);
                desc->u.Memory.Start.QuadPart = (LONGLONG)res->address64_extended.minimum;
                desc->u.Memory.Length = (ULONG)res->address64_extended.address_length;
            }
            desc++;
            break;

        default:
            break;
        }

        res = UACPI_NEXT_RESOURCE(res);
    }

    uacpi_free_resources(resources);
    *OutList = list;
    return STATUS_SUCCESS;
}

NTSTATUS
AcpiNewPdoQueryResources(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ PIRP Irp)
{
    NTSTATUS status;

    if (!PdoExt || !PdoExt->Node)
        return STATUS_NOT_SUPPORTED;

    if (AcpiNewIsPciRootBridge(PdoExt))
    {
        uacpi_u64 busNumber = 0;
        uacpi_status st;
        PCM_RESOURCE_LIST resourceList;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;

        st = uacpi_eval_simple_integer(PdoExt->Node, "_BBN", &busNumber);
        if (uacpi_unlikely_error(st))
            busNumber = 0;

        resourceList = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(PagedPool, sizeof(CM_RESOURCE_LIST), 'RpcA');
        if (!resourceList)
        {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(resourceList, sizeof(*resourceList));
        resourceList->Count = 1;
        resourceList->List[0].InterfaceType = Internal;
        resourceList->List[0].BusNumber = 0;
        resourceList->List[0].PartialResourceList.Version = 1;
        resourceList->List[0].PartialResourceList.Revision = 1;
        resourceList->List[0].PartialResourceList.Count = 1;
        desc = resourceList->List[0].PartialResourceList.PartialDescriptors;

        desc->Type = CmResourceTypeBusNumber;
        desc->ShareDisposition = CmResourceShareDeviceExclusive;
        desc->u.BusNumber.Start = (ULONG)busNumber;
        desc->u.BusNumber.Length = 1;

        Irp->IoStatus.Information = (ULONG_PTR)resourceList;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        return STATUS_SUCCESS;
    }

    status = AcpiNewBuildCmResourceListFromCrs(PdoExt, (PCM_RESOURCE_LIST *)&Irp->IoStatus.Information);
    Irp->IoStatus.Status = status;
    return status;
}

static
ULONG
AcpiNewCountReqDescriptors(_In_ uacpi_resources *Resources)
{
    BOOLEAN seenStartDependent = FALSE;
    ULONG count = 0;
    uacpi_resource *res;

    if (!Resources || !Resources->entries)
        return 0;

    res = Resources->entries;
    while (res->type != UACPI_RESOURCE_TYPE_END_TAG && res->type != UACPI_RESOURCE_TYPE_END_DEPENDENT)
    {
        if (res->type == UACPI_RESOURCE_TYPE_START_DEPENDENT)
        {
            if (seenStartDependent)
                break;
            seenStartDependent = TRUE;
            res = UACPI_NEXT_RESOURCE(res);
            continue;
        }

        switch (res->type)
        {
        case UACPI_RESOURCE_TYPE_IRQ:
            count += res->irq.num_irqs;
            break;
        case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            if (res->extended_irq.direction == UACPI_PRODUCER)
                break;
            count += res->extended_irq.num_irqs;
            break;
        case UACPI_RESOURCE_TYPE_DMA:
            count += res->dma.num_channels;
            break;
        case UACPI_RESOURCE_TYPE_FIXED_DMA:
        case UACPI_RESOURCE_TYPE_IO:
        case UACPI_RESOURCE_TYPE_FIXED_IO:
        case UACPI_RESOURCE_TYPE_MEMORY24:
        case UACPI_RESOURCE_TYPE_MEMORY32:
        case UACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            count++;
            break;
        case UACPI_RESOURCE_TYPE_ADDRESS16:
            if (res->address16.common.direction == UACPI_PRODUCER)
                break;
            count++;
            break;
        case UACPI_RESOURCE_TYPE_ADDRESS32:
            if (res->address32.common.direction == UACPI_PRODUCER)
                break;
            count++;
            break;
        case UACPI_RESOURCE_TYPE_ADDRESS64:
            if (res->address64.common.direction == UACPI_PRODUCER)
                break;
            count++;
            break;
        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED:
            if (res->address64_extended.common.direction == UACPI_PRODUCER)
                break;
            count++;
            break;
        default:
            break;
        }

        res = UACPI_NEXT_RESOURCE(res);
    }

    return count;
}

static
NTSTATUS
AcpiNewBuildRequirementsFromResources(
    _In_ uacpi_resources *Resources,
    _In_ BOOLEAN CurrentRes,
    _Outptr_ PIO_RESOURCE_REQUIREMENTS_LIST *OutList)
{
    ULONG count, size;
    PIO_RESOURCE_REQUIREMENTS_LIST list;
    PIO_RESOURCE_DESCRIPTOR desc;
    uacpi_resource *res;
    ULONG i;
    BOOLEAN seenStartDependent = FALSE;

    *OutList = NULL;

    count = AcpiNewCountReqDescriptors(Resources);
    if (count == 0)
        return STATUS_NOT_SUPPORTED;

    size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + sizeof(IO_RESOURCE_DESCRIPTOR) * (count - 1);
    list = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePoolWithTag(PagedPool, size, 'RpcA');
    if (!list)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(list, size);
    list->ListSize = size;
    list->InterfaceType = Internal;
    list->BusNumber = 0;
    list->SlotNumber = 0;
    list->AlternativeLists = 1;
    list->List[0].Version = 1;
    list->List[0].Revision = 1;
    list->List[0].Count = count;
    desc = list->List[0].Descriptors;

    res = Resources->entries;
    while (res->type != UACPI_RESOURCE_TYPE_END_TAG && res->type != UACPI_RESOURCE_TYPE_END_DEPENDENT)
    {
        if (res->type == UACPI_RESOURCE_TYPE_START_DEPENDENT)
        {
            if (seenStartDependent)
                break;
            seenStartDependent = TRUE;
            res = UACPI_NEXT_RESOURCE(res);
            continue;
        }

        switch (res->type)
        {
        case UACPI_RESOURCE_TYPE_IRQ:
            for (i = 0; i < res->irq.num_irqs; i++)
            {
                desc->Option = CurrentRes ? 0 : ((i == 0) ? IO_RESOURCE_PREFERRED : IO_RESOURCE_ALTERNATIVE);
                desc->Type = CmResourceTypeInterrupt;
                desc->ShareDisposition = (res->irq.sharing == UACPI_SHARED) ?
                                         CmResourceShareShared :
                                         CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewInterruptFlags(res->irq.triggering);
                desc->u.Interrupt.MinimumVector = desc->u.Interrupt.MaximumVector = res->irq.irqs[i];
                desc++;
            }
            break;

        case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            if (res->extended_irq.direction == UACPI_PRODUCER)
                break;
            for (i = 0; i < res->extended_irq.num_irqs; i++)
            {
                desc->Option = CurrentRes ? 0 : ((i == 0) ? IO_RESOURCE_PREFERRED : IO_RESOURCE_ALTERNATIVE);
                desc->Type = CmResourceTypeInterrupt;
                desc->ShareDisposition = (res->extended_irq.sharing == UACPI_SHARED) ?
                                         CmResourceShareShared :
                                         CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewInterruptFlags(res->extended_irq.triggering);
                desc->u.Interrupt.MinimumVector =
                    desc->u.Interrupt.MaximumVector = (ULONG)res->extended_irq.irqs[i];
                desc++;
            }
            break;

        case UACPI_RESOURCE_TYPE_IO:
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            desc->Type = CmResourceTypePort;
            desc->ShareDisposition = CmResourceShareDriverExclusive;
            desc->Flags = CM_RESOURCE_PORT_IO;
            desc->Flags |= (res->io.decode_type == UACPI_DECODE_16) ?
                          CM_RESOURCE_PORT_16_BIT_DECODE :
                          CM_RESOURCE_PORT_10_BIT_DECODE;
            desc->u.Port.Alignment = res->io.alignment;
            desc->u.Port.Length = res->io.length;
            desc->u.Port.MinimumAddress.QuadPart = res->io.minimum;
            desc->u.Port.MaximumAddress.QuadPart = res->io.maximum + res->io.length - 1;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_FIXED_IO:
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            desc->Type = CmResourceTypePort;
            desc->ShareDisposition = CmResourceShareDriverExclusive;
            desc->Flags = CM_RESOURCE_PORT_IO;
            desc->u.Port.Alignment = 1;
            desc->u.Port.Length = res->fixed_io.length;
            desc->u.Port.MinimumAddress.QuadPart = res->fixed_io.address;
            desc->u.Port.MaximumAddress.QuadPart = res->fixed_io.address + res->fixed_io.length - 1;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_MEMORY24:
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            desc->Type = CmResourceTypeMemory;
            desc->ShareDisposition = CmResourceShareDeviceExclusive;
            desc->Flags = AcpiNewMemoryFlags(res->memory24.write_status, UACPI_NON_CACHEABLE);
            desc->u.Memory.Alignment = res->memory24.alignment;
            desc->u.Memory.Length = res->memory24.length;
            desc->u.Memory.MinimumAddress.QuadPart = res->memory24.minimum;
            desc->u.Memory.MaximumAddress.QuadPart = res->memory24.maximum + res->memory24.length - 1;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_MEMORY32:
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            desc->Type = CmResourceTypeMemory;
            desc->ShareDisposition = CmResourceShareDeviceExclusive;
            desc->Flags = AcpiNewMemoryFlags(res->memory32.write_status, UACPI_NON_CACHEABLE);
            desc->u.Memory.Alignment = (ULONG)res->memory32.alignment;
            desc->u.Memory.Length = (ULONG)res->memory32.length;
            desc->u.Memory.MinimumAddress.QuadPart = res->memory32.minimum;
            desc->u.Memory.MaximumAddress.QuadPart = res->memory32.maximum + res->memory32.length - 1;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            desc->Type = CmResourceTypeMemory;
            desc->ShareDisposition = CmResourceShareDeviceExclusive;
            desc->Flags = AcpiNewMemoryFlags(res->fixed_memory32.write_status, UACPI_NON_CACHEABLE);
            desc->u.Memory.Alignment = 1;
            desc->u.Memory.Length = (ULONG)res->fixed_memory32.length;
            desc->u.Memory.MinimumAddress.QuadPart = res->fixed_memory32.address;
            desc->u.Memory.MaximumAddress.QuadPart = res->fixed_memory32.address + res->fixed_memory32.length - 1;
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS16:
            if (res->address16.common.direction == UACPI_PRODUCER)
                break;
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            if (res->address16.common.type == UACPI_RANGE_BUS)
            {
                desc->Type = CmResourceTypeBusNumber;
                desc->ShareDisposition = CmResourceShareShared;
                desc->Flags = 0;
                desc->u.BusNumber.MinBusNumber = res->address16.minimum;
                desc->u.BusNumber.MaxBusNumber = res->address16.minimum + res->address16.address_length - 1;
                desc->u.BusNumber.Length = res->address16.address_length;
            }
            else if (res->address16.common.type == UACPI_RANGE_IO)
            {
                desc->Type = CmResourceTypePort;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = CM_RESOURCE_PORT_IO;
                if (res->address16.common.decode_type == UACPI_POISITIVE_DECODE)
                    desc->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                desc->u.Port.Alignment = res->address16.granularity ? res->address16.granularity : 1;
                desc->u.Port.Length = res->address16.address_length;
                desc->u.Port.MinimumAddress.QuadPart = res->address16.minimum;
                desc->u.Port.MaximumAddress.QuadPart = res->address16.maximum;
            }
            else
            {
                desc->Type = CmResourceTypeMemory;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewMemoryFlags(res->address16.common.attribute.memory.write_status,
                                                 res->address16.common.attribute.memory.caching);
                desc->u.Memory.Alignment = res->address16.granularity ? res->address16.granularity : 1;
                desc->u.Memory.Length = res->address16.address_length;
                desc->u.Memory.MinimumAddress.QuadPart = res->address16.minimum;
                desc->u.Memory.MaximumAddress.QuadPart = res->address16.maximum;
            }
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS32:
            if (res->address32.common.direction == UACPI_PRODUCER)
                break;
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            if (res->address32.common.type == UACPI_RANGE_BUS)
            {
                desc->Type = CmResourceTypeBusNumber;
                desc->ShareDisposition = CmResourceShareShared;
                desc->Flags = 0;
                desc->u.BusNumber.MinBusNumber = (ULONG)res->address32.minimum;
                desc->u.BusNumber.MaxBusNumber = (ULONG)(res->address32.minimum + res->address32.address_length - 1);
                desc->u.BusNumber.Length = (ULONG)res->address32.address_length;
            }
            else if (res->address32.common.type == UACPI_RANGE_IO)
            {
                desc->Type = CmResourceTypePort;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = CM_RESOURCE_PORT_IO;
                if (res->address32.common.decode_type == UACPI_POISITIVE_DECODE)
                    desc->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                desc->u.Port.Alignment = res->address32.granularity ? (ULONG)res->address32.granularity : 1;
                desc->u.Port.Length = (ULONG)res->address32.address_length;
                desc->u.Port.MinimumAddress.QuadPart = res->address32.minimum;
                desc->u.Port.MaximumAddress.QuadPart = res->address32.maximum;
            }
            else
            {
                desc->Type = CmResourceTypeMemory;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewMemoryFlags(res->address32.common.attribute.memory.write_status,
                                                 res->address32.common.attribute.memory.caching);
                desc->u.Memory.Alignment = res->address32.granularity ? (ULONG)res->address32.granularity : 1;
                desc->u.Memory.Length = (ULONG)res->address32.address_length;
                desc->u.Memory.MinimumAddress.QuadPart = res->address32.minimum;
                desc->u.Memory.MaximumAddress.QuadPart = res->address32.maximum;
            }
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS64:
            if (res->address64.common.direction == UACPI_PRODUCER)
                break;
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            if (res->address64.common.type == UACPI_RANGE_BUS)
            {
                desc->Type = CmResourceTypeBusNumber;
                desc->ShareDisposition = CmResourceShareShared;
                desc->Flags = 0;
                desc->u.BusNumber.MinBusNumber = (ULONG)res->address64.minimum;
                desc->u.BusNumber.MaxBusNumber = (ULONG)(res->address64.minimum + res->address64.address_length - 1);
                desc->u.BusNumber.Length = (ULONG)res->address64.address_length;
            }
            else if (res->address64.common.type == UACPI_RANGE_IO)
            {
                desc->Type = CmResourceTypePort;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = CM_RESOURCE_PORT_IO;
                if (res->address64.common.decode_type == UACPI_POISITIVE_DECODE)
                    desc->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                desc->u.Port.Alignment = res->address64.granularity ? (ULONG)res->address64.granularity : 1;
                desc->u.Port.Length = (ULONG)res->address64.address_length;
                desc->u.Port.MinimumAddress.QuadPart = (LONGLONG)res->address64.minimum;
                desc->u.Port.MaximumAddress.QuadPart = (LONGLONG)res->address64.maximum;
            }
            else
            {
                desc->Type = CmResourceTypeMemory;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewMemoryFlags(res->address64.common.attribute.memory.write_status,
                                                 res->address64.common.attribute.memory.caching);
                desc->u.Memory.Alignment = res->address64.granularity ? (ULONG)res->address64.granularity : 1;
                desc->u.Memory.Length = (ULONG)res->address64.address_length;
                desc->u.Memory.MinimumAddress.QuadPart = (LONGLONG)res->address64.minimum;
                desc->u.Memory.MaximumAddress.QuadPart = (LONGLONG)res->address64.maximum;
            }
            desc++;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED:
            if (res->address64_extended.common.direction == UACPI_PRODUCER)
                break;
            desc->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
            if (res->address64_extended.common.type == UACPI_RANGE_BUS)
            {
                desc->Type = CmResourceTypeBusNumber;
                desc->ShareDisposition = CmResourceShareShared;
                desc->Flags = 0;
                desc->u.BusNumber.MinBusNumber = (ULONG)res->address64_extended.minimum;
                desc->u.BusNumber.MaxBusNumber = (ULONG)(res->address64_extended.minimum + res->address64_extended.address_length - 1);
                desc->u.BusNumber.Length = (ULONG)res->address64_extended.address_length;
            }
            else if (res->address64_extended.common.type == UACPI_RANGE_IO)
            {
                desc->Type = CmResourceTypePort;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = CM_RESOURCE_PORT_IO;
                if (res->address64_extended.common.decode_type == UACPI_POISITIVE_DECODE)
                    desc->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                desc->u.Port.Alignment = res->address64_extended.granularity ? (ULONG)res->address64_extended.granularity : 1;
                desc->u.Port.Length = (ULONG)res->address64_extended.address_length;
                desc->u.Port.MinimumAddress.QuadPart = (LONGLONG)res->address64_extended.minimum;
                desc->u.Port.MaximumAddress.QuadPart = (LONGLONG)res->address64_extended.maximum;
            }
            else
            {
                desc->Type = CmResourceTypeMemory;
                desc->ShareDisposition = CmResourceShareDeviceExclusive;
                desc->Flags = AcpiNewMemoryFlags(res->address64_extended.common.attribute.memory.write_status,
                                                 res->address64_extended.common.attribute.memory.caching);
                desc->u.Memory.Alignment = res->address64_extended.granularity ? (ULONG)res->address64_extended.granularity : 1;
                desc->u.Memory.Length = (ULONG)res->address64_extended.address_length;
                desc->u.Memory.MinimumAddress.QuadPart = (LONGLONG)res->address64_extended.minimum;
                desc->u.Memory.MaximumAddress.QuadPart = (LONGLONG)res->address64_extended.maximum;
            }
            desc++;
            break;

        default:
            break;
        }

        res = UACPI_NEXT_RESOURCE(res);
    }

    *OutList = list;
    return STATUS_SUCCESS;
}

NTSTATUS
AcpiNewPdoQueryResourceRequirements(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ PIRP Irp)
{
    uacpi_status st;
    uacpi_resources *resources = NULL;
    BOOLEAN currentRes = FALSE;
    NTSTATUS status;

    if (!PdoExt || !PdoExt->Node)
        return STATUS_NOT_SUPPORTED;

    if (AcpiNewIsPciRootBridge(PdoExt))
        return STATUS_NOT_SUPPORTED;

    st = uacpi_get_possible_resources(PdoExt->Node, &resources);
    if (uacpi_unlikely_error(st) || !resources)
    {
        currentRes = TRUE;
        st = uacpi_get_current_resources(PdoExt->Node, &resources);
        if (uacpi_unlikely_error(st) || !resources)
            return STATUS_NOT_SUPPORTED;
    }

    status = AcpiNewBuildRequirementsFromResources(resources,
                                                   currentRes,
                                                   (PIO_RESOURCE_REQUIREMENTS_LIST *)&Irp->IoStatus.Information);

    uacpi_free_resources(resources);
    Irp->IoStatus.Status = status;
    return status;
}
