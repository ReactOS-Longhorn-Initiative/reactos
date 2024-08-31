#ifndef _RT_REG_H
#define _RT_REG_H

#pragma pack(1)

//-------------------------------------------------------------------------
// Ethernet Frame Structure
//-------------------------------------------------------------------------
//- Ethernet 6-byte Address
typedef struct _ETH_ADDRESS_STRUC {
    UCHAR       EthNodeAddress[ETHERNET_ADDRESS_LENGTH];
} ETH_ADDRESS_STRUC, *PETH_ADDRESS_STRUC;


//- Ethernet 14-byte Header
typedef struct _ETH_HEADER_STRUC {
    UCHAR       Destination[ETHERNET_ADDRESS_LENGTH];
    UCHAR       Source[ETHERNET_ADDRESS_LENGTH];
    USHORT      TypeLength;
} ETH_HEADER_STRUC, *PETH_HEADER_STRUC;

typedef struct _ETH_RX_BUFFER_STRUC {
    ETH_HEADER_STRUC    RxMacHeader;
    UCHAR               RxBufferData[(RCB_BUFFER_SIZE - sizeof(ETH_HEADER_STRUC))];
} ETH_RX_BUFFER_STRUC, *PETH_RX_BUFFER_STRUC;

#pragma pack()

//-------------------------------------------------------------------------
// PCI Register Definitions
// Refer To The PCI Specification For Detailed Explanations
//-------------------------------------------------------------------------
//- Register Offsets
#define PCI_VENDOR_ID_REGISTER      0x00    // PCI Vendor ID Register
#define PCI_DEVICE_ID_REGISTER      0x02    // PCI Device ID Register
#define PCI_CONFIG_ID_REGISTER      0x00    // PCI Configuration ID Register
#define PCI_COMMAND_REGISTER        0x04    // PCI Command Register
#define PCI_STATUS_REGISTER         0x06    // PCI Status Register
#define PCI_REV_ID_REGISTER         0x08    // PCI Revision ID Register
#define PCI_CLASS_CODE_REGISTER     0x09    // PCI Class Code Register
#define PCI_CACHE_LINE_REGISTER     0x0C    // PCI Cache Line Register
#define PCI_LATENCY_TIMER           0x0D    // PCI Latency Timer Register
#define PCI_HEADER_TYPE             0x0E    // PCI Header Type Register
#define PCI_BIST_REGISTER           0x0F    // PCI Built-In SelfTest Register
#define PCI_BAR_0_REGISTER          0x10    // PCI Base Address Register 0
#define PCI_BAR_1_REGISTER          0x14    // PCI Base Address Register 1
#define PCI_BAR_2_REGISTER          0x18    // PCI Base Address Register 2
#define PCI_BAR_3_REGISTER          0x1C    // PCI Base Address Register 3
#define PCI_BAR_4_REGISTER          0x20    // PCI Base Address Register 4
#define PCI_BAR_5_REGISTER          0x24    // PCI Base Address Register 5
#define PCI_SUBVENDOR_ID_REGISTER   0x2C    // PCI SubVendor ID Register
#define PCI_SUBDEVICE_ID_REGISTER   0x2E    // PCI SubDevice ID Register
#define PCI_EXPANSION_ROM           0x30    // PCI Expansion ROM Base Register
#define PCI_INTERRUPT_LINE          0x3C    // PCI Interrupt Line Register
#define PCI_INTERRUPT_PIN           0x3D    // PCI Interrupt Pin Register
#define PCI_MIN_GNT_REGISTER        0x3E    // PCI Min-Gnt Register
#define PCI_MAX_LAT_REGISTER        0x3F    // PCI Max_Lat Register
#define PCI_NODE_ADDR_REGISTER      0x40    // PCI Node Address Register


