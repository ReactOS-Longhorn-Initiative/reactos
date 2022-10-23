/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    common.h

Abstract:

    Common functions that might be moved to port lib

Notes:

Revision History:

--*/

#include <ntddk.h>
#include <ata.h>
#include <storport.h>
#include <srbhelper.h>

#pragma once

#if _MSC_VER >= 1200
#pragma warning(push)
#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union
#else
#define __inline
#endif


#define ATA_BLOCK_SIZE                      512     //0x200

//#define MAXULONG                            0xFFFFFFFF  // winnt

#define ATA_INQUIRYDATA_SIZE                0x3C  // contains 1 Version Descriptor for 1667

#define DEFAULT_DSM_DATA_SIZE_CAP_IN_BLOCKS 0x8

// per ATA spec, the SectorCount value range is 1 - 0xffff. 0 means this entry is not valid.
#define MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE 0xFFFF

// Max of LBAs that can be represented using 28 bits.
#define MAX_28BIT_LBA   (1 << 28)


#define NVC_ATA_NV_CACHE_COMMAND                0xB6

#define IDE_FEATURE_INVALID                     0xFF

#define SCSI_VENDOR_ID_LENGTH                   (8)

//
// ATA function code
//

// 0x100 - 0x1FF indicate ATA commands
#define ATA_FUNCTION_ATA_COMMAND                0x100
#define ATA_FUNCTION_ATA_IDENTIFY               0x101
#define ATA_FUNCTION_ATA_READ                   0x102
#define ATA_FUNCTION_ATA_WRITE                  0x103
#define ATA_FUNCTION_ATA_FLUSH                  0x104
#define ATA_FUNCTION_ATA_SMART                  0x105

// 0x200 - 0x2FF indicate ATAPI commands
#define ATA_FUNCTION_ATAPI_COMMAND              0x200
#define ATA_FUNCTION_REQUEST_SENSE              0x201

// 0x400 - 0x4FF indicate SATA CFIS format
#define ATA_FUNCTION_ATA_CFIS_PAYLOAD           0x400

//
// ATA flags
//

/* defined in ntscsi.h
#define ATA_FLAGS_DRDY_REQUIRED         (1 << 0)
#define ATA_FLAGS_DATA_IN               (1 << 1)
#define ATA_FLAGS_DATA_OUT              (1 << 2)
#define ATA_FLAGS_48BIT_COMMAND         (1 << 3)
#define ATA_FLAGS_USE_DMA               (1 << 4)
#define ATA_FLAGS_NO_MULTIPLE           (1 << 5)
*/

#define ATA_FLAGS_RETURN_RESULTS        (1 << 6)    // task file should be copied back into SenseInfoBuffer
#define ATA_FLAGS_HIGH_PRIORITY         (1 << 7)    // the request should be processed as a high priority request
#define ATA_FLAGS_NEW_CDB               (1 << 8)    // new CDB in SrbExtension should be issued to device rather than CDB in Srb
#define ATA_FLAGS_COMPLETE_SRB          (1 << 9)    // indicates the Srb should be completed, AhciCompleteRequest will not send command from SrbExtension.
#define ATA_FLAGS_ACTIVE_REFERENCE      (1 << 10)   // indicates Active Reference needs to be acquired before processing the Srb and released after processing the Srb
#define ATA_FLAGS_SENSEDATA_SET         (1 << 11)   // indicates sense data has been set to the Srb

//
// helper macros
//
#define IsAtaCommand(AtaFunction)               (AtaFunction & ATA_FUNCTION_ATA_COMMAND)
#define IsAtapiCommand(AtaFunction)             (AtaFunction & ATA_FUNCTION_ATAPI_COMMAND)
#define IsRequestSenseSrb(AtaFunction)          (AtaFunction == ATA_FUNCTION_REQUEST_SENSE)
#define IsAtaCfisPayload(AtaFunction)           (AtaFunction == ATA_FUNCTION_ATA_CFIS_PAYLOAD)

#define IsReturnResults(AtaFlags)       (AtaFlags & ATA_FLAGS_RETURN_RESULTS)
#define IsNewCdbUsed(AtaFlags)          (AtaFlags & ATA_FLAGS_NEW_CDB)
#define Is48BitCommand(AtaFlags)        (AtaFlags & ATA_FLAGS_48BIT_COMMAND)
#define IsHighPriorityCommand(AtaFlags) (AtaFlags & ATA_FLAGS_HIGH_PRIORITY)
#define IsDmaCommand(AtaFlags)          (AtaFlags & ATA_FLAGS_USE_DMA)
#define SrbShouldBeCompleted(AtaFlags)  (AtaFlags & ATA_FLAGS_COMPLETE_SRB)


#define CLRMASK(x, mask)     ((x) &= ~(mask));
#define SETMASK(x, mask)     ((x) |=  (mask));

#define EVENT_BUFFER_MAX_LENGTH     4096
#define EVENT_NAME_MAX_LENGTH       32
#define EVENT_MAX_PARAM_NAME_LEN    32












/* SUSSY WUSSSY BAKAS */



/* Mode Sense/Select page constants */
#define MODE_PAGE_VENDOR_SPECIFIC       0x00
#define MODE_PAGE_ERROR_RECOVERY        0x01
#define MODE_PAGE_DISCONNECT            0x02
#define MODE_PAGE_FORMAT_DEVICE         0x03
#define MODE_PAGE_MRW                   0x03
#define MODE_PAGE_RIGID_GEOMETRY        0x04
#define MODE_PAGE_FLEXIBILE             0x05
#define MODE_PAGE_WRITE_PARAMETERS      0x05
#define MODE_PAGE_VERIFY_ERROR          0x07
#define MODE_PAGE_CACHING               0x08
#define MODE_PAGE_PERIPHERAL            0x09
#define MODE_PAGE_CONTROL               0x0A
#define MODE_PAGE_MEDIUM_TYPES          0x0B
#define MODE_PAGE_NOTCH_PARTITION       0x0C
#define MODE_PAGE_CD_AUDIO_CONTROL      0x0E
#define MODE_PAGE_DATA_COMPRESS         0x0F
#define MODE_PAGE_DEVICE_CONFIG         0x10
#define MODE_PAGE_XOR_CONTROL           0x10
#define MODE_PAGE_MEDIUM_PARTITION      0x11
#define MODE_PAGE_ENCLOSURE_SERVICES_MANAGEMENT 0x14
#define MODE_PAGE_EXTENDED              0x15
#define MODE_PAGE_EXTENDED_DEVICE_SPECIFIC 0x16
#define MODE_PAGE_CDVD_FEATURE_SET      0x18
#define MODE_PAGE_PROTOCOL_SPECIFIC_LUN 0x18
#define MODE_PAGE_PROTOCOL_SPECIFIC_PORT 0x19
#define MODE_PAGE_POWER_CONDITION       0x1A
#define MODE_PAGE_LUN_MAPPING           0x1B
#define MODE_PAGE_FAULT_REPORTING       0x1C
#define MODE_PAGE_CDVD_INACTIVITY       0x1D
#define MODE_PAGE_ELEMENT_ADDRESS       0x1D
#define MODE_PAGE_TRANSPORT_GEOMETRY    0x1E
#define MODE_PAGE_DEVICE_CAPABILITIES   0x1F
#define MODE_PAGE_CAPABILITIES          0x2A

#define MODE_SENSE_RETURN_ALL           0x3f

#define MODE_SENSE_CURRENT_VALUES       0x00
#define MODE_SENSE_CHANGEABLE_VALUES    0x40
#define MODE_SENSE_DEFAULT_VAULES       0x80
#define MODE_SENSE_SAVED_VALUES         0xc0

/* SCSI CDB operation codes */
#define SCSIOP_TEST_UNIT_READY          0x00
#define SCSIOP_REZERO_UNIT              0x01
#define SCSIOP_REWIND                   0x01
#define SCSIOP_REQUEST_BLOCK_ADDR       0x02
#define SCSIOP_REQUEST_SENSE            0x03
#define SCSIOP_FORMAT_UNIT              0x04
#define SCSIOP_READ_BLOCK_LIMITS        0x05
#define SCSIOP_REASSIGN_BLOCKS          0x07
#define SCSIOP_INIT_ELEMENT_STATUS      0x07
#define SCSIOP_READ6                    0x08
#define SCSIOP_RECEIVE                  0x08
#define SCSIOP_WRITE6                   0x0A
#define SCSIOP_PRINT                    0x0A
#define SCSIOP_SEND                     0x0A
#define SCSIOP_SEEK6                    0x0B
#define SCSIOP_TRACK_SELECT             0x0B
#define SCSIOP_SLEW_PRINT               0x0B
#define SCSIOP_SET_CAPACITY             0x0B
#define SCSIOP_SEEK_BLOCK               0x0C
#define SCSIOP_PARTITION                0x0D
#define SCSIOP_READ_REVERSE             0x0F
#define SCSIOP_WRITE_FILEMARKS          0x10
#define SCSIOP_FLUSH_BUFFER             0x10
#define SCSIOP_SPACE                    0x11
#define SCSIOP_INQUIRY                  0x12
#define SCSIOP_VERIFY6                  0x13
#define SCSIOP_RECOVER_BUF_DATA         0x14
#define SCSIOP_MODE_SELECT              0x15
#define SCSIOP_RESERVE_UNIT             0x16
#define SCSIOP_RELEASE_UNIT             0x17
#define SCSIOP_COPY                     0x18
#define SCSIOP_ERASE                    0x19
#define SCSIOP_MODE_SENSE               0x1A
#define SCSIOP_START_STOP_UNIT          0x1B
#define SCSIOP_STOP_PRINT               0x1B
#define SCSIOP_LOAD_UNLOAD              0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC       0x1C
#define SCSIOP_SEND_DIAGNOSTIC          0x1D
#define SCSIOP_MEDIUM_REMOVAL           0x1E

#define SCSIOP_READ_FORMATTED_CAPACITY  0x23
#define SCSIOP_READ_CAPACITY            0x25
#define SCSIOP_READ                     0x28
#define SCSIOP_WRITE                    0x2A
#define SCSIOP_SEEK                     0x2B
#define SCSIOP_LOCATE                   0x2B
#define SCSIOP_POSITION_TO_ELEMENT      0x2B
#define SCSIOP_WRITE_VERIFY             0x2E
#define SCSIOP_VERIFY                   0x2F
#define SCSIOP_SEARCH_DATA_HIGH         0x30
#define SCSIOP_SEARCH_DATA_EQUAL        0x31
#define SCSIOP_SEARCH_DATA_LOW          0x32
#define SCSIOP_SET_LIMITS               0x33
#define SCSIOP_READ_POSITION            0x34
#define SCSIOP_SYNCHRONIZE_CACHE        0x35
#define SCSIOP_COMPARE                  0x39
#define SCSIOP_COPY_COMPARE             0x3A
#define SCSIOP_WRITE_DATA_BUFF          0x3B
#define SCSIOP_READ_DATA_BUFF           0x3C
#define SCSIOP_WRITE_LONG               0x3F
#define SCSIOP_CHANGE_DEFINITION        0x40
#define SCSIOP_WRITE_SAME               0x41
#define SCSIOP_READ_SUB_CHANNEL         0x42
#define SCSIOP_UNMAP                    0x42
#define SCSIOP_READ_TOC                 0x43
#define SCSIOP_READ_HEADER              0x44
#define SCSIOP_REPORT_DENSITY_SUPPORT   0x44
#define SCSIOP_PLAY_AUDIO               0x45
#define SCSIOP_GET_CONFIGURATION        0x46
#define SCSIOP_PLAY_AUDIO_MSF           0x47
#define SCSIOP_PLAY_TRACK_INDEX         0x48
#define SCSIOP_SANITIZE                 0x48
#define SCSIOP_PLAY_TRACK_RELATIVE      0x49
#define SCSIOP_GET_EVENT_STATUS         0x4A
#define SCSIOP_PAUSE_RESUME             0x4B
#define SCSIOP_LOG_SELECT               0x4C
#define SCSIOP_LOG_SENSE                0x4D
#define SCSIOP_STOP_PLAY_SCAN           0x4E
#define SCSIOP_XDWRITE                  0x50
#define SCSIOP_XPWRITE                  0x51
#define SCSIOP_READ_DISK_INFORMATION    0x51
#define SCSIOP_READ_DISC_INFORMATION    0x51
#define SCSIOP_READ_TRACK_INFORMATION   0x52
#define SCSIOP_XDWRITE_READ             0x53
#define SCSIOP_RESERVE_TRACK_RZONE      0x53
#define SCSIOP_SEND_OPC_INFORMATION     0x54
#define SCSIOP_MODE_SELECT10            0x55
#define SCSIOP_RESERVE_UNIT10           0x56
#define SCSIOP_RESERVE_ELEMENT          0x56
#define SCSIOP_RELEASE_UNIT10           0x57
#define SCSIOP_RELEASE_ELEMENT          0x57
#define SCSIOP_REPAIR_TRACK             0x58
#define SCSIOP_MODE_SENSE10             0x5A
#define SCSIOP_CLOSE_TRACK_SESSION      0x5B
#define SCSIOP_READ_BUFFER_CAPACITY     0x5C
#define SCSIOP_SEND_CUE_SHEET           0x5D
#define SCSIOP_PERSISTENT_RESERVE_IN    0x5E
#define SCSIOP_PERSISTENT_RESERVE_OUT   0x5F

#define SCSIOP_OPERATION32              0x7F

#define SCSIOP_XDWRITE_EXTENDED16       0x80
#define SCSIOP_WRITE_FILEMARKS16        0x80
#define SCSIOP_REBUILD16                0x81
#define SCSIOP_READ_REVERSE16           0x81
#define SCSIOP_REGENERATE16             0x82
#define SCSIOP_EXTENDED_COPY            0x83
#define SCSIOP_POPULATE_TOKEN           0x83
#define SCSIOP_WRITE_USING_TOKEN        0x83
#define SCSIOP_RECEIVE_COPY_RESULTS     0x84
#define SCSIOP_RECEIVE_ROD_TOKEN_INFORMATION 0x84
#define SCSIOP_ATA_PASSTHROUGH16        0x85
#define SCSIOP_ACCESS_CONTROL_IN        0x86
#define SCSIOP_ACCESS_CONTROL_OUT       0x87
#define SCSIOP_READ16                   0x88
#define SCSIOP_COMPARE_AND_WRITE        0x89
#define SCSIOP_WRITE16                  0x8A
#define SCSIOP_READ_ATTRIBUTES          0x8C
#define SCSIOP_WRITE_ATTRIBUTES         0x8D
#define SCSIOP_WRITE_VERIFY16           0x8E
#define SCSIOP_VERIFY16                 0x8F
#define SCSIOP_PREFETCH16               0x90
#define SCSIOP_SYNCHRONIZE_CACHE16      0x91
#define SCSIOP_SPACE16                  0x91
#define SCSIOP_LOCK_UNLOCK_CACHE16      0x92
#define SCSIOP_LOCATE16                 0x92
#define SCSIOP_WRITE_SAME16             0x93
#define SCSIOP_ERASE16                  0x93
#define SCSIOP_ZBC_OUT                  0x94
#define SCSIOP_ZBC_IN                   0x95
#define SCSIOP_READ_DATA_BUFF16         0x9B
#define SCSIOP_READ_CAPACITY16          0x9E
#define SCSIOP_GET_LBA_STATUS           0x9E
#define SCSIOP_GET_PHYSICAL_ELEMENT_STATUS 0x9E
#define SCSIOP_REMOVE_ELEMENT_AND_TRUNCATE 0x9E
#define SCSIOP_SERVICE_ACTION_IN16      0x9E
#define SCSIOP_SERVICE_ACTION_OUT16     0x9F

