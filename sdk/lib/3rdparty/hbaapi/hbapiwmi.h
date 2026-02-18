
/* For now NOT a compatible header! Just minimal definitions required for vioscsi */

#ifndef _HBAPIWMI_H_
#define _HBAPIWMI_H_

#ifdef MS_SM_HBA_API
#define MS_SM_AdapterInformationQueryGuid \
    {0xbdc67efa,0xe5e7,0x4777,{0xb1,0x3c,0x62,0x14,0x59,0x65,0x70,0x99}}
#define MS_SM_PortInformationMethodsGuid \
    {0x5b6a8b86,0x708d,0x4ec6,{0x82,0xa6,0x39,0xad,0xcf,0x6f,0x64,0x33}}

typedef struct _MS_SM_AdapterInformationQuery
{
    ULONGLONG UniqueAdapterId;
    #define MS_SM_AdapterInformationQuery_UniqueAdapterId_SIZE sizeof(ULONGLONG)
    #define MS_SM_AdapterInformationQuery_UniqueAdapterId_ID 1
    ULONG HBAStatus;
    #define MS_SM_AdapterInformationQuery_HBAStatus_SIZE sizeof(ULONG)
    #define MS_SM_AdapterInformationQuery_HBAStatus_ID 2
    ULONG NumberOfPorts;
    #define MS_SM_AdapterInformationQuery_NumberOfPorts_SIZE sizeof(ULONG)
    #define MS_SM_AdapterInformationQuery_NumberOfPorts_ID 3
    ULONG VendorSpecificID;
    #define MS_SM_AdapterInformationQuery_VendorSpecificID_SIZE sizeof(ULONG)
    #define MS_SM_AdapterInformationQuery_VendorSpecificID_ID 4
    WCHAR Manufacturer[64 + 1];
    #define MS_SM_AdapterInformationQuery_Manufacturer_ID 5
    WCHAR SerialNumber[64 + 1];
    #define MS_SM_AdapterInformationQuery_SerialNumber_ID 6
    WCHAR Model[256 + 1];
    #define MS_SM_AdapterInformationQuery_Model_ID 7
    WCHAR ModelDescription[256 + 1];
    #define MS_SM_AdapterInformationQuery_ModelDescription_ID 8
    WCHAR HardwareVersion[256 + 1];
    #define MS_SM_AdapterInformationQuery_HardwareVersion_ID 9
    WCHAR DriverVersion[256 + 1];
    #define MS_SM_AdapterInformationQuery_DriverVersion_ID 10
    WCHAR OptionROMVersion[256 + 1];
    #define MS_SM_AdapterInformationQuery_OptionROMVersion_ID 11
    WCHAR FirmwareVersion[256 + 1];
    #define MS_SM_AdapterInformationQuery_FirmwareVersion_ID 12
    WCHAR DriverName[256 + 1];
    #define MS_SM_AdapterInformationQuery_DriverName_ID 13
    WCHAR HBASymbolicName[256 + 1];
    #define MS_SM_AdapterInformationQuery_HBASymbolicName_ID 14
    WCHAR RedundantOptionROMVersion[256 + 1];
    #define MS_SM_AdapterInformationQuery_RedundantOptionROMVersion_ID 15
    WCHAR RedundantFirmwareVersion[256 + 1];
    #define MS_SM_AdapterInformationQuery_RedundantFirmwareVersion_ID 16
    WCHAR MfgDomain[256 + 1];
    #define MS_SM_AdapterInformationQuery_MfgDomain_ID 17
} MS_SM_AdapterInformationQuery, *PMS_SM_AdapterInformationQuery;

