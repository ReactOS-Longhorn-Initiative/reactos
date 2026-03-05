#include "precomp.h"
#include "acpi_new.h"

#include <uacpi/event.h>
#include <uacpi/opregion.h>
#include <uacpi/tables.h>
#include <uacpi/utilities.h>
#include <uacpi/resources.h>
#include <uacpi/namespace.h>

#define EC_STATUS_OBF 0x01
#define EC_STATUS_IBF 0x02

#define EC_CMD_READ   0x80
#define EC_CMD_WRITE  0x81
#define EC_CMD_QUERY  0x84

typedef struct _ACPI_NEW_EC_CONTEXT
{
    BOOLEAN Present;
    USHORT DataPort;
    USHORT CmdPort;
    KSPIN_LOCK Lock;

    BOOLEAN HasQueryGpe;
    UCHAR QueryGpe;
    uacpi_namespace_node *Node;

    volatile LONG QueryWorkQueued;
    WORK_QUEUE_ITEM QueryWorkItem;
} ACPI_NEW_EC_CONTEXT, *PACPI_NEW_EC_CONTEXT;

static ACPI_NEW_EC_CONTEXT g_Ec;

static __forceinline UCHAR EcReadStatus(_In_ PACPI_NEW_EC_CONTEXT Ec)
{
    return READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->CmdPort);
}

static __forceinline UCHAR EcReadData(_In_ PACPI_NEW_EC_CONTEXT Ec)
{
    return READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->DataPort);
}

static __forceinline VOID EcWriteCmd(_In_ PACPI_NEW_EC_CONTEXT Ec, _In_ UCHAR Value)
{
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->CmdPort, Value);
}

static __forceinline VOID EcWriteData(_In_ PACPI_NEW_EC_CONTEXT Ec, _In_ UCHAR Value)
{
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Ec->DataPort, Value);
}

static BOOLEAN EcWaitFor(_In_ PACPI_NEW_EC_CONTEXT Ec, _In_ UCHAR Mask, _In_ BOOLEAN Set)
{
    for (ULONG i = 0; i < 5000; i++)
    {
        UCHAR status = EcReadStatus(Ec);
        if (Set)
        {
            if (status & Mask)
                return TRUE;
        }
        else
        {
            if (!(status & Mask))
                return TRUE;
        }

        KeStallExecutionProcessor(10);
    }

    return FALSE;
}

static BOOLEAN EcReadByte(_In_ PACPI_NEW_EC_CONTEXT Ec, _In_ UCHAR Address, _Out_ UCHAR *Value)
{
    KIRQL oldIrql;

    if (!Value)
        return FALSE;

    KeAcquireSpinLock(&Ec->Lock, &oldIrql);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteCmd(Ec, EC_CMD_READ);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteData(Ec, Address);

    if (!EcWaitFor(Ec, EC_STATUS_OBF, TRUE))
        goto Fail;

    *Value = EcReadData(Ec);

    KeReleaseSpinLock(&Ec->Lock, oldIrql);
    return TRUE;

Fail:
    KeReleaseSpinLock(&Ec->Lock, oldIrql);
    return FALSE;
}

static BOOLEAN EcWriteByte(_In_ PACPI_NEW_EC_CONTEXT Ec, _In_ UCHAR Address, _In_ UCHAR Value)
{
    KIRQL oldIrql;

    KeAcquireSpinLock(&Ec->Lock, &oldIrql);

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

    KeReleaseSpinLock(&Ec->Lock, oldIrql);
    return TRUE;

Fail:
    KeReleaseSpinLock(&Ec->Lock, oldIrql);
    return FALSE;
}

