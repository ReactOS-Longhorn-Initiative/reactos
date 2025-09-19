/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/ntfslib.h
 * PURPOSE:     NTFS lib definitions
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

#ifndef NTFSLIB_H
#define NTFSLIB_H


/* INCLUDES ******************************************************************/

#include <stdlib.h>

#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/umtypes.h>

#include <fmifs/fmifs.h>


/* OTHER MACROSES ************************************************************/

#define KeQuerySystemTime(t)  NtfsGetSystemTimeAsFileTime((LPFILETIME)(t));

#define FREE(p) if (p) RtlFreeHeap(RtlGetProcessHeap(), 0, p);

#define FIRST_ATTRIBUTE(fr) ((PATTR_RECORD)((ULONG_PTR)fr + fr->FirstAttributeOffset))
#define NEXT_ATTRIBUTE(attr) ((PATTR_RECORD)((ULONG_PTR)(attr) + (attr)->Length))

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))


/* BYTES MACROSES ************************************************************/

#define MB_TO_B(x) (x * 1024)

#define GET_BYTE(val, n) (((val) >> (8*(n))) & 0xFF)

#define GET_BYTE_FROM_END(val, n) (((val) >> (8*(sizeof((val)) - 1 - (n)))) & 0xFF)

#define BSWAP16(val) \
 ( (((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00) )

#define BSWAP32(val) \
 ( (((val) >> 24) & 0x000000FF) | (((val) >>  8) & 0x0000FF00) | \
   (((val) <<  8) & 0x00FF0000) | (((val) << 24) & 0xFF000000) )

#define BSWAP64(val) \
 ( (((val) >> 56) & 0x00000000000000FF) | (((val) >> 40) & 0x000000000000FF00) | \
   (((val) >> 24) & 0x0000000000FF0000) | (((val) >>  8) & 0x00000000FF000000) | \
   (((val) <<  8) & 0x000000FF00000000) | (((val) << 24) & 0x0000FF0000000000) | \
   (((val) << 40) & 0x00FF000000000000) | (((val) << 56) & 0xFF00000000000000) )


/* DISK MACROSES *************************************************************/

#define BYTES_PER_SECTOR    (NtfsData.DiskGeometry->BytesPerSector)

#define SECTORS_PER_TRACK   (NtfsData.DiskGeometry->SectorsPerTrack)
#define SECTORS_PER_CLUSTER (GetSectorsPerCluster())
#define BYTES_PER_CLUSTER   ((ULONGLONG)BYTES_PER_SECTOR * (ULONGLONG)SECTORS_PER_CLUSTER)

#define SECTORS_COUNT ( ((ULONGLONG)NtfsData.DiskGeometry->SectorsPerTrack)    * \
                        ((ULONGLONG)NtfsData.DiskGeometry->TracksPerCylinder)  * \
                        ((ULONGLONG)NtfsData.DiskGeometry->Cylinders.QuadPart) )

#define IS_HARD_DRIVE (NtfsData.DiskGeometry->MediaType == FixedMedia)


/* DISK DEFINES **************************************************************/

#define DISK_HEADS             0xFF


/* BOOT SECTOR DEFINES *******************************************************/

#define BPB_HIDDEN_SECTORS  0x3F  // From WinXP

#define OEM_ID              BSWAP64(0x4E54465320202020)
#define EBPB_HEADER         BSWAP32(0x80008000)
#define BOOT_SECTOR_END     BSWAP16(0x55AA)


/* MFT DEFINES ***************************************************************/

#define MFT_ADDRESS         0x0C0000

#define MFT_CLUSTERS_PER_RECORD        0xF6
#define MFT_CLUSTERS_PER_INDEX_RECORD  0x01

#define MFT_RECORD_SIZE     1024


/* OTHER DEFINES *************************************************************/

#define NTFS_MAJOR_VERSION 3
#define NTFS_MINOR_VERSION 1

#define FILE_RECORD_MAGIC  BSWAP32(0x46494C45)

// The beginning and length of an attribute record are always aligned to an 8-byte boundary,
// relative to the beginning of the file record.
#define ATTR_RECORD_ALIGNMENT  8

// FILE_RECORD_END seems to follow AttributeEnd in every file record starting with $Quota.
// No clue what data is being represented here.
#define FILE_RECORD_END  0x11477982

#define FILE_NAME_POSIX          0
#define FILE_NAME_WIN32          1
#define FILE_NAME_DOS            2
#define FILE_NAME_WIN32_AND_DOS  3

#define FILE_TYPE_READ_ONLY  0x1
#define FILE_TYPE_HIDDEN     0x2
#define FILE_TYPE_SYSTEM     0x4
#define FILE_TYPE_ARCHIVE    0x20
#define FILE_TYPE_REPARSE    0x400
#define FILE_TYPE_COMPRESSED 0x800
#define FILE_TYPE_DIRECTORY  0x10000000

#define FILE_CASE_SENSITIVE  FALSE

// Indexed Flag in Resident attributes - still somewhat speculative
#define RA_INDEXED  0x01

#define RA_METAFILES_ATTRIBUTES  (FILE_TYPE_SYSTEM | FILE_TYPE_HIDDEN)
#define RA_HEADER_LENGTH         (FIELD_OFFSET(ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR))

// Metafiles
#define METAFILE_MFT              0
#define METAFILE_MFTMIRR          1
#define METAFILE_LOGFILE          2
#define METAFILE_VOLUME           3
#define METAFILE_ATTRDEF          4
#define METAFILE_ROOT             5
#define METAFILE_BITMAP           6
#define METAFILE_BOOT             7
#define METAFILE_BADCLUS          8
#define METAFILE_SECURE           9
#define METAFILE_UPCASE           10
#define METAFILE_FIRST_USER_FILE  16

#define MFT_DEFAULT_CLUSTERS_SIZE 8
#define MFT_DEFAULT_RECORDS_COUNT (MFT_DEFAULT_CLUSTERS_SIZE * (BYTES_PER_CLUSTER / MFT_RECORD_SIZE))

#define MFT_BITMAP_SIZE    1 
#define MFT_BITMAP_ADDRESS (MFT_ADDRESS - MFT_BITMAP_SIZE)

#define RUN_LIST_ENTRY_HEADER_SIZE 1
#define RUN_LIST_ENTRY_SIZE        8

#define BOOT_SIZE    2
#define BOOT_ADDRESS 0x00

#define UPCASE_SIZE    32
#define UPCASE_ADDRESS 0x03

#define ATTRDEF_SIZE    1
#define ATTRDEF_ADDRESS (MFT_BITMAP_ADDRESS - ATTRDEF_SIZE)

#define LOGFILE_SIZE    11000
#define LOGFILE_ADDRESS (ATTRDEF_ADDRESS - LOGFILE_SIZE)

#define MFT_MIRR_SIZE    1
#define MFT_MIRR_ADDRESS (SECTORS_COUNT / SECTORS_PER_CLUSTER / 2)
#define MFT_MIRR_COUNT   4


/* GLOBAL DATA ***************************************************************/

typedef struct _NtfsFormatData
{
    HANDLE                  DiskHandle;
    GET_LENGTH_INFORMATION* LengthInformation;
    PDISK_GEOMETRY          DiskGeometry;
} NtfsFormatData, *PNtfsFormatData;


/* BOOT SECTOR STRUCTURES ****************************************************/

#include <pshpack1.h>

typedef struct _BIOS_PARAMETERS_BLOCK
{
    USHORT   BytesPerSector;       // 0x0B
    BYTE     SectorsPerCluster;    // 0x0D
    BYTE     Unused0[7];           // 0x0E
    BYTE     MediaId;              // 0x15
    USHORT   Unused1;              // 0x16
    USHORT   SectorsPerTrack;      // 0x18
    USHORT   Heads;                // 0x1A
    DWORD32  HiddenSectorsCount;   // 0x1C
    DWORD32  Unused2;              // 0x20
} BIOS_PARAMETERS_BLOCK, *PBIOS_PARAMETERS_BLOCK;

typedef struct _EXTENDED_BIOS_PARAMETERS_BLOCK
{
    DWORD32    Header;                  // 0x24
    ULONGLONG  SectorCount;             // 0x28
    ULONGLONG  MftLocation;             // 0x30
    ULONGLONG  MftMirrLocation;         // 0x38
    CHAR       ClustersPerMftRecord;    // 0x40
    BYTE       Unused0[3];              // 0x41
    CHAR       ClustersPerIndexRecord;  // 0x44
    BYTE       Unused1[3];              // 0x45
    ULONGLONG  SerialNumber;            // 0x48
    DWORD32    Checksum;                // 0x50, unused
} EXTENDED_BIOS_PARAMETERS_BLOCK, *PEXTENDED_BIOS_PARAMETERS_BLOCK;

typedef struct _BOOT_SECTOR
{
    BYTE                            Jump[3];         // 0x00
    ULARGE_INTEGER                  OEMID;           // 0x03
    BIOS_PARAMETERS_BLOCK           BPB;             // 0x0B
    EXTENDED_BIOS_PARAMETERS_BLOCK  EBPB;            // 0x24
    BYTE                            BootStrap[426];  // 0x54
    USHORT                          EndSector;       // 0x1FE
} BOOT_SECTOR, * PBOOT_SECTOR;

#include <poppack.h>


/* FILES DATA ****************************************************************/

typedef struct _RECORD_HEADER
{
    ULONG      Magic;        // 0x00, magic 'FILE'
    USHORT     UsaOffset;    // 0x04, offset to the update sequence
    USHORT     UsaCount;     // 0x06, size in words of Update Sequence Number & Array (S)
    ULONGLONG  Lsn;          // 0x08, $LogFile Sequence Number
} RECORD_HEADER, *PRECORD_HEADER;

typedef enum _MFT_RECORD_FLAGS
{
    MFT_RECORD_NOT_USED     = 0x0000,
    MFT_RECORD_IN_USE       = 0x0001,
    MFT_RECORD_IS_DIRECTORY = 0x0002
} MFT_RECORD_FLAGS, *PMFT_RECORD_FLAGS;

typedef struct _FILE_RECORD_HEADER
{
    RECORD_HEADER  Header;            // 0x00
    USHORT     SequenceNumber;        // 0x10
    USHORT     HardLinkCount;         // 0x12
    USHORT     FirstAttributeOffset;  // 0x14
    USHORT     Flags;                 // 0x16, flags (see MFT_RECORD_FLAGS)
    ULONG      BytesInUse;            // 0x18, real size of the FILE record
    ULONG      BytesAllocated;        // 0x1C, allocated size of the FILE record
    ULONGLONG  BaseFileRecord;        // 0x20, file reference to the base FILE record
    USHORT     NextAttributeNumber;   // 0x28
    USHORT     Padding;               // 0x2A
    ULONG      MFTRecordNumber;       // 0x2C
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;


/* ATTRIBUTES COMMON *********************************************************/

typedef enum _ATTR_FLAGS
{
    ATTR_IS_COMPRESSED = 0x1,
    ATTR_IS_ENCRYPTED  = 0x4000,
    ATTR_IS_SPARSE     = 0x8000
} ATTR_FLAGS, *PATTR_FLAGS;

typedef enum _ATTRIBUTE_TYPE
{
    AttributeStandardInformation = 0x10,
    AttributeAttributeList       = 0x20,
    AttributeFileName            = 0x30,
    AttributeObjectId            = 0x40,
    AttributeSecurityDescriptor  = 0x50,
    AttributeVolumeName          = 0x60,
    AttributeVolumeInformation   = 0x70,
    AttributeData                = 0x80,
    AttributeIndexRoot           = 0x90,
    AttributeIndexAllocation     = 0xA0,
    AttributeBitmap              = 0xB0,
    AttributeReparsePoint        = 0xC0,
    AttributeEAInformation       = 0xD0,
    AttributeEA                  = 0xE0,
    AttributePropertySet         = 0xF0,
    AttributeLoggedUtilityStream = 0x100,
    AttributeEnd                 = 0xFFFFFFFF
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

typedef struct _ATTR_RECORD
{
    ULONG   Type;           // 0x00
    ULONG   Length;         // 0x04
    UCHAR   IsNonResident;  // 0x08
    UCHAR   NameLength;     // 0x09
    USHORT  NameOffset;     // 0x0A
    USHORT  Flags;          // 0x0C
    USHORT  Instance;       // 0x0E
    union
    {
        struct
        {
            ULONG   ValueLength;  // 0x10
            USHORT  ValueOffset;  // 0x14
            BYTE    Flags;        // 0x16
            BYTE    Reserved;     // 0x17
        } Resident;

        struct
        {
            ULONGLONG  LowestVCN;           // 0x10
            ULONGLONG  HighestVCN;          // 0x18
            USHORT     DataRunsOffset;      // 0x20
            USHORT     CompressionUnit;     // 0x22
            BYTE       Reserved[4];         // 0x24
            LONGLONG   AllocatedSize;       // 0x28
            LONGLONG   DataSize;            // 0x30
            LONGLONG   CompressedSize;     // 0x38
        } NonResident;
    };
} ATTR_RECORD, *PATTR_RECORD;


/* ATTRIBUTES STRUCTURES *****************************************************/

typedef struct _STANDARD_INFORMATION
{
    ULONGLONG CreationTime;
    ULONGLONG ChangeTime;
    ULONGLONG LastWriteTime;
    ULONGLONG LastAccessTime;
    ULONG     FileAttribute;
    ULONG     AlignmentOrReserved[3];
    
    // FIXME: UNIMPLEMENTED
#if 0
    ULONG QuotaId;
    ULONG SecurityId;
    ULONGLONG QuotaCharge;
    USN Usn;
#endif
} STANDARD_INFORMATION, *PSTANDARD_INFORMATION;

typedef struct _FILENAME_ATTRIBUTE
{
    ULONGLONG DirectoryFileReferenceNumber;
    ULONGLONG CreationTime;
    ULONGLONG ChangeTime;
    ULONGLONG LastWriteTime;
    ULONGLONG LastAccessTime;
    ULONGLONG AllocatedSize;
    ULONGLONG DataSize;
    ULONG     FileAttributes;
    union
    {
        struct
        {
            USHORT PackedEaSize;
            USHORT AlignmentOrReserved;
        } EaInfo;

        ULONG ReparseTag;
    } Extended;
    BYTE  NameLength;
    BYTE  NameType;
    WCHAR Name[1];
} FILENAME_ATTRIBUTE, *PFILENAME_ATTRIBUTE;

typedef struct _VOLUME_INFORMATION_ATTRIBUTE
{
    BYTE   Unused[8];
    BYTE   MajorVersion;
    BYTE   MinorVersion;
    USHORT Flags;
} VOLUME_INFORMATION_ATTRIBUTE, *PVOLUME_INFORMATION_ATTRIBUTE;


/* PROTOTYPES ****************************************************************/

ULONG
NTAPI NtGetTickCount(VOID); 

// ntfslib.c

VOID
NtfsGetSystemTimeAsFileTime(OUT PFILETIME lpFileTime);

BYTE GetSectorsPerCluster();

// bootsect.c

NTSTATUS
WriteBootSector();

// attrib.c

VOID
AddStandardInformationAttribute(OUT PFILE_RECORD_HEADER FileRecord, 
                                OUT PATTR_RECORD        Attribute);

VOID
AddFileNameAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                     OUT PATTR_RECORD        Attribute,
                     IN  LPCWSTR             FileName,
                     IN  DWORD32             MftRecordNumber);

VOID
AddEmptyDataAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                      OUT PATTR_RECORD        Attribute);

VOID
AddNonResidentDataAttribute(OUT PFILE_RECORD_HEADER     FileRecord,
                            OUT PATTR_RECORD            Attribute,
                            IN  ULONG                   Address,
                            IN  ULONG                   ClustersCount,
                            OPTIONAL IN  ULONG          DataSize);

VOID
AddMftBitmapAttribute(OUT PFILE_RECORD_HEADER     FileRecord,
                      OUT PATTR_RECORD            Attribute);

VOID
AddEmptyVolumeNameAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                            OUT PATTR_RECORD        Attribute);

VOID
AddVolumeInformationAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                              OUT PATTR_RECORD        Attribute,
                              IN  BYTE                MajorVersion,
                              IN  BYTE                MinorVersion);

VOID
AddIndexRoot(OUT PFILE_RECORD_HEADER FileRecord,
             OUT PATTR_RECORD        Attribute);

VOID
AddIndexAllocation(OUT PFILE_RECORD_HEADER FileRecord,
                   OUT PATTR_RECORD        Attribute);

// files.c

NTSTATUS
WriteMetafiles();

#endif