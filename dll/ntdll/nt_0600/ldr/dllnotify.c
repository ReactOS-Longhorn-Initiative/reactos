/*
 * PROJECT:     ReactOS NT Layer/System API
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Dll Notification Implementation
 * COPYRIGHT:   Copyright 2024 Ratin Gao <ratin@knsoft.org>
 */

#include "ntdll_vista.h"

typedef struct _LDR_DLL_NOTIFICATION_ENTRY
{
    LIST_ENTRY List;
    PLDR_DLL_NOTIFICATION_FUNCTION Callback;
    PVOID Context;
} LDR_DLL_NOTIFICATION_ENTRY, *PLDR_DLL_NOTIFICATION_ENTRY;
_STATIC_ASSERT(FIELD_OFFSET(LDR_DLL_NOTIFICATION_ENTRY, List) == 0);

static RTL_STATIC_LIST_HEAD(LdrpDllNotificationList);

static RTL_CRITICAL_SECTION LdrpDllNotificationLock;

static RTL_CRITICAL_SECTION_DEBUG LdrpDllNotificationLockDebug = {
    .CriticalSection = &LdrpDllNotificationLock
};

static RTL_CRITICAL_SECTION LdrpDllNotificationLock = {
    &LdrpDllNotificationLockDebug,
    -1,
    0,
    0,
    0,
    0
};

NTSTATUS
NTAPI
LdrRegisterDllNotification(
    _In_ ULONG Flags,
    _In_ PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    _In_opt_ PVOID Context,
    _Out_ PVOID *Cookie)
{
    PLDR_DLL_NOTIFICATION_ENTRY NewEntry;

    /* Check input parameters */
    if (Flags != 0 || NotificationFunction == NULL || Cookie == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate new entry and assign input values */
    NewEntry = RtlAllocateHeap(LdrpHeap, 0, sizeof(*NewEntry));
    if (NewEntry == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    NewEntry->Callback = NotificationFunction;
    NewEntry->Context = Context;

    /* Add node to the end of global list */
    RtlEnterCriticalSection(&LdrpDllNotificationLock);
    if (LdrpDllNotificationList.Blink->Flink != &LdrpDllNotificationList)
    {
        FatalListEntryError(LdrpDllNotificationList.Blink,
                            &LdrpDllNotificationList,
                            LdrpDllNotificationList.Flink);
    }
    InsertTailList(&LdrpDllNotificationList, &NewEntry->List);
    RtlLeaveCriticalSection(&LdrpDllNotificationLock);

    /* Cookie is address of the new entry */
    *Cookie = NewEntry;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrUnregisterDllNotification(
    _In_ PVOID Cookie)
{
    NTSTATUS Status = STATUS_DLL_NOT_FOUND;
    PLIST_ENTRY Entry;

    /* Find entry to remove */
    RtlEnterCriticalSection(&LdrpDllNotificationLock);
    for (Entry = LdrpDllNotificationList.Flink;
         Entry != &LdrpDllNotificationList;
         Entry = Entry->Flink)
    {
        if (Entry == Cookie)
        {
            RtlpCheckListEntry(Entry);
            RemoveEntryList(Entry);
            Status = STATUS_SUCCESS;
            break;
        }
    }
    RtlLeaveCriticalSection(&LdrpDllNotificationLock);

    if (NT_SUCCESS(Status))
    {
        RtlFreeHeap(LdrpHeap, 0, Entry);
    }
    return Status;
}

VOID NTAPI LdrpSendDllNotifications(
    _In_ PLDR_DATA_TABLE_ENTRY DllEntry,
    _In_ ULONG NotificationReason)
{
    PLDR_DLL_NOTIFICATION_ENTRY Entry;
    LDR_DLL_NOTIFICATION_DATA NotificationData;

    /*
     * LDR_DLL_LOADED_NOTIFICATION_DATA and LDR_DLL_UNLOADED_NOTIFICATION_DATA
     * currently are the same
     */
#define LdrpAssertDllNotificationDataMember(x)\
    _STATIC_ASSERT(FIELD_OFFSET(LDR_DLL_NOTIFICATION_DATA, Loaded.x) ==\
                   FIELD_OFFSET(LDR_DLL_NOTIFICATION_DATA, Unloaded.x))

    _STATIC_ASSERT(sizeof(NotificationData.Loaded) == sizeof(NotificationData.Unloaded));
    LdrpAssertDllNotificationDataMember(Flags);
    LdrpAssertDllNotificationDataMember(FullDllName);
    LdrpAssertDllNotificationDataMember(BaseDllName);
    LdrpAssertDllNotificationDataMember(DllBase);
    LdrpAssertDllNotificationDataMember(SizeOfImage);
#undef LdrpAssertDllNotificationDataMember

    NotificationData.Loaded.Flags = 0;
    NotificationData.Loaded.FullDllName = &DllEntry->FullDllName;
    NotificationData.Loaded.BaseDllName = &DllEntry->BaseDllName;
    NotificationData.Loaded.DllBase = DllEntry->DllBase;
    NotificationData.Loaded.SizeOfImage = DllEntry->SizeOfImage;

    RtlEnterCriticalSection(&LdrpDllNotificationLock);
    for (Entry = (PLDR_DLL_NOTIFICATION_ENTRY)LdrpDllNotificationList.Flink;
         Entry != (PLDR_DLL_NOTIFICATION_ENTRY)&LdrpDllNotificationList;
         Entry = (PLDR_DLL_NOTIFICATION_ENTRY)Entry->List.Flink)
    {
        Entry->Callback(NotificationReason, &NotificationData, Entry->Context);
    }
    RtlLeaveCriticalSection(&LdrpDllNotificationLock);
}