typedef struct _MS_SMHBA_FC_Port
{
    UCHAR NodeWWN[8];
    #define MS_SMHBA_FC_Port_NodeWWN_SIZE sizeof(UCHAR[8])
    #define MS_SMHBA_FC_Port_NodeWWN_ID 1
    UCHAR PortWWN[8];
    #define MS_SMHBA_FC_Port_PortWWN_SIZE sizeof(UCHAR[8])
    #define MS_SMHBA_FC_Port_PortWWN_ID 2
    ULONG FcId;
    #define MS_SMHBA_FC_Port_FcId_SIZE sizeof(ULONG)
    #define MS_SMHBA_FC_Port_FcId_ID 3
    ULONG PortSupportedClassofService;
    #define MS_SMHBA_FC_Port_PortSupportedClassofService_SIZE sizeof(ULONG)
    #define MS_SMHBA_FC_Port_PortSupportedClassofService_ID 4
    UCHAR PortSupportedFc4Types[32];
    #define MS_SMHBA_FC_Port_PortSupportedFc4Types_SIZE sizeof(UCHAR[32])
    #define MS_SMHBA_FC_Port_PortSupportedFc4Types_ID 5
    UCHAR PortActiveFc4Types[32];
    #define MS_SMHBA_FC_Port_PortActiveFc4Types_SIZE sizeof(UCHAR[32])
    #define MS_SMHBA_FC_Port_PortActiveFc4Types_ID 6
    UCHAR FabricName[8];
    #define MS_SMHBA_FC_Port_FabricName_SIZE sizeof(UCHAR[8])
    #define MS_SMHBA_FC_Port_FabricName_ID 7
    ULONG NumberofDiscoveredPorts;
    #define MS_SMHBA_FC_Port_NumberofDiscoveredPorts_SIZE sizeof(ULONG)
    #define MS_SMHBA_FC_Port_NumberofDiscoveredPorts_ID 8
    UCHAR NumberofPhys;
    #define MS_SMHBA_FC_Port_NumberofPhys_SIZE sizeof(UCHAR)
    #define MS_SMHBA_FC_Port_NumberofPhys_ID 9
    WCHAR PortSymbolicName[256 + 1];
    #define MS_SMHBA_FC_Port_PortSymbolicName_ID 10
} MS_SMHBA_FC_Port, *PMS_SMHBA_FC_Port;

typedef struct _MS_SMHBA_PORTATTRIBUTES
{
    ULONG PortType;
    #define MS_SMHBA_PORTATTRIBUTES_PortType_SIZE sizeof(ULONG)
    #define MS_SMHBA_PORTATTRIBUTES_PortType_ID 1
    ULONG PortState;
    #define MS_SMHBA_PORTATTRIBUTES_PortState_SIZE sizeof(ULONG)
    #define MS_SMHBA_PORTATTRIBUTES_PortState_ID 2
    ULONG PortSpecificAttributesSize;
    #define MS_SMHBA_PORTATTRIBUTES_PortSpecificAttributesSize_SIZE sizeof(ULONG)
    #define MS_SMHBA_PORTATTRIBUTES_PortSpecificAttributesSize_ID 3
    WCHAR OSDeviceName[256 + 1];
    #define MS_SMHBA_PORTATTRIBUTES_OSDeviceName_ID 4
    ULONGLONG Reserved;
    #define MS_SMHBA_PORTATTRIBUTES_Reserved_SIZE sizeof(ULONGLONG)
    #define MS_SMHBA_PORTATTRIBUTES_Reserved_ID 5
    UCHAR PortSpecificAttributes[1];
    #define MS_SMHBA_PORTATTRIBUTES_PortSpecificAttributes_ID 6
} MS_SMHBA_PORTATTRIBUTES, *PMS_SMHBA_PORTATTRIBUTES;

