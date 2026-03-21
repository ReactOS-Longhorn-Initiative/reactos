/*
 * PROJECT:     ReactOS AMD Processor Power Management Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        drivers/processor/amdppm/acpi.c
 * PURPOSE:     ACPI namespace method evaluation via IOCTL.
 *
 *              The AMD PPM driver evaluates ACPI control methods by sending
 *              synchronous IOCTL_ACPI_EVAL_METHOD requests to the processor
 *              PDO.  The ACPI bus driver (acpi.sys / acpi_new.sys) below the
 *              PDO handles these IOCTLs and executes the AML.
 *
 *              Individual evaluators (AcpiEval_CST, AcpiEval_PSS, ...) parse
 *              the ACPI_EVAL_OUTPUT_BUFFER into driver-native structures.
 *
 * REFERENCES:  ACPI 6.x specification
 *              ReactOS cmbatt/cmexec.c (reference IOCTL dispatch pattern)
 *
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 */

/* INCLUDES ******************************************************************/

#include "amdppm.h"
#include <acpiioct.h>

/* PRIVATE HELPERS ***********************************************************/

/*
 * AcpiArgAsUlong
 *
 * Extract a ULONG integer from an ACPI_METHOD_ARGUMENT entry.
 * Returns FALSE if the argument type is not ACPI_METHOD_ARGUMENT_INTEGER.
 */
static
BOOLEAN
AcpiArgAsUlong(
    _In_  PACPI_METHOD_ARGUMENT Arg,
    _Out_ PULONG                Value)
{
    if (!Arg || Arg->Type != ACPI_METHOD_ARGUMENT_INTEGER)
        return FALSE;

    *Value = Arg->Argument;
    return TRUE;
}

/*
 * AcpiArgFirstInPackage
 *
 * Return a pointer to the first ACPI_METHOD_ARGUMENT element inside a
 * package argument.  Writes the package's DataLength into *PackageLength.
 */
static
PACPI_METHOD_ARGUMENT
AcpiArgFirstInPackage(
    _In_  PACPI_METHOD_ARGUMENT Package,
    _Out_ PULONG                PackageLength)
{
    if (!Package ||
        (Package->Type != ACPI_METHOD_ARGUMENT_PACKAGE &&
         Package->Type != ACPI_METHOD_ARGUMENT_PACKAGE_EX))
    {
        *PackageLength = 0;
        return NULL;
    }

    *PackageLength = Package->DataLength;
    return (PACPI_METHOD_ARGUMENT)(ULONG_PTR)Package->Data;
}

/*
 * AcpiSendIoctl
 *
 * Helper that builds and sends a synchronous IOCTL to the processor PDO.
 * Handles STATUS_BUFFER_TOO_SMALL by reallocating and retrying once.
 *
 * The caller receives a heap-allocated ACPI_EVAL_OUTPUT_BUFFER in
 * *OutBuffer (caller frees with TAG_AMDPPM_ACPI).
 */
