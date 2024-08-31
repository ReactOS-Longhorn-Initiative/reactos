/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    kdrealtek.c

Abstract:

    Realtek Network Kernel Debug Support.

Author:

    Joe Ballantyne (joeball)

--*/

#define _REALTEK_C
#include "pch.h"
#undef _REALTEK_C

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4127 4310)

#define ReadRegister(Name) \
    ((RTL_FIELD_SIZE(CSR_STRUC, Name) == sizeof(ULONG)) ?  \
        READ_REGISTER_ULONG((PULONG)&Adapter->BaseAddress->Name) : \
        ((RTL_FIELD_SIZE(CSR_STRUC, Name) == sizeof(USHORT)) ? \
            READ_REGISTER_USHORT((PUSHORT)&Adapter->BaseAddress->Name) : \
            READ_REGISTER_UCHAR((PUCHAR)&Adapter->BaseAddress->Name)))

#define WriteRegister(Name, Value)                                     \
{                                                                      \
    if (RTL_FIELD_SIZE(CSR_STRUC, Name) == sizeof(ULONG)) {            \
        WRITE_REGISTER_ULONG((PULONG)&Adapter->BaseAddress->Name,      \
                             (ULONG)(Value));                          \
    }                                                                  \
    else if (RTL_FIELD_SIZE(CSR_STRUC, Name) == sizeof(USHORT)) {      \
        WRITE_REGISTER_USHORT((PUSHORT)&Adapter->BaseAddress->Name,    \
                              (USHORT)(Value));                        \
    }                                                                  \
    else {                                                             \
        WRITE_REGISTER_UCHAR((PUCHAR)&Adapter->BaseAddress->Name,      \
                             (UCHAR)(Value));                          \
    }                                                                  \
}

#define Swap(Value) \
    (Value) = \
    ((sizeof(Value) == sizeof(ULONG64)) ? RtlUlonglongByteSwap(Value) : \
    ((sizeof(Value) == sizeof(ULONG)) ? RtlUlongByteSwap(Value) : \
    RtlUshortByteSwap(Value)))

#define Equal(_x, _y) \
    (RtlCompareMemory((_x), (_y), sizeof(*_x)) == \
    sizeof(*_x))

ULONG KdNetRxOk;
ULONG KdNetRxError;
ULONG KdNetRxNoDescriptorAvailable;
ULONG KdNetRxFifoOverflow;

ULONG
RealtekGetHardwareContextSize (
    __in PDEBUG_DEVICE_DESCRIPTOR Device
)
{
    UNREFERENCED_PARAMETER(Device);

    return sizeof(REALTEK_ADAPTER);
}

NTSTATUS
HardwareVersionSupported (
    PREALTEK_ADAPTER Adapter
    )
{
    ULONG MacVer;
    NTSTATUS Status;
    ULONG RevId;
    KdStubDbgPrint("HardwareVersionSupported started %X\n", (PVOID)KdNetExtensibilityImports->WritePortUChar);
 

    MacVer=ReadRegister(TCR);
        KdStubDbgPrint("HardwareVersionSupported Passed\n");
 
    for(;;)
    {

    }
    RevId = MacVer;
    KdNetHardwareID = MacVer;
    MacVer &= 0x7C800000;
    RevId &= ( BIT_20 | BIT_21 | BIT_22 );
    Status = STATUS_NO_SUCH_DEVICE;

    if (MacVer==0x00000000)	{
        Adapter->ChipType = RTL8169;

    } else if (MacVer==0x00800000) {
        Adapter->ChipType = RTL8169S1;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x04000000) {
        Adapter->ChipType = RTL8169S2;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x10000000) {
        Adapter->ChipType = RTL8169SB;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x18000000) {
        Adapter->ChipType = RTL8169SC;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x30000000) {
        Adapter->ChipType = RTL8168B;
        Adapter->IsPCIExpress=TRUE;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x30800000) {
        Adapter->ChipType = RTL8136;
        Adapter->IsPCIExpress=TRUE;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x38000000) {
        Adapter->ChipType = RTL8168B_REV_E;
        Adapter->IsPCIExpress=TRUE;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x38800000) {
        Adapter->ChipType = RTL8136_REV_E;
        Adapter->IsPCIExpress=TRUE;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x34000000) {
        Adapter->ChipType = RTL8101;
        Adapter->IsPCIExpress=TRUE;
        Status = STATUS_SUCCESS;

    } else if (MacVer==0x3c000000) {
        Adapter->IsPCIExpress=TRUE;
        if ( RevId == 0x400000 ) {
            Adapter->ChipType = RTL8168C_REV_M;

        } else if ( RevId == 0x300000 ) {
            Adapter->ChipType = RTL8168C_REV_K;

        } else if ( RevId == 0x200000 ) {
            Adapter->ChipType = RTL8168C_REV_G;

        } else if ( RevId == 0x100000 ) {
            Adapter->ChipType = RTL8168C;

        } else {
            Adapter->ChipType = RTL8168C_REV_M;
//            Adapter->HwIcVerUnknown = TRUE;
        }

        Status = STATUS_SUCCESS;

    } else if (MacVer==0x3c800000) {
        Adapter->IsPCIExpress=TRUE;
        if ( RevId == 0x300000 ) {
            Adapter->ChipType = RTL8168CP_REV_D;

        } else if ( RevId == 0x200000 ) {
            Adapter->ChipType = RTL8168CP_REV_C;

        } else if ( RevId == 0x100000 ) {
            Adapter->ChipType = RTL8168CP_REV_B;

        } else if ( RevId == 0x000000 ) {
            Adapter->ChipType = RTL8168CP;

        } else {
            Adapter->ChipType = RTL8168CP_REV_D;
//            Adapter->HwIcVerUnknown = TRUE;
        }

        Status = STATUS_SUCCESS;

    } else if (MacVer==0x24800000) {
        Adapter->ChipType = RTL8136_REV_F;
        Adapter->IsPCIExpress=TRUE;
        Status = STATUS_SUCCESS;

    } else {
        Adapter->ChipType = RTLUNKNOWN;
       // KdNetErrorString = L"Unsupported Realtek chip.";
    }

    return Status;
}

VOID
MP_WritePhyUshort(
    PREALTEK_ADAPTER Adapter,
    UCHAR RegAddr,
    USHORT RegData
)

{
    ULONG TmpUlong=0x80000000;
    ULONG Timeout=0,WaitCount=10;

    TmpUlong |= ( ((ULONG)RegAddr<<16) | (ULONG)RegData );
    WriteRegister(PhyAccessReg, TmpUlong);

    //
    // Wait for writing to Phy ok 
    //

    do {
        KeStallExecutionProcessor(20);
        TmpUlong = ReadRegister(PhyAccessReg);
        Timeout++;
    } while ((TmpUlong & PHYAR_Flag) && (Timeout<WaitCount));

    KeStallExecutionProcessor(5);
    return;
}

USHORT
MP_ReadPhyUshort(
    PREALTEK_ADAPTER Adapter,
    UCHAR RegAddr
)

{
    USHORT RegData;
    ULONG TmpUlong=0x00000000;
    ULONG Timeout=0,WaitCount=10;

    TmpUlong |= ( (ULONG)RegAddr << 16);
    WriteRegister(PhyAccessReg, TmpUlong);

    //
    // Wait for reading from Phy ok 
    //

    do {
        KeStallExecutionProcessor(20);
        TmpUlong = ReadRegister(PhyAccessReg);
        Timeout++;
    } while ( (!(TmpUlong & PHYAR_Flag)) && (Timeout<WaitCount) );

    TmpUlong = ReadRegister(PhyAccessReg);
    RegData = (USHORT)(TmpUlong & 0x0000ffff);
    KeStallExecutionProcessor(5);
    return RegData;
}

NTSTATUS
WritePciExpressPhyUshort(
    PREALTEK_ADAPTER Adapter,
    UCHAR Register,
    USHORT Data
)

{
    ULONG Value;
    ULONG Timeout;
    NTSTATUS Status;

    Status = STATUS_IO_TIMEOUT;

    //
    // Write Data to the desired Phy Register.
    //

    Value = ((ULONG)Register << 16) | (ULONG)Data;
    Value |= EPHYAR_Flag;
    Adapter->BaseAddress->EPhy8168.EPhyAccessReg=Value; 

    //
    // Wait for the write to complete.
    //

    Timeout = 0;
    do {
        KeStallExecutionProcessor(10);
        Value=Adapter->BaseAddress->EPhy8168.EPhyAccessReg;
        if ((Value & EPHYAR_Flag) == FALSE) {
            Status = STATUS_SUCCESS;
            break;
        }

        Timeout++;
    } while (Timeout < 100);

    return Status;
}

NTSTATUS
InitializePciExpressPhy (
    PREALTEK_ADAPTER Adapter)
{
    //
    // Increase the rl5929b RX voltage sensitivity threshold.
    // Original is 0x1b93.
    //

    switch (Adapter->ChipType) {
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8136_REV_E:
        case RTL8136:
        case RTL8101:
            WritePciExpressPhyUshort(Adapter, 0x01, 0x1bd3);
        break;
    }

    return STATUS_SUCCESS;
}

VOID MP_WritePciEConfigSpace(
    PREALTEK_ADAPTER  Adapter,
	USHORT      ByteEnAndAddr,
	ULONG       RegData)
{
	ULONG	Timeout=0;
	ULONG   TmpUlong=0x80000000;
	ULONG   WriteDone;

	WriteRegister(PCIConfigDataReg, RegData);
	TmpUlong|=(ULONG)ByteEnAndAddr;
	WriteRegister(PCIConfigCmdReg, TmpUlong);

	// Wait for writing to PCIE configuration space
	do {
		WriteDone = ReadRegister(PCIConfigCmdReg);
		Timeout++;
		KeStallExecutionProcessor(1);
	} while ( ((WriteDone & 0x80000000)==0) && (Timeout<100) );

}