typedef struct _MS_SMHBA_PROTOCOLSTATISTICS
{
    LONGLONG SecondsSinceLastReset;
    #define MS_SMHBA_PROTOCOLSTATISTICS_SecondsSinceLastReset_SIZE sizeof(LONGLONG)
    #define MS_SMHBA_PROTOCOLSTATISTICS_SecondsSinceLastReset_ID 1
    LONGLONG InputRequests;
    #define MS_SMHBA_PROTOCOLSTATISTICS_InputRequests_SIZE sizeof(LONGLONG)
    #define MS_SMHBA_PROTOCOLSTATISTICS_InputRequests_ID 2
    LONGLONG OutputRequests;
    #define MS_SMHBA_PROTOCOLSTATISTICS_OutputRequests_SIZE sizeof(LONGLONG)
    #define MS_SMHBA_PROTOCOLSTATISTICS_OutputRequests_ID 3
    LONGLONG ControlRequests;
    #define MS_SMHBA_PROTOCOLSTATISTICS_ControlRequests_SIZE sizeof(LONGLONG)
    #define MS_SMHBA_PROTOCOLSTATISTICS_ControlRequests_ID 4
    LONGLONG InputMegabytes;
    #define MS_SMHBA_PROTOCOLSTATISTICS_InputMegabytes_SIZE sizeof(LONGLONG)
    #define MS_SMHBA_PROTOCOLSTATISTICS_InputMegabytes_ID 5
    LONGLONG OutputMegabytes;
    #define MS_SMHBA_PROTOCOLSTATISTICS_OutputMegabytes_SIZE sizeof(LONGLONG)
    #define MS_SMHBA_PROTOCOLSTATISTICS_OutputMegabytes_ID 6
} MS_SMHBA_PROTOCOLSTATISTICS, *PMS_SMHBA_PROTOCOLSTATISTICS;

typedef struct _MS_SMHBA_FC_PHY
{
    ULONG PhySupportSpeed;
    #define MS_SMHBA_FC_PHY_PhySupportSpeed_SIZE sizeof(ULONG)
    #define MS_SMHBA_FC_PHY_PhySupportSpeed_ID 1
    ULONG PhySpeed;
    #define MS_SMHBA_FC_PHY_PhySpeed_SIZE sizeof(ULONG)
    #define MS_SMHBA_FC_PHY_PhySpeed_ID 2
    UCHAR PhyType;
    #define MS_SMHBA_FC_PHY_PhyType_SIZE sizeof(UCHAR)
    #define MS_SMHBA_FC_PHY_PhyType_ID 3
    ULONG MaxFrameSize;
    #define MS_SMHBA_FC_PHY_MaxFrameSize_SIZE sizeof(ULONG)
    #define MS_SMHBA_FC_PHY_MaxFrameSize_ID 4
} MS_SMHBA_FC_PHY, *PMS_SMHBA_FC_PHY;

typedef struct _MS_SMHBA_SAS_PHY
{
    UCHAR PhyIdentifier;
    #define MS_SMHBA_SAS_PHY_PhyIdentifier_SIZE sizeof(UCHAR)
    #define MS_SMHBA_SAS_PHY_PhyIdentifier_ID 1
    ULONG NegotiatedLinkRate;
    #define MS_SMHBA_SAS_PHY_NegotiatedLinkRate_SIZE sizeof(ULONG)
    #define MS_SMHBA_SAS_PHY_NegotiatedLinkRate_ID 2
    ULONG ProgrammedMinLinkRate;
    #define MS_SMHBA_SAS_PHY_ProgrammedMinLinkRate_SIZE sizeof(ULONG)
    #define MS_SMHBA_SAS_PHY_ProgrammedMinLinkRate_ID 3
    ULONG HardwareMinLinkRate;
    #define MS_SMHBA_SAS_PHY_HardwareMinLinkRate_SIZE sizeof(ULONG)
    #define MS_SMHBA_SAS_PHY_HardwareMinLinkRate_ID 4
    ULONG ProgrammedMaxLinkRate;
    #define MS_SMHBA_SAS_PHY_ProgrammedMaxLinkRate_SIZE sizeof(ULONG)
    #define MS_SMHBA_SAS_PHY_ProgrammedMaxLinkRate_ID 5
    ULONG HardwareMaxLinkRate;
    #define MS_SMHBA_SAS_PHY_HardwareMaxLinkRate_SIZE sizeof(ULONG)
    #define MS_SMHBA_SAS_PHY_HardwareMaxLinkRate_ID 6
    UCHAR domainPortWWN[8];
    #define MS_SMHBA_SAS_PHY_domainPortWWN_SIZE sizeof(UCHAR[8])
    #define MS_SMHBA_SAS_PHY_domainPortWWN_ID 7
} MS_SMHBA_SAS_PHY, *PMS_SMHBA_SAS_PHY;

