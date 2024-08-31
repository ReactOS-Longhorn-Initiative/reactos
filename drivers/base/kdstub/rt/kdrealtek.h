/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    kdrealtek.h

Abstract:

    Realtek Network Kernel Debug Extensibility header.

Author:

    Joe Ballantyne (joeball)

--*/

#define PCI_VID_REALTEK 0x10ec
#define ALLOCATED_DESCRIPTORS 256

//
// Although 256 TX descriptors will be allocated only 128 will be used.
// Realtek hw requires the start of the descriptors be 256 byte aligned.
// TX descriptors are 16 bytes so the number of TX descriptors must be a
// multiple of 16 so that the RX descriptor block starts on a 256 byte
// boundary.
//

#define NUM_TX_DESC 128

//
// All 256 RX descriptors will be used.
//

#define NUM_RX_DESC ALLOCATED_DESCRIPTORS
#define REALTEK_MAX_PKT_SIZE 2048

//
// Set the hardware to its maximum possible TX packet size.
// For the RTL8169 this value counts 32 byte chunks, so the max packet
// size is 0x3f * 32 = 2016 bytes.  For newer NICs this value counts
// 128 byte chunks, so the max size is 0x3f * 128 = 8064 bytes.
//

#define MAX_TX_PKT_SIZE        0x3f

typedef struct _REALTEK_PKT_BUFF
{
    union
    {
        unsigned char   buffer [REALTEK_MAX_PKT_SIZE];
    } u;
} REALTEK_PKT_BUFF, *PREALTEK_PKT_BUFF;

#pragma pack(1)

//-------------------------------------------------------------------------
// Control/Status Registers (CSR)
//-------------------------------------------------------------------------
typedef struct _CSR_STRUC {
    UCHAR       ID0;                // 0x00  
    UCHAR       ID1;                // 0x01
    UCHAR       ID2;                // 0x02
    UCHAR       ID3;                // 0x03
    UCHAR       ID4;                // 0x04
    UCHAR       ID5;                // 0x05
    USHORT      RESV_06_07;         // 0x06
    UCHAR       MulticastReg0;      // 0x08
    UCHAR       MulticastReg1;      // 0x09
    UCHAR       MulticastReg2;      // 0x0a
    UCHAR       MulticastReg3;      // 0x0b
    UCHAR       MulticastReg4;      // 0x0c
    UCHAR       MulticastReg5;      // 0x0d
    UCHAR       MulticastReg6;      // 0x0e
    UCHAR       MulticastReg7;      // 0x0f
    ULONG       DTCCRLow;           // 0x10
    ULONG       DTCCRHigh;          // 0x14
    
    ULONG       RESV_18_19_1A_1B;   // 0x18
    ULONG       RESV_1C_1D_1E_1F;   // 0x1c

    ULONG       TNPDSLow;           // 0x20
    ULONG       TNPDSHigh;          // 0x24

    ULONG       THPDSLow;           // 0x28
    ULONG       THPDSHigh;          // 0x2c

    ULONG       RESV_30_31_32_33;   // 0x30

    USHORT      RTRxDMABC;          // 0x34
    UCHAR       ERSR;               // 0x36
    UCHAR       CmdReg;             // 0x37  command - reset, rx rdy, tx rdy
    UCHAR       TPPoll;             // 0x38  tx polling - 
    UCHAR       RESV_39;
    UCHAR       RESV_3A;
    UCHAR       RESV_3B;
    USHORT      IMR;                // 0x3c
    USHORT      ISR;                // 0x3e
    ULONG       TCR;                // 0x40
    ULONG       RCR;                // 0x44
    ULONG       TimerCTR;           // 0x48
    ULONG       MPC;                // 0x4c
    UCHAR       CR9346;             // 0x50
    UCHAR       CONFIG0;            // 0x51
    UCHAR       CONFIG1;            // 0x52
    UCHAR       CONFIG2;            // 0x53
    UCHAR       CONFIG3;            // 0x54
    UCHAR       CONFIG4;            // 0x55
    UCHAR       CONFIG5;            // 0x56
    UCHAR       RESV_57;            // 0x57
    ULONG       Timer;              // 0x58
    USHORT      MultiIntSelReg;     // 0x5c
    USHORT      RESV_5E_5F;         // 0x5e
    ULONG       PhyAccessReg;       // 0x60
    ULONG       PCIConfigDataReg;   // 0x64 68 only
    ULONG       PCIConfigCmdReg;    // 0x68 68 only
    UCHAR       PhyStatus;          // 0x6c
    UCHAR       RESV_6D;            // 0x6d
    UCHAR       RESV_6E;
    UCHAR       RESV_6F;    
    ULONG       ASFDataReg;         // 0x70 
    ULONG       ASFAccessReg;                
    ULONG       BIST;                    
    ULONG       FixSwitch8169SC;    // 69SC only
    union {
        struct {
            ULONG        EPhyAccessReg;    // 68 only
        } EPhy8168;
        struct {
            UCHAR        RESV_80;
            UCHAR        RESV_81;
            UCHAR        FixSwitchPhy;
            UCHAR        RESV_83;
        } FixSwitch8169S;
    };

    ULONG       Wakeup0Low;                    
    ULONG       Wakeup0High;                        
    ULONG       Wakeup1Low;                        
    ULONG       Wakeup1High;

    ULONG       Wakeup2LDDWord0;    
    ULONG       Wakeup2LDDWord1;
    ULONG       Wakeup2LDDWord2;
    ULONG       Wakeup2LDDWord3;    

    ULONG       Wakeup3LDDWord0;
    ULONG       Wakeup3LDDWord1;
    ULONG       Wakeup3LDDWord2;
    ULONG       Wakeup3LDDWord3;   

    ULONG       Wakeup4LDDWord0;
    ULONG       Wakeup4LDDWord1;
    ULONG       Wakeup4LDDWord2;
    ULONG       Wakeup4LDDWord3;

    USHORT      CRC0;
    USHORT      CRC1;
    USHORT      CRC2;
    USHORT      CRC3;    
    USHORT      CRC4;    

    USHORT      RESV_CE_CF;    

    ULONG       RESV_D0_D1_D2_D3;        
    ULONG       RESV_D4_D5_D6_D7;        
    USHORT      RESV_D8_D9;   

    USHORT      RMS;                // 0xda  maximum recv pkt size

    ULONG       RESV_DC_DD_DE_DF;
    USHORT      CPCR;               // 0xe0  C+command, disables rx vlan detagging and chksuming
    USHORT      IntMiti;            // 0xe2
    ULONG       RDSARLow;           // 0xe4
    ULONG       RDSARHigh;          // 0xe8
    UCHAR       MTPS;               // 0xec  max transmit size
    UCHAR       RESV_ED;                                
    UCHAR       RESV_EE;                            
    UCHAR       RESV_EF;                                

    ULONG       FunctionEventReg;
    ULONG       FunctionEventMaskReg;
    ULONG       FunctionPresentStateReg;
    ULONG       FunctionForceEventReg;    
} CSR_STRUC, *PCSR_STRUC;

