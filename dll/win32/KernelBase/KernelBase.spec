;Windows 7 (checked as of 8/17/2021)
@ stdcall -version=0x601+ AccessCheck() ADVAPI32.AccessCheck
@ stdcall -version=0x601+ AccessCheckAndAuditAlarmW() ADVAPI32.AccessCheckAndAuditAlarmW
@ stdcall -version=0x601+ AccessCheckByType() ADVAPI32.AccessCheckByType
@ stdcall -version=0x601+ AccessCheckByTypeAndAuditAlarmW() ADVAPI32.AccessCheckByTypeAndAuditAlarmW
@ stdcall -version=0x601+ AccessCheckByTypeResultList() ADVAPI32.AccessCheckByTypeResultList
@ stdcall -version=0x601+ AccessCheckByTypeResultListAndAuditAlarmByHandleW() ADVAPI32.AccessCheckByTypeResultListAndAuditAlarmByHandleW
@ stdcall -version=0x601+ AccessCheckByTypeResultListAndAuditAlarmW() ADVAPI32.AccessCheckByTypeResultListAndAuditAlarmW 
@ stdcall -version=0x601+ AcquireSRWLockExclusive() ntdll.AcquireSRWLockExclusive
@ stdcall -version=0x601+ AcquireSRWLockShared() ntdll.AcquireSRWLockShared
@ stdcall -version=0x601+ AddAccessAllowedAce() ADVAPI32.AddAccessAllowedAce
@ stdcall -version=0x601+ AddAccessAllowedAceEx() ADVAPI32.AddAccessAllowedAceEx
@ stdcall -version=0x601+ AddAccessAllowedObjectAce() ADVAPI32.AddAccessAllowedObjectAce
@ stdcall -version=0x601+ AddAccessDeniedAce() ADVAPI32.AddAccessDeniedAce
@ stdcall -version=0x601+ AddAccessDeniedAceEx() ADVAPI32.AddAccessDeniedAceEx
@ stdcall -version=0x601+ AddAccessDeniedObjectAce() ADVAPI32.AddAccessDeniedObjectAce
@ stdcall -version=0x601+ AddAce() ADVAPI32.AddAce
@ stdcall -version=0x601+ AddAuditAccessAce() ADVAPI32.AddAuditAccessAce
@ stdcall -version=0x601+ AddAuditAccessAceEx() ADVAPI32.AddAuditAccessAceEx
@ stdcall -version=0x601+ AddAuditAccessObjectAce() ADVAPI32.AddAuditAccessObjectAce
@ stdcall -stub -version=0x601+ AddMandatoryAce() 
@ stdcall -version=0x601+ AdjustTokenGroups() ADVAPI32.AdjustTokenGroups
@ stdcall -version=0x601+ AdjustTokenPrivileges() ADVAPI32.AdjustTokenPrivileges
@ stdcall -version=0x601+ AllocateAndInitializeSid() ADVAPI32.AllocateAndInitializeSid
@ stdcall -version=0x601+ AllocateLocallyUniqueId() ADVAPI32.AllocateLocallyUniqueId
@ stdcall -version=0x601+ AreAllAccessesGranted() ADVAPI32.AreAllAccessesGranted
@ stdcall -version=0x601+ AreAnyAccessesGranted() ADVAPI32.AreAnyAccessesGranted
@ stdcall -version=0x601+ AreFileApisANSI() kernel32.AreFileApisANSI
@ stub -version=0x601+ BaseDllFreeResourceId
@ stub -version=0x601+ BaseDllMapResourceIdW
@ stub -version=0x601 BaseGetProcessDllPath
@ stub -version=0x601 BaseGetProcessExePath
@ stub -version=0x601 BaseInvalidateDllSearchPathCache
@ stub -version=0x601 BaseInvalidateProcessSearchPathCache
@ stub -version=0x601 BaseReleaseProcessDllPath
@ stub -version=0x601 BaseReleaseProcessExePath
@ stdcall -version=0x601+ Beep() kernel32.Beep
@ stub -version=0x601-0x603 BemCopyReference
@ stub -version=0x601-0x603 BemCreateContractFrom
@ stub -version=0x601-0x603 BemCreateReference
@ stub -version=0x601-0x603 BemFreeContract
@ stub -version=0x601-0x603 BemFreeReference
@ stdcall -version=0x601+ CallbackMayRunLong() kernel32.CallbackMayRunLong
@ stdcall -version=0x601+ CancelIoEx() kernel32.CancelIoEx
@ stdcall -version=0x601+ CancelThreadpoolIo() ntdll.TpCancelAsyncIoOperation
@ stdcall -version=0x601+ CancelWaitableTimer() kernel32.CancelWaitableTimer
@ stdcall -version=0x601+ ChangeTimerQueueTimer() kernel32.ChangeTimerQueueTimer
@ stub -version=0x601+ CheckGroupPolicyEnabled
@ stdcall -version=0x601+ CheckTokenMembership() ADVAPI32.CheckTokenMembership
@ stdcall -version=0x601+ CloseHandle() kernel32.CloseHandle
@ stdcall -version=0x601+ CloseThreadpool() ntdll.TpReleasePool
@ stdcall -version=0x601+ CloseThreadpoolCleanupGroup() ntdll.TpReleaseCleanupGroup
@ stdcall -version=0x601+ CloseThreadpoolCleanupGroupMembers() ntdll.TpReleaseCleanupGroupMembers
@ stdcall -version=0x601+ CloseThreadpoolIo() ntdll.TpReleaseIoCompletion
@ stdcall -version=0x601+ CloseThreadpoolTimer() ntdll.TpReleaseTimer
@ stdcall -version=0x601+ CloseThreadpoolWait() ntdll.TpReleaseWait
@ stdcall -version=0x601+ CloseThreadpoolWork() ntdll.TpReleaseWork
@ stdcall -version=0x601+ CompareFileTime() kernel32.CompareFileTime
@ stdcall -version=0x601+ CompareStringA() kernel32.CompareStringA
@ stdcall -version=0x601+ CompareStringEx() kernel32.CompareStringEx
@ stdcall -version=0x601+ CompareStringOrdinal() kernel32.CompareStringOrdinal
@ stdcall -version=0x601+ CompareStringW() kernel32.CompareStringW
@ stdcall -version=0x601+ ConnectNamedPipe() kernel32.ConnectNamedPipe
@ stdcall -version=0x601+ ConvertDefaultLocale() kernel32.ConvertDefaultLocale
@ stdcall -version=0x601+ ConvertToAutoInheritPrivateObjectSecurity() ADVAPI32.ConvertToAutoInheritPrivateObjectSecurity
@ stdcall -version=0x601+ CopySid() ADVAPI32.CopySid
@ stdcall -version=0x601+ CreateDirectoryA() kernel32.CreateDirectoryA
@ stdcall -version=0x601+ CreateDirectoryW() kernel32.CreateDirectoryW
@ stdcall -version=0x601+ CreateEventA() kernel32.CreateEventA
@ stdcall -version=0x601+ CreateEventExA() kernel32.CreateEventExA
@ stdcall -version=0x601+ CreateEventExW() kernel32.CreateEventExW
@ stdcall -version=0x601+ CreateEventW() kernel32.CreateEventW
@ stdcall -version=0x601+ CreateFileA() kernel32.CreateFileA
@ stdcall -version=0x601+ CreateFileMappingNumaW() kernel32.CreateFileMappingNumaW
@ stdcall -version=0x601+ CreateFileMappingW() kernel32.CreateFileMappingW
@ stdcall -version=0x601+ CreateFileW() kernel32.CreateFileW
@ stdcall -version=0x601+ CreateIoCompletionPort() kernel32.CreateIoCompletionPort
@ stdcall -version=0x601+ CreateMutexA() kernel32.CreateMutexA
@ stdcall -version=0x601+ CreateMutexExA() kernel32.CreateMutexExA
@ stdcall -version=0x601+ CreateMutexExW() kernel32.CreateMutexExW
@ stdcall -version=0x601+ CreateMutexW() kernel32.CreateMutexW
@ stdcall -version=0x601+ CreateNamedPipeW() kernel32.CreateNamedPipeW
@ stdcall -version=0x601+ CreatePipe() kernel32.CreatePipe
@ stdcall -version=0x601+ CreatePrivateObjectSecurity() kernel32.CreatePrivateObjectSecurity
@ stdcall -version=0x601+ CreatePrivateObjectSecurityEx() kernel32.CreatePrivateObjectSecurityEx
;@ stdcall -version=0x601+ CreatePrivateObjectSecurityWithMultipleInheritance()
@ stdcall -version=0x601+ CreateRemoteThread() kernel32.CreateRemoteThread
@ stdcall -stub -version=0x601+ CreateRemoteThreadEx() ;kernel32.CreateRemoteThreadEx
@ stdcall -version=0x601+ CreateRestrictedToken() kernel32.CreateRestrictedToken
@ stdcall -version=0x601+ CreateSemaphoreExW() kernel32.CreateSemaphoreExW
@ stdcall -version=0x601+ CreateThread() kernel32.CreateThread
@ stdcall -version=0x601+ CreateThreadpool() kernel32.CreateThreadpool
@ stdcall -version=0x601+ CreateThreadpoolCleanupGroup() kernel32.CreateThreadpoolCleanupGroup
@ stdcall -version=0x601+ CreateThreadpoolIo() kernel32.CreateThreadpoolIo
@ stdcall -version=0x601+ CreateThreadpoolTimer() kernel32.CreateThreadpoolTimer
@ stdcall -version=0x601+ CreateThreadpoolWait() kernel32.CreateThreadpoolWait
@ stdcall -version=0x601+ CreateThreadpoolWork() kernel32.CreateThreadpoolWork
@ stdcall -version=0x601+ CreateTimerQueue() kernel32.CreateTimerQueue
@ stdcall -version=0x601+ CreateTimerQueueTimer() kernel32.CreateTimerQueueTimer
@ stdcall -version=0x601+ CreateWaitableTimerExW() kernel32.CreateWaitableTimerExW
@ stdcall -version=0x601+ CreateWellKnownSid() ADVAPI32.CreateWellKnownSid
@ stdcall -version=0x601+ DebugBreak() kernel32.DebugBreak
@ stdcall -version=0x601+ DecodePointer() ntdll.RtlDecodePointer;
@ stdcall -version=0x601+ DecodeSystemPointer() ntdll.RtlDecodeSystemPointer;
@ stdcall -version=0x601+ DefineDosDeviceW() kernel32.DefineDosDeviceW
@ stdcall -version=0x601+ DeleteAce() ADVAPI32.DeleteAce
@ stdcall -version=0x601+ DeleteCriticalSection() ntdll.RtlDeleteCriticalSection;
@ stdcall -version=0x601+ DeleteFileA() kernel32.DeleteFileA
@ stdcall -version=0x601+ DeleteFileW() kernel32.DeleteFileW
@ stdcall -version=0x601+ DeleteProcThreadAttributeList() kernel32.DeleteProcThreadAttributeList
@ stdcall -version=0x601+ DeleteTimerQueueEx() kernel32.DeleteTimerQueueEx
@ stdcall -version=0x601+ DeleteTimerQueueTimer() kernel32.DeleteTimerQueueTimer
@ stdcall -version=0x601+ DeleteVolumeMountPointW() kernel32.DeleteVolumeMountPointW
@ stdcall -version=0x601+ DestroyPrivateObjectSecurity() ADVAPI32.DestroyPrivateObjectSecurity
@ stdcall -version=0x601+ DeviceIoControl() kernel32.DeviceIoControl
@ stdcall -version=0x601+ DisableThreadLibraryCalls() kernel32.DisableThreadLibraryCalls
@ stdcall -version=0x601+ DisassociateCurrentThreadFromCallback() ntdll.TpDisassociateCallback;
@ stdcall -version=0x601+ DisconnectNamedPipe() kernel32.DisconnectNamedPipe
@ stdcall -version=0x601+ DuplicateHandle() kernel32.DuplicateHandle
@ stdcall -version=0x601+ DuplicateToken() ADVAPI32.DuplicateToken
@ stdcall -version=0x601+ DuplicateTokenEx() ADVAPI32.DuplicateTokenEx
@ stdcall -version=0x601+ EncodePointer() ntdll.RtlEncodePointer;
@ stdcall -version=0x601+ EncodeSystemPointer() ntdll.RtlEncodeSystemPointer;
@ stdcall -version=0x601+ EnterCriticalSection() ntdll.RtlEnterCriticalSection;
@ stdcall -version=0x601+ EnumCalendarInfoExEx() kernel32.EnumCalendarInfoExEx
@ stdcall -version=0x601+ EnumCalendarInfoExW() kernel32.EnumCalendarInfoExW
@ stdcall -version=0x601+ EnumCalendarInfoW() kernel32.EnumCalendarInfoW
@ stdcall -version=0x601+ EnumDateFormatsExEx() kernel32.EnumDateFormatsExEx
@ stdcall -version=0x601+ EnumDateFormatsExW() kernel32.EnumDateFormatsExW
@ stdcall -version=0x601+ EnumDateFormatsW() kernel32.EnumDateFormatsW
@ stdcall -version=0x601+ EnumLanguageGroupLocalesW() kernel32.EnumLanguageGroupLocalesW
@ stdcall -version=0x601+ EnumSystemCodePagesW() kernel32.EnumSystemCodePagesW
@ stdcall -version=0x601+ EnumSystemLanguageGroupsW() kernel32.EnumSystemLanguageGroupsW
@ stdcall -version=0x601+ EnumSystemLocalesA() kernel32.EnumSystemLocalesA
@ stdcall -version=0x601+ EnumSystemLocalesEx() kernel32.EnumSystemLocalesEx
@ stdcall -version=0x601+ EnumSystemLocalesW() kernel32.EnumSystemLocalesW
@ stdcall -version=0x601+ EnumTimeFormatsEx() kernel32.EnumTimeFormatsEx
@ stdcall -version=0x601+ EnumTimeFormatsW() kernel32.EnumTimeFormatsW
@ stdcall -version=0x601+ EnumUILanguagesW() kernel32.EnumUILanguagesW
@ stdcall -version=0x601+ EqualDomainSid() ADVAPI32.EqualDomainSid
@ stdcall -version=0x601+ EqualPrefixSid() ADVAPI32.EqualPrefixSid
@ stdcall -version=0x601+ EqualSid() ADVAPI32.EqualSid
@ stdcall -version=0x601+ ExitProcess() ntdll.RtlExitUserProcess ;6.1 and higher (x64) and 6.2 and higher (x86);
@ stdcall -version=0x601+ ExitThread() ntdll.RtlExitUserThread;
@ stdcall -version=0x601+ ExpandEnvironmentStringsA() kernel32.ExpandEnvironmentStringsA
@ stdcall -version=0x601+ ExpandEnvironmentStringsW() kernel32.ExpandEnvironmentStringsW
@ stdcall -version=0x601+ FatalAppExitA() kernel32.FatalAppExitA
@ stdcall -version=0x601+ FatalAppExitW() kernel32.FatalAppExitW
@ stdcall -version=0x601+ FileTimeToLocalFileTime() kernel32.FileTimeToLocalFileTime
@ stdcall -version=0x601+ FileTimeToSystemTime() kernel32.FileTimeToSystemTime
@ stdcall -version=0x601+ FindClose() kernel32.FindClose
@ stdcall -version=0x601+ FindCloseChangeNotification() kernel32.FindCloseChangeNotification
@ stdcall -version=0x601+ FindFirstChangeNotificationA() kernel32.FindFirstChangeNotificationA
@ stdcall -version=0x601+ FindFirstChangeNotificationW() kernel32.FindFirstChangeNotificationW
@ stdcall -version=0x601+ FindFirstFileA() kernel32.FindFirstFileA
@ stdcall -version=0x601+ FindFirstFileExA() kernel32.FindFirstFileExA
@ stdcall -version=0x601+ FindFirstFileExW() kernel32.FindFirstFileExW
@ stdcall -version=0x601+ FindFirstFileW() kernel32.FindFirstFileW
@ stdcall -version=0x601+ FindFirstFreeAce() ADVAPI32.FindFirstFreeAce
@ stdcall -version=0x601+ FindFirstVolumeW() kernel32.FindFirstVolumeW
@ stdcall -version=0x601+ FindNLSString() kernel32.FindNLSString
@ stdcall -version=0x601+ FindNLSStringEx() kernel32.FindNLSStringEx
@ stdcall -version=0x601+ FindNextChangeNotification() kernel32.FindNextChangeNotification
@ stdcall -version=0x601+ FindNextFileA() kernel32.FindNextFileA
@ stdcall -version=0x601+ FindNextFileW() kernel32.FindNextFileW
@ stdcall -version=0x601+ FindNextVolumeW() kernel32.FindNextVolumeW
@ stdcall -version=0x601+ FindResourceExW() kernel32.FindResourceExW
@ stdcall -stub -version=0x601+ FindStringOrdinal() ;kernel32.FindStringOrdinal
@ stdcall -version=0x601+ FindVolumeClose() kernel32.FindVolumeClose
@ stdcall -version=0x601+ FlsAlloc() kernel32.FlsAlloc
@ stdcall -version=0x601+ FlsFree() kernel32.FlsFree
@ stdcall -version=0x601+ FlsGetValue() kernel32.FlsGetValue
@ stdcall -version=0x601+ FlsSetValue() kernel32.FlsSetValue
@ stdcall -version=0x601+ FlushFileBuffers() kernel32.FlushFileBuffers
@ stdcall -stub -version=0x601+ FlushProcessWriteBuffers()
@ stdcall -version=0x601+ FlushViewOfFile() kernel32.FlushViewOfFile
@ stdcall -version=0x601+ FoldStringW() kernel32.FoldStringA
@ stdcall -version=0x601+ FormatMessageA() kernel32.FormatMessageA
@ stdcall -version=0x601+ FormatMessageW() kernel32.FormatMessageW
@ stdcall -version=0x601+ FreeEnvironmentStringsA() kernel32.FreeEnvironmentStringsA
@ stdcall -version=0x601+ FreeEnvironmentStringsW() kernel32.FreeEnvironmentStringsW
@ stdcall -version=0x601+ FreeLibrary() kernel32.FreeLibrary
@ stdcall -version=0x601+ FreeLibraryAndExitThread() kernel32.FreeLibraryAndExitThread
@ stdcall -version=0x601+ FreeLibraryWhenCallbackReturns() ntdll.TpCallbackUnloadDllOnCompletion;
@ stdcall -version=0x601+ FreeResource() kernel32.FreeResource
@ stdcall -version=0x601+ FreeSid() ADVAPI32.FreeSid
@ stdcall -version=0x601+ GetACP() kernel32.GetACP
@ stdcall -version=0x601+ GetAce() ADVAPI32.GetAce
@ stdcall -version=0x601+ GetAclInformation() ADVAPI32.GetAclInformation
@ stdcall -stub -version=0x601+ GetCPFileNameFromRegistry(); kernel32.GetCPFileNameFromRegistry
@ stub -version=0x601+ GetCPHashNode	 
@ stdcall -version=0x601+ GetCPInfo() kernel32.GetCPInfo
@ stdcall -version=0x601+ GetCPInfoExW() kernel32.GetCPInfoExW 
@ stub -version=0x601+ GetCalendar	 
@ stdcall -version=0x601+ GetCalendarInfoEx() kernel32.GetCalendarInfoEx
@ stdcall -version=0x601+ GetCalendarInfoW() kernel32.GetCalendarInfoW
@ stdcall -version=0x601+ GetCommandLineA() kernel32.GetCommandLineA
@ stdcall -version=0x601+ GetCommandLineW() kernel32.GetCommandLineW
@ stdcall -version=0x601+ GetComputerNameExA() kernel32.GetComputerNameExA
@ stdcall -version=0x601+ GetComputerNameExW() kernel32.GetComputerNameExW
@ stdcall -version=0x601+ GetCurrencyFormatEx() kernel32.GetCurrencyFormatEx
@ stdcall -version=0x601+ GetCurrencyFormatW() kernel32.GetCurrencyFormatW
@ stdcall -version=0x601+ GetCurrentDirectoryA() kernel32.GetCurrentDirectoryA
@ stdcall -version=0x601+ GetCurrentDirectoryW() kernel32.GetCurrentDirectoryW
@ stdcall -version=0x601+ GetCurrentProcess() kernel32.GetCurrentProcess
@ stdcall -version=0x601+ GetCurrentProcessId() kernel32.GetCurrentProcessId
@ stdcall -version=0x601+ GetCurrentThread() kernel32.GetCurrentThread
@ stdcall -version=0x601+ GetCurrentThreadId() kernel32.GetCurrentThreadId
@ stdcall -version=0x601+ GetDiskFreeSpaceA() kernel32.GetDiskFreeSpaceA
@ stdcall -version=0x601+ GetDiskFreeSpaceExA() kernel32.GetDiskFreeSpaceExA	
@ stdcall -version=0x601+ GetDiskFreeSpaceExW() kernel32.GetDiskFreeSpaceExW
@ stdcall -version=0x601+ GetDiskFreeSpaceW() kernel32.GetDiskFreeSpaceW
@ stdcall -version=0x601+ GetDriveTypeA() kernel32.GetDriveTypeA
@ stdcall -version=0x601+ GetDriveTypeW() kernel32.GetDriveTypeW
@ stdcall -version=0x601+ GetDynamicTimeZoneInformation() kernel32.GetDynamicTimeZoneInformation
@ stdcall -version=0x601+ GetEnvironmentStrings() kernel32.GetEnvironmentStrings
@ stdcall -version=0x601+ GetEnvironmentStringsA() kernel32.GetEnvironmentStringsA
@ stdcall -version=0x601+ GetEnvironmentStringsW() kernel32.GetEnvironmentStringsW
@ stdcall -version=0x601+ GetEnvironmentVariableA() kernel32.GetEnvironmentVariableA
@ stdcall -version=0x601+ GetEnvironmentVariableW() kernel32.GetEnvironmentVariableW
@ stub -version=0x601+ GetEraNameCountedString	 
@ stdcall -version=0x601+ GetErrorMode() kernel32.GetErrorMode
@ stdcall -version=0x601+ GetExitCodeProcess() kernel32.GetExitCodeProcess
@ stdcall -version=0x601+ GetExitCodeThread() kernel32.GetExitCodeThread
@ stub -version=0x601+ GetFallbackDisplayName	 
@ stdcall -version=0x601+ GetFileAttributesA() kernel32.GetFileAttributesA
@ stdcall -version=0x601+ GetFileAttributesExA() kernel32.GetFileAttributesExA
@ stdcall -version=0x601+ GetFileAttributesExW() kernel32.GetFileAttributesExW
@ stdcall -version=0x601+ GetFileAttributesW() kernel32.GetFileAttributesW
@ stdcall -version=0x601+ GetFileInformationByHandle() kernel32.GetFileInformationByHandle
@ stdcall -version=0x601+ GetFileMUIInfo() kernel32.GetFileMUIInfo
@ stdcall -version=0x601+ GetFileMUIPath() kernel32.GetFileMUIPath
@ stdcall -version=0x601+ GetFileSecurityW() ADVAPI32.GetFileSecurityW
@ stdcall -version=0x601+ GetFileSize() kernel32.GetFileSize
@ stdcall -version=0x601+ GetFileSizeEx() kernel32.GetFileSizeEx
@ stdcall -version=0x601+ GetFileTime() kernel32.GetFileTime
@ stdcall -version=0x601+ GetFileType() kernel32.GetFileType
@ stdcall -version=0x601+ GetFinalPathNameByHandleA() kernel32.GetFinalPathNameByHandleA
@ stdcall -version=0x601+ GetFinalPathNameByHandleW() kernel32.GetFinalPathNameByHandleW
@ stdcall -version=0x601+ GetFullPathNameA() kernel32.GetFullPathNameA
@ stdcall -version=0x601+ GetFullPathNameW() kernel32.GetFullPathNameW
@ stdcall -version=0x601+ GetHandleInformation() kernel32.GetHandleInformation
@ stdcall -version=0x601+ GetKernelObjectSecurity() advapi32.GetKernelObjectSecurity
@ stdcall -version=0x601+ GetLastError() kernel32.GetLastError
@ stdcall -version=0x601+ GetLengthSid() ADVAPI32.GetLengthSid
@ stdcall -version=0x601+ GetLocalTime() kernel32.GetLocalTime
@ stdcall -version=0x601+ GetLocaleInfoA() kernel32.GetLocaleInfoA
@ stdcall -version=0x601+ GetLocaleInfoEx() kernel32.GetLocaleInfoEx
@ stub -version=0x601+ GetLocaleInfoHelper ;kernel32.GetLocaleInfoHelper
@ stdcall -version=0x601+ GetLocaleInfoW() kernel32.GetLocaleInfoW
@ stdcall -version=0x601+ GetLogicalDriveStringsW() kernel32.GetLogicalDriveStringsW
@ stdcall -version=0x601+ GetLogicalDrives() kernel32.GetLogicalDrives
@ stdcall -version=0x601+ GetLogicalProcessorInformation() kernel32.GetLogicalProcessorInformation
@ stub -version=0x601+ GetLogicalProcessorInformationEx() ;kernel32.GetLogicalProcessorInformationEx
@ stdcall -version=0x601+ GetLongPathNameA() kernel32.GetLongPathNameA
@ stdcall -version=0x601+ GetLongPathNameW() kernel32.GetLongPathNameW
@ stdcall -version=0x601+ GetModuleFileNameA() kernel32.GetModuleFileNameA
@ stdcall -version=0x601+ GetModuleFileNameW() kernel32.GetModuleFileNameW
@ stdcall -version=0x601+ GetModuleHandleA() kernel32.GetModuleHandleA
@ stdcall -version=0x601+ GetModuleHandleExA() kernel32.GetModuleHandleExA
@ stdcall -version=0x601+ GetModuleHandleExW() kernel32.GetModuleHandleExW
@ stdcall -version=0x601+ GetModuleHandleW() kernel32.GetModuleHandleW
@ stdcall -version=0x601+ GetNLSVersion() kernel32.GetNLSVersion
@ stdcall -version=0x601+ GetNLSVersionEx() kernel32.GetNLSVersionEx
@ stub -version=0x601+ GetNamedLocaleHashNode	 
@ stdcall -version=0x601+ GetNamedPipeAttribute() kernel32.GetNamedPipeAttribute
@ stdcall -version=0x601+ GetNamedPipeClientComputerNameW() kernel32.GetNamedPipeClientComputerNameW
@ stdcall -version=0x601+ GetNumberFormatEx	() kernel32.GetNumberFormatEx
@ stdcall -version=0x601+ GetNumberFormatW() kernel32.GetNumberFormatW
@ stdcall -version=0x601+ GetOEMCP() kernel32.GetOEMCP
@ stdcall -version=0x601+ GetOverlappedResult() kernel32.GetOverlappedResult
@ stdcall -version=0x601+ GetPriorityClass() kernel32.GetPriorityClass
@ stdcall -version=0x601+ GetPrivateObjectSecurity() ADVAPI32.GetPrivateObjectSecurity
@ stdcall -version=0x601+ GetProcAddress() kernel32.GetProcAddress
@ stdcall -version=0x601+ GetProcessHeap() kernel32.GetProcessHeap
@ stdcall -version=0x601+ GetProcessHeaps() kernel32.GetProcessHeaps
@ stdcall -version=0x601+ GetProcessId() kernel32.GetProcessId
@ stdcall -version=0x601+ GetProcessIdOfThread() kernel32.GetProcessIdOfThread
@ stub -version=0x601+ GetProcessPreferredUILanguages ;kernel32.GetProcessPreferredUILanguages
@ stdcall -version=0x601+ GetProcessTimes() kernel32.GetProcessTimes
@ stdcall -version=0x601+ GetProcessVersion() kernel32.GetProcessVersion
@ stub -version=0x601+ GetPtrCalData	 
@ stub -version=0x601+ GetPtrCalDataArray	 
@ stdcall -version=0x601+ GetQueuedCompletionStatus() kernel32.GetQueuedCompletionStatus
@ stdcall -version=0x601+ GetQueuedCompletionStatusEx() kernel32.GetQueuedCompletionStatusEx
@ stdcall -version=0x601+ GetSecurityDescriptorControl() ADVAPI32.GetSecurityDescriptorControl
@ stdcall -version=0x601+ GetSecurityDescriptorDacl() ADVAPI32.GetSecurityDescriptorDacl
@ stdcall -version=0x601+ GetSecurityDescriptorGroup() ADVAPI32.GetSecurityDescriptorGroup
@ stdcall -version=0x601+ GetSecurityDescriptorLength() ADVAPI32.GetSecurityDescriptorLength
@ stdcall -version=0x601+ GetSecurityDescriptorOwner() ADVAPI32.GetSecurityDescriptorOwner
@ stdcall -version=0x601+ GetSecurityDescriptorRMControl() ADVAPI32.GetSecurityDescriptorRMControl
@ stdcall -version=0x601+ GetSecurityDescriptorSacl() ADVAPI32.GetSecurityDescriptorSacl
@ stdcall -version=0x601+ GetShortPathNameW() kernel32.GetShortPathNameW
@ stdcall -version=0x601+ GetSidIdentifierAuthority() ADVAPI32.GetSidIdentifierAuthority
@ stdcall -version=0x601+ GetSidLengthRequired() ADVAPI32.GetSidLengthRequired
@ stdcall -version=0x601+ GetSidSubAuthority() ADVAPI32.GetSidSubAuthority
@ stdcall -version=0x601+ GetSidSubAuthorityCount() ADVAPI32.GetSidSubAuthorityCount
@ stdcall -version=0x601+ GetStartupInfoW() kernel32.GetStartupInfoW
@ stdcall -version=0x601+ GetStdHandle() kernel32.GetStdHandle
@ stub -version=0x601+ GetStringTableEntry	 
@ stdcall -version=0x601+ GetStringTypeA() kernel32.GetStringTypeA
@ stdcall -version=0x601+ GetStringTypeExW() kernel32.GetStringTypeExA
@ stdcall -version=0x601+ GetStringTypeW() kernel32.GetStringTypeExW
@ stdcall -version=0x601+ GetSystemDefaultLCID() kernel32.GetSystemDefaultLCID
@ stdcall -version=0x601+ GetSystemDefaultLangID() kernel32.GetSystemDefaultLangID
@ stdcall -version=0x601+ GetSystemDefaultLocaleName() kernel32.GetSystemDefaultLocaleName
@ stdcall -version=0x601+ GetSystemDefaultUILanguage() kernel32.GetSystemDefaultUILanguage
@ stdcall -version=0x601+ GetSystemDirectoryA() kernel32.GetSystemDirectoryA
@ stdcall -version=0x601+ GetSystemDirectoryW() kernel32.GetSystemDirectoryW
@ stdcall -version=0x601+ GetSystemInfo() kernel32.GetSystemInfo
@ stdcall -version=0x601+ GetSystemPreferredUILanguages() kernel32.GetSystemPreferredUILanguages
@ stdcall -version=0x601+ GetSystemTime() kernel32.GetSystemTime
@ stdcall -version=0x601+ GetSystemTimeAdjustment() kernel32.GetSystemTimeAdjustment
@ stdcall -version=0x601+ GetSystemTimeAsFileTime() kernel32.GetSystemTimeAsFileTime
@ stdcall -version=0x601+ GetSystemWindowsDirectoryA() kernel32.GetSystemWindowsDirectoryA
@ stdcall -version=0x601+ GetSystemWindowsDirectoryW() kernel32.GetSystemWindowsDirectoryW
@ stdcall -version=0x601+ GetTempFileNameW() kernel32.GetTempFileNameW
@ stdcall -version=0x601+ GetThreadId() kernel32.GetThreadId
@ stdcall -version=0x601+ GetThreadLocale() kernel32.GetThreadLocale
@ stdcall -version=0x601+ GetThreadPreferredUILanguages() kernel32.GetThreadPreferredUILanguages
@ stdcall -version=0x601+ GetThreadPriority() kernel32.GetThreadPriority
@ stdcall -version=0x601+ GetThreadPriorityBoost() kernel32.GetThreadPriorityBoost
@ stdcall -version=0x601+ GetThreadUILanguage() kernel32.GetThreadUILanguage
@ stdcall -version=0x601+ GetTickCount() kernel32.GetTickCount
@ stdcall -version=0x601+ GetTickCount64() kernel32.GetTickCount64
@ stdcall -version=0x601+ GetTimeZoneInformation() kernel32.GetTimeZoneInformation
@ stdcall -version=0x601+ GetTimeZoneInformationForYear() kernel32.GetTimeZoneInformationForYear
@ stdcall -version=0x601+ GetTokenInformation() ADVAPI32.GetTokenInformation
@ stdcall -version=0x601+ GetUILanguageInfo() kernel32.GetUILanguageInfo
@ stdcall -version=0x601+ GetUserDefaultLCID() kernel32.GetUserDefaultLCID
@ stdcall -version=0x601+ GetUserDefaultLangID() kernel32.GetUserDefaultLangID
@ stdcall -version=0x601+ GetUserDefaultLocaleName() kernel32.GetUserDefaultLocaleName
@ stdcall -version=0x601+ GetUserDefaultUILanguage() kernel32.GetUserDefaultUILanguage
@ stub -version=0x601+ GetUserInfo	 
@ stub -version=0x601+ GetUserInfoWord	 
@ stdcall -version=0x601+ GetUserPreferredUILanguages() kernel32.GetUserPreferredUILanguages
@ stdcall -version=0x601+ GetVersion() kernel32.GetVersion
@ stdcall -version=0x601+ GetVersionExA() kernel32.GetVersionExA
@ stdcall -version=0x601+ GetVersionExW() kernel32.GetVersionExW
@ stdcall -version=0x601+ GetVolumeInformationByHandleW() kernel32.GetVolumeInformationByHandleW
@ stdcall -version=0x601+ GetVolumeInformationW() kernel32.GetVolumeInformationW
@ stdcall -version=0x601+ GetVolumePathNameW() kernel32.GetVolumePathNameA
@ stdcall -version=0x601+ GetWindowsAccountDomainSid() kernel32.GetWindowsAccountDomainSid
@ stdcall -version=0x601+ GetWindowsDirectoryA() kernel32.GetWindowsDirectoryA
@ stdcall -version=0x601+ GetWindowsDirectoryW() kernel32.GetWindowsDirectoryW
@ stdcall -version=0x601+ GlobalAlloc() kernel32.GlobalAlloc
@ stdcall -version=0x601+ GlobalFree() kernel32.GlobalFree
@ stdcall -version=0x601+ GlobalMemoryStatusEx() kernel32.GlobalMemoryStatus
@ stdcall -version=0x601+ HeapAlloc() ntdll.RtlAllocateHeap;
@ stdcall -version=0x601+ HeapCompact() kernel32.HeapCompact
@ stdcall -version=0x601+ HeapCreate() kernel32.HeapCreate
@ stdcall -version=0x601+ HeapDestroy() kernel32.HeapDestroy
@ stdcall -version=0x601+ HeapFree() ntdll.RtlFreeHeap;
@ stdcall -version=0x601+ HeapLock() kernel32.HeapLock
@ stdcall -version=0x601+ HeapQueryInformation() kernel32.HeapQueryInformation
@ stdcall -version=0x601+ HeapReAlloc() ntdll.RtlReAllocateHeap;
@ stdcall -version=0x601+ HeapSetInformation() kernel32.HeapSetInformation
@ stdcall -version=0x601+ HeapSize() ntdll.RtlSizeHeap;
@ stdcall -version=0x601+ HeapSummary() kernel32.HeapSummary
@ stdcall -version=0x601+ HeapUnlock() kernel32.HeapUnlock
@ stdcall -version=0x601+ HeapValidate() kernel32.HeapValidate
@ stdcall -version=0x601+ HeapWalk() kernel32.HeapWalk
@ stdcall -version=0x601+ ImpersonateAnonymousToken() ADVAPI32.ImpersonateAnonymousToken
@ stdcall -version=0x601+ ImpersonateLoggedOnUser() ADVAPI32.ImpersonateLoggedOnUser
@ stdcall -version=0x601+ ImpersonateNamedPipeClient() ADVAPI32.ImpersonateNamedPipeClient
@ stdcall -version=0x601+ ImpersonateSelf() ADVAPI32.ImpersonateSelf
@ stdcall -version=0x601+ InitializeAcl() ADVAPI32.InitializeAcl
@ stdcall -version=0x601+ InitializeCriticalSection() ntdll.RtlInitializeCriticalSection; 
@ stdcall -version=0x601+ InitializeCriticalSectionAndSpinCount() kernel32.InitializeCriticalSectionAndSpinCount
@ stdcall -version=0x601+ InitializeCriticalSectionEx() kernel32.InitializeCriticalSectionEx
@ stdcall -version=0x601+ InitializeProcThreadAttributeList() kernel32.InitializeProcThreadAttributeList
@ stdcall -version=0x601+ InitializeSListHead() ntdll.RtlInitializeSListHead;
@ stdcall -version=0x601+ InitializeSRWLock() ntdll.RtlInitializeSRWLock;
@ stdcall -version=0x601+ InitializeSecurityDescriptor() ADVAPI32.InitializeSecurityDescriptor
@ stdcall -version=0x601+ InitializeSid() ADVAPI32.InitializeSid
;@ stub -arch=i386 -version=0x601+ InterlockedCompareExchange	
@ stdcall -arch=i386 -version=0x601+ InterlockedCompareExchange64() ntdll.RtlInterlockedCompareExchange64;
;@ stub -arch=i386 -version=0x601+ InterlockedDecrement
;@ stub -arch=i386 -version=0x601+ InterlockedExchange
;@ stub -arch=i386 -version=0x601+ InterlockedExchangeAdd
@ stdcall -version=0x601+ InterlockedFlushSList() ntdll.RtlInterlockedFlushSList;
@ stdcall -arch=i386 -version=0x601+ InterlockedIncrement() kernel32.InterlockedIncrement
@ stdcall -version=0x601+ InterlockedPopEntrySList() ntdll.RtlInterlockedPopEntrySList;
@ stdcall -version=0x601+ InterlockedPushEntrySList() ntdll.RtlInterlockedPushEntrySList;
@ stdcall -version=0x601+ InterlockedPushListSList() ntdll.RtlInterlockedPushListSList;
@ stub -version=0x601+ InternalLcidToName	 
@ stub -version=0x601+ Internal_EnumCalendarInfo	 
@ stub -version=0x601+ Internal_EnumDateFormats	 
@ stub -version=0x601+ Internal_EnumLanguageGroupLocales	 
@ stub -version=0x601+ Internal_EnumSystemCodePages	 
@ stub -version=0x601+ Internal_EnumSystemLanguageGroups	 
@ stub -version=0x601+ Internal_EnumSystemLocales	 
@ stub -version=0x601+ Internal_EnumTimeFormats	 
@ stub -version=0x601+ Internal_EnumUILanguages	 
@ stub -version=0x601+ InvalidateTzSpecificCache
@ stdcall -version=0x601+ IsDBCSLeadByte() kernel32.IsDBCSLeadByte
@ stdcall -version=0x601+ IsDBCSLeadByteEx() kernel32.IsDBCSLeadByteEx
@ stdcall -version=0x601+ IsDebuggerPresent() kernel32.IsDebuggerPresent
@ stdcall -version=0x601+ IsNLSDefinedString() kernel32.IsNLSDefinedString
@ stdcall -version=0x601+ IsProcessInJob() kernel32.IsProcessInJob
@ stdcall -version=0x601+ IsThreadpoolTimerSet() ntdll.TpIsTimerSet;
@ stdcall -version=0x601+ IsTokenRestricted() ADVAPI32.IsTokenRestricted
@ stdcall -version=0x601+ IsValidAcl() ADVAPI32.IsValidAcl
@ stdcall -version=0x601+ IsValidCodePage() kernel32.IsValidCodePage
@ stdcall -version=0x601+ IsValidLanguageGroup() kernel32.IsValidLocaleName
@ stdcall -version=0x601+ IsValidLocale() kernel32.IsValidLocale
@ stdcall -version=0x601+ IsValidLocaleName() kernel32.IsValidLocaleName
@ stub -version=0x601+ IsValidRelativeSecurityDescriptor ;ADVAPI32.IsValidRelativeSecurityDescriptor
@ stdcall -version=0x601+ IsValidSecurityDescriptor() ADVAPI32.IsValidSecurityDescriptor
@ stdcall -version=0x601+ IsValidSid() ADVAPI32.IsValidSid
@ stdcall -version=0x601+ IsWellKnownSid() ADVAPI32.IsWellKnownSid
@ stdcall -version=0x601+ IsWow64Process() kernel32.IsWow64Process
@ stub -version=0x601+ KernelBaseGetGlobalData	 
@ stdcall -version=0x601+ LCIDToLocaleName() kernel32.LCIDToLocaleName
@ stdcall -version=0x601+ LCMapStringA() kernel32.LCMapStringA
@ stdcall -version=0x601+ LCMapStringEx() kernel32.LCMapStringEx
@ stdcall -version=0x601+ LCMapStringW() kernel32.LCMapStringW
@ stdcall -version=0x601+ LeaveCriticalSection() ntdll.RtlLeaveCriticalSection;
@ stdcall -version=0x601+ LeaveCriticalSectionWhenCallbackReturns() ntdll.TpCallbackLeaveCriticalSectionOnCompletion;
@ stdcall -version=0x601+ LoadLibraryExA() kernel32.LoadLibraryExA
@ stdcall -version=0x601+ LoadLibraryExW() kernel32.LoadLibraryExW
@ stdcall -version=0x601+ LoadResource() kernel32.LoadResource
@ stub -version=0x601+ LoadStringA() ;user32.LoadStringA
@ stdcall -version=0x601+ LoadStringBaseExW() kernel32.LoadStringBaseExW
@ stdcall -stub -version=0x601+ LoadStringByReference(long wstr wstr wstr long wstr ptr)	 
@ stdcall -version=0x601+ LoadStringW() kernel32.LoadStringW ;user32.LoadStringW	;also exported from USER32 in versions not yet enumerated
@ stdcall -version=0x601+ LocalAlloc() kernel32.LocalAlloc
@ stdcall -version=0x601+ LocalFileTimeToFileTime() kernel32.LocalFileTimeToFileTime
@ stdcall -version=0x601+ LocalFree() kernel32.LocalFree
@ stdcall -version=0x601+ LocalLock() kernel32.LocalLock
@ stdcall -version=0x601+ LocalReAlloc() kernel32.LocalReAlloc
@ stdcall -version=0x601+ LocalUnlock() kernel32.LocalUnlock
@ stdcall -version=0x601+ LocaleNameToLCID() kernel32.LocaleNameToLCID
@ stdcall -version=0x601+ LockFile() kernel32.LockFile
@ stdcall -version=0x601+ LockFileEx() kernel32.LockFileEx
@ stdcall -version=0x601+ LockResource() kernel32.LockResource
@ stdcall -version=0x601+ MakeAbsoluteSD() ADVAPI32.MakeAbsoluteSD
@ stdcall -version=0x601+ MakeAbsoluteSD2() ADVAPI32.MakeAbsoluteSD2
@ stdcall -version=0x601+ MakeSelfRelativeSD() ADVAPI32.MakeSelfRelativeSD
@ stdcall -version=0x601+ MapGenericMask() ADVAPI32.MapGenericMask
@ stdcall -version=0x601+ MapViewOfFile	() kernel32.MapViewOfFile
@ stdcall -version=0x601+ MapViewOfFileEx() kernel32.MapViewOfFileEx
@ stdcall -version=0x601+ MapViewOfFileExNuma() kernel32.MapViewOfFileExNuma
@ stdcall -version=0x601+ MultiByteToWideChar() kernel32.MultiByteToWideChar
@ stdcall -version=0x601+ NeedCurrentDirectoryForExePathA() kernel32.NeedCurrentDirectoryForExePathA
@ stdcall -version=0x601+ NeedCurrentDirectoryForExePathW() kernel32.NeedCurrentDirectoryForExePathW
@ stdcall -version=0x601+ NlsCheckPolicy() kernel32.NlsCheckPolicy
@ stub -version=0x601+ NlsDispatchAnsiEnumProc	 
@ stdcall -version=0x601+ NlsEventDataDescCreate() kernel32.NlsEventDataDescCreate
@ stub -version=0x601+ NlsGetACPFromLocale	 
@ stdcall -version=0x601+ NlsGetCacheUpdateCount() kernel32.NlsGetCacheUpdateCount
@ stub -version=0x601+ NlsIsUserDefaultLocale	 
@ stdcall -version=0x601+ NlsUpdateLocale() kernel32.NlsUpdateLocale
@ stdcall -version=0x601+ NlsUpdateSystemLocale() kernel32.NlsUpdateSystemLocale
@ stub -version=0x601+ NlsValidateLocale	
@ stdcall -version=0x601+ NlsWriteEtwEvent() kernel32.NlsWriteEtwEvent
@ stub -version=0x601+ NotifyMountMgr() ;kernel32.NotifyMountMgr
@ stub -version=0x601+ NotifyRedirectedStringChange	 
@ stdcall -version=0x601+ ObjectCloseAuditAlarmW() ADVAPI32.ObjectCloseAuditAlarmW
@ stdcall -version=0x601+ ObjectDeleteAuditAlarmW() ADVAPI32.ObjectDeleteAuditAlarmW
@ stdcall -version=0x601+ ObjectOpenAuditAlarmW() ADVAPI32.ObjectOpenAuditAlarmW
@ stdcall -version=0x601+ ObjectPrivilegeAuditAlarmW() ADVAPI32.ObjectPrivilegeAuditAlarmW
@ stdcall -version=0x601+ OpenEventA() kernel32.OpenEventA
@ stdcall -version=0x601+ OpenEventW() kernel32.OpenEventW
@ stdcall -version=0x601+ OpenFileMappingW() kernel32.OpenFileMappingW
@ stdcall -version=0x601+ OpenMutexW() kernel32.OpenMutexW
@ stdcall -version=0x601+ OpenProcess() kernel32.OpenProcess
@ stdcall -version=0x601+ OpenProcessToken() ADVAPI32.OpenProcessToken
@ stub -version=0x601+ OpenRegKey	 
@ stdcall -version=0x601+ OpenSemaphoreW() kernel32.OpenSemaphoreW
@ stdcall OpenThread() kernel32.OpenThread
@ stdcall SetThreadPriority() kernel32.SetThreadPriority
@ stdcall SetThreadPriorityBoost() kernel32.SetThreadPriorityBoost
@ stdcall -version=0x601+ OpenThreadToken() ADVAPI32.OpenThreadToken
@ stdcall -version=0x601+ OpenWaitableTimerW() kernel32.OpenWaitableTimerW
@ stdcall -version=0x601+ OutputDebugStringA() kernel32.OutputDebugStringA
@ stdcall -version=0x601+ OutputDebugStringW() kernel32.OutputDebugStringW
@ stdcall -version=0x601+ PeekNamedPipe() kernel32.PeekNamedPipe
@ stdcall -version=0x601+ PostQueuedCompletionStatus() kernel32.PostQueuedCompletionStatus
@ stdcall -version=0x601+ PrivilegeCheck() ADVAPI32.PrivilegeCheck
@ stdcall -version=0x601+ PrivilegedServiceAuditAlarmW() ADVAPI32.PrivilegedServiceAuditAlarmW
@ stdcall -version=0x601+ ProcessIdToSessionId() kernel32.ProcessIdToSessionId
@ stdcall -version=0x601+ PulseEvent() kernel32.PulseEvent
@ stdcall -version=0x601+ QueryDepthSList() ntdll.RtlQueryDepthSList;
@ stdcall -version=0x601+ QueryDosDeviceW() kernel32.QueryDosDeviceW
@ stdcall -stub -version=0x601+ QueryPerformanceCounter()
@ stub -version=0x601+ QueryPerformanceFrequency() ;ntdll.RtlQueryPerformanceFrequency;
@ stdcall -version=0x601+ QueryProcessAffinityUpdateMode() kernel32.QueryProcessAffinityUpdateMode
@ stub -version=0x601+ QuerySecurityAccessMask() ;ADVAPI32.QuerySecurityAccessMask
@ stub -version=0x601+ QueryThreadpoolStackInformation	 
@ stdcall -version=0x601+ QueueUserAPC() kernel32.QueueUserAPC
@ stdcall -version=0x601+ RaiseException() kernel32.RaiseException
@ stdcall -version=0x601+ ReadFile() kernel32.ReadFile
@ stdcall -version=0x601+ ReadFileEx() kernel32.ReadFileEx
@ stdcall -version=0x601+ ReadFileScatter() kernel32.ReadFileScatter
@ stdcall SetThreadToken() advapi32.SetThreadToken
@ stdcall -version=0x601+ ReadProcessMemory() kernel32.ReadProcessMemory
@ stdcall -version=0x601+ RegisterWaitForSingleObjectEx() kernel32.RegisterWaitForSingleObjectEx
@ stdcall -version=0x601+ ReleaseMutex() kernel32.ReleaseMutex
@ stdcall -version=0x601+ ReleaseMutexWhenCallbackReturns() ntdll.TpCallbackReleaseMutexOnCompletion;
@ stdcall -version=0x601+ ReleaseSRWLockExclusive() ntdll.RtlReleaseSRWLockExclusive;
@ stdcall -version=0x601+ ReleaseSRWLockShared() ntdll.RtlReleaseSRWLockShared;
@ stdcall -version=0x601+ ReleaseSemaphore() kernel32.ReleaseSemaphore
@ stdcall -version=0x601+ ReleaseSemaphoreWhenCallbackReturns() ntdll.TpCallbackReleaseSemaphoreOnCompletion
@ stdcall -version=0x601+ RemoveDirectoryA() kernel32.RemoveDirectoryA
@ stdcall -version=0x601+ RemoveDirectoryW() kernel32.RemoveDirectoryW
@ stdcall -version=0x601+ ResetEvent() kernel32.ResetEvent
@ stdcall -stub -version=0x601+ ResolveLocaleName()
@ stdcall -version=0x601+ ResumeThread() kernel32.ResumeThread
@ stdcall -version=0x601+ RevertToSelf() advapi32.RevertToSelf
@ stdcall -version=0x601+ SearchPathW() kernel32.SearchPathW
@ stdcall -version=0x601+ SetAclInformation() ADVAPI32.SetAclInformation
@ stdcall -version=0x601+ SetCalendarInfoW() kernel32.SetCalendarInfoW
@ stdcall -version=0x601+ SetCriticalSectionSpinCount() ntdll.RtlSetCriticalSectionSpinCount
@ stdcall -version=0x601+ SetCurrentDirectoryA() kernel32.SetCurrentDirectoryA
@ stdcall -version=0x601+ SetCurrentDirectoryW() kernel32.SetCurrentDirectoryW
@ stdcall -version=0x601+ SetEndOfFile() kernel32.SetEndOfFile
@ stdcall -version=0x601+ SetEnvironmentStringsW() kernel32.SetEnvironmentStringsW
@ stdcall -version=0x601+ SetEnvironmentVariableA() kernel32.SetEnvironmentVariableA
@ stdcall -version=0x601+ SetEnvironmentVariableW() kernel32.SetEnvironmentVariableW
@ stdcall -version=0x601+ SetErrorMode() kernel32.SetErrorMode
@ stdcall -version=0x601+ SetEvent() kernel32.SetEvent
@ stdcall -version=0x601+ SetEventWhenCallbackReturns() ntdll.TpCallbackSetEventOnCompletion;
@ stdcall -version=0x601+ SetFileApisToANSI() kernel32.SetFileApisToANSI
@ stdcall -version=0x601+ SetFileApisToOEM() kernel32.SetFileApisToOEM
@ stdcall -version=0x601+ TerminateProcess() kernel32.TerminateProcess
@ stdcall -version=0x601+ TerminateThread() kernel32.TerminateThread
@ stdcall -version=0x601+ TlsAlloc() kernel32.TlsAlloc
@ stdcall -version=0x601+ TlsFree() kernel32.TlsFree
@ stdcall -version=0x601+ TlsGetValue() kernel32.TlsGetValue
@ stdcall -version=0x601+ TlsSetValue() kernel32.TlsSetValue
@ stdcall -version=0x601+ TransactNamedPipe() kernel32.TransactNamedPipe
@ stdcall -stub -version=0x601+ TryAcquireSRWLockExclusive() ;ntdll.RtlTryAcquireSRWLockExclusive
@ stdcall -stub -version=0x601+ TryAcquireSRWLockShared() ;ntdll.RtlTryAcquireSRWLockShared
@ stdcall -version=0x601+ TryEnterCriticalSection() ntdll.RtlTryEnterCriticalSection
@ stdcall -version=0x601+ TrySubmitThreadpoolCallback() kernel32.TrySubmitThreadpoolCallback
@ stdcall -version=0x601+ TzSpecificLocalTimeToSystemTime() kernel32.TzSpecificLocalTimeToSystemTime
@ stdcall -version=0x601+ UnlockFile() kernel32.UnlockFile
@ stdcall -version=0x601+ UnlockFileEx() kernel32.UnlockFileEx
@ stdcall -version=0x601+ UnmapViewOfFile() kernel32.UnmapViewOfFile
@ stdcall -version=0x601+ UnregisterWaitEx() kernel32.UnregisterWaitEx
@ stdcall -version=0x601+ UpdateProcThreadAttribute() kernel32.UpdateProcThreadAttribute
@ stdcall -version=0x601+ VerLanguageNameA() kernel32.VerLanguageNameA
@ stdcall -version=0x601+ VerLanguageNameW() kernel32.VerLanguageNameW
@ stdcall -version=0x601+ VirtualAlloc() kernel32.VirtualAlloc
@ stdcall -version=0x601+ VirtualAllocEx() kernel32.VirtualAllocEx
@ stdcall -version=0x601+ VirtualAllocExNuma() kernel32.VirtualAllocExNuma
@ stdcall -version=0x601+ VirtualFree() kernel32.VirtualFree
@ stdcall -version=0x601+ VirtualFreeEx() kernel32.VirtualFreeEx
@ stdcall -version=0x601+ VirtualProtect() kernel32.VirtualProtect
@ stdcall -version=0x601+ VirtualProtectEx() kernel32.VirtualProtectEx
@ stdcall -version=0x601+ VirtualQuery() kernel32.VirtualQuery
@ stdcall -version=0x601+ VirtualQueryEx() kernel32.VirtualQueryEx
@ stdcall -version=0x601+ WaitForMultipleObjectsE() kernel32.WaitForMultipleObjectsEx
@ stdcall -version=0x601+ WaitForSingleObject() kernel32.WaitForSingleObject
@ stdcall -version=0x601+ WaitForSingleObjectEx() kernel32.WaitForSingleObjectEx
@ stdcall -version=0x601+ WaitForThreadpoolIoCallbacks() ntdll.TpWaitForIoCompletion;
@ stdcall -version=0x601+ WaitForThreadpoolTimerCallbacks() ntdll.TpWaitForTimer;
@ stdcall -version=0x601+ WaitForThreadpoolWaitCallbacks() ntdll.TpWaitForWait;
@ stdcall -version=0x601+ WaitForThreadpoolWorkCallbacks() ntdll.TpWaitForWork;
@ stdcall -version=0x601+ WaitNamedPipeW() kernel32.WaitNamedPipeW
@ stdcall -version=0x601+ WideCharToMultiByte() kernel32.WideCharToMultiByte
@ stdcall -version=0x601+ Wow64DisableWow64FsRedirection() kernel32.Wow64DisableWow64FsRedirection
@ stdcall -version=0x601+ Wow64RevertWow64FsRedirection() kernel32.Wow64RevertWow64FsRedirection
@ stdcall -version=0x601+ WriteFile() kernel32.WriteFile
@ stdcall -version=0x601+ WriteFileEx() kernel32.WriteFileEx
@ stdcall -version=0x601+ WriteFileGather()	kernel32.WriteFileGather
@ stdcall -version=0x601+ WriteProcessMemory() kernel32.WriteProcessMemory
@ stdcall -arch=x86_64 -version=0x601+ __C_specific_handler() kernel32.__C_specific_handler
@ stdcall -arch=x86_64 -version=0x601+ __chkstk() kernel32.__chkstk
@ stdcall -arch=x86_64 -version=0x601+ __misaligned_access() kernel32.__misaligned_access
@ stdcall -arch=x86_64 -version=0x601+ _local_unwind() kernel32._local_unwind
@ stdcall -version=0x601+ lstrcmp() kernel32.lstrcmp
@ stdcall -version=0x601+ lstrcmpA() kernel32.lstrcmpA
@ stdcall -version=0x601+ lstrcmpW() kernel32.lstrcmpW 
@ stdcall -version=0x601+ lstrcmpi() kernel32.lstrcmpi 
@ stdcall -version=0x601+ lstrcmpiA() kernel32.lstrcmpiA 
@ stdcall -version=0x601+ lstrcmpiW() kernel32.lstrcmpiW 
@ stdcall -version=0x601+ lstrcpyn() kernel32.lstrcpyn 
@ stdcall -version=0x601+ lstrcpynA() kernel32.lstrcpynA
@ stdcall -version=0x601+ lstrcpynW() kernel32.lstrcpynW 
@ stdcall -version=0x601+ lstrlen() kernel32.lstrlen 
@ stdcall -version=0x601+ lstrlenA() kernel32.lstrlenA 
@ stdcall -version=0x601+ lstrlenW() kernel32.lstrlenW
 
