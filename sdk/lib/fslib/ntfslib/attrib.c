/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/attrib.c
 * PURPOSE:     NTFS lib
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

/* INCLUDES ******************************************************************/

#include <ntfslib.h>

#define NDEBUG
#include <debug.h>

extern NtfsFormatData NtfsData;

/* MACROSES ******************************************************************/

// Get resident attribute data address
#define RESIDENT_DATA(attr, type) ((type)((LONG_PTR)attr + RA_HEADER_LENGTH))


/* FUNCTIONS *****************************************************************/

static
VOID
SetFileRecordEnd(OUT PFILE_RECORD_HEADER FileRecord,
                 OUT PATTR_RECORD        AttrEnd,
                 IN  ULONG               EndMarker)
{
    // Ensure AttrEnd is aligned on an 8-byte boundary, relative to FileRecord
    ASSERT(((ULONG_PTR)AttrEnd - (ULONG_PTR)FileRecord) % ATTR_RECORD_ALIGNMENT == 0);

    // Mark the end of attributes
    AttrEnd->Type = AttributeEnd;

    // Restore the "file-record-end marker." The value is never checked but this behavior is consistent with Win2k3.
    AttrEnd->Length = EndMarker;

    // Recalculate bytes in use
    FileRecord->BytesInUse = (ULONG_PTR)AttrEnd - (ULONG_PTR)FileRecord + sizeof(ULONG) * 2;
}

