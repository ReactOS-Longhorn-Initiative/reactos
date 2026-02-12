#include "precomp.h"

#define NDEBUG
#include <debug.h>

#include <acpica/include/actbl1.h>

#define _COMPONENT ACPI_EC_COMPONENT
ACPI_MODULE_NAME("acpi_ec")

/*
 * Minimal Embedded Controller OpRegion handler.
 *
 * This enables AML EC field accesses (OperationRegion(EmbeddedControl,...))
 * using the standard ACPI EC I/O protocol.
 */

typedef struct _ACPI_EC_DEVICE
{
    BOOLEAN Present;
    BOOLEAN RootHandlerInstalled;
    BOOLEAN DeviceHandlerInstalled;
    BOOLEAN GpeHandlerInstalled;
    USHORT DataPort;
    USHORT CmdPort;
    UCHAR Gpe;
    ACPI_HANDLE Handle;
    struct acpi_device *Device;
    KSPIN_LOCK Lock;
} ACPI_EC_DEVICE, *PACPI_EC_DEVICE;

static ACPI_EC_DEVICE g_EcEarly;
static PACPI_EC_DEVICE g_EcDevice;

#define EC_STATUS_OBF 0x01
#define EC_STATUS_IBF 0x02

#define EC_CMD_READ   0x80
#define EC_CMD_WRITE  0x81
#define EC_CMD_QUERY  0x84

static
__inline
UCHAR
EcReadStatus(_In_ PACPI_EC_DEVICE Ec)
{
    return READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->CmdPort);
}

static
__inline
UCHAR
EcReadData(_In_ PACPI_EC_DEVICE Ec)
{
    return READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->DataPort);
}

static
__inline
VOID
EcWriteCmd(_In_ PACPI_EC_DEVICE Ec, _In_ UCHAR Value)
{
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->CmdPort, Value);
}

static
__inline
VOID
EcWriteData(_In_ PACPI_EC_DEVICE Ec, _In_ UCHAR Value)
{
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->DataPort, Value);
}

static
BOOLEAN
EcWaitFor(_In_ PACPI_EC_DEVICE Ec, _In_ UCHAR Mask, _In_ BOOLEAN Set)
{
    /*
     * EC timing is platform-dependent; keep timeouts conservative.
     * We spin with short stalls to avoid excessive CPU burn.
     */
    for (ULONG i = 0; i < 5000; i++)
    {
        UCHAR Status = EcReadStatus(Ec);
        if (Set)
        {
            if (Status & Mask)
                return TRUE;
        }
        else
        {
            if (!(Status & Mask))
                return TRUE;
        }

        KeStallExecutionProcessor(10);
    }

    return FALSE;
}

static
BOOLEAN
EcReadByte(_In_ PACPI_EC_DEVICE Ec, _In_ UCHAR Address, _Out_ UCHAR *Value)
{
    KIRQL OldIrql;

    if (!Value)
        return FALSE;

    KeAcquireSpinLock(&Ec->Lock, &OldIrql);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteCmd(Ec, EC_CMD_READ);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteData(Ec, Address);

    if (!EcWaitFor(Ec, EC_STATUS_OBF, TRUE))
        goto Fail;

    *Value = EcReadData(Ec);

    KeReleaseSpinLock(&Ec->Lock, OldIrql);
    return TRUE;

Fail:
    KeReleaseSpinLock(&Ec->Lock, OldIrql);
    return FALSE;
}

static
BOOLEAN
EcWriteByte(_In_ PACPI_EC_DEVICE Ec, _In_ UCHAR Address, _In_ UCHAR Value)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ec->Lock, &OldIrql);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteCmd(Ec, EC_CMD_WRITE);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteData(Ec, Address);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteData(Ec, Value);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    KeReleaseSpinLock(&Ec->Lock, OldIrql);
    return TRUE;

Fail:
    KeReleaseSpinLock(&Ec->Lock, OldIrql);
    return FALSE;
}

static
BOOLEAN
EcQuery(_In_ PACPI_EC_DEVICE Ec, _Out_ UCHAR *Value)
{
    KIRQL OldIrql;

    if (!Value)
        return FALSE;

    KeAcquireSpinLock(&Ec->Lock, &OldIrql);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteCmd(Ec, EC_CMD_QUERY);

    if (!EcWaitFor(Ec, EC_STATUS_OBF, TRUE))
        goto Fail;

    *Value = EcReadData(Ec);

    KeReleaseSpinLock(&Ec->Lock, OldIrql);
    return TRUE;

Fail:
    KeReleaseSpinLock(&Ec->Lock, OldIrql);
    return FALSE;
}

