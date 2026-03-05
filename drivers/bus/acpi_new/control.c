#include "precomp.h"
#include "acpi_new.h"

#include <uacpi/resources.h>
#include <uacpi/tables.h>

static PDEVICE_OBJECT gAcpiNewControlDeviceObject;
static UNICODE_STRING gAcpiNewControlSymlink;

#define IOCTL_ACPI_INTERNAL_GET_PCI_IRQ_ROUTE \
    CTL_CODE(FILE_DEVICE_ACPI, 0x80, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/* Private PCI config-space access via MCFG/ECAM (segment-aware). */
#define IOCTL_ACPI_INTERNAL_PCI_CFG_READ \
    CTL_CODE(FILE_DEVICE_ACPI, 0x81, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_ACPI_INTERNAL_PCI_CFG_WRITE \
    CTL_CODE(FILE_DEVICE_ACPI, 0x82, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

typedef struct _ACPI_PCI_CFG_READ_INPUT
{
    ULONG Segment;
    ULONG Bus;
    ULONG Device;
    ULONG Function;
    ULONG Offset;
    ULONG Width; /* 1,2,4 */
} ACPI_PCI_CFG_READ_INPUT, *PACPI_PCI_CFG_READ_INPUT;

typedef struct _ACPI_PCI_CFG_READ_OUTPUT
{
    ULONG Width; /* echoed */
    ULONG Value; /* value in low bits based on Width */
} ACPI_PCI_CFG_READ_OUTPUT, *PACPI_PCI_CFG_READ_OUTPUT;

typedef struct _ACPI_PCI_CFG_WRITE_INPUT
{
    ULONG Segment;
    ULONG Bus;
    ULONG Device;
    ULONG Function;
    ULONG Offset;
    ULONG Width; /* 1,2,4 */
    ULONG Value; /* value in low bits based on Width */
} ACPI_PCI_CFG_WRITE_INPUT, *PACPI_PCI_CFG_WRITE_INPUT;

typedef struct _ACPI_NEW_MCFG_CACHE
{
    BOOLEAN Present;
    ULONG EntryCount;
    struct acpi_mcfg_allocation *Entries;
} ACPI_NEW_MCFG_CACHE, *PACPI_NEW_MCFG_CACHE;

static ACPI_NEW_MCFG_CACHE gAcpiNewMcfg;
static volatile LONG gAcpiNewMcfgInitState; /* 0=uninit, 1=initing, 2=inited */

static
VOID
AcpiNewEnsureMcfgCached(VOID)
{
    uacpi_table tbl;
    uacpi_status st;

    if (gAcpiNewMcfgInitState == 2)
        return;

    if (InterlockedCompareExchange(&gAcpiNewMcfgInitState, 1, 0) != 0)
    {
        while (gAcpiNewMcfgInitState == 1)
            KeStallExecutionProcessor(10);
        return;
    }

    RtlZeroMemory(&gAcpiNewMcfg, sizeof(gAcpiNewMcfg));

    st = uacpi_table_find_by_signature(ACPI_MCFG_SIGNATURE, &tbl);
    if (uacpi_likely_success(st) && tbl.ptr)
    {
        const struct acpi_mcfg *mcfg = (const struct acpi_mcfg *)tbl.ptr;
        if (mcfg->hdr.length >= sizeof(*mcfg))
        {
            ULONG bytes = (ULONG)mcfg->hdr.length - (ULONG)sizeof(*mcfg);
            ULONG count = bytes / sizeof(struct acpi_mcfg_allocation);
            if (count)
            {
                gAcpiNewMcfg.Entries = ExAllocatePoolWithTag(NonPagedPool, count * sizeof(*gAcpiNewMcfg.Entries), 'gAcu');
                if (gAcpiNewMcfg.Entries)
                {
                    RtlCopyMemory(gAcpiNewMcfg.Entries, mcfg->entries, count * sizeof(*gAcpiNewMcfg.Entries));
                    gAcpiNewMcfg.EntryCount = count;
                    gAcpiNewMcfg.Present = TRUE;
                    DPRINT("acpi_new: MCFG cached for PCI config (%lu entries)\n", count);
                }
            }
        }
        uacpi_table_unref(&tbl);
    }

    InterlockedExchange(&gAcpiNewMcfgInitState, 2);
}

static
BOOLEAN
AcpiNewFindMcfgAllocation(
    _In_ ULONG Segment,
    _In_ ULONG Bus,
    _Out_ const struct acpi_mcfg_allocation **OutAlloc)
{
    if (!OutAlloc)
        return FALSE;

    *OutAlloc = NULL;

    AcpiNewEnsureMcfgCached();

    if (!gAcpiNewMcfg.Present || !gAcpiNewMcfg.Entries || gAcpiNewMcfg.EntryCount == 0)
        return FALSE;

    for (ULONG i = 0; i < gAcpiNewMcfg.EntryCount; i++)
    {
        const struct acpi_mcfg_allocation *a = &gAcpiNewMcfg.Entries[i];
        if ((ULONG)a->segment != Segment)
            continue;
        if (Bus < a->start_bus || Bus > a->end_bus)
            continue;
        *OutAlloc = a;
        return TRUE;
    }

    return FALSE;
}

static
BOOLEAN
AcpiNewMmconfigReadWrite(
    _In_ BOOLEAN Write,
    _In_ ULONG Segment,
    _In_ ULONG Bus,
    _In_ ULONG Device,
    _In_ ULONG Function,
    _In_ ULONG Offset,
    _In_ ULONG Width,
    _Inout_ PULONG Value)
{
    const struct acpi_mcfg_allocation *a;
    PHYSICAL_ADDRESS phys;
    ULONG_PTR base;
    ULONG_PTR aligned;
    ULONG_PTR off;
    ULONG_PTR total;
    PVOID mapped;
    PUCHAR p;

    if (!Value)
        return FALSE;

    if (Device > 31 || Function > 7)
        return FALSE;

    if (!(Width == 1 || Width == 2 || Width == 4))
        return FALSE;

    if (Offset + Width > 0x1000)
        return FALSE;

    if (!AcpiNewFindMcfgAllocation(Segment, Bus, &a))
        return FALSE;

    base = (ULONG_PTR)(a->address +
                       (((ULONGLONG)Bus - (ULONGLONG)a->start_bus) << 20) +
                       ((ULONGLONG)Device << 15) +
                       ((ULONGLONG)Function << 12) +
                       (ULONGLONG)Offset);

    aligned = base & ~((ULONG_PTR)PAGE_SIZE - 1);
    off = base - aligned;
    total = off + Width;

    phys.QuadPart = (LONGLONG)aligned;
    mapped = MmMapIoSpace(phys, total, MmNonCached);
    if (!mapped)
        return FALSE;

    p = (PUCHAR)mapped + off;

    if (!Write)
    {
        if (Width == 1)
        {
            *Value = READ_REGISTER_UCHAR(p);
        }
        else if (Width == 2)
        {
            UCHAR b0 = READ_REGISTER_UCHAR(p + 0);
            UCHAR b1 = READ_REGISTER_UCHAR(p + 1);
            *Value = (ULONG)(b0 | ((ULONG)b1 << 8));
        }
        else
        {
            UCHAR b0 = READ_REGISTER_UCHAR(p + 0);
            UCHAR b1 = READ_REGISTER_UCHAR(p + 1);
            UCHAR b2 = READ_REGISTER_UCHAR(p + 2);
            UCHAR b3 = READ_REGISTER_UCHAR(p + 3);
            *Value = (ULONG)(b0 | ((ULONG)b1 << 8) | ((ULONG)b2 << 16) | ((ULONG)b3 << 24));
        }
    }
    else
    {
        ULONG v = *Value;
        if (Width == 1)
        {
            WRITE_REGISTER_UCHAR(p, (UCHAR)(v & 0xFF));
        }
        else if (Width == 2)
        {
            WRITE_REGISTER_UCHAR(p + 0, (UCHAR)(v & 0xFF));
            WRITE_REGISTER_UCHAR(p + 1, (UCHAR)((v >> 8) & 0xFF));
        }
        else
        {
            WRITE_REGISTER_UCHAR(p + 0, (UCHAR)(v & 0xFF));
            WRITE_REGISTER_UCHAR(p + 1, (UCHAR)((v >> 8) & 0xFF));
            WRITE_REGISTER_UCHAR(p + 2, (UCHAR)((v >> 16) & 0xFF));
            WRITE_REGISTER_UCHAR(p + 3, (UCHAR)((v >> 24) & 0xFF));
        }
    }

    MmUnmapIoSpace(mapped, total);
    return TRUE;
}

typedef struct _ACPI_PCI_IRQ_ROUTE_INPUT
{
    ULONG Segment;
    ULONG Bus;
    ULONG Device;
    ULONG Function;
    ULONG Pin; /* 1..4 */
} ACPI_PCI_IRQ_ROUTE_INPUT, *PACPI_PCI_IRQ_ROUTE_INPUT;

typedef struct _ACPI_PCI_IRQ_ROUTE_OUTPUT
{
    ULONG Gsi;
    ULONG Triggering; /* 0=Level, 1=Edge */
    ULONG Polarity;   /* 0=High, 1=Low */
    ULONG Sharing;    /* 0=Exclusive, 1=Shared */
} ACPI_PCI_IRQ_ROUTE_OUTPUT, *PACPI_PCI_IRQ_ROUTE_OUTPUT;

static
NTSTATUS
AcpiNewResolvePciLinkGsi(
    _In_ uacpi_namespace_node *Link,
    _In_ ULONG SourceIndex,
    _Out_ PACPI_PCI_IRQ_ROUTE_OUTPUT Out)
{
    uacpi_resources *resources = NULL;
    uacpi_status st;
    uacpi_resource *res;
    uacpi_size offset;

    if (!Out)
        return STATUS_INVALID_PARAMETER;

    st = uacpi_get_current_resources(Link, &resources);
    if (uacpi_unlikely_error(st) || !resources || !resources->entries)
        return STATUS_UNSUCCESSFUL;

    res = resources->entries;
    offset = 0;

    while (offset + sizeof(*res) <= resources->length)
    {
        if (res->type == UACPI_RESOURCE_TYPE_EXTENDED_IRQ)
        {
            uacpi_resource_extended_irq *irq = &res->extended_irq;
            ULONG idx = SourceIndex;
            if (irq->num_irqs == 0)
                break;

            if (idx >= irq->num_irqs)
                idx = 0;

            Out->Gsi = irq->irqs[idx];
            Out->Triggering = irq->triggering;
            Out->Polarity = irq->polarity;
            Out->Sharing = irq->sharing;
                 DPRINT("acpi_new: _PRT link resolved: GSI=%lu trig=%lu pol=%lu share=%lu\n",
                     Out->Gsi, Out->Triggering, Out->Polarity, Out->Sharing);
            uacpi_free_resources(resources);
            return STATUS_SUCCESS;
        }
        else if (res->type == UACPI_RESOURCE_TYPE_IRQ)
        {
            uacpi_resource_irq *irq = &res->irq;
            ULONG idx = SourceIndex;
            if (irq->num_irqs == 0)
                break;

            if (idx >= irq->num_irqs)
                idx = 0;

            Out->Gsi = irq->irqs[idx];
            Out->Triggering = irq->triggering;
            Out->Polarity = irq->polarity;
            Out->Sharing = irq->sharing;
                 DPRINT("acpi_new: _PRT link resolved: GSI=%lu trig=%lu pol=%lu share=%lu\n",
                     Out->Gsi, Out->Triggering, Out->Polarity, Out->Sharing);
            uacpi_free_resources(resources);
            return STATUS_SUCCESS;
        }

        offset += res->length;
        res = UACPI_NEXT_RESOURCE(res);
    }

    uacpi_free_resources(resources);
    return STATUS_NOT_FOUND;
}

typedef struct _ACPI_NEW_PCI_BUS_LOOKUP
{
    ULONG Segment;
    ULONG Bus;
    uacpi_namespace_node *Node;
} ACPI_NEW_PCI_BUS_LOOKUP, *PACPI_NEW_PCI_BUS_LOOKUP;

static uacpi_iteration_decision
AcpiNewFindPciBusCb(void *user, uacpi_namespace_node *node, uacpi_u32 depth)
{
    PACPI_NEW_PCI_BUS_LOOKUP lookup = (PACPI_NEW_PCI_BUS_LOOKUP)user;
    uacpi_u64 seg = 0;
    uacpi_u64 bbn = 0;
    uacpi_status st;

    UNREFERENCED_PARAMETER(depth);

    if (!lookup || lookup->Node)
        return UACPI_ITERATION_DECISION_BREAK;

    st = uacpi_eval_simple_integer(node, "_SEG", &seg);
    if (uacpi_unlikely_error(st))
        seg = 0;

    st = uacpi_eval_simple_integer(node, "_BBN", &bbn);
    if (uacpi_unlikely_error(st))
        bbn = 0;

    if ((ULONG)seg != lookup->Segment)
        return UACPI_ITERATION_DECISION_CONTINUE;

    if ((ULONG)bbn != lookup->Bus)
        return UACPI_ITERATION_DECISION_CONTINUE;

    lookup->Node = node;
    return UACPI_ITERATION_DECISION_BREAK;
}

static
NTSTATUS
AcpiNewQueryPciIrqRoute(
    _In_ const ACPI_PCI_IRQ_ROUTE_INPUT *In,
    _Out_ ACPI_PCI_IRQ_ROUTE_OUTPUT *Out)
{
    static BOOLEAN gPrtNoTableLogged;
    static BOOLEAN gPrtRouteNotFoundLogged;
    static BOOLEAN gPrtLinkResolveFailedLogged;
    static const uacpi_char *const hids[] = { "PNP0A08", "PNP0A03", UACPI_NULL };
    ACPI_NEW_PCI_BUS_LOOKUP lookup;
    uacpi_pci_routing_table *table = NULL;
    uacpi_status st;
    ULONG i;
    ULONG reqPin;

    if (!In || !Out)
        return STATUS_INVALID_PARAMETER;

    if (In->Pin < 1 || In->Pin > 4)
        return STATUS_INVALID_PARAMETER;

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        return STATUS_INVALID_DEVICE_STATE;

    if (!AcpiNewUacpiStarted)
        return STATUS_DEVICE_NOT_READY;

    RtlZeroMemory(Out, sizeof(*Out));

    lookup.Segment = In->Segment;
    lookup.Bus = In->Bus;
    lookup.Node = NULL;

    st = uacpi_find_devices_at(uacpi_namespace_root(), hids, AcpiNewFindPciBusCb, &lookup);
    if (uacpi_unlikely_error(st) || !lookup.Node)
        return STATUS_NOT_FOUND;

    st = uacpi_get_pci_routing_table(lookup.Node, &table);
    if (uacpi_unlikely_error(st) || !table)
    {
        if (!gPrtNoTableLogged)
        {
            DPRINT1("acpi_new: _PRT query failed: seg=%lu bus=%lu dev=%lu fun=%lu pin=%lu (no table)\n",
                    In->Segment, In->Bus, In->Device, In->Function, In->Pin);
            gPrtNoTableLogged = TRUE;
        }
        return STATUS_NOT_FOUND;
    }

    /* uACPI uses 0..3 pin encoding for INTA..INTD */
    reqPin = In->Pin - 1;

    for (i = 0; i < (ULONG)table->num_entries; i++)
    {
        const uacpi_pci_routing_table_entry *e = &table->entries[i];
        ULONG eDev = (e->address >> 16) & 0xFFFF;
        ULONG eFun = (e->address & 0xFFFF);

        if (e->pin != reqPin)
            continue;

        if (eDev != In->Device)
            continue;

        if (!(eFun == In->Function || eFun == 0xFFFF))
            continue;

        /* Direct GSI */
        if (!e->source)
        {
            Out->Gsi = e->index;
            Out->Triggering = UACPI_TRIGGERING_LEVEL;
            Out->Polarity = UACPI_POLARITY_ACTIVE_LOW;
            Out->Sharing = UACPI_SHARED;
            uacpi_free_pci_routing_table(table);
            return STATUS_SUCCESS;
        }

        /* Link device */
        {
            NTSTATUS status;
            status = AcpiNewResolvePciLinkGsi(e->source, e->index, Out);
            if (!NT_SUCCESS(status))
            {
                if (!gPrtLinkResolveFailedLogged)
                {
                    DPRINT1("acpi_new: _PRT link resolve failed: seg=%lu bus=%lu dev=%lu fun=%lu pin=%lu status=0x%08lx\n",
                            In->Segment, In->Bus, In->Device, In->Function, In->Pin, status);
                    gPrtLinkResolveFailedLogged = TRUE;
                }
            }
            uacpi_free_pci_routing_table(table);
            return status;
        }
    }

    uacpi_free_pci_routing_table(table);
    if (!gPrtRouteNotFoundLogged)
    {
        DPRINT1("acpi_new: _PRT route not found: seg=%lu bus=%lu dev=%lu fun=%lu pin=%lu\n",
                In->Segment, In->Bus, In->Device, In->Function, In->Pin);
        gPrtRouteNotFoundLogged = TRUE;
    }
    return STATUS_NOT_FOUND;
}

NTSTATUS
AcpiNewControlDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Information = 0;

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_ACPI_INTERNAL_GET_PCI_IRQ_ROUTE:
    {
        ACPI_PCI_IRQ_ROUTE_INPUT *in;
        ACPI_PCI_IRQ_ROUTE_OUTPUT *out;
        ACPI_PCI_IRQ_ROUTE_INPUT inLocal;
        static BOOLEAN gIoctlGetPciIrqRouteFailLogged;

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*in) ||
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(*out))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        in = (ACPI_PCI_IRQ_ROUTE_INPUT *)Irp->AssociatedIrp.SystemBuffer;
        out = (ACPI_PCI_IRQ_ROUTE_OUTPUT *)Irp->AssociatedIrp.SystemBuffer;

        /*
         * METHOD_BUFFERED uses a single SystemBuffer for input and output.
         * AcpiNewQueryPciIrqRoute() zeroes the output, which would otherwise
         * clobber the input if we passed the same pointer for both.
         */
        inLocal = *in;
        status = AcpiNewQueryPciIrqRoute(&inLocal, out);
        if (NT_SUCCESS(status))
            Irp->IoStatus.Information = sizeof(*out);
        else
        {
            if (!gIoctlGetPciIrqRouteFailLogged)
            {
                DPRINT1("acpi_new: IOCTL_GET_PCI_IRQ_ROUTE failed: status=0x%08lx\n", status);
                gIoctlGetPciIrqRouteFailLogged = TRUE;
            }
        }
        break;
    }

    case IOCTL_ACPI_INTERNAL_PCI_CFG_READ:
    {
        ACPI_PCI_CFG_READ_INPUT *in;
        ACPI_PCI_CFG_READ_OUTPUT *out;
        ACPI_PCI_CFG_READ_INPUT inLocal;
        ULONG v;

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*in) ||
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(*out))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        in = (ACPI_PCI_CFG_READ_INPUT *)Irp->AssociatedIrp.SystemBuffer;
        out = (ACPI_PCI_CFG_READ_OUTPUT *)Irp->AssociatedIrp.SystemBuffer;

        inLocal = *in;
        RtlZeroMemory(out, sizeof(*out));
        out->Width = inLocal.Width;

        v = 0xFFFFFFFF;
        if (!AcpiNewMmconfigReadWrite(FALSE,
                                      inLocal.Segment,
                                      inLocal.Bus,
                                      inLocal.Device,
                                      inLocal.Function,
                                      inLocal.Offset,
                                      inLocal.Width,
                                      &v))
        {
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        out->Value = v;
        Irp->IoStatus.Information = sizeof(*out);
        status = STATUS_SUCCESS;
        break;
    }

    case IOCTL_ACPI_INTERNAL_PCI_CFG_WRITE:
    {
        ACPI_PCI_CFG_WRITE_INPUT *in;
        ACPI_PCI_CFG_WRITE_INPUT inLocal;
        ULONG v;

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*in))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        in = (ACPI_PCI_CFG_WRITE_INPUT *)Irp->AssociatedIrp.SystemBuffer;
        inLocal = *in;

        v = inLocal.Value;
        if (!AcpiNewMmconfigReadWrite(TRUE,
                                      inLocal.Segment,
                                      inLocal.Bus,
                                      inLocal.Device,
                                      inLocal.Function,
                                      inLocal.Offset,
                                      inLocal.Width,
                                      &v))
        {
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        Irp->IoStatus.Information = 0;
        status = STATUS_SUCCESS;
        break;
    }

    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    return status;
}

