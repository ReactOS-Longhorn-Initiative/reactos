/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/files.c
 * PURPOSE:     NTFS lib
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

/* INCLUDES ******************************************************************/

#include "ntfslib.h"
#include "data.h"

#define NDEBUG
#include <debug.h>

extern NtfsFormatData NtfsData;


/* STRUCTURES ****************************************************************/

typedef struct
{
    ULONG  Number;
    WCHAR* Name;
    PFILE_RECORD_HEADER (*Constructor)();
    NTSTATUS (*AdditionalDataWriter)();
} METAFILE, *PMETAFILE;


/* PROTOTYPES ****************************************************************/

static
NTSTATUS
WriteZerosToClusters(IN  LONGLONG                Address,
                     IN  ULONG                   ClustersCount,
                     OUT PIO_STATUS_BLOCK        IoStatusBlock);

static
NTSTATUS
WriteMetafile(IN  PFILE_RECORD_HEADER      FileRecord,
              OUT PIO_STATUS_BLOCK         IoStatusBlock);

static
NTSTATUS
WriteMetafileMirror(IN  PFILE_RECORD_HEADER      FileRecord,
                    OUT PIO_STATUS_BLOCK         IoStatusBlock);

static
PFILE_RECORD_HEADER
CreateMetafileRecord(IN DWORD32 MftRecordNumber, OUT PATTR_RECORD* Attribute);

static
PFILE_RECORD_HEADER
NtfsCreateBlankFileRecord(IN  DWORD32       MftRecordNumber,
                          OUT PATTR_RECORD* NextAttribute);

static
PFILE_RECORD_HEADER
NtfsCreateEmptyFileRecord(IN DWORD32 MftRecordNumber);

static PFILE_RECORD_HEADER CreateMft();
static PFILE_RECORD_HEADER CreateMftMirr();
static PFILE_RECORD_HEADER CreateLogFile();
static PFILE_RECORD_HEADER CreateVolume();
static PFILE_RECORD_HEADER CreateAttrDef();
static PFILE_RECORD_HEADER CreateRoot();
static PFILE_RECORD_HEADER CreateBitmap();
static PFILE_RECORD_HEADER CreateBoot();
static PFILE_RECORD_HEADER CreateUpCase();
static PFILE_RECORD_HEADER CreateStub(IN DWORD32 MftRecordNumber);

static NTSTATUS WriteMftBitmap();
static NTSTATUS WriteMftMirr();
static NTSTATUS WriteAttributesTable();
static NTSTATUS WriteBitmap();
static NTSTATUS WriteUpCaseTable();


/* CONSTS ********************************************************************/

static const METAFILE METAFILES[] =
{
    { METAFILE_MFT    , L"$MFT"    , CreateMft    , WriteMftBitmap        },
    { METAFILE_MFTMIRR, L"$MFTMirr", CreateMftMirr, WriteMftMirr          },
    { METAFILE_LOGFILE, L"$LogFile", CreateLogFile, NULL                  },  // Partially implemented
    { METAFILE_VOLUME , L"$Volume" , CreateVolume , NULL                  },
    { METAFILE_ATTRDEF, L"$AttrDef", CreateAttrDef, WriteAttributesTable  },
    { METAFILE_ROOT   , L"."       , CreateRoot   , NULL                  },
    { METAFILE_BITMAP , L"$Bitmap" , CreateBitmap , WriteBitmap           },
    { METAFILE_BOOT   , L"$Boot"   , CreateBoot   , NULL                  },
    { METAFILE_BADCLUS, L"$BadClus", NULL         , NULL                  },  // Unimplemented
    { METAFILE_SECURE , L"$Secure" , NULL         , NULL                  },  // Unimplemented
    { METAFILE_UPCASE , L"$UpCase" , CreateUpCase , WriteUpCaseTable      },
    { 11              , L""        , NULL         , NULL                  },  // Reserved
    { 12              , L""        , NULL         , NULL                  },  // Reserved
    { 13              , L""        , NULL         , NULL                  },  // Reserved
    { 14              , L""        , NULL         , NULL                  },  // Reserved
    { 15              , L""        , NULL         , NULL                  },  // Reserved
};


