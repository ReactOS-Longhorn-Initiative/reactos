/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/driver.c
 * PURPOSE:         Driver Object Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 *                  Herv� Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

//#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ERESOURCE IopDriverLoadResource;

LIST_ENTRY DriverReinitListHead;
KSPIN_LOCK DriverReinitListLock;
PLIST_ENTRY DriverReinitTailEntry;

PLIST_ENTRY DriverBootReinitTailEntry;
LIST_ENTRY DriverBootReinitListHead;
KSPIN_LOCK DriverBootReinitListLock;

POBJECT_TYPE IoDriverObjectType = NULL;

extern BOOLEAN ExpInTextModeSetup;
extern BOOLEAN PnpSystemInit;
extern ULONG InitSafeBootMode;

USHORT IopGroupIndex;
PLIST_ENTRY IopGroupTable;

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
IopIsLegacyDriver(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    if (DriverObject->DriverExtension->AddDevice)
        return FALSE;

    if ((DriverObject->Flags & DRVO_LEGACY_DRIVER) != DRVO_LEGACY_DRIVER)
        return FALSE;

    return TRUE;
}

NTSTATUS
NTAPI
IopInvalidDeviceRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
NTAPI
PpDriverObjectDereferenceComplete(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();
    DPRINT("PpDriverObjectDereferenceComplete: Driver '%wZ'\n", &DriverObject->DriverName);

    return PipRequestDeviceAction(IopRootDeviceNode->PhysicalDeviceObject,
                                  PipEnumClearProblem,
                                  0,
                                  CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD, // RequestArgument 
                                  NULL,
                                  NULL);
}

VOID
NTAPI
IopDeleteDriver(
    _In_ PVOID ObjectBody)
{
    PDRIVER_OBJECT DriverObject = ObjectBody;
    PIO_CLIENT_EXTENSION DriverExtension, NextDriverExtension;

    PAGED_CODE();
    DPRINT1("Deleting driver object '%wZ'\n", &DriverObject->DriverName);

    /* There must be no device objects remaining at this point */
    ASSERT(!DriverObject->DeviceObject);

    /* Get the extension and loop them */
    DriverExtension = IoGetDrvObjExtension(DriverObject)->ClientDriverExtension;
    while (DriverExtension)
    {
        /* Get the next one */
        NextDriverExtension = DriverExtension->NextExtension;
        ExFreePoolWithTag(DriverExtension, TAG_DRIVER_EXTENSION);

        /* Move on */
        DriverExtension = NextDriverExtension;
    }

    /* Check if the driver image is still loaded */
    if (DriverObject->DriverSection)
    {
        /* Unload it */
        KeFlushQueuedDpcs();
        MmUnloadSystemImage(DriverObject->DriverSection);
        PpDriverObjectDereferenceComplete(DriverObject);
    }

    /* Check if it has a name */
    if (DriverObject->DriverName.Buffer)
    {
        /* Free it */
        ExFreePool(DriverObject->DriverName.Buffer);
    }

    /* Check if it has a service key name */
    if (DriverObject->DriverExtension->ServiceKeyName.Buffer)
    {
        /* Free it */
        ExFreePool(DriverObject->DriverExtension->ServiceKeyName.Buffer);
    }
}

/*
 * RETURNS
 *  TRUE if String2 contains String1 as a suffix.
 */