static BOOLEAN EcQuery(_In_ PACPI_NEW_EC_CONTEXT Ec, _Out_ UCHAR *Query)
{
    KIRQL oldIrql;

    if (!Query)
        return FALSE;

    KeAcquireSpinLock(&Ec->Lock, &oldIrql);

    if (!EcWaitFor(Ec, EC_STATUS_IBF, FALSE))
        goto Fail;

    EcWriteCmd(Ec, EC_CMD_QUERY);

    if (!EcWaitFor(Ec, EC_STATUS_OBF, TRUE))
        goto Fail;

    *Query = EcReadData(Ec);

    KeReleaseSpinLock(&Ec->Lock, oldIrql);
    return TRUE;

Fail:
    KeReleaseSpinLock(&Ec->Lock, oldIrql);
    return FALSE;
}

static __forceinline CHAR AcpiNewHexDigit(_In_ UCHAR N)
{
    N &= 0xF;
    return (N < 10) ? (CHAR)('0' + N) : (CHAR)('A' + (N - 10));
}

static VOID NTAPI AcpiNewEcQueryWorkItem(_In_ PVOID Context)
{
    PACPI_NEW_EC_CONTEXT ec = (PACPI_NEW_EC_CONTEXT)Context;

    if (!ec)
        return;

    InterlockedExchange(&ec->QueryWorkQueued, 0);

    if (!ec->Present || !ec->Node)
        return;

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        return;

    for (;;)
    {
        UCHAR q;
        CHAR method[5];
        uacpi_status st;

        if (!EcQuery(ec, &q))
            break;

        if (q == 0)
            break;

        method[0] = '_';
        method[1] = 'Q';
        method[2] = AcpiNewHexDigit((UCHAR)(q >> 4));
        method[3] = AcpiNewHexDigit(q);
        method[4] = ANSI_NULL;

        st = uacpi_execute_simple(ec->Node, method);
        if (uacpi_unlikely_error(st))
        {
            DPRINT("acpi_new: EC query %02X execute %s failed: %s\n", q, method, uacpi_status_to_string(st));
        }
    }
}

static uacpi_interrupt_ret AcpiNewEcGpeHandler(
    _In_ uacpi_handle ctx,
    _In_ uacpi_namespace_node *gpe_device,
    _In_ uacpi_u16 idx)
{
    PACPI_NEW_EC_CONTEXT ec = (PACPI_NEW_EC_CONTEXT)ctx;

    UNREFERENCED_PARAMETER(gpe_device);
    UNREFERENCED_PARAMETER(idx);

    if (!ec)
        return UACPI_INTERRUPT_NOT_HANDLED | UACPI_GPE_REENABLE;

    if (InterlockedExchange(&ec->QueryWorkQueued, 1) == 0)
    {
        ExInitializeWorkItem(&ec->QueryWorkItem, AcpiNewEcQueryWorkItem, ec);
        ExQueueWorkItem(&ec->QueryWorkItem, DelayedWorkQueue);
    }

    return UACPI_INTERRUPT_HANDLED | UACPI_GPE_REENABLE;
}