#define SM_GetPortType 1
typedef struct _SM_GetPortType_IN
{
    ULONG PortIndex;
    #define SM_GetPortType_IN_PortIndex_SIZE sizeof(ULONG)
    #define SM_GetPortType_IN_PortIndex_ID 1
} SM_GetPortType_IN, *PSM_GetPortType_IN;

typedef struct _SM_GetPortType_OUT
{
    ULONG HBAStatus;
    #define SM_GetPortType_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SM_GetPortType_OUT_HBAStatus_ID 2
    ULONG PortType;
    #define SM_GetPortType_OUT_PortType_SIZE sizeof(ULONG)
    #define SM_GetPortType_OUT_PortType_ID 3
} SM_GetPortType_OUT, *PSM_GetPortType_OUT;

#define SM_GetPortType_IN_SIZE (FIELD_OFFSET(SM_GetPortType_IN, PortIndex) + SM_GetPortType_IN_PortIndex_SIZE)
#define SM_GetPortType_OUT_SIZE (FIELD_OFFSET(SM_GetPortType_OUT, PortType) + SM_GetPortType_OUT_PortType_SIZE)

#define SM_GetAdapterPortAttributes 2
typedef struct _SM_GetAdapterPortAttributes_IN
{
    ULONG PortIndex;
    #define SM_GetAdapterPortAttributes_IN_PortIndex_SIZE sizeof(ULONG)
    #define SM_GetAdapterPortAttributes_IN_PortIndex_ID 1
    #define SM_PORT_SPECIFIC_ATTRIBUTES_MAXSIZE  max(sizeof(MS_SMHBA_FC_Port),  sizeof(MS_SMHBA_SAS_Port))
    ULONG PortSpecificAttributesMaxSize;
    #define SM_GetAdapterPortAttributes_IN_PortSpecificAttributesMaxSize_SIZE sizeof(ULONG)
    #define SM_GetAdapterPortAttributes_IN_PortSpecificAttributesMaxSize_ID 2
} SM_GetAdapterPortAttributes_IN, *PSM_GetAdapterPortAttributes_IN;

typedef struct _SM_GetAdapterPortAttributes_OUT
{
    ULONG HBAStatus;
    #define SM_GetAdapterPortAttributes_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SM_GetAdapterPortAttributes_OUT_HBAStatus_ID 3
    MS_SMHBA_PORTATTRIBUTES PortAttributes;
    #define SM_GetAdapterPortAttributes_OUT_PortAttributes_SIZE sizeof(MS_SMHBA_PORTATTRIBUTES)
    #define SM_GetAdapterPortAttributes_OUT_PortAttributes_ID 4
} SM_GetAdapterPortAttributes_OUT, *PSM_GetAdapterPortAttributes_OUT;

#define SM_GetAdapterPortAttributes_IN_SIZE (FIELD_OFFSET(SM_GetAdapterPortAttributes_IN, PortSpecificAttributesMaxSize) + SM_GetAdapterPortAttributes_IN_PortSpecificAttributesMaxSize_SIZE)
#define SM_GetAdapterPortAttributes_OUT_SIZE (FIELD_OFFSET(SM_GetAdapterPortAttributes_OUT, PortAttributes) + SM_GetAdapterPortAttributes_OUT_PortAttributes_SIZE)