BOOLEAN
NTAPI
IopSuffixUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2)
{
    PWCHAR pc1;
    PWCHAR pc2;
    ULONG Length;

    if (String2->Length < String1->Length)
        return FALSE;

    Length = String1->Length / 2;
    pc1 = String1->Buffer;
    pc2 = &String2->Buffer[String2->Length / sizeof(WCHAR) - Length];

    if (pc1 && pc2)
    {
        while (Length--)
        {
            if( *pc1++ != *pc2++ )
                return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * IopDisplayLoadingMessage
 *
 * Display 'Loading XXX...' message.
 */
VOID
FASTCALL
IopDisplayLoadingMessage(PUNICODE_STRING ServiceName)
{
    CHAR TextBuffer[256];
    UNICODE_STRING DotSys = RTL_CONSTANT_STRING(L".SYS");

    if (ExpInTextModeSetup) return;
    if (!KeLoaderBlock) return;
    RtlUpcaseUnicodeString(ServiceName, ServiceName, FALSE);
    snprintf(TextBuffer, sizeof(TextBuffer),
            "%s%sSystem32\\Drivers\\%wZ%s\r\n",
            KeLoaderBlock->ArcBootDeviceName,
            KeLoaderBlock->NtBootPathName,
            ServiceName,
            IopSuffixUnicodeString(&DotSys, ServiceName) ? "" : ".SYS");
    HalDisplayString(TextBuffer);
}

/*
 * IopNormalizeImagePath
 *
 * Normalize an image path to contain complete path.
 *
 * Parameters
 *    ImagePath
 *       The input path and on exit the result path. ImagePath.Buffer
 *       must be allocated by ExAllocatePool on input. Caller is responsible
 *       for freeing the buffer when it's no longer needed.
 *
 *    ServiceName
 *       Name of the service that ImagePath belongs to.
 *
 * Return Value
 *    Status
 *
 * Remarks
 *    The input image path isn't freed on error.
 */
NTSTATUS
FASTCALL
IopNormalizeImagePath(
    _Inout_ _When_(return>=0, _At_(ImagePath->Buffer, _Post_notnull_ __drv_allocatesMem(Mem)))
         PUNICODE_STRING ImagePath,
    _In_ PUNICODE_STRING ServiceName)
{
    UNICODE_STRING SystemRootString = RTL_CONSTANT_STRING(L"\\SystemRoot\\");
    UNICODE_STRING DriversPathString = RTL_CONSTANT_STRING(L"\\SystemRoot\\System32\\drivers\\");
    UNICODE_STRING DotSysString = RTL_CONSTANT_STRING(L".sys");
    UNICODE_STRING InputImagePath;

    DPRINT("Normalizing image path '%wZ' for service '%wZ'\n", ImagePath, ServiceName);

    InputImagePath = *ImagePath;
    if (InputImagePath.Length == 0)
    {
        ImagePath->Length = 0;
        ImagePath->MaximumLength = DriversPathString.Length +
                                   ServiceName->Length +
                                   DotSysString.Length +
                                   sizeof(UNICODE_NULL);
        ImagePath->Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                  ImagePath->MaximumLength,
                                                  TAG_IO);
        if (ImagePath->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyUnicodeString(ImagePath, &DriversPathString);
        RtlAppendUnicodeStringToString(ImagePath, ServiceName);
        RtlAppendUnicodeStringToString(ImagePath, &DotSysString);
    }
    else if (InputImagePath.Buffer[0] != L'\\')
    {
        ImagePath->Length = 0;
        ImagePath->MaximumLength = SystemRootString.Length +
                                   InputImagePath.Length +
                                   sizeof(UNICODE_NULL);
        ImagePath->Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                  ImagePath->MaximumLength,
                                                  TAG_IO);
        if (ImagePath->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyUnicodeString(ImagePath, &SystemRootString);
        RtlAppendUnicodeStringToString(ImagePath, &InputImagePath);

        /* Free caller's string */
        ExFreePoolWithTag(InputImagePath.Buffer, TAG_RTLREGISTRY);
    }

    DPRINT("Normalized image path is '%wZ' for service '%wZ'\n", ImagePath, ServiceName);

    return STATUS_SUCCESS;
}

/* Load a module specified by registry settings for service.

   ServiceName
      Name of the service to load.
 */
NTSTATUS
FASTCALL
IopLoadServiceModule(
    _In_ PUNICODE_STRING ServiceName,
    _Out_ PLDR_DATA_TABLE_ENTRY * ModuleObject)
{
    UNICODE_STRING ServicesName = RTL_CONSTANT_STRING(IO_REG_KEY_SERVICES);
    UNICODE_STRING ServiceImagePath;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    HANDLE RootHandle;
    HANDLE ServiceHandle;
    PVOID BaseAddress;
    ULONG ServiceStart;
    NTSTATUS Status;

    DPRINT("IopLoadServiceModule: '%wZ', %p\n", ServiceName, ModuleObject);

    ASSERT(ExIsResourceAcquiredExclusiveLite(&IopDriverLoadResource));
    ASSERT(ServiceName->Length);

    if (ExpInTextModeSetup)
    {
        /* We have no registry, but luckily we know where all the drivers are */
        DPRINT1("IopLoadServiceModule: '%wZ', %p. ExpInTextModeSetup mode.\n", ServiceName, ModuleObject);

        /* ServiceStart < 4 is all that matters */
        ServiceStart = 0;

        /* IopNormalizeImagePath will do all of the work for us if we give it an empty string */
        RtlInitEmptyUnicodeString(&ServiceImagePath, NULL, 0);
    }
    else
    {
        /* Open CurrentControlSet */
        Status = IopOpenRegistryKeyEx(&RootHandle, NULL, &ServicesName, KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopLoadServiceModule: Failed '%wZ' with %X\n", &ServicesName, Status);
            return Status;
        }

        /* Open service key */
        Status = IopOpenRegistryKeyEx(&ServiceHandle, RootHandle, ServiceName, KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopLoadServiceModule: Failed '%wZ' with %X\n", ServiceName, Status);
            ZwClose(RootHandle);
            return Status;
        }

        /* Get information about the service. */
        RtlZeroMemory(QueryTable, sizeof(QueryTable));

        RtlInitUnicodeString(&ServiceImagePath, NULL);

        QueryTable[0].Name = L"Start";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[0].EntryContext = &ServiceStart;

        QueryTable[1].Name = L"ImagePath";
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].EntryContext = &ServiceImagePath;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR)ServiceHandle,
                                        QueryTable,
                                        NULL,
                                        NULL);
        ZwClose(ServiceHandle);
        ZwClose(RootHandle);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopLoadServiceModule: RtlQueryRegistryValues() failed %X\n", Status);
            return Status;
        }
    }

    /* Normalize the image path for all later processing. */
    Status = IopNormalizeImagePath(&ServiceImagePath, ServiceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadServiceModule: IopNormalizeImagePath() failed %X\n", Status);
        return Status;
    }

    /* Case for disabled drivers */
    if (ServiceStart >= 4)
    {
        /* We can't load this */
        Status = STATUS_DRIVER_UNABLE_TO_LOAD;
    }
    else
    {
        DPRINT("IopLoadServiceModule: Loading '%wZ'\n", &ServiceImagePath);

        Status = MmLoadSystemImage(&ServiceImagePath, NULL, NULL, 0, (PVOID)ModuleObject, &BaseAddress);
        if (NT_SUCCESS(Status))
            IopDisplayLoadingMessage(ServiceName);
    }

    ExFreePool(ServiceImagePath.Buffer);

    /* Now check if the module was loaded successfully. */
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopLoadServiceModule: Loading failed %X\n", Status);
    }
    else
    {
        DPRINT("IopLoadServiceModule: Loading Ok %X\n", Status);
    }

    return Status;
}

VOID
NTAPI
MmFreeDriverInitialization(IN PLDR_DATA_TABLE_ENTRY LdrEntry);

NTSTATUS
NTAPI
MiResolveImageReferences(IN PVOID ImageBase,
                         IN PUNICODE_STRING ImageFileDirectory,
                         IN PUNICODE_STRING NamePrefix OPTIONAL,
                         OUT PCHAR *MissingApi,
                         OUT PWCHAR *MissingDriver,
                         OUT PLOAD_IMPORTS *LoadImports);

//
// Used for images already loaded (boot drivers)
//
CODE_SEG("INIT")
NTSTATUS
NTAPI
LdrProcessDriverModule(PLDR_DATA_TABLE_ENTRY LdrEntry,
                       PUNICODE_STRING FileName,
                       PLDR_DATA_TABLE_ENTRY *ModuleObject)
{
    NTSTATUS Status;
    UNICODE_STRING BaseName, BaseDirectory;
    PLOAD_IMPORTS LoadedImports = (PVOID)-2;
    PCHAR MissingApiName, Buffer;
    PWCHAR MissingDriverName;
    PVOID DriverBase = LdrEntry->DllBase;

    /* Allocate a buffer we'll use for names */
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   MAXIMUM_FILENAME_LENGTH,
                                   TAG_LDR_WSTR);
    if (!Buffer)
    {
        /* Fail */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Check for a separator */
    if (FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        PWCHAR p;
        ULONG BaseLength;

        /* Loop the path until we get to the base name */
        p = &FileName->Buffer[FileName->Length / sizeof(WCHAR)];
        while (*(p - 1) != OBJ_NAME_PATH_SEPARATOR) p--;

        /* Get the length */
        BaseLength = (ULONG)(&FileName->Buffer[FileName->Length / sizeof(WCHAR)] - p);
        BaseLength *= sizeof(WCHAR);

        /* Setup the string */
        BaseName.Length = (USHORT)BaseLength;
        BaseName.Buffer = p;
    }
    else
    {
        /* Otherwise, we already have a base name */
        BaseName.Length = FileName->Length;
        BaseName.Buffer = FileName->Buffer;
    }

    /* Setup the maximum length */
    BaseName.MaximumLength = BaseName.Length;

    /* Now compute the base directory */
    BaseDirectory = *FileName;
    BaseDirectory.Length -= BaseName.Length;
    BaseDirectory.MaximumLength = BaseDirectory.Length;

    /* Resolve imports */
    MissingApiName = Buffer;
    Status = MiResolveImageReferences(DriverBase,
                                      &BaseDirectory,
                                      NULL,
                                      &MissingApiName,
                                      &MissingDriverName,
                                      &LoadedImports);

    /* Free the temporary buffer */
    ExFreePoolWithTag(Buffer, TAG_LDR_WSTR);

    /* Check the result of the imports resolution */
    if (!NT_SUCCESS(Status)) return Status;

    /* Return */
    *ModuleObject = LdrEntry;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopCheckUnloadDriver(
    _In_ PDRIVER_OBJECT Object,
    _Out_ PBOOLEAN OutIsSafeToUnload)
{
    KIRQL OldIrql;
    PDEVICE_OBJECT DeviceObject;

    OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);

    DeviceObject = Object->DeviceObject;

    if ((!DeviceObject && (Object->Flags & DRVO_UNLOAD_INVOKED)) ||
        (!(Object->Flags & DRVO_FILESYSTEM_DRIVER) &&
         DeviceObject &&
         ((IoGetDevObjExtension(DeviceObject))->ExtensionFlags & DOE_UNLOAD_PENDING)))
    {
          KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);
          ObDereferenceObject(Object);
          return STATUS_SUCCESS;
    }

    *OutIsSafeToUnload = TRUE;

    while (DeviceObject)
    {
        (IoGetDevObjExtension(DeviceObject))->ExtensionFlags |= DOE_UNLOAD_PENDING;

        if (DeviceObject->ReferenceCount || DeviceObject->AttachedDevice)
            *OutIsSafeToUnload = FALSE;

        DeviceObject = DeviceObject->NextDevice;
    }

    if ((Object->Flags & DRVO_FILESYSTEM_DRIVER) && Object->DeviceObject)
        *OutIsSafeToUnload = FALSE;

    if (*OutIsSafeToUnload)
        Object->Flags |= DRVO_UNLOAD_INVOKED;

    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);

    return STATUS_UNSUCCESSFUL;
}

