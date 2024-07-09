/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/io/iomgr/iomgr.c
 * PURPOSE:         I/O Manager Initialization and Misc Utility Functions
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

//#define NDEBUG
#include <debug.h>

ULONG IopTraceLevel = 0;
extern KEVENT PiEnumerationLock;

VOID
NTAPI
IopTimerDispatch(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

BOOLEAN
NTAPI
WmiInitialize(
    VOID);

/* DATA ********************************************************************/

POBJECT_TYPE IoDeviceObjectType = NULL;
POBJECT_TYPE IoFileObjectType = NULL;
extern POBJECT_TYPE IoControllerObjectType;
BOOLEAN IoCountOperations = TRUE;
ULONG IoReadOperationCount = 0;
LARGE_INTEGER IoReadTransferCount = {{0, 0}};
ULONG IoWriteOperationCount = 0;
LARGE_INTEGER IoWriteTransferCount = {{0, 0}};
ULONG IoOtherOperationCount = 0;
LARGE_INTEGER IoOtherTransferCount = {{0, 0}};
KSPIN_LOCK IoStatisticsLock = 0;
ULONG IopNumTriageDumpDataBlocks;
PVOID IopTriageDumpDataBlocks[64];
BOOLEAN IsWithoutGroupOrderIndex = FALSE;
extern BOOLEAN PnPBootDriversInitialized;

GENERIC_MAPPING IopFileMapping = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS};

extern LIST_ENTRY ShutdownListHead;
extern LIST_ENTRY LastChanceShutdownListHead;
extern KSPIN_LOCK ShutdownListLock;
extern POBJECT_TYPE IoAdapterObjectType;
extern ERESOURCE IopDatabaseResource;
ERESOURCE IopSecurityResource;
extern ERESOURCE IopDriverLoadResource;
extern KGUARDED_MUTEX PnpNotifyListLock;
extern LIST_ENTRY IopDiskFileSystemQueueHead;
extern LIST_ENTRY IopCdRomFileSystemQueueHead;
extern LIST_ENTRY IopTapeFileSystemQueueHead;
extern LIST_ENTRY IopNetworkFileSystemQueueHead;
extern LIST_ENTRY DriverBootReinitListHead;
extern LIST_ENTRY DriverReinitListHead;
extern LIST_ENTRY PnpNotifyListHead;
extern LIST_ENTRY IopFsNotifyChangeQueueHead;
extern LIST_ENTRY IopErrorLogListHead;
extern LIST_ENTRY IopTimerQueueHead;
extern KDPC IopTimerDpc;
extern KTIMER IopTimer;
extern KSPIN_LOCK IoStatisticsLock;
extern KSPIN_LOCK DriverReinitListLock;
extern KSPIN_LOCK DriverBootReinitListLock;
extern KSPIN_LOCK IopLogListLock;
extern KSPIN_LOCK IopTimerLock;

GENERAL_LOOKASIDE IoLargeIrpLookaside;
GENERAL_LOOKASIDE IoSmallIrpLookaside;
GENERAL_LOOKASIDE IopMdlLookasideList;
extern GENERAL_LOOKASIDE IoCompletionPacketLookaside;

PLOADER_PARAMETER_BLOCK IopLoaderBlock;

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, IoInitSystem)
#endif

/* INIT FUNCTIONS ************************************************************/

