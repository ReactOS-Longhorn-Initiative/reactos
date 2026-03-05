#include "precomp.h"
#include "acpi_new.h"

#include <acpiioct.h>
#include <poclass.h>
#include <limits.h>

#define ACPI_OBJECT_NAME_LENGTH (4 + 1)
#define ACPI_MAX_PACKAGE_DEPTH  5

#define AcpiVerifyInBuffer(IoStack, Length) \
    ((IoStack)->Parameters.DeviceIoControl.InputBufferLength >= (Length))

#define AcpiVerifyOutBuffer(IoStack, Length) \
    ((IoStack)->Parameters.DeviceIoControl.OutputBufferLength >= (Length))

static
NTSTATUS
AcpiNewUacpiStatusToNtStatus(_In_ uacpi_status st)
{
    switch (st)
    {
    case UACPI_STATUS_OK:
        return STATUS_SUCCESS;
    case UACPI_STATUS_OUT_OF_MEMORY:
        return STATUS_INSUFFICIENT_RESOURCES;
    case UACPI_STATUS_INVALID_ARGUMENT:
        return STATUS_INVALID_PARAMETER;
    case UACPI_STATUS_NOT_FOUND:
        return STATUS_OBJECT_NAME_NOT_FOUND;
    case UACPI_STATUS_UNIMPLEMENTED:
        return STATUS_NOT_SUPPORTED;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

static
NTSTATUS
AcpiNewEvalGetElementSize(
    _In_ uacpi_object *Obj,
    _In_ ULONG Depth,
    _Out_opt_ PULONG Count,
    _Out_ PULONG Size)
{
    uacpi_object_type type;

    if (!Obj || !Size)
        return STATUS_INVALID_PARAMETER;

    if (Depth >= ACPI_MAX_PACKAGE_DEPTH)
        return STATUS_ACPI_STACK_OVERFLOW;

    type = uacpi_object_get_type(Obj);

    switch (type)
    {
    case UACPI_OBJECT_INTEGER:
        if (Count) *Count = 1;
        *Size = ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));
        return STATUS_SUCCESS;

    case UACPI_OBJECT_STRING:
    {
        uacpi_data_view view;
        if (uacpi_unlikely_error(uacpi_object_get_string(Obj, &view)))
            return STATUS_UNSUCCESSFUL;

        if (Count) *Count = 1;
        *Size = ACPI_METHOD_ARGUMENT_LENGTH((ULONG)view.length + sizeof(UCHAR));
        return STATUS_SUCCESS;
    }

    case UACPI_OBJECT_BUFFER:
    {
        uacpi_data_view view;
        if (uacpi_unlikely_error(uacpi_object_get_buffer(Obj, &view)))
            return STATUS_UNSUCCESSFUL;

        if (Count) *Count = 1;
        *Size = ACPI_METHOD_ARGUMENT_LENGTH((ULONG)view.length);
        return STATUS_SUCCESS;
    }

    case UACPI_OBJECT_PACKAGE:
    {
        uacpi_object_array arr;
        ULONG totalSize = 0;
        ULONG i;
        NTSTATUS status;

        if (uacpi_unlikely_error(uacpi_object_get_package(Obj, &arr)))
            return STATUS_UNSUCCESSFUL;

        for (i = 0; i < (ULONG)arr.count; i++)
        {
            ULONG elementSize;
            status = AcpiNewEvalGetElementSize(arr.objects[i], Depth + 1, NULL, &elementSize);
            if (!NT_SUCCESS(status))
                return status;
            totalSize += elementSize;
        }

        if (Depth > 0)
            totalSize = ACPI_METHOD_ARGUMENT_LENGTH(totalSize);

        if (Count) *Count = (ULONG)arr.count;
        *Size = totalSize;
        return STATUS_SUCCESS;
    }

    default:
        return STATUS_NOT_SUPPORTED;
    }
}

