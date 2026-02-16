/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#ifdef DEFINE_GUID

#ifndef FAR
#define FAR
#endif

DEFINE_GUID(ScsiRawInterfaceGuid,0x53f56309L,0xb6bf,0x11d0,0x94,0xf2,0x00,0xa0,0xc9,0x1e,0xfb,0x8b);
DEFINE_GUID(WmiScsiAddressGuid,0x53f5630fL,0xb6bf,0x11d0,0x94,0xf2,0x00,0xa0,0xc9,0x1e,0xfb,0x8b);
#endif /* DEFINE_GUID */

#ifndef _NTDDSCSIH_
#define _NTDDSCSIH_

#ifdef __cplusplus
extern "C" {
#endif

#define IOCTL_SCSI_BASE     FILE_DEVICE_CONTROLLER

#define DD_SCSI_DEVICE_NAME "\\Device\\ScsiPort"
#define DD_SCSI_DEVICE_NAME_U  L"\\Device\\ScsiPort"

#define IOCTL_SCSI_PASS_THROUGH CTL_CODE(IOCTL_SCSI_BASE,0x0401,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_MINIPORT CTL_CODE(IOCTL_SCSI_BASE,0x0402,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_GET_INQUIRY_DATA CTL_CODE(IOCTL_SCSI_BASE,0x0403,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SCSI_GET_CAPABILITIES CTL_CODE(IOCTL_SCSI_BASE,0x0404,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH_DIRECT CTL_CODE(IOCTL_SCSI_BASE,0x0405,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_GET_ADDRESS CTL_CODE(IOCTL_SCSI_BASE,0x0406,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SCSI_RESCAN_BUS CTL_CODE(IOCTL_SCSI_BASE,0x0407,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SCSI_GET_DUMP_POINTERS CTL_CODE(IOCTL_SCSI_BASE,0x0408,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SCSI_FREE_DUMP_POINTERS CTL_CODE(IOCTL_SCSI_BASE,0x0409,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH_EX CTL_CODE(IOCTL_SCSI_BASE, 0x0411, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH_DIRECT_EX CTL_CODE(IOCTL_SCSI_BASE, 0x0412, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_IDE_PASS_THROUGH CTL_CODE(IOCTL_SCSI_BASE,0x040a,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ATA_PASS_THROUGH CTL_CODE(IOCTL_SCSI_BASE,0x040b,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ATA_PASS_THROUGH_DIRECT CTL_CODE(IOCTL_SCSI_BASE,0x040c,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)

  typedef struct _SCSI_PASS_THROUGH {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG_PTR DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
  }SCSI_PASS_THROUGH,*PSCSI_PASS_THROUGH;

  typedef struct _SCSI_PASS_THROUGH_DIRECT {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
  }SCSI_PASS_THROUGH_DIRECT,*PSCSI_PASS_THROUGH_DIRECT;

#if defined(_WIN64)
  typedef struct _SCSI_PASS_THROUGH32 {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG32 DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
  } SCSI_PASS_THROUGH32,*PSCSI_PASS_THROUGH32;

  typedef struct _SCSI_PASS_THROUGH_DIRECT32 {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    VOID * POINTER_32 DataBuffer;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
  } SCSI_PASS_THROUGH_DIRECT32,*PSCSI_PASS_THROUGH_DIRECT32;
#endif /* _WIN64 */


  typedef struct _SCSI_PASS_THROUGH_EX {
    ULONG Version;
    ULONG Length;
    ULONG CdbLength;
    ULONG StorAddressLength;
    UCHAR ScsiStatus;
    UCHAR SenseInfoLength;
    UCHAR DataDirection;
    UCHAR Reserved;
    ULONG TimeOutValue;
    ULONG StorAddressOffset;
    ULONG SenseInfoOffset;
    ULONG DataOutTransferLength;
    ULONG DataInTransferLength;
    ULONG_PTR DataOutBufferOffset;
    ULONG_PTR DataInBufferOffset;
    UCHAR Cdb[ANYSIZE_ARRAY];
  } SCSI_PASS_THROUGH_EX, *PSCSI_PASS_THROUGH_EX;

typedef struct _SCSI_PASS_THROUGH_DIRECT_EX
{
  ULONG Version;
  ULONG Length;
  ULONG CdbLength;
  ULONG StorAddressLength;
  UCHAR ScsiStatus;
  UCHAR SenseInfoLength;
  UCHAR DataDirection;
  UCHAR Reserved;
  ULONG TimeOutValue;
  ULONG StorAddressOffset;
  ULONG SenseInfoOffset;
  ULONG DataOutTransferLength;
  ULONG DataInTransferLength;
  PVOID DataOutBuffer;
  PVOID DataInBuffer;
  UCHAR Cdb[ANYSIZE_ARRAY];
} SCSI_PASS_THROUGH_DIRECT_EX, *PSCSI_PASS_THROUGH_DIRECT_EX;

#if defined(_WIN64)
typedef struct _SCSI_PASS_THROUGH32_EX
{
  ULONG Version;
  ULONG Length;
  ULONG CdbLength;
  ULONG StorAddressLength;
  UCHAR ScsiStatus;
  UCHAR SenseInfoLength;
  UCHAR DataDirection;
  UCHAR Reserved;
  ULONG TimeOutValue;
  ULONG StorAddressOffset;
  ULONG SenseInfoOffset;
  ULONG DataOutTransferLength;
  ULONG DataInTransferLength;
  ULONG32 DataOutBufferOffset;
  ULONG32 DataInBufferOffset;
  UCHAR Cdb[ANYSIZE_ARRAY];
} SCSI_PASS_THROUGH32_EX, *PSCSI_PASS_THROUGH32_EX;

typedef struct _SCSI_PASS_THROUGH_DIRECT32_EX
{
  ULONG Version;
  ULONG Length;
  ULONG CdbLength;
  ULONG StorAddressLength;
  UCHAR ScsiStatus;
  UCHAR SenseInfoLength;
  UCHAR DataDirection;
  UCHAR Reserved;
  ULONG TimeOutValue;
  ULONG StorAddressOffset;
  ULONG SenseInfoOffset;
  ULONG DataOutTransferLength;
  ULONG DataInTransferLength;
  VOID * POINTER_32 DataOutBuffer;
  VOID * POINTER_32 DataInBuffer;
  UCHAR Cdb[ANYSIZE_ARRAY];
} SCSI_PASS_THROUGH_DIRECT32_EX, *PSCSI_PASS_THROUGH_DIRECT32_EX;
#endif

  typedef struct _ATA_PASS_THROUGH_EX {
    USHORT Length;
    USHORT AtaFlags;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR ReservedAsUchar;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG ReservedAsUlong;
    ULONG_PTR DataBufferOffset;
    UCHAR PreviousTaskFile[8];
    UCHAR CurrentTaskFile[8];
  } ATA_PASS_THROUGH_EX,*PATA_PASS_THROUGH_EX;

  typedef struct _ATA_PASS_THROUGH_DIRECT {
    USHORT Length;
    USHORT AtaFlags;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR ReservedAsUchar;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG ReservedAsUlong;
    PVOID DataBuffer;
    UCHAR PreviousTaskFile[8];
    UCHAR CurrentTaskFile[8];
  } ATA_PASS_THROUGH_DIRECT,*PATA_PASS_THROUGH_DIRECT;

#if defined(_WIN64)

  typedef struct _ATA_PASS_THROUGH_EX32 {
    USHORT Length;
    USHORT AtaFlags;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR ReservedAsUchar;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG ReservedAsUlong;
    ULONG32 DataBufferOffset;
    UCHAR PreviousTaskFile[8];
    UCHAR CurrentTaskFile[8];
  } ATA_PASS_THROUGH_EX32,*PATA_PASS_THROUGH_EX32;

  typedef struct _ATA_PASS_THROUGH_DIRECT32 {
    USHORT Length;
    USHORT AtaFlags;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR ReservedAsUchar;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG ReservedAsUlong;
    VOID * POINTER_32 DataBuffer;
    UCHAR PreviousTaskFile[8];
    UCHAR CurrentTaskFile[8];
  } ATA_PASS_THROUGH_DIRECT32,*PATA_PASS_THROUGH_DIRECT32;
#endif /* _WIN64 */

#define ATA_FLAGS_DRDY_REQUIRED (1 << 0)
#define ATA_FLAGS_DATA_IN (1 << 1)
#define ATA_FLAGS_DATA_OUT (1 << 2)
#define ATA_FLAGS_48BIT_COMMAND (1 << 3)
#define ATA_FLAGS_USE_DMA (1 << 4)
#define ATA_FLAGS_NO_MULTIPLE (1 << 5)

  typedef struct _SCSI_BUS_DATA {
    UCHAR NumberOfLogicalUnits;
    UCHAR InitiatorBusId;
    ULONG InquiryDataOffset;
  }SCSI_BUS_DATA,*PSCSI_BUS_DATA;

  typedef struct _SCSI_ADAPTER_BUS_INFO {
    UCHAR NumberOfBuses;
    SCSI_BUS_DATA BusData[1];
  } SCSI_ADAPTER_BUS_INFO,*PSCSI_ADAPTER_BUS_INFO;

  typedef struct _SCSI_INQUIRY_DATA {
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    BOOLEAN DeviceClaimed;
    ULONG InquiryDataLength;
    ULONG NextInquiryDataOffset;
    UCHAR InquiryData[1];
  }SCSI_INQUIRY_DATA,*PSCSI_INQUIRY_DATA;

  typedef struct _SRB_IO_CONTROL {
    ULONG HeaderLength;
    UCHAR Signature[8];
    ULONG Timeout;
    ULONG ControlCode;
    ULONG ReturnCode;
    ULONG Length;
  } SRB_IO_CONTROL,*PSRB_IO_CONTROL;

  typedef struct _IO_SCSI_CAPABILITIES {
    ULONG Length;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG SupportedAsynchronousEvents;
    ULONG AlignmentMask;
    BOOLEAN TaggedQueuing;
    BOOLEAN AdapterScansDown;
    BOOLEAN AdapterUsesPio;
  } IO_SCSI_CAPABILITIES,*PIO_SCSI_CAPABILITIES;

  typedef struct _SCSI_ADDRESS {
    ULONG Length;
    UCHAR PortNumber;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
  } SCSI_ADDRESS,*PSCSI_ADDRESS;

  struct _ADAPTER_OBJECT;

  typedef struct _DUMP_POINTERS {
    struct _ADAPTER_OBJECT *AdapterObject;
    PVOID MappedRegisterBase;
    PVOID DumpData;
    PVOID CommonBufferVa;
    LARGE_INTEGER CommonBufferPa;
    ULONG CommonBufferSize;
    BOOLEAN AllocateCommonBuffers;
    BOOLEAN UseDiskDump;
    UCHAR Spare1[2];
    PVOID DeviceObject;
  } DUMP_POINTERS,*PDUMP_POINTERS;

/*
 * Data structure and definitions related to IOCTL_SCSI_MINIPORT_FIRMWARE
 */

#define FIRMWARE_FUNCTION_GET_INFO                          0x01
#define FIRMWARE_FUNCTION_DOWNLOAD                          0x02
#define FIRMWARE_FUNCTION_ACTIVATE                          0x03

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
#define FIRMWARE_STATUS_INTERFACE_CRC_ERROR		    0x80
#define FIRMWARE_STATUS_UNCORRECTABLE_DATA_ERROR            0x81
#define FIRMWARE_STATUS_MEDIA_CHANGE                        0x82
#define FIRMWARE_STATUS_ID_NOT_FOUND                        0x83
#define FIRMWARE_STATUS_MEDIA_CHANGE_REQUEST                0x84
#define FIRMWARE_STATUS_COMMAND_ABORT                       0x85
#define FIRMWARE_STATUS_END_OF_MEDIA                        0x86
#define FIRMWARE_STATUS_ILLEGAL_LENGTH                      0x87

#define FIRMWARE_REQUEST_BLOCK_STRUCTURE_VERSION            0x1

typedef struct _FIRMWARE_REQUEST_BLOCK {
    ULONG   Version;
    ULONG   Size;
    ULONG   Function;
    ULONG   Flags;
    ULONG   DataBufferOffset;
    ULONG   DataBufferLength;
} FIRMWARE_REQUEST_BLOCK, *PFIRMWARE_REQUEST_BLOCK;

#define FIRMWARE_REQUEST_FLAG_CONTROLLER                    0x00000001
#define FIRMWARE_REQUEST_FLAG_SWITCH_TO_EXISTING_FIRMWARE   0x80000000

#define STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION         0x1
#define STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION_V2      0x2

#define STORAGE_FIRMWARE_INFO_INVALID_SLOT              0xFF

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
    ULONG   Version;
    ULONG   Size;
    BOOLEAN UpgradeSupport;
    UCHAR   SlotCount;
    UCHAR   ActiveSlot;
    UCHAR   PendingActivateSlot;
    ULONG   Reserved;
    STORAGE_FIRMWARE_SLOT_INFO Slot[0];
} STORAGE_FIRMWARE_INFO, *PSTORAGE_FIRMWARE_INFO;

