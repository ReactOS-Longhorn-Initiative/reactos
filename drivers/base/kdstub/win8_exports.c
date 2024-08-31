#include "intel_net.h"
#include <cportlib/cportlib.h>
#include <stdlib.h>
#include <stdio.h>
#define COM1_DEBUGOUT 0x3F8

/* ***********************************************************************/
CPPORT Rs232ComPortInfo = {(PUCHAR)COM1_DEBUGOUT, 115200, 0};
VOID Rs232PortPutByte(UCHAR ByteToSend)
{
    CpPutByte(&Rs232ComPortInfo, ByteToSend);
}


VOID DebugPrintChar(UCHAR Character)
{
    if (1)
    {
        if (Character == '\n')
            Rs232PortPutByte('\r');

        Rs232PortPutByte(Character);
    }
}

ULONG
KdStubDbgPrint(const char *Format, ...)
{
    va_list ap;
    int Length;
    char* ptr;
    CHAR Buffer[512];

    va_start(ap, Format);
    Length = _vsnprintf(Buffer, sizeof(Buffer), Format, ap);
    va_end(ap);

    /* Check if we went past the buffer */
    if (Length == -1)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 1] = '\n';

        /* Put maximum */
        Length = sizeof(Buffer);
    }

    ptr = Buffer;
    while (Length--)
        DebugPrintChar(*ptr++);

    return 0;
}
/* ***********************************************************************/

#include "rt/pch.h"

VOID
KdSetHibernateRange (
    VOID
    )
{
    KdStubDbgPrint("KdReleaseRxPacket\n");
}
VOID
NTAPI
KdReleaseRxPacket(
    PVOID Adapter,
    ULONG Handle)
{
KdStubDbgPrint("KdReleaseRxPacket\n");
}

NTSTATUS
NTAPI
KdSendTxPacket(
    PVOID Adapter,
    ULONG Handle,
    ULONG Length)
{
    KdStubDbgPrint("KdSendTxPacket\n");
    return 0;
}
VOID
NTAPI
KdShutdownController(
    PVOID Adapter)
{ 
    KdStubDbgPrint("KdShutdownController\n");
 
}

NTSTATUS
NTAPI
KdInitializeController(
    PVOID Adapter)
{
    NTSTATUS Status = 0;
    KdStubDbgPrint("KdInitializeController: Enrty %X\n", Status);
    Status = RealtekInitializeController(Adapter);
    KdStubDbgPrint("KdInitializeController: Status %X\n", Status);
    for(int i = 0; i < 1000000000000000; i++){}
    for(;;)
    {

    }
    return Status;
}

NTSTATUS
NTAPI
KdGetTxPacket(
    PVOID Adapter,
    PULONG Handle)
{
    KdStubDbgPrint("KdGetTxPacket\n");
    return 0;
}

ULONG
NTAPI
KdGetPacketLength(
    PVOID Adapter,
    ULONG Handle)
{
    KdStubDbgPrint("KdGetPacketLength\n");
    return 0;
}

PVOID
NTAPI
KdGetPacketAddress(
    PVOID Adapter,
    ULONG Handle)
{
     KdStubDbgPrint("KdGetPacketAddress\n");
    return NULL;
}


NTSTATUS
NTAPI
KdGetRxPacket(
    _In_ PVOID Adapter,
    _Out_ PULONG Handle,
    _Out_ PVOID *Packet,
    _Out_ PULONG Length)
{
     KdStubDbgPrint("KdGetRxPacket\n");
    return 0;
}

KDNET_EXTENSIBILITY_IMPORTS* KdNetExtensibilityImports;

NTSTATUS
NTAPI
KdInitializeLibrary(
    _In_ PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device)
{
    KdNetExtensibilityImports = ImportTable;

    Device->Memory.Length = RealtekGetHardwareContextSize(Device);

    return STATUS_SUCCESS;
}