ULONG MP_ReadPciEConfigSpace(
    PREALTEK_ADAPTER  Adapter,
	USHORT        ByteEnAndAddr)
{
	ULONG	Timeout=0;
	ULONG   TmpUlong=0x00000000;
	ULONG   ReadDone;
	ULONG RegData;

	TmpUlong|=(ULONG)ByteEnAndAddr;
	WriteRegister(PCIConfigCmdReg, TmpUlong);

	// Wait for reading from PCIE configuration space
	do {
		ReadDone = ReadRegister(PCIConfigCmdReg);
		Timeout++;
		KeStallExecutionProcessor(1);
	} while ( (ReadDone & 0x80000000) && (Timeout<100) );

	RegData = ReadRegister(PCIConfigDataReg);

	return RegData;
}

VOID
SetupPCIConfigSpace (
    PREALTEK_ADAPTER Adapter
    )
{
    UCHAR TmpUchar,LTR, CLS;
    USHORT Command;
    ULONG ulResult;
    ULONG MaxPayloadSize;

    //
    // Some motherboard reset the PCI configuration values on wake-up so set the
    // PCI config space value again.
    //
/*
    if (Adapter->IsPCIExpressChip && Adapter->SupportNdis6MSI) {
        ulResult = KdGetPciDataByOffset(Adapter->KdNet->Device->Bus,
                                        Adapter->KdNet->Device->Slot,
                                        (PVOID)&TmpUchar,
                                        0x52,
                                        sizeof(UCHAR));
    
        TmpUchar |= 0x01;
        ulResult = KdSetPciDataByOffset(Adapter->KdNet->Device->Bus,
                                        Adapter->KdNet->Device->Slot,
                                        (PVOID)&TmpUchar,
                                        0x52,
                                        sizeof(UCHAR));
    }
*/

    //
    // RTL8169S1/S2 MUST set the PCI latency to 0x40.  Set all NICs to the same
    // value of 0x40.
    //

    ulResult = KdGetPciDataByOffset(Adapter->KdNet->Device->Bus,
                                    Adapter->KdNet->Device->Slot,
                                    (PVOID)&TmpUchar,
                                    FIELD_OFFSET(PCI_COMMON_CONFIG, LatencyTimer),
                                    sizeof(UCHAR));

    if(ulResult == sizeof(UCHAR)) {
        LTR = 0x40;
        KdSetPciDataByOffset(Adapter->KdNet->Device->Bus,
                             Adapter->KdNet->Device->Slot,
                             (PVOID)&LTR,
                             FIELD_OFFSET(PCI_COMMON_CONFIG, LatencyTimer),
                             sizeof(UCHAR));
    }

    //
    // 8168SB/8169SC has problem on "Memory write and invalidate cycle enable"
    // (modify PCI configuration space cache line)
    //

    if (Adapter->ChipType == RTL8169SB || Adapter->ChipType == RTL8169SC) {
        ulResult = KdGetPciDataByOffset(Adapter->KdNet->Device->Bus,
                                        Adapter->KdNet->Device->Slot,
                                        (PVOID)&Command,
                                        FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                                        sizeof(USHORT));

        if ((ulResult == sizeof(USHORT)) && ((Command & 0x0010) != 0)) {
            Command &= ~0x0010;
            KdSetPciDataByOffset(Adapter->KdNet->Device->Bus,
                                 Adapter->KdNet->Device->Slot,
                                 (PVOID)&Command,
                                 FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                                 sizeof(USHORT));
        }

        //
        // 8169 SB/SC parameter for performance
        // It cache line size is 0x04, adjust it to 0x08, else do nothing.
        //

        ulResult = KdGetPciDataByOffset(Adapter->KdNet->Device->Bus,
                                        Adapter->KdNet->Device->Slot,
                                        (PVOID)&CLS,
                                        FIELD_OFFSET(PCI_COMMON_CONFIG, CacheLineSize),
                                        sizeof(UCHAR));

        if((ulResult == sizeof(UCHAR)) && (CLS == 0x04)) {
            CLS = 0x08;
            KdSetPciDataByOffset(Adapter->KdNet->Device->Bus,
                                 Adapter->KdNet->Device->Slot,
                                 (PVOID)&CLS,
                                 FIELD_OFFSET(PCI_COMMON_CONFIG, CacheLineSize),
                                 sizeof(UCHAR));
        }
    }

    switch (Adapter->ChipType) {
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8136:
        case RTL8136_REV_E:

            //
            // Those command is for PCI configuration spaces			
            // set DMA max payload size
            // PCI-e cards has the registers to directly write PCI configuration space
            //

            MP_WritePciEConfigSpace(Adapter, 0xf088, 0x00021c01);
            MP_WritePciEConfigSpace(Adapter, 0xf0b0, 0x00004000);
            MaxPayloadSize=MP_ReadPciEConfigSpace(Adapter, 0xf068);
            MaxPayloadSize=MaxPayloadSize|0x00007000;	  
            MaxPayloadSize=MaxPayloadSize&0xffff5fff;	  
            WriteRegister(PCIConfigDataReg, MaxPayloadSize);
            MP_WritePciEConfigSpace(Adapter, 0xf068, MaxPayloadSize);
            break;
        case RTL8101:

            //
            // RTL8101 MUST use default values , DO NOT USE A HIGHER PAYLOAD
            // VALUE TX DMA may send stuck if uses a higher payload value
            //

        break;
    }

    return;
}

