/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC port sections and shared memory views
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

extern POBJECT_TYPE MmSectionObjectType;

/* VIEW ATTRIBUTE TRANSFER **************************************************/

/**
 * @brief Finds a mapped view by its base address and returns a reference to the
 *        backing section object (for transferring a VIEW message attribute).
 *
 * @return Referenced section object, or NULL if no such view.
 */
PVOID
NTAPI
AlpcpReferenceViewSection(
    _In_ PALPC_PORT Port,
    _In_ PVOID ViewBase,
    _Out_ PULONG OutSize)
{
    PVOID SectionObject = NULL;
    PLIST_ENTRY Entry;

    *OutSize = 0;
    KeAcquireGuardedMutex(&AlpcpLock);
    for (Entry = Port->ResourceListHead.Flink;
         Entry != &Port->ResourceListHead;
         Entry = Entry->Flink)
    {
        PKALPC_VIEW View = CONTAINING_RECORD(Entry, KALPC_VIEW, ViewListEntry);
        if (View->Address == ViewBase && View->SecureViewHandle != NULL)
        {
            SectionObject = View->SecureViewHandle;
            ObReferenceObject(SectionObject);
            *OutSize = View->Size;
            break;
        }
    }
    KeReleaseGuardedMutex(&AlpcpLock);

    return SectionObject;
}

/**
 * @brief Maps a transferred section into the receiver and records the view.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
AlpcpMapReceivedView(
    _In_ PALPC_PORT Port,
    _In_ PVOID SectionObject,
    _In_ ULONG Size,
    _Out_ PVOID *OutViewBase,
    _Out_ PSIZE_T OutViewSize)
{
    PKALPC_VIEW View;
    PVOID ViewBase = NULL;
    SIZE_T ViewSize = Size;
    LARGE_INTEGER SectionOffset;
    NTSTATUS Status;

    SectionOffset.QuadPart = 0;
    Status = MmMapViewOfSection(SectionObject, PsGetCurrentProcess(), &ViewBase, 0, 0,
                                &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
        return Status;

    View = ExAllocatePoolWithTag(NonPagedPool, sizeof(KALPC_VIEW), TAG_ALPC_HANDLE);
    if (View == NULL)
    {
        MmUnmapViewOfSection(PsGetCurrentProcess(), ViewBase);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(View, sizeof(KALPC_VIEW));
    View->OwnerPort = Port;
    View->OwnerProcess = PsGetCurrentProcess();
    View->Address = ViewBase;
    View->Size = (ULONG)ViewSize;
    ObReferenceObject(SectionObject);
    View->SecureViewHandle = SectionObject;

    KeAcquireGuardedMutex(&AlpcpLock);
    InsertTailList(&Port->ResourceListHead, &View->ViewListEntry);
    KeReleaseGuardedMutex(&AlpcpLock);

    *OutViewBase = ViewBase;
    *OutViewSize = ViewSize;
    return STATUS_SUCCESS;
}

/* FUNCTIONS ******************************************************************/