/* Unloads a device driver.

   DriverServiceName
      Name of the service to unload (registry key).
   UnloadPnpDrivers
      Whether to unload Plug & Plug or only legacy drivers.
      If this parameter is set to FALSE, the routine will unload only legacy drivers.
*/
NTSTATUS
NTAPI
IopUnloadDriver(
    _In_ PUNICODE_STRING DriverServiceName,
    _In_ BOOLEAN IsUnloadPnpManagers)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode(); 
    LOAD_UNLOAD_PARAMS LoadUnloadDriverContext;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Destination;
    UNICODE_STRING DriverName;
    PDRIVER_OBJECT DriverObject;
    HANDLE Handle;
    HANDLE KeyHandle;
    PVOID Buffer = NULL;
    BOOLEAN IsSafeToUnload;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("IopUnloadDriver: Unload '%wZ'\n", DriverServiceName);

    if (PreviousMode != KernelMode && !IsUnloadPnpManagers)
    {
        if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, PreviousMode))
        {
            ASSERT(FALSE); // IoDbgBreakPointEx();
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        _SEH2_TRY
        {
            if (((ULONG_PTR)DriverServiceName >= MmUserProbeAddress))
                *(PUNICODE_STRING)&DriverName = *((PUNICODE_STRING)MmUserProbeAddress);
            else
                *(PUNICODE_STRING)&DriverName = *(DriverServiceName);

            if (!DriverName.Length)
                return STATUS_INVALID_PARAMETER;

            ProbeForRead(DriverName.Buffer, DriverName.Length, sizeof(WCHAR));

            Buffer = ExAllocatePoolWithQuotaTag(PagedPool, DriverName.Length, '  oI');

            RtlCopyMemory(Buffer, DriverName.Buffer, DriverName.Length);
            DriverName.Buffer = Buffer;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            if (Buffer)
                ExFreePoolWithTag(Buffer, '  oI');

            return _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        Status = ZwUnloadDriver(&DriverName);

        ExFreePoolWithTag(Buffer, '  oI');

        return Status;
    }

    Status = IopOpenRegistryKey(&KeyHandle, NULL, DriverServiceName, KEY_READ, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopUnloadDriver: Status %X\n", Status);
        return Status;
    }

    Status = IopGetDriverNameFromKeyNode(KeyHandle, &Destination);
    ObCloseHandle(KeyHandle, KernelMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopUnloadDriver: Status %X\n", Status);
        return Status;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &Destination,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               NULL,
                               NULL);

    Status = ObOpenObjectByName(&ObjectAttributes,
                                IoDriverObjectType,
                                KernelMode,
                                NULL,
                                FILE_READ_DATA,
                                NULL,
                                &Handle);

    ExFreePool(Destination.Buffer);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopUnloadDriver: Status %X\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       IoDriverObjectType,
                                       KernelMode,
                                       (PVOID *)&DriverObject,
                                       NULL);
    ObCloseHandle(Handle, KernelMode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopUnloadDriver: Status %X\n", Status);
        return Status;
    }

    if (!DriverObject->DriverUnload || !DriverObject->DriverSection)
    {
        DPRINT1("IopUnloadDriver: STATUS_INVALID_DEVICE_REQUEST\n");
        ObDereferenceObject(DriverObject);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (!IsUnloadPnpManagers && !IopIsLegacyDriver(DriverObject))
    {
        DPRINT1("IopUnloadDriver: STATUS_INVALID_DEVICE_REQUEST\n");
        ObDereferenceObject(DriverObject);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Status = IopCheckUnloadDriver(DriverObject, &IsSafeToUnload);
    if (NT_SUCCESS(Status))
        return Status;

    DPRINT1("IopUnloadDriver: Status %X\n", Status);

    if (!IsSafeToUnload)
    {
        DPRINT1("IopUnloadDriver: STATUS_INVALID_DEVICE_REQUEST\n");
        ObDereferenceObject(DriverObject);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (PsGetCurrentProcess() == PsInitialSystemProcess)
    {
        DriverObject->DriverUnload(DriverObject);
        goto Exit;
    }

    LoadUnloadDriverContext.DriverObject = DriverObject;

    KeInitializeEvent(&LoadUnloadDriverContext.Event, NotificationEvent, FALSE);

    ExInitializeWorkItem(&LoadUnloadDriverContext.WorkItem, IopLoadUnloadDriver, &LoadUnloadDriverContext);
    ExQueueWorkItem(&LoadUnloadDriverContext.WorkItem, DelayedWorkQueue);

    KeWaitForSingleObject(&LoadUnloadDriverContext.Event, Executive, KernelMode, FALSE, NULL);

Exit:

    ObMakeTemporaryObject(DriverObject);
    ObDereferenceObject(DriverObject);
    ObDereferenceObject(DriverObject);

    DPRINT("IopUnloadDriver: return %X\n", Status);
    return Status;
}

VOID
NTAPI
IopReinitializeDrivers(VOID)
{
    PDRIVER_REINIT_ITEM ReinitItem;
    PLIST_ENTRY Entry;

    /* Get the first entry and start looping */
    Entry = ExInterlockedRemoveHeadList(&DriverReinitListHead,
                                        &DriverReinitListLock);
    while (Entry)
    {
        /* Get the item */
        ReinitItem = CONTAINING_RECORD(Entry, DRIVER_REINIT_ITEM, ItemEntry);

        /* Increment reinitialization counter */
        ReinitItem->DriverObject->DriverExtension->Count++;

        /* Remove the device object flag */
        ReinitItem->DriverObject->Flags &= ~DRVO_REINIT_REGISTERED;

        /* Call the routine */
        ReinitItem->ReinitRoutine(ReinitItem->DriverObject,
                                  ReinitItem->Context,
                                  ReinitItem->DriverObject->
                                  DriverExtension->Count);

        /* Free the entry */
        ExFreePool(Entry);

        /* Move to the next one */
        Entry = ExInterlockedRemoveHeadList(&DriverReinitListHead,
                                            &DriverReinitListLock);
    }
}

VOID
NTAPI
IopReinitializeBootDrivers(VOID)
{
    PDRIVER_REINIT_ITEM ReinitItem;
    PLIST_ENTRY Entry;

    /* Get the first entry and start looping */
    Entry = ExInterlockedRemoveHeadList(&DriverBootReinitListHead,
                                        &DriverBootReinitListLock);
    while (Entry)
    {
        /* Get the item */
        ReinitItem = CONTAINING_RECORD(Entry, DRIVER_REINIT_ITEM, ItemEntry);

        /* Increment reinitialization counter */
        ReinitItem->DriverObject->DriverExtension->Count++;

        /* Remove the device object flag */
        ReinitItem->DriverObject->Flags &= ~DRVO_BOOTREINIT_REGISTERED;

        /* Call the routine */
        ReinitItem->ReinitRoutine(ReinitItem->DriverObject,
                                  ReinitItem->Context,
                                  ReinitItem->DriverObject->
                                  DriverExtension->Count);

        /* Free the entry */
        ExFreePool(Entry);

        /* Move to the next one */
        Entry = ExInterlockedRemoveHeadList(&DriverBootReinitListHead,
                                            &DriverBootReinitListLock);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
IoCreateDriver(
    _In_ PUNICODE_STRING DriverName OPTIONAL,
    _In_ PDRIVER_INITIALIZE InitializationFunction)
{
    PDRIVER_OBJECT DriverObject;
    HANDLE Handle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING LocalDriverName;
    UNICODE_STRING ServiceKeyName;
    WCHAR NameBuffer[0x3C];
    ULONG Size;
    ULONG ix;
    USHORT NameLength;
    NTSTATUS Status;

    PAGED_CODE();

    if (DriverName)
    {
        /* So we can avoid another code path, use a local var */
        LocalDriverName = *DriverName;
        DPRINT("IopCreateDriver(): '%wZ'\n", &LocalDriverName);
    }
    else
    {
        /* Create a random name and set up the string */
        NameLength = _snwprintf(NameBuffer, (0x3C - 1), L"\\Driver\\%08u", KeTickCount.LowPart);

        LocalDriverName.Length = (NameLength * sizeof(WCHAR));
        LocalDriverName.MaximumLength = ((NameLength + 1) * sizeof(WCHAR));
        LocalDriverName.Buffer = NameBuffer;

        DPRINT("IopCreateDriver(): '%wZ'\n", &LocalDriverName);
    }

    /* Initialize the Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &LocalDriverName,
                               (OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               NULL,
                               NULL);

    /* Create the Object */
    Size = (sizeof(DRIVER_OBJECT) + sizeof(EXTENDED_DRIVER_EXTENSION));

    Status = ObCreateObject(KernelMode,
                            IoDriverObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            Size,
                            0,
                            0,
                            (PVOID*)&DriverObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDriver: Status %X\n", Status);
        return Status;
    }
    RtlZeroMemory(DriverObject, Size);

    DPRINT("IopCreateDriver(): %p, '%wZ'\n", DriverObject, &LocalDriverName);

    /* Set up the Object */
    DriverObject->Type = IO_TYPE_DRIVER;
    DriverObject->Size = sizeof(DRIVER_OBJECT);
    DriverObject->Flags = DRVO_BUILTIN_DRIVER;

    DriverObject->DriverExtension = (PDRIVER_EXTENSION)(DriverObject + 1);
    DriverObject->DriverExtension->DriverObject = DriverObject;

    DriverObject->DriverInit = InitializationFunction;

    /* Loop all Major Functions */
    for (ix = 0; ix <= IRP_MJ_MAXIMUM_FUNCTION; ix++)
        /* Invalidate each function */
        DriverObject->MajorFunction[ix] = IopInvalidDeviceRequest;

    /* Set up the service key name buffer */
    ServiceKeyName.Buffer = ExAllocatePoolWithTag(NonPagedPool, (LocalDriverName.Length + sizeof(UNICODE_NULL)), TAG_IO);
    if (!ServiceKeyName.Buffer)
    {
        /* Fail */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the name and set it in the driver extension */
    RtlCopyUnicodeString(&ServiceKeyName, &LocalDriverName);

    DriverObject->DriverExtension->ServiceKeyName = ServiceKeyName;

    /* Add the Object and get its handle */
    Status = ObInsertObject(DriverObject, NULL, FILE_READ_DATA, 0x200, NULL, &Handle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDriver: Status %X\n", Status);
        return Status;
    }

    /* Now reference it */
    Status = ObReferenceObjectByHandle(Handle, 0, IoDriverObjectType, KernelMode, (PVOID*)&DriverObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ZwMakeTemporaryObject(Handle);
        ZwClose(Handle);
        return Status;
    }

    /* Close the extra handle */
    ZwClose(Handle);

    /* Make a copy of the driver name to store in the driver object */
    DriverObject->DriverName.MaximumLength = LocalDriverName.Length;

    DriverObject->DriverName.Buffer = ExAllocatePoolWithTag(PagedPool, DriverObject->DriverName.MaximumLength, TAG_IO);
    if (DriverObject->DriverName.Buffer)
    {
        RtlCopyUnicodeString(&DriverObject->DriverName, &LocalDriverName);
    }

    /* Finally, call its init function */
    Status = InitializationFunction(DriverObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
    }

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
IoDeleteDriver(IN PDRIVER_OBJECT DriverObject)
{
    /* Simply dereference the Object */
    ObDereferenceObject(DriverObject);
}

/*
 * @implemented
 */
VOID
NTAPI
IoRegisterBootDriverReinitialization(IN PDRIVER_OBJECT DriverObject,
                                     IN PDRIVER_REINITIALIZE ReinitRoutine,
                                     IN PVOID Context)
{
    PDRIVER_REINIT_ITEM ReinitItem;

    /* Allocate the entry */
    ReinitItem = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(DRIVER_REINIT_ITEM),
                                       TAG_REINIT);
    if (!ReinitItem) return;

    /* Fill it out */
    ReinitItem->DriverObject = DriverObject;
    ReinitItem->ReinitRoutine = ReinitRoutine;
    ReinitItem->Context = Context;

    /* Set the Driver Object flag and insert the entry into the list */
    DriverObject->Flags |= DRVO_BOOTREINIT_REGISTERED;
    ExInterlockedInsertTailList(&DriverBootReinitListHead,
                                &ReinitItem->ItemEntry,
                                &DriverBootReinitListLock);
}

/*
 * @implemented
 */
VOID
NTAPI
IoRegisterDriverReinitialization(IN PDRIVER_OBJECT DriverObject,
                                 IN PDRIVER_REINITIALIZE ReinitRoutine,
                                 IN PVOID Context)
{
    PDRIVER_REINIT_ITEM ReinitItem;

    /* Allocate the entry */
    ReinitItem = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(DRIVER_REINIT_ITEM),
                                       TAG_REINIT);
    if (!ReinitItem) return;

    /* Fill it out */
    ReinitItem->DriverObject = DriverObject;
    ReinitItem->ReinitRoutine = ReinitRoutine;
    ReinitItem->Context = Context;

    /* Set the Driver Object flag and insert the entry into the list */
    DriverObject->Flags |= DRVO_REINIT_REGISTERED;
    ExInterlockedInsertTailList(&DriverReinitListHead,
                                &ReinitItem->ItemEntry,
                                &DriverReinitListLock);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoAllocateDriverObjectExtension(IN PDRIVER_OBJECT DriverObject,
                                IN PVOID ClientIdentificationAddress,
                                IN ULONG DriverObjectExtensionSize,
                                OUT PVOID *DriverObjectExtension)
{
    KIRQL OldIrql;
    PIO_CLIENT_EXTENSION DriverExtensions, NewDriverExtension;
    BOOLEAN Inserted = FALSE;

    /* Assume failure */
    *DriverObjectExtension = NULL;

    /* Allocate the extension */
    NewDriverExtension = ExAllocatePoolWithTag(NonPagedPool,
                                               sizeof(IO_CLIENT_EXTENSION) +
                                               DriverObjectExtensionSize,
                                               TAG_DRIVER_EXTENSION);
    if (!NewDriverExtension) return STATUS_INSUFFICIENT_RESOURCES;

    /* Clear the extension for teh caller */
    RtlZeroMemory(NewDriverExtension,
                  sizeof(IO_CLIENT_EXTENSION) + DriverObjectExtensionSize);

    /* Acqure lock */
    OldIrql = KeRaiseIrqlToDpcLevel();

    /* Fill out the extension */
    NewDriverExtension->ClientIdentificationAddress = ClientIdentificationAddress;

    /* Loop the current extensions */
    DriverExtensions = IoGetDrvObjExtension(DriverObject)->
                       ClientDriverExtension;
    while (DriverExtensions)
    {
        /* Check if the identifier matches */
        if (DriverExtensions->ClientIdentificationAddress ==
            ClientIdentificationAddress)
        {
            /* We have a collision, break out */
            break;
        }

        /* Go to the next one */
        DriverExtensions = DriverExtensions->NextExtension;
    }

    /* Check if we didn't collide */
    if (!DriverExtensions)
    {
        /* Link this one in */
        NewDriverExtension->NextExtension =
            IoGetDrvObjExtension(DriverObject)->ClientDriverExtension;
        IoGetDrvObjExtension(DriverObject)->ClientDriverExtension =
            NewDriverExtension;
        Inserted = TRUE;
    }

    /* Release the lock */
    KeLowerIrql(OldIrql);

    /* Check if insertion failed */
    if (!Inserted)
    {
        /* Free the entry and fail */
        ExFreePoolWithTag(NewDriverExtension, TAG_DRIVER_EXTENSION);
        return STATUS_OBJECT_NAME_COLLISION;
    }

    /* Otherwise, return the pointer */
    *DriverObjectExtension = NewDriverExtension + 1;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PVOID
NTAPI
IoGetDriverObjectExtension(IN PDRIVER_OBJECT DriverObject,
                           IN PVOID ClientIdentificationAddress)
{
    KIRQL OldIrql;
    PIO_CLIENT_EXTENSION DriverExtensions;

    /* Acquire lock */
    OldIrql = KeRaiseIrqlToDpcLevel();

    /* Loop the list until we find the right one */
    DriverExtensions = IoGetDrvObjExtension(DriverObject)->ClientDriverExtension;
    while (DriverExtensions)
    {
        /* Check for a match */
        if (DriverExtensions->ClientIdentificationAddress ==
            ClientIdentificationAddress)
        {
            /* Break out */
            break;
        }

        /* Keep looping */
        DriverExtensions = DriverExtensions->NextExtension;
    }

    /* Release lock */
    KeLowerIrql(OldIrql);

    /* Return nothing or the extension */
    if (!DriverExtensions) return NULL;
    return DriverExtensions + 1;
}

NTSTATUS
NTAPI
IopPnpDriverStarted(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ HANDLE ServiceHandle,
    _In_ PUNICODE_STRING DriverPath)
{
    DPRINT("IopPnpDriverStarted: DriverObject %X, DeviceObject %X\n", DriverObject, DriverObject->DeviceObject);

    if (DriverObject->DeviceObject)
        goto Exit;

    if (DriverPath->Buffer == NULL)
        goto Exit;

    DPRINT("IopPnpDriverStarted: DriverPath %wZ\n", DriverPath);

    if (IopIsAnyDeviceInstanceEnabled(DriverPath, NULL, FALSE))
        goto Exit;

    DPRINT("IopPnpDriverStarted: DriverObject->Flags %X\n", DriverObject->Flags);

    if (!(DriverObject->Flags & DRVO_REINIT_REGISTERED))
    {
        IopDriverLoadingFailed(ServiceHandle, NULL);
        DPRINT1("IopPnpDriverStarted: return STATUS_PLUGPLAY_NO_DEVICE\n");
        return STATUS_PLUGPLAY_NO_DEVICE;
    }

Exit:
    IopDeleteLegacyKey(DriverObject);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopBuildFullDriverPath(
    _In_ PUNICODE_STRING DriverPath,
    _In_ HANDLE ServiceHandle,
    _Out_ PUNICODE_STRING OutModuleName)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo = NULL;
    PWSTR DriverPathBuffer;
    ULONG DriverPathLength;
    PWSTR ImagePath;
    PWSTR Buffer;
    PWSTR Path = NULL;
    ULONG PathLength = 0;
    PWSTR Ext;
    ULONG ExtLen = 0;
    ULONG Lenght;
    NTSTATUS Status;

    DPRINT("IopBuildFullDriverPath: Driver '%wZ'\n", DriverPath);

    OutModuleName->Length = 0;
    OutModuleName->MaximumLength = 0;
    OutModuleName->Buffer = 0;

    Status = IopGetRegistryValue(ServiceHandle, L"ImagePath", &ValueInfo);

    if (NT_SUCCESS(Status) && ValueInfo->DataLength)
    {
        DriverPathLength = (ValueInfo->DataLength - sizeof(WCHAR));
        DriverPathBuffer = (PWSTR)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);

        ImagePath = DriverPathBuffer;

        if (ImagePath[0] != '\\')
        {
            Path = L"\\SystemRoot\\";
            PathLength = 12;
        }

        Ext = NULL;
    }
    else
    {
        DriverPathBuffer = DriverPath->Buffer;
        DPRINT("IopBuildFullDriverPath: DriverPathBuffer '%S'\n", DriverPathBuffer);

        DriverPathLength = DriverPath->Length;
        DPRINT("IopBuildFullDriverPath: DriverPathLength %X\n", DriverPathLength);

        ImagePath = DriverPathBuffer;

        PathLength = 29;
        Path = L"\\SystemRoot\\System32\\Drivers\\";
        DPRINT("IopBuildFullDriverPath: Path '%S'\n", Path);

        ExtLen = 8;
        Ext = L".SYS";
        DPRINT("IopBuildFullDriverPath: Ext '%S'\n", Ext);

    }

    Lenght = (DriverPathLength + ExtLen + ((PathLength + 1) * sizeof(WCHAR)));

    Buffer = ExAllocatePoolWithTag(PagedPool, Lenght, '  oI');
    if (!Buffer)
    {
        ASSERT(FALSE); // IoDbgBreakPointEx();

        OutModuleName->MaximumLength = 0;
        OutModuleName->Buffer = NULL;

        if (ValueInfo)
            ExFreePoolWithTag(ValueInfo, 0);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    OutModuleName->Length = (Lenght - sizeof(WCHAR));
    OutModuleName->MaximumLength = Lenght;
    OutModuleName->Buffer = Buffer;

    if (Path)
    {
        RtlCopyMemory(Buffer, Path, (PathLength * sizeof(WCHAR)));
        ImagePath = DriverPathBuffer;
    }

    if (DriverPathLength)
        RtlCopyMemory(&OutModuleName->Buffer[PathLength], ImagePath, DriverPathLength);

    if (ExtLen)
        RtlCopyMemory((&OutModuleName->Buffer[PathLength] + DriverPathLength), Ext, ExtLen);

    OutModuleName->Buffer[OutModuleName->Length >> 1] = 0;

    if (ValueInfo)
        ExFreePoolWithTag(ValueInfo, 0);

    DPRINT("IopBuildFullDriverPath: OutModuleName '%wZ'\n", OutModuleName);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopLoadDriver(
    _In_ HANDLE ServiceHandle,
    _In_ BOOLEAN SafeBootModeFlag,
    _In_ BOOLEAN IsFilter,
    _Out_ NTSTATUS * OutInitStatus)
{
    UNICODE_STRING HardwareKeyName = RTL_CONSTANT_STRING(IO_REG_KEY_DESCRIPTIONSYSTEM);
    PKEY_VALUE_FULL_INFORMATION ValueInfo = NULL;
    PKEY_BASIC_INFORMATION KeyInfo = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PIMAGE_NT_HEADERS NtHeader;
    PUNICODE_STRING ObjectName;
    UNICODE_STRING DriverPath = { 0, 0, NULL };
    UNICODE_STRING DriverName;
    UNICODE_STRING ModuleName;
    KPROCESSOR_MODE AccessMode;
    PDRIVER_OBJECT driver;
    PLIST_ENTRY Entry;
    PVOID DriverSection;
    PVOID ImageBase;
    HANDLE Handle;
    ULONG ResultLength;
    ULONG ix;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopLoadDriver: %p, SafeBootModeFlag %X, IsFilter %X\n", ServiceHandle, SafeBootModeFlag, IsFilter);

    *OutInitStatus = STATUS_SUCCESS;

    DriverName.Buffer = NULL;
    ModuleName.Buffer = NULL;

    Status = NtQueryKey(ServiceHandle, KeyBasicInformation, NULL, 0, &ResultLength);

    if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL)
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);
        Status = STATUS_ILL_FORMED_SERVICE_ENTRY;
        goto Exit;
    }

    KeyInfo = ExAllocatePoolWithTag(NonPagedPool, (ResultLength + (4 * sizeof(WCHAR))), '  oI');
    if (!KeyInfo)
    {
        DPRINT1("IopLoadDriver: STATUS_INSUFFICIENT_RESOURCES\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Status = NtQueryKey(ServiceHandle, KeyBasicInformation, KeyInfo, ResultLength, &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);
        goto Exit;
    }

    ModuleName.Length = (USHORT)KeyInfo->NameLength;
    ModuleName.MaximumLength = ModuleName.Length + (4 * sizeof(WCHAR));
    ModuleName.Buffer = KeyInfo->Name;

    DriverPath.Buffer = ExAllocatePoolWithTag(PagedPool, ModuleName.Length + sizeof(WCHAR), '  oI');
    if (!DriverPath.Buffer)
    {
        DPRINT1("IopLoadDriver: STATUS_INSUFFICIENT_RESOURCES\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    DriverPath.Length = ModuleName.Length;
    DriverPath.MaximumLength = (ModuleName.Length + sizeof(WCHAR));
    RtlCopyMemory(DriverPath.Buffer, ModuleName.Buffer, ModuleName.Length);
    DriverPath.Buffer[DriverPath.Length / sizeof(WCHAR)] = 0;

    RtlAppendUnicodeToString(&ModuleName, L".SYS");

    if (SafeBootModeFlag && InitSafeBootMode)
    {
        DPRINT1("IopLoadDriver: FIXME InitSafeBootMode\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    ExAcquireResourceSharedLite(&PsLoadedModuleResource, TRUE);
    ASSERT(PsLoadedModuleList.Flink != NULL);

    for (Entry = PsLoadedModuleList.Flink;
         Entry != &PsLoadedModuleList;
         Entry = Entry->Flink)
    {
        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (RtlEqualUnicodeString(&ModuleName, &LdrEntry->BaseDllName, TRUE))
        {
            Status = STATUS_IMAGE_ALREADY_LOADED;
            ExReleaseResourceLite(&PsLoadedModuleResource);
            ModuleName.Buffer = NULL;
            goto Exit;
        }
    }
    ExReleaseResourceLite(&PsLoadedModuleResource);

    Status = IopBuildFullDriverPath(&DriverPath, ServiceHandle, &ModuleName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);
        ModuleName.Buffer = NULL;
        goto Exit;
    }

    Status = IopGetDriverNameFromKeyNode(ServiceHandle, &DriverName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);
        goto Exit;
    }

    InitializeObjectAttributes(&ObjectAttributes, &DriverName, OBJ_PERMANENT, NULL, NULL);

    ExAcquireResourceExclusiveLite(&IopDriverLoadResource, TRUE);

    Status = MmLoadSystemImage(&ModuleName, NULL, NULL, FALSE, &DriverSection, &ImageBase);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);

        if (Status != STATUS_IMAGE_ALREADY_LOADED)
        {
            ExReleaseResourceLite(&IopDriverLoadResource);
            goto Exit;
        }

        Status = ObOpenObjectByName(&ObjectAttributes,
                                    IoDriverObjectType,
                                    KernelMode,
                                    NULL,
                                    0,
                                    NULL,
                                    &Handle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopLoadDriver: Status %X\n", Status);
            ASSERT(FALSE); // IoDbgBreakPointEx();
            goto Exit;
        }

        AccessMode = KeGetCurrentThread()->PreviousMode;

        Status = ObReferenceObjectByHandle(Handle,
                                           0,
                                           IoDriverObjectType,
                                           AccessMode,
                                           (PVOID *)&driver,
                                           NULL);
        NtClose(Handle);

        if (NT_SUCCESS(Status))
        {
            DPRINT1("IopLoadDriver: FIXME IopResurrectDriver\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();
        }

        ExReleaseResourceLite(&IopDriverLoadResource);
        goto Exit;
    }

    /* Only probe (check) */
    NtHeader = RtlImageNtHeader(ImageBase);
    DPRINT1("IopLoadDriver: DriverPath %wZ\n", &DriverPath);

    Status = IopPrepareDriverLoading(&DriverPath, ServiceHandle, ImageBase, IsFilter);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);
        MmUnloadSystemImage(DriverSection);
        ExReleaseResourceLite(&IopDriverLoadResource);
        goto Exit;
    }

    AccessMode = KeGetCurrentThread()->PreviousMode;

    Status = ObCreateObject(AccessMode,
                            IoDriverObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            (sizeof(DRIVER_OBJECT) + sizeof(DRIVER_EXTENSION)),
                            0,
                            0,
                            (PVOID *)&driver);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);

        MmUnloadSystemImage(DriverSection);
        ExReleaseResourceLite(&IopDriverLoadResource);
        goto Exit;
    }

    RtlZeroMemory(driver, (sizeof(DRIVER_OBJECT) + sizeof(DRIVER_EXTENSION)));

    driver->Type = IO_TYPE_DRIVER;
    driver->Size = sizeof(DRIVER_OBJECT);

    driver->DriverExtension = (PDRIVER_EXTENSION)&driver[1];
    driver->DriverExtension->DriverObject = driver;

    for (ix = 0; ix <= IRP_MJ_MAXIMUM_FUNCTION; ix++)
        driver->MajorFunction[ix] = IopInvalidDeviceRequest;

    NtHeader = RtlImageNtHeader(ImageBase);
    if (!(NtHeader->OptionalHeader.DllCharacteristics & IMAGE_FILE_DLL))
        driver->Flags |= DRVO_LEGACY_DRIVER;

    driver->DriverInit = (PVOID)((ULONG_PTR)ImageBase + NtHeader->OptionalHeader.AddressOfEntryPoint);
    driver->DriverSection = DriverSection;
    driver->DriverStart = ImageBase;
    driver->DriverSize = NtHeader->OptionalHeader.SizeOfImage;

    Status = ObInsertObject(driver, NULL, FILE_READ_DATA, 0, NULL, &Handle);

    ExReleaseResourceLite(&IopDriverLoadResource);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);
        goto Exit;
    }

    AccessMode = KeGetCurrentThread()->PreviousMode;
    Status = ObReferenceObjectByHandle(Handle, 0, IoDriverObjectType, AccessMode, (PVOID *)&driver, NULL);

    ASSERT(Status == STATUS_SUCCESS);
    NtClose(Handle);

    driver->HardwareDatabase = &HardwareKeyName;

    driver->DriverName.Buffer = ExAllocatePoolWithTag(PagedPool, DriverName.MaximumLength, '  oI');
    if (!driver->DriverName.Buffer)
    {
        DPRINT1("IopLoadDriver: Allocate failed!\n");
        /* No return */
    }
    else
    {
        driver->DriverName.MaximumLength = DriverName.MaximumLength;
        driver->DriverName.Length = DriverName.Length;
        RtlCopyMemory(driver->DriverName.Buffer, DriverName.Buffer, DriverName.MaximumLength);
    }

    ObjectName = ExAllocatePoolWithTag(NonPagedPool, (1 * PAGE_SIZE), '  oI');
    if (!ObjectName)
    {
        DPRINT1("IopLoadDriver: STATUS_INSUFFICIENT_RESOURCES\n");

        ObMakeTemporaryObject(driver);
        ObDereferenceObject(driver);

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Status = NtQueryObject(ServiceHandle, ObjectNameInformation, ObjectName, (1 * PAGE_SIZE), &ix);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);

        ObMakeTemporaryObject(driver);
        ObDereferenceObject(driver);

        ExFreePoolWithTag(ObjectName, '  oI');
        goto Exit;
    }