NTSTATUS
AcpiNewCreateControlDevice(_In_ PDRIVER_OBJECT DriverObject)
{
    NTSTATUS status;
    UNICODE_STRING name;

    gAcpiNewControlDeviceObject = NULL;
    RtlInitUnicodeString(&name, L"\\Device\\ACPI");

    status = IoCreateDevice(DriverObject,
                            sizeof(ACPI_NEW_CONTROL_EXTENSION),
                            &name,
                            FILE_DEVICE_ACPI,
                            0,
                            FALSE,
                            &gAcpiNewControlDeviceObject);
    if (!NT_SUCCESS(status))
        return status;

    ((PACPI_NEW_CONTROL_EXTENSION)gAcpiNewControlDeviceObject->DeviceExtension)->Common.Type = AcpiNewDeviceControl;
    ((PACPI_NEW_CONTROL_EXTENSION)gAcpiNewControlDeviceObject->DeviceExtension)->Common.Self = gAcpiNewControlDeviceObject;

    gAcpiNewControlDeviceObject->Flags |= DO_BUFFERED_IO;
    gAcpiNewControlDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    RtlInitUnicodeString(&gAcpiNewControlSymlink, L"\\DosDevices\\ACPI");
    (void)IoCreateSymbolicLink(&gAcpiNewControlSymlink, &name);

    return STATUS_SUCCESS;
}

VOID
AcpiNewDeleteControlDevice(VOID)
{
    if (gAcpiNewControlSymlink.Buffer)
        (void)IoDeleteSymbolicLink(&gAcpiNewControlSymlink);

    if (gAcpiNewControlDeviceObject)
    {
        IoDeleteDevice(gAcpiNewControlDeviceObject);
        gAcpiNewControlDeviceObject = NULL;
    }
}