static uacpi_status AcpiNewEcRegionHandler(_In_ uacpi_region_op op, _In_ uacpi_handle op_data)
{
    switch (op)
    {
    case UACPI_REGION_OP_ATTACH:
    {
        uacpi_region_attach_data *data = (uacpi_region_attach_data *)op_data;
        if (!data)
            return UACPI_STATUS_INVALID_ARGUMENT;
        data->out_region_context = NULL;
        return UACPI_STATUS_OK;
    }

    case UACPI_REGION_OP_DETACH:
        return UACPI_STATUS_OK;

    case UACPI_REGION_OP_READ:
    case UACPI_REGION_OP_WRITE:
    {
        uacpi_region_rw_data *data = (uacpi_region_rw_data *)op_data;
        uacpi_u64 value = 0;
        uacpi_u64 offset;

        if (!data)
            return UACPI_STATUS_INVALID_ARGUMENT;

        if (!g_Ec.Present)
            return UACPI_STATUS_NOT_FOUND;

        if (data->byte_width == 0 || data->byte_width > sizeof(uacpi_u64))
            return UACPI_STATUS_INVALID_ARGUMENT;

        offset = data->offset;
        if (offset + data->byte_width > 0x100)
            return UACPI_STATUS_INVALID_ARGUMENT;

        if (op == UACPI_REGION_OP_READ)
        {
            for (uacpi_u8 i = 0; i < data->byte_width; i++)
            {
                UCHAR b;
                if (!EcReadByte(&g_Ec, (UCHAR)(offset + i), &b))
                    return UACPI_STATUS_HARDWARE_TIMEOUT;
                value |= ((uacpi_u64)b) << (i * 8);
            }

            data->value = value;
            return UACPI_STATUS_OK;
        }

        value = data->value;
        for (uacpi_u8 i = 0; i < data->byte_width; i++)
        {
            UCHAR b = (UCHAR)((value >> (i * 8)) & 0xFF);
            if (!EcWriteByte(&g_Ec, (UCHAR)(offset + i), b))
                return UACPI_STATUS_HARDWARE_TIMEOUT;
        }

        return UACPI_STATUS_OK;
    }

    default:
        return UACPI_STATUS_UNIMPLEMENTED;
    }
}

static BOOLEAN AcpiNewEcProbeEcdt(_Out_ PUSHORT OutCmdPort, _Out_ PUSHORT OutDataPort, _Out_opt_ PUCHAR OutGpeBit)
{
    uacpi_table tbl;
    struct acpi_ecdt *ecdt;

    if (!OutCmdPort || !OutDataPort)
        return FALSE;

    if (OutGpeBit)
        *OutGpeBit = 0;

    if (uacpi_unlikely_error(uacpi_table_find_by_signature(ACPI_ECDT_SIGNATURE, &tbl)))
        return FALSE;

    ecdt = (struct acpi_ecdt *)tbl.ptr;
    if (!ecdt)
    {
        (void)uacpi_table_unref(&tbl);
        return FALSE;
    }

    if (ecdt->ec_control.address_space_id != ACPI_AS_ID_SYS_IO ||
        ecdt->ec_data.address_space_id != ACPI_AS_ID_SYS_IO)
    {
        (void)uacpi_table_unref(&tbl);
        return FALSE;
    }

    *OutCmdPort = (USHORT)ecdt->ec_control.address;
    *OutDataPort = (USHORT)ecdt->ec_data.address;

    if (OutGpeBit)
        *OutGpeBit = ecdt->gpe_bit;

    (void)uacpi_table_unref(&tbl);
    return TRUE;
}

static uacpi_namespace_node *AcpiNewEcFindNodeFromEcdt(VOID)
{
    uacpi_table tbl;
    struct acpi_ecdt *ecdt;
    uacpi_namespace_node *node = NULL;

    if (uacpi_unlikely_error(uacpi_table_find_by_signature(ACPI_ECDT_SIGNATURE, &tbl)))
        return NULL;

    ecdt = (struct acpi_ecdt *)tbl.ptr;
    if (!ecdt || !ecdt->ec_id[0])
        goto Exit;

    if (uacpi_unlikely_error(uacpi_namespace_node_find(uacpi_namespace_root(), ecdt->ec_id, &node)))
        node = NULL;

Exit:
    (void)uacpi_table_unref(&tbl);
    return node;
}

typedef struct _ACPI_NEW_EC_CRS_PROBE
{
    USHORT DataPort;
    USHORT CmdPort;
    ULONG IoCount;
} ACPI_NEW_EC_CRS_PROBE, *PACPI_NEW_EC_CRS_PROBE;

typedef struct _ACPI_NEW_EC_NS_PROBE
{
    USHORT DataPort;
    USHORT CmdPort;
    BOOLEAN Found;
    uacpi_namespace_node *Node;
} ACPI_NEW_EC_NS_PROBE, *PACPI_NEW_EC_NS_PROBE;