VOID
CODE_SEG("INIT")
NTAPI
IopInitLookasideLists(VOID)
{
    ULONG LargeIrpSize, SmallIrpSize, MdlSize;
    LONG i;
    PKPRCB Prcb;
    PGENERAL_LOOKASIDE CurrentList = NULL;

    /* Calculate the sizes */
    LargeIrpSize = sizeof(IRP) + (8 * sizeof(IO_STACK_LOCATION));
    SmallIrpSize = sizeof(IRP) + sizeof(IO_STACK_LOCATION);
    MdlSize = sizeof(MDL) + (23 * sizeof(PFN_NUMBER));

    /* Initialize the Lookaside List for I\O Completion */
    ExInitializeSystemLookasideList(&IoCompletionPacketLookaside,
                                    NonPagedPool,
                                    sizeof(IOP_MINI_COMPLETION_PACKET),
                                    IOC_TAG1,
                                    32,
                                    &ExSystemLookasideListHead);

    /* Initialize the Lookaside List for Large IRPs */
    ExInitializeSystemLookasideList(&IoLargeIrpLookaside,
                                    NonPagedPool,
                                    LargeIrpSize,
                                    IO_LARGEIRP,
                                    64,
                                    &ExSystemLookasideListHead);


    /* Initialize the Lookaside List for Small IRPs */
    ExInitializeSystemLookasideList(&IoSmallIrpLookaside,
                                    NonPagedPool,
                                    SmallIrpSize,
                                    IO_SMALLIRP,
                                    32,
                                    &ExSystemLookasideListHead);

    /* Initialize the Lookaside List for MDLs */
    ExInitializeSystemLookasideList(&IopMdlLookasideList,
                                    NonPagedPool,
                                    MdlSize,
                                    TAG_MDL,
                                    128,
                                    &ExSystemLookasideListHead);

    /* Allocate the global lookaside list buffer */
    CurrentList = ExAllocatePoolWithTag(NonPagedPool,
                                        4 * KeNumberProcessors *
                                        sizeof(GENERAL_LOOKASIDE),
                                        TAG_IO);

    /* Loop all processors */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        /* Get the PRCB for this CPU */
        Prcb = KiProcessorBlock[i];
        DPRINT("Setting up lookaside for CPU: %x, PRCB: %p\n", i, Prcb);

        /* Write IRP credit limit */
        Prcb->LookasideIrpFloat = 512 / KeNumberProcessors;

        /* Set the I/O Completion List */
        Prcb->PPLookasideList[LookasideCompletionList].L = &IoCompletionPacketLookaside;
        if (CurrentList)
        {
            /* Initialize the Lookaside List for mini-packets */
            ExInitializeSystemLookasideList(CurrentList,
                                            NonPagedPool,
                                            sizeof(IOP_MINI_COMPLETION_PACKET),
                                            IO_SMALLIRP_CPU,
                                            32,
                                            &ExSystemLookasideListHead);
            Prcb->PPLookasideList[LookasideCompletionList].P = CurrentList;
            CurrentList++;

        }
        else
        {
            Prcb->PPLookasideList[LookasideCompletionList].P = &IoCompletionPacketLookaside;
        }

        /* Set the Large IRP List */
        Prcb->PPLookasideList[LookasideLargeIrpList].L = &IoLargeIrpLookaside;
        if (CurrentList)
        {
            /* Initialize the Lookaside List for Large IRPs */
            ExInitializeSystemLookasideList(CurrentList,
                                            NonPagedPool,
                                            LargeIrpSize,
                                            IO_LARGEIRP_CPU,
                                            64,
                                            &ExSystemLookasideListHead);
            Prcb->PPLookasideList[LookasideLargeIrpList].P = CurrentList;
            CurrentList++;

        }
        else
        {
            Prcb->PPLookasideList[LookasideLargeIrpList].P = &IoLargeIrpLookaside;
        }

        /* Set the Small IRP List */
        Prcb->PPLookasideList[LookasideSmallIrpList].L = &IoSmallIrpLookaside;
        if (CurrentList)
        {
            /* Initialize the Lookaside List for Small IRPs */
            ExInitializeSystemLookasideList(CurrentList,
                                            NonPagedPool,
                                            SmallIrpSize,
                                            IO_SMALLIRP_CPU,
                                            32,
                                            &ExSystemLookasideListHead);
            Prcb->PPLookasideList[LookasideSmallIrpList].P = CurrentList;
            CurrentList++;

        }
        else
        {
            Prcb->PPLookasideList[LookasideSmallIrpList].P = &IoSmallIrpLookaside;
        }

        /* Set the MDL Completion List */
        Prcb->PPLookasideList[LookasideMdlList].L = &IopMdlLookasideList;
        if (CurrentList)
        {
            /* Initialize the Lookaside List for MDLs */
            ExInitializeSystemLookasideList(CurrentList,
                                            NonPagedPool,
                                            SmallIrpSize,
                                            TAG_MDL,
                                            128,
                                            &ExSystemLookasideListHead);

            Prcb->PPLookasideList[LookasideMdlList].P = CurrentList;
            CurrentList++;

        }
        else
        {
            Prcb->PPLookasideList[LookasideMdlList].P = &IopMdlLookasideList;
        }
    }
}

