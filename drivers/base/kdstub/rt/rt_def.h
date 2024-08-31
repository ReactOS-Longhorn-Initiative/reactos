// size is 2 bytes
typedef struct  TAG_802_1Q {
    union
    {
        struct
        {
		     USHORT     VLanID1 :  4;
		     USHORT  	CFI     :  1;
		     USHORT  	Priority:  3;
		     USHORT  	VLanID2 :  8;
        }   TagHeader;

        USHORT  Value;
    };
} TAG_8021q;

// size is 2+2+2+2+8 = 16 bytes
typedef struct _TXDESC {
    unsigned short	 length;
    unsigned short	 status;
	TAG_8021q        VLAN_TAG;
    unsigned short 	 TAGC;
    PHYSICAL_ADDRESS BufferAddress;
} TX_DESC, *PTX_DESC;


// size is 2+2+2+2+8 = 16 bytes
typedef struct _RX_DESC{
	unsigned short	 length:14;
	unsigned short	 CheckSumStatus:2;
	unsigned short	 status;
	TAG_8021q		 VLAN_TAG;
	unsigned short 	 TAVA;
	PHYSICAL_ADDRESS BufferAddress;
} RX_DESC, *PRX_DESC;

// move to another space later
#define TXS_CC3_0       BIT_0|BIT_1|BIT_2|BIT_3
#define TXS_EXC         BIT_4
#define TXS_LNKF		BIT_5
#define TXS_OWC			BIT_6
#define TXS_TES		    BIT_7
#define TXS_UNF         BIT_9
#define TXS_LGSEN		BIT_11
#define TXS_LS          BIT_12
#define TXS_FS          BIT_13
#define TXS_EOR         BIT_14
#define TXS_OWN         BIT_15

#define TX_CKSM_IP      BIT_13
#define TX_CKSM_TCP     BIT_14
#define TX_CKSM_UDP     BIT_15

#define CR_RST	        0x10    // software reset
#define CR_RE		    0x08    // rx enable
#define CR_TE		    0x04    // tx enable



#define SHORT_PACKET_PADDING_BUF_SIZE 120

#define TPPool_HPQ    	0x80      		// High priority queue polling
#define TPPool_NPQ		0x40			// normal priority queue polling

#define CR9346_EEM0     0x40            // select 8139 operating mode
#define CR9346_EEM1     0x80            // 00: normal

//----------------------------------------------------------------------------
//       8169 PHY Access Register (0x60-63)
//----------------------------------------------------------------------------
#define PHYAR_Flag      0x80000000

#define RXS_OWN		 BIT_15
#define RXS_EOR		 BIT_14
#define RXS_FS		 BIT_13
#define RXS_LS		 BIT_12



// 8169
#define RXS_MAR		 BIT_11
#define RXS_PAM		 BIT_10
#define RXS_BAR		 BIT_9
#define RXS_BOVF 	 BIT_8
#define RXS_FOVF 	 BIT_7
#define RXS_RWT 	 BIT_6
#define RXS_RES 	 BIT_5
#define RXS_RUNT 	 BIT_4
#define RXS_CRC 	 BIT_3
#define RXS_UDPIP_PACKET 	 BIT_2
#define RXS_TCPIP_PACKET 	 BIT_1
#define RXS_IP_PACKET BIT_2|BIT_1
#define RXS_IPF		 BIT_0
#define RXS_UDPF	 BIT_1
#define RXS_TCPF	 BIT_0
#define RFD_ACT_COUNT_MASK      0x3FFF

#define PKT_RETURN_STATUS_UNUSED 0
#define PKT_RETURN_STATUS_RX_OK 1
#define PKT_RETURN_STATUS_RETURNED 2


#define DTCCR_Cmd BIT_3 

// 8169
typedef struct _GDUMP_TALLY{
	ULONGLONG    TxOK;
	ULONGLONG    RxOK;
	ULONGLONG    TxERR;
	ULONG        RxERR;
	USHORT       MissPkt;
	USHORT       FAE;
	ULONG        Tx1Col;
	ULONG        TxMCol;
	ULONGLONG    RxOKPhy;
	ULONGLONG    RxOKBrd;
	ULONG        RxOKMul;
	USHORT       TxAbt;
	USHORT       TxUndrn; //only possible on jumbo frames
} GDUMP_TALLY, *PGDUMP_TALLY;

typedef enum _NIC_CHIP_TYPE
{
	RTLUNKNOWN,
	RTL8169,		// NO SUPPORT
	RTL8169S1,
	RTL8169S2,
	RTL8169SB,
	RTL8169SC,
	RTL8169SC_REV_E,
	RTL8168B,
	RTL8168B_REV_E,
	RTL8168B_REV_F,
	RTL8136,		// 8168B REV B
	RTL8136_REV_E,		// 8168B REV E
        RTL8136_REV_F,
	RTL8101,		// different with 8168B
	RTL8101_REV_C,
        RTL8168C,
        RTL8168C_REV_G,
        RTL8168C_REV_K,
        RTL8168C_REV_M,
        RTL8168CP,
        RTL8168CP_REV_B,
        RTL8168CP_REV_C,
        RTL8168CP_REV_D
} NIC_CHIP_TYPE, *PNIC_CHIP_TYPE;

//TASK Offload bits
#define	TXS_TCPCS		BIT_0
#define TXS_UDPCS		BIT_1
#define TXS_IPCS		BIT_2


#define	 	NIC_CONFIG0_REG_OFFSET		0x00
#define	 	NIC_CONFIG1_REG_OFFSET		0x01
#define	 	NIC_CONFIG2_REG_OFFSET		0x02
#define	 	NIC_CONFIG3_REG_OFFSET		0x03
#define	 	NIC_CONFIG4_REG_OFFSET		0x04
#define		NIC_CONFIG5_REG_OFFSET		0x05

//----------------------------------------------------------------------------
//       8139 (CONFIG5) CONFIG5 register bits(0xd8)
//----------------------------------------------------------------------------
#define CONFIG5_UWF  	0x10            // Unicast Wakeup Frame
#define CONFIG5_LDPS    0x04            // Link Down Power saving mode

//#define TCR_REG 0x40
#define CR_RST			0x10
#define CR_RE			0x08
#define CR_TE			0x04

#define RCR_AAP    		0x00000001      // accept all physical address
#define RCR_APM         0x00000002      // accept physical match
#define RCR_AM   		0x00000004      // accept multicast
#define RCR_AB			0x00000008      // accept broadcast
#define RCR_AR          0x00000010		// accept runt packet
#define RCR_AER         0x00000020		// accept error packet

#define CONFIG3_Magic  	0x20			// Wake on Magic packet
#define CONFIG4_iMode		0x01	// IP/TCP checksum compatibility with Intel NIC for RTL8169SB


enum {
	PHY_REG_BMCR = 0,
	PHY_REG_BMSR,
	PHY_REG_PHYAD1,
	PHY_REG_PHYAD2,
	PHY_REG_ANAR,
	PHY_REG_ANLPAR,
	PHY_REG_ANER,
	PHY_REG_ANNPTR,
	PHY_REG_ANNRPR,
	PHY_REG_GBCR,
	PHY_REG_GBSR,
	PHY_REG_RESV_11,
	PHY_REG_RESV_12,
	PHY_REG_RESV_13,
	PHY_REG_RESV_14,
	PHY_REG_GBESR
};

#define PHY_REG_BMSR_AUTO_NEG_COMPLETE BIT_5

#define PHY_STATUS_1000MF BIT_4
#define PHY_STATUS_100M BIT_3
#define PHY_STATUS_10M BIT_2
#define PHY_STATUS_FULL_DUPLEX BIT_0

#define ANER_LP_AUTO_NEG_ABLE BIT_0

#define ANAR_10_HALF BIT_5
#define ANAR_10_FULL BIT_6
#define ANAR_100_HALF BIT_7
#define ANAR_100_FULL BIT_8

#define ANAR_MAC_PAUSE BIT_10
#define ANAR_ASYM_PAUSE BIT_11

#define GBCR_1000_FULL BIT_9
#define GBCR_1000_HALF BIT_8

#define MAX_NIC_MULTICAST_REG 8

#define TCR_MXDMA_OFFSET	8
#define TCR_IFG0        0x01000000      // Interframe Gap0
#define TCR_IFG1        0x02000000      // Interframe Gap1
#define TCR_IFG2        0x00080000
#define	RCR_MXDMA_OFFSET	8
#define RCR_FIFO_OFFSET		13
#define RCR_RX_NO_THRESHOLD 7

#define PHY_LINK_STATUS BIT_1

#define CPCR_EN_ANALOG_PLL BIT_14		// 8169S only
#define CPCR_RX_VLAN BIT_6
#define CPCR_RX_CHECKSUM BIT_5
#define CPCR_PCI_DAC BIT_4
#define CPCR_MUL_RW BIT_3
#define CPCR_INT_MITI_TIMER_UNIT_1 BIT_1
#define CPCR_INT_MITI_TIMER_UNIT_0 BIT_0
#define CPCR_DISABLE_INT_MITI_PKT_NUM BIT_7  // 8168B only

#define MIN_RX_DESC 256
#define MAX_RX_DESC 512

#define MIN_TCB 32
#define MAX_TCB 64
#define MIN_TCB_HPQ 8
#define MAX_TCB_HPQ 16

//#define MAX_TX_DESC_JUMBO 64
//#define MAX_RX_DESC_JUMBO 64

// Recv
// How many packets that NetBufferList can link (DO NOT USE A SMALL VALUE)
//
#define RX_PKT_ONE_ROUND 32
//#define RX_PKT_ONE_ROUND_JUMBO 32
//#define RX_PKT_ONE_ROUND_VLANPRIORITY_ONLY 32


//
// PCI clock = 33 MHz
// 1 clock about 30.3 ns
//
#define SW_INT_MITI_INTERVAL_NORMAL (0x1000)
#define SW_INT_MITI_INTERVAL_LONG (0x1600)


// if free RX desc num is less that the value, NDIS will copy the packet
// set the value to MAX_TX_DESC to disable no_copy indication
//#define RX_PKT_COPY_TO_NDIS_THRESHOLD (MAX_RX_DESC)
//#define RX_PKT_COPY_TO_NDIS_THRESHOLD (MAX_RX_DESC / 4)
//#define RX_PKT_COPY_TO_NDIS_THRESHOLD_JUMBO (MAX_RX_DESC_JUMBO / 4)


// if free TX desc num is larger then the value, TX DPC will start send the SendWaitQ
//#define TX_SEND_WAITQ_FREE_TX_DESC_THRESHOLD (MAX_TX_DESC / 2)
//#define TX_SEND_WAITQ_FREE_TX_DESC_THRESHOLD_HPQ (MAX_TX_DESC_HPQ / 2)
//#define TX_SEND_WAITQ_FREE_TX_DESC_THRESHOLD_JUMBO (MAX_TX_DESC_JUMBO / 4)
//#define TX_SEND_WAITQ_FREE_TX_DESC_THRESHOLD_HPQ_JUMBO (MAX_TX_DESC_JUMBO / 4)

/////////////////////
#define PHY_LINK_STATUS BIT_1

#define		EPHYAR_Flag	0x80000000



enum {
TCR_RCR_MXDMA_16_BYTES=0,
TCR_RCR_MXDMA_32_BYTES,
TCR_RCR_MXDMA_64_BYTES,
TCR_RCR_MXDMA_128_BYTES,
TCR_RCR_MXDMA_256_BYTES,
TCR_RCR_MXDMA_512_BYTES,
TCR_RCR_MXDMA_1024_BYTES,
TCR_RCR_MXDMA_UNLIMITED
};

#define ISRIMR_ROK BIT_0
#define ISRIMR_RER BIT_1
#define ISRIMR_TOK BIT_2
#define ISRIMR_TER BIT_3
#define ISRIMR_RDU BIT_4
#define ISRIMR_LINK_CHG BIT_5
#define ISRIMR_RX_FOVW BIT_6
#define ISRIMR_TDU BIT_7
#define ISRIMR_SW_INT BIT_8
#define ISRIMR_TDMAOK BIT_10
#define ISRIMR_TIME_INT BIT_14
#define ISRIMR_SERR BIT_15

#define AUTO_NXET_RX_THRESHOLD 5
#define AUTO_NXET_TX_THRESHOLD 5

enum {
	CHKSUM_OFFLOAD_DISABLED,
	CHKSUM_OFFLOAD_TX_ENABLED,
	CHKSUM_OFFLOAD_RX_ENABLED,
	CHKSUM_OFFLOAD_TX_RX_ENABLED
};

enum {
	LSOGSO_OFFLOAD_DISABLED,
	LSOGSO_OFFLOAD_ENABLED
};

#define ONE_RX_DESC_LEN 16


enum {
	PERFORMANC_LOWEST_CPU_UTIL,
	PERFORMANC_LOW_CPU_UTIL,
	PERFORMANC_MID_CPU_UTIL,
	PERFORMANC_HIGH_CPU_UTIL,
};