/* FUNCTIONS *****************************************************************/

NTSTATUS
WriteMetafiles()
{
    BYTE MetafileIndex;
    METAFILE Metafile;
    PFILE_RECORD_HEADER FileRecord;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    // Clear first clusters
    Status = WriteZerosToClusters(MFT_ADDRESS,
                                  MFT_DEFAULT_CLUSTERS_SIZE,
                                  &IoStatusBlock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to clear sectors. NtWriteFile() failed (Status %lx)\n", Status);
        return Status;
    }

    // Write metafiles
    for (
        MetafileIndex = 0;
        MetafileIndex < ARR_SIZE(METAFILES);
        MetafileIndex++
    )
    {
        Metafile = METAFILES[MetafileIndex];

        // Create metafile record or stub, if metafile is not implemented
        if (!Metafile.Constructor)
        {
            FileRecord = CreateStub(MetafileIndex);
        }
        else
        {
            FileRecord = Metafile.Constructor();
        }

        // Check file record
        if (!FileRecord)
        {
            DPRINT1(
                "ERROR: Unable to allocate memory for file record #%d!\n",
                MetafileIndex
            );

            Status = STATUS_INSUFFICIENT_RESOURCES;

            FREE(FileRecord);
            break;
        }

        // Write metafile to disk
        Status = WriteMetafile(FileRecord, &IoStatusBlock);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1(
                "ERROR: Unable to write metafile #%d to disk. NtWriteFile() failed (Status %lx)\n",
                MetafileIndex,
                Status
            );

            FREE(FileRecord);
            break;
        }

        // Write additional data to disk
        if (Metafile.AdditionalDataWriter)
        {
            Status = Metafile.AdditionalDataWriter();
            if (!NT_SUCCESS(Status))
            {
                DPRINT1(
                    "ERROR: Unable to write additional data for metafile #%d to disk. Status %lx\n",
                    MetafileIndex,
                    Status
                );

                FREE(FileRecord);
                break;
            }
        }

        // Free memory
        FREE(FileRecord);
    }

    return Status;
}


/* DISK FUNCTIONS ************************************************************/

static
NTSTATUS
WriteZerosToClusters(IN  LONGLONG                Address,
                     IN  ULONG                   ClustersCount,
                     OUT PIO_STATUS_BLOCK        IoStatusBlock)
{
    PBYTE         Zeros;
    ULONG         Size;
    LARGE_INTEGER Offset;
    NTSTATUS      Status;

    Size = BYTES_PER_CLUSTER * ClustersCount;
    Offset.QuadPart = Address * BYTES_PER_CLUSTER;

    Zeros = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Zeros)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Zeros, Size);

    Status = NtWriteFile(NtfsData.DiskHandle,
                         NULL,
                         NULL,
                         NULL,
                         IoStatusBlock,
                         Zeros,
                         Size,
                         &Offset,
                         NULL);

    FREE(Zeros);

    return Status;
}

static
NTSTATUS
WriteMetafile(IN  PFILE_RECORD_HEADER      FileRecord,
              OUT PIO_STATUS_BLOCK         IoStatusBlock)
{
    LARGE_INTEGER Offset;

    // Offset to $MFT + offset to record
    Offset.QuadPart =
        ((LONGLONG)MFT_ADDRESS * BYTES_PER_CLUSTER) +
        (LONGLONG)(FileRecord->MFTRecordNumber * MFT_RECORD_SIZE);

    return NtWriteFile(NtfsData.DiskHandle,
                       NULL,
                       NULL,
                       NULL,
                       IoStatusBlock,
                       FileRecord,
                       MFT_RECORD_SIZE,
                       &Offset,
                       NULL);
}