NTSTATUS
PatchHardware (
    PREALTEK_ADAPTER Adapter
    )
{
    UCHAR Config2;

    if (Adapter->ChipType == RTL8169SC) {

        //
        // This hardware fix should be executed as early as possibile.
        //

        WriteRegister(FixSwitch8169SC, 0x0007ff00);
        Config2 = ReadRegister(CONFIG2);
        Config2 &= 7;
        if (Config2 == 1) {

            //
            // PCI 66 MHz
            //

            WriteRegister(FixSwitch8169SC, 0x000700ff);

        } else {

            //
            // PCI 33 MHz
            //

            WriteRegister(FixSwitch8169SC, 0x0007ff00);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ResetAdapter (
    PREALTEK_ADAPTER Adapter)
{
   // PWCHAR ErrorString;
    ULONG Index;
    NTSTATUS Status;

    Status = STATUS_UNSUCCESSFUL;
    //ErrorString = KdNetErrorString;
    //KdNetErrorString = L"NIC reset timed out.";

    //
    // Work around a HW bug in the 8101
    //
    // If the 8101 RX is disabled, and there is a RX overflow, the NIC RX
    // pointer will be wrong the HW will write data to an incorrect memory
    // location.  To prevent this, enable loopback before RESET.
    //

    if (Adapter->ChipType == RTL8101) {
        WriteRegister(TCR, BIT_18 | BIT_17);
    }

    //
    // Reset the NIC and disable transmit and receive.
    //

    WriteRegister(CmdReg, CR_RST);

    //
    // Wait for the reset to complete.
    //

    for (Index = 0; Index<20; Index++) {
		KeStallExecutionProcessor(1);
        if ((ReadRegister(CmdReg) & CR_RST) == FALSE) {
            Status = STATUS_SUCCESS;
           // KdNetErrorString = ErrorString;
            break;
        }
    }

    //
    // Turn off loopback mode in the 8101 after the reset.
    //

    if (Adapter->ChipType==RTL8101)	{
        WriteRegister(TCR, 0);
    }

    //
    // Reapply hardware patches after a successful reset.
    //

    if (NT_SUCCESS(Status)) {
        Status = PatchHardware(Adapter);
    }

    return Status;
}

PVOID
RealtekGetPacketAddress (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle
)

/*++

Routine Description:

    This function returns a pointer to the first byte of a packet associated
    with the passed handle.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    Handle - Supplies a handle to the packet for which to return the
        starting address.

Return Value:

    Pointer to the first byte of the packet.

--*/

{
    if ((Handle & TRANSMIT_HANDLE) == FALSE) {
        return &Adapter->rxBuffers[Handle].u.buffer[0];

    } else {
        Handle &= ~HANDLE_FLAGS;
        return &Adapter->txBuffers[Handle].u.buffer[0];
    }
}

ULONG
RealtekGetPacketLength (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle
)

/*++

Routine Description:

    This function returns the length of the packet associated with the passed
    handle.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    Handle - Supplies a handle to the packet for which to return the
        length.

Return Value:

    The length of the packet.

--*/

{
    if ((Handle & TRANSMIT_HANDLE) == FALSE) {
        return Adapter->rxDesc[Handle].length;

    } else {
        Handle &= ~HANDLE_FLAGS;
        return Adapter->txDesc[Handle].length;
    }
}

NTSTATUS
RealtekGetTxPacket (
    __in PREALTEK_ADAPTER Adapter,
    __out PULONG Handle
)

/*++

Routine Description:

    This function acquires the hardware resources needed to send a packet and
    returns a handle to those resources.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    Handle - Supplies a pointer to the handle for the packet for which hardware
        resources have been reserved.

Return Value:

    STATUS_SUCCESS when hardware resources have been successfully reserved.

    STATUS_IO_TIMEOUT if the hardware resources could not be reserved.

    STATUS_INVALID_PARAMETER if an invalid Handle pointer or Adapter is passed.

--*/

{
    ULONG Index;
    NTSTATUS Status;

    if ((Adapter == NULL) || (Handle == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto GetTxPacketEnd;
    }

    Index = Adapter->TxIndex;
    Status = STATUS_IO_TIMEOUT;
    if ((Adapter->txDesc[Index].status & RXS_OWN) == FALSE) {
        *Handle = Index | TRANSMIT_HANDLE;

        //
        // Reinitialize the packet transmit descriptor.  (Except for the buffer
        // address which is written only during initialization since it does not
        // change.)
        //

        Adapter->txDesc[Index].VLAN_TAG.Value = 0;
        Adapter->txDesc[Index].TAGC = 0;
        Adapter->txDesc[Index].length = 0;
        Adapter->txDesc[Index].status = TXS_LS | TXS_FS;
        if (Index == (NUM_TX_DESC - 1)) {
            Adapter->txDesc[Index].status |= TXS_EOR;
        }

        //
        // Calculate the next transmit index.
        //

        Index += 1;
        if (Index >= NUM_TX_DESC) {
            Index = 0;
        }

        Adapter->TxIndex = Index;
        Status = STATUS_SUCCESS;
    }

GetTxPacketEnd:
    return Status;
}

NTSTATUS
RealtekSendTxPacket (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle,
    ULONG Length
)

/*++

Routine Description:

    This function sends the packet associated with the passed Handle out to the
    network.  It does not return until the packet has been sent.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    Handle - Supplies the handle of the packet to send.

    Length - Supplies the length of the packet to send.

Return Value:

    STATUS_SUCCESS when a packet has been successfully sent.

    STATUS_IO_TIMEOUT if the packet could not be sent within 100ms.

    STATUS_INVALID_PARAMETER if an invalid Handle or Adapter is passed.

--*/

{
    ULONG Index;
    USHORT InterruptStatus;
    NTSTATUS Status;
    ULONG Timeout;
    USHORT TxStatus;

    Index = (Handle & ~HANDLE_FLAGS);
    if ((Adapter == NULL) || ((Handle & TRANSMIT_HANDLE) == FALSE) ||
        (Index >= NUM_TX_DESC) || (Length > 0xffff)) {
        Status = STATUS_INVALID_PARAMETER;
        goto SendTxPacketEnd;
    }

    Status = STATUS_IO_TIMEOUT;
    TxStatus = Adapter->txDesc[Index].status;
    if ((TxStatus & TXS_OWN) == FALSE) {

        //
        // Clear the transmit OK and transmit error bits in the Interrupt status
        // register.
        //

        WriteRegister(ISR, ISRIMR_TOK | ISRIMR_TER);

        //
        // Write the packet length into the packet descriptor.  Then mark the
        // descriptor as owned by the hardware.
        //

        Adapter->txDesc[Index].length = (USHORT)Length;
        TxStatus |= TXS_OWN;
        Adapter->txDesc[Index].status = TxStatus;

        //
        // Tell the adapter to start DMA.
        //

        WriteRegister(TPPoll, TPPool_NPQ);

        //
        // For async transmits, do not wait for the packet to actually be sent.
        // Simply return success.
        //

        if ((Handle & TRANSMIT_ASYNC) != FALSE) {
            Status = STATUS_SUCCESS;
            goto SendTxPacketEnd;
        }

        //
        // Wait for the hardware to send the packet before returning.  First
        // wait for the FIFO to load the packet, then wait for the transmit
        // OK bit to be set.
        //

        Timeout = 100000;
        for (;;) {
            TxStatus = Adapter->txDesc[Index].status;
            if ((TxStatus & TXS_OWN) == FALSE) {
                InterruptStatus = ReadRegister(ISR);
                if ((InterruptStatus & ISRIMR_TER) == ISRIMR_TER) {
                    Status = STATUS_UNSUCCESSFUL;
                    goto SendTxPacketEnd;
                }
                if ((InterruptStatus & ISRIMR_TOK) == ISRIMR_TOK) {
                    break;
                }
            }

            Timeout -= 1;
            if (Timeout == 0) {
                goto SendTxPacketEnd;
            }

            KeStallExecutionProcessor(1);
        }

        Status = STATUS_SUCCESS;
    }

SendTxPacketEnd:
    return Status;
}

NTSTATUS
SetTxOptions (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle,
    USHORT Options
)

{
    ULONG Index;
    NTSTATUS Status;
    USHORT TxStatus;

    Index = (Handle & ~HANDLE_FLAGS);
    if ((Adapter == NULL) || ((Handle & TRANSMIT_HANDLE) == FALSE) ||
        (Index >= NUM_TX_DESC)) {
        Status = STATUS_INVALID_PARAMETER;
        goto SetTxOptionsEnd;
    }

    Status = STATUS_UNSUCCESSFUL;
    TxStatus = Adapter->txDesc[Index].status;
    if ((TxStatus & TXS_OWN) == FALSE) {
        Adapter->txDesc[Index].TAGC= Options;
        Status = STATUS_SUCCESS;
    }

SetTxOptionsEnd:
    return Status;
}

NTSTATUS
GetTxOptions (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle,
    __out PUSHORT Options
)
{
    ULONG Index;
    NTSTATUS Status;
    USHORT TxStatus;

    Index = (Handle & ~HANDLE_FLAGS);
    if ((Adapter == NULL) || ((Handle & TRANSMIT_HANDLE) == FALSE) ||
        (Index >= NUM_TX_DESC) || (Options == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto SetTxOptionsEnd;
    }

    Status = STATUS_UNSUCCESSFUL;
    TxStatus = Adapter->txDesc[Index].status;
    if ((TxStatus & TXS_OWN) == FALSE) {
        *Options = Adapter->txDesc[Index].TAGC;
        Status = STATUS_SUCCESS;
    }

SetTxOptionsEnd:
    return Status;
}

NTSTATUS
RealtekGetRxPacket (
    __in PREALTEK_ADAPTER Adapter,
    __out PULONG Handle,
    __out PVOID *Packet,
    __out PULONG Length
)

/*++

Routine Description:

    This function returns the next available received packet to the caller.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    Handle - Supplies a pointer to the handle for this packet.  This handle
        will be used to release the resources associated with this packet back
        to the hardware.

    Packet - Supplies a pointer that will be written with the address of the
        start of the packet.

    Length - Supplies a pointer that will be written with the length of the
        recieved packet.

Return Value:

    STATUS_SUCCESS when a packet has been received.

    STATUS_IO_TIMEOUT otherwise.

--*/

{
    ULONG Index;
    NTSTATUS Status;

    Index = Adapter->RxIndex;
    Status = STATUS_IO_TIMEOUT;
    if ((Adapter->rxDesc[Index].status & RXS_OWN) == FALSE) {
        *Packet = &Adapter->rxBuffers[Index].u.buffer[0];
        *Length = Adapter->rxDesc[Index].length;
        *Handle = Index;

        //
        // Calculate the next receive index.
        //

        Index += 1;
        if (Index >= NUM_RX_DESC) {
            Index = 0;
        }

        Adapter->RxIndex = Index;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
RealtekReleaseRxPacket (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle
)

/*++

Routine Description:

    This function reclaims the hardware resources used for the packet
    associated with the passed Handle.  It reprograms the hardware to use those
    resources to receive another packet.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    Handle - Supplies the handle of the packet whose resources should be
        reclaimed to receive another packet.

Return Value:

    None.

--*/

{
    ULONG Index;
    USHORT InterruptStatus;
    USHORT Status;

    Index = Handle;

    //
    // Calculate the status bits for this descriptor.  Make sure to set the
    // end of ring bit if the Index points to the last descriptor in the ring.
    //

    Status = RXS_OWN;
    if (Index == (NUM_RX_DESC - 1)) {
        Status |= RXS_EOR;
    }

    //
    // Zero out the packet buffer.
    //

    RtlZeroMemory(&Adapter->rxBuffers[Index], sizeof(Adapter->rxBuffers[Index]));

    //
    // Reinitialize the packet receive descriptor.  (Except for the buffer
    // address which is written only during initialization since it does not
    // change.)
    //

    Adapter->rxDesc[Index].VLAN_TAG.Value = 0;
    Adapter->rxDesc[Index].TAVA = 0;
    Adapter->rxDesc[Index].length = REALTEK_MAX_PKT_SIZE;
    Adapter->rxDesc[Index].CheckSumStatus = 0;
    Adapter->rxDesc[Index].status = Status;

    //
    // Check the Interrupt Status register and track which receive bits are set.
    //

    InterruptStatus = ReadRegister(ISR);
    InterruptStatus &= (ISRIMR_RX_FOVW | ISRIMR_RDU | ISRIMR_RER | ISRIMR_ROK);
    KdNetRxOk += ((InterruptStatus & ISRIMR_ROK) != FALSE);
    KdNetRxError += ((InterruptStatus & ISRIMR_RER) != FALSE);
    KdNetRxNoDescriptorAvailable += ((InterruptStatus & ISRIMR_RDU) != FALSE);
    KdNetRxFifoOverflow += ((InterruptStatus & ISRIMR_RX_FOVW) != FALSE);

    //
    // Clear the receive status bits in the ISR.  This will restart the DMA
    // engine if it stopped because it ran out of receive descriptors, or the
    // FIFO overflowed.
    //

    WriteRegister(ISR, InterruptStatus);
    return;
}

NTSTATUS 
ResetPhysicalLayer (
    __in PREALTEK_ADAPTER Adapter
    )
{
    USHORT Register;
    NTSTATUS Status;
    ULONG WaitCount;

    Status = STATUS_UNSUCCESSFUL;

    //
    // Reset the Physical Layer.
    //

    MP_WritePhyUshort(Adapter, PHY_REG_BMCR, MDI_CR_RESET);

    for (WaitCount = 0; WaitCount < PHY_RESET_TIME; WaitCount += 1) {
        Register = MP_ReadPhyUshort(Adapter, PHY_REG_BMCR);
        if ((Register & 0x8000) == 0) {
            Status = STATUS_SUCCESS;
            break;
        }

        KeStallExecutionProcessor(10);
    }

    return Status;
}

NTSTATUS
InitializePhysicalLayer (
    __in PREALTEK_ADAPTER Adapter
)
{
    USHORT PhyRegData;
    USHORT PhyRegDataANAR;
    USHORT PhyRegDataANER;
    USHORT PhyRegDataBMSR;
    USHORT PhyRegDataGBCR;
    UCHAR PhyStatus;
    ULONG TmpUlong;
    USHORT PhyRegValue;
    NTSTATUS Status;
    ULONG Wait;

    Status = ResetPhysicalLayer(Adapter);
    if (!NT_SUCCESS(Status)) {
        goto InitializePhysicalLayerEnd;
    }

    MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
    switch(Adapter->ChipType) {
        case RTL8169:
            break;

        case RTL8169S1:
        case RTL8169S2:

            //
            // Let 8169S FixSwitchPhy to "Sel_SD = 0"
            // If not, 10/100M switch will link up after several times
            //

            // TODO: Figure out which byte is FixSwitch8169S.FixSwitchPhy

            TmpUlong = ReadRegister(FixSwitch8169S);
            TmpUlong |= BIT_0;
            WriteRegister(FixSwitch8169S, TmpUlong);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);	
            MP_WritePhyUshort(Adapter, 0x0b, 0x0000);

            //
            // ISSUE: Channel estimation bad
            // SOLUTION: Return channel estimation
            // Add the parameter when driver initialize

            MP_WritePhyUshort( Adapter, 0x1f, 0x0001 );

            // delta of cg

            MP_WritePhyUshort( Adapter, 0x06, 0x006e );

            // initial value of cg

            MP_WritePhyUshort( Adapter, 0x08, 0x0708 );

            // snr eye open

            MP_WritePhyUshort( Adapter, 0x15, 0x1000 );

            // kp for loop filter in tracking stage

            MP_WritePhyUshort( Adapter, 0x18, 0x65c7 );
            MP_WritePhyUshort( Adapter, 0x1f, 0x0000 );

            //
            // PHY linking paramter for better linking result.
            // (for Marvell PHY and CentreCOM giga switch)
            //
            // AUTO or 1000 Full

            /* if ((Adapter->SpeedDuplex == SPEED_DUPLUX_MODE_AUTO_NEGOTIATION) ||
                (Adapter->SpeedDuplex == SPEED_DUPLUX_MODE_1G_FULL_DUPLEX)) */ {

                MP_WritePhyUshort( Adapter, 0x1f, 0x0001 );
                MP_WritePhyUshort( Adapter, 0x3, 0x00a1 );
                MP_WritePhyUshort( Adapter, 0x2, 0x0008 );
                MP_WritePhyUshort( Adapter, 0x1, 0xff20 );
                MP_WritePhyUshort( Adapter, 0x0, 0x1000 );
                MP_WritePhyUshort( Adapter, 0x4, 0x0800 );
                MP_WritePhyUshort( Adapter, 0x4, 0x0000 );

                MP_WritePhyUshort( Adapter, 0x3, 0xff41 );
                MP_WritePhyUshort( Adapter, 0x2, 0xdf60 );
                MP_WritePhyUshort( Adapter, 0x1, 0x0140 );
                MP_WritePhyUshort( Adapter, 0x0, 0x0077 );
                MP_WritePhyUshort( Adapter, 0x4, 0x7800 );
                MP_WritePhyUshort( Adapter, 0x4, 0x7000 );

                MP_WritePhyUshort( Adapter, 0x3, 0xdf01 );
                MP_WritePhyUshort( Adapter, 0x2, 0xdf20 );
                MP_WritePhyUshort( Adapter, 0x1, 0xff95 );
                MP_WritePhyUshort( Adapter, 0x0, 0xfa00 );
                MP_WritePhyUshort( Adapter, 0x4, 0xa800 );
                MP_WritePhyUshort( Adapter, 0x4, 0xa000 );

                MP_WritePhyUshort( Adapter, 0x3, 0xff41 );
                MP_WritePhyUshort( Adapter, 0x2, 0xdf20 );
                MP_WritePhyUshort( Adapter, 0x1, 0x0140 );
                MP_WritePhyUshort( Adapter, 0x0, 0x00bb );
                MP_WritePhyUshort( Adapter, 0x4, 0xb800 );
                MP_WritePhyUshort( Adapter, 0x4, 0xb000 );

                MP_WritePhyUshort( Adapter, 0x3, 0xdf41 );
                MP_WritePhyUshort( Adapter, 0x2, 0xdc60 );
                MP_WritePhyUshort( Adapter, 0x1, 0x6340 );
                MP_WritePhyUshort( Adapter, 0x0, 0x007d );
                MP_WritePhyUshort( Adapter, 0x4, 0xd800 );
                MP_WritePhyUshort( Adapter, 0x4, 0xd000 );

                MP_WritePhyUshort( Adapter, 0x3, 0xdf01 );
                MP_WritePhyUshort( Adapter, 0x2, 0xdf20 );
                MP_WritePhyUshort( Adapter, 0x1, 0xff95 );
                MP_WritePhyUshort( Adapter, 0x0, 0xbf00 );
                MP_WritePhyUshort( Adapter, 0x4, 0xf800 );
                MP_WritePhyUshort( Adapter, 0x4, 0xf000 );

                MP_WritePhyUshort( Adapter, 0x1f, 0x0000 );
            }

            /*
            //Issue 5: INRX hang with 100M switch
            //Solution: freeze cb0
            //Parameter:
            //801f0001
            //80153500 // reduce the eye-open snr threshold 
            //8010f41b (link down)
            //8010f01b (link up)
            //801f0000
            //ps. For 5838A~E

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x15, 0x3500);
            MP_WritePhyUshort(Adapter, 0x10, 0xf41b);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            */

            //Issue 9: Buffalo WBR-B11 clock offset issue
            //Solution: Increase timing loop bandwidth.
            //801f0001 // page 1
            //8010f41b
            //8014fb54
            //8018f5c7
            //801f0000 

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x10, 0xf41b);
            MP_WritePhyUshort(Adapter, 0x14, 0xfb54);
            MP_WritePhyUshort(Adapter, 0x18, 0xf5c7);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            break;

        case RTL8169SB:

#if 0

            // For 3-cut to save power 
            //page1(INRX page):
            //reg#27	841E
            //reg#14	7BFB
            //reg#9		273A

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x1B, 0x841E);
            MP_WritePhyUshort(Adapter, 0x0E, 0x7BFB);
            MP_WritePhyUshort(Adapter, 0x09, 0x273A);			

#endif

            // reset to h/w default

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x1B, 0xD41E);
            MP_WritePhyUshort(Adapter, 0x0E, 0x7BFF);
            MP_WritePhyUshort(Adapter, 0x09, 0xE73A);	

            //page2
            //reg#01	0x90D0

            MP_WritePhyUshort(Adapter, 0x1f, 0x0002);
            MP_WritePhyUshort(Adapter, 0x01, 0x90D0);

            // return to page 0

            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

            // ISSUE: 2-paired wired can not link on giga-bit mode
            // SOLUTION: PHY will automatically speed-down 
            //           when restart n-way failed 7 times

            MP_WritePhyUshort(Adapter, 0x0d, 0xf8a0);

            //
            // longlink issue

            /* if ((Adapter->SpeedDuplex == SPEED_DUPLUX_MODE_AUTO_NEGOTIATION) ||
                (Adapter->SpeedDuplex == SPEED_DUPLUX_MODE_1G_FULL_DUPLEX)) */ {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x04, 0x0000);
                MP_WritePhyUshort(Adapter, 0x03, 0x00a1);
                MP_WritePhyUshort(Adapter, 0x02, 0x0008);
                MP_WritePhyUshort(Adapter, 0x01, 0x0120);
                MP_WritePhyUshort(Adapter, 0x00, 0x1000);
                MP_WritePhyUshort(Adapter, 0x04, 0x0800);
                MP_WritePhyUshort(Adapter, 0x04, 0x9000);
                MP_WritePhyUshort(Adapter, 0x03, 0x802f);
                MP_WritePhyUshort(Adapter, 0x02, 0x4f02);
                MP_WritePhyUshort(Adapter, 0x01, 0x0409);
                MP_WritePhyUshort(Adapter, 0x00, 0xf0f9);
                MP_WritePhyUshort(Adapter, 0x04, 0x9800);
                MP_WritePhyUshort(Adapter, 0x04, 0xf000);
                MP_WritePhyUshort(Adapter, 0x03, 0xdf01);
                MP_WritePhyUshort(Adapter, 0x02, 0xdf20);
                MP_WritePhyUshort(Adapter, 0x01, 0x100a);
                MP_WritePhyUshort(Adapter, 0x00, 0xa0ff);
                MP_WritePhyUshort(Adapter, 0x04, 0xf800);
                MP_WritePhyUshort(Adapter, 0x04, 0xa000);
                MP_WritePhyUshort(Adapter, 0x03, 0xdf01);
                MP_WritePhyUshort(Adapter, 0x02, 0xdf20);
                MP_WritePhyUshort(Adapter, 0x01, 0xff95);
                MP_WritePhyUshort(Adapter, 0x00, 0xba00);
                MP_WritePhyUshort(Adapter, 0x04, 0xa800);
                MP_WritePhyUshort(Adapter, 0x04, 0x0000);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
                // This is correct to add NWAY here
                // but add NWAY will fail on NDISTEST
                //SetupPhyGigSpeedOnAndAutoNeg(Adapter);
            }

            // Because we must do test cable before NWAY
            // We can not start NWAY
            // for fast connect, 2 paired cable
            // giga-bit on
            // THIS IS A NOT PERFACT SOLUTION, DISCARD IT
            //if ((Adapter->SpeedDuplex == SPEED_DUPLUX_MODE_AUTO_NEGOTIATION) ||
            //    (Adapter->SpeedDuplex == SPEED_DUPLUX_MODE_1G_FULL_DUPLEX))
            //{
            // disable auto neg
            //MP_WritePhyUshort(Adapter, 0x00, 0x0000);
            // reset and the phy status will be unlink
            //MP_WritePhyUshort(Adapter, 0x00, 0x8000);
            //Adapter->CableTestBeforeAutoNeg = TRUE;
            //}

            //fix compatible issue

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x10, 0xf41b);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            MP_WritePhyUshort(Adapter, 0x19, 0x8c00);

            /*
            //Issue 5: INRX hang with 100M switch
            //Solution: freeze cb0
            //Parameter:
            //801f0001
            //80153500 // reduce the eye-open snr threshold 
            //8010f41b (link down)
            //8010f01b (link up)
            //801f0000
            //ps. For 5838A~E
            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x15, 0x3500);
            MP_WritePhyUshort(Adapter, 0x10, 0xf41b);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            */

            //Issue 9: Buffalo WBR-B11 clock offset issue
            //Solution: Increase timing loop bandwidth.
            //801f0001 // page 1
            //8010f41b
            //8014fb54
            //8018f5c7
            //801f0000 

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x10, 0xf41b);
            MP_WritePhyUshort(Adapter, 0x14, 0xfb54);
            MP_WritePhyUshort(Adapter, 0x18, 0xf5c7);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            break;

        case RTL8169SC:
        case RTL8169SC_REV_E:
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

            //ISSUE: 2-paired wired can not link on giga-bit mode
            //SOLUTION: PHY will automatically speed-down 
            //when restart n-way failed 7 times

            MP_WritePhyUshort(Adapter, 0x0d, 0xf8a0);

            //Issue 1: Long link with CentreCOM/ Netgear GS508t giga switch 
            //Solution: Modify INRX FSM (Finite State Machine)
            //Parameter:
            //801f 0001 // InRx Page (Page 1). 
            //80040000 // state 0
            //800300a1 // Write the 3rd register for state vector. 
            //80020008 // Write the 2nd register for state vector. 
            //80010120 // Write the 1st register for state vector (Change the run time to 1). 
            //80001000 // Write the No. 0 register for state vector. 
            //80040800 // Load the state vector to state 0. 

            //80049000 // state 9. 
            //8003802f // Write the 3rd register for state vector. 
            //80024f02 // Write the 2nd register for state vector. 
            //80010409 // Write the 1st register for state vector. 
            //8000f099 // Write the No. 0 register for state vector. 
            //80049800 // Load the state vector to state 9. 

            //8004a000 // state a. 
            //8003df01 // Write the 3rd register for state vector. 
            //8002df20 // Write the 2nd register for state vector. 
            //8001ff95 // Write the 1st register for state vector. 
            //8000ba00 // Write the No. 0 register for state vector. 
            //8004a800 // Load the state vector to state a.

            //8004f000 // state f (Adding one state, f before running state a). 
            //8003df01 // Write the 3rd register for state vector. 
            //8002df20 // Write the 2nd register for state vector. 
            //8001101a // Write the 1st register for state vector. 
            //8000a0ff // Write the No. 0 register for state vector. 
            //8004f800 // Load the state vector to state f. 
            //80040000 // Use the loaded states. 
            //801f0000 // Switch to the PCS Page (Page 0). 
            //ps. For RTL8110SC A~F 

            // AUTO or 1000 Full

            /*if ((Adapter->SpeedDuplex == SPEED_DUPLUX_MODE_AUTO_NEGOTIATION) ||
                (Adapter->SpeedDuplex == SPEED_DUPLUX_MODE_1G_FULL_DUPLEX)) */ {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x04, 0x0000);
                MP_WritePhyUshort(Adapter, 0x03, 0x00a1);
                MP_WritePhyUshort(Adapter, 0x02, 0x0008);
                MP_WritePhyUshort(Adapter, 0x01, 0x0120);
                MP_WritePhyUshort(Adapter, 0x00, 0x1000);
                MP_WritePhyUshort(Adapter, 0x04, 0x0800);

                MP_WritePhyUshort(Adapter, 0x04, 0x9000);
                MP_WritePhyUshort(Adapter, 0x03, 0x802f);
                MP_WritePhyUshort(Adapter, 0x02, 0x4f02);
                MP_WritePhyUshort(Adapter, 0x01, 0x0409);
                MP_WritePhyUshort(Adapter, 0x00, 0xf099);
                MP_WritePhyUshort(Adapter, 0x04, 0x9800);

                MP_WritePhyUshort(Adapter, 0x04, 0xa000);
                MP_WritePhyUshort(Adapter, 0x03, 0xdf01);
                MP_WritePhyUshort(Adapter, 0x02, 0xdf20);
                MP_WritePhyUshort(Adapter, 0x01, 0xff95);
                MP_WritePhyUshort(Adapter, 0x00, 0xba00);
                MP_WritePhyUshort(Adapter, 0x04, 0xa800);

                MP_WritePhyUshort(Adapter, 0x04, 0xf000);
                MP_WritePhyUshort(Adapter, 0x03, 0xdf01);
                MP_WritePhyUshort(Adapter, 0x02, 0xdf20);
                MP_WritePhyUshort(Adapter, 0x01, 0x101a);
                MP_WritePhyUshort(Adapter, 0x00, 0xa0ff);
                MP_WritePhyUshort(Adapter, 0x04, 0xf800);
                MP_WritePhyUshort(Adapter, 0x04, 0x0000);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }            

            if(Adapter->ChipType == RTL8169SC_REV_E) {

                //Issue 2: Channel estimation tuning
                //Solution: fix delta_a, aagc_lvl
                //Parameter:
                //801f0001	//page 1
                //800b8480	//new delta_a & aagc_lvl
                //801f0000 // Switch to the PCS Page (Page 0)
                //ps. For 5838F only

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x0b, 0x8480);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

                //Issue 3: no link when frequency offset is too large
                //Solution: disable freeze cb0 & fix ste2 state machine as above
                //Parameter:
                //801f0001	//page 1
                //801867c7		/disable freeze cb0
                //80042000 // state 2
                //8003002f // Write the 3rd register for state vector. 
                //80024360 // Write the 2nd register for state vector. 
                //80010109 // Write the 1st register for state vector (Change the run time to 1). 
                //80003022 // Write the No. 0 register for state vector. 
                //80042800 // Load the state vector to state 0. 
                //801f0000 // Switch to the PCS Page (Page 0)
                //ps. For 5838F only

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x18, 0x67c7);
                MP_WritePhyUshort(Adapter, 0x04, 0x2000);
                MP_WritePhyUshort(Adapter, 0x03, 0x002f);
                MP_WritePhyUshort(Adapter, 0x02, 0x4360);
                MP_WritePhyUshort(Adapter, 0x01, 0x0109);
                MP_WritePhyUshort(Adapter, 0x00, 0x3022);
                MP_WritePhyUshort(Adapter, 0x04, 0x2800);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 4: frequency offset coefficients
            //Solution: disable freeze cb0 & fix ste2 state machine as above
            //Parameter:
            //801f0001		//page 1
            //8010f41b		
            //8014fb54 	
            //8018f5c7 	 
            //801f0000		//page 1
            //ps. For 5838A~E

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x10, 0xf41b);
            MP_WritePhyUshort(Adapter, 0x14, 0xfb54);
            MP_WritePhyUshort(Adapter, 0x18, 0xf5c7);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

            /*
            //Issue 5: INRX hang with 100M switch
            //Solution: freeze cb0
            //Parameter:
            //801f0001
            //80153500 // reduce the eye-open snr threshold 
            //8010f41b (link down)
            //8010f01b (link up)
            //801f0000
            //ps. For 5838A~E

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x15, 0x3500);
            MP_WritePhyUshort(Adapter, 0x10, 0xf41b);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            */

            //Issue 6: Can not link to 1Gbps with bad cable (100M made in China)
            //Solution: Decrease SNR threshold form 21.07dB to 19.04dB 
            //Parameter:
            //801f0001  (page1) 
            //80170cc0 
            //Driver: XP, Vista, Linux, ODI ,PXE, MP LAN
            //IC Version: 5838A~G

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x17, 0x0cc0);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

            /*
            // Issue 7: with 0xf41b , gigabyte 120M could not link up

            if (Adapter->SubVendorID == 0x1458 && Adapter->SubSystemID == 0xE000) {

                //only for gigabyte
                //Issue 7: Free cb0 for gigabyte xfmr issue
                //Solution: Free cb0 for gigabyte xfmr issue, only with SSID/VID = 10EC8167
                //Parameter:
                //801f0001		//page 1
                //8010f01b		
                //801f0000		//page 0
                //ps. For 5838A~E,  only with SSID/VID = 10EC8167
                //Driver: XP, Vista, ODI

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x10, 0xf01b);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }
            */

            break;

        case RTL8168B:
            TmpUlong = ReadRegister(TCR);
            TmpUlong &= 0xfC800000;

            if (TmpUlong==0x20000000) {
                // 68				
                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x06, 0x00aa);
                MP_WritePhyUshort(Adapter, 0x07, 0x3173);
                MP_WritePhyUshort(Adapter, 0x08, 0x08fc);
                MP_WritePhyUshort(Adapter, 0x09, 0xe2d0);
                MP_WritePhyUshort(Adapter, 0x0B, 0x941a);
                MP_WritePhyUshort(Adapter, 0x18, 0x65fe);
                MP_WritePhyUshort(Adapter, 0x1c, 0x1e02);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);
                MP_WritePhyUshort(Adapter, 0x07, 0x103e);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

            } else {

                // 68B	
                // SJY
                /*
                Sometimes Rx CRC error with long cable (130M~150M)
                Solution: ADC clipping because initial aagc delta too high 
                Level down delta of aagc and set PGA upper bound.
                */				

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x0B, 0x94b0);				
                MP_WritePhyUshort(Adapter, 0x1f, 0x0003);
                MP_WritePhyUshort(Adapter, 0x12, 0x6096);						 
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);		
            }

            //ISSUE: 2-paired wired can not link on giga-bit mode
            //SOLUTION: PHY will automatically speed-down 
            //when restart n-way failed 7 times

            MP_WritePhyUshort(Adapter, 0x0d, 0xf8a0);
            break;

        case RTL8136_REV_E:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:

            // SJY
            /*
            Sometimes Rx CRC error with long cable (130M~150M)
            Solution: ADC clipping because initial aagc delta too high 
            Level down delta of aagc and set PGA upper bound.
            */		

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x0B, 0x94b0);		
            MP_WritePhyUshort(Adapter, 0x1f, 0x0003);
            MP_WritePhyUshort(Adapter, 0x12, 0x6096);						 
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            break;            

        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:	
        case RTL8168C_REV_M:

            //Issue 1: Can not link with short cable @100Mbps
            //Solution: Modify Cb1 and Cb2 initial value
            //Parameter:
            //801f0001  (page 1)  ( for version B / F/ G / H / J/K/M)
            //80122300  (Cb2 initial value from 0x29 to 0x00)
            //801f0003  (page 3)  ( for Ver F/G/H/J/K/M)
            //80160F0A  
            //801f0000  (page 0)

            //801f0003  (page 3)  (Only for Ver B)
            //8016000A  (Cb1 initial value from 0x23 to 0x0A)
            //801f0000  (page 0)
            //Driver: XP, Vista, Linux, ODI ,PXE

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x12, 0x2300);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            if (Adapter->ChipType == RTL8168C_REV_G ||
                Adapter->ChipType == RTL8168C_REV_K ||
                Adapter->ChipType == RTL8168C_REV_M) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0003);
                MP_WritePhyUshort(Adapter, 0x16, 0x0f0a);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

            } else if(Adapter->ChipType == RTL8168C) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0003);
                MP_WritePhyUshort(Adapter, 0x16, 0x000a);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 2: PGA upper bound is not enough for ADC input range 0x100
            //Solution: Increase PGA upper bound
            //Parameter:
            //801f0003  (page 3)  (Only for VerB)
            //8012C096  (PGA upper bound)
            //801f0000  (page 0)
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (for Ver. B)

            if (Adapter->ChipType == RTL8168C) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0003);	
                MP_WritePhyUshort(Adapter, 0x12, 0xc096);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 3: Increase amplitude
            //Solution: Increase bandgap voltage and Amplitude
            //Parameter:
            //801f0002  (page2)  
            //800088dE, 800182B1
            //801f0000  
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (for Ver. B/F/G/H/J/K/M)

            MP_WritePhyUshort(Adapter, 0x1f, 0x0002);
            MP_WritePhyUshort(Adapter, 0x00, 0x88de);
            MP_WritePhyUshort(Adapter, 0x01, 0x82b1);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

            //Issue 4: 50 Ohm calibration
            //Solution: modify R calibration
            //Parameter:
            //801f0002  (page2)  (Only VerB)
            //80089E30, 800901F0
            //801f0000  
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (Only VerB)

            if (Adapter->ChipType == RTL8168C) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);	
                MP_WritePhyUshort(Adapter, 0x08, 0x9e30);  
                MP_WritePhyUshort(Adapter, 0x09, 0x01f0);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 5: Qualdin periodically pull high in standby mode
            //Solution: Increase LD bias in standby mode
            //Parameter:
            //801f0002  (page2)  (Only VerB)
            //800A5500 
            //801f0000  
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (Only VerB)

            if (Adapter->ChipType == RTL8168C) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);	
                MP_WritePhyUshort(Adapter, 0x0a, 0x5500);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 6: 10M Tx cause link partner Rx error
            //Solution: change clock trigger edge for 10M
            //Parameter:
            //801f0002  (page2)  
            //80037002 
            //801f0000  
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (Only VerB)

            if (Adapter->ChipType == RTL8168C) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);	
                MP_WritePhyUshort(Adapter, 0x03, 0x7002);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 7: Switching regulator default output voltage
            //Solution: Because bandgap voltage has increased one step, we need to decrease switching regulator output voltage for one step
            //Parameter:
            //801f0002  (page2)  (for Ver. F/G/H/J/K/M)
            //800C7EB8 
            //801f0000

            //801f0002  (page2)  (for Ver.B)
            //800C00C8 
            //801f0000  
            //Driver: XP, Vista, Linux, ODI ,PXE

            if (Adapter->ChipType == RTL8168C_REV_G ||
                Adapter->ChipType == RTL8168C_REV_K ||
                Adapter->ChipType == RTL8168C_REV_M) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);	
                MP_WritePhyUshort(Adapter, 0x0c, 0x7eb8);  
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);

            } else if(Adapter->ChipType == RTL8168C) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);	
                MP_WritePhyUshort(Adapter, 0x0c, 0x00c8);  
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue10: TX Reliability issue
            //Solution: Increase Iq of line driver output stage
            //Parameter:
            //801f0002  (page2) 
            //80060761   (Ver2.3 2008.9.9)
            //801f0000  
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (for Ver. F/G/H/J/K)
            //Initial setting

            //Solution: Decrease Iq of line driver output stage (Fix �n��MP LAN RMA issue)
            //Parameter:
            //801f0002  (page2) 
            //80065461   (Ver2.6 2009.5.21)
            //801f0000  
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (only for Ver. M)
            //Initial setting

            if (Adapter->ChipType == RTL8168C_REV_G ||
                Adapter->ChipType == RTL8168C_REV_K ) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);	
                MP_WritePhyUshort(Adapter, 0x06, 0x0761);  
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            if (Adapter->ChipType == RTL8168C_REV_M) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);	
                MP_WritePhyUshort(Adapter, 0x06, 0x5461);  
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue11: AGC Clipping issue
            //Solution: change state machine
            //Parameter:
            //801f0001  (page1)  
            //8003802f
            //80024f02
            //80010409
            //8000f099
            //80049800
            //80049000
            //801f0000 
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (for Ver. F/G/H/J)

            if (Adapter->ChipType == RTL8168C_REV_G) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x03, 0x802f);
                MP_WritePhyUshort(Adapter, 0x02, 0x4f02);
                MP_WritePhyUshort(Adapter, 0x01, 0x0409);
                MP_WritePhyUshort(Adapter, 0x00, 0xf099);
                MP_WritePhyUshort(Adapter, 0x04, 0x9800);
                MP_WritePhyUshort(Adapter, 0x04, 0x9000);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue12: Enable ALDPS
            //Solution: Verify OK, LDPS default on
            //Parameter:
            //801f0000  (page0)  (for Ver. F/G/H/J/K/M)
            //Reg22  Bit 0 : set to 1
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (for Ver. F/G/H/J/K/M)

            if (Adapter->ChipType == RTL8168C_REV_G ||
                Adapter->ChipType == RTL8168C_REV_K ||
                Adapter->ChipType == RTL8168C_REV_M) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x16);
                PhyRegValue |= BIT_0;
                MP_WritePhyUshort(Adapter, 0x16, PhyRegValue);
            }

            //Issue13: Enable Speed Down Function
            //Solution: Verify OK, Enable Speed Down
            //Parameter:
            //801f0000  (page0)  (for Ver. B/ F / G/ H/ J/ K/ M)
            //Reg 20  Bit 5 : set to 1   (enable two pair speed down)
            //Reg 13  Bit 5:  set to 0  //800DF880 
            //(Disable retry speed down feature for E-gun 10Mbps couple issue)  (2009.1.5  V2.5 )
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (for Ver. B/ F/G/H/J/K/M)

            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            PhyRegValue = MP_ReadPhyUshort(Adapter, 0x14);
            PhyRegValue |= BIT_5;
            MP_WritePhyUshort(Adapter, 0x14, PhyRegValue);
            PhyRegValue = MP_ReadPhyUshort(Adapter, 0x0d);
            PhyRegValue &= ~(BIT_5);
            MP_WritePhyUshort(Adapter, 0x0d, PhyRegValue);

            //Issue14: Delta_b tuning for Fnet Long Lines
            //Solution: Cb0_i is not close to the converged value owing to the cost down of update engine so that dalta_b has to be tuned
            //Parameter:
            //801f0001  (page1)  (for Ver. F/G/H/J/K/M)
            //801d3d98
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (for Ver. F/G/H/J/K/M)

            if (Adapter->ChipType == RTL8168C_REV_G ||
                Adapter->ChipType == RTL8168C_REV_K ||
                Adapter->ChipType == RTL8168C_REV_M) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x1d, 0x3d98);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue16: Can not link to 1Gbps with bad cable (100M made in China)
            //Solution: Decrease SNR threshold form 21.07dB to 19.04dB 
            //Parameter:
            //801f0001  (page1) 
            //80170cc0 
            //Driver: XP, Vista, Linux, ODI ,PXE, MP LAN
            //IC Version: (for Ver. F/G/H/J/K/M)

            if (Adapter->ChipType == RTL8168C_REV_G ||
                Adapter->ChipType == RTL8168C_REV_K ||
                Adapter->ChipType == RTL8168C_REV_M) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x17, 0x0cc0);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            break;

        case RTL8168CP:
        case RTL8168CP_REV_B:	
        case RTL8168CP_REV_C:	
        case RTL8168CP_REV_D:
            //Issue 1: Giga Slave Tx cause link partner Rx error
            //Solution: change Digital / Analog interface sample clock edge
            //Setup timing: after booting, write this Parameter before nway:
            //801f0000  (page0)
            //801D0F00 
            //801f0000  
            //Driver: XP, Vista, Linux,PXE,ODI 
            //IC Version: Ver.A

            if (Adapter->ChipType == RTL8168CP) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
                MP_WritePhyUshort(Adapter, 0x1d, 0x0f00);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 2: Switching regulator default output voltage
            //Solution: because bandgap voltage has increased one step, we need to decrease switching regulator output voltage for one step
            //Setup timing: after booting, write this Parameter before nway:
            //Parameter:
            //801f0002  (page2)
            //800C1EC8 
            //801f0000  
            //Driver: XP, Vista, Linux, PXE, ODI 
            //IC Version: Ver.A

            if (Adapter->ChipType == RTL8168CP) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);	
                MP_WritePhyUshort(Adapter, 0x0c, 0x1ec8);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000); 
            }

            //Issue3: Delta_b tuning for Fnet Long Lines
            //Solution: Cb0_i is not close to the converged value owing to the cost down of update engine so that dalta_b has to be tuned
            //Setup timing: after booting, write this Parameter before nway:
            //Parameter:
            //801f0001  (page1)  (for Ver. A/B)
            //801d3d98
            //Driver: XP, Vista, Linux, PXE, ODI 
            //IC Version: Ver.A/B/C

            if (Adapter->ChipType == RTL8168CP ||
                Adapter->ChipType == RTL8168CP_REV_B ||
                Adapter->ChipType == RTL8168CP_REV_C) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x1d, 0x3d98);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 4: Enable Speed Down Function
            //Setup timing: before nway
            //Parameter:
            //801f0000  (page0)  Reg 20  Bit 5 : set to 1   (enable two pair speed down)
            //Reg 13  Bit [5]:  set to 0  (Disable retry speed down feature for E-gun 10Mbps couple issue)  (2009.3.17  V1.7 )
            //Driver: XP, Vista, Linux, PXE, ODI 
            //IC Version: Ver.A/B/C/D

            if (Adapter->ChipType == RTL8168CP ||
                Adapter->ChipType == RTL8168CP_REV_B ||
                Adapter->ChipType == RTL8168CP_REV_C ||
                Adapter->ChipType == RTL8168CP_REV_D) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x14);
                PhyRegValue |= BIT_5;
                MP_WritePhyUshort(Adapter, 0x14, PhyRegValue);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x0d);
                PhyRegValue &= ~(BIT_5);
                MP_WritePhyUshort(Adapter, 0x0d, PhyRegValue);    
            }

            //Issue 6: No link with bad cable (100M made in China)
            //Solution: Decrease SNR threshold form 21.07dB to 19.04dB 
            //Parameter:
            //801f0001  (page1) 
            //80170cc0 
            //Driver: XP, Vista, Linux, ODI ,PXE, MP LAN
            //IC Version: Ver.A/B/C/D

            if (Adapter->ChipType == RTL8168CP ||
                Adapter->ChipType == RTL8168CP_REV_B ||
                Adapter->ChipType == RTL8168CP_REV_C ||
                Adapter->ChipType == RTL8168CP_REV_D) {

                MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
                MP_WritePhyUshort(Adapter, 0x17, 0x0cc0);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

            //Issue 7: Enable ALDPS
            //Solution:Default Enable ALDPS
            //Parameter:
            //801f0000  (page0)  
            //Reg22  Bit[0] : set to 1
            //Driver: XP, Vista, Linux, ODI ,PXE
            //IC Version: (for Ver. D)
            //Initial setting

            if (Adapter->ChipType == RTL8168CP_REV_D) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x16);
                PhyRegValue |= BIT_0;
                MP_WritePhyUshort(Adapter, 0x16, PhyRegValue);
            }

            //Issue 8: DC mode power saving 
            //Parameter:
            //801f0002  (page2)  
            //DC
            //Reg.12 Bit[4]=0  //1.13V
            //AC
            //Reg.12 Bit[4]=1
            //801f0000  (page0)  
            //Driver: XP, Vista, Linux, 
            //IC Version: (for Ver. D)

            if (Adapter->ChipType == RTL8168CP_REV_D) {
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x0C);
                PhyRegValue |= BIT_4;
                MP_WritePhyUshort(Adapter, 0x0C, PhyRegValue);
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            }

        break;

        case RTL8136:

            // 8136
            // SJY
            /*
            Sometimes Rx CRC error with long cable (130M~150M)
            Solution: ADC clipping because initial aagc delta too high 
            Level down delta of aagc and set PGA upper bound.
            */				

            MP_WritePhyUshort(Adapter, 0x1f, 0x0001);
            MP_WritePhyUshort(Adapter, 0x0B, 0x94b0);
            MP_WritePhyUshort(Adapter, 0x1f, 0x0003);
            MP_WritePhyUshort(Adapter, 0x12, 0x6096);						 
            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);	
            break;

        case RTL8101:
        case RTL8101_REV_C:

            //Issue 2: Advance link down power saving mode function 
            //Solution: 
            //Parameter:
            //801f0000  (page 0)
            //Reg.17 Bit12=1  (Enable LDPS )

            MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
            PhyRegValue = MP_ReadPhyUshort(Adapter, 0x11);
            PhyRegValue |= BIT_12;
            MP_WritePhyUshort(Adapter, 0x11, PhyRegValue);
            break;

        default:
            //do nothing
            break;
    }

    //in case forget to switch phy to page 0
    MP_WritePhyUshort(Adapter, 0x1F, 0x0000);

    PhyRegDataGBCR=MP_ReadPhyUshort(Adapter, PHY_REG_GBCR);
    PhyRegDataANAR=MP_ReadPhyUshort(Adapter, PHY_REG_ANAR);
    PhyRegDataANAR |= (ANAR_10_HALF | ANAR_10_FULL | ANAR_100_HALF | ANAR_100_FULL);
    MP_WritePhyUshort(Adapter, PHY_REG_ANAR, PhyRegDataANAR);
    PhyRegDataGBCR |= (GBCR_1000_FULL | GBCR_1000_HALF );
    MP_WritePhyUshort(Adapter, PHY_REG_GBCR, PhyRegDataGBCR);

    //
    // NWAY restart
    //

    MP_WritePhyUshort(Adapter,
                      PHY_REG_BMCR,
                      MDI_CR_AUTO_SELECT | MDI_CR_RESTART_AUTO_NEG);

    //
    // Wait up to 3.5 seconds for auto-negotiation to complete.
    //

    Status = STATUS_UNSUCCESSFUL;
    for (Wait = 0; Wait < PHY_MAX_AUTO_NEGOTIATE; Wait += 1) {

        //
        // MP_ReadPhyUshort takes a minimum of 25 usec per call.
        //

        PhyRegData=MP_ReadPhyUshort(Adapter, PHY_REG_BMSR);

        //
        // Check if Auto-Negotiation is complete (bit 5, reg 1).
        //

        if ((PhyRegData & MDI_SR_AUTO_NEG_COMPLETE)) {
            Status = STATUS_SUCCESS;
            break;
        }

        KeStallExecutionProcessor(25);
    }

    if (!NT_SUCCESS(Status)) {
       // KdNetErrorString = L"NIC auto-negotiation timed out.";
        goto InitializePhysicalLayerEnd;
    }

    PhyRegData=MP_ReadPhyUshort(Adapter, PHY_REG_BMSR);

    //
    // If there is a valid link
    //

    if (PhyRegData & MDI_SR_LINK_STATUS) {
        PhyRegDataANER=MP_ReadPhyUshort(Adapter, PHY_REG_ANER);
        PhyRegDataBMSR=MP_ReadPhyUshort(Adapter, PHY_REG_BMSR);

        //
        // If a True NWAY connection was made, then we can detect speed/duplex by
        // ANDing our adapter's advertised abilities with our link partner's
        // advertised ablilities, and then assuming that the highest common
        // denominator was chosed by NWAY.
        //
        // Realtek MAC done this, driver directly read the speed/duplex directly from MAC


        if ((PhyRegDataANER & ANER_LP_AUTO_NEG_ABLE) &&
            (PhyRegDataBMSR & MDI_SR_AUTO_NEG_COMPLETE)) {

            Adapter->NwayLink=TRUE;
            Adapter->ParallelLink=FALSE;
        }

        //
        // If we are connected to a non-NWAY repeater or hub, and the line
        // speed was determined automatically by parallel detection
        // Realtek MAC can know speed/dulex when a non-NWAY hub connected

        if (!(PhyRegDataANER & ANER_LP_AUTO_NEG_ABLE)) {
            Adapter->NwayLink=FALSE;
            Adapter->ParallelLink=TRUE;
            //SetupPhyForceMode(Adapter);      // Exit with Link Path
        }

        PhyStatus=ReadRegister(PhyStatus);
        if (PhyStatus & PHY_STATUS_1000MF) {
            Adapter->KdNet->LinkSpeed = 1000;

        } else if (PhyStatus & PHY_STATUS_100M) {
            Adapter->KdNet->LinkSpeed = 100;

        } else if (PhyStatus & PHY_STATUS_10M) {
            Adapter->KdNet->LinkSpeed = 10;
        }

        //
        // Get current duplex setting -- if bit is set then FDX is enabled
        //

        Adapter->KdNet->LinkDuplex = FALSE;
        if (PhyStatus & PHY_STATUS_FULL_DUPLEX)	{
            Adapter->KdNet->LinkDuplex = TRUE;
        }

        Status = STATUS_SUCCESS;

    } else {
        Adapter->NwayLink=FALSE;
        Adapter->ParallelLink=FALSE;
        Adapter->KdNet->LinkSpeed = 10;
        Adapter->KdNet->LinkDuplex = FALSE;
        Status = STATUS_UNSUCCESSFUL;
    //    KdNetErrorString = L"No valid link.  The network cable might not be properly connected.";
    }

