#include "precomp.h"

//#define NDEBUG
#include <debug.h>

#include <uacpi/tables.h>

static uacpi_status UacpiInitMcfgCache(VOID);

UINT32
ACPIInitUACPI(void)
{
    uacpi_status status = uacpi_initialize(0);
    if (uacpi_unlikely_error(status))
    {
        DPRINT1("uacpi_initialize error: %s\n", uacpi_status_to_string(status));
    }

    /* Best-effort: cache MCFG so uACPI can access PCI config via ECAM when present. */
    if (uacpi_likely_success(status))
        (void)UacpiInitMcfgCache();

    return status;
}

typedef struct _UACPI_MCFG_CACHE
{
    BOOLEAN Initialized;
    BOOLEAN Present;
    ULONG EntryCount;
    struct acpi_mcfg_allocation *Entries;
} UACPI_MCFG_CACHE, *PUACPI_MCFG_CACHE;

static UACPI_MCFG_CACHE g_UacpiMcfg;
static volatile LONG g_UacpiEcamUseCount;

static
uacpi_status
UacpiInitMcfgCache(VOID)
{
    uacpi_table tbl;
    uacpi_status st;

    if (g_UacpiMcfg.Initialized)
        return g_UacpiMcfg.Present ? UACPI_STATUS_OK : UACPI_STATUS_NOT_FOUND;

    g_UacpiMcfg.Initialized = TRUE;

    st = uacpi_table_find_by_signature(ACPI_MCFG_SIGNATURE, &tbl);
    if (uacpi_unlikely_error(st))
        return st;

    if (!tbl.ptr)
    {
        uacpi_table_unref(&tbl);
        return UACPI_STATUS_NOT_FOUND;
    }

    const struct acpi_mcfg *mcfg = (const struct acpi_mcfg *)tbl.ptr;
    if (mcfg->hdr.length < sizeof(*mcfg))
    {
        uacpi_table_unref(&tbl);
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    ULONG bytes = (ULONG)mcfg->hdr.length - (ULONG)sizeof(*mcfg);
    ULONG count = bytes / sizeof(struct acpi_mcfg_allocation);
    if (count == 0)
    {
        uacpi_table_unref(&tbl);
        return UACPI_STATUS_NOT_FOUND;
    }

    g_UacpiMcfg.Entries = ExAllocatePoolWithTag(NonPagedPool, count * sizeof(*g_UacpiMcfg.Entries), 'gAcu');
    if (!g_UacpiMcfg.Entries)
    {
        uacpi_table_unref(&tbl);
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    RtlCopyMemory(g_UacpiMcfg.Entries, mcfg->entries, count * sizeof(*g_UacpiMcfg.Entries));
    g_UacpiMcfg.EntryCount = count;
    g_UacpiMcfg.Present = TRUE;

    uacpi_table_unref(&tbl);

    DPRINT("uACPI: MCFG cached (%lu entries)\n", count);
    return UACPI_STATUS_OK;
}

static
BOOLEAN
UacpiFindMcfgAllocation(
    _In_ uacpi_u16 Segment,
    _In_ uacpi_u8 Bus,
    _Out_ const struct acpi_mcfg_allocation **OutAlloc)
{
    if (!OutAlloc)
        return FALSE;

    if (!g_UacpiMcfg.Initialized)
        (void)UacpiInitMcfgCache();

    *OutAlloc = NULL;

    if (!g_UacpiMcfg.Present || !g_UacpiMcfg.Entries || g_UacpiMcfg.EntryCount == 0)
        return FALSE;

    for (ULONG i = 0; i < g_UacpiMcfg.EntryCount; i++)
    {
        const struct acpi_mcfg_allocation *a = &g_UacpiMcfg.Entries[i];
        if (a->segment != Segment)
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
UacpiTryEcamRead(
    _In_ const uacpi_pci_address *Address,
    _In_ uacpi_size Offset,
    _Out_writes_bytes_(Width) VOID *OutValue,
    _In_ SIZE_T Width)
{
    const struct acpi_mcfg_allocation *a;

    if (!Address || !OutValue)
        return FALSE;

    if (Width != 1 && Width != 2 && Width != 4)
        return FALSE;

    /* Config space is 4KB per function. */
    if (Offset + Width > 0x1000)
        return FALSE;

    if (!UacpiFindMcfgAllocation((uacpi_u16)Address->segment, (uacpi_u8)Address->bus, &a))
        return FALSE;

    {
        LONG n = InterlockedIncrement(&g_UacpiEcamUseCount);
        if (n <= 32)
        {
            DPRINT("uACPI: ECAM read seg=%u bus=%u dev=%u fn=%u off=0x%Ix w=%Iu (MCFG base=0x%I64x)\n",
                   (ULONG)Address->segment, (ULONG)Address->bus, (ULONG)Address->device, (ULONG)Address->function,
                   (ULONG_PTR)Offset, Width, (ULONGLONG)a->address);
        }
    }

    uacpi_u64 busOff = (uacpi_u64)((uacpi_u32)Address->bus - (uacpi_u32)a->start_bus) << 20;
    uacpi_u64 devOff = (uacpi_u64)Address->device << 15;
    uacpi_u64 funOff = (uacpi_u64)Address->function << 12;
    uacpi_u64 phys = a->address + busOff + devOff + funOff + (uacpi_u64)Offset;

    volatile uacpi_u8 *p = (volatile uacpi_u8 *)uacpi_kernel_map((uacpi_phys_addr)phys, (uacpi_size)Width);
    if (!p)
        return FALSE;

    if (Width == 1)
    {
        *(uacpi_u8 *)OutValue = READ_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)p);
    }
    else if (Width == 2)
    {
        uacpi_u8 b0 = READ_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 0));
        uacpi_u8 b1 = READ_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 1));
        *(uacpi_u16 *)OutValue = (uacpi_u16)(b0 | ((uacpi_u16)b1 << 8));
    }
    else
    {
        uacpi_u8 b0 = READ_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 0));
        uacpi_u8 b1 = READ_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 1));
        uacpi_u8 b2 = READ_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 2));
        uacpi_u8 b3 = READ_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 3));
        *(uacpi_u32 *)OutValue = (uacpi_u32)(b0 | ((uacpi_u32)b1 << 8) | ((uacpi_u32)b2 << 16) | ((uacpi_u32)b3 << 24));
    }

    uacpi_kernel_unmap((void *)(ULONG_PTR)p, (uacpi_size)Width);
    return TRUE;
}