static
NTSTATUS
AcpiSendIoctl(
    _In_     PDEVICE_OBJECT            Pdo,
    _In_     ULONG                     IoControlCode,
    _In_opt_ PVOID                     InBuf,
    _In_     ULONG                     InBufLen,
    _Out_    PACPI_EVAL_OUTPUT_BUFFER *OutBuffer,
    _Out_    PULONG                    OutBufferLen)
{
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    PACPI_EVAL_OUTPUT_BUFFER OutBuf;
    ULONG OutSize;

    *OutBuffer    = NULL;
    *OutBufferLen = 0;

    /* Initial probe – use a small buffer to discover the required size */
    OutSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 64;
    OutBuf = ExAllocatePoolWithTag(PagedPool, OutSize, TAG_AMDPPM_ACPI);
    if (!OutBuf)
        return STATUS_INSUFFICIENT_RESOURCES;

Retry:
    RtlZeroMemory(OutBuf, OutSize);

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(
              IoControlCode,
              Pdo,
              InBuf, InBufLen,
              OutBuf, OutSize,
              FALSE,
              &Event,
              &IoStatus);

    if (!Irp)
    {
        ExFreePoolWithTag(OutBuf, TAG_AMDPPM_ACPI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(Pdo, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (Status == STATUS_BUFFER_TOO_SMALL ||
        Status == STATUS_BUFFER_OVERFLOW)
    {
        ULONG NewSize;

        /*
         * The ACPI driver writes the required size into the Length field of
         * the output buffer when it returns STATUS_BUFFER_TOO_SMALL.
         */
        NewSize = OutBuf->Length;
        ExFreePoolWithTag(OutBuf, TAG_AMDPPM_ACPI);

        if (NewSize <= OutSize || NewSize > 0x10000)
            return STATUS_ACPI_INVALID_DATA;

        OutSize = NewSize;
        OutBuf = ExAllocatePoolWithTag(PagedPool, OutSize, TAG_AMDPPM_ACPI);
        if (!OutBuf)
            return STATUS_INSUFFICIENT_RESOURCES;

        goto Retry;
    }

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(OutBuf, TAG_AMDPPM_ACPI);
        return Status;
    }

    /* Validate the signature */
    if (OutBuf->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
        OutBuf->Count == 0)
    {
        ExFreePoolWithTag(OutBuf, TAG_AMDPPM_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    *OutBuffer    = OutBuf;
    *OutBufferLen = OutSize;

    return STATUS_SUCCESS;
}

/* PUBLIC: ACPI INTERFACE ACQUISITION ***************************************/

/*
 * AcquireAcpiInterfaces
 *
 * For the AMD PPM driver, ACPI method evaluation is done via IOCTLs sent
 * directly to the PDO.  There is no separate "ACPI interface" to acquire –
 * the PDO pointer stored in DevExt->Pdo is sufficient.
 *
 * This function verifies the PDO is valid and pre-stores an observable
 * device reference so the rest of the driver can use it without
 * re-dereferencing.
 */
NTSTATUS
AcquireAcpiInterfaces(
    _In_ PFDO_DATA DevExt)
{
    PAGED_CODE();

    if (!DevExt->Pdo)
    {
        DPRINT1("AmdPpm: AcquireAcpiInterfaces – PDO is NULL\n");
        return STATUS_INVALID_DEVICE_STATE;
    }

    /* Take an extra reference on the PDO so it remains valid for the
     * lifetime of this device extension. */
    ObReferenceObject(DevExt->Pdo);
    DevExt->AcpiPdoReferenced = TRUE;

    DPRINT("AmdPpm: AcquireAcpiInterfaces – PDO %p referenced\n", DevExt->Pdo);

    return STATUS_SUCCESS;
}

/*
 * ReleaseAcpiInterfaces
 *
 * Drops the PDO reference taken by AcquireAcpiInterfaces.
 * Called from EvtDeviceReleaseHardware.
 */
VOID
ReleaseAcpiInterfaces(
    _In_ PFDO_DATA DevExt)
{
    if (DevExt->AcpiPdoReferenced && DevExt->Pdo)
    {
        ObDereferenceObject(DevExt->Pdo);
        DevExt->AcpiPdoReferenced = FALSE;
    }
}

/* PUBLIC: GENERIC EVALUATOR *************************************************/

/*
 * AcpiEvaluateMethod
 *
 * Sends IOCTL_ACPI_EVAL_METHOD to the processor PDO to execute a 4-char
 * ACPI method name (MethodName is a packed ULONG, e.g. ACPI_METHOD_CST).
 *
 * If InputBuffer is NULL, a plain ACPI_EVAL_INPUT_BUFFER is used.
 * If InputBuffer is not NULL, the caller is responsible for providing a
 * properly-formed ACPI_EVAL_INPUT_BUFFER_COMPLEX (with Signature set).
 *
 * On success *OutputBuffer is a heap-allocated ACPI_EVAL_OUTPUT_BUFFER
 * (caller frees with TAG_AMDPPM_ACPI).
 */
NTSTATUS
AcpiEvaluateMethod(
    _In_     PFDO_DATA  DevExt,
    _In_     ULONG      MethodName,
    _In_opt_ PVOID      InputBuffer,
    _Out_    PVOID     *OutputBuffer,
    _Out_    PULONG     OutputBufferReturned)
{
    ACPI_EVAL_INPUT_BUFFER SimpleInput;
    PVOID InBuf;
    ULONG InBufLen;

    PAGED_CODE();

    *OutputBuffer         = NULL;
    *OutputBufferReturned = 0;

    if (!DevExt->Pdo)
        return STATUS_INVALID_DEVICE_STATE;

    if (InputBuffer)
    {
        /* Caller provides a fully-formed input buffer */
        InBuf    = InputBuffer;
        InBufLen = ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->Signature ==
                       ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE
                   ? sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) /* minimum; real len
                                                               is in ->Size */
                   : sizeof(ACPI_EVAL_INPUT_BUFFER);

        /*
         * For complex buffers the actual length is stored in the Size field.
         */
        if (((PACPI_EVAL_INPUT_BUFFER_COMPLEX)InputBuffer)->Signature ==
            ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE)
        {
            InBufLen = ((PACPI_EVAL_INPUT_BUFFER_COMPLEX)InputBuffer)->Size;
        }
    }
    else
    {
        /* Simple no-argument invocation */
        RtlZeroMemory(&SimpleInput, sizeof(SimpleInput));
        SimpleInput.Signature          = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
        SimpleInput.MethodNameAsUlong  = MethodName;

        InBuf    = &SimpleInput;
        InBufLen = sizeof(SimpleInput);
    }

    return AcpiSendIoctl(DevExt->Pdo,
                         IOCTL_ACPI_EVAL_METHOD,
                         InBuf, InBufLen,
                         (PACPI_EVAL_OUTPUT_BUFFER *)OutputBuffer,
                         OutputBufferReturned);
}

/* PUBLIC: INDIVIDUAL METHOD EVALUATORS *************************************/

/*
 * EnumerateControlMethods
 *
 * Probes which ACPI namespace methods are present under the processor device
 * node by evaluating each method with a zero-element output buffer.
 * STATUS_BUFFER_TOO_SMALL / STATUS_SUCCESS → method exists.
 * STATUS_NOT_FOUND / STATUS_OBJECT_NAME_NOT_FOUND → method absent.
 */
NTSTATUS
EnumerateControlMethods(
    _In_  PFDO_DATA  DevExt,
    _Out_ PULONG     FeaturesPresent)
{
    static const struct
    {
        ULONG  Name;
        ULONG  CapBit;
    } MethodTable[] =
    {
        { ACPI_METHOD_OSC,  AMD_CAP_OSC  },
        { ACPI_METHOD_PDC,  AMD_CAP_PDC  },
        { ACPI_METHOD_CST,  AMD_CAP_CST  },
        { ACPI_METHOD_PCT,  AMD_CAP_PCT  },
        { ACPI_METHOD_PSD,  AMD_CAP_PSD  },
        { ACPI_METHOD_PSS,  AMD_CAP_PSS  },
        { ACPI_METHOD_XPSS, AMD_CAP_XPSS },
        { ACPI_METHOD_PPC,  AMD_CAP_PPC  },
        { ACPI_METHOD_PTC,  AMD_CAP_TSS  },
        { ACPI_METHOD_TSD,  AMD_CAP_TSD  },
        { ACPI_METHOD_TSS,  AMD_CAP_TSS  },
        { ACPI_METHOD_TPC,  AMD_CAP_TPC  },
    };

    NTSTATUS Status;
    PVOID   DummyOut;
    ULONG   DummyLen;
    ULONG   Caps = 0;
    ULONG   i;

    PAGED_CODE();

    *FeaturesPresent = 0;

    for (i = 0; i < ARRAYSIZE(MethodTable); i++)
    {
        DummyOut = NULL;
        DummyLen = 0;

        Status = AcpiEvaluateMethod(DevExt,
                                    MethodTable[i].Name,
                                    NULL,
                                    &DummyOut,
                                    &DummyLen);

        if (DummyOut)
            ExFreePoolWithTag(DummyOut, TAG_AMDPPM_ACPI);

        if (NT_SUCCESS(Status) ||
            Status == STATUS_BUFFER_TOO_SMALL ||
            Status == STATUS_BUFFER_OVERFLOW)
        {
            Caps |= MethodTable[i].CapBit;
        }
    }

    *FeaturesPresent = Caps;

    DPRINT("AmdPpm: EnumerateControlMethods: 0x%08lx\n", Caps);

    return STATUS_SUCCESS;
}

/*
 * AcpiEval_OSC
 *
 * Evaluates _OSC (Operating System Capabilities).
 * Input:  OscInput / OscInputSize – caller-allocated OSC_INPUT_BUFFER.
 * Output: *OutBuffer – heap-allocated OSC_OUTPUT_BUFFER (caller frees).
 *
 * _OSC input is encoded as a complex buffer with 4 DWORD arguments:
 *   UUID[0..3], Revision, Count, Caps[0], Caps[1]
 */
NTSTATUS
AcpiEval_OSC(
    _In_  PFDO_DATA          DevExt,
    _In_  POSC_INPUT_BUFFER  OscInput,
    _In_  USHORT             OscInputSize,
    _Out_ OSC_OUTPUT_BUFFER **OutBuffer)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    OSC_OUTPUT_BUFFER *OscOut;

    /* Build ACPI_EVAL_INPUT_BUFFER_COMPLEX for _OSC */
    struct
    {
        ACPI_EVAL_INPUT_BUFFER_COMPLEX Hdr;
        ACPI_METHOD_ARGUMENT           Args[4];
    } InBuf;

    UNREFERENCED_PARAMETER(OscInputSize);

    PAGED_CODE();

    *OutBuffer = NULL;

    RtlZeroMemory(&InBuf, sizeof(InBuf));
    InBuf.Hdr.Signature           = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    InBuf.Hdr.MethodNameAsUlong   = ACPI_METHOD_OSC;
    InBuf.Hdr.Size                = sizeof(InBuf);
    InBuf.Hdr.ArgumentCount       = 4;

    /* Arg0: UUID as buffer */
    InBuf.Args[0].Type            = ACPI_METHOD_ARGUMENT_BUFFER;
    InBuf.Args[0].DataLength      = 16;
    RtlCopyMemory(InBuf.Args[0].Data, OscInput->Uuid, 16);

    /* Arg1: Revision */
    InBuf.Args[1].Type            = ACPI_METHOD_ARGUMENT_INTEGER;
    InBuf.Args[1].DataLength      = sizeof(ULONG);
    InBuf.Args[1].Argument        = OscInput->Revision;

    /* Arg2: Count */
    InBuf.Args[2].Type            = ACPI_METHOD_ARGUMENT_INTEGER;
    InBuf.Args[2].DataLength      = sizeof(ULONG);
    InBuf.Args[2].Argument        = OscInput->Count;

    /* Arg3: Capabilities (buffer of Count DWORDs) */
    InBuf.Args[3].Type            = ACPI_METHOD_ARGUMENT_BUFFER;
    InBuf.Args[3].DataLength      = (USHORT)(OscInput->Count * sizeof(ULONG));
    RtlCopyMemory(InBuf.Args[3].Data,
                  OscInput->Capabilities,
                  OscInput->Count * sizeof(ULONG));

    Status = AcpiSendIoctl(DevExt->Pdo,
                           IOCTL_ACPI_EVAL_METHOD,
                           &InBuf, sizeof(InBuf),
                           &AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    OscOut = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(OSC_OUTPUT_BUFFER),
                                   TAG_AMDPPM_ACPI);
    if (!OscOut)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(OscOut, sizeof(*OscOut));

    /* Parse output: status DWORD + capabilities DWORDs */
    if (AcpiOut->Count >= 1)
        AcpiArgAsUlong(AcpiOut->Argument, &OscOut->Status);

    if (AcpiOut->Count >= 2)
        AcpiArgAsUlong(ACPI_METHOD_NEXT_ARGUMENT(AcpiOut->Argument),
                       &OscOut->Capabilities[0]);

    if (AcpiOut->Count >= 3)
    {
        PACPI_METHOD_ARGUMENT Arg = AcpiOut->Argument;
        Arg = ACPI_METHOD_NEXT_ARGUMENT(Arg);
        Arg = ACPI_METHOD_NEXT_ARGUMENT(Arg);
        AcpiArgAsUlong(Arg, &OscOut->Capabilities[1]);
    }

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    *OutBuffer = OscOut;
    return STATUS_SUCCESS;
}

/*
 * AcpiEval_PDC
 *
 * Evaluates _PDC (Processor Driver Capabilities).
 * Sends the capabilities buffer as a BUFFER argument; discards output.
 */
NTSTATUS
AcpiEval_PDC(
    _In_ PFDO_DATA          DevExt,
    _In_ PPDC_INPUT_BUFFER  InBuffer,
    _In_ USHORT             InBufferSize)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    /* Worst-case complex input: header + 1 BUFFER arg for the caps array */
    ULONG ComplexSize;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX Complex;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(InBufferSize);

    ComplexSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX)
                  + FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Data)
                  + InBuffer->Count * sizeof(ULONG);

    Complex = ExAllocatePoolWithTag(PagedPool, ComplexSize, TAG_AMDPPM_ACPI);
    if (!Complex)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Complex, ComplexSize);
    Complex->Signature           = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    Complex->MethodNameAsUlong   = ACPI_METHOD_PDC;
    Complex->Size                = ComplexSize;
    Complex->ArgumentCount       = 1;

    Complex->Argument[0].Type       = ACPI_METHOD_ARGUMENT_BUFFER;
    Complex->Argument[0].DataLength = (USHORT)(InBuffer->Count * sizeof(ULONG));
    RtlCopyMemory(Complex->Argument[0].Data,
                  InBuffer->Capabilities,
                  InBuffer->Count * sizeof(ULONG));

    Status = AcpiSendIoctl(DevExt->Pdo,
                           IOCTL_ACPI_EVAL_METHOD,
                           Complex, ComplexSize,
                           &AcpiOut, &AcpiOutLen);

    ExFreePoolWithTag(Complex, TAG_AMDPPM_ACPI);
    if (AcpiOut)
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    /* _PDC return value is not meaningful; ignore it */
    if (Status == STATUS_ACPI_INVALID_DATA)
        Status = STATUS_SUCCESS;

    return Status;
}

