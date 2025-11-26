#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#pragma warning( push )
#pragma warning( disable : 4276 )

NTSTATUS NTAPI NtThawRegistry() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtThawTransactions() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }

NTSTATUS NTAPI NtTraceControl() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }

NTSTATUS NTAPI NtAddAtomEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAdjustTokenClaimsAndDeviceGroups() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlertThreadByThreadId(PKTHREAD Thread) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAllocateReserveObject() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcAcceptConnectPort() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcCancelMessage() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcConnectPort() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcConnectPortEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcCreatePort() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcCreatePortSection() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcCreateResourceReserve() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcCreateSectionView() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcCreateSecurityContext() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcDeletePortSection() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcDeleteResourceReserve() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcDeleteSectionView() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcDeleteSecurityContext() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcDisconnectPort() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNtAlpcImpersonateClientContainerOfPort() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcImpersonateClientOfPort() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcOpenSenderProcess() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcOpenSenderThread() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcQueryInformation() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcQueryInformationMessage() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcRevokeSecurityContext() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcSendWaitReceivePort() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcSetInformation() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAssociateWaitCompletionPacket() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCancelIoFileEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCancelSynchronousIoFile() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNtCancelTimer2() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCancelWaitCompletionPacket() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCommitComplete(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCommitEnlistment(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCommitTransaction(  _In_ HANDLE TransactionHandle,
  _In_ BOOLEAN Wait) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateDirectoryObjectEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateEnlistment(  _Out_ PHANDLE EnlistmentHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE ResourceManagerHandle,
  _In_ HANDLE TransactionHandle,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ ULONG CreateOptions,
  _In_ NOTIFICATION_MASK NotificationMask,
  _In_opt_ PVOID EnlistmentKey) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNtCreateIRTimer() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateKeyTransacted() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateLowBoxToken() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreatePartition() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreatePrivateNamespace() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateProfileEx(_Out_ PHANDLE ProfileHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ ULONG Granularity,
    _Out_ PVOID Buffer,
    _In_ ULONG ProfilingSize,
    _In_ KPROFILE_SOURCE Source,
    _In_ USHORT GroupCount,
    _In_reads_(GroupCount) PGROUP_AFFINITY Affinity) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateResourceManager(  _Out_ PHANDLE ResourceManagerHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE TmHandle,
  _In_ LPGUID RmGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ PUNICODE_STRING Description) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateThreadEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateTimer2() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateTokenEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateTransaction(  _Out_ PHANDLE TransactionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ LPGUID Uow,
  _In_opt_ HANDLE TmHandle,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ ULONG IsolationLevel,
  _In_opt_ ULONG IsolationFlags,
  _In_opt_ PLARGE_INTEGER Timeout,
  _In_opt_ PUNICODE_STRING Description) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateTransactionManager(  _Out_ PHANDLE TmHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PUNICODE_STRING LogFileName,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ ULONG CommitStrength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateUserProcess() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateWaitCompletionPacket() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateWnfStateName() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateWorkerFactory() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtDeletePrivateNamespace() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtDeleteWnfStateData() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtDeleteWnfStateName() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtDisableLastKnownGood() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtEnableLastKnownGood() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtEnumerateTransactionObject(  _In_opt_ HANDLE RootObjectHandle,
  _In_ KTMOBJECT_TYPE QueryType,
  _Inout_updates_bytes_(ObjectCursorLength) PKTMOBJECT_CURSOR ObjectCursor,
  _In_ ULONG ObjectCursorLength,
  _Out_ PULONG ReturnLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtFilterBootOption() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtFilterTokenEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtFlushBuffersFileEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtFlushInstallUILanguage() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtFlushProcessWriteBuffers() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtFreezeRegistry() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtFreezeTransactions() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtGetCachedSigningLevel() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtGetCompleteWnfStateSubscription() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtGetCurrentProcessorNumberEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtGetMUIRegistryInfo() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtGetNextProcess() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtGetNextThread() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtGetNlsSectionPtr() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtGetNotificationResourceManager(  _In_ HANDLE ResourceManagerHandle,
  _Out_ PTRANSACTION_NOTIFICATION TransactionNotification,
  _In_ ULONG NotificationLength,
  _In_opt_ PLARGE_INTEGER Timeout,
  _Out_opt_ PULONG ReturnLength,
  _In_ ULONG Asynchronous,
  _In_opt_ ULONG_PTR AsynchronousContext) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtInitializeNlsFiles() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtIsUILanguageComitted() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtMapCMFModule() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNotifyChangeSession() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenEnlistment(  _Out_ PHANDLE EnlistmentHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE ResourceManagerHandle,
  _In_ LPGUID EnlistmentGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenKeyTransacted() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenKeyTransactedEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenPrivateNamespace() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenResourceManager(  _Out_ PHANDLE ResourceManagerHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE TmHandle,
  _In_opt_ LPGUID ResourceManagerGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenSession() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenTransaction(  _Out_ PHANDLE TransactionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ LPGUID Uow,
  _In_opt_ HANDLE TmHandle) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenTransactionManager(  _Out_ PHANDLE TmHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PUNICODE_STRING LogFileName,
  _In_opt_ LPGUID TmIdentity,
  _In_opt_ ULONG OpenOptions) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtPrePrepareComplete(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtPrePrepareEnlistment(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtPrefetchVirtualMemory() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtPrepareComplete(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtPrepareEnlistment(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtPropagationComplete(  _In_ HANDLE ResourceManagerHandle,
  _In_ ULONG RequestCookie,
  _In_ ULONG BufferLength,
  _In_ PVOID Buffer) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtPropagationFailed(  _In_ HANDLE ResourceManagerHandle,
  _In_ ULONG RequestCookie,
  _In_ NTSTATUS PropStatus) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQueryInformationEnlistment(  _In_ HANDLE EnlistmentHandle,
  _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  _Out_writes_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
  _In_ ULONG EnlistmentInformationLength,
  _Out_ PULONG ReturnLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQueryInformationResourceManager(  _In_ HANDLE ResourceManagerHandle,
  _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  _Out_writes_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
  _In_ ULONG ResourceManagerInformationLength,
  _Out_opt_ PULONG ReturnLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQueryInformationTransaction(  _In_ HANDLE TransactionHandle,
  _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  _Out_writes_bytes_(TransactionInformationLength) PVOID TransactionInformation,
  _In_ ULONG TransactionInformationLength,
  _Out_opt_ PULONG ReturnLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQueryInformationTransactionManager(  _In_ HANDLE TransactionManagerHandle,
  _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  _Out_writes_bytes_(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
  _In_ ULONG TransactionManagerInformationLength,
  _Out_ PULONG ReturnLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQueryInformationWorkerFactory() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQueryLicenseValue() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQuerySecurityAttributesToken() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQuerySystemInformationEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQueryWnfStateData() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtQueryWnfStateNameInformation() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtReadOnlyEnlistment(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRecoverEnlistment(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PVOID EnlistmentKey) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRecoverResourceManager(_In_ HANDLE ResourceManagerHandle) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRecoverTransactionManager(HANDLE TransactionManagerHandle) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRegisterProtocolAddressInformation(  _In_ HANDLE ResourceManager,
  _In_ PCRM_PROTOCOL_ID ProtocolId,
  _In_ ULONG ProtocolInformationSize,
  _In_ PVOID ProtocolInformation,
  _In_opt_ ULONG CreateOptions) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtReleaseWorkerFactoryWorker() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRemoveIoCompletionEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRenameTransactionManager(  _In_ PUNICODE_STRING LogFileName,
  _In_ LPGUID ExistingTransactionManagerGuid) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtReplacePartitionUnit() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRollbackComplete(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRollbackEnlistment(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRollbackTransaction(  _In_ HANDLE TransactionHandle,
  _In_ BOOLEAN Wait) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRollforwardTransactionManager(  _In_ HANDLE TransactionManagerHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSerializeBoot() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetInformationEnlistment(  _In_opt_ HANDLE EnlistmentHandle,
  _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  _In_reads_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
  _In_ ULONG EnlistmentInformationLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetInformationResourceManager(  _In_ HANDLE ResourceManagerHandle,
  _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  _In_reads_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
  _In_ ULONG ResourceManagerInformationLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetInformationTransaction(  _In_ HANDLE TransactionHandle,
  _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  _In_reads_bytes_(TransactionInformationLength) PVOID TransactionInformation,
  _In_ ULONG TransactionInformationLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetInformationTransactionManager(  _In_opt_ HANDLE TmHandle,
  _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  _In_reads_bytes_(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
  _In_ ULONG TransactionManagerInformationLength) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetInformationWorkerFactory() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetIoCompletionEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetSystemCodeIntegrityRoots() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetTimerEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtShutdownWorkerFactory() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSinglePhaseReject(  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSubscribeWnfStateChange() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtUmsThreadYield() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtUnmapViewOfSectionEx() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtUnsubscribeWnfStateChange() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtUpdateWnfStateData() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtWaitForAlertByThreadId() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtWaitForWnfNotifications() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtWaitForWorkViaWorkerFactory() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtReleaseCMFViewOwnership() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtManagePartition() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCallEnclave() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateEnclave() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNtInitializeEnclave() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNtLoadEnclaveData() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNtManagePartition() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtOpenPartition() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNtRevertContainerImpersonation() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtNtSetCachedSigningLevel() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetIRTimer() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetInformationSymbolicLink() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetInformationVirtualMemory() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetTimer2() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetWnfProcessNotificationEvent() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtAlpcImpersonateClientContainerOfPort() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCancelTimer2() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCreateIRTimer() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtInitializeEnclave() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtLoadEnclaveData() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtRevertContainerImpersonation() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSetCachedSigningLevel() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtSavepointTransaction() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtCompareObjects() { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS NTAPI NtWorkerFactoryWorkerReady(ULONG a) { UNIMPLEMENTED; return STATUS_NOT_IMPLEMENTED; }
NTSTATUS
NTAPI
NtAllocateVirtualMemoryEx(HANDLE ProcessHandle, IN OUT PVOID* UBaseAddress, PSIZE_T RegionSize, ULONG AllocationType,
                                           ULONG PageProtection, IN OUT PVOID ExtendedParameters OPTIONAL,
                                           ULONG ExtendedParameterCount)
{
    DPRINT1("NtAllocateVirtualMemoryEx stub\n");
    return NtAllocateVirtualMemory(ProcessHandle, UBaseAddress, 0, RegionSize, AllocationType, PageProtection);
}


#pragma warning( pop )
