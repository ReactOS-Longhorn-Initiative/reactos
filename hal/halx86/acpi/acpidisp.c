

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/\
typedef enum _NT_INTERRUPT_CONTROLLER
{
    ControllerTypePIC = 0,
    ControllerTypeAPIC = 1,
    ControllerTypeGIC = 2
} NT_INTERRUPT_CONTROLLER, *PNT_INTERRUPT_CONTROLLER;

PPM_DISPATCH_TABLE PmAcpiDispatchTable;

/* PRIVATE FUNCTIONS **********************************************************/
NTSTATUS
NTAPI
HaliAcpiSleep(
    _In_opt_ PVOID Context,
    _In_opt_ PENTER_STATE_SYSTEM_HANDLER SystemHandler,
    _In_opt_ PVOID SystemContext,
    _In_ LONG NumberProcessors,
    _In_opt_ LONG volatile * Number)
{
    DPRINT("HaliAcpiSleep\n");
    for(;;)
    {

    }
    return STATUS_SUCCESS;
}


VOID
NTAPI
HaliSetWakeEnable(
    _In_ BOOLEAN Enable)
{
    for(;;)
    {

    }
}

VOID
NTAPI
HaliSetWakeAlarm(
    _In_ ULONGLONG AlartTime,
    _In_ ULONGLONG WakeTimeFields)
{
    for(;;)
    {
        
    }
}

/* halfplemented */
VOID
NTAPI
HalpPowerStateCallback(
    _In_ PVOID CallbackContext,
    _In_ PVOID Argument1,
    _In_ PVOID Argument2)
{
    DPRINT("HalpPowerStateCallback: CallbackContext %X, Arg1 %X, Arg2 %X\n", CallbackContext, Argument1, Argument2);

    if (Argument1 != ULongToPtr(3))
    {
        DPRINT1("HalpPowerStateCallback: Argument1 != 3\n");
        ASSERT(FALSE); // HalpDbgBreakPointEx();
        return;
    }

    if (Argument2 == ULongToPtr(0))
    {
        DPRINT1("HalpPowerStateCallback: FIXME HalpMapNvsArea()\n");
        return;
    }

    if (Argument2 == ULongToPtr(1))
    {
        DPRINT1("HalpPowerStateCallback: FIXME HalpFreeNvsBuffers()\n");
        ASSERT(FALSE); // HalpDbgBreakPointEx();
        return;
    }

    DPRINT1("HalpPowerStateCallback: Unsupported Arg2 %X\n", Argument2);
    ASSERT(FALSE); // HalpDbgBreakPointEx();
}

ULONG
NTAPI
HaliAcpiQueryFlags(VOID)
{
   // UNIMPLEMENTED;
   // ASSERT(FALSE);// HalpDbgBreakPointEx();
    return 0;
}

UCHAR
NTAPI
HalpAcpiPicStateIntact(VOID)
{
   // UNIMPLEMENTED;
   // ASSERT(FALSE);// HalpDbgBreakPointEx();
    return 0;
}

VOID
NTAPI
HalpRestoreInterruptControllerState(VOID)
{
   // UNIMPLEMENTED;
   // ASSERT(FALSE);// HalpDbgBreakPointEx();
}

VOID
NTAPI
HaliSetVectorState(
    _In_ ULONG Vector,
    _In_ ULONG Par2)
{
    //HalpSetVectorState(Vector, Par2);
}

ULONG
NTAPI
HalpGetApicVersion(
    _In_ ULONG Par1)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);// HalpDbgBreakPointEx();
    return 0;
}

VOID
NTAPI
HaliSetMaxLegacyPciBusNumber(
    _In_ ULONG MaxLegacyPciBusNumber)
{
   // HalpMaxPciBus = max(HalpMaxPciBus, MaxLegacyPciBusNumber);
}

BOOLEAN
NTAPI
HaliIsVectorValid(
    _In_ ULONG Vector)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);// HalpDbgBreakPointEx();
    return 0;
}
VOID
NTAPI
HaliAcpiTimerInit(    _In_ PULONG TimerPort,
                   _In_ BOOLEAN TimerValExt);