/*
 * AcpiEval_CST
 *
 * Evaluates _CST and parses the result into a heap-allocated ACPI_CST.
 * Caller frees with TAG_AMDPPM_CST.
 *
 * _CST returns:
 *   Package { Count, Package { Register, Type, Latency, Power }, ... }
 */
NTSTATUS
AcpiEval_CST(
    _In_  PFDO_DATA   DevExt,
    _Out_ ACPI_CST  **CStates)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    PACPI_METHOD_ARGUMENT CountArg, PkgArg;
    ULONG Count, i;
    ACPI_CST *Cst;

    PAGED_CODE();

    *CStates = NULL;

    Status = AcpiEvaluateMethod(DevExt, ACPI_METHOD_CST,
                                NULL, (PVOID*)&AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    if (AcpiOut->Count < 2)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    CountArg = AcpiOut->Argument;

    if (!AcpiArgAsUlong(CountArg, &Count) || Count == 0 || Count > 8)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    Cst = ExAllocatePoolWithTag(
              PagedPool,
              FIELD_OFFSET(ACPI_CST, States) + Count * sizeof(ACPI_CST_STATE),
              TAG_AMDPPM_CST);
    if (!Cst)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Cst->Count = Count;

    /* Each subsequent argument is a package: { Register, Type, Latency, Power } */
    PkgArg = ACPI_METHOD_NEXT_ARGUMENT(CountArg);

    for (i = 0; i < Count; i++)
    {
        ULONG PkgLen = 0;
        PACPI_METHOD_ARGUMENT Sub = AcpiArgFirstInPackage(PkgArg, &PkgLen);

        if (Sub && PkgLen >= 4 * (sizeof(USHORT) * 2))
        {
            /* Skip the Register (GEN_ADDR buffer) */
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Cst->States[i].Type);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Cst->States[i].Latency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Cst->States[i].Power);
        }
        else
        {
            RtlZeroMemory(&Cst->States[i], sizeof(ACPI_CST_STATE));
        }

        PkgArg = ACPI_METHOD_NEXT_ARGUMENT(PkgArg);
    }

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    *CStates = Cst;
    return STATUS_SUCCESS;
}