static uacpi_iteration_decision AcpiNewEcCrsCb(_In_ void *user, _In_ uacpi_resource *resource)
{
    PACPI_NEW_EC_CRS_PROBE probe = (PACPI_NEW_EC_CRS_PROBE)user;
    if (!probe || !resource)
        return UACPI_ITERATION_DECISION_CONTINUE;

    if (resource->type == UACPI_RESOURCE_TYPE_IO)
    {
        uacpi_resource_io *io = &resource->io;
        if (io->length >= 1)
        {
            if (probe->IoCount == 0)
                probe->DataPort = io->minimum;
            else if (probe->IoCount == 1)
                probe->CmdPort = io->minimum;
            probe->IoCount++;
        }
    }
    else if (resource->type == UACPI_RESOURCE_TYPE_FIXED_IO)
    {
        uacpi_resource_fixed_io *io = &resource->fixed_io;
        if (io->length >= 1)
        {
            if (probe->IoCount == 0)
                probe->DataPort = io->address;
            else if (probe->IoCount == 1)
                probe->CmdPort = io->address;
            probe->IoCount++;
        }
    }

    if (probe->IoCount >= 2)
        return UACPI_ITERATION_DECISION_BREAK;

    return UACPI_ITERATION_DECISION_CONTINUE;
}

static uacpi_iteration_decision AcpiNewEcNsProbeCb(_In_ void *user, _In_ uacpi_namespace_node *node, _In_ uacpi_u32 node_depth)
{
    PACPI_NEW_EC_NS_PROBE out = (PACPI_NEW_EC_NS_PROBE)user;
    uacpi_id_string *hid = NULL;
    uacpi_resources *res = NULL;
    ACPI_NEW_EC_CRS_PROBE crs;

    UNREFERENCED_PARAMETER(node_depth);

    if (!out || out->Found || !node)
        return UACPI_ITERATION_DECISION_CONTINUE;

    if (uacpi_unlikely_error(uacpi_eval_hid(node, &hid)) || !hid || !hid->value)
        goto Exit;

    if (strcmp(hid->value, "PNP0C09") != 0)
        goto Exit;

    RtlZeroMemory(&crs, sizeof(crs));
    if (uacpi_unlikely_error(uacpi_get_current_resources(node, &res)) || !res)
        goto Exit;

    (void)uacpi_for_each_resource(res, AcpiNewEcCrsCb, &crs);
    if (crs.IoCount >= 2 && crs.CmdPort && crs.DataPort)
    {
        out->CmdPort = crs.CmdPort;
        out->DataPort = crs.DataPort;
        out->Found = TRUE;
        out->Node = node;
    }

Exit:
    if (res)
        uacpi_free_resources(res);
    if (hid)
        uacpi_free_id_string(hid);

    return out->Found ? UACPI_ITERATION_DECISION_BREAK : UACPI_ITERATION_DECISION_CONTINUE;
}

static BOOLEAN AcpiNewEcProbeNamespaceCrs(_Out_ PUSHORT OutCmdPort, _Out_ PUSHORT OutDataPort)
{
    uacpi_namespace_node *scope;
    ACPI_NEW_EC_NS_PROBE probe = { 0 };

    if (!OutCmdPort || !OutDataPort)
        return FALSE;

    scope = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB);
    if (!scope)
        scope = uacpi_namespace_root();

    (void)uacpi_namespace_for_each_child(
        scope,
        AcpiNewEcNsProbeCb,
        NULL,
        UACPI_OBJECT_DEVICE_BIT,
        UACPI_MAX_DEPTH_ANY,
        &probe
    );

    if (!probe.Found || !probe.CmdPort || !probe.DataPort)
        return FALSE;

    *OutCmdPort = probe.CmdPort;
    *OutDataPort = probe.DataPort;
    return TRUE;
}

VOID AcpiNewEcInitEarly(VOID)
{
    uacpi_status st;
    USHORT cmdPort = 0;
    USHORT dataPort = 0;
    UCHAR gpeBit = 0;
    BOOLEAN haveEcdt;

    if (g_Ec.Present)
        return;

    RtlZeroMemory(&g_Ec, sizeof(g_Ec));
    KeInitializeSpinLock(&g_Ec.Lock);

    g_Ec.HasQueryGpe = FALSE;
    g_Ec.QueryGpe = 0;
    g_Ec.Node = NULL;
    g_Ec.QueryWorkQueued = 0;

    haveEcdt = AcpiNewEcProbeEcdt(&cmdPort, &dataPort, &gpeBit);
    if (!haveEcdt)
        (void)AcpiNewEcProbeNamespaceCrs(&cmdPort, &dataPort);

    if (!cmdPort || !dataPort)
    {
        DPRINT("acpi_new: EC not found, skipping EC opregion handler\n");
        return;
    }

    g_Ec.CmdPort = cmdPort;
    g_Ec.DataPort = dataPort;
    g_Ec.Present = TRUE;

    if (haveEcdt)
    {
        g_Ec.HasQueryGpe = TRUE;
        g_Ec.QueryGpe = gpeBit;
    }

    // Best-effort: find the EC device node for executing _Qxx methods.
    {
        uacpi_namespace_node *scope = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB);
        ACPI_NEW_EC_NS_PROBE probe = { 0 };

        if (!scope)
            scope = uacpi_namespace_root();

        (void)uacpi_namespace_for_each_child(
            scope,
            AcpiNewEcNsProbeCb,
            NULL,
            UACPI_OBJECT_DEVICE_BIT,
            UACPI_MAX_DEPTH_ANY,
            &probe
        );

        if (probe.Found && probe.Node)
            g_Ec.Node = probe.Node;
    }

    if (!g_Ec.Node && haveEcdt)
        g_Ec.Node = AcpiNewEcFindNodeFromEcdt();

    if (g_Ec.Node && !g_Ec.HasQueryGpe)
    {
        uacpi_u64 gpe = 0;
        st = uacpi_eval_simple_integer(g_Ec.Node, "_GPE", &gpe);
        if (!uacpi_unlikely_error(st) && gpe <= 0xFF)
        {
            g_Ec.HasQueryGpe = TRUE;
            g_Ec.QueryGpe = (UCHAR)gpe;
        }
    }

    st = uacpi_install_address_space_handler(
        uacpi_namespace_root(),
        UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER,
        AcpiNewEcRegionHandler,
        (uacpi_handle)&g_Ec
    );
    if (uacpi_unlikely_error(st))
    {
        DPRINT1("acpi_new: uacpi_install_address_space_handler(EC) failed: %s\n", uacpi_status_to_string(st));
        g_Ec.Present = FALSE;
        return;
    }

    (void)uacpi_reg_all_opregions(uacpi_namespace_root(), UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER);

    if (g_Ec.HasQueryGpe && g_Ec.Node)
    {
        st = uacpi_install_gpe_handler(
            UACPI_NULL,
            (uacpi_u16)g_Ec.QueryGpe,
            UACPI_GPE_TRIGGERING_LEVEL,
            AcpiNewEcGpeHandler,
            (uacpi_handle)&g_Ec
        );
        if (uacpi_unlikely_error(st))
        {
            DPRINT("acpi_new: EC query GPE handler not installed: %s\n", uacpi_status_to_string(st));
        }
        else
        {
            (void)uacpi_clear_gpe(UACPI_NULL, (uacpi_u16)g_Ec.QueryGpe);
            (void)uacpi_enable_gpe(UACPI_NULL, (uacpi_u16)g_Ec.QueryGpe);
            DPRINT("acpi_new: EC query GPE enabled: %u\n", g_Ec.QueryGpe);
        }
    }

    DPRINT("acpi_new: EC handler installed cmd=%x data=%x\n", g_Ec.CmdPort, g_Ec.DataPort);
}
