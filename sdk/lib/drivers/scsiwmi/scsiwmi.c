
#include <ntddk.h>
#include <scsiwmi.h>
#include <debug.h>

_Must_inspect_result_
SCSIPORT_API
BOOLEAN
NTAPI
ScsiPortWmiDispatchFunction(
    _In_ PSCSI_WMILIB_CONTEXT WmiLibInfo,
    _In_ UCHAR MinorFunction,
    _In_ PVOID DeviceContext,
    _In_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
    _In_ PVOID DataPath,
    _In_ ULONG BufferSize,
    _In_ PVOID Buffer)
{
    UNIMPLEMENTED;
    return FALSE;
}

SCSIPORT_API
VOID
NTAPI
ScsiPortWmiPostProcess(
    _Inout_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
    _In_ UCHAR SrbStatus,
    _In_ ULONG BufferUsed)
{
    UNIMPLEMENTED;
}

SCSIPORT_API
VOID
NTAPI
ScsiPortWmiFireLogicalUnitEvent(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ LPGUID Guid,
    _In_ ULONG InstanceIndex,
    _In_ ULONG EventDataSize,
    _In_ PVOID EventData)
{
    UNIMPLEMENTED;
}
