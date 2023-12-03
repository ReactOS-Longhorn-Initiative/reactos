/*
 * PROJECT:         ReactOS new xHCI Driver
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Primary XHCI Class
 * COPYRIGHT:       Copyright 2023 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhci.hpp"
#include "../usbxhci.hpp"
#include <debug.h>

#define XHCI_USBCMD           0
#define XHCI_USBSTS           1
#define XHCI_PGSZ             2
#define XHCI_DNCTRL           5
#define XHCI_CRCR             6
#define XHCI_DCBAAP           12
#define XHCI_CONFIG           14
#define XHCI_PORTSC           256
#define XHCI_HCSP1            1


static const char*
XhciConvertError(UINT32 error)
{
	switch (error) {
		case COMP_INVALID: return "Invalid";
		case COMP_SUCCESS: return "Success";
		case COMP_DATA_BUFFER: return "Data buffer";
		case COMP_BABBLE: return "Babble detected";
		case COMP_USB_TRANSACTION: return "USB transaction";
		case COMP_TRB: return "TRB";
		case COMP_STALL: return "Stall";
		case COMP_RESOURCE: return "Resource";
		case COMP_BANDWIDTH: return "Bandwidth";
		case COMP_NO_SLOTS: return "No slots";
		case COMP_INVALID_STREAM: return "Invalid stream";
		case COMP_SLOT_NOT_ENABLED: return "Slot not enabled";
		case COMP_ENDPOINT_NOT_ENABLED: return "Endpoint not enabled";
		case COMP_SHORT_PACKET: return "Short packet";
		case COMP_RING_UNDERRUN: return "Ring underrun";
		case COMP_RING_OVERRUN: return "Ring overrun";
		case COMP_VF_RING_FULL: return "VF Event Ring Full";
		case COMP_PARAMETER: return "Parameter";
		case COMP_BANDWIDTH_OVERRUN: return "Bandwidth overrun";
		case COMP_CONTEXT_STATE: return "Context state";
		case COMP_NO_PING_RESPONSE: return "No ping response";
		case COMP_EVENT_RING_FULL: return "Event ring full";
		case COMP_INCOMPATIBLE_DEVICE: return "Incompatible device";
		case COMP_MISSED_SERVICE: return "Missed service";
		case COMP_COMMAND_RING_STOPPED: return "Command ring stopped";
		case COMP_COMMAND_ABORTED: return "Command aborted";
		case COMP_STOPPED: return "Stopped";
		case COMP_LENGTH_INVALID: return "Length invalid";
		case COMP_MAX_EXIT_LATENCY: return "Max exit latency too large";
		case COMP_ISOC_OVERRUN: return "Isoch buffer overrun";
		case COMP_EVENT_LOST: return "Event lost";
		case COMP_UNDEFINED: return "Undefined";
		case COMP_INVALID_STREAM_ID: return "Invalid stream ID";
		case COMP_SECONDARY_BANDWIDTH: return "Secondary bandwidth";
		case COMP_SPLIT_TRANSACTION: return "Split transaction";

		default: return "Undefined";
	}
}

inline void
XHCI::WriteOpReg(UINT32 reg, UINT32 value)
{
	WRITE_REGISTER_ULONG((OperationalRegs + reg), value);
}


inline UINT32
XHCI::ReadOpReg(UINT32 reg)
{
	return READ_REGISTER_ULONG(OperationalRegs + reg);
}

/* Cap Register starts at base of IOSPACE */
inline ULONG32
XHCI::ReadCapReg32(ULONG32 reg)
{
	return READ_REGISTER_ULONG(BaseIoAddress + reg);
}

/* Cap Register starts at base of IOSPACE */
inline void
XHCI::WriteCapReg32(ULONG32 reg, ULONG32 value)
{
	WRITE_REGISTER_ULONG((BaseIoAddress + reg), value);
}

inline ULONG32
XHCI::ReadRunReg32(ULONG32 reg)
{
	return READ_REGISTER_ULONG(RunTimeRegisterBase + reg);
}

inline void
XHCI::WriteRunReg32(ULONG32 reg, ULONG32 value)
{
	WRITE_REGISTER_ULONG((RunTimeRegisterBase + reg), value);
}

