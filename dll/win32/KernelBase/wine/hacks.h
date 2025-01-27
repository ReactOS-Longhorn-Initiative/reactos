#pragma once

//TODO: everything here belongs somewhere, to save time put them here.
#define CRITICAL_SECTION RTL_CRITICAL_SECTION
#define CRITICAL_SECTION_DEBUG RTL_CRITICAL_SECTION_DEBUG

#define RESOURCE_ENUM_LN          0x0001
#define RESOURCE_ENUM_MUI         0x0002
#define RESOURCE_ENUM_MUI_SYSTEM  0x0004
#define RESOURCE_ENUM_VALIDATE    0x0008
#define PROCESSOR_ARCHITECTURE_NEUTRAL          11
#define PROCESSOR_ARCHITECTURE_ARM64            12
//ddk
#define SYMBOLIC_LINK_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | 0x1)

//def ndk
NTSYSAPI
BOOLEAN
NTAPI
RtlGetProductInfo(
  _In_ ULONG OSMajorVersion,
  _In_ ULONG OSMinorVersion,
  _In_ ULONG SpMajorVersion,
  _In_ ULONG SpMinorVersion,
  _Out_ PULONG ReturnedProductType);
NTSYSAPI NTSTATUS  WINAPI RtlQueryActivationContextApplicationSettings(DWORD,HANDLE,const WCHAR*,const WCHAR*,WCHAR*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI LdrSetDefaultDllDirectories(ULONG);
NTSYSAPI NTSTATUS  WINAPI LdrRemoveDllDirectory(void*);

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

//kernel32
 BOOL WINAPI DECLSPEC_HOTPATCH GetThreadGroupAffinity( HANDLE thread, GROUP_AFFINITY *affinity );

typedef struct _THREAD_NAME_INFORMATION
{
    UNICODE_STRING ThreadName;
} THREAD_NAME_INFORMATION, *PTHREAD_NAME_INFORMATION;

HANDLE WINAPI DECLSPEC_HOTPATCH CreateRemoteThreadEx( HANDLE process, SECURITY_ATTRIBUTES *sa,
                                                      SIZE_T stack, LPTHREAD_START_ROUTINE start,
                                                      LPVOID param, DWORD flags,
                                                      LPPROC_THREAD_ATTRIBUTE_LIST attributes, DWORD *id );


BOOL WINAPI GetVolumeInformationByHandleW( HANDLE handle, WCHAR *label, DWORD label_len,
                                           DWORD *serial, DWORD *filename_len, DWORD *flags,
                                           WCHAR *fsname, DWORD fsname_len );


HRESULT WINAPI PathMatchSpecExW(const WCHAR *path, const WCHAR *mask, DWORD flags);
INT WINAPI DECLSPEC_HOTPATCH CompareStringOrdinal( const WCHAR *str1, INT len1,
                                                   const WCHAR *str2, INT len2, BOOL ignore_case );

#define URL_UNESCAPE_AS_UTF8            URL_ESCAPE_AS_UTF8

//shlwapi
HRESULT WINAPI GetAcceptLanguagesW(WCHAR *langbuf, DWORD *buflen);

BOOL WINAPI GetWindowsAccountDomainSid( PSID sid, PSID domain_sid, DWORD *size );
//rtl types
//
// Constant String Macro
//
#define RTL_CONSTANT_STRING(__SOURCE_STRING__)                  \
{                                                               \
    sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
    sizeof(__SOURCE_STRING__),                                  \
    (__SOURCE_STRING__)                                         \
}
//rtl
NTSTATUS
NTAPI
RtlNewSecurityObjectEx(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                       IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                       OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                       IN LPGUID ObjectType,
                       IN BOOLEAN IsDirectoryObject,
                       IN ULONG AutoInheritFlags,
                       IN HANDLE Token,
                       IN PGENERIC_MAPPING GenericMapping);

NTSTATUS
NTAPI
RtlNewSecurityObjectWithMultipleInheritance(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                            IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                            OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                            IN LPGUID *ObjectTypes,
                                            IN ULONG GuidCount,
                                            IN BOOLEAN IsDirectoryObject,
                                            IN ULONG AutoInheritFlags,
                                            IN HANDLE Token,
                                            IN PGENERIC_MAPPING GenericMapping);
NTSTATUS
NTAPI
RtlConvertToAutoInheritSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                      IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                      OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                      IN LPGUID ObjectType,
                                      IN BOOLEAN IsDirectoryObject,
                                      IN PGENERIC_MAPPING GenericMapping);

BOOL WINAPI DECLSPEC_HOTPATCH QueryFullProcessImageNameW( HANDLE process, DWORD flags,
                                                          WCHAR *name, DWORD *size );


typedef struct _REASON_CONTEXT {
    ULONG Version;
    DWORD Flags;
    union {
        struct {
            HMODULE LocalizedReasonModule;
            ULONG LocalizedReasonId;
            ULONG ReasonStringCount;
            LPWSTR *ReasonStrings;

        } Detailed;

        LPWSTR SimpleReasonString;
    } Reason;
} REASON_CONTEXT, *PREASON_CONTEXT;