#if 0
    KeQuerySystemTime(..); // Get timing interval for init driver?
#endif

    if (DriverPath.Buffer)
    {
        driver->DriverExtension->ServiceKeyName.Buffer = ExAllocatePoolWithTag(0, DriverPath.MaximumLength, '  oI');
        if (driver->DriverExtension->ServiceKeyName.Buffer)
        {
            driver->DriverExtension->ServiceKeyName.MaximumLength = DriverPath.MaximumLength;
            driver->DriverExtension->ServiceKeyName.Length = DriverPath.Length;
            RtlCopyMemory(driver->DriverExtension->ServiceKeyName.Buffer, DriverPath.Buffer, DriverPath.MaximumLength);
        }
    }

    //DPRINT1("IopLoadDriver: driver %X\n", driver); // debug initialization driver
    Status = driver->DriverInit(driver, ObjectName);
    //DPRINT1("IopLoadDriver: Status %X\n", Status); // debug initialization driver

    *OutInitStatus = Status;
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: [%X] %wZ DriverInit failed %X\n", driver, &driver->DriverName, Status);
        Status = STATUS_FAILED_DRIVER_ENTRY;
    }

#if 0
    KeQuerySystemTime(..); // Get timing interval for init driver?
#endif

    for (ix = 0; ix <= IRP_MJ_MAXIMUM_FUNCTION; ix++)
    {
        if (!driver->MajorFunction[ix])
        {
            ASSERT(driver->MajorFunction[ix] != NULL);
            driver->MajorFunction[ix] = IopInvalidDeviceRequest;
        }
    }

    ExFreePoolWithTag(ObjectName, '  oI');

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);
        ObMakeTemporaryObject(driver);
        ObDereferenceObject(driver);
        goto Exit;
    }

    if (!IopIsLegacyDriver(driver))
    {
        //DPRINT1("IopLoadDriver: Status %X\n", Status); // debug start driver
        Status = IopPnpDriverStarted(driver, ServiceHandle, &DriverPath);
        //DPRINT1("IopLoadDriver: Status %X\n", Status); // debug start driver

        if (NT_SUCCESS(Status))
        {
            MmFreeDriverInitialization(driver->DriverSection);
            IopReadyDeviceObjects(driver);
            goto Exit;
        }

        DPRINT1("IopLoadDriver: Status %X\n", Status);

        if (driver->DriverUnload)
        {
            driver->Flags |= DRVO_UNLOAD_INVOKED;
            driver->DriverUnload(driver);
        }
        else
        {
            DbgPrint("IopLoadDriver: PnP driver '%wZ' not supported DriverUnload().\n", &DriverName);
        }
    }
    else
    {
        DPRINT("IopLoadDriver: IopIsLegacyDriver - TRUE\n");
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopLoadDriver: Status %X\n", Status);
        ObMakeTemporaryObject(driver);
        ObDereferenceObject(driver);
        goto Exit;
    }

    MmFreeDriverInitialization(driver->DriverSection);
    IopReadyDeviceObjects(driver);