#pragma pack()

typedef struct _REALTEK_ADAPTER {
    TX_DESC txDesc[ALLOCATED_DESCRIPTORS];    // 1024 bytes (64 * 4 dwords)
    RX_DESC rxDesc[ALLOCATED_DESCRIPTORS];    // 1024 bytes (64 * 4 dwords)
    REALTEK_PKT_BUFF txBuffers[NUM_TX_DESC];  // 128 * 2048 bytes
    REALTEK_PKT_BUFF rxBuffers[NUM_RX_DESC];  // 256 * 2048 bytes
    GDUMP_TALLY HardwareStatistics;           // Dump Tally
    PCSR_STRUC BaseAddress;
    NIC_CHIP_TYPE ChipType;
    ULONG IsPCIExpress;
    ULONG RxIndex;
    ULONG TxIndex;
    ULONG NwayLink;
    ULONG ParallelLink;
    PKDNET_SHARED_DATA KdNet;
} REALTEK_ADAPTER, *PREALTEK_ADAPTER;

//
// kdrealtek.c
//

ULONG
RealtekGetHardwareContextSize (
    __in PDEBUG_DEVICE_DESCRIPTOR Device
);

NTSTATUS
RealtekInitializeController(
    __in PKDNET_SHARED_DATA Adapter
    );

VOID
RealtekShutdownController (
    __in PREALTEK_ADAPTER Adapter
    );

NTSTATUS
RealtekGetRxPacket (
    __in PREALTEK_ADAPTER Adapter,
    __out PULONG Handle,
    __out PVOID *Packet,
    __out PULONG Length
);

VOID
RealtekReleaseRxPacket (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle
);

NTSTATUS
RealtekGetTxPacket (
    __in PREALTEK_ADAPTER Adapter,
    __out PULONG Handle
);

NTSTATUS
RealtekSendTxPacket (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle,
    ULONG Length
);

PVOID
RealtekGetPacketAddress (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle
);

ULONG
RealtekGetPacketLength (
    __in PREALTEK_ADAPTER Adapter,
    ULONG Handle
);
