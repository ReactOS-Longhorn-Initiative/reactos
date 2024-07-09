/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/deviface.c
 * PURPOSE:         Device interface functions
 *
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 *                  Matthew Brace (ismarc@austin.rr.com)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* FIXME: This should be somewhere global instead of having 20 different versions */
#define GUID_STRING_CHARS 38
#define GUID_STRING_BYTES (GUID_STRING_CHARS * sizeof(WCHAR))
C_ASSERT(sizeof(L"{01234567-89ab-cdef-0123-456789abcdef}") == GUID_STRING_BYTES + sizeof(UNICODE_NULL));

extern ERESOURCE PpRegistryDeviceResource;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
IopParseSymbolicLinkName(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _Out_ PUNICODE_STRING OutPrefixName,
    _Out_ PUNICODE_STRING OutEnumName,
    _Out_ PUNICODE_STRING OutGuidName,
    _Out_ PUNICODE_STRING OutRefName,
    _Out_ PBOOLEAN OutIsRefString,
    _Out_ GUID* OutGuid)
{
    UNICODE_STRING GuidString;
    GUID Guid;
    PWCHAR Ptr;
    ULONG ix;
    USHORT Count;
    USHORT ReferenceStart = 0;
    BOOLEAN IsRefString;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("IopParseSymbolicLinkName: SymbolicLink '%wZ'\n", SymbolicLinkName);

    if (!SymbolicLinkName)
    {
        DPRINT1("IopParseSymbolicLinkName: STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (!SymbolicLinkName->Buffer)
    {
        DPRINT1("IopParseSymbolicLinkName: STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (!SymbolicLinkName->Length)
    {
        DPRINT1("IopParseSymbolicLinkName: STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (SymbolicLinkName->Length < 0x55)
    {
        DPRINT1("IopParseSymbolicLinkName: SymbolicLinkName->Length %X\n", SymbolicLinkName->Length);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (RtlCompareMemory(SymbolicLinkName->Buffer, L"\\\\?\\", (4 * sizeof(WCHAR))) != (4 * sizeof(WCHAR)) &&
        RtlCompareMemory(SymbolicLinkName->Buffer, L"\\??\\", (4 * sizeof(WCHAR))) != (4 * sizeof(WCHAR)))
    {
        DPRINT1("IopParseSymbolicLinkName: STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    Ptr = &SymbolicLinkName->Buffer[4]; // skip "\??\"
    Count = (SymbolicLinkName->Length / sizeof(WCHAR));

    for (ix = 0; ix < (Count - 4); ix++, Ptr++)
    {
        if(*Ptr == L'\\')
        {
            DPRINT("IopParseSymbolicLinkName: ReferenceStart %X, Link '%wZ'\n", ReferenceStart, SymbolicLinkName);
            ReferenceStart = (ix + 5);
            break;
        }
    }

    if (ReferenceStart)
    {
        IsRefString = TRUE;
    }
    else
    {
        ReferenceStart = (Count + 1); // to end string
        IsRefString = FALSE;
    }

    GuidString.Length = 0x4C;
    GuidString.MaximumLength = 0x4C;
    GuidString.Buffer = &SymbolicLinkName->Buffer[ReferenceStart - 0x27];

    Status = RtlGUIDFromString(&GuidString, &Guid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopParseSymbolicLinkName: RtlGUIDFromString() failed for GUID '%wZ' in '%wZ'\n", &GuidString, SymbolicLinkName);
        DPRINT1("IopParseSymbolicLinkName: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (OutPrefixName)
    {
        OutPrefixName->Length = 8;
        OutPrefixName->MaximumLength = OutPrefixName->Length;
        OutPrefixName->Buffer = SymbolicLinkName->Buffer;

        //DPRINT1("IopParseSymbolicLinkName: OutPrefix '%wZ'\n", OutPrefixName);
    }

    if (OutEnumName)
    {
        OutEnumName->Length = (ReferenceStart * sizeof(WCHAR) - 0x58);
        OutEnumName->MaximumLength = OutEnumName->Length;
        OutEnumName->Buffer = &SymbolicLinkName->Buffer[4];

        //DPRINT1("IopParseSymbolicLinkName: OutEnum '%wZ'\n", OutEnumName);
    }

    if (OutGuidName)
    {
        OutGuidName->Length = 0x4C;
        OutGuidName->MaximumLength = OutGuidName->Length;
        OutGuidName->Buffer = &SymbolicLinkName->Buffer[ReferenceStart - 0x27];

        //DPRINT1("IopParseSymbolicLinkName: OutGuid '%wZ'\n", OutGuidName);
    }

    if (OutRefName)
    {
        if (IsRefString)
        {
            OutRefName->Length = (SymbolicLinkName->Length - ReferenceStart * sizeof(WCHAR));
            OutRefName->MaximumLength = OutRefName->Length;
            OutRefName->Buffer = &SymbolicLinkName->Buffer[ReferenceStart];

            //DPRINT1("IopParseSymbolicLinkName: OutRef '%wZ'\n", OutRefName);
        }
        else
        {
            OutRefName->Length = 0;
            OutRefName->MaximumLength = OutRefName->Length;
            OutRefName->Buffer = NULL;

            //DPRINT1("IopParseSymbolicLinkName: OutRef is ''\n");
        }
    }

    if (OutIsRefString)
        *OutIsRefString = IsRefString;

    if (OutGuid)
        RtlCopyMemory(OutGuid, &Guid, sizeof(*OutGuid));

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopDropReferenceString(
    _Out_ PUNICODE_STRING OutString,
    _In_ PUNICODE_STRING InString)
{
    UNICODE_STRING RefName;
    BOOLEAN IsRefString;
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(InString);
    ASSERT(OutString);

    Status = IopParseSymbolicLinkName(InString, NULL, NULL, NULL, &RefName, &IsRefString, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDropReferenceString: Status %X\n", Status);
        RtlZeroMemory(&OutString, sizeof(OutString));
        return Status;
    }

    if (IsRefString)
        OutString->Length = (InString->Length - RefName.Length - sizeof(WCHAR));
    else
        OutString->Length = InString->Length;

    OutString->MaximumLength = OutString->Length;
    OutString->Buffer = InString->Buffer;

    //DPRINT1("IopDropReferenceString: In  '%wZ'\n", InString);
    //DPRINT1("IopDropReferenceString: Out '%wZ'\n", OutString);

    return Status;
}

NTSTATUS
NTAPI
IopBuildGlobalSymbolicLinkString(
    _In_ PUNICODE_STRING InString,
    _Out_ PUNICODE_STRING OutGlobalString)
{
    UNICODE_STRING Source;
    NTSTATUS Status;
    USHORT length;

    PAGED_CODE();
    DPRINT("IopBuildGlobalSymbolicLinkString: InString '%wZ'\n", InString);

    if (RtlCompareMemory(InString->Buffer, L"\\\\?\\", (4 * sizeof(WCHAR))) != (4 * sizeof(WCHAR)) &&
        RtlCompareMemory(InString->Buffer, L"\\??\\", (4 * sizeof(WCHAR))) != (4 * sizeof(WCHAR)))
    {
        DPRINT1("IopBuildGlobalSymbolicLinkString: STATUS_INVALID_PARAMETER\n");
        return STATUS_INVALID_PARAMETER;
    }

    length = (InString->Length + (6 * sizeof(WCHAR)));

    Status = PnpAllocateUnicodeString(OutGlobalString, length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopBuildGlobalSymbolicLinkString: Status %X\n", Status);
        return Status;
    }

    Status = RtlAppendUnicodeToString(OutGlobalString, L"\\GLOBAL??\\");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopBuildGlobalSymbolicLinkString: Status %X\n", Status);
        ASSERT(NT_SUCCESS(Status));
        RtlFreeUnicodeString(OutGlobalString);
        return Status;
    }

    Source.Length = (InString->Length - (4 * sizeof(WCHAR)));
    Source.MaximumLength = (InString->MaximumLength - (4 * sizeof(WCHAR)));
    Source.Buffer = &InString->Buffer[4];

    Status = RtlAppendUnicodeStringToString(OutGlobalString, &Source);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopBuildGlobalSymbolicLinkString: Status %X\n", Status);
        ASSERT(NT_SUCCESS(Status));
        RtlFreeUnicodeString(OutGlobalString);
        return Status;
    }

    ASSERT(OutGlobalString->Length == length);

    return Status;
}

NTSTATUS
NTAPI
IopReplaceSeperatorWithPound(
    _In_ PUNICODE_STRING InString,
    _Out_ PUNICODE_STRING OutString)
{
    PWSTR InChar;
    PWSTR OutChar;
    ULONG ix;
    USHORT InStringLen;
    WCHAR Char;

    PAGED_CODE();
    //DPRINT("IopReplaceSeperatorWithPound: InString '%wZ'\n", InString);

    ASSERT(InString);
    ASSERT(OutString);

    if (InString->Length > OutString->MaximumLength)
    {
        DPRINT1("IopReplaceSeperatorWithPound: STATUS_BUFFER_TOO_SMALL. In %X, Out %X\n", InString->Length, OutString->MaximumLength);
        return STATUS_BUFFER_TOO_SMALL;
    }

    InChar = InString->Buffer;
    OutChar = OutString->Buffer;

    InStringLen = InString->Length / sizeof(WCHAR);

    for (ix = 0; ix < InStringLen; ix++, InChar++, OutChar++)
    {
        Char = *InChar;

        if (*InChar == '\\' || Char == '/')
            *OutChar = '#';
        else
            *OutChar = Char;
    }

    OutString->Length = InString->Length;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopOpenOrCreateDeviceInterfaceSubKeys(
    _Out_ PHANDLE OutInterfaceHandle,
    _Out_ PULONG OutInterfaceDisposition,
    _Out_ PHANDLE OutInterfaceInstanceHandle,
    _Out_ PULONG OutInterfaceInstanceDisposition,
    _In_ HANDLE InterfaceClassHandle,
    _In_ PUNICODE_STRING InterfaceName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN IsCreateOrOpen)
{
    UNICODE_STRING DestinationString;
    UNICODE_STRING RefName;
    HANDLE ParentHandle;
    HANDLE KeyHandle;
    ULONG Disposition;
    WCHAR CharBuffer;
    BOOLEAN IsRefString = FALSE;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopOpenOrCreateDeviceInterfaceSubKeys: Interface %wZ, IsCreateOrOpen %X\n", InterfaceName, IsCreateOrOpen);

    Status = PnpAllocateUnicodeString(&DestinationString, InterfaceName->Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenOrCreateDeviceInterfaceSubKeys: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return Status;
    }

    RtlCopyUnicodeString(&DestinationString, InterfaceName);
    DPRINT("IopOpenOrCreateDeviceInterfaceSubKeys: Destination '%wZ'\n", &DestinationString);

    Status = IopParseSymbolicLinkName(&DestinationString, NULL, NULL, NULL, &RefName, &IsRefString, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenOrCreateDeviceInterfaceSubKeys: Status %X\n", Status);
        ASSERT(NT_SUCCESS(Status));
        RtlFreeUnicodeString(&DestinationString);
        return Status;
    }

    if (IsRefString)
    {
        RefName.Length += sizeof(WCHAR);
        RefName.MaximumLength += sizeof(WCHAR);
        RefName.Buffer--;

        DPRINT1("IopOpenOrCreateDeviceInterfaceSubKeys: IsRefString\n");

        DestinationString.Length = (USHORT)((ULONG_PTR)RefName.Buffer - (ULONG_PTR)DestinationString.Buffer);
        DestinationString.MaximumLength = DestinationString.Length;
    }
    else
    {
        RefName.Length = sizeof(CharBuffer);
        RefName.MaximumLength = RefName.Length;
        RefName.Buffer = &CharBuffer;
    }

    DestinationString.Buffer[0] = '#';
    DestinationString.Buffer[1] = '#';
    DestinationString.Buffer[2] = '?';
    DestinationString.Buffer[3] = '#';

    IopReplaceSeperatorWithPound(&DestinationString, &DestinationString);

    if (IsCreateOrOpen)
    {
        Status = IopCreateRegistryKeyEx(&ParentHandle,
                                        InterfaceClassHandle,
                                        &DestinationString,
                                        DesiredAccess,
                                        REG_OPTION_NON_VOLATILE,
                                        &Disposition);
    }
    else
    {
        Status = IopOpenRegistryKeyEx(&ParentHandle, InterfaceClassHandle, &DestinationString, DesiredAccess);
        Disposition = 2;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenOrCreateDeviceInterfaceSubKeys: [%X] Iface '%wZ'\n", IsCreateOrOpen, InterfaceName);
        DPRINT1("IopOpenOrCreateDeviceInterfaceSubKeys: Status %X for '%wZ'\n", Status, &DestinationString);
        goto Exit;
    }

    RefName.Buffer[0] = '#';

    if (IsCreateOrOpen)
    {
         Status = IopCreateRegistryKeyEx(&KeyHandle,
                                         ParentHandle,
                                         &RefName,
                                         DesiredAccess,
                                         REG_OPTION_NON_VOLATILE,
                                         OutInterfaceInstanceDisposition);
    }
    else
    {
        Status = IopOpenRegistryKeyEx(&KeyHandle, ParentHandle, &RefName, DesiredAccess);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenOrCreateDeviceInterfaceSubKeys: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();

        if (Disposition == 1)
            ZwDeleteKey(ParentHandle);

        ZwClose(ParentHandle);
        goto Exit;
    }

    if (OutInterfaceHandle)
        *OutInterfaceHandle = ParentHandle;
    else
        ZwClose(ParentHandle);

    if (OutInterfaceDisposition)
        *OutInterfaceDisposition = Disposition;

    if (OutInterfaceInstanceHandle)
        *OutInterfaceInstanceHandle = KeyHandle;
    else
        ZwClose(KeyHandle);

Exit:
    RtlFreeUnicodeString(&DestinationString);
    return Status;
}

NTSTATUS
NTAPI
IopDeviceInterfaceKeysFromSymbolicLink(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE OutInterfaceClassHandle,
    _Out_ PHANDLE OutInterfaceHandle,
    _Out_ PHANDLE OutInterfaceInstanceHandle)
{
    UNICODE_STRING DeviceClassesName = RTL_CONSTANT_STRING(IO_REG_KEY_DEVICECLASSES);
    UNICODE_STRING GuidName;
    HANDLE InterfaceClassHandle;
    HANDLE KeyHandle;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopDeviceInterfaceKeysFromSymbolicLink: SymbolicLink '%wZ'\n", SymbolicLinkName);

    Status = IopParseSymbolicLinkName(SymbolicLinkName, NULL, NULL, &GuidName, NULL, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDeviceInterfaceKeysFromSymbolicLink: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return Status;
    }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PpRegistryDeviceResource, TRUE);

    Status = IopOpenRegistryKeyEx(&KeyHandle, NULL, &DeviceClassesName, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDeviceInterfaceKeysFromSymbolicLink: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit;
    }

    Status = IopOpenRegistryKeyEx(&InterfaceClassHandle, KeyHandle, &GuidName, KEY_READ);
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDeviceInterfaceKeysFromSymbolicLink: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit;
    }

    Status = IopOpenOrCreateDeviceInterfaceSubKeys(OutInterfaceHandle,
                                                   NULL,
                                                   OutInterfaceInstanceHandle,
                                                   NULL,
                                                   InterfaceClassHandle,
                                                   SymbolicLinkName,
                                                   DesiredAccess,
                                                   FALSE);

    if (NT_SUCCESS(Status) && OutInterfaceClassHandle)
        *OutInterfaceClassHandle = InterfaceClassHandle;
    else
        ZwClose(InterfaceClassHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDeviceInterfaceKeysFromSymbolicLink: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

Exit:
    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    return Status;
}

NTSTATUS
NTAPI
PiDeferSetInterfaceState(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PUNICODE_STRING SymbolicLinkName)
{
    PDEVNODE_INTERFACE_STATE State;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PiDeferSetInterfaceState: [%p] SymbolicLink '%wZ'\n", DeviceNode, SymbolicLinkName);

    ASSERT(ExIsResourceAcquiredExclusiveLite(&PpRegistryDeviceResource));
    //ASSERT(PiIsPnpRegistryLocked(TRUE));

    State = ExAllocatePoolWithTag(PagedPool, sizeof(*State), '  pP');
    if (!State)
    {
        DPRINT1("PiDeferSetInterfaceState: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = PnpAllocateUnicodeString(&State->SymbolicLinkName, SymbolicLinkName->Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PiDeferSetInterfaceState: STATUS_INSUFFICIENT_RESOURCES\n");
        ExFreePoolWithTag(State, '  pP');
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&State->SymbolicLinkName, SymbolicLinkName);

    InsertTailList(&DeviceNode->PendedSetInterfaceState, &State->Link);

    return STATUS_SUCCESS;
}

VOID 
NTAPI
PiRemoveDeferredSetInterfaceState(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PCUNICODE_STRING SymbolicLinkName)
{
    PDEVNODE_INTERFACE_STATE State;
    PLIST_ENTRY Entry;

    PAGED_CODE();
    DPRINT("PiRemoveDeferredSetInterfaceState: [%p] SymbolicLink %wZ\n", DeviceNode, SymbolicLinkName);

    ASSERT(ExIsResourceAcquiredExclusiveLite(&PpRegistryDeviceResource));
    //ASSERT(PiIsPnpRegistryLocked(TRUE));

    for (Entry = DeviceNode->PendedSetInterfaceState.Flink;
         Entry != &DeviceNode->PendedSetInterfaceState;
         Entry = Entry->Flink)
    {
        State = CONTAINING_RECORD(Entry, DEVNODE_INTERFACE_STATE, Link);

        if (RtlEqualUnicodeString(&State->SymbolicLinkName, SymbolicLinkName, TRUE))
        {
            RemoveEntryList(&State->Link);

            ExFreePoolWithTag(State->SymbolicLinkName.Buffer, '  pP');
            ExFreePoolWithTag(State, '  pP');

            break;
        }
    }

    return;
}

NTSTATUS
NTAPI
IopProcessSetInterfaceState(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ BOOLEAN Enable,
    _In_ BOOLEAN PdoNotStarted)
{
    UNICODE_STRING ReferenceCountName = RTL_CONSTANT_STRING(L"ReferenceCount");
    UNICODE_STRING ControlName = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING LinkedName = RTL_CONSTANT_STRING(L"Linked");
    UNICODE_STRING MinusReferenceString;
    UNICODE_STRING GlobalSymbolicLink;
    UNICODE_STRING DestinationString;
    UNICODE_STRING ValueName;
    PEXTENDED_DEVOBJ_EXTENSION DeviceObjectExtension;
    PKEY_VALUE_FULL_INFORMATION ValueInfo = NULL;
    HANDLE InterfaceInstanceHandle = NULL;
    HANDLE InstanceControlHandle = NULL;
    HANDLE InterfaceClassHandle = NULL;
    HANDLE ParentControlHandle = NULL;
    HANDLE InterfaceHandle = NULL;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    PVOID PropertyBuffer = NULL;
    GUID DeviceGuid;
    ULONG ReferenceCount;
    ULONG Linked;
    USHORT Length;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopProcessSetInterfaceState: SymbolicLink '%wZ', Enable %X\n", SymbolicLinkName, Enable);

    Status = IopParseSymbolicLinkName(SymbolicLinkName, NULL, NULL, NULL, NULL, NULL, &DeviceGuid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit;
    }

    Status = IopDropReferenceString(&MinusReferenceString, SymbolicLinkName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit;
    }

    Status = IopBuildGlobalSymbolicLinkString(&MinusReferenceString, &GlobalSymbolicLink);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit;
    }

    Status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    (KEY_READ | KEY_WRITE),
                                                    &InterfaceClassHandle,
                                                    &InterfaceHandle,
                                                    &InterfaceInstanceHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit1;
    }

    Status = IopCreateRegistryKeyEx(&ParentControlHandle,
                                    InterfaceHandle,
                                    &ControlName,
                                    KEY_READ,
                                    REG_OPTION_VOLATILE,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit1;
    }

    Status = IopGetRegistryValue(InterfaceHandle, L"DeviceInstance", &ValueInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit2;
    }

    Status = IopCreateRegistryKeyEx(&InstanceControlHandle,
                                    InterfaceInstanceHandle,
                                    &ControlName,
                                    KEY_READ,
                                    REG_OPTION_VOLATILE,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        ExFreePool(ValueInfo);
        InstanceControlHandle = NULL;
        goto Exit2;
    }

    if (ValueInfo->Type != REG_SZ)
    {
        DPRINT1("IopProcessSetInterfaceState: ValueInfo->Type %X\n", ValueInfo->Type);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        PnpRegSzToString((PWCHAR)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset), ValueInfo->DataLength, &Length);

        ValueName.Length = Length;
        ValueName.MaximumLength = (USHORT)ValueInfo->DataLength;
        ValueName.Buffer = (PWCHAR)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);

        DeviceObject = IopDeviceObjectFromDeviceInstance(&ValueName);
        if (!DeviceObject)
        {
            DPRINT1("IopProcessSetInterfaceState: Value '%wZ'\n", &ValueName);
            ASSERT(FALSE); // IoDbgBreakPointEx();
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            DPRINT("IopProcessSetInterfaceState: Value '%wZ'\n", &ValueName);

            if (PdoNotStarted)
            {
                DeviceObjectExtension = IoGetDevObjExtension(DeviceObject);

                if (DeviceObjectExtension->ExtensionFlags & DOE_START_PENDING)
                {
                    DeviceNode = DeviceObjectExtension->DeviceNode;

                    if (Enable)
                    {
                        Status = PiDeferSetInterfaceState(DeviceNode, SymbolicLinkName);
                        if (NT_SUCCESS(Status))
                        {
                            ExFreePool(ValueInfo);
                            ObDereferenceObject(DeviceObject);
                            Status = STATUS_SUCCESS;
                            goto Exit2;
                        }
                    }
                    else
                    {
                        PiRemoveDeferredSetInterfaceState(DeviceNode, SymbolicLinkName);
                        ExFreePool(ValueInfo);
                        ObDereferenceObject(DeviceObject);
                        Status = STATUS_SUCCESS;
                        goto Exit2;
                    }
                }
            }

            if (!Enable || !NT_SUCCESS(Status))
                ObDereferenceObject(DeviceObject);
        }
    }

    if (!Enable)
        Status = STATUS_SUCCESS;

    ExFreePool(ValueInfo);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit2;
    }

    if (Enable)
    {
        ULONG BufferLength = 0x200;
        ULONG size;

        for (size = BufferLength; ; size = BufferLength)
        {
            PropertyBuffer = ExAllocatePoolWithTag(PagedPool, size, '  pP');
            if (!PropertyBuffer)
            {
                DPRINT1("IopProcessSetInterfaceState: STATUS_INSUFFICIENT_RESOURCES\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            Status = IoGetDeviceProperty(DeviceObject,
                                         DevicePropertyPhysicalDeviceObjectName,
                                         BufferLength,
                                         PropertyBuffer,
                                         &BufferLength);
            if (NT_SUCCESS(Status))
                break;

            ExFreePoolWithTag(PropertyBuffer, '  pP');

            if (Status != STATUS_BUFFER_TOO_SMALL)
                break;
        }

        ObDereferenceObject(DeviceObject);

        if (!NT_SUCCESS(Status) || !BufferLength)
        {
            DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
            ASSERT(FALSE); // IoDbgBreakPointEx();
            goto Exit2;
        }

        RtlInitUnicodeString(&DestinationString, PropertyBuffer);
    }

    ValueInfo = NULL;

    Status = IopGetRegistryValue(InstanceControlHandle, L"Linked", &ValueInfo);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Linked = 0;
    }
    else if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit3;
    }
    else if ((ValueInfo->Type != REG_DWORD) || (ValueInfo->DataLength != sizeof(ULONG)))
    {
        Linked = 0;
    }
    else
    {
        Linked = *(PULONG)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
    }

    if (ValueInfo)
        ExFreePool(ValueInfo);

    Status= IopGetRegistryValue(ParentControlHandle, L"ReferenceCount", &ValueInfo);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ReferenceCount = 0;
    }
    else
    {
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopProcessSetInterfaceState: Status %X\n", Status);
            ASSERT(FALSE); // IoDbgBreakPointEx();
            goto Exit3;
        }

        if ((ValueInfo->Type != REG_DWORD) || (ValueInfo->DataLength != sizeof(ULONG)))
            ReferenceCount = 0;
        else
            ReferenceCount = *(PULONG)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);

        ExFreePool(ValueInfo);
    }

    if (!Enable)
    {
        if (Linked)
        {
            if (ReferenceCount > 1)
            {
                ReferenceCount--;
            }
            else
            {
                ReferenceCount = 0;

                Status = IoDeleteSymbolicLink(&GlobalSymbolicLink);
                if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                {
                    DPRINT("IopProcessSetInterfaceState: STATUS_OBJECT_NAME_NOT_FOUND for '%ws' to delete!\n", GlobalSymbolicLink.Buffer);
                    Status = STATUS_SUCCESS;
                }
            }

            Linked = 0;
        }
        else
        {
            DPRINT("IopProcessSetInterfaceState: STATUS_OBJECT_NAME_NOT_FOUND\n");
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }
    else
    {
        if (Linked)
        {
            Status = STATUS_OBJECT_NAME_EXISTS;
            goto Exit3;
        }

        if (ReferenceCount > 0)
        {
            ReferenceCount++;
        }
        else
        {
            ReferenceCount = 1;

            Status = IoCreateSymbolicLink(&GlobalSymbolicLink, &DestinationString);
            if (Status == STATUS_OBJECT_NAME_COLLISION)
            {
                DPRINT("IopProcessSetInterfaceState: STATUS_OBJECT_NAME_COLLISION. '%ws' already exists!\n", GlobalSymbolicLink.Buffer);
                Status = STATUS_SUCCESS;
            }
        }

        Linked = 1;
    }

    if (!NT_SUCCESS(Status))
        goto Exit3;

    ZwSetValueKey(InstanceControlHandle, &LinkedName, 0, REG_DWORD, &Linked, sizeof(Linked));
    Status = ZwSetValueKey(ParentControlHandle, &ReferenceCountName, 0, REG_DWORD, &ReferenceCount, sizeof(ReferenceCount));

    if (Linked)
        PpSetDeviceClassChange(&GUID_DEVICE_INTERFACE_ARRIVAL, &DeviceGuid, SymbolicLinkName);
    else
        PpSetDeviceClassChange(&GUID_DEVICE_INTERFACE_REMOVAL, &DeviceGuid, SymbolicLinkName);

Exit3:
    if (PropertyBuffer)
        ExFreePoolWithTag(PropertyBuffer, '  pP');

Exit2:
    if (ParentControlHandle)
        ZwClose(ParentControlHandle);

    if (InstanceControlHandle)
        ZwClose(InstanceControlHandle);

Exit1:
    RtlFreeUnicodeString(&GlobalSymbolicLink);

    if (InterfaceHandle)
        ZwClose(InterfaceHandle);

    if (InterfaceInstanceHandle)
        ZwClose(InterfaceInstanceHandle);

    if (InterfaceClassHandle)
        ZwClose(InterfaceClassHandle);

    if (NT_SUCCESS(Status))
        return Status;

Exit:
    if (!Enable)
        Status = STATUS_SUCCESS;

    return Status;
}

VOID
NTAPI
IopDoDeferredSetInterfaceState(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVNODE_INTERFACE_STATE State;
    PLIST_ENTRY Entry;

    PAGED_CODE();
    DPRINT("IopDoDeferredSetInterfaceState: DeviceNode %p\n", DeviceNode);

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PpRegistryDeviceResource, TRUE);

    PpMarkDeviceStackStartPending(DeviceNode->PhysicalDeviceObject, FALSE);

    DPRINT("IopDoDeferredSetInterfaceState: Head PendedSetInterfaceState %p\n", &DeviceNode->PendedSetInterfaceState);

    for (Entry = &DeviceNode->PendedSetInterfaceState;
         !IsListEmpty(&DeviceNode->PendedSetInterfaceState);
        )
    {
        State = CONTAINING_RECORD(Entry->Flink, DEVNODE_INTERFACE_STATE, Link);
        RemoveHeadList(Entry);

        IopProcessSetInterfaceState(&State->SymbolicLinkName, TRUE, FALSE);

        DPRINT("IopDoDeferredSetInterfaceState: SymbolicLink '%wZ'\n", &State->SymbolicLinkName);

        ExFreePool(State->SymbolicLinkName.Buffer);
        ExFreePool(State);
    }

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
}

NTSTATUS
NTAPI
IopAllocateBuffer(
    _In_ PCLASS_INFO_BUFFER Info,
    _In_ SIZE_T NumberOfBytes)
{
    PCHAR Buffer;

    DPRINT("IopAllocateBuffer: Info %p, NumberOfBytes %X\n", Info, NumberOfBytes);
    PAGED_CODE();

    ASSERT(Info);

    Buffer = ExAllocatePoolWithTag(PagedPool, NumberOfBytes, '  pP');

    Info->StartBuffer = Buffer;
    Info->LastBuffer = Buffer;

    if (!Buffer)
    {
        DPRINT1("IopAllocateBuffer: Allocate failed\n");
        Info->MaxSize = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Info->MaxSize = NumberOfBytes;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopResizeBuffer(
    _In_ PCLASS_INFO_BUFFER Info,
    _In_ SIZE_T NewSize,
    _In_ BOOLEAN IsCopyBuffer)
{
    PCHAR NewBuffer;
    ULONG Used;

    DPRINT("IopResizeBuffer: Info %p, NewSize %X, IsCopyBuffer %X\n", Info, NewSize, IsCopyBuffer);
    PAGED_CODE();

    ASSERT(Info);

    NewBuffer = ExAllocatePoolWithTag(PagedPool, NewSize, '  pP');
    if (!NewBuffer)
    {
        DPRINT1("IopResizeBuffer: Allocate failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (IsCopyBuffer)
    {
        Used = (Info->LastBuffer - Info->StartBuffer);
        ASSERT(Used < NewSize);

        RtlCopyMemory(NewBuffer, Info->StartBuffer, Used);
        Info->LastBuffer = (NewBuffer + Used);
    }
    else
    {
        Info->LastBuffer = NewBuffer;
    }

    ExFreePoolWithTag(Info->StartBuffer, '  pP');

    Info->MaxSize = NewSize;
    Info->StartBuffer = NewBuffer;

    return STATUS_SUCCESS;
}

VOID
NTAPI
IopFreeBuffer(
    _In_ PCLASS_INFO_BUFFER Info)
{
    DPRINT("IopFreeBuffer: Info %p\n", Info);
    PAGED_CODE();

    ASSERT(Info);

    if (Info->StartBuffer)
        ExFreePoolWithTag(Info->StartBuffer, '  pP');

    Info->StartBuffer = NULL;
    Info->LastBuffer = NULL;
    Info->MaxSize = 0;
}

NTSTATUS
NTAPI
IopGetDeviceInterfaces(
    _In_ GUID* InterfaceClassGuid,
    _In_ PUNICODE_STRING InstancePath,
    _In_ ULONG Flags,
    _In_ BOOLEAN Param4,
    _Out_ PWSTR* SymbolicLinkList,
    _Out_ ULONG* OutSize)
{
    UNICODE_STRING DeviceClassesName = RTL_CONSTANT_STRING(IO_REG_KEY_DEVICECLASSES);
    UNICODE_STRING DeviceInstanceName = RTL_CONSTANT_STRING(L"DeviceInstance");
    UNICODE_STRING SymbolicLinkName = RTL_CONSTANT_STRING(L"SymbolicLink");
    UNICODE_STRING ControlName = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING LinkedName = RTL_CONSTANT_STRING(L"Linked");
    UNICODE_STRING DefaultClassName;
    UNICODE_STRING ClassGuidName;
    UNICODE_STRING SymbolicLink;
    UNICODE_STRING DevnodeName;
    UNICODE_STRING KeyName;
    PKEY_VALUE_FULL_INFORMATION ValueInfo = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    PKEY_BASIC_INFORMATION BasicInfo;
    CLASS_INFO_BUFFER DevnodeNameBuffer;
    CLASS_INFO_BUFFER SymLinkBuffer;
    CLASS_INFO_BUFFER ReturnBuffer;
    CLASS_INFO_BUFFER InfoBuffer;
    HANDLE DeviceClassesHandle;
    HANDLE InstanceHandle;
    HANDLE ControlHandle;
    HANDLE ClassHandle;
    HANDLE Handle;
    ULONG ResultLength;
    ULONG ix;
    ULONG jx;
    USHORT Length;
    BOOLEAN IsDefaultClass = FALSE;
    NTSTATUS Status;

    PAGED_CODE();

    *SymbolicLinkList = NULL;

    Status = RtlStringFromGUID(InterfaceClassGuid, &ClassGuidName);

    if (NT_SUCCESS(Status))
    {
        DPRINT("IopGetDeviceInterfaces: Instance '%wZ', Flags %X, Guid '%wZ'\n", InstancePath, Flags, &ClassGuidName);
    }
    else
    {
        DPRINT1("IopGetDeviceInterfaces: Instance '%wZ', Flags %X, Status %X\n", InstancePath, Flags, Status);
        goto Exit;
    }

    Status = IopAllocateBuffer(&ReturnBuffer, 0x1000);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeUnicodeString(&ClassGuidName);
        goto Exit;
    }

    Status = IopAllocateBuffer(&InfoBuffer, 0x200);
    if (!NT_SUCCESS(Status))
    {
        IopFreeBuffer(&ReturnBuffer);
        RtlFreeUnicodeString(&ClassGuidName);
        goto Exit;
    }

    Status = IopAllocateBuffer(&SymLinkBuffer, 0x400);
    if (!NT_SUCCESS(Status))
    {
        IopFreeBuffer(&InfoBuffer);
        IopFreeBuffer(&ReturnBuffer);
        RtlFreeUnicodeString(&ClassGuidName);
        goto Exit;
    }

    Status = IopAllocateBuffer(&DevnodeNameBuffer, 0x19C);
    if (!NT_SUCCESS(Status))
    {
        IopFreeBuffer(&SymLinkBuffer);
        IopFreeBuffer(&InfoBuffer);
        IopFreeBuffer(&ReturnBuffer);
        RtlFreeUnicodeString(&ClassGuidName);
        goto Exit;
    }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PpRegistryDeviceResource, TRUE);

    Status = IopCreateRegistryKeyEx(&DeviceClassesHandle,
                                    NULL,
                                    &DeviceClassesName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
        goto Exit1;
    }

    Status = IopOpenRegistryKeyEx(&ClassHandle, DeviceClassesHandle, &ClassGuidName, KEY_ALL_ACCESS);
    ZwClose(DeviceClassesHandle);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND || Status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        goto Exit3;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
        goto Exit1;
    }

    Status = IopGetRegistryValue(ClassHandle, REGSTR_VAL_DEFAULT, &ValueInfo);

    if (NT_SUCCESS(Status) && ValueInfo->Type == REG_SZ && ValueInfo->DataLength >= sizeof(WCHAR))
    {
        DefaultClassName.Length = (ValueInfo->DataLength - sizeof(WCHAR));
        DefaultClassName.MaximumLength = DefaultClassName.Length;
        DefaultClassName.Buffer = (PWSTR)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);

        DPRINT("IopGetDeviceInterfaces: DefaultClassName '%wZ'\n", &DefaultClassName);

        IsDefaultClass = TRUE;

        Status = IopOpenOrCreateDeviceInterfaceSubKeys(NULL,
                                                       NULL,
                                                       &Handle,
                                                       NULL,
                                                       ClassHandle,
                                                       &DefaultClassName,
                                                       KEY_READ,
                                                       FALSE);
        if (NT_SUCCESS(Status))
        {
            if (!(Flags & 1))
            {
                IsDefaultClass = FALSE;

                Status = IopOpenRegistryKeyEx(&ControlHandle, Handle, &ControlName, KEY_ALL_ACCESS);
                if (NT_SUCCESS(Status))
                {
                    ASSERT(InfoBuffer.MaxSize < 0x10);

                    Status = ZwQueryValueKey(ControlHandle,
                                             &LinkedName,
                                             KeyValuePartialInformation,
                                             InfoBuffer.StartBuffer,
                                             InfoBuffer.MaxSize,
                                             &ResultLength);

                    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

                    ZwClose(ControlHandle);

                    PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)InfoBuffer.StartBuffer;

                    if (NT_SUCCESS(Status) &&
                        PartialInfo->Type == REG_DWORD &&
                        PartialInfo->DataLength == sizeof(ULONG))
                    {
                        IsDefaultClass = (*(PULONG)PartialInfo->Data != 0);
                    }
                }
            }

            ZwClose(Handle);

            if (IsDefaultClass)
            {
                DPRINT1("IopGetDeviceInterfaces: IsDefaultClass TRUE\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();
            }
            else
            {
                ExFreePool(ValueInfo);
            }
        }
        else
        {
            IsDefaultClass = FALSE;
            ExFreePool(ValueInfo);
        }
    }
    else if (Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND)
    {
        if (NT_SUCCESS(Status))
        {
            ExFreePool(ValueInfo);
            Status = STATUS_UNSUCCESSFUL;
        }

        ZwClose(ClassHandle);
        goto Exit1;
    }

    ASSERT(InfoBuffer.MaxSize >= 0x18);

    ix = 0;
    while (TRUE)
    {
        Status = ZwEnumerateKey(ClassHandle,
                                ix,
                                KeyBasicInformation,
                                InfoBuffer.StartBuffer,
                                InfoBuffer.MaxSize,
                                &ResultLength);

        if (Status == STATUS_NO_MORE_ENTRIES)
            break;

        ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            IopResizeBuffer(&InfoBuffer, ResultLength, FALSE);
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
            ZwClose(ClassHandle);
            goto Exit2;
        }

        BasicInfo = (PKEY_BASIC_INFORMATION)InfoBuffer.StartBuffer;

        KeyName.Length = BasicInfo->NameLength;
        KeyName.MaximumLength = KeyName.Length;
        KeyName.Buffer = BasicInfo->Name;

        DPRINT("IopGetDeviceInterfaces: ix %X, Key '%wZ'\n", ix, &KeyName);

        Status = IopOpenRegistryKeyEx(&Handle, ClassHandle, &KeyName, KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
            ix++;
            continue;
        }

        ASSERT(DevnodeNameBuffer.MaxSize >= 0x10);

        while (TRUE)
        {
            Status = ZwQueryValueKey(Handle,
                                     &DeviceInstanceName,
                                     KeyValuePartialInformation,
                                     DevnodeNameBuffer.StartBuffer,
                                     DevnodeNameBuffer.MaxSize,
                                     &ResultLength);

            if (Status != STATUS_BUFFER_OVERFLOW)
                break;

            Status = IopResizeBuffer(&DevnodeNameBuffer, ResultLength, FALSE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
                ZwClose(Handle);
                ZwClose(ClassHandle);
                goto Exit2;
            }
        }

        ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

        PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)DevnodeNameBuffer.StartBuffer;

        if (!(NT_SUCCESS(Status) && PartialInfo->Type == REG_SZ && PartialInfo->DataLength > sizeof(WCHAR)))
        {
            DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
            ZwClose(Handle);
            ix++;
            continue;
        }

        DevnodeName.Length = (PartialInfo->DataLength - sizeof(WCHAR));
        DevnodeName.MaximumLength = KeyName.Length;
        DevnodeName.Buffer = (PWSTR)PartialInfo->Data;

        ASSERT(InfoBuffer.MaxSize >= 0x18);

        jx = 0;
        while (TRUE)
        {
            Status = ZwEnumerateKey(Handle,
                                    jx,
                                    KeyBasicInformation,
                                    InfoBuffer.StartBuffer,
                                    InfoBuffer.MaxSize,
                                    &ResultLength);

            if (Status == STATUS_NO_MORE_ENTRIES)
                break;

            ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

            if (Status == STATUS_BUFFER_OVERFLOW)
            {
                IopResizeBuffer(&InfoBuffer, ResultLength, FALSE);
                continue;
            }

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
                ZwClose(Handle);
                ZwClose(ClassHandle);
                goto Exit2;
            }

            BasicInfo = (PKEY_BASIC_INFORMATION)InfoBuffer.StartBuffer;

            KeyName.Length = BasicInfo->NameLength;
            KeyName.MaximumLength = KeyName.Length;
            KeyName.Buffer = BasicInfo->Name;

            DPRINT("IopGetDeviceInterfaces: Instance key %X enumerated '%wZ'\n", jx, &KeyName);

            Status = IopOpenRegistryKeyEx(&InstanceHandle, Handle, &KeyName, KEY_READ);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
                jx++;
                continue;
            }

            if (!(Flags & 1))
            {
                Status = IopOpenRegistryKeyEx(&ControlHandle, InstanceHandle, &ControlName, KEY_READ);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
                    goto Next;
                }

                ASSERT(InfoBuffer.MaxSize >= 0x10);

                Status = ZwQueryValueKey(ControlHandle,
                                         &LinkedName,
                                         KeyValuePartialInformation,
                                         InfoBuffer.StartBuffer,
                                         InfoBuffer.MaxSize,
                                         &ResultLength);

                ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

                ZwClose(ControlHandle);

                PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)InfoBuffer.StartBuffer;

                if (!NT_SUCCESS(Status) ||
                    PartialInfo->Type != REG_DWORD ||
                    PartialInfo->DataLength != sizeof(ULONG) ||
                    *(PULONG)PartialInfo->Data == 0)
                {
                    DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
                    goto Next;
                }
            }

            ASSERT(SymLinkBuffer.MaxSize >= 0x10);

            while (TRUE)
            {
                Status = ZwQueryValueKey(InstanceHandle,
                                         &SymbolicLinkName,
                                         KeyValuePartialInformation,
                                         SymLinkBuffer.StartBuffer,
                                         SymLinkBuffer.MaxSize,
                                         &ResultLength);

                if (Status != STATUS_BUFFER_OVERFLOW)
                    break;

                Status = IopResizeBuffer(&SymLinkBuffer, ResultLength, FALSE);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
                    ZwClose(InstanceHandle);
                    ZwClose(Handle);
                    ZwClose(ClassHandle);
                    goto Exit2;
                }
            }

            ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

            PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)SymLinkBuffer.StartBuffer;

            if (!(NT_SUCCESS(Status) && PartialInfo->Type == REG_SZ && PartialInfo->DataLength > sizeof(WCHAR)))
            {
                DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);
                goto Next;
            }

            Length = (PartialInfo->DataLength - sizeof(WCHAR));

            SymbolicLink.Length = Length;
            SymbolicLink.MaximumLength = SymbolicLink.Length;
            SymbolicLink.Buffer = (PWSTR)PartialInfo->Data;

            if (IsDefaultClass)
            {
                if (!RtlCompareUnicodeString(&DefaultClassName, &SymbolicLink, TRUE))
                {
                    DPRINT("IopGetDeviceInterfaces: '%wZ', '%wZ'\n", &DefaultClassName, &SymbolicLink);
                    goto Next;
                }
            }

            if (InstancePath)
            {
                if (RtlCompareUnicodeString(InstancePath, &DevnodeName, TRUE))
                {
                    DPRINT("IopGetDeviceInterfaces: '%wZ', '%wZ'\n", InstancePath, &DevnodeName);
                    goto Next;
                }
            }

            DPRINT1("IopGetDeviceInterfaces: FIXME IopAppendBuffer()\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();

            ASSERT(((PWSTR)ReturnBuffer.LastBuffer)[-1] == UNICODE_NULL);

            DPRINT("IopGetDeviceInterfaces: Added to return buffer\n");

            if (!Param4)
            {
                Length = (sizeof(L"\\??\\") - sizeof(WCHAR));
                RtlCopyMemory((ReturnBuffer.LastBuffer - (SymbolicLink.Length + sizeof(WCHAR))), L"\\??\\", Length);
            }

Next:
            ZwClose(InstanceHandle);
            jx++;
        }

        ZwClose(Handle);
        ix++;
    }

    ZwClose(ClassHandle);