/*
 * AcpiEval_PSS
 *
 * Evaluates _PSS and returns a heap-allocated ACPI_PSS.
 * Caller frees with TAG_AMDPPM_PSS.
 *
 * Each _PSS package entry: { CoreFrequency, Power, TransLatency,
 *                             BusMasterLatency, Control, Status }
 */
NTSTATUS
AcpiEval_PSS(
    _In_  PFDO_DATA   DevExt,
    _Out_ ACPI_PSS  **Address)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    PACPI_METHOD_ARGUMENT PkgArg;
    ULONG Count, i;
    ACPI_PSS *Pss;

    PAGED_CODE();

    *Address = NULL;

    Status = AcpiEvaluateMethod(DevExt, ACPI_METHOD_PSS,
                                NULL, (PVOID*)&AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    Count = AcpiOut->Count;

    if (Count == 0 || Count > 32)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    Pss = ExAllocatePoolWithTag(
              PagedPool,
              FIELD_OFFSET(ACPI_PSS, States) + Count * sizeof(ACPI_PSS_STATE),
              TAG_AMDPPM_PSS);
    if (!Pss)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Pss->Count = Count;
    PkgArg = AcpiOut->Argument;

    for (i = 0; i < Count; i++)
    {
        ULONG PkgLen = 0;
        PACPI_METHOD_ARGUMENT Sub = AcpiArgFirstInPackage(PkgArg, &PkgLen);

        if (Sub)
        {
            AcpiArgAsUlong(Sub, &Pss->States[i].CoreFrequency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].Power);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].TransitionLatency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].BusMasterLatency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].Control);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].Status);
        }

        PkgArg = ACPI_METHOD_NEXT_ARGUMENT(PkgArg);
    }

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    *Address = Pss;
    return STATUS_SUCCESS;
}