static
BOOLEAN
UacpiTryEcamWrite(
    _In_ const uacpi_pci_address *Address,
    _In_ uacpi_size Offset,
    _In_reads_bytes_(Width) const VOID *InValue,
    _In_ SIZE_T Width)
{
    const struct acpi_mcfg_allocation *a;

    if (!Address || !InValue)
        return FALSE;

    if (Width != 1 && Width != 2 && Width != 4)
        return FALSE;

    if (Offset + Width > 0x1000)
        return FALSE;

    if (!UacpiFindMcfgAllocation((uacpi_u16)Address->segment, (uacpi_u8)Address->bus, &a))
        return FALSE;

    {
        LONG n = InterlockedIncrement(&g_UacpiEcamUseCount);
        if (n <= 32)
        {
            DPRINT("uACPI: ECAM write seg=%u bus=%u dev=%u fn=%u off=0x%Ix w=%Iu (MCFG base=0x%I64x)\n",
                   (ULONG)Address->segment, (ULONG)Address->bus, (ULONG)Address->device, (ULONG)Address->function,
                   (ULONG_PTR)Offset, Width, (ULONGLONG)a->address);
        }
    }

    uacpi_u64 busOff = (uacpi_u64)((uacpi_u32)Address->bus - (uacpi_u32)a->start_bus) << 20;
    uacpi_u64 devOff = (uacpi_u64)Address->device << 15;
    uacpi_u64 funOff = (uacpi_u64)Address->function << 12;
    uacpi_u64 phys = a->address + busOff + devOff + funOff + (uacpi_u64)Offset;

    volatile uacpi_u8 *p = (volatile uacpi_u8 *)uacpi_kernel_map((uacpi_phys_addr)phys, (uacpi_size)Width);
    if (!p)
        return FALSE;

    if (Width == 1)
    {
        WRITE_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)p, *(const uacpi_u8 *)InValue);
    }
    else if (Width == 2)
    {
        uacpi_u16 v = *(const uacpi_u16 *)InValue;
        WRITE_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 0), (uacpi_u8)(v & 0xFF));
        WRITE_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 1), (uacpi_u8)((v >> 8) & 0xFF));
    }
    else
    {
        uacpi_u32 v = *(const uacpi_u32 *)InValue;
        WRITE_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 0), (uacpi_u8)(v & 0xFF));
        WRITE_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 1), (uacpi_u8)((v >> 8) & 0xFF));
        WRITE_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 2), (uacpi_u8)((v >> 16) & 0xFF));
        WRITE_REGISTER_UCHAR((PUCHAR)(ULONG_PTR)(p + 3), (uacpi_u8)((v >> 24) & 0xFF));
    }

    uacpi_kernel_unmap((void *)(ULONG_PTR)p, (uacpi_size)Width);
    return TRUE;
}

#ifndef UACPI_FORMATTED_LOGGING
void uacpi_kernel_log(uacpi_log_level Level, const uacpi_char* Char)
{
    UNREFERENCED_PARAMETER(Level);
    if (!Char)
        return;
    DPRINT("uACPI: %s", Char);
}
#else
UACPI_PRINTF_DECL(2, 3)
void uacpi_kernel_log(uacpi_log_level Level, const uacpi_char* Char, ...)
{
    va_list args;
    UNREFERENCED_PARAMETER(Level);
    if (!Char)
        return;
    va_start(args, Char);
    vDbgPrintEx(-1, DPFLTR_ERROR_LEVEL, Char, args);
    va_end(args);
}
void uacpi_kernel_vlog(uacpi_log_level Level, const uacpi_char* Char, uacpi_va_list list)
{
    UNREFERENCED_PARAMETER(Level);
    if (!Char)
        return;
    vDbgPrintEx(-1, DPFLTR_ERROR_LEVEL, Char, list);
}
#endif

typedef struct _UACPI_EVENT
{
    volatile LONG Counter;
    KSPIN_LOCK Lock;
    KEVENT Event;
} UACPI_EVENT, *PUACPI_EVENT;

typedef struct _UACPI_MUTEX
{
    KSEMAPHORE Sem;
} UACPI_MUTEX, *PUACPI_MUTEX;

typedef struct _UACPI_PCI_HANDLE
{
    uacpi_pci_address Address;
    PCI_SLOT_NUMBER Slot;
} UACPI_PCI_HANDLE, *PUACPI_PCI_HANDLE;

typedef struct _UACPI_IRQ_HANDLE
{
    PKINTERRUPT Interrupt;
    uacpi_interrupt_handler Handler;
    uacpi_handle Context;
    uacpi_u32 Irq;
} UACPI_IRQ_HANDLE, *PUACPI_IRQ_HANDLE;

typedef struct _UACPI_WORK_ITEM
{
    WORK_QUEUE_ITEM Item;
    uacpi_work_handler Handler;
    uacpi_handle Context;
    uacpi_work_type Type;
} UACPI_WORK_ITEM, *PUACPI_WORK_ITEM;

static volatile LONG g_UacpiOutstandingWorkItems = 0;
static KEVENT g_UacpiOutstandingWorkEvent;
static BOOLEAN g_UacpiWorkInit = FALSE;

static
BOOLEAN
NTAPI
UacpiIsrStub(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID ServiceContext)
{
    PUACPI_IRQ_HANDLE H = (PUACPI_IRQ_HANDLE)ServiceContext;
    UNREFERENCED_PARAMETER(Interrupt);
    if (!H || !H->Handler)
        return FALSE;
    return (H->Handler(H->Context) == UACPI_INTERRUPT_HANDLED);
}

