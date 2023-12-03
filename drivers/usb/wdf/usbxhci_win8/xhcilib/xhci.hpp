/*
 * PROJECT:         ReactOS xHCI Driver
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Resource definitions
 * COPYRIGHT:       Copyright 2023 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <drivers/usbport/usbmport.h>

#include "xhcihardware.hpp"

class XHCI
{
public:
	static	NTSTATUS			NoOp();

								XHCI(PULONG LocBaseIoAddress,
                                     PULONG LocOperationalRegs,
                                     PULONG LocRunTimeRegisterBase,
                                     PULONG LocDoorBellRegisterBase,
                                     USHORT LocPageSize,
                                     USHORT LocMaxScratchPadBuffers
                                        );
								~XHCI();

    inline void                 WriteOpReg(UINT32 reg, UINT32 value);
    inline  UINT32              ReadOpReg(UINT32 reg);
    inline  UINT32              ReadCapReg32(ULONG32 reg);
    inline  void                WriteCapReg32(ULONG32 reg, ULONG32 value);
	inline	UINT32				ReadDoorReg32(ULONG32 reg);
	inline	void				WriteDoorReg32(ULONG32 reg, ULONG32 value);
    inline  UINT32              ReadRunReg32(ULONG32 reg);
    inline  void                WriteRunReg32(ULONG32 reg, ULONG32 value);

    inline NTSTATUS             WaitOpBits(ULONG32 reg, ULONG32 mask, ULONG32 expected);


    NTSTATUS                    ControllerHalt();
    NTSTATUS                    ControllerReset();

    /* reactos specific */
    
    UINT32                      GetNumOfPorts();
private:
	/* hehehehehehehehehheeheheh - kekw */
 	PULONG BaseIoAddress; /* mapped registers*/
    PULONG OperationalRegs;
    PULONG RunTimeRegisterBase;
    PULONG DoorBellRegisterBase;
    UCHAR FrameLengthAdjustment;
    USHORT HcSystemErrors;
    ULONG PortRoutingControl;
    USHORT NumberOfPorts; // HCSPARAMS1 => N_PORTS
    USHORT SlotCount;
    USHORT PortPowerControl; // HCSPARAMS => Port Power Control (PPC)
    USHORT PageSize;
    USHORT MaxScratchPadBuffers;
    PMDL ScratchPadArrayMDL;
    PMDL ScratchPadBufferMDL;
    PXHCI_HC_RESOURCES HcResourcesVA;
    PHYSICAL_ADDRESS HcResourcesPA;

    /* Haiku specific */

    ULONG32 OperationalRegisterOffset;
    ULONG32 RuntimeRegisterOffset;
    ULONG32 DoorbellRegisterOffset;

    usb_speed			PortSpeeds[XHCI_MAX_PORTS];
};