BOOLEAN
CODE_SEG("INIT")
NTAPI
IopCreateObjectTypes(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    /* Initialize default settings */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.GenericMapping = IopFileMapping;

    /* Do the Adapter Type */
    RtlInitUnicodeString(&Name, L"Adapter");
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoAdapterObjectType))) return FALSE;

    /* Do the Controller Type */
    RtlInitUnicodeString(&Name, L"Controller");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(CONTROLLER_OBJECT);
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoControllerObjectType))) return FALSE;

    /* Do the Device Type */
    RtlInitUnicodeString(&Name, L"Device");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DEVICE_OBJECT);
    ObjectTypeInitializer.DeleteProcedure = IopDeleteDevice;
    ObjectTypeInitializer.ParseProcedure = IopParseDevice;
    ObjectTypeInitializer.SecurityProcedure = IopGetSetSecurityObject;
    ObjectTypeInitializer.CaseInsensitive = TRUE;
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoDeviceObjectType))) return FALSE;

    /* Initialize the Driver object type */
    RtlInitUnicodeString(&Name, L"Driver");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DRIVER_OBJECT);
    ObjectTypeInitializer.DeleteProcedure = IopDeleteDriver;
    ObjectTypeInitializer.ParseProcedure = NULL;
    ObjectTypeInitializer.SecurityProcedure = NULL;
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoDriverObjectType))) return FALSE;

    /* Initialize the I/O Completion object type */
    RtlInitUnicodeString(&Name, L"IoCompletion");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KQUEUE);
    ObjectTypeInitializer.ValidAccessMask = IO_COMPLETION_ALL_ACCESS;
    ObjectTypeInitializer.InvalidAttributes |= OBJ_PERMANENT;
    ObjectTypeInitializer.GenericMapping = IopCompletionMapping;
    ObjectTypeInitializer.DeleteProcedure = IopDeleteIoCompletion;
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoCompletionType))) return FALSE;

    /* Initialize the File object type  */
    RtlInitUnicodeString(&Name, L"File");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(FILE_OBJECT);
    ObjectTypeInitializer.InvalidAttributes |= OBJ_EXCLUSIVE;
    ObjectTypeInitializer.MaintainHandleCount = TRUE;
    ObjectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    ObjectTypeInitializer.GenericMapping = IopFileMapping;
    ObjectTypeInitializer.CloseProcedure = IopCloseFile;
    ObjectTypeInitializer.DeleteProcedure = IopDeleteFile;
    ObjectTypeInitializer.SecurityProcedure = IopGetSetSecurityObject;
    ObjectTypeInitializer.QueryNameProcedure = IopQueryName;
    ObjectTypeInitializer.ParseProcedure = IopParseFile;
    ObjectTypeInitializer.UseDefaultObject = FALSE;
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoFileObjectType))) return FALSE;

    /* Success */
    return TRUE;
}

BOOLEAN
CODE_SEG("INIT")
NTAPI
IopCreateRootDirectories(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirName;
    HANDLE Handle;
    NTSTATUS Status;

    /* Create the '\Driver' object directory */
    RtlInitUnicodeString(&DirName, L"\\Driver");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = NtCreateDirectoryObject(&Handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create \\Driver directory: 0x%lx\n", Status);
        return FALSE;
    }
    NtClose(Handle);

    /* Create the '\FileSystem' object directory */
    RtlInitUnicodeString(&DirName, L"\\FileSystem");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = NtCreateDirectoryObject(&Handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create \\FileSystem directory: 0x%lx\n", Status);
        return FALSE;
    }
    NtClose(Handle);

    /* Create the '\FileSystem' object directory */
    RtlInitUnicodeString(&DirName, L"\\FileSystem\\Filters");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = NtCreateDirectoryObject(&Handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create \\FileSystem\\Filters directory: 0x%lx\n", Status);
        return FALSE;
    }
    NtClose(Handle);

    /* Return success */
    return TRUE;
}

