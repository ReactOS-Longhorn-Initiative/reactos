/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/bootsect.c
 * PURPOSE:     NTFS lib
 * PROGRAMMERS: Pierre Schweitzer, Klachkov Valery
 */

/* INCLUDES ******************************************************************/

#include <ntfslib.h>

#define NDEBUG
#include <debug.h>

extern NtfsFormatData NtfsData;


/* FUNCTIONS *****************************************************************/

static
VOID
FillJumpInstruction(OUT PBOOT_SECTOR BootSector)
{
    BootSector->Jump[0] = 0xEB;  // jmp
    BootSector->Jump[1] = 0x52;  // 82
    BootSector->Jump[2] = 0x90;  // nop
}

static
VOID
FillOemId(OUT PBOOT_SECTOR BootSector)
{
    BootSector->OEMID.QuadPart = OEM_ID;
}

static
VOID
FillBiosParametersBlock(OUT PBIOS_PARAMETERS_BLOCK BiosParametersBlock)
{
    // See: https://en.wikipedia.org/wiki/BIOS_parameter_block
    
    BiosParametersBlock->BytesPerSector    = BYTES_PER_SECTOR;
    BiosParametersBlock->SectorsPerCluster = SECTORS_PER_CLUSTER;

    BiosParametersBlock->MediaId = IS_HARD_DRIVE ? 0xF8 : 0x00;

    BiosParametersBlock->SectorsPerTrack    = SECTORS_PER_TRACK;
    BiosParametersBlock->Heads              = DISK_HEADS;
    BiosParametersBlock->HiddenSectorsCount = BPB_HIDDEN_SECTORS;
}

static
ULONGLONG
CalcVolumeSerialNumber()
{
    BYTE  i;
    ULONG r;
    ULONG seed;
    ULONGLONG serial;

    seed = NtGetTickCount();

    for (i = 0; i < 32; i += 2)
    {
        r = RtlRandom(&seed);

        serial |= ((r & 0xff00) >> 8) << (i * 8);
        serial |= ((r & 0xff)) << (i * 8 * 2);
    }

    return serial;
}

static
VOID
FillExBiosParametersBlock(OUT PEXTENDED_BIOS_PARAMETERS_BLOCK ExBiosParametersBlock)
{
    // See: https://en.wikipedia.org/wiki/BIOS_parameter_block

    ExBiosParametersBlock->Header      = EBPB_HEADER;
    ExBiosParametersBlock->SectorCount = SECTORS_COUNT;

    ExBiosParametersBlock->MftLocation     = MFT_ADDRESS;
    ExBiosParametersBlock->MftMirrLocation = MFT_MIRR_ADDRESS;

    ExBiosParametersBlock->ClustersPerMftRecord   = MFT_CLUSTERS_PER_RECORD;
    ExBiosParametersBlock->ClustersPerIndexRecord = MFT_CLUSTERS_PER_INDEX_RECORD;

    ExBiosParametersBlock->SerialNumber = CalcVolumeSerialNumber();
}

NTSTATUS
WriteBootSector()
{
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PBOOT_SECTOR    BootSector;

    BootSector = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(BOOT_SECTOR));
    if (!BootSector)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(BootSector, sizeof(BOOT_SECTOR));

    FillJumpInstruction(BootSector);
    FillOemId(BootSector);

    FillBiosParametersBlock(&(BootSector->BPB));
    FillExBiosParametersBlock(&(BootSector->EBPB));

    BootSector->EndSector = BOOT_SECTOR_END;

    Status = NtWriteFile(NtfsData.DiskHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootSector,
                         sizeof(BOOT_SECTOR),
                         0ULL,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("BootSector write failed. NtWriteFile() failed (Status %lx)\n", Status);
    }

    FREE(BootSector);

    return Status;
}