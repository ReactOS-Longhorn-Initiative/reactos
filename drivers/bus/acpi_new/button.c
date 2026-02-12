#include "precomp.h"
#include "acpi_new.h"

#include <poclass.h>
#include <uacpi/event.h>

static KSPIN_LOCK g_ButtonLock;
static KEVENT g_ButtonEvent;
static volatile ULONG g_ButtonPendingMask = 0;
static volatile LONG g_ButtonInitOnce = 0;

static KEVENT g_LidEvent;
static volatile LONG g_LidInitOnce = 0;
static volatile LONG g_LidNotifySeq = 0;

static BOOLEAN AcpiNewPdoIsButton(_In_ const PACPI_NEW_PDO_EXTENSION PdoExt, _Out_opt_ PULONG OutMask)
{
    ULONG mask = 0;

    if (!PdoExt || !PdoExt->HardwareIds)
        return FALSE;

    if (wcsstr(PdoExt->HardwareIds, L"PNP0C0C") != NULL || wcsstr(PdoExt->HardwareIds, L"ACPI_FPB") != NULL)
        mask |= SYS_BUTTON_POWER;

    if (wcsstr(PdoExt->HardwareIds, L"PNP0C0E") != NULL || wcsstr(PdoExt->HardwareIds, L"ACPI_FSB") != NULL)
        mask |= SYS_BUTTON_SLEEP;

    if (wcsstr(PdoExt->HardwareIds, L"PNP0C0D") != NULL)
        mask |= SYS_BUTTON_LID;

    if (OutMask)
        *OutMask = mask;

    return (mask != 0);
}

static BOOLEAN AcpiNewPdoIsLid(_In_ const PACPI_NEW_PDO_EXTENSION PdoExt)
{
    ULONG mask = 0;
    return AcpiNewPdoIsButton(PdoExt, &mask) && (mask & SYS_BUTTON_LID);
}