BOOLEAN
CODE_SEG("INIT")
NTAPI
IoInitSystem(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    LARGE_INTEGER ExpireTime;
    NTSTATUS Status;
    CHAR Buffer[256];
    ANSI_STRING NtBootPath, RootString;

    /* Initialize empty NT Boot Path */
    RtlInitEmptyAnsiString(&NtBootPath, Buffer, sizeof(Buffer));

    /* Initialize the lookaside lists */
    IopInitLookasideLists();

    /* Initialize all locks and lists */
    ExInitializeResourceLite(&IopDatabaseResource);
    ExInitializeResourceLite(&IopSecurityResource);
    ExInitializeResourceLite(&IopDriverLoadResource);
    KeInitializeGuardedMutex(&PnpNotifyListLock);
    InitializeListHead(&IopDiskFileSystemQueueHead);
    InitializeListHead(&IopCdRomFileSystemQueueHead);
    InitializeListHead(&IopTapeFileSystemQueueHead);
    InitializeListHead(&IopNetworkFileSystemQueueHead);
    InitializeListHead(&DriverBootReinitListHead);
    InitializeListHead(&DriverReinitListHead);
    InitializeListHead(&PnpNotifyListHead);
    InitializeListHead(&ShutdownListHead);
    InitializeListHead(&LastChanceShutdownListHead);
    InitializeListHead(&IopFsNotifyChangeQueueHead);
    InitializeListHead(&IopErrorLogListHead);
    KeInitializeSpinLock(&IoStatisticsLock);
    KeInitializeSpinLock(&DriverReinitListLock);
    KeInitializeSpinLock(&DriverBootReinitListLock);
    KeInitializeSpinLock(&ShutdownListLock);
    KeInitializeSpinLock(&IopLogListLock);

    /* Initialize the reserve IRP */
    if (!IopInitializeReserveIrp(&IopReserveIrpAllocator))
    {
        DPRINT1("IopInitializeReserveIrp failed!\n");
        return FALSE;
    }

    /* Initialize Timer List Lock */
    KeInitializeSpinLock(&IopTimerLock);

    /* Initialize Timer List */
    InitializeListHead(&IopTimerQueueHead);

    /* Initialize the DPC/Timer which will call the other Timer Routines */
    ExpireTime.QuadPart = -10000000;
    KeInitializeDpc(&IopTimerDpc, IopTimerDispatch, NULL);
    KeInitializeTimerEx(&IopTimer, SynchronizationTimer);
    KeSetTimerEx(&IopTimer, ExpireTime, 1000, &IopTimerDpc);

    /* Create Object Types */
    if (!IopCreateObjectTypes())
    {
        DPRINT1("IopCreateObjectTypes failed!\n");
        return FALSE;
    }

    /* Create Object Directories */
    if (!IopCreateRootDirectories())
    {
        DPRINT1("IopCreateRootDirectories failed!\n");
        return FALSE;
    }

    /* Initialize PnP manager */
    DPRINT("IoInitSystem: IopInitializePlugPlayServices(0)\n");
    IopInitializePlugPlayServices(LoaderBlock, 0);

    /* Initialize SHIM engine */
    ApphelpCacheInitialize();

    /* Initialize WMI */
    WmiInitialize();

    /* Initialize HAL Root Bus Driver */
    DPRINT("IoInitSystem: HalInitPnpDriver()\n");
    HalInitPnpDriver();
    DPRINT("IoInitSystem: IopMarkHalDeviceNode()\n");
    IopMarkHalDeviceNode();

    /* Make loader block available for the whole kernel */
    IopLoaderBlock = LoaderBlock;

    /* Initialize PnP manager */
    DPRINT("IoInitSystem: IopInitializePlugPlayServices(1)\n");
    IopInitializePlugPlayServices(LoaderBlock, 1);

    /* Load boot start drivers */
    DPRINT("IoInitSystem: IopInitializeBootDrivers()\n");
    IopInitializeBootDrivers(LoaderBlock);

    /* No one should need loader block any longer */
    IopLoaderBlock = NULL;

#ifndef _WINKD_
    /* Read KDB Data */
    KdbInit();

    /* I/O is now setup for disk access, so phase 3 */
    KdInitSystem(3, LoaderBlock);
#endif

    /* Load system start drivers */
    DPRINT("IoInitSystem: IopInitializeSystemDrivers()\n");
    IopInitializeSystemDrivers();

    /* Reinitialize drivers that requested it */
    DPRINT("IoInitSystem: Reinitialize drivers\n");
    IopReinitializeDrivers();

    /* Convert SystemRoot from ARC to NT path */
    DPRINT("IoInitSystem: Convert SystemRoot from ARC to NT path\n");
    Status = IopReassignSystemRoot(LoaderBlock, &NtBootPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopReassignSystemRoot failed: %lx\n", Status);
        return FALSE;
    }

    /* Set the ANSI_STRING for the root path */
    RootString.MaximumLength = NtSystemRoot.MaximumLength / sizeof(WCHAR);
    RootString.Length = 0;
    RootString.Buffer = ExAllocatePoolWithTag(PagedPool,
                                              RootString.MaximumLength,
                                              TAG_IO);

    /* Convert the path into the ANSI_STRING */
    Status = RtlUnicodeStringToAnsiString(&RootString, &NtSystemRoot, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlUnicodeStringToAnsiString failed: %lx\n", Status);
        return FALSE;
    }

    /* Assign drive letters */
    DPRINT("IoInitSystem: Assign drive letters\n");
    IoAssignDriveLetters(LoaderBlock,
                         &NtBootPath,
                         (PUCHAR)RootString.Buffer,
                         &RootString);

    /* Update system root */
    Status = RtlAnsiStringToUnicodeString(&NtSystemRoot, &RootString, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAnsiStringToUnicodeString failed: %lx\n", Status);
        return FALSE;
    }

    /* Load the System DLL and its Entrypoints */
    DPRINT("IoInitSystem: Load the System DLL and its Entrypoints\n");
    Status = PsLocateSystemDll();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PsLocateSystemDll failed: %lx\n", Status);
        return FALSE;
    }

    /* Return success */
    DPRINT("IoInitSystem: return TRUE\n");
    return TRUE;
}

BOOLEAN
NTAPI
IoInitializeCrashDump(IN HANDLE PageFileHandle)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