static
NTSTATUS
AcpiNewEvalConvertResults(
    _Out_ ACPI_METHOD_ARGUMENT *Argument,
    _In_ ULONG Depth,
    _In_ uacpi_object *Obj)
{
    uacpi_object_type type;

    if (!Argument || !Obj)
        return STATUS_INVALID_PARAMETER;

    if (Depth >= ACPI_MAX_PACKAGE_DEPTH)
        return STATUS_ACPI_STACK_OVERFLOW;

    type = uacpi_object_get_type(Obj);

    switch (type)
    {
    case UACPI_OBJECT_INTEGER:
    {
        uacpi_u64 value = 0;
        if (uacpi_unlikely_error(uacpi_object_get_integer(Obj, &value)))
            return STATUS_UNSUCCESSFUL;

        Argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        Argument->DataLength = sizeof(ULONG);
        Argument->Argument = (ULONG)value;
        return STATUS_SUCCESS;
    }

    case UACPI_OBJECT_STRING:
    {
        uacpi_data_view view;
        if (uacpi_unlikely_error(uacpi_object_get_string(Obj, &view)))
            return STATUS_UNSUCCESSFUL;

        if (view.length + 1 > USHRT_MAX)
            return STATUS_BUFFER_OVERFLOW;

        Argument->Type = ACPI_METHOD_ARGUMENT_STRING;
        Argument->DataLength = (USHORT)(view.length + 1);
        if (view.length)
            RtlCopyMemory(&Argument->Data[0], view.const_text, view.length);
        Argument->Data[view.length] = ANSI_NULL;
        return STATUS_SUCCESS;
    }

    case UACPI_OBJECT_BUFFER:
    {
        uacpi_data_view view;
        if (uacpi_unlikely_error(uacpi_object_get_buffer(Obj, &view)))
            return STATUS_UNSUCCESSFUL;

        if (view.length > USHRT_MAX)
            return STATUS_BUFFER_OVERFLOW;

        Argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
        Argument->DataLength = (USHORT)view.length;
        if (view.length)
            RtlCopyMemory(&Argument->Data[0], view.const_bytes, view.length);
        return STATUS_SUCCESS;
    }

    case UACPI_OBJECT_PACKAGE:
    {
        uacpi_object_array arr;
        ACPI_METHOD_ARGUMENT *ptr;
        NTSTATUS status;
        ULONG i;

        if (uacpi_unlikely_error(uacpi_object_get_package(Obj, &arr)))
            return STATUS_UNSUCCESSFUL;

        ptr = Argument;
        if (Depth > 0)
        {
            ULONG totalSize = 0;

            for (i = 0; i < (ULONG)arr.count; i++)
            {
                ULONG elementSize;
                status = AcpiNewEvalGetElementSize(arr.objects[i], Depth + 1, NULL, &elementSize);
                if (!NT_SUCCESS(status))
                    return status;
                totalSize += elementSize;
            }

            if (totalSize > USHRT_MAX)
                return STATUS_BUFFER_OVERFLOW;

            Argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
            Argument->DataLength = (USHORT)totalSize;
            ptr = (ACPI_METHOD_ARGUMENT UNALIGNED *)Argument->Data;
        }

        for (i = 0; i < (ULONG)arr.count; i++)
        {
            status = AcpiNewEvalConvertResults(ptr, Depth + 1, arr.objects[i]);
            if (!NT_SUCCESS(status))
                return status;
            ptr = ACPI_METHOD_NEXT_ARGUMENT(ptr);
        }

        return STATUS_SUCCESS;
    }

    default:
        return STATUS_NOT_SUPPORTED;
    }
}

static
NTSTATUS
AcpiNewBuildArgsFromComplex(
    _In_reads_bytes_(InputLength) const UCHAR *InputBuffer,
    _In_ ULONG InputLength,
    _In_ const ACPI_EVAL_INPUT_BUFFER_COMPLEX *Complex,
    _Out_ uacpi_object_array *OutArgs,
    _Outptr_result_maybenull_ uacpi_object ***OutObjArrayToFree)
{
    ULONG i;
    uacpi_object **objects;
    const ACPI_METHOD_ARGUMENT UNALIGNED *arg;
    ULONG remaining;

    if (!OutArgs || !OutObjArrayToFree)
        return STATUS_INVALID_PARAMETER;

    *OutObjArrayToFree = NULL;
    OutArgs->objects = NULL;
    OutArgs->count = 0;

    if (Complex->ArgumentCount == 0)
        return STATUS_SUCCESS;

    objects = (uacpi_object **)ExAllocatePoolWithTag(PagedPool,
                                                     sizeof(uacpi_object *) * Complex->ArgumentCount,
                                                     'gAcu');
    if (!objects)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(objects, sizeof(uacpi_object *) * Complex->ArgumentCount);

    arg = &Complex->Argument[0];
    remaining = InputLength - FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument);

    for (i = 0; i < Complex->ArgumentCount; i++)
    {
        ULONG argLen;
        uacpi_object *obj = NULL;

        if (remaining < sizeof(ACPI_METHOD_ARGUMENT))
            goto invalid;

        argLen = ACPI_METHOD_ARGUMENT_LENGTH(arg->DataLength);
        if (argLen > remaining)
            goto invalid;

        switch (arg->Type)
        {
        case ACPI_METHOD_ARGUMENT_INTEGER:
            obj = uacpi_object_create_integer((uacpi_u64)arg->Argument);
            break;

        case ACPI_METHOD_ARGUMENT_STRING:
        {
            uacpi_data_view view;
            view.const_bytes = &arg->Data[0];
            view.length = arg->DataLength;
            obj = uacpi_object_create_string(view);
            break;
        }

        case ACPI_METHOD_ARGUMENT_BUFFER:
        {
            uacpi_data_view view;
            view.const_bytes = &arg->Data[0];
            view.length = arg->DataLength;
            obj = uacpi_object_create_buffer(view);
            break;
        }

        default:
            goto invalid;
        }

        if (!obj)
            goto oom;

        objects[i] = obj;

        arg = ACPI_METHOD_NEXT_ARGUMENT(arg);
        remaining -= argLen;
    }

    OutArgs->objects = objects;
    OutArgs->count = Complex->ArgumentCount;
    *OutObjArrayToFree = objects;
    return STATUS_SUCCESS;