static VOID AcpiNewButtonSignal(_In_ ULONG Mask)
{
    KIRQL oldIrql;

    if (!Mask)
        return;

    KeAcquireSpinLock(&g_ButtonLock, &oldIrql);
    g_ButtonPendingMask |= Mask;
    KeSetEvent(&g_ButtonEvent, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLock(&g_ButtonLock, oldIrql);
}

static uacpi_interrupt_ret AcpiNewFixedPowerButtonHandler(_In_ uacpi_handle user)
{
    UNREFERENCED_PARAMETER(user);
    AcpiNewButtonSignal(SYS_BUTTON_POWER);
    return UACPI_INTERRUPT_HANDLED;
}

static uacpi_interrupt_ret AcpiNewFixedSleepButtonHandler(_In_ uacpi_handle user)
{
    UNREFERENCED_PARAMETER(user);
    AcpiNewButtonSignal(SYS_BUTTON_SLEEP);
    return UACPI_INTERRUPT_HANDLED;
}

VOID AcpiNewButtonInit(VOID)
{
    uacpi_status st;

    if (InterlockedCompareExchange(&g_ButtonInitOnce, 1, 0) != 0)
        return;

    KeInitializeSpinLock(&g_ButtonLock);
    KeInitializeEvent(&g_ButtonEvent, NotificationEvent, FALSE);

    if (InterlockedCompareExchange(&g_LidInitOnce, 1, 0) == 0)
    {
        KeInitializeEvent(&g_LidEvent, NotificationEvent, FALSE);
        g_LidNotifySeq = 0;
    }

    st = uacpi_install_fixed_event_handler(UACPI_FIXED_EVENT_POWER_BUTTON, AcpiNewFixedPowerButtonHandler, UACPI_NULL);
    if (uacpi_unlikely_error(st))
        DPRINT("acpi_new: power fixed event handler not installed: %s\n", uacpi_status_to_string(st));

    st = uacpi_install_fixed_event_handler(UACPI_FIXED_EVENT_SLEEP_BUTTON, AcpiNewFixedSleepButtonHandler, UACPI_NULL);
    if (uacpi_unlikely_error(st))
        DPRINT("acpi_new: sleep fixed event handler not installed: %s\n", uacpi_status_to_string(st));
}

VOID AcpiNewButtonOnNotify(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ ULONG NotifyCode)
{
    ULONG mask;

    if (!PdoExt)
        return;

    if (NotifyCode != 0x80)
        return;

    if (!AcpiNewPdoIsButton(PdoExt, &mask))
        return;

    mask &= (SYS_BUTTON_POWER | SYS_BUTTON_SLEEP | SYS_BUTTON_LID);
    if (mask)
        AcpiNewButtonSignal(mask);

    if ((mask & SYS_BUTTON_LID) != 0)
    {
        InterlockedIncrement(&g_LidNotifySeq);
        KeSetEvent(&g_LidEvent, IO_NO_INCREMENT, FALSE);
    }
}

NTSTATUS AcpiNewButtonQueryCaps(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _Out_ PULONG OutCaps)
{
    ULONG mask;

    if (!OutCaps)
        return STATUS_INVALID_PARAMETER;

    *OutCaps = 0;

    if (!AcpiNewPdoIsButton(PdoExt, &mask))
        return STATUS_INVALID_PARAMETER;

    *OutCaps = mask;
    return STATUS_SUCCESS;
}

typedef struct _ACPI_NEW_BUTTON_WAIT_CTX
{
    PIRP Irp;
} ACPI_NEW_BUTTON_WAIT_CTX, *PACPI_NEW_BUTTON_WAIT_CTX;

static VOID NTAPI AcpiNewButtonWaitThread(_In_ PVOID Context)
{
    PACPI_NEW_BUTTON_WAIT_CTX ctx = (PACPI_NEW_BUTTON_WAIT_CTX)Context;
    PIRP Irp;
    ULONG outMask = 0;
    KIRQL oldIrql;

    if (!ctx)
        PsTerminateSystemThread(STATUS_INVALID_PARAMETER);

    Irp = ctx->Irp;
    ExFreePoolWithTag(ctx, 'bAcu');

    if (!Irp)
        PsTerminateSystemThread(STATUS_INVALID_PARAMETER);

    (void)KeWaitForSingleObject(&g_ButtonEvent, Executive, KernelMode, FALSE, NULL);

    KeAcquireSpinLock(&g_ButtonLock, &oldIrql);
    outMask = g_ButtonPendingMask;
    g_ButtonPendingMask = 0;
    KeClearEvent(&g_ButtonEvent);
    KeReleaseSpinLock(&g_ButtonLock, oldIrql);

    if (Irp->AssociatedIrp.SystemBuffer)
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &outMask, sizeof(outMask));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(outMask);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS AcpiNewButtonQueueWaitIrp(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ PIRP Irp)
{
    HANDLE threadHandle;
    PACPI_NEW_BUTTON_WAIT_CTX ctx;
    ULONG mask;

    if (!AcpiNewPdoIsButton(PdoExt, &mask))
        return STATUS_INVALID_PARAMETER;

    if (!Irp)
        return STATUS_INVALID_PARAMETER;

    ctx = (PACPI_NEW_BUTTON_WAIT_CTX)ExAllocatePoolWithTag(NonPagedPool, sizeof(*ctx), 'bAcu');
    if (!ctx)
        return STATUS_INSUFFICIENT_RESOURCES;

    ctx->Irp = Irp;

    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;
    IoMarkIrpPending(Irp);

    (void)PsCreateSystemThread(
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        AcpiNewButtonWaitThread,
        ctx
    );
    if (threadHandle)
        ZwClose(threadHandle);

    return STATUS_PENDING;
}

typedef struct _ACPI_NEW_LID_WAIT_CTX
{
    PIRP Irp;
    PDEVICE_OBJECT Pdo;
    LONG StartSeq;
} ACPI_NEW_LID_WAIT_CTX, *PACPI_NEW_LID_WAIT_CTX;

static VOID NTAPI AcpiNewLidWaitThread(_In_ PVOID Context)
{
    PACPI_NEW_LID_WAIT_CTX ctx = (PACPI_NEW_LID_WAIT_CTX)Context;
    PIRP Irp;
    PDEVICE_OBJECT pdo;
    LONG currentSeq;
    ULONG lidState = 0;

    if (!ctx)
        PsTerminateSystemThread(STATUS_INVALID_PARAMETER);

    Irp = ctx->Irp;
    pdo = ctx->Pdo;
    currentSeq = ctx->StartSeq;
    ExFreePoolWithTag(ctx, 'lAcu');

    if (!Irp || !pdo)
        PsTerminateSystemThread(STATUS_INVALID_PARAMETER);

    for (;;)
    {
        (void)KeWaitForSingleObject(&g_LidEvent, Executive, KernelMode, FALSE, NULL);
        KeClearEvent(&g_LidEvent);

        if (g_LidNotifySeq != currentSeq)
            break;
    }

    if (AcpiNewIsPdo(pdo))
    {
        PACPI_NEW_PDO_EXTENSION pdoExt = (PACPI_NEW_PDO_EXTENSION)pdo->DeviceExtension;
        if (pdoExt && pdoExt->Node && KeGetCurrentIrql() < DISPATCH_LEVEL)
        {
            uacpi_u64 lid = 0;
            uacpi_status st = uacpi_eval_simple_integer(pdoExt->Node, "_LID", &lid);
            if (!uacpi_unlikely_error(st))
                lidState = (lid ? 1u : 0u);
        }
    }

    if (Irp->AssociatedIrp.SystemBuffer)
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &lidState, sizeof(lidState));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(lidState);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    ObDereferenceObject(pdo);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS AcpiNewLidQueueWaitIrp(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ PIRP Irp)
{
    HANDLE threadHandle;
    PACPI_NEW_LID_WAIT_CTX ctx;

    if (!AcpiNewPdoIsLid(PdoExt))
        return STATUS_INVALID_PARAMETER;

    if (!Irp)
        return STATUS_INVALID_PARAMETER;

    if (InterlockedCompareExchange(&g_LidInitOnce, 1, 0) == 0)
    {
        KeInitializeEvent(&g_LidEvent, NotificationEvent, FALSE);
        g_LidNotifySeq = 0;
    }

    ctx = (PACPI_NEW_LID_WAIT_CTX)ExAllocatePoolWithTag(NonPagedPool, sizeof(*ctx), 'lAcu');
    if (!ctx)
        return STATUS_INSUFFICIENT_RESOURCES;

    ctx->Irp = Irp;
    ctx->Pdo = PdoExt->Common.Self;
    ctx->StartSeq = g_LidNotifySeq;
    ObReferenceObject(ctx->Pdo);

    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;
    IoMarkIrpPending(Irp);

    (void)PsCreateSystemThread(
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        AcpiNewLidWaitThread,
        ctx
    );
    if (threadHandle)
        ZwClose(threadHandle);

    return STATUS_PENDING;
}
