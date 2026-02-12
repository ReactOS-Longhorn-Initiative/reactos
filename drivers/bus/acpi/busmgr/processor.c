/*
 * ACPI Processor helper driver (embedded into acpi.sys).
 *
 * Purpose: minimal bring-up glue for ACPI Processor objects.
 * Optional hack: force the highest performance state (P0) once at start-up
 * when firmware exposes legacy ACPI P-state control registers.
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define _COMPONENT ACPI_PROCESSOR_COMPONENT
ACPI_MODULE_NAME("acpi_processor")

extern ULONG AcpiForceMaxPerf;

typedef struct _ACPI_PROCESSOR_CONTEXT
{
    ACPI_HANDLE Handle;
    struct acpi_device *Device;
} ACPI_PROCESSOR_CONTEXT, *PACPI_PROCESSOR_CONTEXT;

static int acpi_processor_add(struct acpi_device *device);
static int acpi_processor_remove(struct acpi_device *device, int type);

static struct acpi_driver acpi_processor_driver = {
    {0, 0},
    ACPI_PROCESSOR_DRIVER_NAME,
    ACPI_PROCESSOR_CLASS,
    0,
    0,
    ACPI_PROCESSOR_HID,
    {acpi_processor_add, acpi_processor_remove}
};

static BOOLEAN
acpi_processor_parse_gas(const ACPI_OBJECT *Obj, ACPI_GENERIC_ADDRESS *Gas)
{
    if (!Obj || !Gas)
        return FALSE;

    RtlZeroMemory(Gas, sizeof(*Gas));

    if (Obj->Type == ACPI_TYPE_BUFFER)
    {
        if (Obj->Buffer.Pointer && Obj->Buffer.Length >= sizeof(ACPI_GENERIC_ADDRESS))
        {
            RtlCopyMemory(Gas, Obj->Buffer.Pointer, sizeof(ACPI_GENERIC_ADDRESS));
            return TRUE;
        }

        return FALSE;
    }

    if (Obj->Type == ACPI_TYPE_PACKAGE)
    {
        /*
         * Some AML implementations return Register() as a Package of integers.
         * We only handle the common 5/6-integer forms.
         */
        const ACPI_OBJECT *E;
        UINT32 Count;

        Count = Obj->Package.Count;
        E = Obj->Package.Elements;
        if (!E)
            return FALSE;

        if (Count >= 5 &&
            E[0].Type == ACPI_TYPE_INTEGER &&
            E[1].Type == ACPI_TYPE_INTEGER &&
            E[2].Type == ACPI_TYPE_INTEGER &&
            E[3].Type == ACPI_TYPE_INTEGER &&
            E[4].Type == ACPI_TYPE_INTEGER)
        {
            Gas->SpaceId = (UINT8)E[0].Integer.Value;
            Gas->BitWidth = (UINT8)E[1].Integer.Value;
            Gas->BitOffset = (UINT8)E[2].Integer.Value;
            Gas->AccessWidth = (UINT8)E[3].Integer.Value;
            Gas->Address = (UINT64)E[4].Integer.Value;
            return TRUE;
        }

        return FALSE;
    }

    return FALSE;
}

static ACPI_STATUS
acpi_processor_get_p0_control(ACPI_HANDLE Handle, UINT64 *Control)
{
    ACPI_BUFFER Buffer = {ACPI_ALLOCATE_BUFFER, NULL};
    ACPI_OBJECT *Obj;
    ACPI_OBJECT *P0;
    ACPI_STATUS Status;

    if (!Control)
        return AE_BAD_PARAMETER;

    *Control = 0;

    Status = AcpiEvaluateObject(Handle, "_PSS", NULL, &Buffer);
    if (ACPI_FAILURE(Status))
        return Status;

    Obj = (ACPI_OBJECT *)Buffer.Pointer;
    if (!Obj || Obj->Type != ACPI_TYPE_PACKAGE || Obj->Package.Count == 0)
    {
        Status = AE_BAD_DATA;
        goto Exit;
    }

    P0 = &Obj->Package.Elements[0];
    if (!P0 || P0->Type != ACPI_TYPE_PACKAGE || P0->Package.Count < 6)
    {
        Status = AE_BAD_DATA;
        goto Exit;
    }

    /* ACPI _PSS: [Freq, Power, TransLat, BusMastLat, Control, Status] */
    if (P0->Package.Elements[4].Type != ACPI_TYPE_INTEGER)
    {
        Status = AE_BAD_DATA;
        goto Exit;
    }

    *Control = (UINT64)P0->Package.Elements[4].Integer.Value;
    Status = AE_OK;

Exit:
    if (Buffer.Pointer)
        AcpiOsFree(Buffer.Pointer);

    return Status;
}