static
VOID
NTAPI
UacpiWorkItemRoutine(_In_ PVOID Parameter)
{
    PUACPI_WORK_ITEM W = (PUACPI_WORK_ITEM)Parameter;
    if (W && W->Handler)
        W->Handler(W->Context);

    if (InterlockedDecrement(&g_UacpiOutstandingWorkItems) == 0)
        KeSetEvent(&g_UacpiOutstandingWorkEvent, IO_NO_INCREMENT, FALSE);

    ExFreePoolWithTag(W, 'wAcu');
}

static
VOID
UacpiEnsureWorkInit(VOID)
{
    if (!g_UacpiWorkInit)
    {
        KeInitializeEvent(&g_UacpiOutstandingWorkEvent, NotificationEvent, TRUE);
        g_UacpiWorkInit = TRUE;
    }
}

static
__forceinline
ULONG_PTR
UacpiAlignDown(ULONG_PTR Value, ULONG_PTR Align)
{
    return Value & ~(Align - 1);
}

static
__forceinline
ULONG_PTR
UacpiAlignUp(ULONG_PTR Value, ULONG_PTR Align)
{
    return (Value + (Align - 1)) & ~(Align - 1);
}

static
BOOLEAN
UacpiChecksum8(_In_reads_bytes_(Length) const UCHAR *Buffer, _In_ ULONG Length)
{
    UCHAR Sum = 0;
    for (ULONG i = 0; i < Length; i++)
        Sum = (UCHAR)(Sum + Buffer[i]);
    return (Sum == 0);
}

static
UCHAR
UacpiComputeChecksumByte(_In_reads_bytes_(Length) const UCHAR *Buffer, _In_ ULONG Length)
{
    UCHAR Sum = 0;
    for (ULONG i = 0; i < Length; i++)
        Sum = (UCHAR)(Sum + Buffer[i]);
    return (UCHAR)(0 - Sum);
}

#include <pshpack1.h>
typedef struct _UACPI_RSDP_10
{
    CHAR Signature[8];
    UCHAR Checksum;
    CHAR OemId[6];
    UCHAR Revision;
    ULONG RsdtAddress;
} UACPI_RSDP_10, *PUACPI_RSDP_10;

typedef struct _UACPI_RSDP_20
{
    UACPI_RSDP_10 FirstPart;
    ULONG Length;
    ULONGLONG XsdtAddress;
    UCHAR ExtendedChecksum;
    UCHAR Reserved[3];
} UACPI_RSDP_20, *PUACPI_RSDP_20;
#include <poppack.h>

static
BOOLEAN
UacpiLooksLikeRsdp(_In_reads_bytes_(sizeof(UACPI_RSDP_20)) const VOID *Ptr)
{
    const UACPI_RSDP_10 *Rsdp10 = (const UACPI_RSDP_10 *)Ptr;
    if (RtlCompareMemory(Rsdp10->Signature, "RSD PTR ", 8) != 8)
        return FALSE;

    if (!UacpiChecksum8((const UCHAR *)Ptr, sizeof(UACPI_RSDP_10)))
        return FALSE;

    if (Rsdp10->Revision >= 2)
    {
        const UACPI_RSDP_20 *Rsdp20 = (const UACPI_RSDP_20 *)Ptr;
        if (Rsdp20->Length < sizeof(UACPI_RSDP_20))
            return FALSE;
        if (!UacpiChecksum8((const UCHAR *)Ptr, Rsdp20->Length))
            return FALSE;
    }

    return TRUE;
}

typedef struct _UACPI_ACPI_BIOS_MULTI_NODE
{
    PHYSICAL_ADDRESS RsdtAddress;
    ULONGLONG Count;
    UCHAR Payload[1];
} UACPI_ACPI_BIOS_MULTI_NODE, *PUACPI_ACPI_BIOS_MULTI_NODE;