Exit3:

    Length = (ReturnBuffer.LastBuffer - ReturnBuffer.StartBuffer + sizeof(WCHAR));

    Status = IopResizeBuffer(&ReturnBuffer, Length, TRUE);
    if (NT_SUCCESS(Status))
        *(PWCHAR *)ReturnBuffer.LastBuffer = UNICODE_NULL;

Exit2:

    if (IsDefaultClass)
        ExFreePool(ValueInfo);

Exit1:

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    IopFreeBuffer(&DevnodeNameBuffer);
    IopFreeBuffer(&SymLinkBuffer);
    IopFreeBuffer(&InfoBuffer);

    if (!NT_SUCCESS(Status))
        IopFreeBuffer(&ReturnBuffer);

    RtlFreeUnicodeString(&ClassGuidName);

Exit:

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopGetDeviceInterfaces: Status %X\n", Status);

        *SymbolicLinkList = NULL;

        if (OutSize)
            *OutSize = 0;
    }
    else
    {
        *SymbolicLinkList = (PWSTR)ReturnBuffer.StartBuffer;

        if (OutSize)
            *OutSize = ReturnBuffer.MaxSize;
    }

    return Status;
}

NTSTATUS
NTAPI
IopDisableDeviceInterfaces(
    _In_ PUNICODE_STRING InstancePath)
{
    UNICODE_STRING DeviceClassesKeyName = RTL_CONSTANT_STRING(IO_REG_KEY_DEVICECLASSES);
    PKEY_BASIC_INFORMATION KeyInformation;
    CLASS_INFO_BUFFER ClassInfoBuffer;
    UNICODE_STRING InterfaceString;
    UNICODE_STRING GuidString;
    HANDLE KeyHandle = NULL;
    PWSTR SourceString;
    PWCHAR String;
    GUID Guid;
    ULONG ResultLength;
    ULONG DummySize;
    ULONG Index;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT1("IopDisableDeviceInterfaces: Instance '%wZ'\n", InstancePath);

    Status = IopAllocateBuffer(&ClassInfoBuffer, 0x66);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDisableDeviceInterfaces: Status %X\n", Status);
        return Status;
    }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PpRegistryDeviceResource, TRUE);

    Status = IopOpenRegistryKeyEx(&KeyHandle, NULL, &DeviceClassesKeyName, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDisableDeviceInterfaces: Status %X\n", Status);
        goto Exit;
    }

    ASSERT(ClassInfoBuffer.MaxSize >= sizeof(KEY_BASIC_INFORMATION));

    for (Index = 0; ; Index++)
    {
        Status = ZwEnumerateKey(KeyHandle,
                                Index,
                                KeyBasicInformation,
                                ClassInfoBuffer.StartBuffer,
                                ClassInfoBuffer.MaxSize,
                                &ResultLength);

        if (Status == STATUS_NO_MORE_ENTRIES)
            break;

        ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            IopResizeBuffer(&ClassInfoBuffer, ResultLength, FALSE);
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopDisableDeviceInterfaces: Status %X\n", Status);
            break;
        }

        KeyInformation = (PKEY_BASIC_INFORMATION)ClassInfoBuffer.StartBuffer;

        GuidString.Length = (USHORT)KeyInformation->NameLength;
        GuidString.MaximumLength = GuidString.Length;
        GuidString.Buffer = KeyInformation->Name;

        RtlGUIDFromString(&GuidString, &Guid);

        Status = IopGetDeviceInterfaces(&Guid, InstancePath, 0, FALSE, &SourceString, &DummySize);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopDisableDeviceInterfaces: Status %X\n", Status);
            continue;
        }

        for (String = SourceString;
             *String;
             String += ((InterfaceString.Length + sizeof(WCHAR)) / sizeof(WCHAR)))
        {
            RtlInitUnicodeString(&InterfaceString, String);
            DPRINT1("IopDisableDeviceInterfaces: '%wZ', '%wZ'\n", &InterfaceString, InstancePath);

            IoSetDeviceInterfaceState(&InterfaceString, FALSE);
        }

        ExFreePool(SourceString);
    }

    ZwClose(KeyHandle);