invalid:
    for (i = 0; i < Complex->ArgumentCount; i++)
        if (objects[i])
            uacpi_object_unref(objects[i]);
    ExFreePoolWithTag(objects, 'gAcu');
    return STATUS_INVALID_PARAMETER;

oom:
    for (i = 0; i < Complex->ArgumentCount; i++)
        if (objects[i])
            uacpi_object_unref(objects[i]);
    ExFreePoolWithTag(objects, 'gAcu');
    return STATUS_INSUFFICIENT_RESOURCES;
}

static
NTSTATUS
AcpiNewPdoEvalMethod(
    _In_ PACPI_NEW_PDO_EXTENSION PdoExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS status;
    uacpi_status st;
    uacpi_object *ret = NULL;
    uacpi_object_array args;
    uacpi_object *stackArgs[1];
    uacpi_object **heapArgs = NULL;

    ACPI_EVAL_INPUT_BUFFER *in = (ACPI_EVAL_INPUT_BUFFER *)Irp->AssociatedIrp.SystemBuffer;
    CHAR methodName[ACPI_OBJECT_NAME_LENGTH];

    args.objects = NULL;
    args.count = 0;

    if (!PdoExt->Node)
        return STATUS_INVALID_DEVICE_STATE;

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        return STATUS_INVALID_DEVICE_STATE;

    if (!AcpiVerifyInBuffer(IoStack, sizeof(ACPI_EVAL_INPUT_BUFFER)))
        return STATUS_BUFFER_TOO_SMALL;

    if (in->Signature != ACPI_EVAL_INPUT_BUFFER_SIGNATURE &&
        in->Signature != ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE &&
        in->Signature != ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE &&
        in->Signature != ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(methodName, in->MethodName, 4);
    methodName[4] = ANSI_NULL;

    if (in->Signature == ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE)
    {
        ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER *si = (ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER *)in;
        stackArgs[0] = uacpi_object_create_integer((uacpi_u64)si->IntegerArgument);
        if (!stackArgs[0])
            return STATUS_INSUFFICIENT_RESOURCES;
        args.objects = &stackArgs[0];
        args.count = 1;
    }
    else if (in->Signature == ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE)
    {
        ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING *ss = (ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING *)in;
        uacpi_data_view view;

        if (!AcpiVerifyInBuffer(IoStack, FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING, String) + ss->StringLength))
            return STATUS_INVALID_PARAMETER;

        view.const_bytes = &ss->String[0];
        view.length = ss->StringLength;
        stackArgs[0] = uacpi_object_create_string(view);
        if (!stackArgs[0])
            return STATUS_INSUFFICIENT_RESOURCES;
        args.objects = &stackArgs[0];
        args.count = 1;
    }
    else if (in->Signature == ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE)
    {
        ACPI_EVAL_INPUT_BUFFER_COMPLEX *cx = (ACPI_EVAL_INPUT_BUFFER_COMPLEX *)in;
        status = AcpiNewBuildArgsFromComplex((const UCHAR *)in,
                                             IoStack->Parameters.DeviceIoControl.InputBufferLength,
                                             cx,
                                             &args,
                                             &heapArgs);
        if (!NT_SUCCESS(status))
            return status;
    }

    st = uacpi_eval(PdoExt->Node, methodName, (args.count ? &args : NULL), &ret);

    if (args.objects)
    {
        uacpi_size i;
        for (i = 0; i < args.count; i++)
            if (args.objects[i])
                uacpi_object_unref(args.objects[i]);
    }
    if (heapArgs)
        ExFreePoolWithTag(heapArgs, 'gAcu');

    if (uacpi_unlikely_error(st))
        return AcpiNewUacpiStatusToNtStatus(st);

    status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    if (ret && IoStack->Parameters.DeviceIoControl.OutputBufferLength)
    {
        PACPI_EVAL_OUTPUT_BUFFER out;
        ULONG count = 0;
        ULONG extraBytes = 0;
        ULONG outSize;

        out = (PACPI_EVAL_OUTPUT_BUFFER)Irp->AssociatedIrp.SystemBuffer;

        status = AcpiNewEvalGetElementSize(ret, 0, &count, &extraBytes);
        if (NT_SUCCESS(status))
        {
            outSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) + extraBytes;

            out->Signature = ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE;
            out->Length = outSize;
            out->Count = count;

            if (!AcpiVerifyOutBuffer(IoStack, outSize))
            {
                Irp->IoStatus.Information = outSize;
                status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                status = AcpiNewEvalConvertResults(out->Argument, 0, ret);
                if (NT_SUCCESS(status))
                    Irp->IoStatus.Information = outSize;
            }
        }
    }

    if (ret)
        uacpi_object_unref(ret);

    return status;
}