static
NTSTATUS
UacpiQueryAcpiRootFromRegistry(_Out_ PHYSICAL_ADDRESS *OutRootTable)
{
    static const WCHAR RootPath[] = L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultiFunctionAdapter";
    static const WCHAR IdentifierName[] = L"Identifier";
    static const WCHAR ConfigDataName[] = L"Configuration Data";
    static const WCHAR AcpiBiosId[] = L"ACPI BIOS";

    HANDLE KeyHandle = NULL;
    HANDLE SubKeyHandle = NULL;
    PKEY_FULL_INFORMATION FullInfo = NULL;
    PKEY_BASIC_INFORMATION KeyInfo = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = NULL;
    NTSTATUS Status;
    ULONG Bytes;

    if (!OutRootTable)
        return STATUS_INVALID_PARAMETER;

    OutRootTable->QuadPart = 0;

    UNICODE_STRING RootUs;
    RtlInitUnicodeString(&RootUs, RootPath);
    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes, &RootUs, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        return Status;

    Bytes = 0;
    Status = ZwQueryKey(KeyHandle, KeyFullInformation, NULL, 0, &Bytes);
    if (Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW)
        goto Exit;

    FullInfo = ExAllocatePoolWithTag(PagedPool, Bytes, 'rAcu');
    if (!FullInfo)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Status = ZwQueryKey(KeyHandle, KeyFullInformation, FullInfo, Bytes, &Bytes);
    if (!NT_SUCCESS(Status))
        goto Exit;

    /* Allocate enough to hold the longest subkey name (+ terminator) */
    Bytes = FullInfo->MaxNameLen + sizeof(WCHAR);
    KeyInfo = ExAllocatePoolWithTag(PagedPool, Bytes, 'rAcu');
    if (!KeyInfo)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    /* Buffer for Identifier (REG_SZ) */
    ValueInfo = ExAllocatePoolWithTag(PagedPool,
                                      sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(AcpiBiosId),
                                      'rAcu');
    if (!ValueInfo)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    for (ULONG i = 0;; i++)
    {
        ULONG NameBytes;
        Status = ZwEnumerateKey(KeyHandle, i, KeyBasicInformation, KeyInfo, Bytes, &NameBytes);
        if (Status == STATUS_NO_MORE_ENTRIES)
            break;
        if (!NT_SUCCESS(Status))
            goto Exit;

        /* Null-terminate the key name (kernel doesn't do it) */
        KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;

        UNICODE_STRING SubName;
        SubName.Buffer = KeyInfo->Name;
        SubName.Length = (USHORT)KeyInfo->NameLength;
        SubName.MaximumLength = (USHORT)(KeyInfo->NameLength + sizeof(WCHAR));

        InitializeObjectAttributes(&ObjectAttributes, &SubName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, KeyHandle, NULL);
        Status = ZwOpenKey(&SubKeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
            continue;

        UNICODE_STRING ValueUs;
        RtlInitUnicodeString(&ValueUs, IdentifierName);
        ULONG ValueBytes = 0;
        Status = ZwQueryValueKey(SubKeyHandle,
                                 &ValueUs,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(AcpiBiosId),
                                 &ValueBytes);
        if (NT_SUCCESS(Status) && ValueInfo->Type == REG_SZ)
        {
            if (wcsncmp((PWCHAR)ValueInfo->Data, AcpiBiosId, (sizeof(AcpiBiosId) / sizeof(WCHAR)) - 1) == 0)
            {
                /* Found the ACPI BIOS node; query Configuration Data */
                RtlInitUnicodeString(&ValueUs, ConfigDataName);

                /* Query size first */
                ZwClose(SubKeyHandle);
                SubKeyHandle = NULL;

                InitializeObjectAttributes(&ObjectAttributes, &SubName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, KeyHandle, NULL);
                Status = ZwOpenKey(&SubKeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
                if (!NT_SUCCESS(Status))
                    goto Exit;

                ValueBytes = 0;
                Status = ZwQueryValueKey(SubKeyHandle,
                                         &ValueUs,
                                         KeyValuePartialInformation,
                                         NULL,
                                         0,
                                         &ValueBytes);
                if (Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW)
                    goto Exit;

                PKEY_VALUE_PARTIAL_INFORMATION ConfigInfo = ExAllocatePoolWithTag(PagedPool, ValueBytes, 'rAcu');
                if (!ConfigInfo)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }

                Status = ZwQueryValueKey(SubKeyHandle,
                                         &ValueUs,
                                         KeyValuePartialInformation,
                                         ConfigInfo,
                                         ValueBytes,
                                         &ValueBytes);
                if (!NT_SUCCESS(Status))
                {
                    ExFreePoolWithTag(ConfigInfo, 'rAcu');
                    goto Exit;
                }

                if (ConfigInfo->Type != REG_FULL_RESOURCE_DESCRIPTOR || ConfigInfo->DataLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR))
                {
                    ExFreePoolWithTag(ConfigInfo, 'rAcu');
                    Status = STATUS_OBJECT_TYPE_MISMATCH;
                    goto Exit;
                }

                PCM_FULL_RESOURCE_DESCRIPTOR Full = (PCM_FULL_RESOURCE_DESCRIPTOR)ConfigInfo->Data;
                if (Full->PartialResourceList.Count < 1)
                {
                    ExFreePoolWithTag(ConfigInfo, 'rAcu');
                    Status = STATUS_DATA_ERROR;
                    goto Exit;
                }

                /* Device-specific data starts right after the CM_FULL_RESOURCE_DESCRIPTOR (which contains 1 descriptor). */
                PUACPI_ACPI_BIOS_MULTI_NODE Node = (PUACPI_ACPI_BIOS_MULTI_NODE)(Full + 1);
                *OutRootTable = Node->RsdtAddress;
                ExFreePoolWithTag(ConfigInfo, 'rAcu');
                Status = STATUS_SUCCESS;
                goto Exit;
            }
        }

        ZwClose(SubKeyHandle);
        SubKeyHandle = NULL;
    }

    Status = STATUS_NOT_FOUND;

Exit:
    if (SubKeyHandle) ZwClose(SubKeyHandle);
    if (ValueInfo) ExFreePoolWithTag(ValueInfo, 'rAcu');
    if (KeyInfo) ExFreePoolWithTag(KeyInfo, 'rAcu');
    if (FullInfo) ExFreePoolWithTag(FullInfo, 'rAcu');
    if (KeyHandle) ZwClose(KeyHandle);
    return Status;
}

static PVOID g_UacpiSyntheticRsdp;
static uacpi_phys_addr g_UacpiSyntheticRsdpPhys;