Exit:

    IopFreeBuffer(&ClassInfoBuffer);

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    DPRINT1("IopDisableDeviceInterfaces: return %X\n", Status);
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

static PWCHAR BaseKeyString = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\DeviceClasses\\";

static
NTSTATUS
OpenRegistryHandlesFromSymbolicLink(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ OPTIONAL PHANDLE GuidKey,
    _In_ OPTIONAL PHANDLE DeviceKey,
    _In_ OPTIONAL PHANDLE InstanceKey)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ReferenceString;
    UNICODE_STRING GuidString;
    UNICODE_STRING SubKeyName;
    UNICODE_STRING BaseKeyU;
    PHANDLE InstanceKeyRealP;
    PHANDLE DeviceKeyRealP;
    PHANDLE GuidKeyRealP;
    HANDLE InstanceKeyReal;
    HANDLE DeviceKeyReal;
    HANDLE GuidKeyReal;
    HANDLE ClassesKey;
    PWCHAR StartPosition;
    PWCHAR EndPosition;
    NTSTATUS Status;

    SubKeyName.Buffer = NULL;

    if (GuidKey)
        GuidKeyRealP = GuidKey;
    else
        GuidKeyRealP = &GuidKeyReal;

    if (DeviceKey)
        DeviceKeyRealP = DeviceKey;
    else
        DeviceKeyRealP = &DeviceKeyReal;

    if (InstanceKey)
        InstanceKeyRealP = InstanceKey;
    else
        InstanceKeyRealP = &InstanceKeyReal;

    *GuidKeyRealP = NULL;
    *DeviceKeyRealP = NULL;
    *InstanceKeyRealP = NULL;

    RtlInitUnicodeString(&BaseKeyU, BaseKeyString);

    /* Open the DeviceClasses key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &BaseKeyU,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               NULL,
                               NULL);

    Status = ZwOpenKey(&ClassesKey, (DesiredAccess | KEY_ENUMERATE_SUB_KEYS), (&ObjectAttributes));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenRegistryHandlesFromSymbolicLink: Failed to open '%wZ'\n", &BaseKeyU);
        goto cleanup;
    }

    StartPosition = wcschr(SymbolicLinkName->Buffer, L'{');
    EndPosition = wcschr(SymbolicLinkName->Buffer, L'}');

    if (!StartPosition || !EndPosition || StartPosition > EndPosition)
    {
        DPRINT1("OpenRegistryHandlesFromSymbolicLink: Bad symbolic link '%wZ'\n", SymbolicLinkName);
        return STATUS_INVALID_PARAMETER_1;
    }

    GuidString.Length = (USHORT)((ULONG_PTR)(EndPosition + 1) - (ULONG_PTR)StartPosition);
    GuidString.MaximumLength = GuidString.Length;
    GuidString.Buffer = StartPosition;

    InitializeObjectAttributes(&ObjectAttributes,
                               &GuidString,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               ClassesKey,
                               NULL);

    Status = ZwCreateKey(GuidKeyRealP,
                         (DesiredAccess | KEY_ENUMERATE_SUB_KEYS),
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    ZwClose(ClassesKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenRegistryHandlesFromSymbolicLink: Failed to open %wZ%wZ (%X)\n", &BaseKeyU, &GuidString, Status);
        goto cleanup;
    }

    SubKeyName.Length = 0;
    SubKeyName.MaximumLength = (SymbolicLinkName->Length + sizeof(WCHAR));

    SubKeyName.Buffer = ExAllocatePool(PagedPool, SubKeyName.MaximumLength);
    if (!SubKeyName.Buffer)
    {
        DPRINT1("OpenRegistryHandlesFromSymbolicLink: STATUS_INSUFFICIENT_RESOURCES\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    RtlAppendUnicodeStringToString(&SubKeyName, SymbolicLinkName);

    SubKeyName.Buffer[SubKeyName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    SubKeyName.Buffer[0] = L'#';
    SubKeyName.Buffer[1] = L'#';
    SubKeyName.Buffer[2] = L'?';
    SubKeyName.Buffer[3] = L'#';

    ReferenceString.Buffer = wcsrchr(SubKeyName.Buffer, '\\');
    if (ReferenceString.Buffer)
    {
        ReferenceString.Buffer[0] = L'#';

        SubKeyName.Length = (USHORT)((ULONG_PTR)(ReferenceString.Buffer) - (ULONG_PTR)SubKeyName.Buffer);
        ReferenceString.Length = (SymbolicLinkName->Length - SubKeyName.Length);
    }
    else
    {
        RtlInitUnicodeString(&ReferenceString, L"#");
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &SubKeyName,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               *GuidKeyRealP,
                               NULL);

    Status = ZwCreateKey(DeviceKeyRealP,
                         (DesiredAccess | KEY_ENUMERATE_SUB_KEYS),
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenRegistryHandlesFromSymbolicLink: Failed to open %wZ%wZ\\%wZ Status %X\n",
                &BaseKeyU, &GuidString, &SubKeyName, Status);

        goto cleanup;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &ReferenceString,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               *DeviceKeyRealP,
                               NULL);

    Status = ZwCreateKey(InstanceKeyRealP, DesiredAccess, &ObjectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenRegistryHandlesFromSymbolicLink: Failed to open %wZ%wZ\\%wZ%\\%wZ (%X)\n",
                &BaseKeyU, &GuidString, &SubKeyName, &ReferenceString, Status);

        goto cleanup;
    }

    Status = STATUS_SUCCESS;

cleanup:

    if (SubKeyName.Buffer)
       ExFreePool(SubKeyName.Buffer);

    if (NT_SUCCESS(Status))
    {
       if (!GuidKey)
          ZwClose(*GuidKeyRealP);

       if (!DeviceKey)
          ZwClose(*DeviceKeyRealP);

       if (!InstanceKey)
          ZwClose(*InstanceKeyRealP);
    }
    else
    {
       if (*GuidKeyRealP)
          ZwClose(*GuidKeyRealP);

       if (*DeviceKeyRealP)
          ZwClose(*DeviceKeyRealP);

       if (*InstanceKeyRealP)
          ZwClose(*InstanceKeyRealP);
    }

    return Status;
}

/* Provides a handle to the device's interface instance registry key.
   Documented in WDK.
 
   SymbolicLinkName
      Pointer to a string which identifies the device interface instance
    DesiredAccess
      Desired ACCESS_MASK used to access the key (like KEY_READ, KEY_WRITE, etc)
    DeviceInterfaceKey
      If a call has been succesfull, a handle to the registry key will be stored there
 
   return Three different NTSTATUS values in case of errors, and STATUS_SUCCESS otherwise (see WDK for details)
 
   Must be called at IRQL = PASSIVE_LEVEL in the context of a system thread
*/
NTSTATUS
NTAPI
IoOpenDeviceInterfaceRegistryKey(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DeviceInterfaceKey)
{
   UNICODE_STRING DeviceParametersU = RTL_CONSTANT_STRING(L"Device Parameters");
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE DeviceParametersKey;
   HANDLE InstanceKey;
   NTSTATUS Status;

   Status = OpenRegistryHandlesFromSymbolicLink(SymbolicLinkName, KEY_CREATE_SUB_KEY, NULL, NULL, &InstanceKey);
   if (!NT_SUCCESS(Status))
   {
       DPRINT1("IoOpenDeviceInterfaceRegistryKey: Status %X\n", Status);
       return Status;
   }

   InitializeObjectAttributes(&ObjectAttributes,
                              &DeviceParametersU,
                              (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF),
                              InstanceKey,
                              NULL);

   Status = ZwCreateKey(&DeviceParametersKey, DesiredAccess, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
   ZwClose(InstanceKey);

   if (NT_SUCCESS(Status))
       *DeviceInterfaceKey = DeviceParametersKey;

   return Status;
}

/* unimplemented

   Returns the alias device interface of the specified device interface instance, if the alias exists.
   Documented in WDK.
 
   SymbolicLinkName
      Pointer to a string which identifies the device interface instance
   AliasInterfaceClassGuid
      See WDK
   AliasSymbolicLinkName
      See WDK
 
   return Three different NTSTATUS values in case of errors, and STATUS_SUCCESS otherwise (see WDK for details)
 
   Must be called at IRQL = PASSIVE_LEVEL in the context of a system thread
*/
NTSTATUS
NTAPI
IoGetDeviceInterfaceAlias(IN PUNICODE_STRING SymbolicLinkName,
                          IN CONST GUID* AliasInterfaceClassGuid,
                          OUT PUNICODE_STRING AliasSymbolicLinkName)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);//IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