static
NTSTATUS
WriteMetafileMirror(IN  PFILE_RECORD_HEADER      FileRecord,
                    OUT PIO_STATUS_BLOCK         IoStatusBlock)
{
    LARGE_INTEGER Offset;

    // Offset to $MFT + offset to record
    Offset.QuadPart =
        ((LONGLONG)MFT_MIRR_ADDRESS * BYTES_PER_CLUSTER) +
        (LONGLONG)(FileRecord->MFTRecordNumber * MFT_RECORD_SIZE);

    return NtWriteFile(NtfsData.DiskHandle,
                       NULL,
                       NULL,
                       NULL,
                       IoStatusBlock,
                       FileRecord,
                       MFT_RECORD_SIZE,
                       &Offset,
                       NULL);
}


/* METAFILES FUNCTIONS *******************************************************/


static
PFILE_RECORD_HEADER
CreateMetafileRecord(IN  DWORD32       MftRecordNumber,
                     OUT PATTR_RECORD* Attribute)
{
    PFILE_RECORD_HEADER FileRecord;

    FileRecord = NtfsCreateBlankFileRecord(MftRecordNumber, Attribute);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record #%d!\n", MftRecordNumber);
        return NULL;
    }

    return FileRecord;
}

static
PFILE_RECORD_HEADER
NtfsCreateBlankFileRecord(IN  DWORD32       MftRecordNumber,
                          OUT PATTR_RECORD* NextAttribute)
{
    PFILE_RECORD_HEADER FileRecord;

    // Create empty file record
    FileRecord = NtfsCreateEmptyFileRecord(MftRecordNumber);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record #%d!\n", MftRecordNumber);
        return NULL;
    }

    // $STANDARD_INFORMATION
    (*NextAttribute) = FIRST_ATTRIBUTE(FileRecord);
    AddStandardInformationAttribute(FileRecord, *NextAttribute);

    // $FILE_NAME
    (*NextAttribute) = NEXT_ATTRIBUTE(*NextAttribute);
    AddFileNameAttribute(FileRecord, *NextAttribute, METAFILES[MftRecordNumber].Name, MftRecordNumber);

    (*NextAttribute) = NEXT_ATTRIBUTE(*NextAttribute);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
NtfsCreateEmptyFileRecord(IN DWORD32 MftRecordNumber)
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD NextAttribute;

    // Allocate memory for file record
    FileRecord = RtlAllocateHeap(RtlGetProcessHeap(), 0, MFT_RECORD_SIZE);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for file record #%d!\n", MftRecordNumber);
        return NULL;
    }

    RtlZeroMemory(FileRecord, MFT_RECORD_SIZE);

    FileRecord->Header.Magic    = FILE_RECORD_MAGIC;
    FileRecord->MFTRecordNumber = MftRecordNumber;

    // Calculate USA offset and count
    FileRecord->Header.UsaOffset = FIELD_OFFSET(FILE_RECORD_HEADER, MFTRecordNumber) + sizeof(ULONG);

    // Size of USA (in ULONG's) will be 1 (for USA number) + 1 for every sector the file record uses
    FileRecord->BytesAllocated = MFT_RECORD_SIZE;
    FileRecord->Header.UsaCount = (FileRecord->BytesAllocated / BYTES_PER_SECTOR) + 1;

    // Setup other file record fields
    FileRecord->SequenceNumber  = 1;
    FileRecord->FirstAttributeOffset = FileRecord->Header.UsaOffset + (2 * FileRecord->Header.UsaCount);
    FileRecord->FirstAttributeOffset = ALIGN_UP_BY(FileRecord->FirstAttributeOffset, ATTR_RECORD_ALIGNMENT);
    FileRecord->Flags      = MFT_RECORD_IN_USE;
    FileRecord->BytesInUse = FileRecord->FirstAttributeOffset + sizeof(ULONG) * 2;

    // Find where the first attribute will be added
    NextAttribute = (PATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->FirstAttributeOffset);

    // Temporary mark the end of the file-record
    NextAttribute->Type   = AttributeEnd;
    NextAttribute->Length = FILE_RECORD_END;

    return FileRecord;
}