typedef enum _THREAD_INFORMATION_CLASS {
    ThreadMemoryPriority,
    ThreadAbsoluteCpuPriority,
    ThreadDynamicCodePolicy,
    ThreadPowerThrottling,
    ThreadInformationClassMax
} THREAD_INFORMATION_CLASS;
typedef void *PUMS_CONTEXT;
typedef void *PUMS_COMPLETION_LIST;
typedef PVOID PUMS_SCHEDULER_ENTRY_POINT;
typedef struct _UMS_SCHEDULER_STARTUP_INFO
{
    ULONG UmsVersion;
    PUMS_COMPLETION_LIST CompletionList;
    PUMS_SCHEDULER_ENTRY_POINT SchedulerProc;
    PVOID SchedulerParam;
} UMS_SCHEDULER_STARTUP_INFO, *PUMS_SCHEDULER_STARTUP_INFO;

typedef enum _RTL_UMS_SCHEDULER_REASON UMS_SCHEDULER_REASON;
typedef enum _RTL_UMS_THREAD_INFO_CLASS UMS_THREAD_INFO_CLASS, *PUMS_THREAD_INFO_CLASS;
typedef enum AppPolicyMediaFoundationCodecLoading
{
    AppPolicyMediaFoundationCodecLoading_All = 0,
    AppPolicyMediaFoundationCodecLoading_InboxOnly = 1
} AppPolicyMediaFoundationCodecLoading;
typedef void (CALLBACK *PTP_IO_CALLBACK)(PTP_CALLBACK_INSTANCE,void*,void*,IO_STATUS_BLOCK*,PTP_IO);

NTSYSAPI NTSTATUS  WINAPI RtlWow64SetThreadContext(HANDLE,const WOW64_CONTEXT*);
NTSYSAPI NTSTATUS  WINAPI RtlWow64GetThreadContext(HANDLE,WOW64_CONTEXT*);

NTSYSAPI NTSTATUS  WINAPI TpAllocCleanupGroup(TP_CLEANUP_GROUP **);
NTSYSAPI NTSTATUS  WINAPI TpAllocIoCompletion(TP_IO **,HANDLE,PTP_IO_CALLBACK,void *,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpAllocPool(TP_POOL **,PVOID);
NTSYSAPI NTSTATUS  WINAPI TpAllocTimer(TP_TIMER **,PTP_TIMER_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpAllocWait(TP_WAIT **,PTP_WAIT_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpAllocWork(TP_WORK **,PTP_WORK_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI void      WINAPI TpCallbackLeaveCriticalSectionOnCompletion(TP_CALLBACK_INSTANCE *,RTL_CRITICAL_SECTION *);
NTSYSAPI NTSTATUS  WINAPI TpCallbackMayRunLong(TP_CALLBACK_INSTANCE *);
NTSYSAPI void      WINAPI TpCallbackReleaseMutexOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE);
NTSYSAPI void      WINAPI TpCallbackReleaseSemaphoreOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE,DWORD);
NTSYSAPI void      WINAPI TpCallbackSetEventOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE);
NTSYSAPI void      WINAPI TpCallbackUnloadDllOnCompletion(TP_CALLBACK_INSTANCE *,HMODULE);
NTSYSAPI void      WINAPI TpCancelAsyncIoOperation(TP_IO *);
NTSYSAPI void      WINAPI TpDisassociateCallback(TP_CALLBACK_INSTANCE *);
NTSYSAPI BOOL      WINAPI TpIsTimerSet(TP_TIMER *);
NTSYSAPI void      WINAPI TpPostWork(TP_WORK *);
NTSYSAPI NTSTATUS  WINAPI TpQueryPoolStackInformation(TP_POOL *, TP_POOL_STACK_INFORMATION *stack_info);
NTSYSAPI void      WINAPI TpReleaseCleanupGroup(TP_CLEANUP_GROUP *);
NTSYSAPI void      WINAPI TpReleaseCleanupGroupMembers(TP_CLEANUP_GROUP *,BOOL,PVOID);
NTSYSAPI void      WINAPI TpReleaseIoCompletion(TP_IO *);
NTSYSAPI void      WINAPI TpReleasePool(TP_POOL *);
NTSYSAPI void      WINAPI TpReleaseTimer(TP_TIMER *);
NTSYSAPI void      WINAPI TpReleaseWait(TP_WAIT *);
NTSYSAPI void      WINAPI TpReleaseWork(TP_WORK *);
NTSYSAPI void      WINAPI TpSetPoolMaxThreads(TP_POOL *,DWORD);
NTSYSAPI BOOL      WINAPI TpSetPoolMinThreads(TP_POOL *,DWORD);
NTSYSAPI NTSTATUS  WINAPI TpSetPoolStackInformation(TP_POOL *, TP_POOL_STACK_INFORMATION *stack_info);
NTSYSAPI void      WINAPI TpSetTimer(TP_TIMER *, LARGE_INTEGER *,LONG,LONG);
NTSYSAPI void      WINAPI TpSetWait(TP_WAIT *,HANDLE,LARGE_INTEGER *);
NTSYSAPI NTSTATUS  WINAPI TpSimpleTryPost(PTP_SIMPLE_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI void      WINAPI TpStartAsyncIoOperation(TP_IO *);
NTSYSAPI void      WINAPI TpWaitForIoCompletion(TP_IO *,BOOL);
NTSYSAPI void      WINAPI TpWaitForTimer(TP_TIMER *,BOOL);
NTSYSAPI void      WINAPI TpWaitForWait(TP_WAIT *,BOOL);
NTSYSAPI void      WINAPI TpWaitForWork(TP_WORK *,BOOL);
