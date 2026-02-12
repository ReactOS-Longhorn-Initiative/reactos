#include "precomp.h"
#include "acpi_new.h"

volatile BOOLEAN AcpiNewUacpiStarted = FALSE;

NTSTATUS
AcpiNewStartUacpiAndEnumerate(_In_ PACPI_NEW_FDO_EXTENSION FdoExt)
{
    uacpi_status st;

    if (FdoExt->Started)
        return STATUS_SUCCESS;

    st = (uacpi_status)ACPIInitUACPI();
    if (uacpi_unlikely_error(st))
        return STATUS_UNSUCCESSFUL;

    st = uacpi_namespace_load();
    if (uacpi_unlikely_error(st))
    {
        DPRINT1("uacpi_namespace_load error: %s\n", uacpi_status_to_string(st));
        return STATUS_UNSUCCESSFUL;
    }

    /* APIC-focused: prefer IOAPIC interrupt model for _PRT/link devices.
     * Must be done after namespace load (uACPI requires a higher init level).
     */
    st = uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC);
    if (uacpi_unlikely_error(st))
        DPRINT1("uacpi_set_interrupt_model(IOAPIC) failed: %s\n", uacpi_status_to_string(st));

    uacpi_install_default_address_space_handlers();

    /* Install early EC handler before namespace initialization runs _REG/_INI */
    AcpiNewEcInitEarly();

    st = uacpi_namespace_initialize();
    if (uacpi_unlikely_error(st))
    {
        DPRINT1("uacpi_namespace_initialize error: %s\n", uacpi_status_to_string(st));
        return STATUS_UNSUCCESSFUL;
    }

    AcpiNewUacpiStarted = TRUE;

    /* System button fixed events (power/sleep) */
    AcpiNewButtonInit();

    (void)uacpi_install_notify_handler(
        uacpi_namespace_root(),
        AcpiNewNotifyHandler,
        (uacpi_handle)FdoExt
    );

    AcpiNewEnumerateNamespace(FdoExt);

    FdoExt->Started = TRUE;
    FdoExt->Enumerated = TRUE;
    InterlockedExchange(&FdoExt->EnumerationDirty, 0);

    if (FdoExt->PhysicalDeviceObject)
        IoInvalidateDeviceRelations(FdoExt->PhysicalDeviceObject, BusRelations);
    return STATUS_SUCCESS;
}