inline ULONG32
XHCI::ReadDoorReg32(ULONG32 reg)
{
	return READ_REGISTER_ULONG(DoorBellRegisterBase + reg);
}


inline void
XHCI::WriteDoorReg32(ULONG32 reg, ULONG32 value)
{
	WRITE_REGISTER_ULONG((DoorBellRegisterBase + reg), value);
}

inline NTSTATUS
XHCI::WaitOpBits(ULONG32 reg, ULONG32 mask, ULONG32 expected)
{
	int loops = 0;
	ULONG32 value = ReadOpReg(reg);
	while ((value & mask) != expected) {
		KeStallExecutionProcessor(1000);
		value = ReadOpReg(reg);
		if (loops == 100) {

		} else if (loops > 250) {
			return 1;
		}
		loops++;
	}
	return 0;
}


NTSTATUS
XHCI::ControllerHalt()
{
	// Mask off run state
	WriteOpReg(XHCI_CMD, ReadOpReg(XHCI_CMD) & ~CMD_RUN);

	// wait for shutdown state
	if (WaitOpBits(XHCI_STS, STS_HCH, STS_HCH) != 0) {
		DPRINT1("HCH shutdown timeout\n");
		return 1;
	}
	return 0;
}

NTSTATUS
XHCI::ControllerReset()
{
    XHCI_USB_COMMAND Command;
    XHCI_USB_STATUS Status;
    LARGE_INTEGER CurrentTime = {{0, 0}};
    LARGE_INTEGER LastTime = {{0, 0}};
    //XHCI_HC_STRUCTURAL_PARAMS_1 StructuralParams_1;
    //XHCI_CONFIGURE Config;
    
    DPRINT("XHCI_InitializeHardware: function initiated\n");
    DPRINT("XHCI_InitializeHardware: ... \n");

    KeQuerySystemTime(&CurrentTime);
    CurrentTime.QuadPart += 100 * 10000;  
    
    Status.AsULONG = ReadOpReg(XHCI_USBSTS);
    ASSERT(Status.ControllerNotReady != 1); // this is needed before writing anything to the operaational or doorbell registers

    Command.AsULONG = ReadOpReg(XHCI_USBCMD);
    Command.HCReset = 1;
    WriteOpReg(XHCI_USBCMD, Command.AsULONG);
    while (TRUE)
    {
        KeQuerySystemTime(&LastTime);
        
        Command.AsULONG = ReadOpReg(XHCI_USBCMD);
       
        if (Command.HCReset != 1)
        {
            break;
        }

        if (LastTime.QuadPart >= CurrentTime.QuadPart)
        {
            DPRINT("XHCI_InitializeHardware: Software Reset failed!\n");
           // return 1;
        }
    }
    DPRINT("XHCI_InitializeHardware: Reset - OK\n");
	return 0;
}


NTSTATUS
XHCI::NoOp()
{
    /* Do nothing for now */
    return MP_STATUS_SUCCESS;
}

UINT32
XHCI::GetNumOfPorts()
{
	return NumberOfPorts;
}