VOID
AddStandardInformationAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                                OUT PATTR_RECORD        Attribute)
{
    PSTANDARD_INFORMATION StandardInfo;
    LARGE_INTEGER         SystemTime;

    StandardInfo = RESIDENT_DATA(Attribute, PSTANDARD_INFORMATION);

    Attribute->Type     = AttributeStandardInformation;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    // Default setup for resident attribute
    Attribute->Length = sizeof(STANDARD_INFORMATION) + RA_HEADER_LENGTH;
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = sizeof(STANDARD_INFORMATION);
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    // Set dates and times
    KeQuerySystemTime(&SystemTime);
    StandardInfo->CreationTime   = SystemTime.QuadPart;
    StandardInfo->ChangeTime     = SystemTime.QuadPart;
    StandardInfo->LastWriteTime  = SystemTime.QuadPart;
    StandardInfo->LastAccessTime = SystemTime.QuadPart;
    StandardInfo->FileAttribute  = RA_METAFILES_ATTRIBUTES;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddFileNameAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                     OUT PATTR_RECORD        Attribute,
                     IN  LPCWSTR             FileName,
                     IN  DWORD32             MftRecordNumber)
{
    PFILENAME_ATTRIBUTE FileNameAttribute;
    LARGE_INTEGER       SystemTime;

    DWORD32 FileNameLength = wcslen(FileName);                // Count of chars
    DWORD32 FileNameSize   = FileNameLength * sizeof(WCHAR);  // Count of bytes

    FileNameAttribute = RESIDENT_DATA(Attribute, PFILENAME_ATTRIBUTE);

    Attribute->Type     = AttributeFileName;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    // Set dates and times
    KeQuerySystemTime(&SystemTime);
    FileNameAttribute->CreationTime   = SystemTime.QuadPart;
    FileNameAttribute->ChangeTime     = SystemTime.QuadPart;
    FileNameAttribute->LastWriteTime  = SystemTime.QuadPart;
    FileNameAttribute->LastAccessTime = SystemTime.QuadPart;

    FileNameAttribute->FileAttributes = (FileRecord->Flags & MFT_RECORD_IS_DIRECTORY)
        ? FILE_TYPE_DIRECTORY
        : RA_METAFILES_ATTRIBUTES;
    
    // Set reference to parent directory
    FileNameAttribute->DirectoryFileReferenceNumber = MftRecordNumber;
    FileNameAttribute->DirectoryFileReferenceNumber |= (ULONGLONG)METAFILE_ROOT << 48;

    // Copy file name and save it length
    FileNameAttribute->NameLength = FileNameLength;
    RtlCopyMemory(FileNameAttribute->Name, FileName, FileNameSize);

    // TODO: Check filename for DOS compatibility and set NameType to FILE_NAME_WIN32_AND_DOS
    FileNameAttribute->NameType = FILE_NAME_POSIX;

    FileRecord->HardLinkCount++;

    // Setup for resident attribute
    Attribute->Length = RA_HEADER_LENGTH + FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + FileNameSize;
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + FileNameSize;
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    Attribute->Resident.Flags = RA_INDEXED;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddEmptyDataAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                      OUT PATTR_RECORD        Attribute)
{
    Attribute->Type     = AttributeData;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->Length = RA_HEADER_LENGTH;
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = 0;
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    // For unnamed $DATA attributes, NameOffset equals header length
    Attribute->NameOffset = RA_HEADER_LENGTH;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

static
VOID
AddNonResidentAttribute(OUT PFILE_RECORD_HEADER     FileRecord,
                        OUT PATTR_RECORD            Attribute,
                        IN  ULONG                   AttributeType,
                        IN  ULONG                   Address,
                        IN  ULONG                   ClustersCount,
                        OPTIONAL IN ULONG           DataSize)
{
    ULONG LCN = BSWAP32(Address);
    BYTE  LCNOffset;
    BYTE  LCNCutSize;

    ULONG Clusters = BSWAP32(ClustersCount);
    BYTE  ClustersOffset = RUN_LIST_ENTRY_HEADER_SIZE;
    BYTE  ClustersCutSize;

    PBYTE RunListEntry;
    BYTE  RunListEntryOffset;

    // Setup attribute
    Attribute->Type     = AttributeType;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->IsNonResident = 1;
    Attribute->Flags         = 0;

    Attribute->NameLength = 0;
    Attribute->NameOffset = sizeof(ATTR_RECORD);

    Attribute->NonResident.LowestVCN       = 0;
    Attribute->NonResident.HighestVCN      = (ClustersCount - 1);
    Attribute->NonResident.DataRunsOffset  = sizeof(ATTR_RECORD);
    Attribute->NonResident.CompressionUnit = 0;
    
    Attribute->NonResident.AllocatedSize   = ClustersCount * BYTES_PER_CLUSTER;
    Attribute->NonResident.DataSize        = !DataSize ? Attribute->NonResident.AllocatedSize : DataSize;
    Attribute->NonResident.CompressedSize  = !DataSize ? Attribute->NonResident.AllocatedSize : DataSize;

    Attribute->Length = sizeof(ATTR_RECORD) + RUN_LIST_ENTRY_SIZE;

    // Setup run list entry
    RunListEntry = (PBYTE)((ULONG_PTR)Attribute + sizeof(ATTR_RECORD));
    RtlZeroMemory(RunListEntry, RUN_LIST_ENTRY_SIZE);

    // Calculating the minimum segment length for data placement

    if (Address < 0x0100)
    {
        LCNCutSize = 1;
    }
    else if (Address < 0x010000)
    {
        LCNCutSize = 2;
    }
    else if (Address < 0x01000000)
    {
        LCNCutSize = 3;
    }
    else
    {
        LCNCutSize = 4;
    }

    if (ClustersCount < 0x0100)
    {
        ClustersCutSize = 1;
    }
    else if (ClustersCount < 0x010000)
    {
        ClustersCutSize = 2;
    }
    else if (ClustersCount < 0x01000000)
    {
        ClustersCutSize = 3;
    }
    else
    {
        ClustersCutSize = 4;
    }

    // Check whether the data is fit in the record
    ASSERT((LCNCutSize + ClustersCutSize) < RUN_LIST_ENTRY_SIZE);

    // Calculate offsets
    LCNOffset = ClustersOffset + ClustersCutSize;

    // Setup header
    RunListEntry[0] = (LCNCutSize) << 4 | ClustersCutSize;

    // Copy clusters count
    for (
        RunListEntryOffset = ClustersOffset;
        RunListEntryOffset < ClustersOffset + ClustersCutSize;
        RunListEntryOffset++
    )
    {
        RunListEntry[RunListEntryOffset] = GET_BYTE_FROM_END(Clusters, RunListEntryOffset - ClustersOffset);
    }

    // Copy LCN
    for (
        RunListEntryOffset = LCNOffset;
        RunListEntryOffset < LCNOffset + LCNCutSize;
        RunListEntryOffset++
    )
    {
        RunListEntry[RunListEntryOffset] = GET_BYTE_FROM_END(LCN, RunListEntryOffset - LCNOffset);
    }

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddNonResidentDataAttribute(OUT PFILE_RECORD_HEADER     FileRecord,
                            OUT PATTR_RECORD            Attribute,
                            IN  ULONG                   Address,
                            IN  ULONG                   ClustersCount,
                            OPTIONAL IN ULONG           DataSize)
{
    AddNonResidentAttribute(FileRecord,
                            Attribute,
                            AttributeData,
                            Address,
                            ClustersCount,
                            DataSize);
}

VOID
AddMftBitmapAttribute(OUT PFILE_RECORD_HEADER     FileRecord,
                      OUT PATTR_RECORD            Attribute)
{
    AddNonResidentAttribute(FileRecord,
                            Attribute,
                            AttributeBitmap,
                            MFT_BITMAP_ADDRESS,
                            1,
                            MFT_DEFAULT_RECORDS_COUNT);
}

VOID
AddEmptyVolumeNameAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                            OUT PATTR_RECORD        Attribute)
{
    Attribute->Type     = AttributeVolumeName;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    Attribute->Length = RA_HEADER_LENGTH;
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = 0;
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;

    Attribute->NameOffset = RA_HEADER_LENGTH;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddVolumeInformationAttribute(OUT PFILE_RECORD_HEADER FileRecord,
                              OUT PATTR_RECORD        Attribute,
                              IN  BYTE                MajorVersion,
                              IN  BYTE                MinorVersion)
{
    PVOLUME_INFORMATION_ATTRIBUTE VolumeInfo;

    Attribute->Type     = AttributeVolumeInformation;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    VolumeInfo = RESIDENT_DATA(Attribute, PVOLUME_INFORMATION_ATTRIBUTE);

    VolumeInfo->MajorVersion = MajorVersion;
    VolumeInfo->MinorVersion = MinorVersion;
    VolumeInfo->Flags = 0;

    FileRecord->HardLinkCount++;

    Attribute->Length = RA_HEADER_LENGTH + sizeof(VOLUME_INFORMATION_ATTRIBUTE);
    Attribute->Length = ALIGN_UP_BY(Attribute->Length, ATTR_RECORD_ALIGNMENT);

    Attribute->Resident.ValueLength = sizeof(VOLUME_INFORMATION_ATTRIBUTE);
    Attribute->Resident.ValueOffset = RA_HEADER_LENGTH;
    Attribute->Resident.Flags = RA_INDEXED;

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddIndexRoot(OUT PFILE_RECORD_HEADER FileRecord,
             OUT PATTR_RECORD        Attribute)
{
    Attribute->Type     = AttributeIndexRoot;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    // TODO: Implement this

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}

VOID
AddIndexAllocation(OUT PFILE_RECORD_HEADER FileRecord,
                   OUT PATTR_RECORD        Attribute)
{
    Attribute->Type     = AttributeIndexAllocation;
    Attribute->Instance = FileRecord->NextAttributeNumber++;

    // TODO: Implement this

    // Move the attribute-end and file-record-end markers to the end of the file record
    Attribute = NEXT_ATTRIBUTE(Attribute);
    SetFileRecordEnd(FileRecord, Attribute, Attribute->Length);
}