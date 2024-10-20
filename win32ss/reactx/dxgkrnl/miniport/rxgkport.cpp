#include <rxgkrnl.h>

#include <debug.h>

/**
 * @brief The first real transaction between WDDM Miniport and Dxgkrnl, this internal routine
 * gets called by Displib and passes the callback list into a internal struct
 *
 * @param DriverObject DriverObject of the miniport driver itself
 *
 * @param SourceString The registry path provided by Displib
 *
 * @param DriverInitData The callback list provided by a WDDM miniport driver via displib
 *
 * @return NTSTATUS Standard NT return value
 */
NTSTATUS
NTAPI
RxgkPortInitializeMiniport(_In_ PDRIVER_OBJECT DriverObject,
                           _In_ PUNICODE_STRING SourceString,
                           _In_ PDRIVER_INITIALIZATION_DATA DriverInitData)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 1;
}