XHCI::XHCI(PULONG LocBaseIoAddress,
		   PULONG LocOperationalRegs,
		   PULONG LocRunTimeRegisterBase,
		   PULONG LocDoorBellRegisterBase,
		   USHORT LocPageSize,
		   USHORT LocMaxScratchPadBuffers)
{

    XHCI_USB_COMMAND Command;
    //XHCI_USB_STATUS Status;
	BaseIoAddress = LocBaseIoAddress;
	OperationalRegs = LocOperationalRegs;
	RunTimeRegisterBase = LocRunTimeRegisterBase;
	DoorBellRegisterBase = LocDoorBellRegisterBase;

    DPRINT1("mapped registers: %X\n",     BaseIoAddress);
	DPRINT1("operational register: %X\n", OperationalRegs);
	DPRINT1("runtime register : %X\n",    RunTimeRegisterBase);
	DPRINT1("doorbell register: %X\n",    DoorBellRegisterBase);

	ULONG32 interfaceVersion = HCI_VERSION(ReadCapReg32(XHCI_HCI_VERSION));
	DPRINT1("Interface Version %X\n", interfaceVersion);

	
	// halt the host controller
	if (ControllerHalt() != STATUS_SUCCESS) {
		DPRINT1("XHCI_InitializeHardware: Controll halt failed\n");
	}
	
		// reset the host controller
	if (ControllerReset() != STATUS_SUCCESS) {
		DPRINT1("XHCI_InitializeHardware: ControllerReset failed\n");
	}
	
	DPRINT1("starting XHCI host controller\n");

	if (WaitOpBits(XHCI_STS, STS_CNR, 0) != 0) {
		DPRINT1("Start() failed STS_CNR\n");
	}

	if ((ReadOpReg(XHCI_CMD) & CMD_RUN) != 0) {
		DPRINT1("Start() warning, starting running XHCI controller!\n");
	}

	if ((ReadOpReg(XHCI_PAGESIZE) & (1 << 0)) == 0) {
		DPRINT1("controller does not support 4K page size\n");
	}

	
	// read port count from capability register
	ULONG capabilities = ReadCapReg32(XHCI_HCSPARAMS1);
    NumberOfPorts = HCS_MAX_PORTS(capabilities);
	if (NumberOfPorts == 0) {
		DPRINT1("invalid number of ports: %u\n", NumberOfPorts);

	}
	else
	{
		DPRINT1("Port Count %d\n", NumberOfPorts);
	}

	SlotCount = HCS_MAX_SLOTS(capabilities);
	if (SlotCount > XHCI_MAX_DEVICES)
		SlotCount = XHCI_MAX_DEVICES;
	WriteOpReg(XHCI_CONFIG, SlotCount);

	XHCI_HC_STRUCTURAL_PARAMS_1 StructuralParams_1;
	StructuralParams_1.AsULONG = READ_REGISTER_ULONG(BaseIoAddress + XHCI_HCSPARAMS1); // HCSPARAMS1 register

    NumberOfPorts = StructuralParams_1.NumberOfPorts;
    DPRINT1("Port Count %d\n", NumberOfPorts);

		__debugbreak();
	// find out which protocol is used for each port
	UINT32 portFound = 0;
	UINT32 cparams = READ_REGISTER_ULONG(BaseIoAddress + XHCI_HCSP1);
	UINT32 eec = 0xffffffff;
	UINT32 eecp = HCS0_XECP(cparams) << 2;
	for (; eecp != 0 && XECP_NEXT(eec) && portFound < NumberOfPorts;
		eecp += XECP_NEXT(eec) << 2) {
		eec = ReadCapReg32(eecp);
		if (XECP_ID(eec) != XHCI_SUPPORTED_PROTOCOLS_CAPID)
			continue;
		if (XHCI_SUPPORTED_PROTOCOLS_0_MAJOR(eec) > 3)
			continue;
		UINT32 temp = ReadCapReg32(eecp + 8);
		UINT32 offset = XHCI_SUPPORTED_PROTOCOLS_1_OFFSET(temp);
		UINT32 count = XHCI_SUPPORTED_PROTOCOLS_1_COUNT(temp);
		if (offset == 0 || count == 0)
			continue;
		offset--;
		for (UINT32 i = offset; i < offset + count; i++) {
			if (XHCI_SUPPORTED_PROTOCOLS_0_MAJOR(eec) == 0x3)
				PortSpeeds[i] = LOC_USB_SPEED_SUPERSPEED;
			else
				PortSpeeds[i] = LOC_USB_SPEED_HIGHSPEED;

			DPRINT1("speed for port %d is %X\n", i,
				PortSpeeds[i] == LOC_USB_SPEED_SUPERSPEED ? "super" : "high");
		}
		portFound += count;
	}
	__debugbreak();

	
	// clear interrupts & disable device notifications
	WriteOpReg(XHCI_STS, ReadOpReg(XHCI_STS));
	WriteOpReg(XHCI_DNCTRL, 0);

    /* grab port counts */

    /* Get DCBAA */

    /* Setup Scratchpad */

    /* Setup Event Ring */

    /* Setup command ring */

    /* Enable Interrupts */

    /* write controller start */
    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD);
    Command.RunStop = 1;
    WRITE_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD, Command.AsULONG);
    DPRINT1("The HC is now active\n");
}

XHCI::~XHCI()
{
    /* Do nothing for now */
}