/*
 * AcpiEval_XPSS
 *
 * Same as AcpiEval_PSS but for _XPSS (extended P-state table).
 * The first 6 fields are layout-compatible with ACPI_PSS_STATE.
 */
NTSTATUS
AcpiEval_XPSS(
    _In_  PFDO_DATA   DevExt,
    _Out_ ACPI_PSS  **Address)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    PACPI_METHOD_ARGUMENT PkgArg;
    ULONG Count, i;
    ACPI_PSS *Pss;

    PAGED_CODE();

    *Address = NULL;

    Status = AcpiEvaluateMethod(DevExt, ACPI_METHOD_XPSS,
                                NULL, (PVOID*)&AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    Count = AcpiOut->Count;

    if (Count == 0 || Count > 32)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    Pss = ExAllocatePoolWithTag(
              PagedPool,
              FIELD_OFFSET(ACPI_PSS, States) + Count * sizeof(ACPI_PSS_STATE),
              TAG_AMDPPM_PSS);
    if (!Pss)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Pss->Count = Count;
    PkgArg = AcpiOut->Argument;

    for (i = 0; i < Count; i++)
    {
        ULONG PkgLen = 0;
        PACPI_METHOD_ARGUMENT Sub = AcpiArgFirstInPackage(PkgArg, &PkgLen);

        if (Sub)
        {
            AcpiArgAsUlong(Sub, &Pss->States[i].CoreFrequency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].Power);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].TransitionLatency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].BusMasterLatency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].Control);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Pss->States[i].Status);
        }

        PkgArg = ACPI_METHOD_NEXT_ARGUMENT(PkgArg);
    }

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    *Address = Pss;
    return STATUS_SUCCESS;
}