//
// "Hybrid Information Field Bits" structure definition. e.g. Auxiliary(23:16) field.
//
#pragma pack(push, ata_hybrid_info_fields, 1)
typedef union _ATA_HYBRID_INFO_FIELDS {

    struct {
        UCHAR   HybridPriority  : 4;
        UCHAR   Reserved0       : 1;
        UCHAR   InfoValid       : 1;
        UCHAR   Reserved1       : 2;
    };

    UCHAR   AsUchar;

} ATA_HYBRID_INFO_FIELDS, *PATA_HYBRID_INFO_FIELDS;
#pragma pack (pop, ata_hybrid_info_fields)

#pragma pack(push, ATA_PHYSICAL_ELEMENT_STATUS_DESCRIPTOR, 1)
typedef struct _ATA_PHYSICAL_ELEMENT_STATUS_DESCRIPTOR {

    UCHAR Reserved1[4];
    ULONG ElementIdentifier;

    UCHAR Reserved2[6];
    UCHAR PhysicalElementType;
    UCHAR PhysicalElementHealth;

    ULONGLONG AssociatedCapacity;

    UCHAR Reserved3[8];

} ATA_PHYSICAL_ELEMENT_STATUS_DESCRIPTOR, *PATA_PHYSICAL_ELEMENT_STATUS_DESCRIPTOR;
#pragma pack(pop, ATA_PHYSICAL_ELEMENT_STATUS_DESCRIPTOR)

#pragma pack(push, ATA_GET_PHYSICAL_ELEMENT_STATUS_PARAMETER_DATA, 1)
typedef struct _ATA_GET_PHYSICAL_ELEMENT_STATUS_PARAMETER_DATA {

    ULONG NumberOfDescriptors;
    ULONG NumberOfDescriptorsReturned;

    ULONG ElementIdentifierBeingDepoped;
    UCHAR Reserved[20];

    ATA_PHYSICAL_ELEMENT_STATUS_DESCRIPTOR Descriptors[ANYSIZE_ARRAY];

} ATA_GET_PHYSICAL_ELEMENT_STATUS_PARAMETER_DATA, *PATA_GET_PHYSICAL_ELEMENT_STATUS_PARAMETER_DATA;
#pragma pack(pop, ATA_GET_PHYSICAL_ELEMENT_STATUS_PARAMETER_DATA)

//
// Definitions related to 0x9B - SCSIOP_READ_DATA_BUFF16(Mode 0x1C: Error History)
//

//
// Input: Mode field for Read buffer command
//

#define READ_BUFFER_MODE_ERROR_HISTORY                                              0x1C

//
// Input: Mode specific field for Read buffer command
//

#define MODE_SPECIFIC_CREATE_VENDOR_SPECIFIC_DATA                                   0x0
#define MODE_SPECIFIC_CREATE_CURRENT_INTERNAL_STATUS_DATA                           0x1

//
// Input: Buffer ID field for Read buffer command
//

//
// Return error history directory.
//
#define BUFFER_ID_RETURN_ERROR_HISTORY_DIRECTORY                                    0x0

//
// Return error history directory and create new error history snapshot.
//
#define BUFFER_ID_RETURN_ERROR_HISTORY_DIRECTORY_CREATE_NEW_ERROR_HISTORY_SNAPSHOT  0x1

//
// Return error history directory and establish new error history I_T nexus.
//
#define BUFFER_ID_RETURN_ERROR_HISTORY_DIRECTORY_ESTABLISH_NEW_NEXUS                0x2

//
// Return error history directory, establish new error history I_T nexus, 
// and create new error history snapshot.
//
#define BUFFER_ID_RETURN_ERROR_HISTORY_DIRECTORY_ESTABLISH_NEW_NEXUS_AND_SNAPSHOT   0x3

//
// 0x04h - 0x0Fh    Reserved.
//

//
// 0x10h - 0xEFh    Return error history.
//
#define BUFFER_ID_RETURN_ERROR_HISTORY_MINIMUM_THRESHOLD                            0x10

#define BUFFER_ID_RETURN_ERROR_HISTORY_MAXIMUM_THRESHOLD                            0xEF

//
// 0xF0h - 0xFDh    Reserved.
//

//
// Clear error history I_T nexus.
//
#define BUFFER_ID_CLEAR_ERROR_HISTORY_NEXUS                                         0xFE

//
// Clear error history I_T nexus and release any error history snapshots.
//
#define BUFFER_ID_CLEAR_ERROR_HISTORY_AND_RELEASE_ANY_SNAPSHOT                      0xFF

//
// Output: Error history source field
//

#define ERROR_HISTORY_SOURCE_CREATED_BY_DEVICE_SERVER                               0x0
#define ERROR_HISTORY_SOURCE_CREATED_DUE_TO_CURRENT_READ_BUFFER_COMMAND             0x1
#define ERROR_HISTORY_SOURCE_CREATED_DUE_TO_PREVIOUS_READ_BUFFER_COMMAND            0x2
#define ERROR_HISTORY_SOURCE_INDICATED_IN_BUFFER_SOURCE_FIELD                       0x3

//
// Output: Error history retrieved field
//

#define ERROR_HISTORY_RETRIEVED_NO_INFORMATION                                      0x0

//
// The error history I_T nexus has requested buffer ID FEh (i.e., clear error history I_T nexus) or buffer ID FFh
// (i.e., clear error history I_T nexus and release snapshot) for the current error history snapshot.
//
#define ERROR_HISTORY_RETRIEVED_BUFFER_ID_FE_OR_FF                                  0x1

//
// An error history I_T nexus has not requested buffer ID FEh (i.e., clear error history I_T nexus) or buffer ID FFh
// (i.e., clear error history I_T nexus and release snapshot) for the current error history snapshot.
//
#define ERROR_HISTORY_RETRIEVED_NOT_BUFFER_ID_FE_OR_FF                              0x2
#define ERROR_HISTORY_RETRIEVED_RESERVED                                            0x3

//
// Output: Buffer format
//

#define BUFFER_FORMAT_VENDOR_SPECIFIC                                               0x0
#define BUFFER_FORMAT_CURRENT_INTERNAL_STATUS_DATA                                  0x1
#define BUFFER_FORMAT_SAVED_INTERNAL_STATUS_DATA                                    0x2

//
// Output: Buffer source
//

#define BUFFER_SOURCE_INDICATED_IN_EHS_SOURCE_FIELD                                 0x0
#define BUFFER_SOURCE_UNKNOWN                                                       0x1
#define BUFFER_SOURCE_CREATED_BY_DEVICE_SERVER                                      0x2
#define BUFFER_SOURCE_CREATED_DUE_TO_CURRENT_COMMAND                                0x3
#define BUFFER_SOURCE_CREATED_DUE_TO_PREVIOUS_COMMAND                               0x4

#define STATUS_DATA_SET_SIZE_INCREMENT_IN_BYTES                                     0x200




typedef struct _NVCACHE_REQUEST_BLOCK {
    ULONG           NRBSize;
    USHORT          Function;
    ULONG           NRBFlags;
    ULONG           NRBStatus;
    ULONG           Count;
    ULONGLONG       LBA;
    ULONG           DataBufSize;
    ULONG           NVCacheStatus;
    ULONG           NVCacheSubStatus;
} NVCACHE_REQUEST_BLOCK, *PNVCACHE_REQUEST_BLOCK;

#define NRB_FUNCTION_NVCACHE_INFO               0xEC
#define NRB_FUNCTION_SPINDLE_STATUS                 0xE5
#define NRB_FUNCTION_NVCACHE_POWER_MODE_SET         0x00
#define NRB_FUNCTION_NVCACHE_POWER_MODE_RETURN  0x01
#define NRB_FUNCTION_FLUSH_NVCACHE              0x14
#define NRB_FUNCTION_QUERY_PINNED_SET           0x12
#define NRB_FUNCTION_QUERY_CACHE_MISS           0x13
#define NRB_FUNCTION_ADD_LBAS_PINNED_SET        0x10
#define NRB_FUNCTION_REMOVE_LBAS_PINNED_SET     0x11
#define NRB_FUNCTION_QUERY_ASCENDER_STATUS      0xD0
#define NRB_FUNCTION_QUERY_HYBRID_DISK_STATUS   0xD1

static const GUID STORPORT_POFX_ADAPTER_GUID = { 0xdcaf9c10, 0x895f, 0x481f, { 0xa4, 0x92, 0xd4, 0xce, 0xd2, 0xf5, 0x56, 0x33 } };
static const GUID STORPORT_POFX_LUN_GUID = { 0x585d326b, 0xb3a, 0x4088, { 0x89, 0x39, 0x88, 0xb0, 0xf, 0x69, 0x58, 0xbe } };

#define STOR_POFX_UNKNOWN_POWER 0x1 /* what */


typedef struct _STOR_POFX_COMPONENT_V2 {
    ULONG Version;
    ULONG Size;
    ULONG FStateCount;
    ULONG DeepestWakeableFState;
    GUID Id;

    //
    // Indicates the deepest F-State that this component can be in where the
    // adapter still requires power.  Only relevant for components whose Id
    // is STORPORT_POFX_LUN_GUID.
    //
    ULONG DeepestAdapterPowerRequiredFState;

    //
    // The deepest F-State that the component can be in where the miniport can
    // power up the component should a crash occur and a crash dump needs to be
    // written.
    // NOTE: In this case, the STOR_POFX_DEVICE_FLAG_NO_DUMP_ACTIVE flag does
    // not apply to the Idle Condition; it will only apply to D3Cold.
    //
    ULONG DeepestCrashDumpReadyFState;

    _Field_size_full_(FStateCount) STOR_POFX_COMPONENT_IDLE_STATE FStates[ANYSIZE_ARRAY];
} STOR_POFX_COMPONENT_V2, *PSTOR_POFX_COMPONENT_V2;

ULONG
FORCEINLINE
StorPortGetD3ColdSupport(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PVOID Address,
    _Out_ PBOOLEAN Supported
)

/*
Description:
    A miniport can call this function to inquire whether the given device
    supports D3Cold or not.

    This function must be called at PASSIVE_LEVEL.

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
              the unit object.
    Supported - TRUE if the device supports D3Cold, FALSE if it does not.

Returns:
    A STOR_STATUS code.

*/
{
    return StorPortExtendedFunction(ExtFunctionGetD3ColdSupport,
                                    HwDeviceExtension,
                                    Address,
                                    Supported);
}



//
// STOR_POFX_DEVICE defines a device to be runtime power-managed and should be
// used to register a device when calling StorPortInitializeFxPower().
//
typedef struct _STOR_POFX_DEVICE {
    ULONG Version;
    ULONG Size;
    ULONG ComponentCount;
    ULONG Flags;
    _Field_size_full_(ComponentCount) STOR_POFX_COMPONENT Components[ANYSIZE_ARRAY];
} STOR_POFX_DEVICE, *PSTOR_POFX_DEVICE;

#define STOR_POFX_DEVICE_SIZE ((ULONG)FIELD_OFFSET(STOR_POFX_DEVICE, Components))
#define STOR_POFX_DEVICE_VERSION_V1 1


#define STOR_POFX_DEVICE_V2_SIZE ((ULONG)FIELD_OFFSET(STOR_POFX_DEVICE_V2, Components))
#define STOR_POFX_DEVICE_VERSION_V2 2


#define STOR_POFX_DEVICE_V3_SIZE ((ULONG)FIELD_OFFSET(STOR_POFX_DEVICE_V3, Components))
#define STOR_POFX_DEVICE_VERSION_V3 3

//
// A miniport must only specify one component for a unit.
//
#define STOR_POFX_UNIT_MIN_COMPONENTS 1
#define STOR_POFX_UNIT_MAX_COMPONENTS 1

//
// A miniport must only specify one component for an adapter.
//
#define STOR_POFX_ADAPTER_MIN_COMPONENTS 1
#define STOR_POFX_ADAPTER_MAX_COMPONENTS 1


#define STOR_POFX_COMPONENT_V2_SIZE ((ULONG)FIELD_OFFSET(STOR_POFX_COMPONENT_V2, FStates))
#define STOR_POFX_COMPONENT_VERSION_V2 2


static const GUID STORPORT_DEVICEOPERATION_CACHED_SETTINGS_INIT_GUID = { 0x2b9443ac, 0xf89b, 0x48e8, { 0xb2, 0x92, 0x2c, 0xb6, 0xc9, 0x6e, 0xfd, 0x5a } };



#define NRB_SUCCESS                             0
#define NRB_ILLEGAL_REQUEST                     1
#define NRB_INVALID_PARAMETER                   2
#define NRB_INPUT_DATA_OVERRUN                  3
#define NRB_INPUT_DATA_UNDERRUN                 4
#define NRB_OUTPUT_DATA_OVERRUN                 5
#define NRB_OUTPUT_DATA_UNDERRUN                6
//
// Multiple roles are allowed for a single device.
//
#define STORAGE_COMPONENT_ROLE_CACHE        0x00000001
#define STORAGE_COMPONENT_ROLE_TIERING      0x00000002
#define STORAGE_COMPONENT_ROLE_DATA         0x00000004

//
// Log page for Device Statistics log
//

#define IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE           0x01
#define IDE_GP_LOG_DEVICE_STATISTICS_FREE_FALL_PAGE         0x02
#define IDE_GP_LOG_DEVICE_STATISTICS_ROTATING_MEDIA_PAGE    0x03
#define IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_ERROR_PAGE     0x04
#define IDE_GP_LOG_DEVICE_STATISTICS_TEMPERATURE_PAGE       0x05
#define IDE_GP_LOG_DEVICE_STATISTICS_TRANSPORT_PAGE         0x06
#define IDE_GP_LOG_DEVICE_STATISTICS_SSD_PAGE               0x07

#define HYBRID_REQUEST_INFO_STRUCTURE_VERSION           0x1
#define FIRMWARE_REQUEST_BLOCK_STRUCTURE_VERSION            0x1

//
// A driver should keep a "RefCount" for HYBRID_FUNCTION_ENABLE_CACHING_MEDIUM.
// e.g. it increases this value every time when it receives a HYBRID_FUNCTION_ENABLE_CACHING_MEDIUM call.
// 
// When driver receives HYBRID_FUNCTION_DISABLE_CACHING_MEDIUM call, it decreases "RefCount" when it's bigger than 0, 
// and then only send command to disable caching medium when the new "RefCount" value is 0.
// 
// In case of HYBRID_FUNCTION_DISABLE_CACHING_MEDIUM is received but no command is sent to device because of decreased "RefCount" is not 0,
// the driver returns following status to inform caller.
//
#define HYBRID_STATUS_ENABLE_REFCOUNT_HOLD                0x10