#define SM_GetDiscoveredPortAttributes 3
typedef struct _SM_GetDiscoveredPortAttributes_IN
{
    ULONG PortIndex;
    #define SM_GetDiscoveredPortAttributes_IN_PortIndex_SIZE sizeof(ULONG)
    #define SM_GetDiscoveredPortAttributes_IN_PortIndex_ID 1
    ULONG DiscoveredPortIndex;
    #define SM_GetDiscoveredPortAttributes_IN_DiscoveredPortIndex_SIZE sizeof(ULONG)
    #define SM_GetDiscoveredPortAttributes_IN_DiscoveredPortIndex_ID 2
    ULONG PortSpecificAttributesMaxSize;
    #define SM_GetDiscoveredPortAttributes_IN_PortSpecificAttributesMaxSize_SIZE sizeof(ULONG)
    #define SM_GetDiscoveredPortAttributes_IN_PortSpecificAttributesMaxSize_ID 3
} SM_GetDiscoveredPortAttributes_IN, *PSM_GetDiscoveredPortAttributes_IN;

typedef struct _SM_GetDiscoveredPortAttributes_OUT
{
    ULONG HBAStatus;
    #define SM_GetDiscoveredPortAttributes_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SM_GetDiscoveredPortAttributes_OUT_HBAStatus_ID 4
    MS_SMHBA_PORTATTRIBUTES PortAttributes;
    #define SM_GetDiscoveredPortAttributes_OUT_PortAttributes_SIZE sizeof(MS_SMHBA_PORTATTRIBUTES)
    #define SM_GetDiscoveredPortAttributes_OUT_PortAttributes_ID 5
} SM_GetDiscoveredPortAttributes_OUT, *PSM_GetDiscoveredPortAttributes_OUT;

#define SM_GetDiscoveredPortAttributes_OUT_SIZE (FIELD_OFFSET(SM_GetDiscoveredPortAttributes_OUT, PortAttributes) + SM_GetDiscoveredPortAttributes_OUT_PortAttributes_SIZE)
#define SM_GetDiscoveredPortAttributes_IN_SIZE (FIELD_OFFSET(SM_GetDiscoveredPortAttributes_IN, PortSpecificAttributesMaxSize) + SM_GetDiscoveredPortAttributes_IN_PortSpecificAttributesMaxSize_SIZE)

#define SM_GetPortAttributesByWWN 4
typedef struct _SM_GetPortAttributesByWWN_IN
{
    UCHAR PortWWN[8];
    #define SM_GetPortAttributesByWWN_IN_PortWWN_SIZE sizeof(UCHAR[8])
    #define SM_GetPortAttributesByWWN_IN_PortWWN_ID 1
    UCHAR DomainPortWWN[8];
    #define SM_GetPortAttributesByWWN_IN_DomainPortWWN_SIZE sizeof(UCHAR[8])
    #define SM_GetPortAttributesByWWN_IN_DomainPortWWN_ID 2
    ULONG PortSpecificAttributesMaxSize;
    #define SM_GetPortAttributesByWWN_IN_PortSpecificAttributesMaxSize_SIZE sizeof(ULONG)
    #define SM_GetPortAttributesByWWN_IN_PortSpecificAttributesMaxSize_ID 3
} SM_GetPortAttributesByWWN_IN, *PSM_GetPortAttributesByWWN_IN;

typedef struct _SM_GetPortAttributesByWWN_OUT
{
    ULONG HBAStatus;
    #define SM_GetPortAttributesByWWN_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SM_GetPortAttributesByWWN_OUT_HBAStatus_ID 4
    MS_SMHBA_PORTATTRIBUTES PortAttributes;
    #define SM_GetPortAttributesByWWN_OUT_PortAttributes_SIZE sizeof(MS_SMHBA_PORTATTRIBUTES)
    #define SM_GetPortAttributesByWWN_OUT_PortAttributes_ID 5
} SM_GetPortAttributesByWWN_OUT, *PSM_GetPortAttributesByWWN_OUT;