/**
 * @brief Creates a section that can be mapped as a view into a port.
 *
 * When no section handle is supplied a pagefile-backed section is created. The
 * size is rounded up to a page boundary and an ALPC handle for the section is
 * returned along with the actual (rounded) size.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcCreatePortSection(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_opt_ HANDLE SectionHandle,
    _In_ SIZE_T SectionSize,
    _Out_ PALPC_HANDLE AlpcSectionHandle,
    _Out_ PSIZE_T ActualSectionSize)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_SECTION Section;
    PVOID SectionObject = NULL;
    SIZE_T RoundedSize;
    ALPC_HANDLE Handle;
    NTSTATUS Status;

    /*
     * Section size is rounded up to the allocation granularity (64 KB), not the
     * page size: the section is mapped via the view APIs, whose ViewBase must be
     * granularity-aligned, so the kernel reports the granularity-rounded size as
     * the actual size. Verified against the Win11 oracle (req=1 -> 0x10000).
     */
    RoundedSize = ROUND_UP(SectionSize, MM_ALLOCATION_GRANULARITY);
    if (RoundedSize == 0)
        return STATUS_INVALID_PARAMETER;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    if (SectionHandle != NULL)
    {
        Status = ObReferenceObjectByHandle(SectionHandle, SECTION_MAP_READ | SECTION_MAP_WRITE,
                                           MmSectionObjectType, PreviousMode, &SectionObject, NULL);
    }
    else
    {
        HANDLE LocalSection;
        LARGE_INTEGER MaximumSize;
        OBJECT_ATTRIBUTES SectionAttributes;

        MaximumSize.QuadPart = RoundedSize;
        InitializeObjectAttributes(&SectionAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
        Status = ZwCreateSection(&LocalSection,
                                 SECTION_MAP_READ | SECTION_MAP_WRITE,
                                 &SectionAttributes,
                                 &MaximumSize,
                                 PAGE_READWRITE,
                                 SEC_COMMIT,
                                 NULL);
        if (NT_SUCCESS(Status))
        {
            Status = ObReferenceObjectByHandle(LocalSection, SECTION_MAP_READ | SECTION_MAP_WRITE,
                                               MmSectionObjectType, KernelMode, &SectionObject, NULL);
            ZwClose(LocalSection);
        }
    }
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    Section = ExAllocatePoolWithTag(NonPagedPool, sizeof(KALPC_SECTION), TAG_ALPC_HANDLE);
    if (Section == NULL)
    {
        ObDereferenceObject(SectionObject);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(Section, sizeof(KALPC_SECTION));
    Section->SectionObject = SectionObject;
    Section->Size = (ULONG)RoundedSize;
    Section->OwnerPort = Port;
    InitializeListHead(&Section->RegionListHead);

    Handle = AlpcpInsertHandle(&Port->CommunicationInfo->HandleTable, Section);
    if (Handle == NULL)
    {
        ObDereferenceObject(SectionObject);
        ExFreePoolWithTag(Section, TAG_ALPC_HANDLE);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
        {
            ProbeForWrite(AlpcSectionHandle, sizeof(ALPC_HANDLE), sizeof(ALPC_HANDLE));
            ProbeForWrite(ActualSectionSize, sizeof(SIZE_T), sizeof(SIZE_T));
        }
        *AlpcSectionHandle = Handle;
        *ActualSectionSize = RoundedSize;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

Cleanup:
    ObDereferenceObject(Port);
    return Status;
}

/**
 * @brief Deletes a previously created port section.
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcDeletePortSection(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_ ALPC_HANDLE SectionHandle)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_SECTION Section;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Section = AlpcpRemoveHandle(&Port->CommunicationInfo->HandleTable, SectionHandle);
    if (Section == NULL)
    {
        ObDereferenceObject(Port);
        return STATUS_INVALID_HANDLE;
    }

    if (Section->SectionObject != NULL)
        ObDereferenceObject(Section->SectionObject);
    ExFreePoolWithTag(Section, TAG_ALPC_HANDLE);

    ObDereferenceObject(Port);
    return STATUS_SUCCESS;
}

/**
 * @brief Maps a view of a port section into the caller's address space.
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcCreateSectionView(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _Inout_ PALPC_DATA_VIEW_ATTR ViewAttributes)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_SECTION Section;
    PKALPC_VIEW View;
    ALPC_DATA_VIEW_ATTR LocalAttr;
    PVOID ViewBase = NULL;
    SIZE_T ViewSize;
    LARGE_INTEGER SectionOffset;
    NTSTATUS Status;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForWrite(ViewAttributes, sizeof(ALPC_DATA_VIEW_ATTR), sizeof(ULONG));
        LocalAttr = *ViewAttributes;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Section = AlpcpReferenceHandle(&Port->CommunicationInfo->HandleTable, LocalAttr.SectionHandle);
    if (Section == NULL)
    {
        ObDereferenceObject(Port);
        return STATUS_INVALID_HANDLE;
    }

    ViewSize = LocalAttr.ViewSize;
    if (ViewSize == 0 || ViewSize > Section->Size)
        ViewSize = Section->Size;

    SectionOffset.QuadPart = 0;
    Status = MmMapViewOfSection(Section->SectionObject,
                                PsGetCurrentProcess(),
                                &ViewBase,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Port);
        return Status;
    }

    View = ExAllocatePoolWithTag(NonPagedPool, sizeof(KALPC_VIEW), TAG_ALPC_HANDLE);
    if (View == NULL)
    {
        MmUnmapViewOfSection(PsGetCurrentProcess(), ViewBase);
        ObDereferenceObject(Port);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(View, sizeof(KALPC_VIEW));
    View->OwnerPort = Port;
    View->OwnerProcess = PsGetCurrentProcess();
    View->Address = ViewBase;
    View->Size = (ULONG)ViewSize;
    /* Keep the section object alive through the view so a VIEW message attribute
     * can re-map it into a receiver (SecureViewHandle holds the reference). */
    ObReferenceObject(Section->SectionObject);
    View->SecureViewHandle = Section->SectionObject;

    KeAcquireGuardedMutex(&AlpcpLock);
    InsertTailList(&Port->ResourceListHead, &View->ViewListEntry);
    KeReleaseGuardedMutex(&AlpcpLock);

    LocalAttr.ViewBase = ViewBase;
    LocalAttr.ViewSize = ViewSize;

    _SEH2_TRY
    {
        *ViewAttributes = LocalAttr;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ObDereferenceObject(Port);
    return Status;
}

/**
 * @brief Unmaps a section view from the caller's address space.
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcDeleteSectionView(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_ PVOID ViewBase)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_VIEW View = NULL;
    PLIST_ENTRY Entry;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    KeAcquireGuardedMutex(&AlpcpLock);
    for (Entry = Port->ResourceListHead.Flink;
         Entry != &Port->ResourceListHead;
         Entry = Entry->Flink)
    {
        PKALPC_VIEW Candidate = CONTAINING_RECORD(Entry, KALPC_VIEW, ViewListEntry);
        if (Candidate->Address == ViewBase)
        {
            View = Candidate;
            RemoveEntryList(&View->ViewListEntry);
            break;
        }
    }
    KeReleaseGuardedMutex(&AlpcpLock);

    if (View == NULL)
    {
        ObDereferenceObject(Port);
        return STATUS_INVALID_PARAMETER;
    }

    MmUnmapViewOfSection(PsGetCurrentProcess(), View->Address);
    if (View->SecureViewHandle != NULL)
        ObDereferenceObject(View->SecureViewHandle);
    ExFreePoolWithTag(View, TAG_ALPC_HANDLE);

    ObDereferenceObject(Port);
    return STATUS_SUCCESS;
}