//
// General input data structure.
// Any data structures in input buffer after SRB_IO_CONTROL should contain HYBRID_REQUEST_BLOCK.
//
#define HYBRID_REQUEST_BLOCK_STRUCTURE_VERSION          0x1


//
// "Hybrid Information log page" strucutre definition
//
#define HYBRID_INFORMATION_DISABLED             0x00
#define HYBRID_INFORMATION_DISABLE_IN_PROCESS   0x80
#define HYBRID_INFORMATION_ENABLED              0xFF

#define HYBRID_HEALTH_UNUSEABLE                 0x01
#define HYBRID_HEALTH_NVM_SIZE_CHANGED          0x02
#define HYBRID_HEALTH_READ_ONLY                 0x04
#define HYBRID_HEALTH_DATA_LOSS                 0x08



#define HYBRID_FUNCTION_GET_INFO                            0x01

#define HYBRID_FUNCTION_DISABLE_CACHING_MEDIUM              0x10
#define HYBRID_FUNCTION_ENABLE_CACHING_MEDIUM               0x11
#define HYBRID_FUNCTION_SET_DIRTY_THRESHOLD                 0x12
#define HYBRID_FUNCTION_DEMOTE_BY_SIZE                      0x13

//
// HYBRID IOCTL status
//
#define HYBRID_STATUS_SUCCESS                             0x0
#define HYBRID_STATUS_ILLEGAL_REQUEST                     0x1
#define HYBRID_STATUS_INVALID_PARAMETER                   0x2
#define HYBRID_STATUS_OUTPUT_BUFFER_TOO_SMALL             0x3

typedef struct _HYBRID_REQUEST_BLOCK {
    ULONG   Version;            // HYBRID_REQUEST_BLOCK_STRUCTURE_VERSION
    ULONG   Size;               // Size of the data structure.
    ULONG   Function;           // Function code
    ULONG   Flags;

    ULONG   DataBufferOffset;   // the offset is from the beginning of buffer. e.g. from beginning of SRB_IO_CONTROL. The value should be multiple of sizeof(PVOID); Value 0 means that there is no data buffer.
    ULONG   DataBufferLength;   // length of the buffer
} HYBRID_REQUEST_BLOCK, *PHYBRID_REQUEST_BLOCK;


//
// Parameter for HYBRID_FUNCTION_SET_DIRTY_THRESHOLD
// Input buffer should contain SRB_IO_CONTROL, HYBRID_REQUEST_BLOCK and HYBRID_DIRTY_THRESHOLDS data structures.
// Field "DataBufferOffset" of HYBRID_REQUEST_BLOCK should be set to the starting offset of HYBRID_DIRTY_THRESHOLDS from beginning of buffer.
// NOTE that these functions don't have output parameter.
//
typedef struct _HYBRID_DIRTY_THRESHOLDS {
    ULONG   Version;
    ULONG   Size;               // sizeof(HYBRID_DIRTY_THRESHOLDS)

    ULONG   DirtyLowThreshold;  //
    ULONG   DirtyHighThreshold; // >= DirtyLowThreshold 
} HYBRID_DIRTY_THRESHOLDS, *PHYBRID_DIRTY_THRESHOLDS;

#pragma pack(push, hybrid_info_descriptor, 1)
typedef struct _GP_LOG_HYBRID_INFORMATION_DESCRIPTOR {

    UCHAR   HybridPriority;
    UCHAR   ConsumedNVMSizeFraction;
    UCHAR   ConsumedMappingResourcesFraction;
    UCHAR   ConsumedNVMSizeForDirtyDataFraction;
    UCHAR   ConsumedMappingResourcesForDirtyDataFraction;

    UCHAR   Reserved[11];

} GP_LOG_HYBRID_INFORMATION_DESCRIPTOR, *PGP_LOG_HYBRID_INFORMATION_DESCRIPTOR;
#pragma pack (pop, hybrid_info_descriptor)

#pragma pack(push, hybrid_info_log, 1)
typedef struct _GP_LOG_HYBRID_INFORMATION {

    GP_LOG_HYBRID_INFORMATION_HEADER        Header;

    GP_LOG_HYBRID_INFORMATION_DESCRIPTOR    Descriptor[0];

} GP_LOG_HYBRID_INFORMATION, *PGP_LOG_HYBRID_INFORMATION;
#pragma pack (pop, hybrid_info_log)
typedef enum _STORAGE_PROTOCOL_ATA_DATA_TYPE {
    AtaDataTypeUnknown = 0,
    AtaDataTypeIdentify,        // Retrieved by command - IDENTIFY DEVICE
    AtaDataTypeLogPage,         // Retrieved by command - READ LOG EXT
} STORAGE_PROTOCOL_ATA_DATA_TYPE, *PSTORAGE_PROTOCOL_ATA_DATA_TYPE;


// Storport interfaces to allow miniports to log ETW events.
//
typedef enum _STORPORT_ETW_LEVEL {

    // The following event levels are not throttled
    // i.e. they are always logged.
    StorportEtwLevelLogAlways = 0,
    StorportEtwLevelCritical = 1,

    // The following event levels are throttled
    // at the adapter or unit level. There is an
    // upper limit on the count of events for each
    // level per hour per adapter/unit.
    StorportEtwLevelError = 2,
    StorportEtwLevelWarning = 3,
    StorportEtwLevelInformational = 4,

    // The following event levels are not throttled
    // i.e. they are always logged.
    StorportEtwLevelVerbose = 5,
    StorportEtwLevelMax = StorportEtwLevelVerbose

} STORPORT_ETW_LEVEL, *PSTORPORT_ETW_LEVEL;
#define RTL_SIZEOF_THROUGH_FIELD(type, field) \
    (FIELD_OFFSET(type, field) + RTL_FIELD_SIZE(type, field))








typedef enum _STORPORT_ETW_EVENT_CHANNEL{
    StorportEtwEventDiagnostic = 0,
    StorportEtwEventOperational = 1,
    StorportEtwEventHealth = 2
} STORPORT_ETW_EVENT_CHANNEL, *PSTORPORT_ETW_EVENT_CHANNEL;

typedef enum _STORPORT_ETW_EVENT_OPCODE {
    StorportEtwEventOpcodeInfo = 0,
    StorportEtwEventOpcodeStart = 1,
    StorportEtwEventOpcodeStop = 2,
    StorportEtwEventOpcodeDC_Start = 3,
    StorportEtwEventOpcodeDC_Stop = 4,
    StorportEtwEventOpcodeExtension = 5,
    StorportEtwEventOpcodeReply = 6,
    StorportEtwEventOpcodeResume = 7,
    StorportEtwEventOpcodeSuspend = 8,
    StorportEtwEventOpcodeSend = 9,
    StorportEtwEventOpcodeReceive = 240
} STORPORT_ETW_EVENT_OPCODE, *PSTORPORT_ETW_EVENT_OPCODE;
////////////////////////////////////////////////////////////////////////////////
//
// DeviceDsmAction_NvCache_Change_Priority
//

typedef struct _DEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS {

    ULONG Size;

    UCHAR TargetPriority;
    UCHAR Reserved[3];

} DEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS, *PDEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS;



//
// Temperature Statistics Log Page structure definition.
// NOTE: all temperature value fields are signed byte.
//
#pragma pack(push, gplog_temperature_statistics_log, 1)
typedef struct _GP_LOG_TEMPERATURE_STATISTICS {

    DEVICE_STATISTICS_LOG_PAGE_HEADER Header;   //byte 0..7

    struct {                                //byte 8..15
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } CurrentTemperature;

    struct {                                //byte 16..23
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } AverageShortTermTemperature;

    struct {                                //byte 24..31
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } AverageLongTermTemperature;

    struct {                                //byte 32..39
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } HighestTemperature;

    struct {                                //byte 40..47
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } LowestTemperature;

    struct {                                //byte 48..55
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } HighestAverageShortTermTemperature;

    struct {                                //byte 56..63
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } LowestAverageShortTermTemperature;

    struct {                                //byte 64..71
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } HighstAverageLongTermTemperature;

    struct {                                //byte 72..79
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } LowestAverageLongTermTemperature;

    struct {                                //byte 80..87
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } TimeInOverTemperature;

    struct {                                //byte 88..95
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } SpecifiedMaximumOperatingTemperature;

    struct {                                //byte 96..103
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } TimeInUnderTemperature;

    struct {                                //byte 104..111
        ULONGLONG Value : 8;
        ULONGLONG Reserved : 48;
        GP_LOG_DEVICE_STATISTICS_FLAGS;
    } SpecifiedMinimumOperatingTemperature;

    UCHAR Reserved[400];

} GP_LOG_TEMPERATURE_STATISTICS, *PGP_LOG_TEMPERATURE_STATISTICS;
#pragma pack(pop, gplog_temperature_statistics_log)


#define STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION         0x1
#define STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION_V2      0x2

#define STORAGE_FIRMWARE_INFO_INVALID_SLOT              0xFF

typedef struct _FIRMWARE_REQUEST_BLOCK {
    ULONG   Version;            // FIRMWARE_REQUEST_BLOCK_STRUCTURE_VERSION
    ULONG   Size;               // Size of the data structure.
    ULONG   Function;           // Function code
    ULONG   Flags;

    ULONG   DataBufferOffset;   // the offset is from the beginning of buffer. e.g. from beginning of SRB_IO_CONTROL. The value should be multiple of sizeof(PVOID); Value 0 means that there is no data buffer.
    ULONG   DataBufferLength;   // length of the buffer
} FIRMWARE_REQUEST_BLOCK, *PFIRMWARE_REQUEST_BLOCK;

typedef struct _STORAGE_FIRMWARE_SLOT_INFO {

    UCHAR   SlotNumber;
    BOOLEAN ReadOnly;
    UCHAR   Reserved[6];

    union {
        UCHAR     Info[8];
        ULONGLONG AsUlonglong;
    } Revision;

} STORAGE_FIRMWARE_SLOT_INFO, *PSTORAGE_FIRMWARE_SLOT_INFO;

#define STORAGE_FIRMWARE_SLOT_INFO_V2_REVISION_LENGTH   16

typedef struct _STORAGE_FIRMWARE_SLOT_INFO_V2 {

    UCHAR   SlotNumber;
    BOOLEAN ReadOnly;
    UCHAR   Reserved[6];

    UCHAR   Revision[STORAGE_FIRMWARE_SLOT_INFO_V2_REVISION_LENGTH];

} STORAGE_FIRMWARE_SLOT_INFO_V2, *PSTORAGE_FIRMWARE_SLOT_INFO_V2;









typedef struct _STORAGE_FIRMWARE_INFO {

    ULONG   Version;        // STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION
    ULONG   Size;           // sizeof(STORAGE_FIRMWARE_INFO)

    BOOLEAN UpgradeSupport;
    UCHAR   SlotCount;
    UCHAR   ActiveSlot;
    UCHAR   PendingActivateSlot;

    ULONG   Reserved;

    STORAGE_FIRMWARE_SLOT_INFO Slot[0];

} STORAGE_FIRMWARE_INFO, *PSTORAGE_FIRMWARE_INFO;

typedef struct _STORAGE_FIRMWARE_INFO_V2 {

    ULONG   Version;        // STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION_V2
    ULONG   Size;           // sizeof(STORAGE_FIRMWARE_INFO_V2)

    BOOLEAN UpgradeSupport;
    UCHAR   SlotCount;
    UCHAR   ActiveSlot;
    UCHAR   PendingActivateSlot;

    BOOLEAN FirmwareShared;         // The firmware applies to both device and adapter. For example: PCIe SSD.
    UCHAR   Reserved[3];

    ULONG   ImagePayloadAlignment;  // Number of bytes. Max: PAGE_SIZE. The transfer size should be multiple of this unit size. Some protocol requires at least sector size. 0 means the value is not valid.
    ULONG   ImagePayloadMaxSize;    // for a single command.

    STORAGE_FIRMWARE_SLOT_INFO_V2 Slot[0];

} STORAGE_FIRMWARE_INFO_V2, *PSTORAGE_FIRMWARE_INFO_V2;





typedef enum _STORAGE_PROTOCOL_TYPE {
    ProtocolTypeUnknown = 0x00,
    ProtocolTypeScsi,
    ProtocolTypeAta,
    ProtocolTypeNvme,
    ProtocolTypeSd,
    ProtocolTypeUfs,
    ProtocolTypeProprietary = 0x7E,
    ProtocolTypeMaxReserved = 0x7F
} STORAGE_PROTOCOL_TYPE, *PSTORAGE_PROTOCOL_TYPE;





//


//
// Parameter for HYBRID_FUNCTION_DEMOTE_BY_SIZE
// Input buffer should contain SRB_IO_CONTROL, HYBRID_REQUEST_BLOCK and HYBRID_DEMOTE_BY_SIZE data structures.
// Field "DataBufferOffset" of HYBRID_REQUEST_BLOCK should be set to the starting offset of HYBRID_DEMOTE_BY_SIZE from beginning of buffer.
// NOTE that these functions don't have output parameter.
//
typedef struct _HYBRID_DEMOTE_BY_SIZE {
    ULONG       Version;
    ULONG       Size;               // sizeof(HYBRID_DEMOTE_BY_SIZE)

    UCHAR       SourcePriority;     // 1 ~ max priority
    UCHAR       TargetPriority;     // < SourcePriority
    USHORT      Reserved0;
    ULONG       Reserved1;
    ULONGLONG   LbaCount;           // How many LBAs should be demoted 
} HYBRID_DEMOTE_BY_SIZE, *PHYBRID_DEMOTE_BY_SIZE;


#define STORAGE_TEMPERATURE_VALUE_NOT_REPORTED           0x8000
//
// Protocol Data should follow this data structure in the same buffer.
// The offset of Protocol Data from the beginning of this data structure
// is reported in data field - "ProtocolDataOffset".
//
typedef struct _STORAGE_PROTOCOL_SPECIFIC_DATA {

    STORAGE_PROTOCOL_TYPE ProtocolType;
    ULONG   DataType;                     // The value will be protocol specific, as defined in STORAGE_PROTOCOL_NVME_DATA_TYPE or STORAGE_PROTOCOL_ATA_DATA_TYPE.

    ULONG   ProtocolDataRequestValue;
    ULONG   ProtocolDataRequestSubValue;  // Data sub request value

    ULONG   ProtocolDataOffset;           // The offset of data buffer is from beginning of this data structure.
    ULONG   ProtocolDataLength;

    ULONG   FixedProtocolReturnData;
    ULONG   ProtocolDataRequestSubValue2; // First additional data sub request value

    ULONG   ProtocolDataRequestSubValue3; // Second additional data sub request value
    ULONG   ProtocolDataRequestSubValue4; // Third additional data sub request value

} STORAGE_PROTOCOL_SPECIFIC_DATA, *PSTORAGE_PROTOCOL_SPECIFIC_DATA;

//
//
// Out parameters for StorageAdapterProtocolSpecificProperty (or StorageDeviceProtocolSpecificProperty) & PropertyStandardQuery
//
typedef struct _STORAGE_PROTOCOL_DATA_DESCRIPTOR {

    ULONG   Version;
    ULONG   Size;

    STORAGE_PROTOCOL_SPECIFIC_DATA ProtocolSpecificData;

} STORAGE_PROTOCOL_DATA_DESCRIPTOR, *PSTORAGE_PROTOCOL_DATA_DESCRIPTOR;