;Windows 8 stuff (Unchecked)
;@ stdcall -version=0x602+ ActivateActCt() kernel32.ActivateActCt
;@ stub -version=0x602+ AddRefActCtx	
;@ stub -version=0x602+ AddResourceAttributeAce	
;@ stub -version=0x602+ AddSIDToBoundaryDescriptor	
;@ stub -version=0x602+ AddScopedPolicyIDAce	
;@ stub -version=0x602+ AdjustTokenDeviceClaims	
;@ stub -version=0x602+ AdjustTokenDeviceGroups	
;@ stub -version=0x602+ AdjustTokenUserClaims	
;@ stub -version=0x602+ AllocConsole	
;@ stub -version=0x602+ AttachConsole	
;@ stub -version=0x602+ BaseCheckAppcompatCache	
;@ stub -version=0x602+ BaseCheckAppcompatCacheEx	
;@ stub -version=0x602+ BaseCleanupAppcompatCacheSupport	
;@ stub -version=0x602+ BaseDumpAppcompatCache	
;@ stub -version=0x602+ BaseFlushAppcompatCache	
;@ stub -version=0x602+ BaseInitAppcompatCacheSupport	
;@ stub -version=0x602+ BaseIsAppcompatInfrastructureDisabled	
;@ stub -version=0x602+ BaseMarkFileForDelete	
;@ stub -version=0x602+ BaseUpdateAppcompatCache	
;@ stub -version=0x602+ BasepAdjustObjectAttributesForPrivateNamespace	
;@ stub -version=0x602+ BasepCopyFileCallback	
;@ stub -version=0x602+ BasepCopyFileExW	
;@ stub -version=0x602+ BasepNotifyTrackingService	
;@ stub -version=0x602+ CLOSE_LOCAL_HANDLE_INTERNAL	
;@ stub -version=0x602+ CallbackDetectedUnrecoverableError	
;@ stub -version=0x602+ CancelIo	
@ stdcall -version=0x602+ CancelSynchronousIo() kernel32.CancelSynchronousIo
@ stdcall -stub -version=0x602+ CharLowerA	()
@ stdcall -stub -version=0x602+ CharLowerBuffA	()
@ stdcall -stub -version=0x602+ CharLowerBuffW	()
@ stdcall -stub -version=0x602+ CharLowerW	()
@ stdcall -stub -version=0x602+ CharNextA	()
@ stdcall -stub -version=0x602+ CharNextExA	()
@ stdcall -stub -version=0x602+ CharNextW	()
@ stdcall -stub -version=0x602+ CharPrevA	()
@ stdcall -stub -version=0x602+ CharPrevExA	()
@ stdcall -stub -version=0x602+ CharPrevW	()
@ stdcall -stub -version=0x602+ CharUpperA	()
@ stdcall -stub -version=0x602+ CharUpperBuffA	()
@ stdcall -stub -version=0x602+ CharUpperBuffW	()
@ stdcall -stub -version=0x602+ CharUpperW	()
@ stdcall -stub -version=0x602+ CheckTokenCapability	()
@ stdcall -stub -version=0x602+ CheckTokenMembershipEx	()
@ stdcall -stub -version=0x602+ ChrCmpIA	()
@ stdcall -stub -version=0x602+ ChrCmpIW	()
@ stdcall -stub -version=0x602+ ClearCommBreak	()
@ stdcall -stub -version=0x602+ ClearCommError	()
@ stdcall -stub -version=0x602+ ClosePrivateNamespace	()
@ stdcall -stub -version=0x602+ CloseTrace	()
@ stdcall -stub -version=0x602+ ContinueDebugEvent	()
@ stdcall -stub -version=0x602+ ControlTraceA	()
@ stdcall -stub -version=0x602+ ControlTraceW	()
@ stdcall -stub -version=0x602+ CopyFileExW	()
@ stdcall -stub -version=0x602+ CreateActCtxW	()
@ stdcall -stub -version=0x602+ CreateBoundaryDescriptorW	()
@ stdcall -stub -version=0x602+ CreateConsoleScreenBuffer	()
@ stdcall -stub -version=0x602+ CreateDirectoryExW	()
@ stdcall -stub -version=0x602+ CreateFile2	()
@ stdcall -stub -version=0x602+ CreateHardLinkW	()
@ stdcall -stub -version=0x602+ CreateMemoryResourceNotification	()
@ stdcall -stub -version=0x602+ CreatePrivateNamespaceW	()
@ stdcall -stub -version=0x602+ CreateProcessA	()
@ stdcall CreateProcessAsUserW() advapi32.CreateProcessAsUserW
;@stdcall - stub -version=0x602+ CreateProcessAsUserW	()
@ stdcall -stub -version=0x602+ CreateProcessInternalA	()
@ stdcall -stub -version=0x602+ CreateProcessInternalW	()
@ stdcall    CreateProcessW	() kernel32.CreateProcessW
@ stdcall -stub -version=0x602+ CreateSymbolicLinkW	()
@ stdcall -stub -version=0x602+ CtrlRoutine	()
@ stdcall -stub -version=0x602+ DeactivateActCtx	()
@ stdcall -stub -version=0x602+ DebugActiveProcess	()
@ stdcall -stub -version=0x602+ DebugActiveProcessStop	()
@ stdcall -stub -version=0x602+ DelayLoadFailureHook	()
@ stdcall -stub -version=0x602+ DelayLoadFailureHookLookup	()
@ stdcall -stub -version=0x602+ DeleteBoundaryDescriptor	()
@ stdcall -stub -version=0x602+ DeleteSynchronizationBarrier	()
@ stdcall -stub -version=0x602+ DisablePredefinedHandleTableInternal	()
@ stdcall -stub -version=0x602+ DsBindWithSpnExW	()
@ stdcall -stub -version=0x602+ DsCrackNamesW	()
@ stdcall -stub -version=0x602+ DsFreeDomainControllerInfoW	()
@ stdcall -stub -version=0x602+ DsFreeNameResultW	()
@ stdcall -stub -version=0x602+ DsFreePasswordCredentials	()
@ stdcall -stub -version=0x602+ DsGetDomainControllerInfoW	()
@ stdcall -stub -version=0x602+ DsMakePasswordCredentialsW	()
@ stdcall -stub -version=0x602+ DsUnBindW	()
@ stdcall -stub -version=0x602+ EmptyWorkingSet	()
@ stdcall -stub -version=0x602+ EnableTraceEx2	()
@ stdcall -stub -version=0x602+ EnterCriticalPolicySectionInternal	()
@ stdcall -stub -version=0x602+ EnterSynchronizationBarrier	()
@ stdcall -stub -version=0x602+ EnumDeviceDrivers	()
@ stdcall -stub -version=0x602+ EnumDynamicTimeZoneInformation	()
@ stdcall -stub -version=0x602+ EnumPageFilesA	()
@ stdcall -stub -version=0x602+ EnumPageFilesW	()
@ stdcall -stub -version=0x602+ EnumProcessModules	()
@ stdcall -stub -version=0x602+ EnumProcessModulesEx	()
@ stdcall -stub -version=0x602+ EnumProcesses	()
@ stdcall -stub -version=0x602+ EnumResourceLanguagesExA	()
@ stdcall -stub -version=0x602+ EnumResourceLanguagesExW	()
@ stdcall -stub -version=0x602+ EnumResourceNamesExA	()
@ stdcall -stub -version=0x602+ EnumResourceNamesExW	()
@ stdcall -stub -version=0x602+ EnumResourceTypesExA	()
@ stdcall -stub -version=0x602+ EnumResourceTypesExW	()
@ stdcall -stub -version=0x602+ EnumSystemFirmwareTables	()
@ stdcall -stub -version=0x602+ EnumerateTraceGuidsEx	()
@ stdcall -stub -version=0x602+ EnumerateTraces	()
@ stdcall -stub -version=0x602+ EscapeCommFunction	()
@ stdcall -stub -version=0x602+ EventAccessControl	()
@ stdcall -stub -version=0x602+ EventAccessQuery	()
@ stdcall -stub -version=0x602+ EventAccessRemove	()
@ stdcall -stub -version=0x602+ EventActivityIdControl	()
@ stdcall -stub -version=0x602+ EventEnabled	()
@ stdcall -stub -version=0x602+ EventProviderEnabled	()
@ stdcall -stub -version=0x602+ EventRegister	()
@ stdcall -stub -version=0x602+ EventSetInformation	()
@ stdcall -stub -version=0x602+ EventUnregister	()
@ stdcall -stub -version=0x602+ EventWrite	()
@ stdcall -stub -version=0x602+ EventWriteEx	()
@ stdcall -stub -version=0x602+ EventWriteString	()
@ stdcall -stub -version=0x602+ EventWriteTransfer	()
@ stdcall -stub -version=0x602+ FillConsoleOutputAttribute	()
@ stdcall -stub -version=0x602+ FillConsoleOutputCharacterA	()
@ stdcall -stub -version=0x602+ FillConsoleOutputCharacterW	()
@ stdcall -stub -version=0x602+ FindActCtxSectionGuid	()
@ stdcall -stub -version=0x602+ FindActCtxSectionStringW	()
@ stdcall -stub -version=0x602+ FlushConsoleInputBuffer	()
@ stdcall -stub -version=0x602+ FlushInstructionCache	()
@ stdcall -stub -version=0x602+ ForceSyncFgPolicyInternal	()
@ stdcall -stub -version=0x602+ FreeConsole	()
@ stdcall -stub -version=0x602+ FreeGPOListInternalA	()
@ stdcall -stub -version=0x602+ FreeGPOListInternalW	()
@ stdcall -stub -version=0x602+ GenerateConsoleCtrlEvent	()
@ stdcall -stub -version=0x602+ GenerateGPNotificationInternal	()
@ stdcall -stub -version=0x602+ GetAcceptLanguagesA	()
@ stdcall -stub -version=0x602+ GetAcceptLanguagesW	()
@ stdcall -stub -version=0x602+ GetAdjustObjectAttributesForPrivateNamespaceRoutine	()
@ stdcall -stub -version=0x602+ GetAppContainerAce	()
@ stdcall -stub -version=0x602+ GetAppContainerNamedObjectPath	()
@ stdcall -stub -version=0x602+ GetApplicationRecoveryCallback	()
@ stdcall -stub -version=0x602+ GetApplicationRestartSettings	()
@ stdcall -stub -version=0x602+ GetAppliedGPOListInternalA	()
@ stdcall -stub -version=0x602+ GetAppliedGPOListInternalW	()
@ stdcall -stub -version=0x602+ GetCommConfig	()
@ stdcall -stub -version=0x602+ GetCommMask	()
@ stdcall -stub -version=0x602+ GetCommModemStatus	()
@ stdcall -stub -version=0x602+ GetCommProperties	()
@ stdcall -stub -version=0x602+ GetCommState	()
@ stdcall -stub -version=0x602+ GetCommTimeouts	()
@ stdcall -stub -version=0x602+ GetConsoleCP	()
@ stdcall -stub -version=0x602+ GetConsoleCursorInfo	()
@ stdcall -stub -version=0x602+ GetConsoleInputExeNameA	()
@ stdcall -stub -version=0x602+ GetConsoleInputExeNameW	()
@ stdcall -stub -version=0x602+ GetConsoleMode	()
@ stdcall -stub -version=0x602+ GetConsoleOutputCP	()
@ stdcall -stub -version=0x602+ GetConsoleScreenBufferInfo	()
@ stdcall -stub -version=0x602+ GetConsoleScreenBufferInfoEx	()
@ stdcall -stub -version=0x602+ GetConsoleTitleW	()
@ stdcall -stub -version=0x602+ GetCurrentActCtx	()
@ stdcall -stub -version=0x602+ GetCurrentThreadStackLimits	()
@ stdcall -stub -version=0x602+ GetDateFormatA	()
@ stdcall -stub -version=0x602+ GetDateFormatEx	()
@ stdcall -stub -version=0x602+ GetDateFormatW	()
@ stdcall -stub -version=0x602+ GetDeviceDriverBaseNameA	()
@ stdcall -stub -version=0x602+ GetDeviceDriverBaseNameW	()
@ stdcall -stub -version=0x602+ GetDeviceDriverFileNameA	()
@ stdcall -stub -version=0x602+ GetDeviceDriverFileNameW	()
@ stdcall -stub -version=0x602+ GetDynamicTimeZoneInformationEffectiveYears	()
@ stdcall -stub -version=0x602+ GetEightBitStringToUnicodeSizeRoutine	()
@ stdcall -stub -version=0x602+ GetEightBitStringToUnicodeStringRoutine	()
@ stdcall -stub -version=0x602+ GetFileInformationByHandleEx	()
@ stdcall -stub -version=0x602+ GetFileVersionInfoByHandle	()
@ stdcall -stub -version=0x602+ GetFileVersionInfoExA	()
@ stdcall -stub -version=0x602+ GetFileVersionInfoExW	()
@ stdcall -stub -version=0x602+ GetFileVersionInfoSizeExA	()
@ stdcall -stub -version=0x602+ GetFileVersionInfoSizeExW	()
@ stdcall -stub -version=0x602+ GetGPOListInternalA	()
@ stdcall -stub -version=0x602+ GetGPOListInternalW	()
@ stdcall -stub -version=0x602+ GetLargePageMinimum	()
@ stdcall -stub -version=0x602+ GetLargestConsoleWindowSize	()
@ stdcall -stub -version=0x602+ GetMappedFileNameA	()
@ stdcall -stub -version=0x602+ GetMappedFileNameW	()
@ stdcall -stub -version=0x602+ GetModuleBaseNameA	()
@ stdcall -stub -version=0x602+ GetModuleBaseNameW	()
@ stdcall -stub -version=0x602+ GetModuleFileNameExA	()
@ stdcall -stub -version=0x602+ GetModuleFileNameExW	()
@ stdcall -stub -version=0x602+ GetModuleInformation	()
@ stdcall -stub -version=0x602+ GetNativeSystemInfo	()
@ stdcall -stub -version=0x602+ GetNativeSystemPageSize	()
@ stdcall -stub -version=0x602+ GetNextFgPolicyRefreshInfoInternal	()
@ stdcall -stub -version=0x602+ GetNumaHighestNodeNumber	()
@ stdcall -stub -version=0x602+ GetNumaNodeProcessorMaskEx	()
@ stdcall -stub -version=0x602+ GetNumberOfConsoleInputEvents	()
@ stdcall -stub -version=0x602+ GetOsSafeBootMode	()
@ stdcall -stub -version=0x602+ GetOverlappedResultEx	()
@ stdcall -stub -version=0x602+ GetPerformanceInfo	()
@ stdcall -stub -version=0x602+ GetPreviousFgPolicyRefreshInfoInternal	()
@ stdcall -stub -version=0x602+ GetProcAddressForCaller	()
@ stdcall -stub -version=0x602+ GetProcessGroupAffinity	()
@ stdcall -stub -version=0x602+ GetProcessImageFileNameA	()
@ stdcall -stub -version=0x602+ GetProcessImageFileNameW	()
@ stdcall -stub -version=0x602+ GetProcessMemoryInfo	()
@ stdcall -stub -version=0x602+ GetProcessWorkingSetSizeEx	()
@ stdcall -stub -version=0x602+ GetProductInfo	()
@ stdcall -stub -version=0x602+ GetRegistryExtensionFlags	()
@ stdcall -stub -version=0x602+ GetSystemFileCacheSize	()
@ stdcall -stub -version=0x602+ GetSystemFirmwareTable	()
@ stdcall -stub -version=0x602+ GetSystemPageSize	()
@ stdcall -stub -version=0x602+ GetSystemTimePreciseAsFileTime	()
@ stdcall -stub -version=0x602+ GetTempPathW	()
@ stdcall -stub -version=0x602+ GetThreadContext	()
@ stdcall -stub -version=0x602+ GetThreadGroupAffinity	()
@ stub -version=0x602+ GetThreadIdealProcessorEx	
@ stdcall -stub -version=0x601+ GetThreadTimes	()
@ stdcall -stub -version=0x601+ GetTimeFormatA	()
@ stdcall -stub -version=0x601+ GetTimeFormatEx	()
@ stdcall -stub -version=0x601+ GetTimeFormatW	()
@ stdcall -stub -version=0x601+ GetTraceEnableFlags	()
@ stdcall -stub -version=0x601+ GetTraceEnableLevel	()
@ stdcall -stub -version=0x601+ GetTraceLoggerHandle	()
@ stdcall -stub -version=0x601+ GetUnicodeStringToEightBitSizeRoutine	()
@ stdcall -stub -version=0x601+ GetUnicodeStringToEightBitStringRoutine	()
@ stdcall -stub -version=0x601+ GetVolumeNameForVolumeMountPointW	()
@ stdcall -stub -version=0x601+ GetVolumePathNamesForVolumeNameW	()
@ stdcall -stub -version=0x601+ GetWriteWatch	()
@ stdcall -stub -version=0x601+ GetWsChanges	()
@ stdcall -stub -version=0x601+ GetWsChangesEx	()
@ stdcall -stub -version=0x601+ HashData	()
@ stdcall -stub -version=0x601+ IdnToAscii	()
@ stdcall -stub -version=0x601+ IdnToUnicode	()
@ stdcall -stub -version=0x601+ InitOnceBeginInitialize	()
@ stdcall -stub -version=0x601+ InitOnceComplete	()
@ stdcall -stub -version=0x601+ InitOnceExecuteOnce	()
@ stdcall -stub -version=0x601+ InitOnceInitialize	()
@ stdcall -stub -version=0x601+ InitializeConditionVariable	()
@ stdcall -stub -version=0x601+ InitializeProcessForWsWatch	()
@ stdcall -stub -version=0x601+ InitializeSynchronizationBarrier	()
@ stdcall -stub -version=0x601+ InternetTimeFromSystemTimeA	()
@ stdcall -stub -version=0x601+ InternetTimeFromSystemTimeW	()
@ stdcall -stub -version=0x601+ InternetTimeToSystemTimeA	()
@ stdcall -stub -version=0x601+ InternetTimeToSystemTimeW	()
@ stdcall -stub -version=0x601+ IsCharAlphaA	()
@ stdcall -stub -version=0x601+ IsCharAlphaNumericA	()
@ stdcall -stub -version=0x601+ IsCharAlphaNumericW	()
@ stdcall -stub -version=0x601+ IsCharAlphaW	()
@ stdcall -stub -version=0x601+ IsCharBlankW	()
@ stdcall -stub -version=0x601+ IsCharCntrlW	()
@ stdcall -stub -version=0x601+ IsCharDigitW	()
@ stdcall -stub -version=0x601+ IsCharLowerA	()
@ stdcall -stub -version=0x601+ IsCharLowerW	()
@ stdcall -stub -version=0x601+ IsCharPunctW	()
@ stdcall -stub -version=0x601+ IsCharSpaceA	()
@ stdcall -stub -version=0x601+ IsCharSpaceW	()
@ stdcall -stub -version=0x601+ IsCharUpperA	()
@ stdcall -stub -version=0x601+ IsCharUpperW	()
@ stdcall -stub -version=0x601+ IsCharXDigitW	()
@ stdcall -stub -version=0x601+ IsInternetESCEnabled	()
@ stdcall -stub -version=0x601+ IsProcessorFeaturePresent	()
@ stdcall -stub -version=0x601+ IsSyncForegroundPolicyRefresh	()
@ stdcall -stub -version=0x601+ IsThreadAFiber	()
@ stdcall -stub -version=0x601+ IsTimeZoneRedirectionEnabled	()
@ stdcall -stub -version=0x601+ IsValidNLSVersion	()
@ stdcall -stub -version=0x601+ LeaveCriticalPolicySectionInternal	()
@ stdcall -stub -version=0x601+ LoadAppInitDlls	()
@ stdcall -stub -version=0x601+ MapPredefinedHandleInternal	()
@ stdcall -stub -version=0x601+ MoveFileExW	()
@ stdcall -stub -version=0x601+ MoveFileWithProgressTransactedW	()
@ stdcall -stub -version=0x601+ MoveFileWithProgressW	()
@ stdcall -stub -version=0x601+ OpenPrivateNamespaceW	()
@ stdcall -stub -version=0x601+ OpenTraceW	()
@ stdcall -stub -version=0x601+ ParseURLA	()
@ stdcall -stub -version=0x601+ ParseURLW	()
@ stdcall -stub -version=0x601+ PathAddBackslashA	()
@ stdcall -stub -version=0x601+ PathAddBackslashW	()
@ stdcall -stub -version=0x601+ PathAddExtensionA	()
@ stdcall -stub -version=0x601+ PathAddExtensionW	()
@ stdcall -stub -version=0x601+ PathAllocCanonicalize	()
@ stdcall -stub -version=0x601+ PathAllocCombine	()
@ stdcall -stub -version=0x601+ PathAppendA	()
@ stdcall -stub -version=0x601+ PathAppendW	()
@ stdcall -stub -version=0x601+ PathCanonicalizeA	()
@ stdcall -stub -version=0x601+ PathCanonicalizeW	()
@ stdcall -stub -version=0x601+ PathCchAddBackslash	()
@ stdcall -stub -version=0x601+ PathCchAddBackslashEx	()
@ stdcall -stub -version=0x601+ PathCchAddExtension	()
@ stdcall -stub -version=0x601+ PathCchAppend	()
@ stdcall -stub -version=0x601+ PathCchAppendEx	()
@ stdcall -stub -version=0x601+ PathCchCanonicalize	()
@ stdcall -stub -version=0x601+ PathCchCanonicalizeEx	()
@ stdcall -stub -version=0x601+ PathCchCombine	()
@ stdcall -stub -version=0x601+ PathCchCombineEx	()
@ stdcall -stub -version=0x601+ PathCchFindExtension	()
@ stdcall -stub -version=0x601+ PathCchIsRoot	()
@ stdcall -stub -version=0x601+ PathCchRemoveBackslash	()
@ stdcall -stub -version=0x601+ PathCchRemoveBackslashEx	()
@ stdcall -stub -version=0x601+ PathCchRemoveExtension	()
@ stdcall -stub -version=0x601+ PathCchRemoveFileSpec	()
@ stdcall -stub -version=0x601+ PathCchRenameExtension	()
@ stdcall -stub -version=0x601+ PathCchSkipRoot	()
@ stdcall -stub -version=0x601+ PathCchStripPrefix	()
@ stdcall -stub -version=0x601+ PathCchStripToRoot	()
@ stdcall -stub -version=0x601+ PathCombineA	()
@ stdcall -stub -version=0x601+ PathCombineW	()
@ stdcall -stub -version=0x601+ PathCommonPrefixA	()
@ stdcall -stub -version=0x601+ PathCommonPrefixW	()
@ stdcall -stub -version=0x601+ PathCreateFromUrlA	()
@ stdcall -stub -version=0x601+ PathCreateFromUrlAlloc	()
@ stdcall -stub -version=0x601+ PathCreateFromUrlW	()
@ stdcall -stub -version=0x601+ PathFileExistsA	()
@ stdcall -stub -version=0x601+ PathFileExistsW	()
@ stdcall -stub -version=0x601+ PathFindExtensionA	()
@ stdcall -stub -version=0x601+ PathFindExtensionW	()
@ stdcall -stub -version=0x601+ PathFindFileNameA	()
@ stdcall -stub -version=0x601+ PathFindFileNameW	()
@ stdcall -stub -version=0x601+ PathFindNextComponentA	()
@ stdcall -stub -version=0x601+ PathFindNextComponentW	()
@ stdcall -stub -version=0x601+ PathGetCharTypeA	()
@ stdcall -stub -version=0x601+ PathGetCharTypeW	()
@ stdcall -stub -version=0x601+ PathGetDriveNumberA	()
@ stdcall -stub -version=0x601+ PathGetDriveNumberW	()
@ stdcall -stub -version=0x601+ PathIsFileSpecA	()
@ stdcall -stub -version=0x601+ PathIsFileSpecW	()
@ stdcall -stub -version=0x601+ PathIsLFNFileSpecA	()
@ stdcall -stub -version=0x601+ PathIsLFNFileSpecW	()
@ stdcall -stub -version=0x601+ PathIsPrefixA	()
@ stdcall -stub -version=0x601+ PathIsPrefixW	()
@ stdcall -stub -version=0x601+ PathIsRelativeA	()
@ stdcall -stub -version=0x601+ PathIsRelativeW	()
@ stdcall -stub -version=0x601+ PathIsRootA	()
@ stdcall -stub -version=0x601+ PathIsRootW	()
@ stdcall -stub -version=0x601+ PathIsSameRootA	()
@ stdcall -stub -version=0x601+ PathIsSameRootW	()
@ stdcall -stub -version=0x601+ PathIsUNCA	()
@ stdcall -stub -version=0x601+ PathIsUNCEx	()
@ stdcall -stub -version=0x601+ PathIsUNCServerA	()
@ stdcall -stub -version=0x601+ PathIsUNCServerShareA	()
@ stdcall -stub -version=0x601+ PathIsUNCServerShareW	()
@ stdcall -stub -version=0x601+ PathIsUNCServerW	()
@ stdcall -stub -version=0x601+ PathIsUNCW	()
@ stdcall -stub -version=0x601+ PathIsURLA	()
@ stdcall -stub -version=0x601+ PathIsURLW	()
@ stdcall -stub -version=0x601+ PathIsValidCharA	()
@ stdcall -stub -version=0x601+ PathIsValidCharW	()
@ stdcall -stub -version=0x601+ PathMatchSpecA	()
@ stdcall -stub -version=0x601+ PathMatchSpecExA	()
@ stdcall -stub -version=0x601+ PathMatchSpecExW	()
@ stdcall -stub -version=0x601+ PathMatchSpecW	()
@ stdcall -stub -version=0x601+ PathParseIconLocationA	()
@ stdcall -stub -version=0x601+ PathParseIconLocationW	()
@ stdcall -stub -version=0x601+ PathRelativePathToA	()
@ stdcall -stub -version=0x601+ PathRelativePathToW	()
@ stdcall -stub -version=0x601+ PathRemoveBackslashA	()
@ stdcall -stub -version=0x601+ PathRemoveBackslashW	()
@ stdcall -stub -version=0x601+ PathRemoveBlanksA	()
@ stdcall -stub -version=0x601+ PathRemoveBlanksW	()
@ stdcall -stub -version=0x601+ PathRemoveExtensionA	()
@ stdcall -stub -version=0x601+ PathRemoveExtensionW	()
@ stdcall -stub -version=0x601+ PathRemoveFileSpecA	()
@ stdcall -stub -version=0x601+ PathRemoveFileSpecW	()
@ stdcall -stub -version=0x601+ PathRenameExtensionA	()
@ stdcall -stub -version=0x601+ PathRenameExtensionW	()
@ stdcall -stub -version=0x601+ PathSkipRootA	()
@ stdcall -stub -version=0x601+ PathSkipRootW	()
@ stdcall -stub -version=0x601+ PathStripPathA	()
@ stdcall -stub -version=0x601+ PathStripPathW	()
@ stdcall -stub -version=0x601+ PathStripToRootA	()
@ stdcall -stub -version=0x601+ PathStripToRootW	()
@ stdcall -stub -version=0x601+ PathUnExpandEnvStringsA	()
@ stdcall -stub -version=0x601+ PathUnExpandEnvStringsW	()
@ stdcall -stub -version=0x601+ PathUnquoteSpacesA	()
@ stdcall -stub -version=0x601+ PathUnquoteSpacesW	()
@ stdcall -stub -version=0x601+ PeekConsoleInputA	()
@ stdcall -stub -version=0x601+ PeekConsoleInputW	()
@ stdcall -stub -version=0x601+ PoolPerAppKeyStateInternal	()
@ stdcall -stub -version=0x601+ PrivCopyFileExW	()
@ stdcall -stub -version=0x601+ ProcessTrace	()
@ stdcall -stub -version=0x601+ PurgeComm	()
@ stdcall -stub -version=0x601+ QISearch	()
@ stdcall -stub -version=0x601+ QueryActCtxSettingsW	()
@ stdcall -stub -version=0x601+ QueryActCtxW	()
@ stdcall -stub -version=0x601+ QueryAllTracesA	()
@ stdcall -stub -version=0x601+ QueryAllTracesW	()
@ stdcall -stub -version=0x601+ QueryMemoryResourceNotification	()
@ stdcall -stub -version=0x601+ QueryOptionalDelayLoadedAPI	()
@ stdcall -stub -version=0x601+ QueryUnbiasedInterruptTime	()
@ stdcall -stub -version=0x601+ QueryWorkingSet	()
@ stdcall -stub -version=0x601+ QueryWorkingSetEx	()
@ stdcall -stub -version=0x601+ QueueUserWorkItem	()
@ stdcall -stub -version=0x601+ ReadConsoleA	()
@ stdcall -stub -version=0x601+ ReadConsoleInputA	()
@ stdcall -stub -version=0x601+ ReadConsoleInputExA	()
@ stdcall -stub -version=0x601+ ReadConsoleInputExW	()
@ stdcall -stub -version=0x601+ ReadConsoleInputW	()
@ stdcall -stub -version=0x601+ ReadConsoleOutputA	()
@ stdcall -stub -version=0x601+ ReadConsoleOutputAttribute	()
@ stdcall -stub -version=0x601+ ReadConsoleOutputCharacterA	()
@ stdcall -stub -version=0x601+ ReadConsoleOutputCharacterW	()
@ stdcall -stub -version=0x601+ ReadConsoleOutputW	()
@ stdcall -stub -version=0x601+ ReadConsoleW	()
@ stdcall -stub -version=0x601+ RefreshPolicyExInternal	()
@ stdcall -stub -version=0x601+ RefreshPolicyInternal	()
@ stdcall -stub -version=0x601+ RegCloseKey	()
@ stdcall -stub -version=0x601+ RegCopyTreeW	()
@ stdcall -stub -version=0x601+ RegCreateKeyExA	()
@ stdcall -stub -version=0x601+ RegCreateKeyExInternalA	()
@ stdcall -stub -version=0x601+ RegCreateKeyExInternalW	()
@ stdcall -stub -version=0x601+ RegCreateKeyExW	()
@ stdcall -stub -version=0x601+ RegDeleteKeyExA	()
@ stdcall -stub -version=0x601+ RegDeleteKeyExInternalA	()
@ stdcall -stub -version=0x601+ RegDeleteKeyExInternalW	()
@ stdcall -stub -version=0x601+ RegDeleteKeyExW	()
@ stdcall -stub -version=0x601+ RegDeleteTreeA	()
@ stdcall -stub -version=0x601+ RegDeleteTreeW	()
@ stdcall -stub -version=0x601+ RegDeleteValueA	()
@ stdcall -stub -version=0x601+ RegDeleteValueW	()
@ stdcall -stub -version=0x601+ RegDisablePredefinedCacheEx	()
@ stdcall -stub -version=0x601+ RegEnumKeyExA	()
@ stdcall -stub -version=0x601+ RegEnumKeyExW	()
@ stdcall -stub -version=0x601+ RegEnumValueA	()
@ stdcall -stub -version=0x601+ RegEnumValueW	()
@ stdcall -stub -version=0x601+ RegFlushKey	()
@ stdcall -stub -version=0x601+ RegGetKeySecurity	()
@ stdcall -stub -version=0x601+ RegGetValueA	()
@ stdcall -stub -version=0x601+ RegGetValueW	()
@ stdcall -stub -version=0x601+ RegKrnGetAppKeyEventAddressInternal	()
@ stdcall -stub -version=0x601+ RegKrnGetAppKeyLoaded	()
@ stdcall -stub -version=0x601+ RegKrnGetClassesEnumTableAddressInternal	()
@ stdcall -stub -version=0x601+ RegKrnGetHKEY_ClassesRootAddress	()
@ stdcall -stub -version=0x601+ RegKrnGetTermsrvRegistryExtensionFlags	()
@ stdcall -stub -version=0x601+ RegKrnResetAppKeyLoaded	()
@ stdcall -stub -version=0x601+ RegKrnSetDllHasThreadStateGlobal	()
@ stdcall -stub -version=0x601+ RegKrnSetTermsrvRegistryExtensionFlags	()
@ stdcall -stub -version=0x601+ RegLoadAppKeyA	()
@ stdcall -stub -version=0x601+ RegLoadAppKeyW	()
@ stdcall -stub -version=0x601+ RegLoadKeyA	()
@ stdcall -stub -version=0x601+ RegLoadKeyW	()
@ stdcall -stub -version=0x601+ RegLoadMUIStringA	()
@ stdcall -stub -version=0x601+ RegLoadMUIStringW	()
@ stdcall -stub -version=0x601+ RegNotifyChangeKeyValue	()
@ stdcall -stub -version=0x601+ RegOpenCurrentUser	()
@ stdcall -stub -version=0x601+ RegOpenKeyExA	()
@ stdcall -stub -version=0x601+ RegOpenKeyExInternalA	()
@ stdcall -stub -version=0x601+ RegOpenKeyExInternalW	()
@ stdcall -stub -version=0x601+ RegOpenKeyExW	()
@ stdcall -stub -version=0x601+ RegOpenUserClassesRoot	()
@ stdcall -stub -version=0x601+ RegQueryInfoKeyA	()
@ stdcall -stub -version=0x601+ RegQueryInfoKeyW	()
@ stdcall -stub -version=0x601+ RegQueryValueExA	()
@ stdcall -stub -version=0x601+ RegQueryValueExW	()
@ stdcall -stub -version=0x601+ RegRestoreKeyA	()
@ stdcall -stub -version=0x601+ RegRestoreKeyW	()
@ stdcall -stub -version=0x601+ RegSaveKeyExA	()
@ stdcall -stub -version=0x601+ RegSaveKeyExW	()
@ stdcall -stub -version=0x601+ RegSetKeySecurity	()
@ stdcall -stub -version=0x601+ RegSetValueExA	()
@ stdcall -stub -version=0x601+ RegSetValueExW	()
@ stdcall -stub -version=0x601+ RegUnLoadKeyA	()
@ stdcall -stub -version=0x601+ RegUnLoadKeyW	()
@ stdcall -stub -version=0x601+ RegisterGPNotificationInternal	()
@ stdcall -stub -version=0x601+ RegisterTraceGuidsA	()
@ stdcall -stub -version=0x601+ RegisterTraceGuidsW	()
@ stdcall -stub -version=0x601+ ReleaseActCtx	()
@ stdcall -stub -version=0x601+ RemapPredefinedHandleInternal	()
@ stdcall -stub -version=0x601+ RemoveTraceCallback	()
@ stdcall -stub -version=0x601+ ReplaceFileW	()
@ stdcall -stub -version=0x601+ ResetWriteWatch	()
@ stdcall -stub -version=0x601+ ResolveDelayLoadedAPI	()
@ stdcall -stub -version=0x601+ ResolveDelayLoadsFromDll	()
@ stdcall -stub -version=0x601+ RsopLoggingEnabledInternal	()
@ stdcall -stub -version=0x601+ SHExpandEnvironmentStringsA	()
@ stdcall -stub -version=0x601+ SHExpandEnvironmentStringsW	()
@ stdcall -stub -version=0x601+ SHLoadIndirectString	()
@ stdcall -stub -version=0x601+ SHRegCloseUSKey	()
@ stdcall -stub -version=0x601+ SHRegCreateUSKeyA	()
@ stdcall -stub -version=0x601+ SHRegCreateUSKeyW	()
@ stdcall -stub -version=0x601+ SHRegDeleteEmptyUSKeyA	()
@ stdcall -stub -version=0x601+ SHRegDeleteEmptyUSKeyW	()
@ stdcall -stub -version=0x601+ SHRegDeleteUSValueA	()
@ stdcall -stub -version=0x601+ SHRegDeleteUSValueW	()
@ stdcall -stub -version=0x601+ SHRegEnumUSKeyA	()
@ stdcall -stub -version=0x601+ SHRegEnumUSKeyW	()
@ stdcall -stub -version=0x601+ SHRegEnumUSValueA	()
@ stdcall -stub -version=0x601+ SHRegEnumUSValueW	()
@ stdcall -stub -version=0x601+ SHRegGetBoolUSValueA	()
@ stdcall -stub -version=0x601+ SHRegGetBoolUSValueW	()
@ stdcall -stub -version=0x601+ SHRegGetUSValueA	()
@ stdcall -stub -version=0x601+ SHRegGetUSValueW	()
@ stdcall -stub -version=0x601+ SHRegOpenUSKeyA	()
@ stdcall -stub -version=0x601+ SHRegOpenUSKeyW	()
@ stdcall -stub -version=0x601+ SHRegQueryInfoUSKeyA	()
@ stdcall -stub -version=0x601+ SHRegQueryInfoUSKeyW	()
@ stdcall -stub -version=0x601+ SHRegQueryUSValueA	()
@ stdcall -stub -version=0x601+ SHRegQueryUSValueW	()
@ stdcall -stub -version=0x601+ SHRegSetUSValueA	()
@ stdcall -stub -version=0x601+ SHRegSetUSValueW	()
@ stdcall -stub -version=0x601+ SHRegWriteUSValueA	()
@ stdcall -stub -version=0x601+ SHRegWriteUSValueW	()
@ stdcall -stub -version=0x601+ SHTruncateString	()
@ stdcall -stub -version=0x601+ ScrollConsoleScreenBufferA	()
@ stdcall -stub -version=0x601+ ScrollConsoleScreenBufferW	()
@ stdcall -stub -version=0x601+ SetClientTimeZoneInformation	()
@ stdcall -stub -version=0x601+ SetCommBreak	()
@ stdcall -stub -version=0x601+ SetCommConfig	()
@ stdcall -stub -version=0x601+ SetCommMask	()
@ stdcall -stub -version=0x601+ SetCommState	()
@ stdcall -stub -version=0x601+ SetCommTimeouts	()
@ stdcall -stub -version=0x601+ SetComputerNameExW	()
@ stdcall -stub -version=0x601+ SetConsoleActiveScreenBuffer	()
@ stdcall -stub -version=0x601+ SetConsoleCP	()
@ stdcall -stub -version=0x601+ SetConsoleCtrlHandler	()
@ stdcall -stub -version=0x601+ SetConsoleCursorInfo	()
@ stdcall -stub -version=0x601+ SetConsoleCursorPosition	()
@ stdcall -stub -version=0x601+ SetConsoleInputExeNameA	()
@ stdcall -stub -version=0x601+ SetConsoleInputExeNameW	()
@ stdcall -stub -version=0x601+ SetConsoleMode	()
@ stdcall -stub -version=0x601+ SetConsoleOutputCP	()
@ stdcall -stub -version=0x601+ SetConsoleScreenBufferInfoEx	()
@ stdcall -stub -version=0x601+ SetConsoleScreenBufferSize	()
@ stdcall -stub -version=0x601+ SetConsoleTextAttribute	()
@ stdcall -stub -version=0x601+ SetConsoleTitleW	()
@ stdcall -stub -version=0x601+ SetConsoleWindowInfo	()
@ stdcall -stub -version=0x601+ SetDynamicTimeZoneInformation	()
@ stdcall -stub -version=0x601+ SetLastConsoleEventActive	()
@ stdcall -stub -version=0x601+ SetProcessGroupAffinity	()
@ stdcall -stub -version=0x601+ SetProcessPreferredUILanguages	()
@ stdcall -stub -version=0x601+ SetProcessWorkingSetSizeEx	()
@ stdcall -stub -version=0x601+ SetSystemFileCacheSize	()
@ stdcall -stub -version=0x601+ SetSystemTime	()
@ stdcall -stub -version=0x601+ SetThreadContext	()
@ stdcall -stub -version=0x601+ SetThreadGroupAffinity	()
@ stdcall -stub -version=0x601+ SetThreadIdealProcessorEx	()
@ stdcall -stub -version=0x601+ SetThreadPreferredUILanguages	()
@ stdcall -stub -version=0x601+ SetThreadUILanguage	()
@ stdcall -stub -version=0x601+ SetThreadpoolTimerEx	()
@ stdcall -stub -version=0x601+ SetThreadpoolWaitEx	()
@ stdcall -stub -version=0x601+ SetTimeZoneInformation	()
@ stdcall -stub -version=0x601+ SetTraceCallback	()
@ stdcall -stub -version=0x601+ SetUnhandledExceptionFilter	()
@ stdcall -stub -version=0x601+ SetupComm	()
@ stdcall -stub -version=0x601+ SleepConditionVariableCS	()
@ stdcall -stub -version=0x601+ SleepConditionVariableSRW	()
@ stdcall -stub -version=0x601+ StartTraceA	()
@ stdcall -stub -version=0x601+ StartTraceW	()
@ stdcall -stub -version=0x601+ StopTraceW	()
@ stdcall -stub -version=0x601+ StrCSpnA	()
@ stdcall -stub -version=0x601+ StrCSpnIA	()
@ stdcall -stub -version=0x601+ StrCSpnIW	()
@ stdcall -stub -version=0x601+ StrCSpnW	()
@ stdcall -stub -version=0x601+ StrCatBuffA	()
@ stdcall -stub -version=0x601+ StrCatBuffW	()
@ stdcall -stub -version=0x601+ StrCatChainW	()
@ stdcall -stub -version=0x601+ StrChrA	()
@ stdcall -stub -version=0x601+ StrChrA_MB	()
@ stdcall -stub -version=0x601+ StrChrIA	()
@ stdcall -stub -version=0x601+ StrChrIW	()
@ stdcall -stub -version=0x601+ StrChrNIW	()
@ stdcall -stub -version=0x601+ StrChrNW	()
@ stdcall -stub -version=0x601+ StrChrW	()
@ stdcall -stub -version=0x601+ StrCmpCA	()
@ stdcall -stub -version=0x601+ StrCmpCW	()
@ stdcall -stub -version=0x601+ StrCmpICA	()
@ stdcall -stub -version=0x601+ StrCmpICW	()
@ stdcall -stub -version=0x601+ StrCmpIW	()
@ stdcall -stub -version=0x601+ StrCmpLogicalW	()
@ stdcall -stub -version=0x601+ StrCmpNA	()
@ stdcall -stub -version=0x601+ StrCmpNCA	()
@ stdcall -stub -version=0x601+ StrCmpNCW	()
@ stdcall -stub -version=0x601+ StrCmpNIA	()
@ stdcall -stub -version=0x601+ StrCmpNICA	()
@ stdcall -stub -version=0x601+ StrCmpNICW	()
@ stdcall -stub -version=0x601+ StrCmpNIW	()
@ stdcall -stub -version=0x601+ StrCmpNW	()
@ stdcall -stub -version=0x601+ StrCmpW	()
@ stdcall -stub -version=0x601+ StrCpyNW	()
@ stdcall -stub -version=0x601+ StrCpyNXA	()
@ stdcall -stub -version=0x601+ StrCpyNXW	()
@ stdcall -stub -version=0x601+ StrDupA	()
@ stdcall -stub -version=0x601+ StrDupW	()
@ stdcall -stub -version=0x601+ StrIsIntlEqualA	()
@ stdcall -stub -version=0x601+ StrIsIntlEqualW	()
@ stdcall -stub -version=0x601+ StrPBrkA	()
@ stdcall -stub -version=0x601+ StrPBrkW	()
@ stdcall -stub -version=0x601+ StrRChrA	()
@ stdcall -stub -version=0x601+ StrRChrIA	()
@ stdcall -stub -version=0x601+ StrRChrIW	()
@ stdcall -stub -version=0x601+ StrRChrW	()
@ stdcall -stub -version=0x601+ StrRStrIA	()
@ stdcall -stub -version=0x601+ StrRStrIW	()
@ stdcall -stub -version=0x601+ StrSpnA	()
@ stdcall -stub -version=0x601+ StrSpnW	()
@ stdcall -stub -version=0x601+ StrStrA	()
@ stdcall -stub -version=0x601+ StrStrIA	()
@ stdcall -stub -version=0x601+ StrStrIW	()
@ stdcall -stub -version=0x601+ StrStrNIW	()
@ stdcall -stub -version=0x601+ StrStrNW	()
@ stdcall -stub -version=0x601+ StrStrW	()
@ stdcall -stub -version=0x601+ StrToInt64ExA	()
@ stdcall -stub -version=0x601+ StrToInt64ExW	()
@ stdcall -stub -version=0x601+ StrToIntA	()
@ stdcall -stub -version=0x601+ StrToIntExA	()
@ stdcall -stub -version=0x601+ StrToIntExW	()
@ stdcall -stub -version=0x601+ StrToIntW	()
@ stdcall -stub -version=0x601+ StrTrimA	()
@ stdcall -stub -version=0x601+ StrTrimW	()
@ stdcall -stub -version=0x601+ TraceEvent	()
@ stdcall -stub -version=0x601+ TraceMessage	()
@ stdcall -stub -version=0x601+ TraceMessageVa	()
@ stdcall -stub -version=0x601+ TraceQueryInformation	()
@ stdcall -stub -version=0x601+ TraceSetInformation	()
@ stdcall -stub -version=0x601+ TransmitCommChar	()
@ stdcall -stub -version=0x601+ UnhandledExceptionFilter	()
@ stdcall -stub -version=0x601+ UnregisterGPNotificationInternal	()
@ stdcall -stub -version=0x601+ UnregisterTraceGuids	()
@ stdcall -stub -version=0x601+ UrlApplySchemeA	()
@ stdcall -stub -version=0x601+ UrlApplySchemeW	()
@ stdcall -stub -version=0x601+ UrlCanonicalizeA	()
@ stdcall -stub -version=0x601+ UrlCanonicalizeW	()
@ stdcall -stub -version=0x601+ UrlCombineA	()
@ stdcall -stub -version=0x601+ UrlCombineW	()
@ stdcall -stub -version=0x601+ UrlCompareA	()
@ stdcall -stub -version=0x601+ UrlCompareW	()
@ stdcall -stub -version=0x601+ UrlCreateFromPathA	()
@ stdcall -stub -version=0x601+ UrlCreateFromPathW	()
@ stdcall -stub -version=0x601+ UrlEscapeA	()
@ stdcall -stub -version=0x601+ UrlEscapeW	()
@ stdcall -stub -version=0x601+ UrlFixupW	()
@ stdcall -stub -version=0x601+ UrlGetLocationA	()
@ stdcall -stub -version=0x601+ UrlGetLocationW	()
@ stdcall -stub -version=0x601+ UrlGetPartA	()
@ stdcall -stub -version=0x601+ UrlGetPartW	()
@ stdcall -stub -version=0x601+ UrlHashA	()
@ stdcall -stub -version=0x601+ UrlHashW	()
@ stdcall -stub -version=0x601+ UrlIsA	()
@ stdcall -stub -version=0x601+ UrlIsNoHistoryA	()
@ stdcall -stub -version=0x601+ UrlIsNoHistoryW	()
@ stdcall -stub -version=0x601+ UrlIsOpaqueA	()
@ stdcall -stub -version=0x601+ UrlIsOpaqueW	()
@ stdcall -stub -version=0x601+ UrlIsW	()
@ stdcall -stub -version=0x601+ UrlUnescapeA	()
@ stdcall -stub -version=0x601+ UrlUnescapeW	()
@ stdcall -stub -version=0x601+ VerFindFileA	()
@ stdcall -stub -version=0x601+ VerFindFileW	()
@ stdcall -stub -version=0x601+ VerQueryValueA	()
@ stdcall -stub -version=0x601+ VerQueryValueW	()
@ stdcall -stub -version=0x601+ VirtualLock	()
@ stdcall -stub -version=0x601+ VirtualUnlock	()
@ stdcall -stub -version=0x601+ WaitCommEvent	()
@ stdcall -stub -version=0x601+ WaitForDebugEvent	()
@ stdcall -stub -version=0x601+ WaitForMachinePolicyForegroundProcessingInternal	()
@ stdcall -stub -version=0x601+ WaitForUserPolicyForegroundProcessingInternal	()
@ stdcall -stub -version=0x601+ WakeAllConditionVariable	()
@ stdcall -stub -version=0x601+ WakeConditionVariable	()
@ stdcall -stub -version=0x601+ WerRegisterMemoryBlock	()
@ stdcall -stub -version=0x601+ WerUnregisterMemoryBlock	()
@ stdcall -stub -version=0x601+ WerpNotifyLoadStringResource	()
@ stdcall -stub -version=0x601+ WerpNotifyUseStringResource	()
@ stdcall -stub -version=0x601+ WriteConsoleA	()
@ stdcall -stub -version=0x601+ WriteConsoleInputA	()
@ stdcall -stub -version=0x601+ WriteConsoleInputW	()
@ stdcall -stub -version=0x601+ WriteConsoleOutputA	()
@ stdcall -stub -version=0x601+ WriteConsoleOutputAttribute	()
@ stdcall -stub -version=0x601+ WriteConsoleOutputCharacterA	()
@ stdcall -stub -version=0x601+ WriteConsoleOutputCharacterW	()
@ stdcall -stub -version=0x601+ WriteConsoleOutputW	()
@ stdcall -stub -version=0x601+ WriteConsoleW	()
@ stdcall -stub -version=0x601+ ZombifyActCtx	()
@ stdcall -stub -version=0x601+ _AddMUIStringToCache	()
@ stdcall -stub -version=0x601+ _GetMUIStringFromCache	()
@ stdcall -stub -version=0x601+ _OpenMuiStringCache	()
@ stdcall -stub -version=0x601+ __dllonexit3	()
@ stdcall -stub -version=0x601+ __wgetmainargs	()
@ stdcall -stub -version=0x601+ _amsg_exit	()
@ stdcall -stub -version=0x601+ _c_exit	()
@ stdcall -stub -version=0x601+ _cexit	()
;@ stub -version=0x602+ _exit	
@ stub -version=0x602+ _initterm	
@ stub -version=0x602+ _initterm_e	
@ stub -version=0x602+ _invalid_parameter	
@ stub -version=0x602+ _onexit	
@ stub -version=0x602+ _purecall	
@ stub -version=0x602+ _time64	
@ stub -version=0x602+ atexit	
;@ stub -version=0x602+ exit	
@ stub -version=0x602+ hgets	
@ stub -version=0x602+ hwprintf	
@ stub -version=0x602+ time	
@ stub -version=0x602+ wprintf