typedef struct _STORAGE_FIRMWARE_INFO_V2 {
    ULONG   Version;
    ULONG   Size;
    BOOLEAN UpgradeSupport;
    UCHAR   SlotCount;
    UCHAR   ActiveSlot;
    UCHAR   PendingActivateSlot;
    BOOLEAN FirmwareShared;
    UCHAR   Reserved[3];
    ULONG   ImagePayloadAlignment;
    ULONG   ImagePayloadMaxSize;
    STORAGE_FIRMWARE_SLOT_INFO_V2 Slot[0];
} STORAGE_FIRMWARE_INFO_V2, *PSTORAGE_FIRMWARE_INFO_V2;

#define STORAGE_FIRMWARE_DOWNLOAD_STRUCTURE_VERSION         0x1
#define STORAGE_FIRMWARE_DOWNLOAD_STRUCTURE_VERSION_V2      0x2

typedef struct _STORAGE_FIRMWARE_DOWNLOAD {
    ULONG       Version;
    ULONG       Size;
    ULONGLONG   Offset;
    ULONGLONG   BufferSize;
    UCHAR       ImageBuffer[0];
} STORAGE_FIRMWARE_DOWNLOAD, *PSTORAGE_FIRMWARE_DOWNLOAD;

typedef struct _STORAGE_FIRMWARE_DOWNLOAD_V2 {
    ULONG       Version;
    ULONG       Size;
    ULONGLONG   Offset;
    ULONGLONG   BufferSize;
    UCHAR       Slot;
    UCHAR       Reserved[7];
    UCHAR       ImageBuffer[0];
} STORAGE_FIRMWARE_DOWNLOAD_V2, *PSTORAGE_FIRMWARE_DOWNLOAD_V2;