/* Returns the alias device interface of the specified device interface */
static
NTSTATUS
IopOpenInterfaceKey(
    _In_ CONST GUID* InterfaceClassGuid,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ HANDLE* pInterfaceKey)
{
    UNICODE_STRING LocalMachine = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE InterfaceKey = NULL;
    UNICODE_STRING GuidString;
    UNICODE_STRING KeyName;
    USHORT Size;
    NTSTATUS Status;

    GuidString.Buffer = KeyName.Buffer = NULL;

    Status = RtlStringFromGUID(InterfaceClassGuid, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenInterfaceKey: RtlStringFromGUID() failed %X\n", Status);
        goto cleanup;
    }

    Size = (((USHORT)wcslen(REGSTR_PATH_DEVICE_CLASSES) + 1) * sizeof(WCHAR));
    KeyName.MaximumLength = LocalMachine.Length + Size + GuidString.Length;
    KeyName.Length = 0;

    KeyName.Buffer = ExAllocatePool(PagedPool, KeyName.MaximumLength);
    if (!KeyName.Buffer)
    {
        DPRINT1("IopOpenInterfaceKey: STATUS_INSUFFICIENT_RESOURCES\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    Status = RtlAppendUnicodeStringToString(&KeyName, &LocalMachine);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenInterfaceKey: RtlAppendUnicodeStringToString() failed %X\n", Status);
        goto cleanup;
    }

    Status = RtlAppendUnicodeToString(&KeyName, REGSTR_PATH_DEVICE_CLASSES);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenInterfaceKey: RtlAppendUnicodeToString() failed %X\n", Status);
        goto cleanup;
    }

    Status = RtlAppendUnicodeToString(&KeyName, L"\\");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenInterfaceKey: RtlAppendUnicodeToString() failed %X\n", Status);
        goto cleanup;
    }

    Status = RtlAppendUnicodeStringToString(&KeyName, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenInterfaceKey: RtlAppendUnicodeStringToString() failed %X\n", Status);
        goto cleanup;
    }

    DPRINT("IopOpenInterfaceKey: KeyName '%wZ'\n", &KeyName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               NULL,
                               NULL);

    Status = ZwOpenKey(&InterfaceKey, DesiredAccess, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenInterfaceKey: ZwOpenKey() failed %X\n", Status);
        goto cleanup;
    }

    *pInterfaceKey = InterfaceKey;
    Status = STATUS_SUCCESS;