ULONG
NTAPI
HaliPciInterfaceWriteConfig(_In_ PBUS_HANDLER RootBusHandler,
                            _In_ ULONG BusNumber,
                            _In_ PCI_SLOT_NUMBER SlotNumber,
                            _In_ PVOID Buffer,
                            _In_ ULONG Offset,
                            _In_ ULONG Length)
{
    BUS_HANDLER BusHandler;

    /* Setup fake PCI Bus handler */
    RtlCopyMemory(&BusHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    BusHandler.BusNumber = BusNumber;

    /* Write configuration data */
    HalpWritePCIConfig(&BusHandler, SlotNumber, Buffer, Offset, Length);

    /* Return length */
    return Length;
}
ULONG
NTAPI
HaliPciInterfaceReadConfig(IN PBUS_HANDLER RootBusHandler,
                           IN ULONG BusNumber,
                           IN PCI_SLOT_NUMBER SlotNumber,
                           IN PVOID Buffer,
                           IN ULONG Offset,
                           IN ULONG Length);
#include "dispatch.h"
#if 1
ACPI_PM_DISPATCH_TABLE HalAcpiDispatchTable =
{
    0x48414C20, // 'HAL '
    2,
    HaliAcpiTimerInit,
    NULL,
    HaliAcpiMachineStateInit,
    HaliAcpiQueryFlags,
    HalpAcpiPicStateIntact,
    HalpRestoreInterruptControllerState,
    HaliPciInterfaceReadConfig,
    HaliPciInterfaceWriteConfig,
    HaliSetVectorState,
    HalpGetApicVersion,
    HaliSetMaxLegacyPciBusNumber,
    HaliIsVectorValid 
};
#endif

NTSTATUS
NTAPI
HaliInitPowerManagement(
    _In_ PPM_DISPATCH_TABLE PmDriverDispatchTable,
    _Out_ PPM_DISPATCH_TABLE* PmHalDispatchTable)
{
    UNICODE_STRING CallbackName = RTL_CONSTANT_STRING(L"\\Callback\\PowerState");
    OBJECT_ATTRIBUTES ObjectAttributes;
    PCALLBACK_OBJECT CallbackObject;

    PAGED_CODE();

    /* ReactOS ACPI doesn't bother filling out a ACPI private dispatch*/
    if (PmDriverDispatchTable)
        DPRINT1("Microsoft or VGAL ACPI.sus has been detected\n");
    PmAcpiDispatchTable = PmDriverDispatchTable;

    HalSetWakeEnable = HaliSetWakeEnable;
    HalSetWakeAlarm = HaliSetWakeAlarm;

    *PmHalDispatchTable = (PPM_DISPATCH_TABLE)&HalAcpiDispatchTable; // HAL export interface

    /* Create a power callback */    
    InitializeObjectAttributes(&ObjectAttributes,
                               &CallbackName,
                               (OBJ_CASE_INSENSITIVE | OBJ_PERMANENT),
                               NULL,
                               NULL);

    ExCreateCallback(&CallbackObject, &ObjectAttributes, FALSE, TRUE);
    ExRegisterCallback(CallbackObject, HalpPowerStateCallback, NULL);

    return STATUS_SUCCESS;
}

VOID
NTAPI
HaliAcpiMachineStateInit(
    _In_ ULONG Par1,
    _In_ PHALP_STATE_DATA StateData,
    _Out_ ULONG* OutInterruptModel)
{
    POWER_STATE_HANDLER handler;
    HALP_STATE_CONTEXT Context;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("HaliAcpiMachineStateInit: StateData %p\n", StateData);

    //*OutInterruptModel = 0; // PIC HAL
    *OutInterruptModel = 1; // APIC HAL

    if (StateData[0].Data0)
    {
        handler.Type = 0;
        handler.RtcWake = TRUE;
        handler.Handler = HaliAcpiSleep;

        Context.AsULONG = 0;
        Context.Data1 = StateData[0].Data1;
        Context.Data2 = StateData[0].Data2;
        Context.Flags = 0x4;

        handler.Context = UlongToPtr(Context.AsULONG);

        Status = ZwPowerInformation(SystemPowerStateHandler, &handler, sizeof(handler), NULL, 0);
        ASSERT(NT_SUCCESS(Status));
    }

    KeFlushWriteBuffer();
}