/* METAFILES CONSTRUCTORS ****************************************************/

static
PFILE_RECORD_HEADER
CreateMft()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;
    
    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_MFT, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord, 
                                Attribute,
                                MFT_ADDRESS,
                                MFT_DEFAULT_CLUSTERS_SIZE,
                                0);

    // $BITMAP
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddMftBitmapAttribute(FileRecord, Attribute);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateMftMirr()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_MFTMIRR, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                MFT_MIRR_ADDRESS,
                                MFT_MIRR_SIZE,
                                0);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateLogFile()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_LOGFILE, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                LOGFILE_ADDRESS,
                                LOGFILE_SIZE,
                                0);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateVolume()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_VOLUME, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $VOLUME_NAME
    AddEmptyVolumeNameAttribute(FileRecord, Attribute);

    // $VOLUME_INFORMATION
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddVolumeInformationAttribute(FileRecord, 
                                  Attribute, 
                                  NTFS_MAJOR_VERSION, 
                                  NTFS_MINOR_VERSION);

    // $DATA
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddEmptyDataAttribute(FileRecord, Attribute);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateAttrDef()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_ATTRDEF, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }
    
    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                ATTRDEF_ADDRESS,
                                ATTRDEF_SIZE,
                                sizeof(ATTRIBUTES_TABLE));

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateRoot()
{
    // FIXME!

    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD   Attribute = NULL;

    // Create file record
    FileRecord = NtfsCreateBlankFileRecord(METAFILE_ROOT, &Attribute);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for . file record!\n");
        return NULL;
    }

    // $INDEX_ROOT [$I30]
    AddIndexRoot(FileRecord, Attribute);

    // $INDEX_ALLOCATION
    Attribute = NEXT_ATTRIBUTE(Attribute);
    AddIndexAllocation(FileRecord, Attribute);

    // TODO: $BITMAP

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateBitmap()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_BITMAP, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // TODO: $DATA

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateBoot()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_BOOT, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                BOOT_ADDRESS,
                                BOOT_SIZE,
                                0);

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateUpCase()
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD        Attribute = NULL;

    // Create file record
    FileRecord = CreateMetafileRecord(METAFILE_UPCASE, &Attribute);
    if (!FileRecord)
    {
        return NULL;
    }

    // $DATA
    AddNonResidentDataAttribute(FileRecord,
                                Attribute,
                                UPCASE_ADDRESS,
                                UPCASE_SIZE,
                                sizeof(UPCASE_TABLE));

    return FileRecord;
}

static
PFILE_RECORD_HEADER
CreateStub(IN DWORD32 MftRecordNumber)
{
    PFILE_RECORD_HEADER FileRecord;
    PATTR_RECORD   Attribute = NULL;

    // Create file record
    FileRecord = NtfsCreateBlankFileRecord(MftRecordNumber, &Attribute);
    if (!FileRecord)
    {
        DPRINT1("ERROR: Unable to allocate memory for stub #%d file record!\n", MftRecordNumber);
        return NULL;
    }

    // Create empty resident $DATA attribute
    AddEmptyDataAttribute(FileRecord, Attribute);

    return FileRecord;
}


/* METAFILES ADDITIONAL DATA WRITERS *****************************************/