Exit:

    if (DriverName.Buffer)
        ExFreePoolWithTag(DriverName.Buffer, '  oI');

    if (KeyInfo)
        ExFreePoolWithTag(KeyInfo, '  oI');

    if (DriverPath.Buffer)
        ExFreePoolWithTag(DriverPath.Buffer, '  oI');

    if (ModuleName.Buffer)
        ExFreePool(ModuleName.Buffer);

    if (NT_SUCCESS(Status))
    {
        ObCloseHandle(ServiceHandle, KernelMode);
        return Status;
    }

    DPRINT1("IopLoadDriver: Status %X\n", Status);

    if (Status != STATUS_PLUGPLAY_NO_DEVICE && Status != STATUS_IMAGE_ALREADY_LOADED)
    {
        NTSTATUS Sts;

        IopDriverLoadingFailed(ServiceHandle, NULL);

        Sts = IopGetRegistryValue(ServiceHandle, L"ErrorControl", &ValueInfo);
        if (NT_SUCCESS(Sts))
        {
            if (ValueInfo->DataLength)
            {
                DPRINT1("IopLoadDriver: FIXME CmBootLastKnownGood\n");
                //ASSERT(FALSE); // IoDbgBreakPointEx();
            }

            ExFreePool(ValueInfo);
        }
    }

    ObCloseHandle(ServiceHandle, KernelMode);

    return Status;
}