// MDI Control register bit definitions
#define MDI_CR_1000                 BIT_6       // Collision test enable
#define MDI_CR_COLL_TEST_ENABLE     BIT_7       // Collision test enable
#define MDI_CR_FULL_HALF            BIT_8       // FDX =1, half duplex =0
#define MDI_CR_RESTART_AUTO_NEG     BIT_9       // Restart auto negotiation
#define MDI_CR_ISOLATE              BIT_10      // Isolate PHY from MII
#define MDI_CR_POWER_DOWN           BIT_11      // Power down
#define MDI_CR_AUTO_SELECT          BIT_12      // Auto speed select enable
#define MDI_CR_10_100               BIT_13      // 0 = 10Mbs, 1 = 100Mbs
#define MDI_CR_LOOPBACK             BIT_14      // 0 = normal, 1 = loopback
#define MDI_CR_RESET                BIT_15      // 0 = normal, 1 = PHY reset

// MDI Status register bit definitions
#define MDI_SR_EXT_REG_CAPABLE      BIT_0       // Extended register capabilities
#define MDI_SR_JABBER_DETECT        BIT_1       // Jabber detected
#define MDI_SR_LINK_STATUS          BIT_2       // Link Status -- 1 = link
#define MDI_SR_AUTO_SELECT_CAPABLE  BIT_3       // Auto speed select capable
#define MDI_SR_REMOTE_FAULT_DETECT  BIT_4       // Remote fault detect
#define MDI_SR_AUTO_NEG_COMPLETE    BIT_5       // Auto negotiation complete
#define MDI_SR_10T_HALF_DPX         BIT_11      // 10BaseT Half Duplex capable
#define MDI_SR_10T_FULL_DPX         BIT_12      // 10BaseT full duplex capable
#define MDI_SR_TX_HALF_DPX          BIT_13      // TX Half Duplex capable
#define MDI_SR_TX_FULL_DPX          BIT_14      // TX full duplex capable
#define MDI_SR_T4_CAPABLE           BIT_15      // T4 capable

// Auto-Negotiation advertisement register bit definitions
#define NWAY_AD_SELCTOR_FIELD       BIT_0_4     // identifies supported protocol
#define NWAY_AD_ABILITY             BIT_5_12    // technologies that are supported
#define NWAY_AD_10T_HALF_DPX        BIT_5       // 10BaseT Half Duplex capable
#define NWAY_AD_10T_FULL_DPX        BIT_6       // 10BaseT full duplex capable
#define NWAY_AD_TX_HALF_DPX         BIT_7       // TX Half Duplex capable
#define NWAY_AD_TX_FULL_DPX         BIT_8       // TX full duplex capable
#define NWAY_AD_T4_CAPABLE          BIT_9       // T4 capable
#define NWAY_AD_REMOTE_FAULT        BIT_13      // indicates local remote fault
#define NWAY_AD_RESERVED            BIT_14      // reserved
#define NWAY_AD_NEXT_PAGE           BIT_15      // Next page (not supported)

// Auto-Negotiation link partner ability register bit definitions
#define NWAY_LP_SELCTOR_FIELD       BIT_0_4     // identifies supported protocol
#define NWAY_LP_ABILITY             BIT_5_9     // technologies that are supported
#define NWAY_LP_REMOTE_FAULT        BIT_13      // indicates partner remote fault
#define NWAY_LP_ACKNOWLEDGE         BIT_14      // acknowledge
#define NWAY_LP_NEXT_PAGE           BIT_15      // Next page (not supported)

// Auto-Negotiation expansion register bit definitions
#define NWAY_EX_LP_NWAY             BIT_0       // link partner is NWAY
#define NWAY_EX_PAGE_RECEIVED       BIT_1       // link code word received
#define NWAY_EX_NEXT_PAGE_ABLE      BIT_2       // local is next page able
#define NWAY_EX_LP_NEXT_PAGE_ABLE   BIT_3       // partner is next page able
#define NWAY_EX_PARALLEL_DET_FLT    BIT_4       // parallel detection fault
#define NWAY_EX_RESERVED            BIT_5_15    // reserved

#endif  // _RT_REG_H