static ACPI_STATUS
acpi_processor_get_pct_control_gas(ACPI_HANDLE Handle, ACPI_GENERIC_ADDRESS *ControlGas)
{
    ACPI_BUFFER Buffer = {ACPI_ALLOCATE_BUFFER, NULL};
    ACPI_OBJECT *Obj;
    ACPI_STATUS Status;

    if (!ControlGas)
        return AE_BAD_PARAMETER;

    RtlZeroMemory(ControlGas, sizeof(*ControlGas));

    Status = AcpiEvaluateObject(Handle, "_PCT", NULL, &Buffer);
    if (ACPI_FAILURE(Status))
        return Status;

    Obj = (ACPI_OBJECT *)Buffer.Pointer;
    if (!Obj || Obj->Type != ACPI_TYPE_PACKAGE || Obj->Package.Count < 1)
    {
        Status = AE_BAD_DATA;
        goto Exit;
    }

    if (!acpi_processor_parse_gas(&Obj->Package.Elements[0], ControlGas))
    {
        Status = AE_BAD_DATA;
        goto Exit;
    }

    Status = AE_OK;

Exit:
    if (Buffer.Pointer)
        AcpiOsFree(Buffer.Pointer);

    return Status;
}

static VOID
acpi_processor_try_force_p0(struct acpi_device *Device)
{
    ACPI_GENERIC_ADDRESS Cntl;
    UINT64 P0Control;
    ACPI_STATUS Status;

    if (!Device || !Device->handle)
        return;

    if (!AcpiForceMaxPerf)
        return;

    Status = acpi_processor_get_p0_control(Device->handle, &P0Control);
    if (ACPI_FAILURE(Status))
    {
        DPRINT("ACPI CPU: ForceMaxPerf enabled but _PSS missing/invalid (%08x)\n", Status);
        return;
    }

    Status = acpi_processor_get_pct_control_gas(Device->handle, &Cntl);
    if (ACPI_FAILURE(Status))
    {
        DPRINT("ACPI CPU: ForceMaxPerf enabled but _PCT missing/invalid (%08x)\n", Status);
        return;
    }

    DPRINT("ACPI CPU: ForceMaxPerf _PCT SpaceId=%u BitWidth=%u Addr=%I64x\n",
           (UINT32)Cntl.SpaceId,
           (UINT32)Cntl.BitWidth,
           (unsigned long long)Cntl.Address);

    /*
     * Safety: only attempt legacy register programming.
     * Many modern systems use FixedHardware/MSR-based control; we do not
     * poke those here.
     */
    if (Cntl.SpaceId != ACPI_ADR_SPACE_SYSTEM_IO &&
        Cntl.SpaceId != ACPI_ADR_SPACE_SYSTEM_MEMORY)
    {
        DPRINT("ACPI CPU: ForceMaxPerf not applicable (non-IO/MMIO SpaceId=%u)\n", (UINT32)Cntl.SpaceId);
        return;
    }

    Status = AcpiWrite(P0Control, &Cntl);
    DPRINT("ACPI CPU: ForceMaxPerf P0 write %s (%08x), Control=%I64x\n",
           ACPI_SUCCESS(Status) ? "ok" : "failed",
           Status,
           (unsigned long long)P0Control);
}

static int
acpi_processor_add(struct acpi_device *device)
{
    PACPI_PROCESSOR_CONTEXT Ctx;

    ACPI_FUNCTION_TRACE("acpi_processor_add");

    if (!device)
        return_VALUE(-1);

    Ctx = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Ctx), 'PpcA');
    if (!Ctx)
        return_VALUE(-1);

    RtlZeroMemory(Ctx, sizeof(*Ctx));
    Ctx->Handle = device->handle;
    Ctx->Device = device;

    device->driver_data = Ctx;

    /* One-shot kick to P0 if enabled. */
    acpi_processor_try_force_p0(device);

    return_VALUE(0);
}

static int
acpi_processor_remove(struct acpi_device *device, int type)
{
    PACPI_PROCESSOR_CONTEXT Ctx;

    UNREFERENCED_PARAMETER(type);

    ACPI_FUNCTION_TRACE("acpi_processor_remove");

    if (!device)
        return_VALUE(-1);

    Ctx = (PACPI_PROCESSOR_CONTEXT)device->driver_data;
    device->driver_data = NULL;

    if (Ctx)
        ExFreePoolWithTag(Ctx, 'PpcA');

    return_VALUE(0);
}

int
acpi_processor_init(void)
{
    return acpi_bus_register_driver(&acpi_processor_driver);
}

void
acpi_processor_exit(void)
{
    acpi_bus_unregister_driver(&acpi_processor_driver);
}