VOID
NTAPI
IopLoadUnloadDriver(
    _In_ PVOID Context)
{
    PLOAD_UNLOAD_PARAMS LoadUnloadDriverContext = Context;
    PDRIVER_OBJECT DriverObject;
    HANDLE KeyHandle;
    NTSTATUS InitStatus;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopLoadUnloadDriver: Context %p\n", LoadUnloadDriverContext);

    DriverObject = LoadUnloadDriverContext->DriverObject;
    if (DriverObject)
    {
        DriverObject->DriverUnload(LoadUnloadDriverContext->DriverObject);
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    Status = IopOpenRegistryKey(&KeyHandle,
                                NULL,
                                (PUNICODE_STRING)LoadUnloadDriverContext->RegistryPath,
                                KEY_READ,
                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopLoadUnloadDriver: Status %X\n", Status);
        goto Exit;
    }

    Status = IopLoadDriver(KeyHandle, TRUE, FALSE, &InitStatus);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopLoadUnloadDriver: Status %X\n", Status);
    }

    if (Status == STATUS_FAILED_DRIVER_ENTRY)
        Status = InitStatus;
    else if (Status == STATUS_DRIVER_FAILED_PRIOR_UNLOAD)
        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    IopReinitializeDrivers();

Exit:

    LoadUnloadDriverContext->Status = Status;
    KeSetEvent(&LoadUnloadDriverContext->Event, IO_NO_INCREMENT, FALSE);
}