/*
 * AcpiEval_PCT_PTC
 *
 * Evaluates _PCT (performance control/status registers) or
 * _PTC (throttle control/status registers).
 * Both return a two-element package of GEN_ADDR register descriptors.
 *
 * ObjectName: ACPI_METHOD_PCT or ACPI_METHOD_PTC
 */
NTSTATUS
AcpiEval_PCT_PTC(
    _In_  PFDO_DATA          DevExt,
    _In_  ULONG              ObjectName,
    _Out_ ACPI_CTRL_STATUS  *Address)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    PACPI_METHOD_ARGUMENT Arg;
    ACPI_EVAL_INPUT_BUFFER InBuf;

    PAGED_CODE();

    RtlZeroMemory(Address, sizeof(*Address));

    /* Build a simple no-argument input buffer with the target method name */
    RtlZeroMemory(&InBuf, sizeof(InBuf));
    InBuf.Signature          = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    InBuf.MethodNameAsUlong  = ObjectName;

    Status = AcpiSendIoctl(DevExt->Pdo,
                           IOCTL_ACPI_EVAL_METHOD,
                           &InBuf, sizeof(InBuf),
                           &AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    if (AcpiOut->Count < 2)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    Arg = AcpiOut->Argument;

    /* Each register is returned as a BUFFER containing a raw GEN_ADDR */
    if (Arg->Type == ACPI_METHOD_ARGUMENT_BUFFER &&
        Arg->DataLength >= sizeof(GEN_ADDR))
    {
        RtlCopyMemory(&Address->Control, Arg->Data, sizeof(GEN_ADDR));
    }

    Arg = ACPI_METHOD_NEXT_ARGUMENT(Arg);

    if (Arg->Type == ACPI_METHOD_ARGUMENT_BUFFER &&
        Arg->DataLength >= sizeof(GEN_ADDR))
    {
        RtlCopyMemory(&Address->Status, Arg->Data, sizeof(GEN_ADDR));
    }

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    return STATUS_SUCCESS;
}