cleanup:

    if (!NT_SUCCESS(Status) && (InterfaceKey))
        ZwClose(InterfaceKey);

    RtlFreeUnicodeString(&GuidString);
    RtlFreeUnicodeString(&KeyName);

    return Status;
}

/* Returns a list of device interfaces of a particular device interface class.
   Documented in WDK

   InterfaceClassGuid
      Points to a class GUID specifying the device interface class
   PhysicalDeviceObject
      Points to an optional PDO that narrows the search to only the device interfaces of the device represented by the PDO
   Flags
      Specifies flags that modify the search for device interfaces.
      The DEVICE_INTERFACE_INCLUDE_NONACTIVE flag specifies that the list of returned symbolic links
      should contain also disabled device interfaces in addition to the enabled ones.
   SymbolicLinkList
      Points to a character pointer that is filled in on successful return with a list
      of unicode strings identifying the device interfaces that match the search criteria.
      The newly allocated buffer contains a list of symbolic link names.
      Each unicode string in the list is null-terminated; the end of the whole list is marked by an additional NULL.
      The caller is responsible for freeing the buffer (ExFreePool) when it is no longer needed.
      If no device interfaces match the search criteria, this routine returns STATUS_SUCCESS and the string contains a single NULL character.
*/
NTSTATUS
NTAPI
IoGetDeviceInterfaces(
    _In_ CONST GUID* InterfaceClassGuid,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
    _In_ ULONG Flags,
    _Out_ PWSTR* SymbolicLinkList)
{
    UNICODE_STRING DeviceInstanceName = RTL_CONSTANT_STRING(L"DeviceInstance");
    UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(L"SymbolicLink");
    UNICODE_STRING LinkedName = RTL_CONSTANT_STRING(L"Linked");
    UNICODE_STRING Control = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING ReturnBuffer = { 0, 0, NULL };
    UNICODE_STRING KeyName;
    PUNICODE_STRING InstanceDevicePath = NULL;
    PEXTENDED_DEVOBJ_EXTENSION DeviceObjectExtension;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    PKEY_VALUE_PARTIAL_INFORMATION bip = NULL;
    PKEY_BASIC_INFORMATION ReferenceBi = NULL;
    PKEY_BASIC_INFORMATION DeviceBi = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE InterfaceKey = NULL;
    HANDLE ReferenceKey = NULL;
    HANDLE ControlKey = NULL;
    HANDLE DeviceKey = NULL;
    ULONG NeededLength;
    ULONG ActualLength;
    ULONG LinkedValue;
    ULONG Size;
    ULONG ix;
    ULONG jx;
    USHORT Length;
    BOOLEAN FoundRightPDO = FALSE;
    NTSTATUS Status;

    PAGED_CODE();

    if (PhysicalDeviceObject)
    {
        /* Parameters must pass three border of checks */
        DeviceObjectExtension = (PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension;

        /* 1st level: Presence of a Device Node */
        if (!DeviceObjectExtension->DeviceNode)
        {
            DPRINT1("IoGetDeviceInterfaces: PDO %p doesn't have a DeviceNode\n", PhysicalDeviceObject);
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        /* 2nd level: Presence of an non-zero length InstancePath */
        if (!DeviceObjectExtension->DeviceNode->InstancePath.Length)
        {
            DPRINT1("IoGetDeviceInterfaces: PDO's %p DOE has zero-length InstancePath\n", PhysicalDeviceObject);
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        InstanceDevicePath = &DeviceObjectExtension->DeviceNode->InstancePath;
    }


    Status = IopOpenInterfaceKey(InterfaceClassGuid, KEY_ENUMERATE_SUB_KEYS, &InterfaceKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoGetDeviceInterfaces: IopOpenInterfaceKey() failed %X\n", Status);
        goto cleanup;
    }

    /* Enumerate subkeys (ix.e. the different device objects) */
    for (ix = 0; ; ix++)
    {
        Status = ZwEnumerateKey(InterfaceKey, ix, KeyBasicInformation, NULL, 0, &Size);
        if (Status == STATUS_NO_MORE_ENTRIES)
            break;

        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
        {
            DPRINT1("IoGetDeviceInterfaces: ZwEnumerateKey() failed %X\n", Status);
            goto cleanup;
        }

        DeviceBi = ExAllocatePool(PagedPool, Size);
        if (!DeviceBi)
        {
            DPRINT1("IoGetDeviceInterfaces: ExAllocatePool() failed\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        Status = ZwEnumerateKey(InterfaceKey, ix, KeyBasicInformation, DeviceBi, Size, &Size);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IoGetDeviceInterfaces: ZwEnumerateKey() failed %X\n", Status);
            goto cleanup;
        }

        /* Open device key */
        KeyName.Length = KeyName.MaximumLength = (USHORT)DeviceBi->NameLength;
        KeyName.Buffer = DeviceBi->Name;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   InterfaceKey,
                                   NULL);

        Status = ZwOpenKey(&DeviceKey, KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoGetDeviceInterfaces: ZwOpenKey() failed %X\n", Status);
            goto cleanup;
        }

        if (PhysicalDeviceObject)
        {
            /* Check if we are on the right physical device object, by reading the DeviceInstance string */
            Status = ZwQueryValueKey(DeviceKey, &DeviceInstanceName, KeyValuePartialInformation, NULL, 0, &NeededLength);
            if (Status != STATUS_BUFFER_TOO_SMALL)
                /* error */
                break;

            ActualLength = NeededLength;
            PartialInfo = ExAllocatePool(NonPagedPool, ActualLength);
            if (!PartialInfo)
            {
                DPRINT1("IoGetDeviceInterfaces: STATUS_INSUFFICIENT_RESOURCES\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            Status = ZwQueryValueKey(DeviceKey, &KeyName, KeyValuePartialInformation, PartialInfo, ActualLength, &NeededLength);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IoGetDeviceInterfaces: ZwQueryValueKey #2 failed (%X)\n", Status);
                ExFreePool(PartialInfo);
                goto cleanup;
            }

            Length = InstanceDevicePath->Length;

            if (PartialInfo->DataLength == Length)
            {
                if (RtlCompareMemory(PartialInfo->Data, InstanceDevicePath->Buffer, Length) == Length)
                    /* found right pdo */
                    FoundRightPDO = TRUE;
            }

            ExFreePool(PartialInfo);
            PartialInfo = NULL;

            if (!FoundRightPDO)
                /* not yet found */
                continue;
        }

        /* Enumerate subkeys (ie the different reference strings) */
        for (jx = 0; ; jx++)
        {
            Status = ZwEnumerateKey(DeviceKey, jx, KeyBasicInformation, NULL, 0, &Size);
            if (Status == STATUS_NO_MORE_ENTRIES)
                break;

            if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
            {
                DPRINT1("IoGetDeviceInterfaces: ZwEnumerateKey() failed %X\n", Status);
                goto cleanup;
            }

            ReferenceBi = ExAllocatePool(PagedPool, Size);
            if (!ReferenceBi)
            {
                DPRINT1("IoGetDeviceInterfaces: STATUS_INSUFFICIENT_RESOURCES\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            Status = ZwEnumerateKey(DeviceKey, jx, KeyBasicInformation, ReferenceBi, Size, &Size);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IoGetDeviceInterfaces: ZwEnumerateKey() failed %X\n", Status);
                goto cleanup;
            }

            KeyName.Length = KeyName.MaximumLength = (USHORT)ReferenceBi->NameLength;
            KeyName.Buffer = ReferenceBi->Name;

            if (RtlEqualUnicodeString(&KeyName, &Control, TRUE))
                /* Skip Control subkey */
                goto NextReferenceString;

            /* Open reference key */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                       DeviceKey,
                                       NULL);

            Status = ZwOpenKey(&ReferenceKey, KEY_QUERY_VALUE, &ObjectAttributes);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IoGetDeviceInterfaces: ZwOpenKey() failed %X\n", Status);
                goto cleanup;
            }

            if (!(Flags & DEVICE_INTERFACE_INCLUDE_NONACTIVE))
            {
                /* We have to check if the interface is enabled, by reading the Linked value in the Control subkey */
                InitializeObjectAttributes(&ObjectAttributes,
                                           &Control,
                                           (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                           ReferenceKey,
                                           NULL);

                Status = ZwOpenKey(&ControlKey, KEY_QUERY_VALUE, &ObjectAttributes);
                if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                    /* That's OK. The key doesn't exist (yet) because the interface is not activated. */
                    goto NextReferenceString;

                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("IoGetDeviceInterfaces: ZwOpenKey() failed %X\n", Status);
                    goto cleanup;
                }

                Status = ZwQueryValueKey(ControlKey, &LinkedName, KeyValuePartialInformation, NULL, 0, &NeededLength);
                if (Status != STATUS_BUFFER_TOO_SMALL)
                {
                    DPRINT1("IoGetDeviceInterfaces: ZwQueryValueKey #1 failed (%X)\n", Status);
                    goto cleanup;
                }

                ActualLength = NeededLength;

                PartialInfo = ExAllocatePool(NonPagedPool, ActualLength);
                if (!PartialInfo)
                {
                    DPRINT1("IoGetDeviceInterfaces: STATUS_INSUFFICIENT_RESOURCES\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto cleanup;
                }

                Status = ZwQueryValueKey(ControlKey,
                                         &LinkedName,
                                         KeyValuePartialInformation,
                                         PartialInfo,
                                         ActualLength,
                                         &NeededLength);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("IoGetDeviceInterfaces: ZwQueryValueKey #2 failed (%X)\n", Status);
                    ExFreePool(PartialInfo);
                    goto cleanup;
                }

                if (PartialInfo->Type != REG_DWORD || PartialInfo->DataLength != sizeof(ULONG))
                {
                    DPRINT1("IoGetDeviceInterfaces: Bad registry read\n");
                    ExFreePool(PartialInfo);
                    goto cleanup;
                }

                RtlCopyMemory(&LinkedValue, PartialInfo->Data, PartialInfo->DataLength);
                ExFreePool(PartialInfo);

                if (!LinkedValue)
                    /* This interface isn't active */
                    goto NextReferenceString;
            }

            /* Read the SymbolicLink string and add it into SymbolicLinkList */
            Status = ZwQueryValueKey(ReferenceKey, &SymbolicLink, KeyValuePartialInformation, NULL, 0, &Size);
            if (!NT_SUCCESS(Status))
            {
                if (Status != STATUS_BUFFER_TOO_SMALL)
                {
                    DPRINT("IoGetDeviceInterfaces: ZwQueryValueKey() failed %X\n", Status);
                    goto cleanup;
                }
            }

            bip = ExAllocatePool(PagedPool, Size);
            if (!bip)
            {
                DPRINT1("IoGetDeviceInterfaces: STATUS_INSUFFICIENT_RESOURCES\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            Status = ZwQueryValueKey(ReferenceKey, &SymbolicLink, KeyValuePartialInformation, bip, Size, &Size);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IoGetDeviceInterfaces: ZwQueryValueKey() failed %X\n", Status);
                goto cleanup;
            }

            if (bip->Type != REG_SZ)
            {
                DPRINT("IoGetDeviceInterfaces: registry type %X (expected %X)\n", bip->Type, REG_SZ);
                Status = STATUS_UNSUCCESSFUL;
                goto cleanup;
            }

            if (bip->DataLength < (5 * sizeof(WCHAR)))
            {
                DPRINT("IoGetDeviceInterfaces: Registry string short (%X), expected >= %X)\n",
                       bip->DataLength, (5 * sizeof(WCHAR)));

                Status = STATUS_UNSUCCESSFUL;
                goto cleanup;
            }

            KeyName.Length = KeyName.MaximumLength = (USHORT)bip->DataLength;
            KeyName.Buffer = (PWSTR)bip->Data;

            /* Fixup the prefix (from "\\?\") */
            RtlCopyMemory(KeyName.Buffer, L"\\??\\", 4 * sizeof(WCHAR));

            /* Add new symbolic link to symbolic link list */
            Length = (ReturnBuffer.Length + KeyName.Length + sizeof(WCHAR));
            if (Length > ReturnBuffer.MaximumLength)
            {
                PWSTR NewBuffer;

                Length += sizeof(WCHAR);
                ReturnBuffer.MaximumLength = (USHORT)max((2 * ReturnBuffer.MaximumLength), Length);

                NewBuffer = ExAllocatePool(PagedPool, ReturnBuffer.MaximumLength);
                if (!NewBuffer)
                {
                    DPRINT1("IoGetDeviceInterfaces: STATUS_INSUFFICIENT_RESOURCES\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto cleanup;
                }

                if (ReturnBuffer.Buffer)
                {
                    RtlCopyMemory(NewBuffer, ReturnBuffer.Buffer, ReturnBuffer.Length);
                    ExFreePool(ReturnBuffer.Buffer);
                }

                ReturnBuffer.Buffer = NewBuffer;
            }

            DPRINT("IoGetDeviceInterfaces: Adding symbolic link '%wZ'\n", &KeyName);

            Status = RtlAppendUnicodeStringToString(&ReturnBuffer, &KeyName);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IoGetDeviceInterfaces: RtlAppendUnicodeStringToString() failed %X\n", Status);
                goto cleanup;
            }

            /* RtlAppendUnicodeStringToString added a NULL at the end of the destination string,
               but didn't increase the Length field. Do it for it.
            */
            ReturnBuffer.Length += sizeof(WCHAR);

NextReferenceString:

            ExFreePool(ReferenceBi);
            ReferenceBi = NULL;

            if (bip)
                ExFreePool(bip);
            bip = NULL;

            if (ReferenceKey)
            {
                ZwClose(ReferenceKey);
                ReferenceKey = NULL;
            }

            if (ControlKey)
            {
                ZwClose(ControlKey);
                ControlKey = NULL;
            }
        }

        if (FoundRightPDO)
            /* No need to go further, as we already have found what we searched */
            break;

        ExFreePool(DeviceBi);
        DeviceBi = NULL;

        ZwClose(DeviceKey);
        DeviceKey = NULL;
    }

    /* Add final NULL to ReturnBuffer */
    ASSERT(ReturnBuffer.Length <= ReturnBuffer.MaximumLength);

    if (ReturnBuffer.Length >= ReturnBuffer.MaximumLength)
    {
        PWSTR NewBuffer;
        ReturnBuffer.MaximumLength += sizeof(WCHAR);

        NewBuffer = ExAllocatePool(PagedPool, ReturnBuffer.MaximumLength);
        if (!NewBuffer)
        {
            DPRINT1("IoGetDeviceInterfaces: STATUS_INSUFFICIENT_RESOURCES\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        if (ReturnBuffer.Buffer)
        {
            RtlCopyMemory(NewBuffer, ReturnBuffer.Buffer, ReturnBuffer.Length);
            ExFreePool(ReturnBuffer.Buffer);
        }

        ReturnBuffer.Buffer = NewBuffer;
    }

    ReturnBuffer.Buffer[ReturnBuffer.Length / sizeof(WCHAR)] = UNICODE_NULL;

    *SymbolicLinkList = ReturnBuffer.Buffer;
    Status = STATUS_SUCCESS;

cleanup:

    if (!NT_SUCCESS(Status) && ReturnBuffer.Buffer)
        ExFreePool(ReturnBuffer.Buffer);

    if (InterfaceKey)
        ZwClose(InterfaceKey);

    if (DeviceKey)
        ZwClose(DeviceKey);

    if (ReferenceKey)
        ZwClose(ReferenceKey);

    if (ControlKey)
        ZwClose(ControlKey);

    if (DeviceBi)
        ExFreePool(DeviceBi);

    if (ReferenceBi)
        ExFreePool(ReferenceBi);

    if (bip)
        ExFreePool(bip);

    return Status;
}

/* Registers a device interface class, if it has not been previously registered, and creates a new instance of the interface class,
   which a driver can subsequently enable for use by applications or other system components.
   Documented in WDK.

   PhysicalDeviceObject
      Points to an optional PDO that narrows the search to only the device interfaces of the device represented by the PDO
   InterfaceClassGuid
      Points to a class GUID specifying the device interface class
   ReferenceString
      Optional parameter, pointing to a unicode string.
      For a full description of this rather rarely used param (usually drivers pass NULL here) see WDK
   SymbolicLinkName
      Pointer to the resulting unicode string

   Must be called at IRQL = PASSIVE_LEVEL in the context of a system thread
*/
NTSTATUS
NTAPI
IoRegisterDeviceInterface(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ CONST GUID* InterfaceClassGuid,
    _In_ PUNICODE_STRING ReferenceString OPTIONAL,
    _Out_ PUNICODE_STRING SymbolicLinkName)
{
    UCHAR PdoNameInfoBuffer[sizeof(OBJECT_NAME_INFORMATION) + (256 * sizeof(WCHAR))];
    POBJECT_NAME_INFORMATION PdoNameInfo = (POBJECT_NAME_INFORMATION)PdoNameInfoBuffer;
    UNICODE_STRING DeviceInstance = RTL_CONSTANT_STRING(L"DeviceInstance");
    UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(L"SymbolicLink");
    UNICODE_STRING InterfaceKeyName;
    UNICODE_STRING BaseKeyName;
    UNICODE_STRING GuidString;
    UNICODE_STRING SubKeyName;
    PUNICODE_STRING InstancePath;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PDEVICE_NODE DeviceNode;
    HANDLE InterfaceKey;
    HANDLE ClassKey;
    HANDLE SubKey;
    ULONG StartIndex;
    ULONG ix;
    NTSTATUS SymLinkStatus;
    NTSTATUS Status;

    PAGED_CODE();

    Status = RtlStringFromGUID(InterfaceClassGuid, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterDeviceInterface: RtlStringFromGUID() failed %X\n", Status);
        return Status;
    }

    //DPRINT1("IoRegisterDeviceInterface: [%p], Guid '%wZ'\n", PhysicalDeviceObject,  &GuidString);

    if (ReferenceString && ReferenceString->Buffer)
    {
        DPRINT1("IoRegisterDeviceInterface(): RefString: '%wZ'\n", ReferenceString);
    }

    DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);

    /* 1st level: Presence of a Device Node */
    if (!DeviceNode)
    {
        DPRINT("IoRegisterDeviceInterface: PDO %p doesn't have a DeviceNode\n", PhysicalDeviceObject);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)
    {
        DPRINT1("IoRegisterDeviceInterface: DeviceNode %p have a DNF_LEGACY_RESOURCE_DEVICENODE\n", DeviceNode);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* 2nd level: Presence of an non-zero length InstancePath */
    if (!DeviceNode->InstancePath.Length)
    {
        DPRINT1("IoRegisterDeviceInterface: PDO's %p DOE has zero-length InstancePath\n", PhysicalDeviceObject);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* 3rd level: Optional, based on WDK documentation */
    if (ReferenceString)
    {
        /* Reference string must not contain path-separator symbols */
        for (ix = 0; ix < ReferenceString->Length / sizeof(WCHAR); ix++)
        {
            if ((ReferenceString->Buffer[ix] == '\\') ||
                (ReferenceString->Buffer[ix] == '/'))
            {
                DPRINT("Reference string must not contain path-separator symbols\n");
                return STATUS_INVALID_DEVICE_REQUEST;
            }
        }
    }

    Status = RtlStringFromGUID(InterfaceClassGuid, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterDeviceInterface: RtlStringFromGUID() failed %X\n", Status);
        return Status;
    }

    /* Create Pdo name: \Device\xxxxxxxx (unnamed device) */
    Status = ObQueryNameString(PhysicalDeviceObject, PdoNameInfo, sizeof(PdoNameInfoBuffer), &ix);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterDeviceInterface: ObQueryNameString() failed %X\n", Status);
        return Status;
    }

    ASSERT(PdoNameInfo->Name.Length);

    /* Create base key name for this interface: HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{GUID} */
    ASSERT(((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode);
    InstancePath = &((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode->InstancePath;

    BaseKeyName.Length = ((USHORT)wcslen(BaseKeyString) * sizeof(WCHAR));
    BaseKeyName.MaximumLength = (BaseKeyName.Length + GuidString.Length);

    BaseKeyName.Buffer = ExAllocatePool(PagedPool, BaseKeyName.MaximumLength);
    if (!BaseKeyName.Buffer)
    {
        DPRINT1("IoRegisterDeviceInterface: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    wcscpy(BaseKeyName.Buffer, BaseKeyString);
    RtlAppendUnicodeStringToString(&BaseKeyName, &GuidString);

    /* Create BaseKeyName key in registry */
    InitializeObjectAttributes(&ObjectAttributes,
                               &BaseKeyName,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF),
                               NULL,
                               NULL);

    Status = ZwCreateKey(&ClassKey, KEY_WRITE, &ObjectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterDeviceInterface: ZwCreateKey() failed %X\n", Status);
        ExFreePool(BaseKeyName.Buffer);
        return Status;
    }

    /* Create key name for this interface: ##?#ACPI#PNP0501#1#{GUID} */
    InterfaceKeyName.Length = 0;
    InterfaceKeyName.MaximumLength = (4 * sizeof(WCHAR))  + /* 4  = size of ##?# */
                                     InstancePath->Length +
                                     sizeof(WCHAR)        + /* 1  = size of # */
                                     GuidString.Length;

    InterfaceKeyName.Buffer = ExAllocatePool(PagedPool, InterfaceKeyName.MaximumLength);
    if (!InterfaceKeyName.Buffer)
    {
        DPRINT1("IoRegisterDeviceInterface: ExAllocatePool() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeToString(&InterfaceKeyName, L"##?#");
    StartIndex = InterfaceKeyName.Length / sizeof(WCHAR);
    RtlAppendUnicodeStringToString(&InterfaceKeyName, InstancePath);

    for (ix = 0; ix < InstancePath->Length / sizeof(WCHAR); ix++)
    {
        if (InterfaceKeyName.Buffer[StartIndex + ix] == '\\')
            InterfaceKeyName.Buffer[StartIndex + ix] = '#';
    }

    RtlAppendUnicodeToString(&InterfaceKeyName, L"#");
    RtlAppendUnicodeStringToString(&InterfaceKeyName, &GuidString);

    /* Create the interface key in registry */
    InitializeObjectAttributes(&ObjectAttributes,
                               &InterfaceKeyName,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF),
                               ClassKey,
                               NULL);

    Status = ZwCreateKey(&InterfaceKey, KEY_WRITE, &ObjectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterDeviceInterface: ZwCreateKey() failed %X\n", Status);
        ZwClose(ClassKey);
        ExFreePool(BaseKeyName.Buffer);
        return Status;
    }

    /* Write DeviceInstance entry. Value is InstancePath */
    Status = ZwSetValueKey(InterfaceKey, &DeviceInstance, 0, REG_SZ, InstancePath->Buffer, InstancePath->Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoRegisterDeviceInterface: ZwSetValueKey() failed %X\n", Status);

        ZwClose(InterfaceKey);
        ZwClose(ClassKey);

        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);

        return Status;
    }

    /* Create subkey. Name is #ReferenceString */
    SubKeyName.Length = 0;
    SubKeyName.MaximumLength = sizeof(WCHAR);

    if (ReferenceString && ReferenceString->Length)
        SubKeyName.MaximumLength += ReferenceString->Length;

    SubKeyName.Buffer = ExAllocatePool(PagedPool, SubKeyName.MaximumLength);
    if (!SubKeyName.Buffer)
    {
        DPRINT1("IoRegisterDeviceInterface: STATUS_INSUFFICIENT_RESOURCES\n");

        ZwClose(InterfaceKey);
        ZwClose(ClassKey);

        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeToString(&SubKeyName, L"#");

    if (ReferenceString && ReferenceString->Length)
        RtlAppendUnicodeStringToString(&SubKeyName, ReferenceString);

    /* Create SubKeyName key in registry */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SubKeyName,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               InterfaceKey,
                               NULL);

    Status = ZwCreateKey(&SubKey, KEY_WRITE, &ObjectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterDeviceInterface: ZwCreateKey() failed %X\n", Status);

        ZwClose(InterfaceKey);
        ZwClose(ClassKey);

        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);

        return Status;
    }

    /* Create symbolic link name: \??\ACPI#PNP0501#1#{GUID}\ReferenceString */
    SymbolicLinkName->Length = 0;
    SymbolicLinkName->MaximumLength = (4 * sizeof(WCHAR))  + /* size of \??\ */
                                      InstancePath->Length +
                                      sizeof(WCHAR)        + /* size of # */
                                      GuidString.Length    +
                                      sizeof(WCHAR);         /* final NULL */

    if (ReferenceString && ReferenceString->Length)
        SymbolicLinkName->MaximumLength += (sizeof(WCHAR) + ReferenceString->Length);

    SymbolicLinkName->Buffer = ExAllocatePool(PagedPool, SymbolicLinkName->MaximumLength);
    if (!SymbolicLinkName->Buffer)
    {
        DPRINT1("IoRegisterDeviceInterface: STATUS_INSUFFICIENT_RESOURCES\n");

        ZwClose(SubKey);
        ZwClose(InterfaceKey);
        ZwClose(ClassKey);

        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(SubKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeToString(SymbolicLinkName, L"\\??\\");
    StartIndex = SymbolicLinkName->Length / sizeof(WCHAR);
    RtlAppendUnicodeStringToString(SymbolicLinkName, InstancePath);

    for (ix = 0; ix < InstancePath->Length / sizeof(WCHAR); ix++)
    {
        if (SymbolicLinkName->Buffer[StartIndex + ix] == '\\')
            SymbolicLinkName->Buffer[StartIndex + ix] = '#';
    }

    RtlAppendUnicodeToString(SymbolicLinkName, L"#");
    RtlAppendUnicodeStringToString(SymbolicLinkName, &GuidString);
    SymbolicLinkName->Buffer[SymbolicLinkName->Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Create symbolic link */
    DPRINT("IoRegisterDeviceInterface(): creating symbolic link %wZ -> %wZ\n", SymbolicLinkName, &PdoNameInfo->Name);
    SymLinkStatus = IoCreateSymbolicLink(SymbolicLinkName, &PdoNameInfo->Name);

    /* If the symbolic link already exists, return an informational success status */
    if (SymLinkStatus == STATUS_OBJECT_NAME_COLLISION)
    {
        /* HACK: Delete the existing symbolic link and update it to the new PDO name */
        IoDeleteSymbolicLink(SymbolicLinkName);
        IoCreateSymbolicLink(SymbolicLinkName, &PdoNameInfo->Name);

        SymLinkStatus = STATUS_OBJECT_NAME_EXISTS;
    }

    if (!NT_SUCCESS(SymLinkStatus))
    {
        DPRINT1("IoRegisterDeviceInterface: IoCreateSymbolicLink() failed %X\n", SymLinkStatus);

        ZwClose(SubKey);
        ZwClose(InterfaceKey);
        ZwClose(ClassKey);

        ExFreePool(SubKeyName.Buffer);
        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);
        ExFreePool(SymbolicLinkName->Buffer);

        return SymLinkStatus;
    }

    if (ReferenceString && ReferenceString->Length)
    {
        RtlAppendUnicodeToString(SymbolicLinkName, L"\\");
        RtlAppendUnicodeStringToString(SymbolicLinkName, ReferenceString);
    }

    SymbolicLinkName->Buffer[SymbolicLinkName->Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Write symbolic link name in registry */
    SymbolicLinkName->Buffer[1] = '\\';

    Status = ZwSetValueKey(SubKey, &SymbolicLink, 0, REG_SZ, SymbolicLinkName->Buffer, SymbolicLinkName->Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterDeviceInterface: ZwSetValueKey() failed %X\n", Status);
        ExFreePool(SymbolicLinkName->Buffer);
    }
    else
    {
        SymbolicLinkName->Buffer[1] = '?';
    }

    ZwClose(SubKey);
    ZwClose(InterfaceKey);
    ZwClose(ClassKey);

    ExFreePool(SubKeyName.Buffer);
    ExFreePool(InterfaceKeyName.Buffer);
    ExFreePool(BaseKeyName.Buffer);

    return (NT_SUCCESS(Status) ? SymLinkStatus : Status);
}

/* Enables or disables an instance of a previously registered device interface class.
   Documented in WDK.

   SymbolicLinkName
      Pointer to the string identifying instance to enable or disable
   Enable
      TRUE = enable, FALSE = disable

   remarks: Must be called at IRQL = PASSIVE_LEVEL in the context of a system thread
*/
NTSTATUS
NTAPI
IoSetDeviceInterfaceState(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ BOOLEAN Enable)
{
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IoSetDeviceInterfaceState: SymbolicLink '%wZ', IsEnable %X\n", SymbolicLinkName, Enable);

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PpRegistryDeviceResource, TRUE);

    Status = IopProcessSetInterfaceState(SymbolicLinkName, Enable, TRUE);

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    if (!NT_SUCCESS(Status) && !Enable)
        Status = STATUS_SUCCESS;

    return Status;
}

/* EOF */
