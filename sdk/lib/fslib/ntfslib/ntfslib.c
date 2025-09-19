/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/ntfslib.c
 * PURPOSE:     NTFS lib
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

/* INCLUDES ******************************************************************/

#include "ntfslib.h"

#define NDEBUG
#include <debug.h>

NtfsFormatData NtfsData;


/* FUNCTIONS *****************************************************************/

VOID
NtfsGetSystemTimeAsFileTime(OUT PFILETIME lpFileTime)
{
    LARGE_INTEGER SystemTime;

    do
    {
        SystemTime.HighPart = SharedUserData->SystemTime.High1Time;
        SystemTime.LowPart = SharedUserData->SystemTime.LowPart;
    }
    while (SystemTime.HighPart != SharedUserData->SystemTime.High2Time);

    lpFileTime->dwLowDateTime = SystemTime.LowPart;
    lpFileTime->dwHighDateTime = SystemTime.HighPart;
}

BYTE
GetSectorsPerCluster()
{
    GET_LENGTH_INFORMATION* LengthInformation = NtfsData.LengthInformation;

    if (LengthInformation->Length.QuadPart < MB_TO_B(512))
    {
        return 1;
    }
    else if (LengthInformation->Length.QuadPart < MB_TO_B(1024))
    {
        return 2;
    }
    else if (LengthInformation->Length.QuadPart < MB_TO_B(2048))
    {
        return 4;
    }
    else
    {
        return 8;
    }
}

BOOLEAN
NTAPI
NtfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize)
{
    HANDLE                 DiskHandle;
    OBJECT_ATTRIBUTES      Attributes;
    IO_STATUS_BLOCK        Iosb;
    GET_LENGTH_INFORMATION LengthInformation;
    DISK_GEOMETRY          DiskGeometry;
    NTSTATUS               Status;

    DPRINT1("NtfsFormat(DriveRoot '%wZ')\n", DriveRoot);

    InitializeObjectAttributes(&Attributes, DriveRoot, 0, NULL, NULL);

    // Open volume
    Status = NtOpenFile(&DiskHandle,
                        FILE_GENERIC_READ | FILE_GENERIC_WRITE | SYNCHRONIZE,
                        &Attributes,
                        &Iosb,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed with status 0x%.08x\n", Status);
        return FALSE;
    }

    // Get length info
    Status = NtDeviceIoControlFile(DiskHandle, 
                                   NULL,
                                   NULL,
                                   NULL, 
                                   &Iosb, 
                                   IOCTL_DISK_GET_LENGTH_INFO,
                                   NULL, 
                                   0, 
                                   &LengthInformation, 
                                   sizeof(GET_LENGTH_INFORMATION));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IOCTL_DISK_GET_LENGTH_INFO failed with status 0x%.08x\n", Status);
        NtClose(DiskHandle);
        return FALSE;
    }

    // Get disk geometry
    Status = NtDeviceIoControlFile(DiskHandle, 
                                   NULL,
                                   NULL, 
                                   NULL, 
                                   &Iosb,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL, 
                                   0,
                                   &DiskGeometry,
                                   sizeof(DiskGeometry));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IOCTL_DISK_GET_DRIVE_GEOMETRY failed with status 0x%.08x\n", Status);
        NtClose(DiskHandle);
        return FALSE;
    }

    // Initialize progress bar
    if (Callback)
    {
        ULONG pc = 0;
        Callback(PROGRESS, 0, (PVOID)&pc);
    }

    // Setup global data
    NtfsData.DiskHandle = DiskHandle;
    NtfsData.DiskGeometry = &DiskGeometry;
    NtfsData.LengthInformation = &LengthInformation;

    // Lock volume
    NtFsControlFile(DiskHandle, 
                    NULL,
                    NULL,
                    NULL, 
                    &Iosb, 
                    FSCTL_LOCK_VOLUME,
                    NULL, 
                    0, 
                    NULL,
                    0);

    // Write boot sector
    Status = WriteBootSector();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WriteBootSector() failed with status 0x%.08x\n", Status);
        NtClose(DiskHandle);
        goto end;
    }

    // Create metafiles
    Status = WriteMetafiles();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WriteMetafiles() failed with status 0x%.08x\n", Status);
        NtClose(DiskHandle);
        goto end;
    }
    
    DPRINT1("Went through fine\n");

end:

    // Clear global data structure
    NtfsData.DiskHandle = NULL;
    NtfsData.DiskGeometry = NULL;
    NtfsData.LengthInformation = NULL;

    // Dismount and unlock volume
    NtFsControlFile(DiskHandle, NULL, NULL, NULL, &Iosb, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);
    NtFsControlFile(DiskHandle, NULL, NULL, NULL, &Iosb, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0);

    // Clear memory
    NtClose(DiskHandle);

    // Update progress bar
    if (Callback)
    {
        BOOL success = NT_SUCCESS(Status);
        Callback(DONE, 0, (PVOID)&success);
    }

    return TRUE;
}

BOOLEAN
NTAPI
NtfsChkdsk(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PVOID pUnknown1,
    IN PVOID pUnknown2,
    IN PVOID pUnknown3,
    IN PVOID pUnknown4,
    IN PULONG ExitStatus)
{
    // STUB

    if (Callback)
    {
        TEXTOUTPUT TextOut;

        TextOut.Lines = 1;
        TextOut.Output = "stub, not implemented";

        Callback(OUTPUT, 0, &TextOut);
    }
    
    *ExitStatus = (ULONG)STATUS_SUCCESS;
    return TRUE;
}