/* Loads a device driver.

   DriverServiceName
      Name of the service to load (registry key).
 */
NTSTATUS
NTAPI
NtLoadDriver(PUNICODE_STRING DriverServiceName)
{
    LOAD_UNLOAD_PARAMS LoadUnloadDriverContext;
    UNICODE_STRING DriverName = { 0, 0, NULL };
    KPROCESSOR_MODE PreviousMode;
    PETHREAD CurrentThread;
    PVOID Buffer = NULL;

    PAGED_CODE();
    DPRINT("NtLoadDriver: Driver '%wZ'\n", DriverServiceName);

    CurrentThread = PsGetCurrentThread();
    ASSERT(&CurrentThread->Tcb == KeGetCurrentThread());

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode == KernelMode)
    {
        DriverName.Length = DriverServiceName->Length;
        DriverName.MaximumLength = DriverServiceName->MaximumLength;
        DriverName.Buffer = DriverServiceName->Buffer;
    }
    else
    {
        if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, PreviousMode))
        {
            ASSERT(FALSE); // IoDbgBreakPointEx();
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        _SEH2_TRY
        {
            if (((ULONG_PTR)DriverServiceName >= MmUserProbeAddress))
                *(PUNICODE_STRING)&DriverName = *((PUNICODE_STRING)MmUserProbeAddress);
            else
                *(PUNICODE_STRING)&DriverName = *(DriverServiceName);

            if (!DriverName.Length)
                return STATUS_INVALID_PARAMETER;

            ProbeForRead(DriverName.Buffer, DriverName.Length, sizeof(WCHAR));

            Buffer = ExAllocatePoolWithQuotaTag(PagedPool, DriverName.Length, '  oI');

            RtlCopyMemory(Buffer, DriverName.Buffer, DriverName.Length);
            DriverName.Buffer = Buffer;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            if (Buffer)
                ExFreePoolWithTag(Buffer, '  oI');

            return _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    KeInitializeEvent(&LoadUnloadDriverContext.Event, NotificationEvent, FALSE);

    LoadUnloadDriverContext.DriverObject = NULL;
    LoadUnloadDriverContext.RegistryPath = &DriverName;

    CurrentThread = PsGetCurrentThread();
    ASSERT(&CurrentThread->Tcb == KeGetCurrentThread());

    if (PsGetCurrentProcess() == PsInitialSystemProcess)
    {
        IopLoadUnloadDriver(&LoadUnloadDriverContext);
    }
    else
    {
        ExInitializeWorkItem(&LoadUnloadDriverContext.WorkItem,
                             IopLoadUnloadDriver,
                             &LoadUnloadDriverContext);

        ExQueueWorkItem(&LoadUnloadDriverContext.WorkItem, DelayedWorkQueue);

        KeWaitForSingleObject(&LoadUnloadDriverContext.Event, UserRequest, KernelMode, FALSE, NULL);
    }

    if (Buffer)
        ExFreePoolWithTag(DriverName.Buffer, '  oI');

    return LoadUnloadDriverContext.Status;
}

/*
 * NtUnloadDriver
 *
 * Unloads a legacy device driver.
 *
 * Parameters
 *    DriverServiceName
 *       Name of the service to unload (registry key).
 *
 * Return Value
 *    Status
 *
 * Status
 *    implemented
 */

NTSTATUS NTAPI
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
    return IopUnloadDriver(DriverServiceName, FALSE);
}

/* EOF */
