
#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>


NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoFxRegisterDevice(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PPO_FX_DEVICE Device,
    _Out_ POHANDLE *Handle)
{
    UNIMPLEMENTED;

    /* Fake success for storport */
    return STATUS_SUCCESS;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxUnregisterDevice(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxStartDevicePowerManagement(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxActivateComponent(
    _In_ POHANDLE Handle,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxCompleteDevicePowerNotRequired(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxIdleComponent(
    _In_ POHANDLE Handle,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxCompleteIdleCondition(
    _In_ POHANDLE Handle,
    _In_ ULONG Component)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxCompleteIdleState(
    _In_ POHANDLE Handle,
    _In_ ULONG Component)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxSetDeviceIdleTimeout(
    _In_ POHANDLE Handle,
    _In_ ULONGLONG IdleTimeout)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxReportDevicePoweredOn(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxSetComponentLatency(
    _In_ POHANDLE  Handle,
    _In_ ULONG     Component,
    _In_ ULONGLONG Latency
)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxSetComponentResidency(
    _In_ POHANDLE  Handle,
    _In_ ULONG     Component,
    _In_ ULONGLONG Residency
)
{
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoFxPowerControl(
    _In_ POHANDLE Handle,
    _In_ LPCGUID  PowerControlCode,
    _In_opt_ PVOID    InBuffer,
    _In_ SIZE_T   InBufferSize,
    _Out_opt_ PVOID    OutBuffer,
    _In_ SIZE_T   OutBufferSize,
    _Out_opt_ PSIZE_T  BytesReturned
)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