//
// Parameters for StorageAdapterTemperatureProperty (or StorageDeviceTemperatureProperty) & PropertyStandardQuery
//


//
// Input parameters for StorageAdapterTemperatureProperty (or StorageDeviceTemperatureProperty) & PropertyStandardQuery
// uses data structure STORAGE_PROPERTY_QUERY.
//

//
// Out parameters for StorageAdapterTemperatureProperty (or StorageDeviceTemperatureProperty) & PropertyStandardQuery
// For temperature/threshold data fields, the smallest value of SHORT type - 0x8000 indicates the value is not reported.
//
#define STORAGE_TEMPERATURE_VALUE_NOT_REPORTED           0x8000

typedef struct _STORAGE_TEMPERATURE_INFO {

    USHORT  Index;                      // Starts from 0. Index 0 may indicate a composite value.
    SHORT   Temperature;                // Signed value; in Celsius.
    SHORT   OverThreshold;              // Signed value; in Celsius.
    SHORT   UnderThreshold;             // Signed value; in Celsius.

    BOOLEAN OverThresholdChangable;     // Can the threshold value being changed by using IOCTL_STORAGE_SET_TEMPERATURE_THRESHOLD.
    BOOLEAN UnderThresholdChangable;    // Can the threshold value being changed by using IOCTL_STORAGE_SET_TEMPERATURE_THRESHOLD.
    BOOLEAN EventGenerated;             // Indicates that notification will be generated when temperature cross threshold.
    UCHAR   Reserved0;
    ULONG   Reserved1;

} STORAGE_TEMPERATURE_INFO, *PSTORAGE_TEMPERATURE_INFO;

typedef struct _STORAGE_TEMPERATURE_DATA_DESCRIPTOR {

    ULONG   Version;
    ULONG   Size;

    //
    // Indicates the maximum temperature in degrees Celsius that may prevent continued normal operation,
    // possibility of data loss, automatic device shutdown, extreme performance throttling, or permanent damage.
    //
    SHORT   CriticalTemperature;    // Signed value; in Celsius.

    //
    // Indicates the maximum temperature in degrees Celsius at which the device is capable of
    // operating continuously without degrading operation or reliability.
    //
    SHORT   WarningTemperature;     // Signed value; in Celsius.

    USHORT  InfoCount;              // Some devices may report more than one temperature information as there can be multiple sensors implemented.

    UCHAR   Reserved0[2];

    ULONG   Reserved1[2];

    STORAGE_TEMPERATURE_INFO TemperatureInfo[ANYSIZE_ARRAY];

} STORAGE_TEMPERATURE_DATA_DESCRIPTOR, *PSTORAGE_TEMPERATURE_DATA_DESCRIPTOR;


typedef struct _STORAGE_PHYSICAL_NODE_DATA {

    ULONG       NodeId;

    ULONG       AdapterCount;           // 0 or 1
    ULONG       AdapterDataLength;
    ULONG       AdapterDataOffset;      // Offset from beginning of this data structure. The buffer contains an array of STORAGE_PHYSICAL_ADAPTER_DATA.

    ULONG       DeviceCount;            // >= 1
    ULONG       DeviceDataLength;
    ULONG       DeviceDataOffset;       // Offset from beginning of this data structure. The buffer contains an array of STORAGE_PHYSICAL_DEVICE_DATA.

    ULONG       Reserved[3];

} STORAGE_PHYSICAL_NODE_DATA, *PSTORAGE_PHYSICAL_NODE_DATA;


typedef struct _STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR {

    ULONG       Version;            // sizeof(STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR)
    ULONG       Size;               // Total size of the data. Should be >= sizeof(STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR)

    ULONG       NodeCount;
    ULONG       Reserved;

    STORAGE_PHYSICAL_NODE_DATA Node[ANYSIZE_ARRAY];

} STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR, *PSTORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR;


ULONG
FORCEINLINE
StorPortEtwChannelEvent2(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ STORPORT_ETW_EVENT_CHANNEL EventChannel,
    _In_ ULONG EventId,
    _In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value
)
/*
Description:
    A miniport can call this function to log an ETW event to a specific channel with
    two extra general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventChannel - ETW channel where event is logged
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than STORPORT_ETW_MAX_DESCRIPTION_LENGTH characters,
        not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than STORPORT_ETW_MAX_PARAM_NAME_LENGTH characters, not including
        the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successfully logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)

    status = StorPortExtendedFunction(ExtFunctionMiniportChannelEtwEvent2,
                                        HwDeviceExtension,
                                        Address,
                                        EventChannel,
                                        EventId,
                                        EventDescription,
                                        EventKeywords,
                                        EventLevel,
                                        EventOpcode,
                                        Srb,
                                        Parameter1Name,
                                        Parameter1Value,
                                        Parameter2Name,
                                        Parameter2Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventChannel);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
#endif

    return status;
}


ULONG
FORCEINLINE
StorPortEtwEvent2(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG EventId,
    _In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value
)
/*
Description:
    A miniport can call this function to log an ETW event with two extra
    general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than STORPORT_ETW_MAX_DESCRIPTION_LENGTH characters,
        not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than STORPORT_ETW_MAX_PARAM_NAME_LENGTH characters, not including
        the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successfully logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)

    status = StorPortExtendedFunction(ExtFunctionMiniportChannelEtwEvent2,
                                        HwDeviceExtension,
                                        Address,
                                        StorportEtwEventDiagnostic,
                                        EventId,
                                        EventDescription,
                                        EventKeywords,
                                        EventLevel,
                                        EventOpcode,
                                        Srb,
                                        Parameter1Name,
                                        Parameter1Value,
                                        Parameter2Name,
                                        Parameter2Value);
#elif (NTDDI_VERSION >= NTDDI_WINBLUE)

    status = StorPortExtendedFunction(ExtFunctionMiniportEtwEvent2,
                                        HwDeviceExtension,
                                        Address,
                                        EventId,
                                        EventDescription,
                                        EventKeywords,
                                        EventLevel,
                                        EventOpcode,
                                        Srb,
                                        Parameter1Name,
                                        Parameter1Value,
                                        Parameter2Name,
                                        Parameter2Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
#endif

    return status;
}





ULONG
FORCEINLINE
StorPortAsyncNotificationDetected(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_ADDRESS Address,
    _In_ ULONGLONG Flags
    )
{
    ULONG Status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN8)

    StorPortNotification (AsyncNotificationDetected,
                          HwDeviceExtension,
                          Address,
                          Flags,
                          &Status);

#else

    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(Flags);

#endif //(NTDDI_VERSION >= NTDDI_WIN8)

    return Status;
}
















ULONG
FORCEINLINE
StorPortRegistryReadAdapterKey(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PUCHAR SubKeyName,
    _In_ PUCHAR ValueName,
    _In_ ULONG ValueType,
    _Inout_ PVOID *ValueData,
    _Inout_ PULONG ValueDataLength
    )
/*++

Routine Description:

    This routine is used by the miniport to read the registry data at
    HKLM/CurrentControlSet/Enum/<Instance path>/DeviceParameters/...

Arguments:

    HwDeviceExtension - The miniport's Device Extension.
    SubKeyName - The miniport subkey
    ValueName - Name of the Value under the key
    ValueType - REG_XXXX
    ValueData - Pointer to the data buffer
    ValueLength - Pointer to the data length

Return Value:

    A STOR_STATUS code.

--*/
{
    return StorPortExtendedFunction(ExtFunctionRegistryReadAdapterKey,
                                    HwDeviceExtension,
                                    SubKeyName,
                                    ValueName,
                                    ValueType,
                                    ValueData,
                                    ValueDataLength);
}


ULONG
FORCEINLINE
StorPortInvokeAcpiMethod (
    _In_      PVOID HwDeviceExtension,
    _In_opt_  PSTOR_ADDRESS Address,
    _In_      ULONG MethodName,
    _In_opt_  PVOID InputBuffer,
    _In_      ULONG InputBufferLength,
    _Out_opt_ PVOID OutputBuffer,
    _In_      ULONG OutputBufferLength,
    _Out_opt_ PULONG BytesReturned
    )
{
    return StorPortExtendedFunction(ExtFunctionInvokeAcpiMethod,
                                    HwDeviceExtension,
                                    Address,
                                    MethodName,
                                    InputBuffer,
                                    InputBufferLength,
                                    OutputBuffer,
                                    OutputBufferLength,
                                    BytesReturned);
}






ULONG
FORCEINLINE
StorPortIsDeviceOperationAllowed(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_ADDRESS Address,
    _In_ LPCGUID DeviceOperation,
    _Out_ ULONG *AllowedFlag
)
/*
Description:
    A miniport can call this function to inquire whether the current operating environment allows for the particular class of device management operation as indicated by
    the parameter.

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    Device Operation - an ID of a specific device management operation class
        AllowedFlag - true or false to indicate if a given operation is permitted on a current system

Returns:
    A STOR_STATUS code.
*/
{

    return StorPortExtendedFunction(ExtFunctionDeviceOperationAllowed,
                                    HwDeviceExtension,
                                    Address,
                                    DeviceOperation,
                                    AllowedFlag);
}



ULONG
FORCEINLINE
StorPortRegistryWriteAdapterKey(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PUCHAR SubKeyName,
    _In_ PUCHAR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID ValueData,
    _In_ ULONG ValueDataLength
    )