#define SM_GetPortAttributesByWWN_IN_SIZE (FIELD_OFFSET(SM_GetPortAttributesByWWN_IN, PortSpecificAttributesMaxSize) + SM_GetPortAttributesByWWN_IN_PortSpecificAttributesMaxSize_SIZE)
#define SM_GetPortAttributesByWWN_OUT_SIZE (FIELD_OFFSET(SM_GetPortAttributesByWWN_OUT, PortAttributes) + SM_GetPortAttributesByWWN_OUT_PortAttributes_SIZE)

#define SM_GetProtocolStatistics 5
typedef struct _SM_GetProtocolStatistics_IN
{
    ULONG PortIndex;
    #define SM_GetProtocolStatistics_IN_PortIndex_SIZE sizeof(ULONG)
    #define SM_GetProtocolStatistics_IN_PortIndex_ID 1
    ULONG ProtocolType;
    #define SM_GetProtocolStatistics_IN_ProtocolType_SIZE sizeof(ULONG)
    #define SM_GetProtocolStatistics_IN_ProtocolType_ID 2
} SM_GetProtocolStatistics_IN, *PSM_GetProtocolStatistics_IN;

typedef struct _SM_GetProtocolStatistics_OUT
{
    ULONG HBAStatus;
    #define SM_GetProtocolStatistics_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SM_GetProtocolStatistics_OUT_HBAStatus_ID 3
    MS_SMHBA_PROTOCOLSTATISTICS ProtocolStatistics;
    #define SM_GetProtocolStatistics_OUT_ProtocolStatistics_SIZE sizeof(MS_SMHBA_PROTOCOLSTATISTICS)
    #define SM_GetProtocolStatistics_OUT_ProtocolStatistics_ID 4
} SM_GetProtocolStatistics_OUT, *PSM_GetProtocolStatistics_OUT;

#define SM_GetProtocolStatistics_IN_SIZE (FIELD_OFFSET(SM_GetProtocolStatistics_IN, ProtocolType) + SM_GetProtocolStatistics_IN_ProtocolType_SIZE)
#define SM_GetProtocolStatistics_OUT_SIZE (FIELD_OFFSET(SM_GetProtocolStatistics_OUT, ProtocolStatistics) + SM_GetProtocolStatistics_OUT_ProtocolStatistics_SIZE)

#define SM_GetPhyStatistics 6
typedef struct _SM_GetPhyStatistics_IN
{
    ULONG PortIndex;
    #define SM_GetPhyStatistics_IN_PortIndex_SIZE sizeof(ULONG)
    #define SM_GetPhyStatistics_IN_PortIndex_ID 1
    ULONG PhyIndex;
    #define SM_GetPhyStatistics_IN_PhyIndex_SIZE sizeof(ULONG)
    #define SM_GetPhyStatistics_IN_PhyIndex_ID 2
    ULONG InNumOfPhyCounters;
    #define SM_GetPhyStatistics_IN_InNumOfPhyCounters_SIZE sizeof(ULONG)
    #define SM_GetPhyStatistics_IN_InNumOfPhyCounters_ID 3
} SM_GetPhyStatistics_IN, *PSM_GetPhyStatistics_IN;

typedef struct _SM_GetPhyStatistics_OUT
{
    ULONG HBAStatus;
    #define SM_GetPhyStatistics_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SM_GetPhyStatistics_OUT_HBAStatus_ID 4
    ULONG TotalNumOfPhyCounters;
    #define SM_GetPhyStatistics_OUT_TotalNumOfPhyCounters_SIZE sizeof(ULONG)
    #define SM_GetPhyStatistics_OUT_TotalNumOfPhyCounters_ID 5
    ULONG OutNumOfPhyCounters;
    #define SM_GetPhyStatistics_OUT_OutNumOfPhyCounters_SIZE sizeof(ULONG)
    #define SM_GetPhyStatistics_OUT_OutNumOfPhyCounters_ID 6
    LONGLONG PhyCounter[1];
    #define SM_GetPhyStatistics_OUT_PhyCounter_ID 7
} SM_GetPhyStatistics_OUT, *PSM_GetPhyStatistics_OUT;