;Windows 8.1 (checked as of 8/17/2021)
@ stub -version=0x603+ AcquireStateLock	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AllocateUserPhysicalPages	;KERNEL32 in 5.0 and higher
@ stub -version=0x603+ AllocateUserPhysicalPagesNuma	;KERNEL32 in 6.0 and higher
@ stub -version=0x603+ AppContainerDeriveSidFromMoniker	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppContainerFreeMemory	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppContainerLookupDisplayNameMrtReference	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppContainerLookupMoniker	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppContainerRegisterSid ;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppContainerUnregisterSid	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppXFreeMemory	;ERNEL32 in 6.2 only
@ stub -version=0x603+ AppXGetApplicationData	; KERNEL32 in 6.2 only
@ stub -version=0x603+ AppXGetDevelopmentMode	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppXGetOSMaxVersionTested	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ AppXGetOSMinVersion	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppXGetPackageCapabilities	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppXGetPackageSid	;KERNEL32 in 6.2 only
@ stub -version=0x603 AppXGetPackageState ;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppXLookupDisplayName	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppXLookupMoniker	;KERNEL32 in 6.2 only
@ stub -version=0x603+ AppXPostSuccessExtension	 
@ stub -version=0x603+ AppXPreCreationExtension	 
@ stub -version=0x603+ AppXReleaseAppXContext	 
@ stub -version=0x603 AppXSetPackageState
@ stub -version=0x603+ BaseFreeAppCompatDataForProcess	 
@ stub -version=0x603+ BaseReadAppCompatDataForProcess	 
@ stub -version=0x603+ CheckIfStateChangeNotificationExists	 
@ stub -version=0x603+ ClosePackageInfo	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ CloseState	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ CloseStateAtom	;KERNEL32 in 6.2 only
@ stub -version=0x603+ CloseStateChangeNotification	;KERNEL32 in 6.2 only
@ stub -version=0x603+ CloseStateContainer	;KERNEL32 in 6.2 only
@ stub -version=0x603+ CloseStateLock	;KERNEL32 in 6.2 only
@ stub -version=0x603+ CommitStateAtom	;KERNEL32 in 6.2 only
@ stdcall -version=0x603+ CopyContext() kernel32.CopyContext
@ stub -version=0x603+ CreateAppContainerToken	 
@ stub -version=0x603+ CreateStateAtom	;KERNEL32 in 6.2 only
@ stub -version=0x603+ CreateStateChangeNotification	;KERNEL32 in 6.2 only
@ stub -version=0x603+ CreateStateContainer	;KERNEL32 in 6.2 only
@ stub -version=0x603+ CreateStateLock	;KERNEL32 in 6.2 only
@ stub -version=0x603+ CreateStateSubcontainer	;KERNEL32 in 6.2 only
@ stub -version=0x603+ DeleteStateAtomValue	;KERNEL32 in 6.2 only
@ stub -version=0x603+ DeleteStateContainer	;KERNEL32 in 6.2 only
@ stub -version=0x603+ DeleteStateContainerValue	;KERNEL32 in 6.2 only
@ stub -version=0x603+ DiscardVirtualMemory	;KERNEL32 in 10.0 and higher
@ stub -version=0x603+ DnsHostnameToComputerNameExW	;KERNEL32 in 6.3 and higher
@ stub -version=0x603+ DuplicateStateContainerHandle	;ERNEL32 in 6.2 only
@ stub -version=0x603+ EnumerateStateAtomValues	; KERNEL32 in 6.2 only
@ stub -version=0x603+ EnumerateStateContainerItems	;KERNEL32 in 6.2 only
@ stub -version=0x603+ FindPackagesByPackageFamily	;KERNEL32 in 6.3 and higher
@ stub -version=0x603+ FormatApplicationUserModelId	;KERNEL32 in 6.3 and higher
@ stdcall -version=0x603+ FreeUserPhysicalPages() kernel32.FreeUserPhysicalPages
@ stub -version=0x603+ GetAppModelVersion	 
@ stub -version=0x603+ GetApplicationUserModelId	;KERNEL32 in 6.2 and higher
@ stdcall -version=0x603+ GetCompressedFileSizeA() kernel32.GetCompressedFileSizeA
@ stdcall -version=0x603+ GetCompressedFileSizeW() kernel32.GetCompressedFileSizeW
@ stub -version=0x603+ GetCurrentApplicationUserModelId	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetCurrentPackageApplicationContext	 
@ stub -version=0x603+ GetCurrentPackageApplicationResourcesContext	 
@ stub -version=0x603+ GetCurrentPackageContext	 
@ stub -version=0x603+ GetCurrentPackageFamilyName	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetCurrentPackageFullName	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetCurrentPackageId	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetCurrentPackageInfo	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetCurrentPackagePath	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetCurrentPackageResourcesContext	 
@ stub -version=0x603+ GetCurrentPackageSecurityContext	 
@ stub -version=0x603+ GetEnabledXStateFeatures	;KERNEL32 in some 6.1 and higher
@ stub -version=0x603+ GetHivePath	;KERNEL32 in 6.2 only
@ stub -version=0x603+ GetMemoryErrorHandlingCapabilities	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetPackageApplicationContext	 
@ stub -version=0x603+ GetPackageApplicationIds	;KERNEL32 in 6.3 and higher
@ stub -version=0x603+ GetPackageApplicationProperty	 
@ stub -version=0x603+ GetPackageApplicationPropertyString	 
@ stub -version=0x603+ GetPackageApplicationResourcesContext	 
@ stub -version=0x603+ GetPackageContext	 
@ stub -version=0x603+ GetPackageFamilyName	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetPackageFullName	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetPackageId	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetPackageInfo	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetPackageInstallTime	 
@ stub -version=0x603+ GetPackageOSMaxVersionTested	 
@ stub -version=0x603+ GetPackagePath	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetPackagePathByFullName	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetPackageProperty	 
@ stub -version=0x603+ GetPackagePropertyString	 
@ stub -version=0x603+ GetPackageResourcesContext	 
@ stub -version=0x603+ GetPackageResourcesProperty	 
@ stub -version=0x603+ GetPackageSecurityContext	 
@ stub -version=0x603+ GetPackageSecurityProperty	 
@ stub -version=0x603+ GetPackagesByPackageFamily	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetPhysicallyInstalledSystemMemory	;KERNEL32 in some 6.0 and higher
@ stub -version=0x603+ GetProcessPriorityBoost	;KERNEL32 in some 4.0 and higher
@ stub -version=0x603+ GetRoamingLastObservedChangeTime	;KERNEL32 in 6.2 only
@ stub -version=0x603+ GetSerializedAtomBytes	;KERNEL32 in 6.2 only
@ stub -version=0x603+ GetStagedPackageOrigin	 
@ stub -version=0x603+ GetStagedPackagePathByFullName	;KERNEL32 in 6.3 and higher
@ stub -version=0x603+ GetStateContainerDepth	;KERNEL32 in 6.2 only
@ stub -version=0x603+ GetStateFolder	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetStateRootFolder	;KERNEL32 in 6.2 only
@ stub -version=0x603+ GetStateSettingsFolder	;KERNEL32 in 6.2 only
@ stub -version=0x603+ GetStateVersion	;KERNEL32 in 6.2 only
@ stub -version=0x603+ GetSystemAppDataFolder	;KERNEL32 in 6.2 only
@ stub -version=0x603+ GetSystemAppDataKey	;KERNEL32 in 6.2 and higher
@ stdcall -version=0x603+ GetSystemTimes() kernel32.GetSystemTimes
@ stdcall -version=0x603+ GetThreadIOPendingFlag() kernel32.GetThreadIOPendingFlag
@ stub -version=0x603+ GetThreadInformation	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ GetXStateFeaturesMask	;KERNEL32 in some 6.1 and higher
@ stub -version=0x603+ InitializeContext	;KERNEL32 in some 6.1 and higher
@ stub -version=0x603+ InstallELAMCertificateInfo	;KERNEL32 in 6.3 and higher
@ stub -version=0x603+ InvalidateAppModelVersionCache	 
@ stub -version=0x603+ IsProcessCritical	;KERNEL32 in 6.3 and higher
@ stub -version=0x603+ LocateXStateFeature	;KERNEL32 in some 6.1 and higher
@ stub -version=0x603+ MapUserPhysicalPages() kernel32.MapUserPhysicalPages
@ stub -version=0x603+ OfferVirtualMemory
@ stub -version=0x603+ OpenFileById	;KERNEL32 in 6.0 and higher
@ stub -version=0x603+ OpenPackageInfoByFullName	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ OpenState	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ OpenStateAtom	;KERNEL32 in 6.2 only
@ stub -version=0x603+ OpenStateExplicit	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ OverrideRoamingDataModificationTimesInRange	;KERNEL32 in 6.2 only
@ stub -version=0x603+ PackageFamilyNameFromFullName	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ PackageFamilyNameFromId	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ PackageFullNameFromId	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ PackageIdFromFullName	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ PackageNameAndPublisherIdFromFamilyName;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ ParseApplicationUserModelId	;KERNEL32 in 6.3 and higher
@ stub -version=0x603+ PsmCreateKey	 
@ stub -version=0x603+ PsmEqualApplication	 
@ stub -version=0x603+ PsmEqualPackage	 
@ stub -version=0x603+ PsmGetApplicationNameFromKey	 
@ stub -version=0x603+ PsmGetKeyFromProcess	 
@ stub -version=0x603+ PsmGetKeyFromToken	 
@ stub -version=0x603+ PsmGetPackageFullNameFromKey	 
@ stub -version=0x603+ PsmIsChildKey	 
@ stub -version=0x603+ PsmIsDynamicKey	 
@ stub -version=0x603+ PsmIsValidKey	 
@ stub -version=0x603+ PublishStateChangeNotification	;KERNEL32 in 6.2 only
@ stub -version=0x603+ QueryStateAtomValueInfo	;KERNEL32 in 6.2 only
@ stub -version=0x603+ QueryStateContainerItemInfo;KERNEL32 in 6.2 only
@ stub -version=0x603+ QuirkGetData	 
@ stub -version=0x603+ QuirkGetData2	 
@ stub -version=0x603+ QuirkIsEnabled	 
@ stub -version=0x603+ QuirkIsEnabled2	 
@ stub -version=0x603+ QuirkIsEnabled3	 
@ stub -version=0x603+ QuirkIsEnabledForPackage	 
@ stub -version=0x603+ QuirkIsEnabledForPackage2	 
@ stub -version=0x603+ QuirkIsEnabledForProcess	 
@ stub -version=0x603+ ReadStateAtomValue	;KERNEL32 in 6.2 only
@ stub -version=0x603+ ReadStateContainerValue	;KERNEL32 in 6.2 only
@ stub -version=0x603+ ReclaimVirtualMemory
@ stub -version=0x603+ RegisterBadMemoryNotification	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ RegisterStateChangeNotification	;KERNEL32 in 6.2 only
@ stub -version=0x603+ RegisterStateLock	;KERNEL32 in 6.2 only
@ stub -version=0x603+ ReleaseStateLock	;KERNEL32 in 6.2 only
@ stub -version=0x603+ ReplaceFileExInternal	 
@ stub -version=0x603+ ResetState	;KERNEL32 in 6.2 only
@ stub -version=0x603+ SetComputerNameEx2W	;KERNEL32 in 6.3 and higher
@ stub -version=0x603+ SetFileIoOverlappedRange	;KERNEL32 in 6.0 and higher
@ stdcall -version=0x603+ SetProcessPriorityBoost() kernel32.SetProcessPriorityBoost
@ stub -version=0x603+ SetRoamingLastObservedChangeTime	 
@ stub -version=0x603+ SetStateVersion	;KERNEL32 in 6.2 only
@ stdcall -version=0x603+ SetSystemTimeAdjustment() kernel32.SetSystemTimeAdjustment
@ stub -version=0x603+ SetThreadInformation	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ SetXStateFeaturesMask	;KERNEL32 in some 6.1 and higher
@ stub -version=0x603+ SubscribeStateChangeNotification	;KERNEL32 in 6.2 only
@ stub -version=0x603+ UnregisterBadMemoryNotification	;KERNEL32 in 6.2 and higher
@ stub -version=0x603+ UnregisterStateChangeNotification	;KERNEL32 in 6.2 only
@ stub -version=0x603+ UnregisterStateLock ;KERNEL32 in 6.2 only
@ stub -version=0x603+ UnsubscribeStateChangeNotification	;KERNEL32 in 6.2 only
@ stub -version=0x603+ WriteStateAtomValue	;KERNEL32 in 6.2 only
@ stub -version=0x603+ WriteStateContainerValue	;KERNEL32 in 6.2 only