static
uacpi_status
UacpiGetOrBuildSyntheticRsdpFromRoot(_In_ PHYSICAL_ADDRESS RootTable, _Out_ uacpi_phys_addr *OutRsdpPhys)
{
    if (!OutRsdpPhys)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (g_UacpiSyntheticRsdp)
    {
        *OutRsdpPhys = g_UacpiSyntheticRsdpPhys;
        return UACPI_STATUS_OK;
    }

    /* Determine whether RootTable points to an XSDT or RSDT */
    UCHAR *Hdr = (UCHAR *)uacpi_kernel_map((uacpi_phys_addr)RootTable.QuadPart, 4);
    if (!Hdr)
        return UACPI_STATUS_MAPPING_FAILED;

    BOOLEAN IsXsdt = (memcmp(Hdr, "XSDT", 4) == 0);
    BOOLEAN IsRsdt = (memcmp(Hdr, "RSDT", 4) == 0);
    uacpi_kernel_unmap(Hdr, 4);

    if (!IsXsdt && !IsRsdt)
        return UACPI_STATUS_INVALID_ARGUMENT;

    PHYSICAL_ADDRESS Low;
    PHYSICAL_ADDRESS High;
    PHYSICAL_ADDRESS Boundary;
    Low.QuadPart = 0;
    High.QuadPart = ~0ULL;
    Boundary.QuadPart = 0;

    /* Allocate a page of physically contiguous memory for a synthetic RSDP */
    PVOID Buf = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, Low, High, Boundary, MmCached);
    if (!Buf)
        return UACPI_STATUS_OUT_OF_MEMORY;

    RtlZeroMemory(Buf, PAGE_SIZE);

    UACPI_RSDP_20 *Rsdp = (UACPI_RSDP_20 *)Buf;
    RtlCopyMemory(Rsdp->FirstPart.Signature, "RSD PTR ", 8);
    RtlCopyMemory(Rsdp->FirstPart.OemId, "ReactO", 6);
    Rsdp->FirstPart.Revision = 2;

    if (IsXsdt)
    {
        Rsdp->XsdtAddress = (ULONGLONG)RootTable.QuadPart;
        Rsdp->FirstPart.RsdtAddress = 0;
    }
    else
    {
        Rsdp->XsdtAddress = 0;
        Rsdp->FirstPart.RsdtAddress = (ULONG)(RootTable.QuadPart & 0xFFFFFFFFULL);
    }

    Rsdp->Length = sizeof(UACPI_RSDP_20);

    /* Compute checksums */
    Rsdp->FirstPart.Checksum = 0;
    Rsdp->ExtendedChecksum = 0;
    Rsdp->FirstPart.Checksum = UacpiComputeChecksumByte((const UCHAR *)Rsdp, sizeof(UACPI_RSDP_10));
    Rsdp->ExtendedChecksum = UacpiComputeChecksumByte((const UCHAR *)Rsdp, Rsdp->Length);

    /* Validate it looks like a proper RSDP */
    if (!UacpiLooksLikeRsdp(Rsdp))
    {
        MmFreeContiguousMemory(Buf);
        return UACPI_STATUS_INTERNAL_ERROR;
    }

    PHYSICAL_ADDRESS Phys = MmGetPhysicalAddress(Buf);
    g_UacpiSyntheticRsdp = Buf;
    g_UacpiSyntheticRsdpPhys = (uacpi_phys_addr)Phys.QuadPart;
    *OutRsdpPhys = g_UacpiSyntheticRsdpPhys;
    return UACPI_STATUS_OK;
}

uacpi_u64
uacpi_kernel_get_nanoseconds_since_boot(void)
{
    /* KeQueryInterruptTime returns 100ns units since boot. */
    ULONGLONG T100ns = KeQueryInterruptTime();
    return (uacpi_u64)(T100ns * 100ULL);
}

void
uacpi_kernel_stall(uacpi_u8 usec)
{
	KeStallExecutionProcessor(usec);
}

void uacpi_kernel_sleep(uacpi_u64 msec)
{
    if (msec == 0)
        return;

    if (KeGetCurrentIrql() > APC_LEVEL)
    {
        /* Best-effort fallback; should not normally happen. */
        KeStallExecutionProcessor((ULONG)(msec * 1000ULL));
        return;
    }

    LARGE_INTEGER Interval;
    Interval.QuadPart = -(LONGLONG)(msec * 10000ULL);
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

uacpi_handle
uacpi_kernel_create_event(void)
{
    PUACPI_EVENT Ev = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Ev), 'vAcu');
    if (!Ev)
        return NULL;
    Ev->Counter = 0;
    KeInitializeSpinLock(&Ev->Lock);
    KeInitializeEvent(&Ev->Event, NotificationEvent, FALSE);
    return (uacpi_handle)Ev;
}

void
uacpi_kernel_free_event(uacpi_handle Handle)
{
    if (!Handle)
        return;
    ExFreePoolWithTag(Handle, 'vAcu');
}

uacpi_bool
uacpi_kernel_wait_for_event(uacpi_handle Handle, uacpi_u16 Timeout)
{
    PUACPI_EVENT Ev = (PUACPI_EVENT)Handle;
    LARGE_INTEGER Interval;
    PLARGE_INTEGER PInterval = NULL;
    KIRQL OldIrql;

    if (!Ev)
        return UACPI_FALSE;

    if (Timeout != 0xFFFF)
    {
        Interval.QuadPart = -(LONGLONG)((ULONGLONG)Timeout * 10000ULL);
        PInterval = &Interval;
    }

    for (;;)
    {
        KeAcquireSpinLock(&Ev->Lock, &OldIrql);
        if (Ev->Counter > 0)
        {
            Ev->Counter--;
            if (Ev->Counter == 0)
                KeClearEvent(&Ev->Event);
            KeReleaseSpinLock(&Ev->Lock, OldIrql);
            return UACPI_TRUE;
        }
        KeReleaseSpinLock(&Ev->Lock, OldIrql);

        NTSTATUS Status = KeWaitForSingleObject(&Ev->Event, Executive, KernelMode, FALSE, PInterval);
        if (Status == STATUS_TIMEOUT)
            return UACPI_FALSE;

        /* We were signaled; loop back and try decrementing. */
    }
}

void
uacpi_kernel_signal_event(uacpi_handle Handle)
{
    PUACPI_EVENT Ev = (PUACPI_EVENT)Handle;
    KIRQL OldIrql;
    if (!Ev)
        return;
    KeAcquireSpinLock(&Ev->Lock, &OldIrql);
    Ev->Counter++;
    KeSetEvent(&Ev->Event, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLock(&Ev->Lock, OldIrql);
}

void 
uacpi_kernel_reset_event(uacpi_handle Handle)
{
    PUACPI_EVENT Ev = (PUACPI_EVENT)Handle;
    KIRQL OldIrql;
    if (!Ev)
        return;
    KeAcquireSpinLock(&Ev->Lock, &OldIrql);
    Ev->Counter = 0;
    KeClearEvent(&Ev->Event);
    KeReleaseSpinLock(&Ev->Lock, OldIrql);
}

uacpi_handle
uacpi_kernel_create_spinlock(void)
{
    PKSPIN_LOCK SpinLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(KSPIN_LOCK), 'lAcu');
    if (!SpinLock)
        return NULL;
    KeInitializeSpinLock(SpinLock);
    return (uacpi_handle)SpinLock;
}

void
uacpi_kernel_free_spinlock(uacpi_handle Handle)
{
    if (!Handle)
        return;
    ExFreePoolWithTag(Handle, 'lAcu');
}