/*++

Routine Description:

    This routine is used by the miniport to write registry data to
    HKLM/CurrentControlSet/Enum/<Instance path>/DeviceParameters/...

Arguments:

    HwDeviceExtension - The miniport's Device Extension.
    SubKeyName - The miniport subkey
    ValueName - Name of the Value under the key
    ValueType - REG_XXXX
    ValueData - Data Buffer to be written
    ValueLength - Pointer to the data length

Return Value:

    A STOR_STATUS code.

--*/
{
    return StorPortExtendedFunction(ExtFunctionRegistryWriteAdapterKey,
                                    HwDeviceExtension,
                                    SubKeyName,
                                    ValueName,
                                    ValueType,
                                    ValueData,
                                    ValueDataLength);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG
FORCEINLINE
StorPortFreeWorker(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Worker
    )
{
    return StorPortExtendedFunction(ExtFunctionFreeWorker,
                                    HwDeviceExtension,
                                    Worker);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG
FORCEINLINE
StorPortInitializeWorker(
    _In_ PVOID HwDeviceExtension,
    _Out_ PVOID *Worker
    )
{
    return StorPortExtendedFunction(ExtFunctionInitializeWorker,
                                    HwDeviceExtension,
                                    Worker);
}


_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG
FORCEINLINE
StorPortQueueWorkItem(
    _In_ PVOID HwDeviceExtension,
    _In_ PHW_WORKITEM WorkItemCallback,
    _In_ PVOID Worker,
    _In_opt_ PVOID Context
    )
{
    return StorPortExtendedFunction(ExtFunctionQueueWorkItem,
                                    HwDeviceExtension,
                                    WorkItemCallback,
                                    Worker,
                                    Context);
}

typedef enum _STORAGE_COMPONENT_HEALTH_STATUS {
    HealthStatusUnknown = 0,
    HealthStatusNormal,
    HealthStatusThrottled,
    HealthStatusWarning,
    HealthStatusDisabled,
    HealthStatusFailed,
} STORAGE_COMPONENT_HEALTH_STATUS, *PSTORAGE_COMPONENT_HEALTH_STATUS;


typedef enum _STORAGE_DEVICE_FORM_FACTOR {
    FormFactorUnknown = 0,

    FormFactor3_5,          // 3.5 inch nominal form factor
    FormFactor2_5,          // 2.5 inch nominal form factor
    FormFactor1_8,          // 1.8 inch nominal form factor
    FormFactor1_8Less,      // Less than 1.8 inch nominal form factor

    FormFactorEmbedded,     // Embedded on board.
    FormFactorMemoryCard,   // Memory card such as SD, CF.
    FormFactormSata,        // mSATA
    FormFactorM_2,          // M.2
    FormFactorPCIeBoard,    // PCIe card plug into slot.
    FormFactorDimm,         // DIMM Slot

} STORAGE_DEVICE_FORM_FACTOR, *PSTORAGE_DEVICE_FORM_FACTOR;

typedef union _STORAGE_SPEC_VERSION {

    struct {
        union {
            struct {
                UCHAR   SubMinor;
                UCHAR   Minor;
            } DUMMYSTRUCTNAME;

            USHORT  AsUshort;

        } MinorVersion;

        USHORT  MajorVersion;
    } DUMMYSTRUCTNAME;

    ULONG   AsUlong;

} STORAGE_SPEC_VERSION, *PSTORAGE_SPEC_VERSION;

typedef struct _STORAGE_PHYSICAL_DEVICE_DATA {

    ULONG       DeviceId;
    ULONG       Role;                                   // Value(s) of bitmask from STORAGE_COMPONENT_ROLE_xxx

    STORAGE_COMPONENT_HEALTH_STATUS HealthStatus;
    STORAGE_PROTOCOL_TYPE           CommandProtocol;
    STORAGE_SPEC_VERSION            SpecVersion;        // Supported storage spec version. For example: SBC 3, SATA 3.2, NVMe 1.2
    STORAGE_DEVICE_FORM_FACTOR      FormFactor;

    UCHAR       Vendor[8];
    UCHAR       Model[40];
    UCHAR       FirmwareRevision[16];

    ULONGLONG   Capacity;                               // in unit of Kilo-Bytes (1024 bytes).

    UCHAR       PhysicalLocation[32];                   // Reserved for future.

    ULONG       Reserved[2];

} STORAGE_PHYSICAL_DEVICE_DATA, *PSTORAGE_PHYSICAL_DEVICE_DATA;








_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
FORCEINLINE
StorPortInitializePoFxPower(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ PSTOR_POFX_DEVICE Device,
    _Inout_ PBOOLEAN D3ColdEnabled
)
/*
Description:
    Miniports should call this function to register a device for runtime power
    management.

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device being registered is an adapter, otherwise the
        address specifies the unit object.
    Device - STOR_POFX_DEVICE representing the runtime power management
        properties of the device being registered.
    D3ColdEnabled - A pointer to a boolean that Storport will set to indicate
        whether or not D3Cold has been enabled for the given device.

Returns:
    A STOR_STATUS code.  Anything other than STOR_STATUS_SUCCESS indicates the
    device was not registered for runtime power management.
*/
{

    return StorPortExtendedFunction(ExtFunctionInitializePoFxPower,
                                    HwDeviceExtension,
                                    Address,
                                    Device,
                                    D3ColdEnabled);
}


#include <ntddstor.h>

typedef struct _STORPORT_TELEMETRY_EVENT {
    ULONG   DriverVersion;
    ULONG   EventId;
    UCHAR   EventName[EVENT_NAME_MAX_LENGTH];
    ULONG   EventVersion;
    ULONG   Flags;
    _Field_range_(0, EVENT_BUFFER_MAX_LENGTH)
    ULONG   EventBufferLength;
    _Field_size_bytes_(EventBufferLength)
    PUCHAR  EventBuffer;
    UCHAR   ParameterName0[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue0;
    UCHAR   ParameterName1[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue1;
    UCHAR   ParameterName2[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue2;
    UCHAR   ParameterName3[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue3;
    UCHAR   ParameterName4[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue4;
    UCHAR   ParameterName5[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue5;
    UCHAR   ParameterName6[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue6;
    UCHAR   ParameterName7[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue7;
} STORPORT_TELEMETRY_EVENT, *PSTORPORT_TELEMETRY_EVENT;

typedef struct _STORAGE_PHYSICAL_ADAPTER_DATA {

    ULONG       AdapterId;
    STORAGE_COMPONENT_HEALTH_STATUS HealthStatus;
    STORAGE_PROTOCOL_TYPE           CommandProtocol;
    STORAGE_SPEC_VERSION            SpecVersion;        // Supported storage spec version. For example: AHCI 1.3.1

    UCHAR       Vendor[8];
    UCHAR       Model[40];
    UCHAR       FirmwareRevision[16];

    UCHAR       PhysicalLocation[32];   // Reserve for future.

    BOOLEAN     ExpanderConnected;
    UCHAR       Reserved0[3];
    ULONG       Reserved1[3];

} STORAGE_PHYSICAL_ADAPTER_DATA, *PSTORAGE_PHYSICAL_ADAPTER_DATA;


#define STOR_POFX_DEVICE_FLAG_NO_D0                     0x01
#define STOR_POFX_DEVICE_FLAG_NO_D3                     0x02
#define STOR_POFX_DEVICE_FLAG_ENABLE_D3_COLD            0x04
#define STOR_POFX_DEVICE_FLAG_NO_DUMP_ACTIVE            0x08
#define STOR_POFX_DEVICE_FLAG_IDLE_TIMEOUT              0x10
#define STOR_POFX_DEVICE_FLAG_ADAPTIVE_D3_IDLE_TIMEOUT  0x20
#define STOR_POFX_DEVICE_FLAG_NO_UNIT_REGISTRATION      0x40
#define STOR_POFX_DEVICE_FLAG_PERF_STATE_PEP_OPTIONAL   0x80
#define STOR_POFX_DEVICE_FLAG_NO_IDLE_DEBOUNCE          0x100
#define STOR_POFX_DEVICE_FLAG_DUMP_ALWAYS_POWER_ON      0x200
#define STOR_POFX_DEVICE_FLAG_DISABLE_INTERRUPTS_ON_D3  0x400
#define STOR_POFX_DEVICE_FLAG_ADAPTER_D3_WAKE           0x800
#define STOR_POFX_DEVICE_FLAG_GET_PERF_STATE_FROM_PEP   0x1000

//
// Data structure and definitions related to IOCTL_SCSI_MINIPORT_FIRMWARE
//

#define FIRMWARE_FUNCTION_GET_INFO                          0x01
#define FIRMWARE_FUNCTION_DOWNLOAD                          0x02
#define FIRMWARE_FUNCTION_ACTIVATE                          0x03

//
// FIRMWARE IOCTL status
//
#define FIRMWARE_STATUS_SUCCESS                             0x0
#define FIRMWARE_STATUS_ERROR                               0x1
#define FIRMWARE_STATUS_ILLEGAL_REQUEST                     0x2
#define FIRMWARE_STATUS_INVALID_PARAMETER                   0x3
#define FIRMWARE_STATUS_INPUT_BUFFER_TOO_BIG                0x4
#define FIRMWARE_STATUS_OUTPUT_BUFFER_TOO_SMALL             0x5
#define FIRMWARE_STATUS_INVALID_SLOT                        0x6
#define FIRMWARE_STATUS_INVALID_IMAGE                       0x7
#define FIRMWARE_STATUS_CONTROLLER_ERROR                    0x10
#define FIRMWARE_STATUS_POWER_CYCLE_REQUIRED                0x20
#define FIRMWARE_STATUS_DEVICE_ERROR                        0x40
#define FIRMWARE_STATUS_INTERFACE_CRC_ERROR                 0x80
#define FIRMWARE_STATUS_UNCORRECTABLE_DATA_ERROR            0x81
#define FIRMWARE_STATUS_MEDIA_CHANGE                        0x82
#define FIRMWARE_STATUS_ID_NOT_FOUND                        0x83
#define FIRMWARE_STATUS_MEDIA_CHANGE_REQUEST                0x84
#define FIRMWARE_STATUS_COMMAND_ABORT                       0x85
#define FIRMWARE_STATUS_END_OF_MEDIA                        0x86
#define FIRMWARE_STATUS_ILLEGAL_LENGTH                      0x87

//
// FIRMWARE IOCTL status
//
#define FIRMWARE_STATUS_SUCCESS                             0x0
#define FIRMWARE_STATUS_ERROR                               0x1
#define FIRMWARE_STATUS_ILLEGAL_REQUEST                     0x2
#define FIRMWARE_STATUS_INVALID_PARAMETER                   0x3
#define FIRMWARE_STATUS_INPUT_BUFFER_TOO_BIG                0x4
#define FIRMWARE_STATUS_OUTPUT_BUFFER_TOO_SMALL             0x5
#define FIRMWARE_STATUS_INVALID_SLOT                        0x6
#define FIRMWARE_STATUS_INVALID_IMAGE                       0x7
#define FIRMWARE_STATUS_CONTROLLER_ERROR                    0x10
#define FIRMWARE_STATUS_POWER_CYCLE_REQUIRED                0x20
#define FIRMWARE_STATUS_DEVICE_ERROR                        0x40
#define FIRMWARE_STATUS_INTERFACE_CRC_ERROR                 0x80
#define FIRMWARE_STATUS_UNCORRECTABLE_DATA_ERROR            0x81
#define FIRMWARE_STATUS_MEDIA_CHANGE                        0x82
#define FIRMWARE_STATUS_ID_NOT_FOUND                        0x83
#define FIRMWARE_STATUS_MEDIA_CHANGE_REQUEST                0x84
#define FIRMWARE_STATUS_COMMAND_ABORT                       0x85
#define FIRMWARE_STATUS_END_OF_MEDIA                        0x86

//
// These keyword bits can be OR'd together to specify more than one keyword.
//
#define STORPORT_ETW_EVENT_KEYWORD_IO                    0x01
#define STORPORT_ETW_EVENT_KEYWORD_PERFORMANCE           0x02
#define STORPORT_ETW_EVENT_KEYWORD_POWER                 0x04
#define STORPORT_ETW_EVENT_KEYWORD_ENUMERATION           0x08
#define STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE         0x10
#define STORPORT_ETW_EVENT_KEYWORD_ASYNC_EVENT           0x20
#define STORPORT_ETW_EVENT_KEYWORD_NON_IO                0x40
#define STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE_IO      0x80
#define STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE_NON_IO  0x100

// Flags to indicate miniport support
#define STOR_FEATURE_VIRTUAL_MINIPORT                       0x00000001  // indicating it's a virtual miniport driver
#define STOR_FEATURE_ATA_PASS_THROUGH                       0x00000002  // indicating the miniport driver supports ATA PASS THROUGH (16)
#define STOR_FEATURE_FULL_PNP_DEVICE_CAPABILITIES           0x00000004  // indicating the miniport driver fills in complete information in STOR_DEVICE_CAPABILITIES
#define STOR_FEATURE_DUMP_POINTERS                          0x00000008  // indicating physical miniport driver support the dump pointers SRBs
#define STOR_FEATURE_DEVICE_NAME_NO_SUFFIX                  0x00000010  // indicating the miniport driver does not want the suffix "SCSI <type> Device" in device friendly name
#define STOR_FEATURE_DUMP_RESUME_CAPABLE                    0x00000020  // indicating the miniport driver is resume capable (i.e. dump stack functions after a hibernation resume)
#define STOR_FEATURE_DEVICE_DESCRIPTOR_FROM_ATA_INFO_VPD    0x00000040  // indicating that port driver forms STORAGE_DEVICE_DESCRIPTOR from ATA Information VPD page rather than INQUIRY data
#define STOR_FEATURE_EXTRA_IO_INFORMATION                   0x00000080  // Indicating that miniport driver wants SRBEX_DATA_IO_INFO in a SRBEX if available
#define STOR_FEATURE_ADAPTER_CONTROL_PRE_FINDADAPTER        0x00000100  // Indicating that miniport driver can safely process AdapterControl call from Storport before receiving HwFindAdapter.
#define STOR_FEATURE_ADAPTER_NOT_REQUIRE_IO_PORT            0x00000200  // Indicating that miniport driver doesn't require IO Port resource for its adapter.
#define STOR_FEATURE_DUMP_16_BYTE_ALIGNMENT                 0x00000400  // Indicating that the miniport driver wants its HwDeviceExtension to be 16 byte aligned in dump mode
#define STOR_FEATURE_SET_ADAPTER_INTERFACE_TYPE             0x00000800  // Indicating that the miniport driver wants storport to set the adapter interface type.
#define STOR_FEATURE_DUMP_INFO                              0x00001000  // Indicating that the miniport driver supports the dump info SRBs
#define STOR_FEATURE_DMA_ALLOCATION_NO_BOUNDARY             0x00002000  // Indicating that the miniport driver supports to allocate DMA to physical memory without boundaries.
#define STOR_FEATURE_NVME                                   0x00004000  // Indicating that the miniport driver supports NVMe SRBEX and NVMe workflow
#define STOR_FEATURE_REPORT_INTERNAL_DATA                   0x00008000  // Indicating that the miniport driver supports reporting internal data
#define STOR_FEATURE_EARLY_DUMP                             0x00010000  // Indicating that the miniport driver supports early crash dump generation


#define MINIPORT_REG_SZ         1
#define MINIPORT_REG_BINARY     3
#define MINIPORT_REG_DWORD      4
#define MINIPORT_REG_MULTI_SZ   7
#define CURRENT_DEVICE_INTERNAL_STATUS_DATA_LOG_ADDRESS 0x24

#pragma pack(push, gplog_current_device_data, 1)
typedef struct _CURRENT_DEVICE_INTERNAL_STATUS_LOG {

    UCHAR   LogAddress;                         // Byte 0
    UCHAR   Reserved0[3];                       // Byte 1-3
    ULONG   OrganizationID;                     // Bytes 4-7, 31:24 reserved, 23:0 - IEEE OUI
    USHORT  Area1LastLogPage;                   // Bytes 8-9
    USHORT  Area2LastLogPage;                   // Bytes 10-11
    USHORT  Area3LastLogPage;                   // Bytes 12-13
    UCHAR   Reserved2[368];                     // Bytes 14-381
    UCHAR   SavedDataAvailable;                 // Byte 382
    UCHAR   SavedDataGenerationNumber;          // Byte 383
    UCHAR   ReasonIdentifier[128];              // Bytes 384-511

} CURRENT_DEVICE_INTERNAL_STATUS_LOG, *PCURRENT_DEVICE_INTERNAL_STATUS_LOG;
#pragma pack (pop, gplog_current_device_data)

//
#define IOCTL_MINIPORT_SIGNATURE_SCSIDISK           "SCSIDISK"
#define IOCTL_MINIPORT_SIGNATURE_HYBRDISK           "HYBRDISK"
#define IOCTL_MINIPORT_SIGNATURE_DSM_NOTIFICATION   "MPDSM   "
#define IOCTL_MINIPORT_SIGNATURE_DSM_GENERAL        "MPDSMGEN"

#define IOCTL_MINIPORT_SIGNATURE_FIRMWARE           "FIRMWARE"
#define IOCTL_MINIPORT_SIGNATURE_QUERY_PROTOCOL     "PROTOCOL"
#define IOCTL_MINIPORT_SIGNATURE_QUERY_TEMPERATURE  "TEMPERAT"
#define IOCTL_MINIPORT_SIGNATURE_SET_TEMPERATURE_THRESHOLD  "SETTEMPT"
#define IOCTL_MINIPORT_SIGNATURE_QUERY_PHYSICAL_TOPOLOGY    "TOPOLOGY"


    typedef struct _NVFEATURE_PARAMETER_{
        USHORT NVPowerModeEnabled;
        USHORT NVParameterReserv1;
        USHORT NVCmdEnabled;
        USHORT NVParameterReserv2;
        USHORT NVPowerModeVer;
        USHORT NVCmdVer;
        ULONG  NVSize;               // in number of LBA
        USHORT NVReadSpeed;          // in MB/s
        USHORT NVWrtSpeed;
        ULONG  DeviceSpinUpTime;             // in second
} NV_FEATURE_PARAMETER, *PNV_FEATURE_PARAMETER;

//
// Output parameter for HYBRID_FUNCTION_GET_INFO
//
typedef enum _NVCACHE_TYPE {
    NvCacheTypeUnknown        = 0,  // Driver can't report the type of the nvcache
    NvCacheTypeNone           = 1,  // Device doesn't support non-volatile cache
    NvCacheTypeWriteBack      = 2,  // Device supports write back caching
    NvCacheTypeWriteThrough   = 3   // Device supports write through caching
} NVCACHE_TYPE;

typedef enum _NVCACHE_STATUS {
    NvCacheStatusUnknown     = 0,   // Driver can't report non-volatile cache status
    NvCacheStatusDisabling   = 1,   // non-volatile cache is in process of being disabled.
    NvCacheStatusDisabled    = 2,   // non-volatile cache has been disabled.
    NvCacheStatusEnabled     = 3    // non-volatile cache has been enabled.
} NVCACHE_STATUS;

typedef struct _NVCACHE_PRIORITY_LEVEL_DESCRIPTOR {
    UCHAR   PriorityLevel;
    UCHAR   Reserved0[3];
    ULONG   ConsumedNVMSizeFraction;
    ULONG   ConsumedMappingResourcesFraction;
    ULONG   ConsumedNVMSizeForDirtyDataFraction;
    ULONG   ConsumedMappingResourcesForDirtyDataFraction;
    ULONG   Reserved1;
} NVCACHE_PRIORITY_LEVEL_DESCRIPTOR, *PNVCACHE_PRIORITY_LEVEL_DESCRIPTOR;

typedef struct _HYBRID_INFORMATION {
    ULONG           Version;                // HYBRID_REQUEST_INFO_STRUCTURE_VERSION
    ULONG           Size;                   // sizeof(HYBRID_INFORMATION)

    BOOLEAN         HybridSupported;
    NVCACHE_STATUS  Status;                 // for hybrid disk, expect values can be: NvCacheStatusDisabling, NvCacheStatusDisabled or NvCacheStatusEnabled
    NVCACHE_TYPE    CacheTypeEffective;     // for hybrid disk, expect value will be: NvCacheTypeWriteBack
    NVCACHE_TYPE    CacheTypeDefault;       // for hybrid disk, expect values can be: NvCacheTypeWriteBack

    ULONG           FractionBase;           // Base value of all fraction type of fields in the data structure. For hybrid disk, value of this field will be 255.

    ULONGLONG       CacheSize;              // total size of NVCache. unit: LBA count

    struct {
        ULONG   WriteCacheChangeable    : 1;    // Does the device respect change in write caching policy
        ULONG   WriteThroughIoSupported : 1;    // Does the device support WriteThrough semantics for the NVCache on individual Writes.
        ULONG   FlushCacheSupported     : 1;    // Does the device support flushing of the NVCache
        ULONG   Removable               : 1;    // Does the nvcache can be removed.
        ULONG   ReservedBits            : 28;
    } Attributes;

    struct {
        UCHAR     PriorityLevelCount;           // A non-zero value indicates the non-volatile cache supports priority levels.
        BOOLEAN   MaxPriorityBehavior;          // If set to TRUE, the disk may fail IO sent with max priority level when it cannot find space for the IO in caching medium.
        UCHAR     OptimalWriteGranularity;      // In LBAs. Value is the power value (of 2). Value 0xFF means that Optimal Write Granularity is not indicated. 
                                                //          For example: value 0 indicates 2^0 = 1 logical sector, 1 indicates 2^1 = 2 logical sectors
        UCHAR     Reserved;

        ULONG     DirtyThresholdLow;            // fraction type of value, with base "FractionBase".
        ULONG     DirtyThresholdHigh;           // fraction type of value, with base "FractionBase".

        struct {
            ULONG   CacheDisable                : 1;    // support of disabling the caching medium
            ULONG   SetDirtyThreshold           : 1;    // support of Setting dirty threshold for the entire caching medium
            ULONG   PriorityDemoteBySize        : 1;    // support of demote by size command
            ULONG   PriorityChangeByLbaRange    : 1;    // support of change by lba command
            ULONG   Evict                       : 1;    // support of evict command
            ULONG   ReservedBits                : 27;

            ULONG   MaxEvictCommands;                   // Max outstanding Evict commands concurrently. Only value when "Evict" value is 1.

            ULONG   MaxLbaRangeCountForEvict;           // Count of LBA ranges can be associated with evict command. Only value when "Evict" value is 1.
            ULONG   MaxLbaRangeCountForChangeLba;       // Count of LBA ranges associated with PriorityChangeByLbaRange command. Only value when "PriorityChangeByLbaRange" value is 1.
        } SupportedCommands;

        NVCACHE_PRIORITY_LEVEL_DESCRIPTOR   Priority[0];

    } Priorities;

} HYBRID_INFORMATION, *PHYBRID_INFORMATION;


//
// Non Volatile Cache support
//

#define IOCTL_SCSI_MINIPORT_NVCACHE           ((FILE_DEVICE_SCSI << 16) + 0x0600)

//
// Hybrid Device support
//
#define IOCTL_SCSI_MINIPORT_HYBRID            ((FILE_DEVICE_SCSI << 16) + 0x0620)

//
// Firmware upgrade support
//
#define IOCTL_SCSI_MINIPORT_FIRMWARE          ((FILE_DEVICE_SCSI << 16) + 0x0780)

//
// Diagnostic support
//
#define IOCTL_SCSI_MINIPORT_DIAGNOSTIC        ((FILE_DEVICE_SCSI << 16) + 0x0900)

//
// Define the SCSI pass through structure.
//




ULONG
FORCEINLINE
StorPortLogTelemetry(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS StorAddress,
    _In_ PSTORPORT_TELEMETRY_EVENT Event
)
/*
Description:

    A miniport can call this function to log a tracelogging telemetry event with
    miniport customized data, which encapsulated in the EventBuffer structure.

Parameters:

    HwDeviceExtension - The miniport's device extension.

    Address - NULL if the device is an adapter, otherwise the address specifies
            the unit object.

    Event - Telemetry data that includes standard event fields and miniport
            payload, which both general buffer and name/value pairs.

Returns:

    STOR_STATUS_SUCCESS if the telemetry event was successfully logged.

    STOR_STATUS_NOT_IMPLEMENTED if the API is called on the OS that not support it.

    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)

    status = StorPortExtendedFunction(ExtFunctionMiniportTelemetry,
                                      HwDeviceExtension,
                                      StorAddress,
                                      Event);

#else

    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(StorAddress);
    UNREFERENCED_PARAMETER(Event);

#endif

    return status;
}







ULONG
FORCEINLINE
StorPortEtwEvent8(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG EventId,
    _In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter3Name,
    _In_ ULONGLONG Parameter3Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter4Name,
    _In_ ULONGLONG Parameter4Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter5Name,
    _In_ ULONGLONG Parameter5Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter6Name,
    _In_ ULONGLONG Parameter6Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter7Name,
    _In_ ULONGLONG Parameter7Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter8Name,
    _In_ ULONGLONG Parameter8Value
)
/*
Description:
    A miniport can call this function to log an ETW event with eight extra
    general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than STORPORT_ETW_MAX_DESCRIPTION_LENGTH characters,
        not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than STORPORT_ETW_MAX_PARAM_NAME_LENGTH characters, not including
        the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successfully logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)

    status = StorPortExtendedFunction(ExtFunctionMiniportChannelEtwEvent8,
                                    HwDeviceExtension,
                                    Address,
                                    StorportEtwEventDiagnostic,
                                    EventId,
                                    EventDescription,
                                    EventKeywords,
                                    EventLevel,
                                    EventOpcode,
                                    Srb,
                                    Parameter1Name,
                                    Parameter1Value,
                                    Parameter2Name,
                                    Parameter2Value,
                                    Parameter3Name,
                                    Parameter3Value,
                                    Parameter4Name,
                                    Parameter4Value,
                                    Parameter5Name,
                                    Parameter5Value,
                                    Parameter6Name,
                                    Parameter6Value,
                                    Parameter7Name,
                                    Parameter7Value,
                                    Parameter8Name,
                                    Parameter8Value);
#elif (NTDDI_VERSION >= NTDDI_WINBLUE)

    status = StorPortExtendedFunction(ExtFunctionMiniportEtwEvent8,
                                    HwDeviceExtension,
                                    Address,
                                    EventId,
                                    EventDescription,
                                    EventKeywords,
                                    EventLevel,
                                    EventOpcode,
                                    Srb,
                                    Parameter1Name,
                                    Parameter1Value,
                                    Parameter2Name,
                                    Parameter2Value,
                                    Parameter3Name,
                                    Parameter3Value,
                                    Parameter4Name,
                                    Parameter4Value,
                                    Parameter5Name,
                                    Parameter5Value,
                                    Parameter6Name,
                                    Parameter6Value,
                                    Parameter7Name,
                                    Parameter7Value,
                                    Parameter8Name,
                                    Parameter8Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
    UNREFERENCED_PARAMETER(Parameter3Name);
    UNREFERENCED_PARAMETER(Parameter3Value);
    UNREFERENCED_PARAMETER(Parameter4Name);
    UNREFERENCED_PARAMETER(Parameter4Value);
    UNREFERENCED_PARAMETER(Parameter5Name);
    UNREFERENCED_PARAMETER(Parameter5Value);
    UNREFERENCED_PARAMETER(Parameter6Name);
    UNREFERENCED_PARAMETER(Parameter6Value);
    UNREFERENCED_PARAMETER(Parameter7Name);
    UNREFERENCED_PARAMETER(Parameter7Value);
    UNREFERENCED_PARAMETER(Parameter8Name);
    UNREFERENCED_PARAMETER(Parameter8Value);
#endif

    return status;
}






typedef struct _CURRENT_INTERNAL_STATUS_PARAMETER_DATA {

    UCHAR Reserved0[4];
    UCHAR IEEECompanyId[4];
    UCHAR CurrentInternalStatusDataSetOneLength[2];
    UCHAR CurrentInternalStatusDataSetTwoLength[2];
    UCHAR CurrentInternalStatusDataSetThreeLength[2];
    UCHAR CurrentInternalStatusDataSetFourLength[4];
    UCHAR Reserved1[364];
    UCHAR NewSavedDataAvailable;
    UCHAR SavedDataGenerationNumber;
    UCHAR CurrentReasonIdentifier[128];

    UCHAR CurrentInternalStatusData[ANYSIZE_ARRAY];

} CURRENT_INTERNAL_STATUS_PARAMETER_DATA, *PCURRENT_INTERNAL_STATUS_PARAMETER_DATA;

// Flags to indicate Adapter support
#define STOR_ADAPTER_FEATURE_DEVICE_TELEMETRY               0x00000001  // Indicating that miniport driver supports storage device telemetry.
#define STOR_ADAPTER_FEATURE_STOP_UNIT_DURING_POWER_DOWN    0x00000002  // Indicating the adapter wants STOP_UNIT command during system shutdown
#define STOR_ADAPTER_UNCACHED_EXTENSION_NUMA_NODE_PREFERRED 0x00000004  // Indicating that miniport driver wants UncachedExtension to be allocated from adapter NUMA node.
#define STOR_ADAPTER_DMA_V3_PREFERRED                       0x00000008  // Indicating that miniport driver prefers to use DMA V3 kernel API for the adapter.
#define STOR_ADAPTER_FEATURE_ABORT_COMMAND                  0x00000010  // Indicating the miniport driver supports the ability to abort an outstanding command via SRB_FUNCTION_ABORT_COMMAND.
#define STOR_ADAPTER_FEATURE_RICH_TEMPERATURE_THRESHOLD     0x00000020  // Indicating the adapter supports richer temperature threshold information than defined in SCSI SPC4 spec.
#define STOR_ADAPTER_DMA_ADDRESS_WIDTH_SPECIFIED            0x00000040  // Indicating that miniport driver specified DMA address width for the adapter.


typedef struct _ERROR_HISTORY_DIRECTORY_ENTRY {

    UCHAR SupportedBufferId;
    UCHAR BufferFormat;
    UCHAR BufferSource : 4;
    UCHAR Reserved0 : 4;
    UCHAR Reserved1;
    UCHAR MaxAvailableLength[4];

} ERROR_HISTORY_DIRECTORY_ENTRY, *PERROR_HISTORY_DIRECTORY_ENTRY;


typedef struct _ERROR_HISTORY_DIRECTORY {

    UCHAR T10VendorId[8];
    UCHAR ErrorHistoryVersion;
    UCHAR ClearSupport : 1;
    UCHAR ErrorHistorySource : 2;
    UCHAR ErrorHistoryRetrieved : 2;
    UCHAR Reserved0 : 3;
    UCHAR Reserved1[20];
    UCHAR DirectoryLength[2];

    ERROR_HISTORY_DIRECTORY_ENTRY ErrorHistoryDirectoryList[ANYSIZE_ARRAY];

} ERROR_HISTORY_DIRECTORY, *PERROR_HISTORY_DIRECTORY;

ULONG
FORCEINLINE
StorPortSetPowerSettingNotificationGuids (
    _In_                   PVOID  HwDeviceExtension,
    _In_                   ULONG  GuidCount,
    _In_reads_(GuidCount) LPGUID Guid
    )
{
    return StorPortExtendedFunction(ExtFunctionSetPowerSettingNotificationGuids,
                                    HwDeviceExtension,
                                    GuidCount,
                                    Guid);
}

ULONG
FORCEINLINE
StorPortMarkDumpMemory(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Address,
    _In_ ULONG_PTR Length,
    _In_ ULONG Flags
    )
{
    return StorPortExtendedFunction(ExtFunctionMarkDumpMemory,
                                    HwDeviceExtension,
                                    Address,
                                    Length,
                                    Flags);
}



VOID
FORCEINLINE
StorPortMarkDeviceFailed(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS StorAddress,
    _In_ ULONG Flags,
    _In_ PWSTR FailReason
    )
/*

Description:

    A miniport can call this function to mark a failed device.

    Afer this call, PnP manager will send IRP to query device PnP state, then try
    to remove the failed device.

Parameters:

    HwDeviceExtension - The miniport's device extension.

    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.

    Flags - Indicates the specific behavior that miniport wants to do.

    FailReason - Indicates device failure reason and details.

Returns:

    None.

*/
{
#if (NTDDI_VERSION > NTDDI_WIN10_19H1)

    StorPortMarkDeviceFailedEx(HwDeviceExtension,
                               StorAddress,
                               Flags,
                               0xFFFF,
                               FailReason,
                               0,
                               NULL,
                               0,
                               NULL);

#elif (NTDDI_VERSION == NTDDI_WIN10_19H1)

    StorPortNotification(MarkDeviceFailed,
                         HwDeviceExtension,
                         StorAddress,
                         Flags,
                         FailReason);

#else

    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(StorAddress);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(FailReason);

#endif
}

ULONG
FORCEINLINE
StorPortSetUnitAttributes(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_ADDRESS Address,
    _In_ STOR_UNIT_ATTRIBUTES Attributes
    )
{
    return StorPortExtendedFunction(ExtFunctionSetUnitAttributes,
                                    HwDeviceExtension,
                                    Address,
                                    Attributes);
}




VOID
FORCEINLINE
StorPortMarkDeviceFailed(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS StorAddress,
    _In_ ULONG Flags,
    _In_ PWSTR FailReason
    );

//
// VPD Page 0x86, Extended INQUIRY Data
//
typedef struct _VPD_EXTENDED_INQUIRY_DATA_PAGE {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;

    UCHAR PageCode;         // 86h
    UCHAR PageLength[2];    // [0] - 00h, [1] - 3Ch

    UCHAR RefChk : 1;       // byte 4 bit 0
    UCHAR AppChk : 1;
    UCHAR GrdChk : 1;
    UCHAR Spt : 3;
    UCHAR ActivateMicrocode : 2;

    UCHAR SimpSup : 1;      // byte 5 bit 0
    UCHAR OrdSup : 1;
    UCHAR HeadSup : 1;
    UCHAR PriorSup : 1;
    UCHAR GroupSup : 1;
    UCHAR UaskSup : 1;
    UCHAR Reserved0 : 2;

    UCHAR VSup : 1;         // byte 6 bit 0
    UCHAR NvSup : 1;
    UCHAR Obsolete0 : 1;
    UCHAR WuSup : 1;
    UCHAR Reserved1 : 4;

    UCHAR LuiClr : 1;       // byte 7 bit 0
    UCHAR Reserved2 : 3;
    UCHAR PiiSup : 1;
    UCHAR NoPiChk : 1;
    UCHAR Reserved3 : 2;

    UCHAR Obsolete1 : 1;    // byte 8 bit 0
    UCHAR HssRelef : 1;
    UCHAR Reserved4 : 1;
    UCHAR RtdSup : 1;
    UCHAR RSup : 1;
    UCHAR LuCollectionType : 3;

    UCHAR Multi_i_t_Nexus_Microcode_Download : 4;   // byte 9 bit 0
    UCHAR Reserved5 : 4;

    UCHAR ExtendedSelfTestCompletionMinutes[2];

    UCHAR Reserved6 : 5;    // byte 12 bit 0
    UCHAR VsaSup : 1;
    UCHAR HraSup : 1;
    UCHAR PoaSup : 1;

    UCHAR MaxSupportedSenseDataLength;

    UCHAR Nrd0 : 1;         // byte 14 bit 0
    UCHAR Nrd1 : 1;
    UCHAR Sac : 1;
    UCHAR Reserved7 : 3;
    UCHAR Ias : 1;
    UCHAR Ibs : 1;

    UCHAR MaxInquiryChangeLogs[2];
    UCHAR MaxModePageChangeLogs[2];

    UCHAR Reserved8[45];
} VPD_EXTENDED_INQUIRY_DATA_PAGE, *PVPD_EXTENDED_INQUIRY_DATA_PAGE;

ULONG
FORCEINLINE
StorPortEtwChannelEvent8(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ STORPORT_ETW_EVENT_CHANNEL EventChannel,
    _In_ ULONG EventId,
    _In_reads_or_z_(STORPORT_ETW_MAX_DESCRIPTION_LENGTH) PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter3Name,
    _In_ ULONGLONG Parameter3Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter4Name,
    _In_ ULONGLONG Parameter4Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter5Name,
    _In_ ULONGLONG Parameter5Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter6Name,
    _In_ ULONGLONG Parameter6Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter7Name,
    _In_ ULONGLONG Parameter7Value,
    _In_reads_or_z_opt_(STORPORT_ETW_MAX_PARAM_NAME_LENGTH) PWSTR Parameter8Name,
    _In_ ULONGLONG Parameter8Value
)
/*
Description:
    A miniport can call this function to log an ETW event to a specific channel
    with eight extra general purpose parameters (expressed as name-value pairs).

Parameters:
    HwDeviceExtension - The miniport's device extension.
    Address - NULL if the device is an adapter, otherwise the address specifies
        the unit object.
    EventChannel - ETW channel where event is logged
    EventId - A miniport-specific event ID to uniquely identify the type of event.
    EventDescription - Required.  A short string describing the event.  Must
        not be longer than STORPORT_ETW_MAX_DESCRIPTION_LENGTH characters,
        not including the NULL terminator.
    EventKeywords - Bitmask of STORPORT_ETW_EVENT_KEYWORD_* values to further
        characterize the event.  Can be 0 if no keywords are desired.
    EventLevel - The level of the event (e.g. Informational, Error, etc.).
    EventOpcode - The opcode of the event (e.g. Info, Start, Stop, etc.).
    Srb - Optional pointer to an SRB.  If specified, the SRB pointer and the
        pointer of the associated IRP will be logged.
    Parameter<N>Name - A short string that gives meaning to parameter N's value.
        If NULL or an empty string, parameter N will be ignored.  Must not be
        longer than STORPORT_ETW_MAX_PARAM_NAME_LENGTH characters, not including
        the NULL terminator.
    Parameter<N>Value - Value of parameter N.  If the associated parameter N
        name is NULL or empty, the value will be logged as 0.

Returns:
    STOR_STATUS_SUCCESS if the ETW event was successfully logged.
    STOR_STATUS_INVALID_PARAMETER if there is an invalid parameter. This is
        typically returned if a passed-in string has too many characters.
    STOR_STATUS_UNSUCCESSFUL may also be returned for other, internal reasons.

*/
{
    ULONG status = STOR_STATUS_NOT_IMPLEMENTED;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)

    status = StorPortExtendedFunction(ExtFunctionMiniportChannelEtwEvent8,
                                    HwDeviceExtension,
                                    Address,
                                    EventChannel,
                                    EventId,
                                    EventDescription,
                                    EventKeywords,
                                    EventLevel,
                                    EventOpcode,
                                    Srb,
                                    Parameter1Name,
                                    Parameter1Value,
                                    Parameter2Name,
                                    Parameter2Value,
                                    Parameter3Name,
                                    Parameter3Value,
                                    Parameter4Name,
                                    Parameter4Value,
                                    Parameter5Name,
                                    Parameter5Value,
                                    Parameter6Name,
                                    Parameter6Value,
                                    Parameter7Name,
                                    Parameter7Value,
                                    Parameter8Name,
                                    Parameter8Value);
#else
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(EventChannel);
    UNREFERENCED_PARAMETER(EventId);
    UNREFERENCED_PARAMETER(EventDescription);
    UNREFERENCED_PARAMETER(EventKeywords);
    UNREFERENCED_PARAMETER(EventLevel);
    UNREFERENCED_PARAMETER(EventOpcode);
    UNREFERENCED_PARAMETER(Srb);
    UNREFERENCED_PARAMETER(Parameter1Name);
    UNREFERENCED_PARAMETER(Parameter1Value);
    UNREFERENCED_PARAMETER(Parameter2Name);
    UNREFERENCED_PARAMETER(Parameter2Value);
    UNREFERENCED_PARAMETER(Parameter3Name);
    UNREFERENCED_PARAMETER(Parameter3Value);
    UNREFERENCED_PARAMETER(Parameter4Name);
    UNREFERENCED_PARAMETER(Parameter4Value);
    UNREFERENCED_PARAMETER(Parameter5Name);
    UNREFERENCED_PARAMETER(Parameter5Value);
    UNREFERENCED_PARAMETER(Parameter6Name);
    UNREFERENCED_PARAMETER(Parameter6Value);
    UNREFERENCED_PARAMETER(Parameter7Name);
    UNREFERENCED_PARAMETER(Parameter7Value);
    UNREFERENCED_PARAMETER(Parameter8Name);
    UNREFERENCED_PARAMETER(Parameter8Value);
#endif

    return status;
}

ULONG
FORCEINLINE
StorPortGetRequestInfo (
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PSTOR_REQUEST_INFO RequestInfo
    )
{
    return StorPortExtendedFunction(ExtFunctionGetRequestInfo,
                                    HwDeviceExtension,
                                    Srb,
                                    RequestInfo);
}

ULONG
FORCEINLINE
StorPortQueryPerformanceCounter(
    _In_ PVOID HwDeviceExtension,
    _Out_opt_  PLARGE_INTEGER PerformanceFrequency,
    _Out_ PLARGE_INTEGER PerformanceCounter
    )
{
    return StorPortExtendedFunction(ExtFunctionQueryPerformanceCounter,
                                    HwDeviceExtension,
                                    PerformanceFrequency,
                                    PerformanceCounter);
}


ULONG StorPortStateChangeDetected(
  PVOID            HwDeviceExtension,
  ULONG            ChangedEntity,
  PSTOR_ADDRESS    Address,
  ULONG            Attributes,
  PHW_STATE_CHANGE HwStateChange,
  PVOID            HwStateChangeContext
);


ULONG StorPortInitializeTimer(
  PVOID HwDeviceExtension,
  PVOID *TimerHandle
);

ULONG StorPortRequestTimer(
  PVOID        HwDeviceExtension,
  PVOID        TimerHandle,
  PHW_TIMER_EX TimerCallback,
  PVOID        CallbackContext,
  ULONGLONG    TimerValue,
  ULONGLONG    TolerableDelay
);

ULONG StorPortFreeTimer(
  PVOID HwDeviceExtension,
  PVOID TimerHandle
);

ULONG
FORCEINLINE
StorPortSetUnitAttributes(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_ADDRESS Address,
    _In_ STOR_UNIT_ATTRIBUTES Attributes
    );

/* SUSSY WUSSSY BAKAS */


typedef struct _GP_LOG_NCQ_COMMAND_ERROR {
    UCHAR   NcqTag          : 5;
    UCHAR   Reserved0       : 1;
    UCHAR   UNL             : 1;        // error: IDLE IMMEDIATE with UNLOAD
    UCHAR   NonQueuedCmd    : 1;

    UCHAR   Reserved1;
    UCHAR   Status;
    UCHAR   Error;
    UCHAR   LBA7_0;
    UCHAR   LBA15_8;
    UCHAR   LBA23_16;
    UCHAR   Device;
    UCHAR   LBA31_24;
    UCHAR   LBA39_32;
    UCHAR   LBA47_40;
    UCHAR   Reserved2;
    UCHAR   Count7_0;
    UCHAR   Count15_8;
    UCHAR   SenseKey;
    UCHAR   ASC;
    UCHAR   ASCQ;
    UCHAR   Reserved3[239];
    UCHAR   Vendor[255];
    UCHAR   Checksum;
} GP_LOG_NCQ_COMMAND_ERROR, *PGP_LOG_NCQ_COMMAND_ERROR;

typedef enum _AHCI_ETW_EVENT_IDS {
    AhciEtwEventSystemPowerHint = 0,
    AhciEtwEventUnitHybridGetInfo = 6,
    AhciEtwEventUnitHybridCachingMediumEnable = 7,
    AhciEtwEventUnitHybridCachingMediumDisable = 8,
    AhciEtwEventUnitHybridChangePriorityByLBA = 9,
    AhciEtwEventUnitHybridDemoteBySize = 10,
    AhciEtwEventUnitHybridEvict = 11,
    AhciEtwEventUnitHybridSetDirtyThreshold = 12,
    AhciEtwEventUnitHybridWriteThrough = 13,
    AhciEtwEventUnitFirmwareIoctl = 14,
    AhciEtwEventUnitFirmwareInfo = 15,
    AhciEtwEventUnitFirmwareDownload = 16,
    AhciEtwEventUnitFirmwareDownloadComplete = 17,
    AhciEtwEventUnitFirmwareActivate = 18,
    AhciEtwEventUnitFirmwareActivateComplete = 19,
    AhciEtwEventUnitGetPhysicalElementStatusComplete = 20,
    AhciEtwEventUnitRemoveElementAndTruncateComplete = 21,
    AhciEtwEventUnitGetInternalStatusDataHeaderComplete = 22,
    AhciEtwEventUnitGetInternalStatusDataComplete = 23,
    AhciEtwEventBuildIO = 24,
    AhciEtwEventStartIO = 25,
    AhciEtwEventHandleInterrupt = 26,
    AhciEtwEventPortReset = 27,
    AhciEtwEventIOCompletion = 28
} AHCI_ETW_EVENT_IDS, *PAHCI_ETW_EVENT_IDS;

//
// task file register contents
//
#include <pshpack1.h>
typedef struct _ATAREGISTERS {

    // ATA features/error register
    UCHAR bFeaturesReg;

    // sector count
    UCHAR bSectorCountReg;

    // block number (7:0)
    UCHAR bSectorNumberReg;

    // cylinder number (7:0) or LBA (15:8)
    UCHAR bCylLowReg;

    // cylinder number (15:8) or LBA (23:16)
    UCHAR bCylHighReg;

    // device/Head and LBA (27:24)
    UCHAR bDriveHeadReg;

    // command/status register
    UCHAR bCommandReg;

    // Reserved for future use. Shall be 0
    UCHAR bReserved;

} ATAREGISTERS, *PATAREGISTERS;
#include <poppack.h>

//
// task file (to hold 48 bit)
//
typedef struct _ATA_TASK_FILE {
    ATAREGISTERS Current;
    ATAREGISTERS Previous;
} ATA_TASK_FILE, *PATA_TASK_FILE;

// helper functions for setting TaskFile fields
#define SetFeaturesReg(reg, val)      (reg->bFeaturesReg = val)
#define SetSectorCount(reg, val)      (reg->bSectorCountReg = val)
#define SetSectorNumber(reg, val)     (reg->bSectorNumberReg = val)
#define SetCylinderLow(reg, val)      (reg->bCylLowReg = val)
#define SetCylinderHigh(reg, val)     (reg->bCylHighReg = val)
#define SetDeviceReg(reg, val)        (reg->bDriveHeadReg |= (0xA0 | val)) // only have Device0 for SATA
#define SetCommandReg(reg, val)       (reg->bCommandReg = val)

#define ATA_NCQ_FUA_BIT         (1 << 7)

//
// Device type
//
typedef enum  {
    DeviceUnknown = 0,
    DeviceIsAta,
    DeviceIsAtapi,
    DeviceNotExist
} ATA_DEVICE_TYPE;

//
// addressing mode
//
typedef enum {
    UnknownMode = 0,
    LbaMode,
    Lba48BitMode
} ATA_ADDRESS_TRANSLATION;


typedef struct _ATA_DEVICE_PARAMETERS {

    ATA_DEVICE_TYPE AtaDeviceType;  // device type (ata, atapi etc)
    UCHAR           ScsiDeviceType; // Scsi device type from inquiry data; Ata device: DIRECT_ACCESS_DEVICE

    UCHAR           MaximumLun;

    UCHAR           MaxDeviceQueueDepth;  // number of requests the device can handle at a time

    struct {
        ULONG   RemovableMedia: 1;
        ULONG   SystemPoweringDown: 1;
        ULONG   FuaSupported: 1;
        ULONG   FuaSucceeded: 1;

        ULONG   NeedUpdateIdentifyDeviceData: 1;
    } StateFlags;

    ULONG   BytesPerLogicalSector;
    ULONG   BytesPerPhysicalSector;
    ULONG   BytesOffsetForSectorAlignment;

    // Geometry
    ATA_ADDRESS_TRANSLATION AddressTranslation;

    LARGE_INTEGER           MaxLba;

    // Strings used to construct device ids, +1 for 'NULL'
    UCHAR   VendorId[40 + 1];
    UCHAR   RevisionId[8 + 1];
    UCHAR   SerialNumber[20 + 1];

    // max number of blocks that can be transferred for DSM - Trim command
    ULONG   DsmCapBlockCount;

} ATA_DEVICE_PARAMETERS, *PATA_DEVICE_PARAMETERS;

typedef struct _ATA_LBA_RANGE {
    ULONGLONG   StartSector:48;
    ULONGLONG   SectorCount:16;
} ATA_LBA_RANGE, *PATA_LBA_RANGE;

typedef struct _ATA_TRIM_CONTEXT {
    // Block Descriptor for UNMAP request
    PUNMAP_BLOCK_DESCRIPTOR BlockDescriptors;

    // Block Descriptor count for UNMAP request
    ULONG   BlockDescrCount;

    // count of ATA Lba Range entries can be sent by one Trim command
    ULONG   MaxLbaRangeEntryCountPerCmd;

    // count of ATA Lba Range entries need for an UNMAP request
    //    max Block Descriptors carried by one Unmap command: 0xFFFE; max ATA Lba Ranges need for a Unmap Block Descriptor: 0xFFFFFFFF/0xFFFF
    //    ULONG is big enough to hold the value.
    ULONG   NeededLbaRangeEntryCount;

    // the length of allocated. Need to cache the value for calling AhciFreeDmaBuffer
    ULONG   AllocatedBufferLength;

    // count of ATA Lba Range entries that already be processed.
    ULONG   ProcessedLbaRangeEntryCount;

    // current Index of the input UNMAP Block Descriptors
    ULONG   BlockDescrIndex;

    // current UNMAP Block Descriptor being processed
    UNMAP_BLOCK_DESCRIPTOR  CurrentBlockDescr;

} ATA_TRIM_CONTEXT, *PATA_TRIM_CONTEXT;

typedef struct _HYBRID_CHANGE_BY_LBA_CONTEXT {
    // Data Set Ranges of DSM CHANGE BY LBA request
    PDEVICE_DATA_SET_RANGE DataSetRanges;

    // Data Set Range count of DSM CHANGE BY LBA request
    ULONG       DataSetRangeCount;

    // count of ATA Lba Range entries need for one DSM CHANGE BY LBA request
    ULONG       NeededLbaRangeEntryCount;

    // count of ATA Lba Range entries that already be processed.
    ULONG       ProcessedLbaRangeEntryCount;

    // current Index of the input Data Set Ranges of DSM CHANGE BY LBA request
    ULONG       DataSetRangeIndex;

    // current Data Set Range (in sectors) being processed
    ULONGLONG   CurrentRangeStartLba;
    ULONGLONG   CurrentRangeLbaCount;

    UCHAR       TargetPriority;
    UCHAR       Reserved[7];
} HYBRID_CHANGE_BY_LBA_CONTEXT, *PHYBRID_CHANGE_BY_LBA_CONTEXT;

typedef struct _HYBRID_EVICT_CONTEXT {
    // Data Set Ranges of DSM EVICT request
    PDEVICE_DATA_SET_RANGE DataSetRanges;

    // Data Set Range count of DSM EVICT request
    ULONG   DataSetRangeCount;

    // count of ATA Lba Range entries can be sent by one Evict command
    ULONG   MaxLbaRangeEntryCountPerCmd;

    // count of ATA Lba Range entries need for an Evict request
    ULONG   NeededLbaRangeEntryCount;

    // the length of allocated. Need to cache the value for calling AhciFreeDmaBuffer
    ULONG   AllocatedBufferLength;

    // count of ATA Lba Range entries that already be processed.
    ULONG   ProcessedLbaRangeEntryCount;

    // current Index of the input DSM EVICT Data Set Ranges
    ULONG   DataSetRangeIndex;

    // current DSM EVICT Data Set Range (in sectors) being processed
    ULONGLONG CurrentRangeStartLba;
    ULONGLONG CurrentRangeLbaCount;

} HYBRID_EVICT_CONTEXT, *PHYBRID_EVICT_CONTEXT;

__inline
BOOLEAN
IsUnknownDevice(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->AtaDeviceType == DeviceUnknown);
}

__inline
BOOLEAN
IsAtapiDevice(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->AtaDeviceType == DeviceIsAtapi);
}

__inline
BOOLEAN
IsAtaDevice(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->AtaDeviceType == DeviceIsAta);
}

__inline
BOOLEAN
IsRemovableMedia(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->StateFlags.RemovableMedia == 1);
}



__inline
BOOLEAN
Support48Bit(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->AddressTranslation == Lba48BitMode);
}


__inline
ULONG
BytesPerLogicalSector(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return DeviceParameters->BytesPerLogicalSector;
}

__inline
ULONG64
MaxUserAddressableLba(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->MaxLba.QuadPart);
}

__inline
PCDB
RequestGetSrbScsiData (
    _In_ PSTORAGE_REQUEST_BLOCK Srb,
    _In_opt_ PULONG             CdbLength,
    _In_opt_ PUCHAR             ScsiStatus,
    _In_opt_ PVOID*             SenseInfoBuffer,
    _In_opt_ PUCHAR             SenseInfoBufferLength
    )
/*++

Routine Description:

    Helper function to retrieve SCSI related fields from a SRB.

Arguments:

    Srb - Pointer to Srb or SrbEx.

    CdbLength - Pointer to buffer to hold CdbLength field value for CDB

    ScsiStatus - Buffer to hold address ScsiStatus field value.

    SenseInfoBuffer - Pointer to SenseInfoBuffer buffer.

    SenseInfoBufferLength - Pointer to buffer to hold SenseInfoBufferLength value.

Return Value:

    Pointer to Cdb field

--*/
{
    PCDB                    cdb = NULL;

    if (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
        //
        // This is SrbEx - STORAGE_REQUEST_BLOCK
        //
        cdb = SrbGetScsiData(Srb, (PUCHAR)CdbLength, CdbLength, ScsiStatus, SenseInfoBuffer, SenseInfoBufferLength);

    }  else if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {
        //
        // This is legacy SCSI_REQUEST_BLOCK
        //
        PSCSI_REQUEST_BLOCK  srb = (PSCSI_REQUEST_BLOCK)Srb;

        if (CdbLength) {
            *CdbLength = srb->CdbLength;
        }

        if (srb->CdbLength > 0) {
            cdb = (PCDB)srb->Cdb;
        }

        if (ScsiStatus) {
            *ScsiStatus = srb->ScsiStatus;
        }

        if (SenseInfoBuffer) {
            *SenseInfoBuffer = srb->SenseInfoBuffer;
        }

        if (SenseInfoBufferLength) {
            *SenseInfoBufferLength = srb->SenseInfoBufferLength;
        }

    } else {
        if (CdbLength) {
            *CdbLength = 0;
        }

        if (ScsiStatus) {
            *ScsiStatus = SCSISTAT_GOOD;
        }

        if (SenseInfoBuffer) {
            *SenseInfoBuffer = NULL;
        }

        if (SenseInfoBufferLength) {
            *SenseInfoBufferLength = 0;
        }
    }

    return cdb;
}


__inline
ULONG64
GetLbaFromCdb(
    _In_ PCDB  Cdb,
    _In_ ULONG CdbLength
    )
{
    LARGE_INTEGER lba;

    lba.QuadPart = 0;

    if (CdbLength == 0x10) {
        // 16 bytes CDB
        REVERSE_BYTES_QUAD((&lba.QuadPart), Cdb->CDB16.LogicalBlock);
    } else {
        lba.LowPart = (ULONG)(Cdb->CDB10.LogicalBlockByte3 |
                              Cdb->CDB10.LogicalBlockByte2 << 8 |
                              Cdb->CDB10.LogicalBlockByte1 << 16 |
                              Cdb->CDB10.LogicalBlockByte0 << 24 );
        lba.HighPart = 0;
    }

    return (ULONG64)lba.QuadPart;
}

__inline
ULONG
GetSectorCountFromCdb(
    PCDB  Cdb,
    ULONG CdbLength
    )
{
    ULONG sectorCount = 0;

    if (CdbLength == 0x10) {
        // 16 byte cdb
        REVERSE_BYTES(&sectorCount, Cdb->CDB16.TransferLength);

    } else {
        sectorCount = (ULONG)(Cdb->CDB10.TransferBlocksMsb << 8 |
                              Cdb->CDB10.TransferBlocksLsb);
    }

    return sectorCount;
}

//
// Util routines
//
VOID
__inline
ByteSwap (
    _Inout_updates_bytes_(Length) PUCHAR Buffer,
    _In_                   ULONG Length
    )
/*++

Routine Description:

    Swap the bytes in the buffer.

Arguments:

    Buffer
    Length

Return Value:

    None.

--*/
{
    UCHAR temp;
    ULONG i;

    if (Length == 0) {
        return;
    }

    // swap the bytes
    for (i=0; i < Length-1; i+=2) {
        temp = Buffer[i];
        Buffer[i] = Buffer[i+1];
#ifdef _MSC_VER
#pragma warning (suppress: 6386) // i is within bound
#endif
        Buffer[i+1] = temp;
    }

    return;
}

ULONG
__inline
RemoveTrailingBlanks (
    _Inout_updates_z_(Length) PUCHAR Buffer,
    _In_                   ULONG Length
    )
/*++

Routine Description:

    Remove trailing blanks from the buffer and null terminates it

Arguments:

    Buffer
    Length

Return Value:

    Returns the length of the string excluding the null character.

--*/
{
    LONG i;

    if (Length < 2) {
        return 0;
    }

    Buffer[Length-1] = '\0';

    // find the first non-space char from the end
    for (i=Length-2; i >= 0; i--) {

        if (Buffer[i] != ' ') {
            break;
        }
    }

    NT_ASSERT((i + 1) >= 0);

    Buffer[i+1] = '\0';

    return (i+1);
}

_Success_(return == STOR_STATUS_SUCCESS)
ULONG
__inline
AhciAllocateDmaBuffer (
    _In_ PVOID   AdapterExtension,
    _In_ ULONG   BufferLength,
    _Post_writable_byte_size_(BufferLength) PVOID* Buffer,
    _Out_ PSTOR_PHYSICAL_ADDRESS PhysicalAddress
    )
{
    ULONG            status;
    PHYSICAL_ADDRESS minPhysicalAddress;
    PHYSICAL_ADDRESS maxPhysicalAddress;
    PHYSICAL_ADDRESS boundaryPhysicalAddress;

    minPhysicalAddress.QuadPart = 0;
    maxPhysicalAddress.QuadPart = 0x7FFFFFFF;   // (2GB - 1)
    boundaryPhysicalAddress.QuadPart = 0;

    status = StorPortAllocateDmaMemory(AdapterExtension,
                                       BufferLength,
                                       minPhysicalAddress,
                                       maxPhysicalAddress,
                                       boundaryPhysicalAddress,
                                       MmCached,
                                       MM_ANY_NODE_OK,
                                       Buffer,
                                       PhysicalAddress);

    return status;
}

_Success_(return == STOR_STATUS_SUCCESS)
ULONG
__inline
AhciFreeDmaBuffer (
    _In_ PVOID      AdapterExtension,
    _In_ ULONG_PTR  BufferLength,
    _In_reads_bytes_(BufferLength) _Post_invalid_ PVOID Buffer,
    _In_opt_ STOR_PHYSICAL_ADDRESS PhysicalAddress
    )
{
    ULONG status;

    status = StorPortFreeDmaMemory(AdapterExtension,
                                   Buffer,
                                   BufferLength,
                                   MmCached,
                                   PhysicalAddress);

    return status;
}

__inline
BOOLEAN
DmaSafeAtapiCommand (
    _In_ UCHAR CdbCommand
    )
{
    ULONG i;
    BOOLEAN useDma;
    static const UCHAR dmaCommandList[] =
        {SCSIOP_READ6,
         SCSIOP_READ,
         SCSIOP_READ_CD,
         SCSIOP_READ_CD_MSF,
         SCSIOP_READ12,
         SCSIOP_WRITE6,
         SCSIOP_WRITE,
         SCSIOP_WRITE12,
         SCSIOP_WRITE_VERIFY,
         SCSIOP_WRITE_VERIFY12,
         0};

    useDma = FALSE;
    i = 0;

    while (dmaCommandList[i]) {
        if (CdbCommand == dmaCommandList[i]) {
            useDma = TRUE;
            break;
        }

        i++;
    }

    return useDma;
}

__inline
BOOLEAN
IsSupportedReadCdb (
    _In_ PCDB   Cdb
    )
{
    return ((Cdb->CDB10.OperationCode == SCSIOP_READ) || (Cdb->CDB10.OperationCode == SCSIOP_READ16));
}

__inline
BOOLEAN
IsSupportedWriteCdb (
    _In_ PCDB   Cdb
    )
{
    return ((Cdb->CDB10.OperationCode == SCSIOP_WRITE) || (Cdb->CDB10.OperationCode == SCSIOP_WRITE16));
}


struct AHCI_CHANNEL_EXTENSION;
typedef struct _AHCI_CHANNEL_EXTENSION
               AHCI_CHANNEL_EXTENSION,
               *PAHCI_CHANNEL_EXTENSION;

typedef struct _AHCI_ADAPTER_EXTENSION  AHCI_ADAPTER_EXTENSION, *PAHCI_ADAPTER_EXTENSION;

ULONG
SCSItoATA(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
SrbConvertToATAPICommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtapiCommonRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtapiInquiryRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtapiModeSenseRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtapiModeSelectRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
SrbConvertToATACommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    );

ULONG
AtaReadWriteRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    );

ULONG
AtaVerifyRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    );

ULONG
AtaModeSenseRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaModeSelectRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaReadCapacityRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    );

