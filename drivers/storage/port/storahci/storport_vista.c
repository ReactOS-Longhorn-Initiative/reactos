#include "common.h"
#include <debug.h>
#if 0
//DONE
StorPortStateChangeDetected
StorPortInitializeTimer 
StorPortRequestTimer
StorPortFreeTimer
StorPortMarkDeviceFailed
StorPortLogTelemetry
StorPortSetUnitAttributes
StorPortGetRequestInfo
StorPortEtwChannelEvent8
StorportGetD3ColdSupport
StorPortInitializePoFxPower
StorPortEtwEvent2
StorPortEtwChannelEvent2
StorPortASyncNotificationDetected
StorPortEtwLevelError
StorPortRegistryWriteAdapterKey
StorPortFreeWorker
StorPortInitializeWorker
StorPortQueueWorkItem
StorPortIsDeviceOperationAllowed
StorPortRegistryReadAdapterKey
StorPortInvokeAcpiMethod
#endif
#if 0
//TODO
StorPortEtwLevelError
StorPortEtwChannelEvent2
StorPortEtwLevelError
StorPortEtwEventDiagnostic
#endif

ULONG StorPortFreeTimer(
  PVOID HwDeviceExtension,
  PVOID TimerHandle
)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}
ULONG StorPortStateChangeDetected(
  PVOID            HwDeviceExtension,
  ULONG            ChangedEntity,
  PSTOR_ADDRESS    Address,
  ULONG            Attributes,
  PHW_STATE_CHANGE HwStateChange,
  PVOID            HwStateChangeContext
)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}


ULONG StorPortInitializeTimer(
  PVOID HwDeviceExtension,
  PVOID *TimerHandle
)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

ULONG StorPortRequestTimer(
  PVOID        HwDeviceExtension,
  PVOID        TimerHandle,
  PHW_TIMER_EX TimerCallback,
  PVOID        CallbackContext,
  ULONGLONG    TimerValue,
  ULONGLONG    TolerableDelay
)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}