uacpi_cpu_flags
uacpi_kernel_lock_spinlock(uacpi_handle Handle)
{
    KIRQL OldIrql;
    if (!Handle)
        return 0;

    if ((OldIrql = KeGetCurrentIrql()) >= DISPATCH_LEVEL)
    {
        KeAcquireSpinLockAtDpcLevel((PKSPIN_LOCK)Handle);
    }
    else
    {
        KeAcquireSpinLock((PKSPIN_LOCK)Handle, &OldIrql);
    }

    return (uacpi_cpu_flags)OldIrql;
}

void
uacpi_kernel_unlock_spinlock(uacpi_handle Handle, uacpi_cpu_flags Flags)
{
    KIRQL OldIrql = (KIRQL)Flags;
    if (!Handle)
        return;

    if (OldIrql >= DISPATCH_LEVEL)
    {
        KeReleaseSpinLockFromDpcLevel((PKSPIN_LOCK)Handle);
    }
    else
    {
        KeReleaseSpinLock((PKSPIN_LOCK)Handle, OldIrql);
    }
}

void*
uacpi_kernel_alloc(uacpi_size size)
{
    if (size == 0)
        size = 1;
    return ExAllocatePoolWithTag(NonPagedPool, (SIZE_T)size, 'mAcu');
}

void *
uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
{
    SIZE_T Total;
    if (count == 0 || size == 0)
        Total = 1;
    else
        Total = (SIZE_T)count * (SIZE_T)size;
    PVOID Ptr = ExAllocatePoolWithTag(NonPagedPool, Total, 'mAcu');
    if (Ptr)
        RtlZeroMemory(Ptr, Total);
    return Ptr;
}

#ifndef UACPI_SIZED_FREES
void
uacpi_kernel_free(void *mem)
{
    if (!mem)
        return;
    ExFreePoolWithTag(mem, 'mAcu');
}
#else
void
uacpi_kernel_free(void *mem, uacpi_size size_hint)
{
    UNREFERENCED_PARAMETER(size_hint);
    if (!mem)
        return;
    ExFreePoolWithTag(mem, 'mAcu');
}
#endif

uacpi_handle
uacpi_kernel_create_mutex(void)
{
    PUACPI_MUTEX Mtx = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Mtx), 'tAcu');
    if (!Mtx)
        return NULL;
    KeInitializeSemaphore(&Mtx->Sem, 1, 1);
    return (uacpi_handle)Mtx;
}

void
uacpi_kernel_free_mutex(uacpi_handle handle)
{
    if (!handle)
        return;
    ExFreePoolWithTag(handle, 'tAcu');
}

uacpi_status
uacpi_kernel_acquire_mutex(uacpi_handle Handle, uacpi_u16 Timeout)
{
    PUACPI_MUTEX Mtx = (PUACPI_MUTEX)Handle;
    LARGE_INTEGER Interval;
    PLARGE_INTEGER PInterval = NULL;
    NTSTATUS Status;

    if (!Mtx)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (Timeout == 0)
    {
        Interval.QuadPart = 0;
        PInterval = &Interval;
    }
    else if (Timeout != 0xFFFF)
    {
        Interval.QuadPart = -(LONGLONG)((ULONGLONG)Timeout * 10000ULL);
        PInterval = &Interval;
    }

    Status = KeWaitForSingleObject(&Mtx->Sem, Executive, KernelMode, FALSE, PInterval);
    if (Status == STATUS_TIMEOUT)
        return UACPI_STATUS_TIMEOUT;
    if (!NT_SUCCESS(Status))
        return UACPI_STATUS_INTERNAL_ERROR;

    return UACPI_STATUS_OK;
}

void
uacpi_kernel_release_mutex(uacpi_handle Handle)
{
    PUACPI_MUTEX Mtx = (PUACPI_MUTEX)Handle;
    if (!Mtx)
        return;
    KeReleaseSemaphore(&Mtx->Sem, IO_NO_INCREMENT, 1, FALSE);
}

uacpi_status
uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle)
{
    UNREFERENCED_PARAMETER(len);
    if (!out_handle)
        return UACPI_STATUS_INVALID_ARGUMENT;
    *out_handle = (uacpi_handle)(ULONG_PTR)base;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle)
{
	UNREFERENCED_PARAMETER(handle);
}

uacpi_status
uacpi_kernel_handle_firmware_request(uacpi_firmware_request* Req)
{
    if (!Req)
        return UACPI_STATUS_INVALID_ARGUMENT;

    switch (Req->type)
    {
        case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
            DPRINT1("uACPI breakpoint request (ctx=%p)\n", Req->breakpoint.ctx);
            return UACPI_STATUS_OK;
        case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
            DPRINT1("uACPI fatal request type=%u code=0x%08lx arg=0x%I64x\n",
                    Req->fatal.type, Req->fatal.code, Req->fatal.arg);
            return UACPI_STATUS_INTERNAL_ERROR;
        default:
            return UACPI_STATUS_UNIMPLEMENTED;
    }
}

uacpi_thread_id
uacpi_kernel_get_thread_id(void)
{
	/* Thread ID must be non-zero */
	return (uacpi_thread_id)((ULONG_PTR)PsGetCurrentThreadId() + 1);
}

void *
uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
    PHYSICAL_ADDRESS Phys;
    ULONG_PTR Addr = (ULONG_PTR)addr;
    ULONG_PTR Aligned = UacpiAlignDown(Addr, PAGE_SIZE);
    ULONG_PTR Offset = Addr - Aligned;
    ULONG_PTR TotalLen = UacpiAlignUp((ULONG_PTR)len + Offset, PAGE_SIZE);
    PVOID Base;

    Phys.QuadPart = (LONGLONG)Aligned;
    Base = MmMapIoSpace(Phys, (SIZE_T)TotalLen, MmNonCached);
    if (!Base)
        return NULL;

    return (PVOID)((PUCHAR)Base + Offset);
}