static
ACPI_STATUS
AcpiEcSpaceSetup(
    _In_ ACPI_HANDLE RegionHandle,
    _In_ UINT32 Function,
    _In_opt_ PVOID HandlerContext,
    _Outptr_result_maybenull_ PVOID *RegionContext)
{
    UNREFERENCED_PARAMETER(RegionHandle);
    UNREFERENCED_PARAMETER(Function);
    UNREFERENCED_PARAMETER(HandlerContext);

    if (RegionContext)
        *RegionContext = NULL;

    return AE_OK;
}

static
ACPI_STATUS
AcpiEcSpaceHandler(
    _In_ UINT32 Function,
    _In_ ACPI_PHYSICAL_ADDRESS Address,
    _In_ UINT32 BitWidth,
    _Inout_ UINT64 *Value,
    _In_opt_ PVOID HandlerContext,
    _In_opt_ PVOID RegionContext)
{
    ULONG ByteWidth;
    UCHAR ByteValue;

    PACPI_EC_DEVICE Ec = (PACPI_EC_DEVICE)HandlerContext;
    UNREFERENCED_PARAMETER(RegionContext);

    if (!Ec || !Ec->Present || !Value)
        return AE_NOT_EXIST;

    if ((BitWidth == 0) || (BitWidth % 8) != 0)
        return AE_BAD_PARAMETER;

    ByteWidth = BitWidth / 8;
    if (ByteWidth > sizeof(UINT64))
        return AE_BAD_PARAMETER;

    switch (Function)
    {
        case ACPI_READ:
            *Value = 0;
            for (ULONG i = 0; i < ByteWidth; i++)
            {
                if (!EcReadByte(Ec, (UCHAR)(Address + i), &ByteValue))
                    return AE_ERROR;
                *Value |= ((UINT64)ByteValue) << (i * 8);
            }
            return AE_OK;

        case ACPI_WRITE:
            for (ULONG i = 0; i < ByteWidth; i++)
            {
                ByteValue = (UCHAR)((*Value >> (i * 8)) & 0xFF);
                if (!EcWriteByte(Ec, (UCHAR)(Address + i), ByteValue))
                    return AE_ERROR;
            }
            return AE_OK;

        default:
            return AE_BAD_PARAMETER;
    }
}

static
int
acpi_ec_ecdt_probe_into(_Inout_ PACPI_EC_DEVICE Ec)
{
    ACPI_TABLE_HEADER *Table = NULL;
    ACPI_STATUS Status;

    Status = AcpiGetTable(ACPI_SIG_ECDT, 1, &Table);
    if (ACPI_FAILURE(Status) || !Table)
    {
        DPRINT("EC: no ECDT (status %08x)\n", Status);
        return 0;
    }

    if (Table->Length < sizeof(ACPI_TABLE_ECDT))
    {
        DPRINT1("EC: ECDT too small\n");
        return -1;
    }

    ACPI_TABLE_ECDT *Ecdt = (ACPI_TABLE_ECDT *)Table;

    if (Ecdt->Control.SpaceId != ACPI_ADR_SPACE_SYSTEM_IO ||
        Ecdt->Data.SpaceId != ACPI_ADR_SPACE_SYSTEM_IO)
    {
        DPRINT1("EC: ECDT non-IO GAS not supported\n");
        return -1;
    }

    Ec->CmdPort = (USHORT)Ecdt->Control.Address;
    Ec->DataPort = (USHORT)Ecdt->Data.Address;
    Ec->Gpe = Ecdt->Gpe;
    Ec->Present = TRUE;

    DPRINT("EC: ECDT cmd=%x data=%x gpe=%u\n", Ec->CmdPort, Ec->DataPort, Ec->Gpe);
    return 0;
}

int
acpi_ec_ecdt_probe(void)
{
    return acpi_ec_ecdt_probe_into(&g_EcEarly);
}

static
BOOLEAN
acpi_ec_get_ports_from_crs(_In_ ACPI_HANDLE Object, _Inout_ PACPI_EC_DEVICE Ec)
{
    ACPI_BUFFER Buffer;
    ACPI_RESOURCE *Res;
    ACPI_RESOURCE_IO *Io1 = NULL;
    ACPI_RESOURCE_IO *Io2 = NULL;

    Buffer.Length = ACPI_ALLOCATE_BUFFER;
    Buffer.Pointer = NULL;

    if (ACPI_FAILURE(AcpiGetCurrentResources(Object, &Buffer)) || !Buffer.Pointer)
        return FALSE;

    Res = (ACPI_RESOURCE *)Buffer.Pointer;
    while (Res->Type != ACPI_RESOURCE_TYPE_END_TAG)
    {
        if (Res->Type == ACPI_RESOURCE_TYPE_IO)
        {
            if (!Io1) Io1 = &Res->Data.Io;
            else if (!Io2) { Io2 = &Res->Data.Io; break; }
        }
        Res = ACPI_NEXT_RESOURCE(Res);
    }

    if (Io1 && Io2 && Io1->AddressLength >= 1 && Io2->AddressLength >= 1)
    {
        Ec->DataPort = (USHORT)Io1->Minimum;
        Ec->CmdPort = (USHORT)Io2->Minimum;
        Ec->Present = TRUE;
    }

    AcpiOsFree(Buffer.Pointer);
    return Ec->Present ? TRUE : FALSE;
}