/*
 * AcpiEval_PSD_TSD
 *
 * Evaluates _PSD (P-state coordination domain) or _TSD (T-state domain).
 * Returns a heap-allocated ACPI_XSD (caller frees with TAG_AMDPPM_ACPI).
 *
 * Each package entry: { NumEntries, Revision, Domain, CoordType, NumProcs }
 */
NTSTATUS
AcpiEval_PSD_TSD(
    _In_  PFDO_DATA   DevExt,
    _In_  ULONG       ObjectName,
    _Out_ ACPI_XSD  **Address)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    PACPI_METHOD_ARGUMENT PkgArg;
    ULONG Count, i;
    ACPI_XSD *Xsd;
    ACPI_EVAL_INPUT_BUFFER InBuf;

    PAGED_CODE();

    *Address = NULL;

    RtlZeroMemory(&InBuf, sizeof(InBuf));
    InBuf.Signature         = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    InBuf.MethodNameAsUlong = ObjectName;

    Status = AcpiSendIoctl(DevExt->Pdo,
                           IOCTL_ACPI_EVAL_METHOD,
                           &InBuf, sizeof(InBuf),
                           &AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    Count = AcpiOut->Count;

    if (Count == 0 || Count > 64)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    Xsd = ExAllocatePoolWithTag(
              PagedPool,
              FIELD_OFFSET(ACPI_XSD, Entries) + Count * sizeof(ACPI_XSD_ENTRY),
              TAG_AMDPPM_ACPI);
    if (!Xsd)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Xsd->Count = Count;
    PkgArg = AcpiOut->Argument;

    for (i = 0; i < Count; i++)
    {
        ULONG PkgLen = 0;
        PACPI_METHOD_ARGUMENT Sub = AcpiArgFirstInPackage(PkgArg, &PkgLen);

        if (Sub)
        {
            AcpiArgAsUlong(Sub, &Xsd->Entries[i].NumEntries);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Xsd->Entries[i].Revision);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Xsd->Entries[i].Domain);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Xsd->Entries[i].CoordType);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Xsd->Entries[i].NumProcessors);
        }

        PkgArg = ACPI_METHOD_NEXT_ARGUMENT(PkgArg);
    }

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    *Address = Xsd;
    return STATUS_SUCCESS;
}