NTSTATUS
NTAPI
AcpiNewDispatchDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    Irp->IoStatus.Information = 0;

    if (AcpiNewIsFdo(DeviceObject))
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (AcpiNewIsControl(DeviceObject))
    {
        status = AcpiNewControlDeviceControl(DeviceObject, Irp, irpSp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (!AcpiNewIsPdo(DeviceObject))
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_ACPI_EVAL_METHOD:
        status = AcpiNewPdoEvalMethod((PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension, Irp, irpSp);
        break;

    case IOCTL_QUERY_LID:
    {
        PACPI_NEW_PDO_EXTENSION pdoExt = (PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension;
        uacpi_u64 lid = 0;
        uacpi_status st;

        if (!pdoExt || !pdoExt->Node)
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BOOLEAN))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        st = uacpi_eval_simple_integer(pdoExt->Node, "_LID", &lid);
        if (uacpi_unlikely_error(st))
        {
            status = AcpiNewUacpiStatusToNtStatus(st);
            break;
        }

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG))
        {
            ULONG out = (lid ? 1u : 0u);
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &out, sizeof(out));
            Irp->IoStatus.Information = sizeof(out);
        }
        else
        {
            BOOLEAN out = (lid ? TRUE : FALSE);
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &out, sizeof(out));
            Irp->IoStatus.Information = sizeof(out);
        }

        status = STATUS_SUCCESS;
        break;
    }

    case IOCTL_NOTIFY_SWITCH_EVENT:
    {
        PACPI_NEW_PDO_EXTENSION pdoExt = (PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension;
        uacpi_u64 lid = 0;
        uacpi_status st;
        ULONG current = 0;

        if (!pdoExt || !pdoExt->Node)
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        if (!pdoExt->HardwareIds || wcsstr(pdoExt->HardwareIds, L"PNP0C0D") == NULL)
        {
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        if (!AcpiVerifyOutBuffer(irpSp, sizeof(ULONG)))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        st = uacpi_eval_simple_integer(pdoExt->Node, "_LID", &lid);
        if (uacpi_unlikely_error(st))
        {
            status = AcpiNewUacpiStatusToNtStatus(st);
            break;
        }

        current = (lid ? 1u : 0u);

        if (AcpiVerifyInBuffer(irpSp, sizeof(ULONG)))
        {
            ULONG last = *(ULONG *)Irp->AssociatedIrp.SystemBuffer;
            if (last != current)
            {
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &current, sizeof(current));
                Irp->IoStatus.Information = sizeof(current);
                status = STATUS_SUCCESS;
                break;
            }
        }

        status = AcpiNewLidQueueWaitIrp(pdoExt, Irp);
        if (status == STATUS_PENDING)
            return status;
        break;
    }

    case IOCTL_GET_SYS_BUTTON_CAPS:
    {
        ULONG caps = 0;
        if (!AcpiVerifyOutBuffer(irpSp, sizeof(ULONG)))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = AcpiNewButtonQueryCaps((PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension, &caps);
        if (NT_SUCCESS(status))
        {
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &caps, sizeof(caps));
            Irp->IoStatus.Information = sizeof(caps);
        }
        break;
    }

    case IOCTL_GET_SYS_BUTTON_EVENT:
        if (!AcpiVerifyOutBuffer(irpSp, sizeof(ULONG)))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = AcpiNewButtonQueueWaitIrp((PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension, Irp);
        if (status == STATUS_PENDING)
            return status;
        break;

#if (NTDDI_VERSION >= NTDDI_VISTA)
    case IOCTL_ACPI_EVAL_METHOD_EX:
    case IOCTL_ACPI_ENUM_CHILDREN:
#endif
    case IOCTL_ACPI_ASYNC_EVAL_METHOD:
    case IOCTL_ACPI_ACQUIRE_GLOBAL_LOCK:
    case IOCTL_ACPI_RELEASE_GLOBAL_LOCK:
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