static
ACPI_STATUS
AcpiFindEcDeviceCallback(
    _In_ ACPI_HANDLE Object,
    _In_ UINT32 NestingLevel,
    _Inout_opt_ VOID *Context,
    _Inout_opt_ VOID **ReturnValue)
{
    PACPI_EC_DEVICE Ec = (PACPI_EC_DEVICE)Context;

    UNREFERENCED_PARAMETER(NestingLevel);
    UNREFERENCED_PARAMETER(ReturnValue);

    if (!Ec)
        return AE_OK;

    if (acpi_ec_get_ports_from_crs(Object, Ec))
    {
        DPRINT("EC: _CRS ports data=%x cmd=%x\n", Ec->DataPort, Ec->CmdPort);
        return AE_CTRL_TERMINATE;
    }

    return AE_OK;
}

static
UINT32
AcpiEcGpeHandler(
    _In_ ACPI_HANDLE GpeDevice,
    _In_ UINT32 GpeNumber,
    _In_opt_ PVOID Context)
{
    PACPI_EC_DEVICE Ec = (PACPI_EC_DEVICE)Context;
    UCHAR Query;
    char Method[5];

    UNREFERENCED_PARAMETER(GpeDevice);
    UNREFERENCED_PARAMETER(GpeNumber);

    if (!Ec || !Ec->Present || !Ec->Handle)
        return ACPI_INTERRUPT_NOT_HANDLED | ACPI_REENABLE_GPE;

    if (EcQuery(Ec, &Query) && Query)
    {
        sprintf(Method, "_Q%02X", (unsigned int)Query);
        (void)AcpiEvaluateObject(Ec->Handle, Method, NULL, NULL);
    }

    return ACPI_INTERRUPT_HANDLED | ACPI_REENABLE_GPE;
}

int
acpi_ec_early_init(void)
{
    ACPI_STATUS Status;

    if (g_EcEarly.RootHandlerInstalled)
        return 0;

    RtlZeroMemory(&g_EcEarly, sizeof(g_EcEarly));
    KeInitializeSpinLock(&g_EcEarly.Lock);

    /* Prefer ECDT if present */
    (void)acpi_ec_ecdt_probe_into(&g_EcEarly);

    /* Fallback: look for the first EC device and try to read its _CRS */
    if (!g_EcEarly.Present)
    {
        Status = AcpiGetDevices(ACPI_EC_HID,
                                AcpiFindEcDeviceCallback,
                                &g_EcEarly,
                                NULL);
        if (ACPI_FAILURE(Status))
            DPRINT("EC: AcpiGetDevices failed (%08x)\n", Status);
    }

    if (!g_EcEarly.Present)
    {
        DPRINT("EC: not present, not installing handler\n");
        return 0;
    }

    Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
                                            ACPI_ADR_SPACE_EC,
                                            AcpiEcSpaceHandler,
                                            AcpiEcSpaceSetup,
                                            &g_EcEarly);
    if (ACPI_FAILURE(Status))
    {
        DPRINT1("EC: failed to install EC address space handler (%08x)\n", Status);
        return -1;
    }

    g_EcEarly.RootHandlerInstalled = TRUE;
    DPRINT("EC: EC OpRegion handler installed\n");
    return 0;
}

static int acpi_ec_add(struct acpi_device *device);
static int acpi_ec_remove(struct acpi_device *device, int type);

static struct acpi_driver acpi_ec_driver = {
    {0,0},
    ACPI_EC_DRIVER_NAME,
    ACPI_EC_CLASS,
    0,
    0,
    ACPI_EC_HID,
    {acpi_ec_add, acpi_ec_remove}
};

static
int
acpi_ec_add(
    struct acpi_device *device)
{
    ACPI_STATUS Status;
    PACPI_EC_DEVICE Ec;

    if (!device)
        return -1;

    if (g_EcDevice)
    {
        /* Only one EC supported for now */
        return 0;
    }