#define STORAGE_FIRMWARE_ACTIVATE_STRUCTURE_VERSION         0x1

typedef struct _STORAGE_FIRMWARE_ACTIVATE {
    ULONG   Version;
    ULONG   Size;
    UCHAR   SlotToActivate;
    UCHAR   Reserved0[3];
} STORAGE_FIRMWARE_ACTIVATE, *PSTORAGE_FIRMWARE_ACTIVATE;

#define SCSI_IOCTL_DATA_OUT 0
#define SCSI_IOCTL_DATA_IN 1
#define SCSI_IOCTL_DATA_UNSPECIFIED 2

#define IOCTL_SCSI_MINIPORT_NVCACHE           ((FILE_DEVICE_SCSI << 16) + 0x0600)
#define IOCTL_SCSI_MINIPORT_HYBRID            ((FILE_DEVICE_SCSI << 16) + 0x0620)
#define IOCTL_SCSI_MINIPORT_DSM               ((FILE_DEVICE_SCSI << 16) + 0x0720)
#define IOCTL_SCSI_MINIPORT_DSM_GENERAL       ((FILE_DEVICE_SCSI << 16) + 0x0721)
#define IOCTL_SCSI_MINIPORT_FIRMWARE          ((FILE_DEVICE_SCSI << 16) + 0x0780)

#ifdef __cplusplus
}
#endif

#endif /* _NTDDSCSIH_ */