#define SM_GetPhyStatistics_IN_SIZE (FIELD_OFFSET(SM_GetPhyStatistics_IN, InNumOfPhyCounters) + SM_GetPhyStatistics_IN_InNumOfPhyCounters_SIZE)

#define SM_GetFCPhyAttributes 7
typedef struct _SM_GetFCPhyAttributes_IN
{
    ULONG PortIndex;
    #define SM_GetFCPhyAttributes_IN_PortIndex_SIZE sizeof(ULONG)
    #define SM_GetFCPhyAttributes_IN_PortIndex_ID 1
    ULONG PhyIndex;
    #define SM_GetFCPhyAttributes_IN_PhyIndex_SIZE sizeof(ULONG)
    #define SM_GetFCPhyAttributes_IN_PhyIndex_ID 2
} SM_GetFCPhyAttributes_IN, *PSM_GetFCPhyAttributes_IN;

typedef struct _SM_GetFCPhyAttributes_OUT
{
    ULONG HBAStatus;
    #define SM_GetFCPhyAttributes_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SM_GetFCPhyAttributes_OUT_HBAStatus_ID 3
    MS_SMHBA_FC_PHY PhyType;
    #define SM_GetFCPhyAttributes_OUT_PhyType_SIZE sizeof(MS_SMHBA_FC_PHY)
    #define SM_GetFCPhyAttributes_OUT_PhyType_ID 4
} SM_GetFCPhyAttributes_OUT, *PSM_GetFCPhyAttributes_OUT;

#define SM_GetFCPhyAttributes_IN_SIZE (FIELD_OFFSET(SM_GetFCPhyAttributes_IN, PhyIndex) + SM_GetFCPhyAttributes_IN_PhyIndex_SIZE)
#define SM_GetFCPhyAttributes_OUT_SIZE (FIELD_OFFSET(SM_GetFCPhyAttributes_OUT, PhyType) + SM_GetFCPhyAttributes_OUT_PhyType_SIZE)

#define SM_GetSASPhyAttributes 8
typedef struct _SM_GetSASPhyAttributes_IN
{
    ULONG PortIndex;
    #define SM_GetSASPhyAttributes_IN_PortIndex_SIZE sizeof(ULONG)
    #define SM_GetSASPhyAttributes_IN_PortIndex_ID 1
    ULONG PhyIndex;
    #define SM_GetSASPhyAttributes_IN_PhyIndex_SIZE sizeof(ULONG)
    #define SM_GetSASPhyAttributes_IN_PhyIndex_ID 2
} SM_GetSASPhyAttributes_IN, *PSM_GetSASPhyAttributes_IN;

typedef struct _SM_GetSASPhyAttributes_OUT
{
    ULONG HBAStatus;
    #define SM_GetSASPhyAttributes_OUT_HBAStatus_SIZE sizeof(ULONG)
    #define SM_GetSASPhyAttributes_OUT_HBAStatus_ID 3
    MS_SMHBA_SAS_PHY PhyType;
    #define SM_GetSASPhyAttributes_OUT_PhyType_SIZE sizeof(MS_SMHBA_SAS_PHY)
    #define SM_GetSASPhyAttributes_OUT_PhyType_ID 4
} SM_GetSASPhyAttributes_OUT, *PSM_GetSASPhyAttributes_OUT;

#define SM_GetSASPhyAttributes_IN_SIZE (FIELD_OFFSET(SM_GetSASPhyAttributes_IN, PhyIndex) + SM_GetSASPhyAttributes_IN_PhyIndex_SIZE)
#define SM_GetSASPhyAttributes_OUT_SIZE (FIELD_OFFSET(SM_GetSASPhyAttributes_OUT, PhyType) + SM_GetSASPhyAttributes_OUT_PhyType_SIZE)

#define SM_RefreshInformation     10

#endif // MS_SM_HBA_API
#endif // _HBAPIWMI_H_