void
uacpi_kernel_unmap(void *addr, uacpi_size len)
{
    ULONG_PTR Addr = (ULONG_PTR)addr;
    ULONG_PTR Aligned = UacpiAlignDown(Addr, PAGE_SIZE);
    ULONG_PTR Offset = Addr - Aligned;
    ULONG_PTR TotalLen = UacpiAlignUp((ULONG_PTR)len + Offset, PAGE_SIZE);
    MmUnmapIoSpace((PVOID)Aligned, (SIZE_T)TotalLen);
}

uacpi_status
uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rdsp_address)
{
    if (!out_rdsp_address)
        return UACPI_STATUS_INVALID_ARGUMENT;

    *out_rdsp_address = 0;

    /* Preferred path: read loader-provided ACPI root table from registry and synthesize an RSDP.
     * This works for UEFI boots where the real RSDP may not be discoverable in EBDA/BIOS memory.
     */
    PHYSICAL_ADDRESS RootTable;
    NTSTATUS RegSt = UacpiQueryAcpiRootFromRegistry(&RootTable);
    if (NT_SUCCESS(RegSt) && RootTable.QuadPart)
    {
        uacpi_status St = UacpiGetOrBuildSyntheticRsdpFromRoot(RootTable, out_rdsp_address);
        if (uacpi_likely_success(St))
            return St;
    }

    /* Read EBDA segment pointer from BDA at 0x40E */
    UCHAR *Bda = (UCHAR *)uacpi_kernel_map(0x400, 0x200);
    USHORT EbdaSeg = 0;
    if (Bda)
    {
        EbdaSeg = *(USHORT *)(Bda + 0x0E);
        uacpi_kernel_unmap(Bda, 0x200);
    }

    ULONG_PTR EbdaPhys = ((ULONG_PTR)EbdaSeg) << 4;
    if (EbdaPhys)
    {
        UCHAR *Ebda = (UCHAR *)uacpi_kernel_map(EbdaPhys, 1024);
        if (Ebda)
        {
            for (ULONG Off = 0; Off < 1024; Off += 16)
            {
                if (UacpiLooksLikeRsdp(Ebda + Off))
                {
                    *out_rdsp_address = (uacpi_phys_addr)(EbdaPhys + Off);
                    uacpi_kernel_unmap(Ebda, 1024);
                    return UACPI_STATUS_OK;
                }
            }
            uacpi_kernel_unmap(Ebda, 1024);
        }
    }

    /* Search BIOS area 0xE0000..0xFFFFF */
    const ULONG_PTR BiosStart = 0xE0000;
    const ULONG BiosLen = 0x20000;
    UCHAR *Bios = (UCHAR *)uacpi_kernel_map(BiosStart, BiosLen);
    if (!Bios)
        return UACPI_STATUS_MAPPING_FAILED;

    for (ULONG Off = 0; Off < BiosLen; Off += 16)
    {
        if (UacpiLooksLikeRsdp(Bios + Off))
        {
            *out_rdsp_address = (uacpi_phys_addr)(BiosStart + Off);
            uacpi_kernel_unmap(Bios, BiosLen);
            return UACPI_STATUS_OK;
        }
    }

    uacpi_kernel_unmap(Bios, BiosLen);
    return UACPI_STATUS_NOT_FOUND;
}


uacpi_status
uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle)
{
    ULONG Vector;
    KIRQL Irql;
    KAFFINITY Affinity;
    NTSTATUS Status;
    PUACPI_IRQ_HANDLE IrqHandle;

    if (!handler || !out_irq_handle)
        return UACPI_STATUS_INVALID_ARGUMENT;

    IrqHandle = ExAllocatePoolWithTag(NonPagedPool, sizeof(*IrqHandle), 'qAcu');
    if (!IrqHandle)
        return UACPI_STATUS_OUT_OF_MEMORY;

    RtlZeroMemory(IrqHandle, sizeof(*IrqHandle));
    IrqHandle->Handler = handler;
    IrqHandle->Context = ctx;
    IrqHandle->Irq = irq;

    Vector = HalGetInterruptVector(Internal, 0, irq, irq, &Irql, &Affinity);

    Status = IoConnectInterrupt(
        &IrqHandle->Interrupt,
        UacpiIsrStub,
        (PVOID)IrqHandle,
        NULL,
        Vector,
        Irql,
        Irql,
        LevelSensitive,
        TRUE,
        Affinity,
        FALSE);

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(IrqHandle, 'qAcu');
        return UACPI_STATUS_INTERNAL_ERROR;
    }

    *out_irq_handle = (uacpi_handle)IrqHandle;
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler handler, uacpi_handle irq_handle)
{
    UNREFERENCED_PARAMETER(handler);
    PUACPI_IRQ_HANDLE H = (PUACPI_IRQ_HANDLE)irq_handle;
    if (!H)
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (H->Interrupt)
        IoDisconnectInterrupt(H->Interrupt);
    ExFreePoolWithTag(H, 'qAcu');
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler Handler, uacpi_handle ctx)
{
    PUACPI_WORK_ITEM Work;

    if (!Handler)
        return UACPI_STATUS_INVALID_ARGUMENT;

    UacpiEnsureWorkInit();

    Work = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Work), 'wAcu');
    if (!Work)
        return UACPI_STATUS_OUT_OF_MEMORY;

    Work->Handler = Handler;
    Work->Context = ctx;
    Work->Type = type;

    KeClearEvent(&g_UacpiOutstandingWorkEvent);
    InterlockedIncrement(&g_UacpiOutstandingWorkItems);

    ExInitializeWorkItem(&Work->Item, UacpiWorkItemRoutine, Work);

    ExQueueWorkItem(&Work->Item, DelayedWorkQueue);
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_wait_for_work_completion(void)
{
    UacpiEnsureWorkInit();

    while (g_UacpiOutstandingWorkItems != 0)
    {
        KeWaitForSingleObject(&g_UacpiOutstandingWorkEvent, Executive, KernelMode, FALSE, NULL);
    }

    return UACPI_STATUS_OK;
}