static NTSTATUS WriteMftBitmap()
{
    PBYTE Data = NULL;
    LARGE_INTEGER Offset;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    // Clear bitmap cluster
    Status = WriteZerosToClusters(MFT_BITMAP_ADDRESS,
                                  1,
                                  &IoStatusBlock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to clear sectors for $MFT bitmap! NtWriteFile() failed (Status %lx)\n", Status);
        goto end;
    }

    // Allocate memory for bitmap
    Data = RtlAllocateHeap(RtlGetProcessHeap(), 0, BYTES_PER_SECTOR);
    if (!Data)
    {
        DPRINT1("ERROR: Unable to allocate memory for $MFT bitmap!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    // Clear memory
    RtlZeroMemory(Data, BYTES_PER_SECTOR);

    // Every bit is cluster
    // First 16 clusters used by metafiles
    Data[0] = 0xFF;
    Data[1] = 0xFF;

    // Calculate offset
    Offset.QuadPart = MFT_BITMAP_ADDRESS * BYTES_PER_CLUSTER;

    // Write file
    Status = NtWriteFile(NtfsData.DiskHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Data,
                         BYTES_PER_SECTOR,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write $MFT bitmap! NtWriteFile() failed (Status %lx)\n", Status);
        goto end;
    }

end:
    FREE(Data);
    return Status;
}

static NTSTATUS WriteMftMirr()
{
    BYTE MetafileIndex;
    METAFILE Metafile;
    PFILE_RECORD_HEADER FileRecord;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    // Clear cluster for MFT mirror
    Status = WriteZerosToClusters(MFT_MIRR_ADDRESS,
                                  MFT_MIRR_SIZE,
                                  &IoStatusBlock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to clear sectors for $MFTMirr! NtWriteFile() failed (Status %lx)\n", Status);
        return Status;
    }

    // Write metafiles to mirror
    for (
        MetafileIndex = 0;
        MetafileIndex < MFT_MIRR_COUNT;
        MetafileIndex++
        )
    {
        Metafile = METAFILES[MetafileIndex];

        // Create metafile record or stub, if metafile is not implemented
        if (!Metafile.Constructor)
        {
            FileRecord = CreateStub(MetafileIndex);
        }
        else
        {
            FileRecord = Metafile.Constructor();
        }

        // Check file record
        if (!FileRecord)
        {
            DPRINT1(
                "ERROR: Unable to allocate memory for file record #%d!\n",
                MetafileIndex
            );

            Status = STATUS_INSUFFICIENT_RESOURCES;

            FREE(FileRecord);
            break;
        }

        // Write metafile to disk
        Status = WriteMetafileMirror(FileRecord, &IoStatusBlock);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1(
                "ERROR: Unable to write metafile mirror #%d to disk. NtWriteFile() failed (Status %lx)\n",
                MetafileIndex,
                Status
            );

            FREE(FileRecord);
            break;
        }

        // Free memory
        FREE(FileRecord);
    }

    return Status;
}

static NTSTATUS WriteAttributesTable()
{
    PBYTE Table;
    LARGE_INTEGER Offset;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    Offset.QuadPart = ATTRDEF_ADDRESS * BYTES_PER_CLUSTER;

    // Allocate memory
    Table = RtlAllocateHeap(RtlGetProcessHeap(), 0, BYTES_PER_CLUSTER);
    if (!Table)
    {
        DPRINT1("ERROR: Unable to allocate memory for attributes table!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Clear memory
    RtlZeroMemory(Table, BYTES_PER_CLUSTER);

    // Copy table to memory
    RtlCopyBytes(Table, &ATTRIBUTES_TABLE, sizeof(ATTRIBUTES_TABLE));

    // Write table to disk
    Status = NtWriteFile(NtfsData.DiskHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Table,
                         BYTES_PER_CLUSTER,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write attributes table to disk! NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(Table);

    return Status;
}

static NTSTATUS WriteBitmap()
{
    return STATUS_SUCCESS;
}

static NTSTATUS WriteUpCaseTable()
{
    PUSHORT       Table;
    LARGE_INTEGER Offset;
    ULONG         Size;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    Offset.QuadPart = UPCASE_ADDRESS * BYTES_PER_CLUSTER;
    Size = UPCASE_SIZE * BYTES_PER_CLUSTER;

    // Allocate memory
    Table = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!Table)
    {
        DPRINT1("ERROR: Unable to allocate memory for upcase table!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Clear memory
    RtlZeroMemory(Table, Size);

    // Copy table to memory
    RtlCopyBytes(Table, &UPCASE_TABLE, sizeof(UPCASE_TABLE));

    // Write table to disk
    Status = NtWriteFile(NtfsData.DiskHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Table,
                         Size,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write upcase table to disk! NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(Table);

    return Status;
}