ULONG
InquiryComplete(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtaInquiryRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaStartStopUnitRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaTestUnitReadyRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtaMediumRemovalRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaFlushCommandRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtaPassThroughRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaUnmapRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtaSecurityProtocolRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaWriteBufferRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaReportLunsCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PVOID Context
    );

ULONG
AtaGetPhysicalElementStatusRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaRemoveElementAndTruncateRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaGetDeviceCurrentInternalStatusData(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaGetDeviceCurrentInternalStatusDataHeader(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

UCHAR
AtaMapError(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN FUAcommand
    );

VOID
AhciSetSenseData (
    _In_ PSTORAGE_REQUEST_BLOCK Srb,
    _In_ UCHAR                  SrbStatus,
    _In_ UCHAR                  SenseKey,
    _In_ UCHAR                  ASC,
    _In_ UCHAR                  ASCQ
);

VOID
UpdateDeviceParameters(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

VOID
DeviceInitAtapiIds(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PINQUIRYDATA InquiryData
    );

VOID
AhciPortIdentifyDevice(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
  );

ULONG
IOCTLtoATA(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

BOOLEAN
AdapterProcessIOCTL(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
SmartVersion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

BOOLEAN
  FillClippedSGL(
    _In_    PSTOR_SCATTER_GATHER_LIST SourceSgl,
    _Inout_ PSTOR_SCATTER_GATHER_LIST LocalSgl,
    _In_    ULONG BytesLeft,
    _In_    ULONG BytesNeeded
    );

ULONG
SmartIdentifyData(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
SmartGeneric(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
NVCacheGeneric(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
HybridIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
DsmGeneralIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
DatasetManagementIoctl(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
FirmwareIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );


//
// AHCI Telemetry event related.
//
#define AHCI_TELEMETRY_EVENT_VERSION    0x1
#define AHCI_TELEMETRY_DRIVER_VERSION   0x1

#define AHCI_TELEMETRY_FLAG_NOT_SUPPRESS_LOGGING 0x1

typedef enum _AHCI_TELEMETRY_EVENT_ID {
    AhciTelemetryEventIdGeneral = 0,
    AhciTelemetryEventIdPortReset = 1,
    AhciTelemetryEventIdPortRunningStartFail = 2,
    AhciTelemetryEventIdPortErrorRecovery = 3,
    AhciTelemetryEventIdNonqueuedErrorRecovery = 4,
    AhciTelemetryEventIdNCQErrorRecovery = 5,
    AhciTelemetryEventIdNCQErrorRecoveryComplete = 6,
    AhciTelemetryEventIdResetBus = 7,
    AhciTelemetryEventIdResetDeviceRequest = 8,
    AhciTelemetryEventIdSurpriseRemove = 9,
    AhciTelemetryEventIdLpmAdaptiveSetting = 10,
    AhciTelemetryEventIdLpmSettingsModes = 11,
    AhciTelemetryEventIdPortStartSuccess = 12,
    AhciTelemetryEventIdReservedSlotStuck = 13,
    AhciTelemetryEventIdMax = 256
} AHCI_TELEMETRY_EVENT_ID, *PAHCI_TELEMETRY_EVENT_ID;

//
// AHCI mark device failure related.
//
#define AHCI_BUS_CHANGE_WARNING_THROTTLE_MASK           (0x1 << 0)
#define AHCI_BUS_CHANGE_COUNT_WARNING_THRESHOLD         (20)

#define AHCI_NCQ_ERROR_WARNING_THROTTLE_MASK            (0x1 << 1)
#define AHCI_NCQ_ERROR_COUNT_WARNING_THRESHOLD          (100)

#define AHCI_NON_QUEUED_ERROR_WARNING_THROTTLE_MASK     (0x1 << 2)
#define AHCI_NON_QUEUED_ERROR_COUNT_WARNING_THRESHOLD   (100)

#define AHCI_DEVICE_STUCK_WARNING_THROTTLE_MASK         (0x1 << 3)

typedef enum _AHCI_DEVICE_FAILURE_REASON {
    AhciDeviceFailureUnspecific = 0,
    AhciDeviceFailureTooManyBusChange = 1,
    AhciDeviceFailureTooManyNCQError = 2,
    AhciDeviceFailureTooManyNonQueuedError = 3,
    AhciDeviceFailureDeviceStuck = 4
} AHCI_DEVICE_FAILURE_REASON, *PAHCI_DEVICE_FAILURE_REASON;

#if _MSC_VER >= 1200
#pragma warning(pop)
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif
