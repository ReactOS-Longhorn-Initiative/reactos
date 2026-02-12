#include "precomp.h"
#include "acpi_new.h"

NTSTATUS
AcpiNewBuildIdsFromHid(
    _In_z_ const uacpi_char *Hid,
    _Outptr_result_nullonfailure_ PWSTR *OutDeviceId,
    _Outptr_result_nullonfailure_ PWSTR *OutHardwareIds)
{
    ANSI_STRING hidAnsi;
    UNICODE_STRING hidUni = { 0 };
    NTSTATUS status;
    SIZE_T hidLen;
    SIZE_T deviceIdChars;
    SIZE_T hardwareIdsChars;
    PWSTR deviceId;
    PWSTR hwids;
    WCHAR *p;

    *OutDeviceId = NULL;
    *OutHardwareIds = NULL;

    if (!Hid)
        return STATUS_INVALID_PARAMETER;

    hidLen = strlen(Hid);
    RtlInitAnsiString(&hidAnsi, Hid);
    status = RtlAnsiStringToUnicodeString(&hidUni, &hidAnsi, TRUE);
    if (!NT_SUCCESS(status))
        return status;

    // DeviceId: "ACPI\\" + HID
    deviceIdChars = 5 + hidLen;
    deviceId = (PWSTR)ExAllocatePoolWithTag(PagedPool, (deviceIdChars + 1) * sizeof(WCHAR), 'iAcu');
    if (!deviceId)
    {
        RtlFreeUnicodeString(&hidUni);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    p = deviceId;
    RtlCopyMemory(p, L"ACPI\\", 5 * sizeof(WCHAR));
    p += 5;
    RtlCopyMemory(p, hidUni.Buffer, hidUni.Length);
    p += hidUni.Length / sizeof(WCHAR);
    *p = UNICODE_NULL;

    // HardwareIds: "ACPI\\" + HID + "\0" + "*" + HID + "\0\0"
    hardwareIdsChars = (5 + hidLen) + 1 + (1 + hidLen) + 2;
    hwids = (PWSTR)ExAllocatePoolWithTag(PagedPool, hardwareIdsChars * sizeof(WCHAR), 'hAcu');
    if (!hwids)
    {
        ExFreePoolWithTag(deviceId, 'iAcu');
        RtlFreeUnicodeString(&hidUni);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    p = hwids;
    RtlCopyMemory(p, L"ACPI\\", 5 * sizeof(WCHAR));
    p += 5;
    RtlCopyMemory(p, hidUni.Buffer, hidUni.Length);
    p += hidUni.Length / sizeof(WCHAR);
    *p++ = UNICODE_NULL;
    *p++ = L'*';
    RtlCopyMemory(p, hidUni.Buffer, hidUni.Length);
    p += hidUni.Length / sizeof(WCHAR);
    *p++ = UNICODE_NULL;
    *p++ = UNICODE_NULL;

    RtlFreeUnicodeString(&hidUni);
    *OutDeviceId = deviceId;
    *OutHardwareIds = hwids;
    return STATUS_SUCCESS;
}

NTSTATUS
AcpiNewBuildCompatibleIdsFromCid(
    _In_ uacpi_namespace_node *Node,
    _Outptr_result_maybenull_ PWSTR *OutCompatibleIds)
{
    uacpi_pnp_id_list *cid = NULL;
    uacpi_status st;
    SIZE_T totalChars;
    SIZE_T i;
    PWSTR buf, p;

    *OutCompatibleIds = NULL;

    st = uacpi_eval_cid(Node, &cid);
    if (uacpi_unlikely_error(st) || !cid || cid->num_ids == 0)
    {
        if (cid)
            uacpi_free_pnp_id_list(cid);
        return STATUS_SUCCESS;
    }

    // Each CID -> "ACPI\\" + cid + NUL, plus final NUL.
    totalChars = 1;
    for (i = 0; i < cid->num_ids; i++)
    {
        const uacpi_id_string *id = &cid->ids[i];
        if (!id->value)
            continue;
        totalChars += (5 + strlen(id->value) + 1);
    }

    buf = (PWSTR)ExAllocatePoolWithTag(PagedPool, totalChars * sizeof(WCHAR), 'cAcu');
    if (!buf)
    {
        uacpi_free_pnp_id_list(cid);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    p = buf;
    for (i = 0; i < cid->num_ids; i++)
    {
        ANSI_STRING ansi;
        UNICODE_STRING uni;
        const uacpi_id_string *id = &cid->ids[i];
        if (!id->value)
            continue;

        RtlInitAnsiString(&ansi, id->value);
        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&uni, &ansi, TRUE)))
            continue;

        RtlCopyMemory(p, L"ACPI\\", 5 * sizeof(WCHAR));
        p += 5;
        RtlCopyMemory(p, uni.Buffer, uni.Length);
        p += uni.Length / sizeof(WCHAR);
        *p++ = UNICODE_NULL;
        RtlFreeUnicodeString(&uni);
    }
    *p++ = UNICODE_NULL;

    uacpi_free_pnp_id_list(cid);
    *OutCompatibleIds = buf;
    return STATUS_SUCCESS;
}