InitializePhysicalLayerEnd:
    return Status;
}

NTSTATUS
RealtekInitializeController(
    __in PKDNET_SHARED_DATA KdNet
    )

/*++

Routine Description:

    This function initializes the Network controller.  The controller is setup
    to send and recieve packets at the fastest rate supported by the hardware
    link.  Packet send and receive will be functional on successful exit of
    this routine.  The controller will be initialized with Interrupts masked
    since all debug devices must operate without interrupt support.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

Return Value:

    STATUS_SUCCESS on successful initialization.  Appropriate failure code if
    initialization fails.

--*/

{
    PREALTEK_ADAPTER Adapter;
    PHYSICAL_ADDRESS    physAddr;
    ULONG Index;
    NTSTATUS Status;

    Adapter = (PREALTEK_ADAPTER)KdNet->Hardware;
    Adapter->KdNet = KdNet;

    //
    // Immediately fail if the device does not have a Realtek vendor ID.
    // This check MUST NOT be removed, so that shared boot and hyper-v
    // debugging work correctly.
    //

    if ((Adapter->KdNet->Device->BaseClass != PCI_CLASS_NETWORK_CTLR) ||
        (Adapter->KdNet->Device->VendorID != PCI_VID_REALTEK)) {

        Status = STATUS_NO_SUCH_DEVICE;
     //   KdNetErrorString = L"Not a Realtek network controller.";
        goto InitializeControllerEnd;
    }

    //
    // Find the base address for the controller.  We scan through the BARs
    // until we find a valid memory address range that has been mapped
    // with a virtual address.  We must scan through the BARs because
    // different devices use different BARs to map their registers.
    //

    Status = STATUS_UNSUCCESSFUL;
    for (Index = 0; Index < PCI_TYPE0_ADDRESSES; Index += 1) {
        if (Adapter->KdNet->Device->BaseAddress[Index].Valid == FALSE) {
            continue;
        }

        if (Adapter->KdNet->Device->BaseAddress[Index].Type != CmResourceTypeMemory) {
            continue;
        }

        if (Adapter->KdNet->Device->BaseAddress[Index].TranslatedAddress != NULL) {
            Adapter->BaseAddress = 
                (PCSR_STRUC)Adapter->KdNet->Device->BaseAddress[Index].TranslatedAddress;
            Status = STATUS_SUCCESS;
            break;
        }
    }

    if (!NT_SUCCESS(Status)) {
       // KdNetErrorString = L"No valid device register BaseAddress found.";
        goto InitializeControllerEnd;
    }

    Status = HardwareVersionSupported(Adapter);
    if (!NT_SUCCESS(Status)) {
        goto InitializeControllerEnd;
    }
    KdStubDbgPrint("HardwareVersionSupported Passed\n");
 
    for(;;)
    {

    }
    //
    // Patch various hardware settings.
    //

    Status = PatchHardware(Adapter);
    if (!NT_SUCCESS(Status)) {
        goto InitializeControllerEnd;
    }

    //
    // Modify PCI config space for certain NICs.
    //

    SetupPCIConfigSpace(Adapter);

    //
    // Reset the NIC and disable transmit and receive.
    //

    Status = ResetAdapter(Adapter);
    if (!NT_SUCCESS(Status)) {
        goto InitializeControllerEnd;
    }

    //
    // Mask all interrupts from the NIC.
    //

    WriteRegister(IMR, 0);

    //
    // Initialize the physical layer.
    //

    Status = InitializePciExpressPhy(Adapter);
    if (!NT_SUCCESS(Status)) {
        goto InitializeControllerEnd;
    }

    //
    // Do not set the 8168B chips to 1 GBit/sec speeds until restart on Intel
    // prototype machines is fixed to power off the chip, since otherwise the
    // NIC fails initialization after reboot - as its hardware ID changes to
    // 8169.  Assume a speed of 100Mb/s and a full duplex connection for cards
    // which do not get their PHY initialized.
    //

    switch (Adapter->ChipType) {
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
            Status = InitializePhysicalLayer(Adapter);
            if (!NT_SUCCESS(Status)) {
                goto InitializeControllerEnd;
            }

            break;

        default:
            Adapter->KdNet->LinkSpeed = 100;
            Adapter->KdNet->LinkDuplex = TRUE;
            break;
    }

    //
    // Configure C+CR register.  MUST BE DONE FIRST!
    // Disable PCI Multiple R/W Enable - bit 3
    // Disable 64 bit PCI addressing - bit 4
    // Enable receive checksum offload - bit 5
    // Enable Receive VLAN detagging - bit 6
    //

    WriteRegister(CPCR, 0x060);

    //
    // Clear the Interrupt status register.
    //

    WriteRegister(ISR, ReadRegister(ISR));

    //
    // Set iMode for RTL8169SB
    //

    if (Adapter->ChipType == RTL8169SB) {
        //SetConfigBit(Adapter, NIC_CONFIG4_REG_OFFSET, CONFIG4_iMode);
    }

    //
    // Initialize the transmit and receive indices.
    //

    Adapter->TxIndex = 0;
    Adapter->RxIndex = 0;

    //
    // Read the hardware MAC address.
    //

    Adapter->KdNet->TargetMacAddress[0] = ReadRegister(ID0);
    Adapter->KdNet->TargetMacAddress[1] = ReadRegister(ID1);
    Adapter->KdNet->TargetMacAddress[2] = ReadRegister(ID2);
    Adapter->KdNet->TargetMacAddress[3] = ReadRegister(ID3);
    Adapter->KdNet->TargetMacAddress[4] = ReadRegister(ID4);
    Adapter->KdNet->TargetMacAddress[5] = ReadRegister(ID5);

    //
    // Set the maximum transmit packet size.
    //

    WriteRegister(MTPS, MAX_TX_PKT_SIZE);

    //
    // Initialize the transmit descriptors.  Only the first 128 will be used.
    // Mark each as pointing to a complete packet, and then mark the last one
    // as the last in the ring of descriptors.  The unused 128 TX descriptors
    // are left zeroed out.
    //

    for (Index = 0; Index < NUM_TX_DESC; Index++) {
        Adapter->txDesc[Index].BufferAddress = KdGetPhysicalAddress(&Adapter->txBuffers[Index]);
        Adapter->txDesc[Index].VLAN_TAG.Value = 0;
        Adapter->txDesc[Index].TAGC = 0;
        Adapter->txDesc[Index].length = 0;
        Adapter->txDesc[Index].status = TXS_LS | TXS_FS;
    }

    Adapter->txDesc[NUM_TX_DESC - 1].status |= TXS_EOR;

    //
    // Write the TX descriptor base physical address into the NIC.
    //

    physAddr = KdGetPhysicalAddress(&Adapter->txDesc[0]);
    WriteRegister(TNPDSLow, physAddr.LowPart);
    WriteRegister(TNPDSHigh, physAddr.HighPart);

    //
    // Initialize the receive descriptors.  The RX buffer physical addresses
    // will never change.
    //

    for (Index = 0; Index < NUM_RX_DESC; Index++)
    {
        Adapter->rxDesc[Index].BufferAddress = KdGetPhysicalAddress(&Adapter->rxBuffers[Index]);
        Adapter->rxDesc[Index].VLAN_TAG.Value = 0;
        Adapter->rxDesc[Index].TAVA = 0;
        Adapter->rxDesc[Index].length = REALTEK_MAX_PKT_SIZE;
        Adapter->rxDesc[Index].CheckSumStatus = 0;
        Adapter->rxDesc[Index].status = RXS_OWN;
    }

    Adapter->rxDesc[NUM_RX_DESC-1].status |= RXS_EOR;

    //
    // Write the RX descriptor base physical address into the NIC.
    //

    physAddr = KdGetPhysicalAddress(&Adapter->rxDesc[0]);
    WriteRegister(RDSARLow, physAddr.LowPart);
    WriteRegister(RDSARHigh, physAddr.HighPart);

    //
    // Set the maximum receive packet size.
    //

    WriteRegister(RMS, REALTEK_MAX_PKT_SIZE);

    //
    // Configure the receive control register.  Set to recieve ALL packets.
    //

    WriteRegister(RCR, (TCR_RCR_MXDMA_UNLIMITED << RCR_MXDMA_OFFSET) |
                       (RCR_RX_NO_THRESHOLD << RCR_FIFO_OFFSET) |
                       RCR_AR | RCR_AB | RCR_APM | RCR_AAP);

    //
    // Enable transmit and receive.
    //

    WriteRegister(CmdReg, CR_RE | CR_TE);

    //
    // Configure the transmit control register.
    // Setup transmit for unlimited DMA length, standard interframe gap timing
    // and append a CRC after the packet (CRC bit written with zero).
    //

    WriteRegister(TCR, (TCR_RCR_MXDMA_UNLIMITED << TCR_MXDMA_OFFSET) |
                       (TCR_IFG0 | TCR_IFG1));

InitializeControllerEnd:
    return Status;
}

VOID
RealtekShutdownController (
    __in PREALTEK_ADAPTER Adapter
    )

/*++

Routine Description:

    This function shuts down the Network controller.  No further packets can
    be sent or received until the controller is reinitialized.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

Return Value:

    None.

--*/

{

    //
    // Turn off transmit and receive.
    //

    WriteRegister(CmdReg, 0);

}

#pragma warning(default:4127 4310)
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