uacpi_status 
uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
)
{
    PUACPI_PCI_HANDLE Handle;

    if (!out_handle)
        return UACPI_STATUS_INVALID_ARGUMENT;

    /*
     * ReactOS currently only supports segment 0 via Hal*BusData accessors.
     * However, failing AML method execution is often worse than treating the
     * device as absent (PCI config reads returning all-ones).
     */

    Handle = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Handle), 'pAcu');
    if (!Handle)
        return UACPI_STATUS_OUT_OF_MEMORY;

    RtlZeroMemory(Handle, sizeof(*Handle));
    Handle->Address = address;
    Handle->Slot.u.AsULONG = 0;
    Handle->Slot.u.bits.DeviceNumber = address.device;
    Handle->Slot.u.bits.FunctionNumber = address.function;

    *out_handle = (uacpi_handle)Handle;
    return UACPI_STATUS_OK;
}

void
uacpi_kernel_pci_device_close(uacpi_handle Handle)
{
    if (!Handle)
        return;
    ExFreePoolWithTag(Handle, 'pAcu');
}

uacpi_status
uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset, uacpi_u8 *Value)
{
    PUACPI_PCI_HANDLE H = (PUACPI_PCI_HANDLE)device;
    ULONG ReadLength;
    if (!H || !Value)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (UacpiTryEcamRead(&H->Address, offset, Value, sizeof(*Value)))
        return UACPI_STATUS_OK;

    if (H->Address.segment != 0)
    {
        *Value = 0xFF;
        return UACPI_STATUS_OK;
    }

    ReadLength = HalGetBusDataByOffset(PCIConfiguration, H->Address.bus, H->Slot.u.AsULONG, Value, (ULONG)offset, sizeof(*Value));
    if (ReadLength != sizeof(*Value))
    {
        /*
         * PCI config cycles for non-existent devices/functions return all-ones.
         * Returning NOT_FOUND would abort AML methods that probe via PCI_Config
         * operation regions (e.g. _STA).
         */
        *Value = 0xFF;
    }
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset, uacpi_u16 *value)
{
    PUACPI_PCI_HANDLE H = (PUACPI_PCI_HANDLE)device;
    ULONG ReadLength;
    if (!H || !value)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (UacpiTryEcamRead(&H->Address, offset, value, sizeof(*value)))
        return UACPI_STATUS_OK;

    if (H->Address.segment != 0)
    {
        *value = 0xFFFF;
        return UACPI_STATUS_OK;
    }

    ReadLength = HalGetBusDataByOffset(PCIConfiguration, H->Address.bus, H->Slot.u.AsULONG, value, (ULONG)offset, sizeof(*value));
    if (ReadLength != sizeof(*value))
    {
        *value = 0xFFFF;
    }
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset, uacpi_u32 *value)
{
    PUACPI_PCI_HANDLE H = (PUACPI_PCI_HANDLE)device;
    ULONG ReadLength;
    if (!H || !value)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (UacpiTryEcamRead(&H->Address, offset, value, sizeof(*value)))
        return UACPI_STATUS_OK;

    if (H->Address.segment != 0)
    {
        *value = 0xFFFFFFFF;
        return UACPI_STATUS_OK;
    }

    ReadLength = HalGetBusDataByOffset(PCIConfiguration, H->Address.bus, H->Slot.u.AsULONG, value, (ULONG)offset, sizeof(*value));
    if (ReadLength != sizeof(*value))
    {
        *value = 0xFFFFFFFF;
    }
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset, uacpi_u8 value)
{
    PUACPI_PCI_HANDLE H = (PUACPI_PCI_HANDLE)device;
    ULONG Written;
    if (!H)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (UacpiTryEcamWrite(&H->Address, offset, &value, sizeof(value)))
        return UACPI_STATUS_OK;

    if (H->Address.segment != 0)
        return UACPI_STATUS_OK;

    Written = HalSetBusDataByOffset(PCIConfiguration, H->Address.bus, H->Slot.u.AsULONG, &value, (ULONG)offset, sizeof(value));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset, uacpi_u16 value)
{
    PUACPI_PCI_HANDLE H = (PUACPI_PCI_HANDLE)device;
    ULONG Written;
    if (!H)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (UacpiTryEcamWrite(&H->Address, offset, &value, sizeof(value)))
        return UACPI_STATUS_OK;

    if (H->Address.segment != 0)
        return UACPI_STATUS_OK;

    Written = HalSetBusDataByOffset(PCIConfiguration, H->Address.bus, H->Slot.u.AsULONG, &value, (ULONG)offset, sizeof(value));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset, uacpi_u32 value)
{
    PUACPI_PCI_HANDLE H = (PUACPI_PCI_HANDLE)device;
    ULONG Written;
    if (!H)
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (UacpiTryEcamWrite(&H->Address, offset, &value, sizeof(value)))
        return UACPI_STATUS_OK;

    if (H->Address.segment != 0)
        return UACPI_STATUS_OK;

    Written = HalSetBusDataByOffset(PCIConfiguration, H->Address.bus, H->Slot.u.AsULONG, &value, (ULONG)offset, sizeof(value));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value)
{
    if (!out_value)
        return UACPI_STATUS_INVALID_ARGUMENT;
    *out_value = READ_PORT_UCHAR((PUCHAR)((ULONG_PTR)handle + (ULONG_PTR)offset));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value)
{
    if (!out_value)
        return UACPI_STATUS_INVALID_ARGUMENT;
    *out_value = READ_PORT_USHORT((PUSHORT)((ULONG_PTR)handle + (ULONG_PTR)offset));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value)
{
    if (!out_value)
        return UACPI_STATUS_INVALID_ARGUMENT;
    *out_value = READ_PORT_ULONG((PULONG)((ULONG_PTR)handle + (ULONG_PTR)offset));
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value)
{
	WRITE_PORT_UCHAR((PUCHAR)((ULONG_PTR)handle + (ULONG_PTR)offset), in_value);
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value)
{
	WRITE_PORT_USHORT((PUSHORT)((ULONG_PTR)handle + (ULONG_PTR)offset), in_value);
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value)
{
	WRITE_PORT_ULONG((PULONG)((ULONG_PTR)handle + (ULONG_PTR)offset), in_value);
	return UACPI_STATUS_OK;
}