/*
 * AcpiEval_PPC
 *
 * Evaluates _PPC (Performance Present Capabilities).
 * Returns an integer: highest P-state index the platform currently allows.
 */
NTSTATUS
AcpiEval_PPC(
    _In_  PFDO_DATA  DevExt,
    _Out_ PULONG     PPC)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;

    PAGED_CODE();

    *PPC = 0;

    Status = AcpiEvaluateMethod(DevExt, ACPI_METHOD_PPC,
                                NULL, (PVOID*)&AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    if (AcpiOut->Count >= 1)
        AcpiArgAsUlong(AcpiOut->Argument, PPC);

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    return STATUS_SUCCESS;
}

/*
 * AcpiEval_TSS
 *
 * Evaluates _TSS and returns a heap-allocated ACPI_TSS.
 * Caller frees with TAG_AMDPPM_TSS.
 *
 * Each entry: { Percent, Power, TransitionLatency, Control, Status }
 */
NTSTATUS
AcpiEval_TSS(
    _In_  PFDO_DATA   DevExt,
    _Out_ ACPI_TSS  **Address)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    PACPI_METHOD_ARGUMENT PkgArg;
    ULONG Count, i;
    ACPI_TSS *Tss;

    PAGED_CODE();

    *Address = NULL;

    Status = AcpiEvaluateMethod(DevExt, ACPI_METHOD_TSS,
                                NULL, (PVOID*)&AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    Count = AcpiOut->Count;

    if (Count == 0 || Count > 32)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    Tss = ExAllocatePoolWithTag(
              PagedPool,
              FIELD_OFFSET(ACPI_TSS, States) + Count * sizeof(ACPI_TSS_STATE),
              TAG_AMDPPM_TSS);
    if (!Tss)
    {
        ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Tss->Count = Count;
    PkgArg = AcpiOut->Argument;

    for (i = 0; i < Count; i++)
    {
        ULONG PkgLen = 0;
        PACPI_METHOD_ARGUMENT Sub = AcpiArgFirstInPackage(PkgArg, &PkgLen);

        if (Sub)
        {
            AcpiArgAsUlong(Sub, &Tss->States[i].Percent);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Tss->States[i].Power);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Tss->States[i].TransitionLatency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Tss->States[i].Control);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            AcpiArgAsUlong(Sub, &Tss->States[i].Status);
        }

        PkgArg = ACPI_METHOD_NEXT_ARGUMENT(PkgArg);
    }

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    *Address = Tss;
    return STATUS_SUCCESS;
}

/*
 * AcpiEval_TPC
 *
 * Evaluates _TPC (Throttling Present Capabilities).
 * Returns an integer: highest T-state index the platform currently allows.
 */
NTSTATUS
AcpiEval_TPC(
    _In_  PFDO_DATA  DevExt,
    _Out_ PULONG     TPC)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;

    PAGED_CODE();

    *TPC = 0;

    Status = AcpiEvaluateMethod(DevExt, ACPI_METHOD_TPC,
                                NULL, (PVOID*)&AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    if (AcpiOut->Count >= 1)
        AcpiArgAsUlong(AcpiOut->Argument, TPC);

    ExFreePoolWithTag(AcpiOut, TAG_AMDPPM_ACPI);

    return STATUS_SUCCESS;
}
