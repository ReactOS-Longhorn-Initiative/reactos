#ifndef _RT_EQU_H
#define _RT_EQU_H

//-------------------------------------------------------------------------
// Phy related constants
//-------------------------------------------------------------------------
/*
#define PHY_503                 0
#define PHY_100_A               0x000003E0
#define PHY_100_C               0x035002A8
#define PHY_TX_ID               0x015002A8
#define PHY_NSC_TX              0x5c002000
#define PHY_OTHER               0xFFFF

#define PHY_MODEL_REV_ID_MASK   0xFFF0FFFF
#define PARALLEL_DETECT         0
#define N_WAY                   1
*/

//
// 8169S takes longer time to establish the link
//
#define RENEGOTIATE_TIME         7 // (4200 / 600)
#define PHY_RESET_TIME        2400 // (2400 * 25)
#define PHY_MAX_AUTO_NEGOTIATE (3500000 / 50)

/*
#define CONNECTOR_AUTO          0
#define CONNECTOR_TPE           1
#define CONNECTOR_MII           2
*/

//-------------------------------------------------------------------------
// Ethernet Frame Sizes
//-------------------------------------------------------------------------
#define ETHERNET_ADDRESS_LENGTH         6
#define ETHERNET_HEADER_SIZE            14
#define MINIMUM_ETHERNET_PACKET_SIZE    60
#define MAXIMUM_ETHERNET_PACKET_SIZE    1514

#define MAX_MULTICAST_ADDRESSES         32
/*
#define TCB_BUFFER_SIZE                 0XE0 // 224
#define COALESCE_BUFFER_SIZE            2048
#define ETH_MAX_COPY_LENGTH             0x80 // 128
*/

// Make receive area 1520 for 16 bit alignment.
#define RCB_BUFFER_SIZE                 1520 // 0x5F0

//- Area reserved for all Non Transmit command blocks
//#define MAX_NON_TX_CB_AREA              512

//-------------------------------------------------------------------------
// Ndis/Adapter driver constants
//-------------------------------------------------------------------------
/*
#define MAX_PHYS_DESC                   16
#define MAX_RECEIVE_DESCRIPTORS         1024 // 0x400
#define NUM_RMD                         10
*/

//--------------------------------------------------------------------------
// System wide Equates
//--------------------------------------------------------------------------
/*
#define MAX_NUMBER_OF_EISA_SLOTS        15
#define MAX_NUMBER_OF_PCI_SLOTS         15
*/

//-------------------------------------------------------------------------
//- Miscellaneous Equates
//-------------------------------------------------------------------------
#define CR      0x0D        // Carriage Return
#define LF      0x0A        // Line Feed

#ifndef FALSE
#define FALSE       0
#define TRUE        1
#endif

#define DRIVER_NULL ((ULONG)0xffffffff)
#define DRIVER_ZERO 0

//-------------------------------------------------------------------------
// Bit Mask definitions
//-------------------------------------------------------------------------
#define BIT_0       0x0001
#define BIT_1       0x0002
#define BIT_2       0x0004
#define BIT_3       0x0008
#define BIT_4       0x0010
#define BIT_5       0x0020
#define BIT_6       0x0040
#define BIT_7       0x0080
#define BIT_8       0x0100
#define BIT_9       0x0200
#define BIT_10      0x0400
#define BIT_11      0x0800
#define BIT_12      0x1000
#define BIT_13      0x2000
#define BIT_14      0x4000
#define BIT_15      0x8000
#define BIT_16      0x010000
#define BIT_17      0x020000
#define BIT_18      0x040000
#define BIT_19      0x080000
#define BIT_20      0x0100000
#define BIT_21      0x0200000
#define BIT_22      0x0400000
#define BIT_24      0x01000000
#define BIT_28      0x10000000

#define BIT_0_2     0x0007
#define BIT_0_3     0x000F
#define BIT_0_4     0x001F
#define BIT_0_5     0x003F
#define BIT_0_6     0x007F
#define BIT_0_7     0x00FF
#define BIT_0_8     0x01FF
#define BIT_0_13    0x3FFF
#define BIT_0_15    0xFFFF
#define BIT_1_2     0x0006
#define BIT_1_3     0x000E
#define BIT_2_5     0x003C
#define BIT_3_4     0x0018
#define BIT_4_5     0x0030
#define BIT_4_6     0x0070
#define BIT_4_7     0x00F0
#define BIT_5_7     0x00E0
#define BIT_5_9     0x03E0
#define BIT_5_12    0x1FE0
#define BIT_5_15    0xFFE0
#define BIT_6_7     0x00c0
#define BIT_7_11    0x0F80
#define BIT_8_10    0x0700
#define BIT_9_13    0x3E00
#define BIT_12_15   0xF000
#define BIT_8_15    0xFF00 

#define BIT_16_20   0x001F0000
#define BIT_21_25   0x03E00000
#define BIT_26_27   0x0C000000

// in order to make our custom oids hopefully somewhat unique
// we will use 0xFF (indicating implementation specific OID)
//               81 (first byte of non zero realtek unique identifier)
//               69 (second byte of non zero realtek unique identifier)
//               XX (the custom OID number - providing 255 possible custom oids)

#define CMD_BUS_MASTER              BIT_2

#endif  // _RT_EQU_H