    Ec = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Ec), 'CECA');
    if (!Ec)
        return -14;

    RtlZeroMemory(Ec, sizeof(*Ec));
    KeInitializeSpinLock(&Ec->Lock);
    Ec->Handle = device->handle;
    Ec->Device = device;

    /* Reuse early-detected ports when available; otherwise try this device's _CRS */
    if (g_EcEarly.Present)
    {
        Ec->CmdPort = g_EcEarly.CmdPort;
        Ec->DataPort = g_EcEarly.DataPort;
        Ec->Gpe = g_EcEarly.Gpe;
        Ec->Present = TRUE;
    }
    else
    {
        (void)acpi_ec_get_ports_from_crs(device->handle, Ec);
    }

    if (!Ec->Present)
    {
        ExFreePoolWithTag(Ec, 'CECA');
        return -1;
    }

    sprintf(acpi_device_name(device), "%s", ACPI_EC_DEVICE_NAME);
    sprintf(acpi_device_class(device), "%s", ACPI_EC_CLASS);
    acpi_driver_data(device) = Ec;
    g_EcDevice = Ec;

    /* Install an EC OpRegion handler scoped to the EC device */
    Status = AcpiInstallAddressSpaceHandler(device->handle,
                                            ACPI_ADR_SPACE_EC,
                                            AcpiEcSpaceHandler,
                                            AcpiEcSpaceSetup,
                                            Ec);
    if (ACPI_FAILURE(Status) && Status != AE_ALREADY_EXISTS)
    {
        DPRINT1("EC: failed to install device EC handler (%08x)\n", Status);
        g_EcDevice = NULL;
        acpi_driver_data(device) = NULL;
        ExFreePoolWithTag(Ec, 'CECA');
        return -1;
    }
    Ec->DeviceHandlerInstalled = TRUE;

    /* If we know the EC GPE, install a handler and enable it */
    if (Ec->Gpe)
    {
        Status = AcpiInstallGpeHandler(NULL,
                                       Ec->Gpe,
                                       ACPI_GPE_LEVEL_TRIGGERED,
                                       AcpiEcGpeHandler,
                                       Ec);
        if (ACPI_SUCCESS(Status) || Status == AE_ALREADY_EXISTS)
        {
            (void)AcpiEnableGpe(NULL, Ec->Gpe);
            Ec->GpeHandlerInstalled = TRUE;
        }
        else
        {
            DPRINT("EC: unable to install GPE handler (%08x)\n", Status);
        }
    }

    DPRINT("EC: attached to %s (cmd=%x data=%x gpe=%u)\n",
           acpi_device_name(device), Ec->CmdPort, Ec->DataPort, (unsigned int)Ec->Gpe);

    return 0;
}

static
int
acpi_ec_remove(
    struct acpi_device *device,
    int type)
{
    PACPI_EC_DEVICE Ec;

    UNREFERENCED_PARAMETER(type);

    if (!device)
        return -1;

    Ec = (PACPI_EC_DEVICE)acpi_driver_data(device);
    if (!Ec)
        return -1;

    if (Ec->GpeHandlerInstalled && Ec->Gpe)
    {
        (void)AcpiDisableGpe(NULL, Ec->Gpe);
        (void)AcpiRemoveGpeHandler(NULL, Ec->Gpe, AcpiEcGpeHandler);
        Ec->GpeHandlerInstalled = FALSE;
    }

    if (Ec->DeviceHandlerInstalled)
    {
        (void)AcpiRemoveAddressSpaceHandler(device->handle,
                                            ACPI_ADR_SPACE_EC,
                                            AcpiEcSpaceHandler);
        Ec->DeviceHandlerInstalled = FALSE;
    }

    acpi_driver_data(device) = NULL;
    g_EcDevice = NULL;
    ExFreePoolWithTag(Ec, 'CECA');

    return 0;
}

int
acpi_ec_init(void)
{
    int result;

    /* Ensure early OpRegion handler is installed if possible */
    (void)acpi_ec_early_init();

    result = acpi_bus_register_driver(&acpi_ec_driver);
    if (result < 0)
        return -15;

    return 0;
}

void
acpi_ec_exit(void)
{
    acpi_bus_unregister_driver(&acpi_ec_driver);

    if (g_EcEarly.RootHandlerInstalled)
    {
        (void)AcpiRemoveAddressSpaceHandler(ACPI_ROOT_OBJECT,
                                            ACPI_ADR_SPACE_EC,
                                            AcpiEcSpaceHandler);
        g_EcEarly.RootHandlerInstalled = FALSE;
    }

    g_EcEarly.Present = FALSE;
}
