
#include <rxgkrnl.h>
//#define NDEBUG
#include <debug.h>


#define TAG_VIDEO_PORT          'PDIV'
#define TAG_VIDEO_PORT_BUFFER   '\0mpV'
#define GUID_STRING_LENGTH (38 * sizeof(WCHAR))
#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE         1
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING   2

BOOLEAN       VideoPortUseNewKey = TRUE;

/* Im not including an entire header for one definietion... */
extern "C"
{
NTSTATUS
NTAPI
ObCloseHandle(
    _In_ HANDLE Handle,
    _In_ KPROCESSOR_MODE AccessMode
);
}
/*
 * All these registry APIs are a direct rip from videoprt. literally
 * Honestly even internally dxgkrnl seems to do some similar stuff.
 * I wonder if the dxgkrnl varients are just direct rips to..
 * this is just to make win32k happy after all.
 */
NTSTATUS
NTAPI
IntCreateRegistryPath(
    IN PCUNICODE_STRING DriverRegistryPath,
    IN ULONG DeviceNumber,
    OUT PUNICODE_STRING DeviceRegistryPath)
{
    static WCHAR RegistryMachineSystem[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\";
    static WCHAR CurrentControlSet[] = L"CURRENTCONTROLSET\\";
    static WCHAR ControlSet[] = L"CONTROLSET";
    static WCHAR Insert1[] = L"Hardware Profiles\\Current\\System\\CurrentControlSet\\";
    static WCHAR Insert2[] = L"\\Device";
    UNICODE_STRING DeviceNumberString;
    WCHAR DeviceNumberBuffer[20];
    BOOLEAN Valid;
    UNICODE_STRING AfterControlSet;
    NTSTATUS Status;

    AfterControlSet = *DriverRegistryPath;

    /* Convert DeviceNumber to string */
    DeviceNumberString.Length = 0;
    DeviceNumberString.MaximumLength = sizeof(DeviceNumberBuffer);
    DeviceNumberString.Buffer = DeviceNumberBuffer;
    Status = RtlIntegerToUnicodeString(DeviceNumber, 10, &DeviceNumberString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlIntegerToUnicodeString(%u) returned 0x%08x\n", DeviceNumber, Status);
        return Status;
    }

    /* Check if path begins with \\REGISTRY\\MACHINE\\SYSTEM\\ */
    Valid = (DriverRegistryPath->Length > sizeof(RegistryMachineSystem) &&
             0 == _wcsnicmp(DriverRegistryPath->Buffer, RegistryMachineSystem,
                            wcslen(RegistryMachineSystem)));
    if (Valid)
    {
        AfterControlSet.Buffer += wcslen(RegistryMachineSystem);
        AfterControlSet.Length -= sizeof(RegistryMachineSystem) - sizeof(UNICODE_NULL);

        /* Check if path contains CURRENTCONTROLSET */
        if (AfterControlSet.Length > sizeof(CurrentControlSet) &&
            0 == _wcsnicmp(AfterControlSet.Buffer, CurrentControlSet, wcslen(CurrentControlSet)))
        {
            AfterControlSet.Buffer += wcslen(CurrentControlSet);
            AfterControlSet.Length -= sizeof(CurrentControlSet) - sizeof(UNICODE_NULL);
        }
        /* Check if path contains CONTROLSETnum */
        else if (AfterControlSet.Length > sizeof(ControlSet) &&
                 0 == _wcsnicmp(AfterControlSet.Buffer, ControlSet, wcslen(ControlSet)))
        {
            AfterControlSet.Buffer += wcslen(ControlSet);
            AfterControlSet.Length -= sizeof(ControlSet) - sizeof(UNICODE_NULL);
            while (AfterControlSet.Length > 0 &&
                    *AfterControlSet.Buffer >= L'0' &&
                    *AfterControlSet.Buffer <= L'9')
            {
                AfterControlSet.Buffer++;
                AfterControlSet.Length -= sizeof(WCHAR);
            }

            Valid = (AfterControlSet.Length > 0 && L'\\' == *AfterControlSet.Buffer);
            AfterControlSet.Buffer++;
            AfterControlSet.Length -= sizeof(WCHAR);
            AfterControlSet.MaximumLength = AfterControlSet.Length;
        }
        else
        {
            Valid = FALSE;
        }
    }

    if (!VideoPortUseNewKey)
    {
        DPRINT1("Using old registry key as 'UseNewKey' is FALSE\n");
        Valid = FALSE;
    }
    else if (Valid)
    {
        DeviceRegistryPath->MaximumLength = DriverRegistryPath->Length + sizeof(Insert1) + sizeof(Insert2)
                                          + DeviceNumberString.Length;
        DeviceRegistryPath->Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool,
                                                           DeviceRegistryPath->MaximumLength,
                                                           TAG_VIDEO_PORT);
        if (DeviceRegistryPath->Buffer != NULL)
        {
            /* Build device path */
            wcsncpy(DeviceRegistryPath->Buffer,
                    DriverRegistryPath->Buffer,
                    AfterControlSet.Buffer - DriverRegistryPath->Buffer);
            DeviceRegistryPath->Length = (AfterControlSet.Buffer - DriverRegistryPath->Buffer) * sizeof(WCHAR);
            RtlAppendUnicodeToString(DeviceRegistryPath, Insert1);
            RtlAppendUnicodeStringToString(DeviceRegistryPath, &AfterControlSet);
            RtlAppendUnicodeToString(DeviceRegistryPath, Insert2);
            RtlAppendUnicodeStringToString(DeviceRegistryPath, &DeviceNumberString);

            /* Check if registry key exists */
            Valid = NT_SUCCESS(RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, DeviceRegistryPath->Buffer));

            if (!Valid)
                ExFreePoolWithTag(DeviceRegistryPath->Buffer, TAG_VIDEO_PORT);
        }
        else
        {
            Valid = FALSE;
        }
    }
    else
    {
        DPRINT1("Unparsable registry path %wZ\n", DriverRegistryPath);
    }

    /* If path doesn't point to *ControlSet*, use DriverRegistryPath directly */
    if (!Valid)
    {
        DeviceRegistryPath->MaximumLength = DriverRegistryPath->Length + sizeof(Insert2) + DeviceNumberString.Length;
        DeviceRegistryPath->Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool,
                                                           DeviceRegistryPath->MaximumLength,
                                                           TAG_VIDEO_PORT);

        if (!DeviceRegistryPath->Buffer)
            return STATUS_NO_MEMORY;

        RtlCopyUnicodeString(DeviceRegistryPath, DriverRegistryPath);
        RtlAppendUnicodeToString(DeviceRegistryPath, Insert2);
        RtlAppendUnicodeStringToString(DeviceRegistryPath, &DeviceNumberString);
    }

    DPRINT1("Formatted registry key '%wZ' -> '%wZ'\n",
           DriverRegistryPath, DeviceRegistryPath);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntVideoPortAddDeviceMapLink(
    PRXGK_PRIVATE_EXTENSION DeviceExtension)
{
    PUNICODE_STRING RegistryPath;
    WCHAR DeviceBuffer[20];
    UNICODE_STRING DeviceName;
    WCHAR SymlinkBuffer[20];
    UNICODE_STRING SymlinkName;
    ULONG DeviceNumber;
    NTSTATUS Status;

    /* Create a unicode device name. */
    DeviceNumber = 0;
    swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceExtension->InternalDeviceNumber);

    if (VideoPortUseNewKey)
        RegistryPath = &DeviceExtension->NewRegistryPath;
    else
        RegistryPath = &DeviceExtension->RegistryPath;

    /* Add entry to DEVICEMAP\VIDEO key in registry. */
    Status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                   L"VIDEO",
                                   DeviceBuffer,
                                   REG_SZ,
                                   RegistryPath->Buffer,
                                   RegistryPath->Length + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create DEVICEMAP registry entry: 0x%X\n", Status);
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                   L"VIDEO",
                                   L"MaxObjectNumber",
                                   REG_DWORD,
                                   &DeviceNumber,
                                   sizeof(DeviceNumber));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to write MaxObjectNumber: 0x%X\n", Status);
        return Status;
    }

    /* Create symbolic link "\??\DISPLAYx" */
    swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber + 1);
    RtlInitUnicodeString(&SymlinkName, SymlinkBuffer);
    RtlInitUnicodeString(&DeviceName, DeviceBuffer);
    Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create symbolic link: 0x%X\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntDuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString)
{
    if (SourceString == NULL ||
        DestinationString == NULL ||
        SourceString->Length > SourceString->MaximumLength ||
        (SourceString->Length == 0 && SourceString->MaximumLength > 0 && SourceString->Buffer == NULL) ||
        Flags == RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING ||
        Flags >= 4)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((SourceString->Length == 0) &&
        (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                   RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }
    else
    {
        USHORT DestMaxLength = SourceString->Length;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestMaxLength += sizeof(UNICODE_NULL);

        DestinationString->Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, DestMaxLength, TAG_VIDEO_PORT);
        if (DestinationString->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
        DestinationString->Length = SourceString->Length;
        DestinationString->MaximumLength = DestMaxLength;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntCopyRegistryKey(
    _In_ HANDLE SourceKeyHandle,
    _In_ HANDLE DestKeyHandle)
{
    PVOID InfoBuffer;
    PKEY_BASIC_INFORMATION KeyInformation;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Index, InformationLength, RequiredLength;
    UNICODE_STRING NameString;
    NTSTATUS Status;
    HANDLE SourceSubKeyHandle, DestSubKeyHandle;

    /* Start with no buffer, set initial size */
    InfoBuffer = NULL;
    InformationLength = 256;

    /* Start looping with key index 0 */
    Index = 0;
    while (TRUE)
    {
        /* Check if we have no buffer */
        if (InfoBuffer == NULL)
        {
            /* Allocate a new buffer */
            InfoBuffer = ExAllocatePoolWithTag(PagedPool,
                                               InformationLength,
                                               TAG_VIDEO_PORT_BUFFER);
            if (InfoBuffer == NULL)
            {
                DPRINT1( "Could not allocate buffer for key info\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        /* Enumerate the next sub-key */
        KeyInformation = (PKEY_BASIC_INFORMATION)InfoBuffer;
        Status = ZwEnumerateKey(SourceKeyHandle,
                                Index,
                                KeyBasicInformation,
                                KeyInformation,
                                InformationLength,
                                &RequiredLength);
        if ((Status == STATUS_BUFFER_OVERFLOW) ||
            (Status == STATUS_BUFFER_TOO_SMALL))
        {
            /* Free the buffer and remember the required size */
            ExFreePoolWithTag(InfoBuffer, TAG_VIDEO_PORT_BUFFER);
            InfoBuffer = NULL;
            InformationLength = RequiredLength;

            /* Try again */
            continue;
        }
        else if (Status == STATUS_NO_MORE_ENTRIES)
        {
            /* We are done with the sub-keys */
            break;
        }
        else if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwEnumerateKey failed, status 0x%lx\n", Status);
            goto Cleanup;
        }

        /* Initialize a unicode string from the key name */
        NameString.Buffer = KeyInformation->Name;
        NameString.Length = (USHORT)KeyInformation->NameLength;
        NameString.MaximumLength = NameString.Length;

        /* Initialize object attributes and open the source sub-key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &NameString,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   SourceKeyHandle,
                                   NULL);
        Status = ZwOpenKey(&SourceSubKeyHandle, KEY_READ, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("failed to open the source key.\n");
            goto Cleanup;
        }

        /* Initialize object attributes and create the dest sub-key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &NameString,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   DestKeyHandle,
                                   NULL);
        Status = ZwCreateKey(&DestSubKeyHandle,
                             KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
             DPRINT1("failed to create the destination key.\n");
            ObCloseHandle(SourceSubKeyHandle, KernelMode);
            goto Cleanup;
        }

        /* Recursively copy the sub-key */
        Status = IntCopyRegistryKey(SourceSubKeyHandle, DestSubKeyHandle);
        if (!NT_SUCCESS(Status))
        {
            /* Just warn, but continue with the remaining sub-keys */
             DPRINT1("failed to copy subkey '%wZ'.\n", &NameString);
        }

        /* Close the sub-key handles */
        ObCloseHandle(SourceSubKeyHandle, KernelMode);
        ObCloseHandle(DestSubKeyHandle, KernelMode);

        /* Next sub-key */
        Index++;
    }

    /* Start looping with value index 0 */
    Index = 0;
    while (TRUE)
    {
        /* Check if we have no buffer */
        if (InfoBuffer == NULL)
        {
            /* Allocate a new buffer */
            InfoBuffer = ExAllocatePoolWithTag(PagedPool,
                                               InformationLength,
                                               TAG_VIDEO_PORT_BUFFER);
            if (InfoBuffer == NULL)
            {
                 DPRINT1("Could not allocate buffer for key values\n");
                return Status;
            }
        }

        /* Enumerate the next value */
        KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)InfoBuffer;
        Status = ZwEnumerateValueKey(SourceKeyHandle,
                                     Index,
                                     KeyValueFullInformation,
                                     KeyValueInformation,
                                     InformationLength,
                                     &RequiredLength);
        if ((Status == STATUS_BUFFER_OVERFLOW) ||
            (Status == STATUS_BUFFER_TOO_SMALL))
        {
            /* Free the buffer and remember the required size */
            ExFreePoolWithTag(InfoBuffer, TAG_VIDEO_PORT_BUFFER);
            InfoBuffer = NULL;
            InformationLength = RequiredLength;

            /* Try again */
            continue;
        }
        else if (Status == STATUS_NO_MORE_ENTRIES)
        {
            /* We are done with the values */
            Status = STATUS_SUCCESS;
            break;
        }
        else if (!NT_SUCCESS(Status))
        {
             DPRINT1("ZwEnumerateValueKey failed, status 0x%lx\n", Status);
            goto Cleanup;
        }

        /* Initialize a unicode string from the value name */
        NameString.Buffer = KeyValueInformation->Name;
        NameString.Length = (USHORT)KeyValueInformation->NameLength;
        NameString.MaximumLength = NameString.Length;

        /* Create the key value in the destination key */
        Status = ZwSetValueKey(DestKeyHandle,
                               &NameString,
                               KeyValueInformation->TitleIndex,
                               KeyValueInformation->Type,
                               (PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset,
                               KeyValueInformation->DataLength);
        if (!NT_SUCCESS(Status))
        {
            /* Just warn, but continue with the remaining sub-keys */
             DPRINT1("failed to set value '%wZ'.\n", &NameString);
        }

        /* Next subkey */
        Index++;
    }

Cleanup:
    /* Free the buffer and return the failure code */
    if (InfoBuffer != NULL)
		ExFreePoolWithTag(InfoBuffer, TAG_VIDEO_PORT_BUFFER);
    return Status;
}

NTSTATUS
NTAPI
IntCreateNewRegistryPath(
        PRXGK_PRIVATE_EXTENSION DeviceExtension)
{
    static UNICODE_STRING VideoIdValueName = RTL_CONSTANT_STRING(L"VideoId");
    static UNICODE_STRING ControlVideoPathName =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video\\");
    HANDLE DevInstRegKey, SettingsKey, NewKey;
    UCHAR VideoIdBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + GUID_STRING_LENGTH];
    UNICODE_STRING VideoIdString;
    UUID VideoId;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInformation ;
    NTSTATUS Status;
    ULONG ResultLength;
    USHORT KeyMaxLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PWCHAR InstanceIdBuffer;

    if (!DeviceExtension->MiniportPdo)
    {
        Status = IntDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                           &DeviceExtension->RegistryPath,
                                           &DeviceExtension->NewRegistryPath);
        if (!NT_SUCCESS(Status))
            DPRINT1("IntDuplicateUnicodeString() failed with status 0x%lx\n", Status);
        return Status;
    }

    /* Open the hardware key: HKLM\System\CurrentControlSet\Enum\... */
    Status = IoOpenDeviceRegistryKey(DeviceExtension->MiniportPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &DevInstRegKey);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("IoOpenDeviceRegistryKey failed: status 0x%lx\n", Status);
        return Status;
    }

    /* Query the VideoId value */
    ValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)VideoIdBuffer;
    Status = ZwQueryValueKey(DevInstRegKey,
                             &VideoIdValueName,
                             KeyValuePartialInformation,
                             ValueInformation,
                             sizeof(VideoIdBuffer),
                             &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        /* Create a new video Id */
        Status = ExUuidCreate(&VideoId);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ExUuidCreate failed: status 0x%lx\n", Status);
            ObCloseHandle(DevInstRegKey, KernelMode);
            return Status;
        }

        /* Convert the GUID into a string */
        Status = RtlStringFromGUID((const GUID )VideoId, &VideoIdString);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlStringFromGUID failed: status 0x%lx\n", Status);
            ObCloseHandle(DevInstRegKey, KernelMode);
            return Status;
        }

        /* Copy the GUID String to our buffer */
        ValueInformation->DataLength = min(VideoIdString.Length, GUID_STRING_LENGTH);
        RtlCopyMemory(ValueInformation->Data,
                      VideoIdString.Buffer,
                      ValueInformation->DataLength);

        /* Free the GUID string */
        RtlFreeUnicodeString(&VideoIdString);

        /* Write the VideoId registry value */
        Status = ZwSetValueKey(DevInstRegKey,
                               &VideoIdValueName,
                               0,
                               REG_SZ,
                               ValueInformation->Data,
                               ValueInformation->DataLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwSetValueKey failed: status 0x%lx\n", Status);
            ObCloseHandle(DevInstRegKey, KernelMode);
            return Status;
        }
    }

    /* Initialize the VideoId string from the registry data */
    VideoIdString.Buffer = (PWCHAR)ValueInformation->Data;
    VideoIdString.Length = (USHORT)ValueInformation->DataLength;
    VideoIdString.MaximumLength = VideoIdString.Length;

    /* Close the hardware key */
    ObCloseHandle(DevInstRegKey, KernelMode);

    /* Calculate the size needed for the new registry path name */
    KeyMaxLength = ControlVideoPathName.Length +
                   VideoIdString.Length +
                   sizeof(L"\\0000");

    /* Allocate the path name buffer */
    DeviceExtension->NewRegistryPath.Length = 0;
    DeviceExtension->NewRegistryPath.MaximumLength = KeyMaxLength;
    DeviceExtension->NewRegistryPath.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool,
                                                                    KeyMaxLength,
                                                                    TAG_VIDEO_PORT);
    if (DeviceExtension->NewRegistryPath.Buffer == NULL)
    {
        DPRINT1("Failed to allocate key name buffer.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the root key name and append the VideoId string */
    RtlCopyUnicodeString(&DeviceExtension->NewRegistryPath,
                         &ControlVideoPathName);
    RtlAppendUnicodeStringToString(&DeviceExtension->NewRegistryPath,
                                   &VideoIdString);

    /* Check if we have the key already */
    Status = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                 DeviceExtension->NewRegistryPath.Buffer);
    if (Status != STATUS_SUCCESS)
    {
        /* Try to create the new key */
        Status = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                      DeviceExtension->NewRegistryPath.Buffer);
    }

    /* Append a the instance path */ /// \todo HACK
    RtlAppendUnicodeToString(&DeviceExtension->NewRegistryPath, L"\\");
    InstanceIdBuffer = DeviceExtension->NewRegistryPath.Buffer +
        DeviceExtension->NewRegistryPath.Length / sizeof(WCHAR);
    RtlAppendUnicodeToString(&DeviceExtension->NewRegistryPath, L"0000");

    /* Write instance ID */
    swprintf(InstanceIdBuffer, L"%04u", 0);

    /* Check if the name exists */
    Status = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                 DeviceExtension->NewRegistryPath.Buffer);
    if (Status != STATUS_SUCCESS)
    {
        /* Try to create the new key */
        Status = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                      DeviceExtension->NewRegistryPath.Buffer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed create key '%wZ'\n", &DeviceExtension->NewRegistryPath);
            return Status;
        }

        /* Open the new key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceExtension->NewRegistryPath,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(&NewKey, KEY_WRITE, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open settings key. Status 0x%lx\n", Status);
            return Status;
        }

        /* Open the device profile key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceExtension->RegistryPath,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(&SettingsKey, KEY_READ, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open settings key. Status 0x%lx\n", Status);
            ObCloseHandle(NewKey, KernelMode);
            return Status;
        }

        /* Copy the registry data from the legacy key */
        Status = IntCopyRegistryKey(SettingsKey, NewKey);

        /* Close the key handles */
        ObCloseHandle(SettingsKey, KernelMode);
        ObCloseHandle(NewKey, KernelMode);
    }

    return Status;
}