;Windows 10 stuff 
;AppXUpdatePackageCapabilities	 
;ApplicationUserModelIdFromProductId	 
;BaseGetNamedObjectDirectory	also exported from KERNEL32 in 6.1 and higher
;CalloutOnFiberStack	also exported from KERNEL32 in 6.3 and higher
;CeipIsOptedIn	also exported from KERNEL32 in 6.2 and higher
;CloseGlobalizationUserSettingsKey	 
;CompareObjectHandles	 
;ConvertFiberToThread	also exported from KERNEL32 in 5.1 and higher
;ConvertThreadToFiber	also exported from KERNEL32 in some 3.51 and higher (NT) and 4.10 and higher (Windows)
;ConvertThreadToFiberEx	also exported from KERNEL32 in 5.2 and higher
;CopyFileW	also exported from KERNEL32 in 3.51 and higher
;CreateFiber	also exported from KERNEL32 in some 3.51 and higher (NT) and 4.10 and higher (Windows)
;CreateFiberEx	also exported from KERNEL32 in some 5.0 and higher
;CreateHardLinkA	also exported from KERNEL32 in 5.0 and higher
;CreateProcessAsUserA	also exported from ADVAPI32 in 3.51 and higher;
;also exported from KERNEL32 in 10.0 and higher
;CreateSemaphoreW	also exported from KERNEL32 in 3.51 and higher
;CreateWaitableTimerW	also exported from KERNEL32 in some 4.0 and higher
;DecodeRemotePointer	forwarded to NTDLL function RtlDecodeRemotePointer
;DeleteFiber	also exported from KERNEL32 in some 3.51 and higher (NT) and 4.10 and higher (Windows)
;DsFreeNgcKey	 
;DsReadNgcKeyW	 
;DsWriteNgcKeyW	 
;EncodeRemotePointer	forwarded to NTDLL function RtlEncodeRemotePointer
;FindFirstFileNameW	also exported from KERNEL32 in 6.0 and higher
;FindFirstStreamW	also exported from KERNEL32 in 5.2 and higher
;FindNextFileNameW	also exported from KERNEL32 in 6.0 and higher
;FindNextStreamW	also exported from KERNEL32 in 5.2 and higher
;FindResourceW	also exported from KERNEL32 in 3.51 and higher
;GetAlternatePackageRoots	 
;GetAppDataFolder	 
;GetApplicationUserModelIdFromToken	 
;GetCurrentTargetPlatformContext	 
;GetDurationFormatEx	also exported from KERNEL32 in 6.0 and higher
;GetEffectivePackageStatusForUser	 
;GetFileVersionInfoA	 
;GetFileVersionInfoSizeA	 
;GetFileVersionInfoSizeW	 
;GetFileVersionInfoW	 
;GetIntegratedDisplaySize	 
;GetNamedPipeHandleStateW	also exported from KERNEL32 in 3.51 and higher
;GetNamedPipeInfo	also exported from KERNEL32 in 3.51 and higher
;GetOsManufacturingMode	 
;GetPackageFamilyNameFromToken	 
;GetPackageFullNameFromToken	 
;GetPackagePathOnVolume	 
;GetPackageStatus	 
;GetPackageStatusForUser	 
;GetPackageTargetPlatformProperty	 
;GetPackageVolumeSisPath	 
;GetProcessDefaultCpuSets	also exported from KERNEL32 in 10.0 and higher
;GetProcessInformation	also exported from KERNEL32 in 6.2 and higher
;GetProcessShutdownParameters	also exported from KERNEL32 in 3.51 and higher
;GetProcessorSystemCycleTime	also exported from KERNEL32 in 6.1 and higher
;GetPublisherCacheFolder	 
;GetPublisherRootFolder	 
;GetSharedLocalFolder	 
;GetStateRootFolderBase	 
;GetSystemCpuSetInformation	also exported from KERNEL32 in 10.0 and higher
;GetSystemMetadataPath	 
;GetSystemMetadataPathForPackage	 
;GetSystemMetadataPathForPackageFamily	 
;GetSystemStateRootFolder	 
;GetSystemWow64DirectoryA	also exported from KERNEL32 in 5.1 and higher
;GetSystemWow64DirectoryW	also exported from KERNEL32 in 5.1 and higher
;GetTargetPlatformContext	 
;GetTempFileNameA	also exported from KERNEL32 in 3.51 and higher
;GetTempPathA	also exported from KERNEL32 in 3.51 and higher
;GetThreadErrorMode	also exported from KERNEL32 in 6.1 and higher
;GetThreadSelectedCpuSets	also exported from KERNEL32 in 10.0 and higher
;GetUserOverrideString	 
;GetUserOverrideWord	 
;GetVolumeInformationA	also exported from KERNEL32 in 3.51 and higher
;IncrementPackageStatusVersion	 
;IsDeveloperModeEnabled	 
;IsDeveloperModePolicyApplied	 
;IsSideloadingEnabled	 
;IsSideloadingPolicyApplied	 
;LoadLibraryA	also exported from KERNEL32 in 3.51 and higher
;LoadLibraryW	also exported from KERNEL32 in 3.51 and higher
;LoadPackagedLibrary	also exported from KERNEL32 in 6.2 and higher
;MulDiv	also exported from KERNEL32 in 3.51 and higher
@ stub -version=0xA00+ NamedPipeEventEnum	 
@ stub -version=0xA00+ NamedPipeEventSelect	 
@ stub -version=0xA00+ OpenFileMappingFromApp	 
@ stub -version=0xA00+ OpenGlobalizationUserSettingsKey	 
@ stub -version=0xA00+ OpenPackageInfoByFullNameForUser	 
@ stub -version=0xA00+ OpenStateExplicitForUserSid	 
@ stub -version=0xA00+ OpenStateExplicitForUserSidString	 
@ stub -version=0xA00+ PackageFamilyNameFromProductId	 
@ stub -version=0xA00+ PackageFullNameFromProductId	 
@ stub -version=0xA00+ PackageIdFromProductId	 
@ stub -version=0xA00+ PackageRelativeApplicationIdFromProductId	 
@ stub -version=0xA00+ PackageSidFromFamilyName	 
@ stub -version=0xA00+ PackageSidFromProductId	 
@ stub -version=0xA00+ PcwAddQueryItem	 
@ stub -version=0xA00+ PcwClearCounterSetSecurity	 
@ stub -version=0xA00+ PcwCollectData	 
@ stub -version=0xA00+ PcwCompleteNotification	 
@ stub -version=0xA00+ PcwCreateNotifier	 
@ stub -version=0xA00+ PcwCreateQuery	 
@ stub -version=0xA00+ PcwDisconnectCounterSet	 
@ stub -version=0xA00+ PcwEnumerateInstances	 
@ stub -version=0xA00+ PcwIsNotifierAlive	 
@ stub -version=0xA00+ PcwQueryCounterSetSecurity	 
@ stub -version=0xA00+ PcwReadNotificationData	 
@ stub -version=0xA00+ PcwRegisterCounterSet	 
@ stub -version=0xA00+ PcwRemoveQueryItem	 
@ stub -version=0xA00+ PcwSendNotification	 
@ stub -version=0xA00+ PcwSendStatelessNotification	 
@ stub -version=0xA00+ PcwSetCounterSetSecurity	 
@ stub -version=0xA00+ PcwSetQueryItemUserData	 
;PerfCreateInstance	also exported from ADVAPI32 in 6.0 and higher
;PerfDecrementULongCounterValue	also exported from ADVAPI32 in 6.0 and higher
;PerfDecrementULongLongCounterValue	also exported from ADVAPI32 in 6.0 and higher
;PerfDeleteInstance	also exported from ADVAPI32 in 6.0 and higher
;PerfIncrementULongCounterValue	also exported from ADVAPI32 in 6.0 and higher
;PerfIncrementULongLongCounterValue	also exported from ADVAPI32 in 6.0 and higher
;PerfQueryInstance	also exported from ADVAPI32 in 6.0 and higher
;PerfSetCounterRefValue	also exported from ADVAPI32 in 6.0 and higher
;PerfSetCounterSetInfo	also exported from ADVAPI32 in 6.0 and higher
;PerfSetULongCounterValue	also exported from ADVAPI32 in 6.0 and higher
;PerfSetULongLongCounterValue	also exported from ADVAPI32 in 6.0 and higher
;PerfStartProvider	also exported from ADVAPI32 in 6.0 and higher
;PerfStartProviderEx	also exported from ADVAPI32 in 6.0 and higher
;PerfStopProvider	also exported from ADVAPI32 in 6.0 and higher
;ProductIdFromPackageFamilyName	 
;PsmCreateKeyWithDynamicId	 
;PssCaptureSnapshot	also exported from KERNEL32 in 6.3 and higher
;PssDuplicateSnapshot	also exported from KERNEL32 in 6.3 and higher
;PssFreeSnapshot	also exported from KERNEL32 in 6.3 and higher
;PssQuerySnapshot	also exported from KERNEL32 in 6.3 and higher
;PssWalkMarkerCreate	also exported from KERNEL32 in 6.3 and higher
;PssWalkMarkerFree	also exported from KERNEL32 in 6.3 and higher
;PssWalkMarkerGetPosition	also exported from KERNEL32 in 6.3 and higher
;PssWalkMarkerSeekToBeginning	also exported from KERNEL32 in 6.3 and higher
;PssWalkMarkerSetPosition	also exported from KERNEL32 in 6.3 and higher
;PssWalkSnapshot	also exported from KERNEL32 in 6.3 and higher
;QueryInterruptTime	 
;QueryInterruptTimePrecise	 
;QueryProtectedPolicy	also exported from KERNEL32 in 10.0 and higher
;QueryUnbiasedInterruptTimePrecise	 
;QuirkIsEnabledForPackage3	 
;QuirkIsEnabledForPackage4	 
;RaiseFailFastException	also exported from KERNEL32 in 6.1 and higher
@ stub -version=0xA00+ RegDeleteKeyValueA	 
@ stub -version=0xA00+ RegDeleteKeyValueW	 
@ stub -version=0xA00+ RegSetKeyValueA	 
@ stub -version=0xA00+ RegSetKeyValueW	 
@ stub -version=0xA00+ RemovePackageStatus	 
@ stub -version=0xA00+ RemovePackageStatusForUser	 
@ stub -version=0xA00+ SHLoadIndirectStringInternal	 
@ stub -version=0xA00+ SaveAlternatePackageRootPath	 
@ stub -version=0xA00+ SaveStateRootFolderPath	 
;SetComputerNameA	also exported from KERNEL32 in 3.51 and higher
;SetComputerNameExA	also exported from KERNEL32 in 5.0 and higher
;SetComputerNameW	also exported from KERNEL32 in 3.51 and higher
;SetIsDeveloperModeEnabled	 
;SetIsSideloadingEnabled	 
;SetProcessDefaultCpuSets	also exported from KERNEL32 in 10.0 and higher
;SetProcessInformation	also exported from KERNEL32 in 6.2 and higher
;SetProcessValidCallTargets	 
;SetProtectedPolicy	also exported from KERNEL32 in 10.0 and higher
;SetThreadErrorMode	also exported from KERNEL32 in 6.1 and higher
;SetThreadIdealProcessor	also exported from KERNEL32 in some 4.0 and higher
;SetThreadSelectedCpuSets	also exported from KERNEL32 in 10.0 and higher
@ stub -version=0xA00+ SharedLocalIsEnabled	 
@ stub -version=0xA00+ StmAlignSize	 
@ stub -version=0xA00+ StmAllocateFlat	 
@ stub -version=0xA00+ StmCoalesceChunks	 
@ stub -version=0xA00+ StmDeinitialize	 
@ stub -version=0xA00+ StmInitialize	 
@ stub -version=0xA00+ StmReduceSize	 
@ stub -version=0xA00+ StmReserve	 
@ stub -version=0xA00+ StmWrite	 
@ stdcall SwitchToFiber() kernel32.SwitchToFiber
@ stub -version=0xA00+ TerminateProcessOnMemoryExhaustion	 
@ stub -version=0xA00+ UpdatePackageStatus	 
@ stub -version=0xA00+ UpdatePackageStatusForUser	 
@ stub -version=0xA00+ VerifyApplicationUserModelId	 
@ stub -version=0xA00+ VerifyPackageFamilyName	 
@ stub -version=0xA00+ VerifyPackageFullName	 
@ stub -version=0xA00+ VerifyPackageId	 
@ stub -version=0xA00+ VerifyPackageRelativeApplicationId	 
@ stub -version=0xA00+ VirtualAllocFromApp	 
@ stub -version=0xA00+ VirtualProtectFromApp	 
@ stub -version=0xA00+ WTSGetServiceSessionId	 
@ stdcall SystemTimeToFileTime(ptr ptr) kernel32.SystemTimeToFileTime
@ stub -version=0xA00+ WTSIsServerContainer	 
@ stub -version=0xA00+ WaitForDebugEventEx;	also exported from KERNEL32 in 10.0 and higher
@ stdcall -version=0xA00+ WaitForMultipleObjects() kernel32.WaitForMultipleObjects
@ stdcall -version=0xA00+ WerGetFlags() kernel32.WerGetFlags
@ stdcall -version=0xA00+ WerSetFlags() kernel32.WerSetFlags

@ stdcall SetSecurityDescriptorDacl() advapi32.SetSecurityDescriptorDacl
@ stdcall SetSecurityDescriptorOwner() advapi32.SetSecurityDescriptorOwner
@ stdcall SetFileSecurityW() advapi32.SetFileSecurityW
@ stdcall SetTokenInformation() advapi32.SetTokenInformation



; HACKS
@ stub LookupAccountSidLocalW
@ stub LookupAccountNameLocalW


@ stdcall ConvertStringSidToSidW() advapi32.ConvertStringSidToSidW
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorW() advapi32.ConvertStringSecurityDescriptorToSecurityDescriptorW
@ stdcall ConvertSecurityDescriptorToStringSecurityDescriptorW() advapi32.ConvertSecurityDescriptorToStringSecurityDescriptorW
@ stdcall ConvertSidToStringSidW() advapi32.ConvertSidToStringSidW