@ stdcall -version=0x600+ AcquireSRWLockExclusive(ptr) NTDLL.RtlAcquireSRWLockExclusive
@ stdcall -version=0x600+ AcquireSRWLockShared(ptr) NTDLL.RtlAcquireSRWLockShared
@ stdcall ActivateActCtx(ptr ptr) kernelbase.ActivateActCtx
@ stdcall AddAtomA(str) 
@ stdcall AddAtomW(wstr)
@ stdcall AddConsoleAliasA(str str str) kernelbase.AddConsoleAliasA ;check
@ stdcall AddConsoleAliasW(wstr wstr wstr) kernelbase.AddConsoleAliasW ;check
@ stdcall -version=0x600+ AddDllDirectory(wstr) kernelbase.AddDllDirectory
@ stdcall -stub -version=0x601+ AddIntegrityLabelToBoundaryDescriptor(ptr ptr)
@ stdcall AddLocalAlternateComputerNameA(str ptr) kernelbase.AddLocalAlternateComputerNameA
@ stdcall AddLocalAlternateComputerNameW(wstr ptr) kernelbase.AddLocalAlternateComputerNameW
@ stdcall AddRefActCtx(ptr) kernelbase.AddRefActCtx
@ stdcall -stub -version=0x600+ AddSIDToBoundaryDescriptor(ptr ptr)
@ stdcall -stub -version=0x600+ AddSecureMemoryCacheCallback(ptr)
@ stdcall AddVectoredContinueHandler(long ptr) ntdll.RtlAddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) ntdll.RtlAddVectoredExceptionHandler
@ stdcall -stub -version=0x600+ AdjustCalendarDate(ptr long long)
@ stdcall AllocConsole() kernelbase.AllocConsole
@ stdcall -stub PrefetchVirtualMemory(ptr ptr ptr long)
@ stdcall AllocateUserPhysicalPages(long ptr ptr) kernelbase.AllocateUserPhysicalPages
@ stdcall -stub -version=0x600+ AllocateUserPhysicalPagesNuma(ptr ptr ptr long)
@ stdcall -version=0x600+ ApplicationRecoveryFinished(long)
@ stdcall -version=0x600+ ApplicationRecoveryInProgress(ptr)
@ stdcall -version=0xA00+ AppPolicyGetMediaFoundationCodecLoading(ptr ptr) kernelbase.AppPolicyGetMediaFoundationCodecLoading
@ stdcall -version=0xA00+ AppPolicyGetWindowingModel(ptr ptr) kernelbase.AppPolicyGetWindowingModel
@ stdcall AreFileApisANSI() kernelbase.AreFileApisANSI
@ stdcall AssignProcessToJobObject(ptr ptr)
@ stdcall AttachConsole(long) kernelbase.AttachConsole
@ stdcall BackupRead(ptr ptr long ptr long long ptr) kernelbase.BackupRead
@ stdcall BackupSeek(ptr long long ptr ptr ptr) kernelbase.BackupSeek
@ stdcall BackupWrite(ptr ptr long ptr long long ptr) kernelbase.BackupWrite
@ stdcall BaseCheckAppcompatCache(wstr ptr wstr ptr) kernelbase.BaseCheckAppcompatCache
# @ stub BaseCheckAppcompatCacheEx
@ stdcall BaseCheckRunApp(long ptr long long long long long long long long) kernelbase.BaseCheckRunApp
@ stdcall BaseCleanupAppcompatCacheSupport(ptr) kernelbase.BaseCleanupAppcompatCacheSupport
# @ stub -version=0x601+ BaseDllReadWriteIniFile
@ stdcall BaseDumpAppcompatCache() kernelbase.BaseDumpAppcompatCache
@ stdcall BaseFlushAppcompatCache() kernelbase.BaseFlushAppcompatCache
@ stub -version=0x600+ BaseGenerateAppCompatData
@ stdcall BaseInitAppcompatCacheSupport() kernelbase.BaseInitAppcompatCacheSupport
@ stdcall BaseIsAppcompatInfrastructureDisabled() kernelbase.BaseIsAppcompatInfrastructureDisabled
@ stdcall -version=0x501-0x502 BaseProcessInitPostImport() kernelbase.BaseProcessInitPostImport
@ stdcall -version=0x600+ BaseProcessInitPostImport() # HACK: This export is dynamicaly imported by ntdll kernelbase.BaseProcessInitPostImport
;@ stdcall -version=0x502 -arch=x86_64 BaseProcessStart()
@ stdcall BaseQueryModuleData(str str ptr ptr ptr) ;check kernelbase.BaseQueryModuleData
@ stub -version=0x600+ BaseThreadInitThunk
;@ stdcall -version=0x502 -arch=x86_64 BaseThreadStart()
@ stdcall BaseUpdateAppcompatCache(long long long) kernelbase.BaseUpdateAppcompatCache
@ stdcall BasepCheckBadapp(long ptr long long long long long long long) kernelbase.BasepCheckBadapp
@ stdcall BasepCheckWinSaferRestrictions(long long long long long long) kernelbase.BasepCheckWinSaferRestrictions
@ stdcall BasepFreeAppCompatData(ptr ptr) kernelbase.BasepFreeAppCompatData
@ stdcall Beep(long long) kernelbase.Beep
@ stdcall BeginUpdateResourceA(str long)
@ stdcall BeginUpdateResourceW(wstr long)
@ stdcall BindIoCompletionCallback(long ptr long) kernelbase.BindIoCompletionCallback
@ stdcall BuildCommDCBA(str ptr) kernelbase.BuildCommDCBA
@ stdcall BuildCommDCBAndTimeoutsA(str ptr ptr) kernelbase.BuildCommDCBAndTimeoutsA
@ stdcall BuildCommDCBAndTimeoutsW(wstr ptr ptr) kernelbase.BuildCommDCBAndTimeoutsW
@ stdcall BuildCommDCBW(wstr ptr) kernelbase.BuildCommDCBW
@ stdcall CallNamedPipeA(str ptr long ptr long ptr long) kernelbase.CallNamedPipeA
@ stdcall CallNamedPipeW(wstr ptr long ptr long ptr long) kernelbase.CallNamedPipeW
@ stdcall -version=0x600+ CallbackMayRunLong(ptr) kernelbase.CallbackMayRunLong
@ stdcall CancelDeviceWakeupRequest(long) kernelbase.CancelDeviceWakeupRequest
@ stdcall CancelIo(long) kernelbase.CancelIo
@ stdcall -version=0x600+ CancelIoEx(ptr ptr) kernelbase.CancelIoEx
@ stdcall -stub -version=0x600+ CancelSynchronousIo(ptr)
@ stdcall -version=0x600+ CancelThreadpoolIo(ptr) ntdll.TpCancelAsyncIoOperation
@ stdcall CancelTimerQueueTimer(long long) kernelbase.CancelTimerQueueTimer
@ stdcall CancelWaitableTimer(long) kernelbase.CancelWaitableTimer
@ stdcall ChangeTimerQueueTimer(ptr ptr long long) kernelbase.ChangeTimerQueueTimer
@ stdcall -stub -version=0x600+ CheckElevation(ptr ptr ptr ptr ptr)
@ stdcall -stub -version=0x600+ CheckElevationEnabled(ptr)
@ stub -version=0x600+ CheckForReadOnlyResource
@ stdcall CheckNameLegalDOS8Dot3A(str str long long long) kernelbase.CheckNameLegalDOS8Dot3A
@ stdcall CheckNameLegalDOS8Dot3W(wstr str long long long) kernelbase.CheckNameLegalDOS8Dot3W
@ stdcall CheckRemoteDebuggerPresent(long ptr) kernelbase.CheckRemoteDebuggerPresent
@ stdcall ClearCommBreak(long) kernelbase.ClearCommBreak
@ stdcall ClearCommError(long ptr ptr) kernelbase.ClearCommError
@ stdcall CloseConsoleHandle(long) kernelbase.CloseConsoleHandle
@ stdcall CloseHandle(long) kernelbase.CloseHandle
@ stdcall -stub -version=0x600+ ClosePrivateNamespace(ptr long)
@ stdcall CloseProfileUserMapping() kernelbase.CloseProfileUserMapping
@ stdcall -stub -version=0xA00+ ClosePseudoConsole(ptr)
@ stdcall -version=0x600+ CloseThreadpool(ptr) ntdll.TpReleasePool
@ stdcall -version=0x600+ CloseThreadpoolCleanupGroup(ptr) ntdll.TpReleaseCleanupGroup
@ stdcall -version=0x600+ CloseThreadpoolCleanupGroupMembers(ptr long ptr) ntdll.TpReleaseCleanupGroupMembers
@ stdcall -version=0x600+ CloseThreadpoolIo(ptr) ntdll.TpReleaseIoCompletion
@ stdcall -version=0x600+ CloseThreadpoolTimer(ptr) ntdll.TpReleaseTimer
@ stdcall -version=0x600+ CloseThreadpoolWait(ptr) ntdll.TpReleaseWait
@ stdcall -version=0x600+ CloseThreadpoolWork(ptr) ntdll.TpReleaseWork
@ stdcall CmdBatNotification(long) kernelbase.CmdBatNotification
@ stdcall CommConfigDialogA(str long ptr) kernelbase.CommConfigDialogA
@ stdcall CommConfigDialogW(wstr long ptr) kernelbase.CommConfigDialogW
@ stdcall -stub -version=0x600+ CompareCalendarDates(ptr ptr ptr)
@ stdcall CompareFileTime(ptr ptr) kernelbase.CompareFileTime
@ stdcall CompareStringA(long long str long str long) kernelbase.CompareStringA
@ stdcall -version=0x600+ CompareStringEx(wstr long wstr long wstr long ptr ptr ptr)
@ stdcall -version=0x600+ CompareStringOrdinal(wstr long wstr long long)
@ stdcall CompareStringW(long long wstr long wstr long) kernelbase.CompareStringW
@ stdcall ConnectNamedPipe(long ptr) kernelbase.ConnectNamedPipe
;@ stdcall -arch=x86_64 ConsoleIMERoutine()
@ stdcall ConsoleMenuControl(long long long) kernelbase.ConsoleMenuControl
@ stdcall ContinueDebugEvent(long long long) kernelbase.ContinueDebugEvent
@ stdcall -stub -version=0x600+ ConvertCalDateTimeToSystemTime(ptr ptr)
@ stdcall ConvertDefaultLocale(long) kernelbase.ConvertDefaultLocale
@ stdcall ConvertFiberToThread() kernelbase.ConvertFiberToThread
@ stdcall -stub -version=0x600+ ConvertNLSDayOfWeekToWin32DayOfWeek(long)
@ stdcall -stub -version=0x600+ ConvertSystemTimeToCalDateTime(ptr long ptr)
@ stdcall ConvertThreadToFiber(ptr) kernelbase.ConvertThreadToFiber
@ stdcall ConvertThreadToFiberEx(ptr long) kernelbase.ConvertThreadToFiberEx
@ stdcall -version=0x601+ CopyContext(ptr long ptr) kernelbase.CopyContext
@ stdcall -version=0x602+ CopyFile2(wstr wstr ptr) kernelbase.CopyFile2
@ stdcall CopyFileA(str str long) kernelbase.CopyFileA
@ stdcall CopyFileExA(str str ptr ptr ptr long) kernelbase.CopyFileExA
@ stdcall CopyFileExW(wstr wstr ptr ptr ptr long) kernelbase.CopyFileExW
@ stdcall -stub -version=0x600+ CopyFileTransactedA(str str ptr ptr ptr long ptr)
@ stdcall -stub -version=0x600+ CopyFileTransactedW(wstr wstr ptr ptr ptr long ptr)
@ stdcall CopyFileW(wstr wstr long) kernelbase.CopyFileW
@ stdcall CopyLZFile(long long) LZCopy
@ stdcall CreateActCtxA(ptr) kernelbase.CreateActCtxA
@ stdcall CreateActCtxW(ptr) kernelbase.CreateActCtxW
@ stdcall -stub -version=0x600+ CreateBoundaryDescriptorA(str long)
@ stdcall -version=0x600+ CreateBoundaryDescriptorW(wstr long) kernelbase.CreateBoundaryDescriptorW
@ stdcall CreateConsoleScreenBuffer(long long ptr long ptr) kernelbase.CreateConsoleScreenBuffer
@ stdcall CreateDirectoryA(str ptr) kernelbase.CreateDirectoryA
@ stdcall CreateDirectoryExA(str str ptr) kernelbase.CreateDirectoryExA
@ stdcall CreateDirectoryExW(wstr wstr ptr) kernelbase.CreateDirectoryExW
@ stdcall -version=0x600+ CreateDirectoryTransactedA(str str ptr ptr)
@ stdcall -version=0x600+ CreateDirectoryTransactedW(wstr wstr ptr ptr)
@ stdcall CreateDirectoryW(wstr ptr) kernelbase.CreateDirectoryW
@ stdcall CreateEventA(ptr long long str) kernelbase.CreateEventA
@ stdcall -version=0x600+ CreateEventExA(ptr str long long) kernelbase.CreateEventExA
@ stdcall -version=0x600+ CreateEventExW(ptr wstr long long) kernelbase.CreateEventExW
@ stdcall CreateEventW(ptr long long wstr) kernelbase.CreateEventW
@ stdcall CreateFiber(long ptr ptr) kernelbase.CreateFiber
@ stdcall CreateFiberEx(long long long ptr ptr) kernelbase.CreateFiberEx
@ stdcall -version=0x602+ CreateFile2(wstr long long long ptr) kernelbase.CreateFile2
@ stdcall CreateFileA(str long long ptr long long long) kernelbase.CreateFileA
@ stdcall CreateFileMappingA(long ptr long long long str) kernelbase.CreateFileMappingA
@ stdcall -version=0x600+ CreateFileMappingFromApp(long ptr long int64 wstr) kernelbase.CreateFileMappingFromApp
@ stdcall -stub -version=0x600+ CreateFileMappingNumaA(ptr ptr long long long str long)
@ stdcall -stub -version=0x600+ CreateFileMappingNumaW(ptr ptr long long long wstr long)
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernelbase.CreateFileMappingW
@ stdcall -version=0x600+ CreateFileTransactedA(str long long ptr long long ptr ptr ptr ptr)
@ stdcall -version=0x600+ CreateFileTransactedW(wstr long long ptr long long ptr ptr ptr ptr)
@ stdcall CreateFileW(wstr long long ptr long long long) kernelbase.CreateFileW
@ stdcall CreateHardLinkA(str str ptr) kernelbase.CreateHardLinkA
@ stdcall -version=0x600+ CreateHardLinkTransactedA(str str ptr ptr)
@ stdcall -version=0x600+ CreateHardLinkTransactedW(wstr wstr ptr ptr)
@ stdcall CreateHardLinkW(wstr wstr ptr) kernelbase.CreateHardLinkW
@ stdcall CreateIoCompletionPort(long long long long) kernelbase.CreateIoCompletionPort
@ stdcall CreateJobObjectA(ptr str)
@ stdcall CreateJobObjectW(ptr wstr)
@ stdcall CreateJobSet(long ptr long)
@ stdcall CreateMailslotA(ptr long long ptr)
@ stdcall CreateMailslotW(ptr long long ptr)
@ stdcall CreateMemoryResourceNotification(long) kernelbase.CreateMemoryResourceNotification
@ stdcall CreateMutexA(ptr long str) kernelbase.CreateMutexA
@ stdcall -version=0x600+ CreateMutexExA(ptr str long long) kernelbase.CreateMutexExA
@ stdcall -version=0x600+ CreateMutexExW(ptr wstr long long) kernelbase.CreateMutexExW
@ stdcall CreateMutexW(ptr long wstr) kernelbase.CreateMutexW
@ stdcall CreateNamedPipeA(str long long long long long long ptr) kernelbase.CreateNamedPipeA
@ stdcall CreateNamedPipeW(wstr long long long long long long ptr) kernelbase.CreateNamedPipeW
@ stdcall -version=0x501-0x502 CreateNlsSecurityDescriptor(ptr long long) kernelbase.CreateNlsSecurityDescriptor
@ stdcall CreatePipe(ptr ptr ptr long) kernelbase.CreatePipe
@ stdcall -stub -version=0x600+ CreatePrivateNamespaceA(ptr ptr str)
@ stdcall -stub -version=0x600+ CreatePrivateNamespaceW(ptr ptr wstr)
@ stdcall CreateProcessA(str str ptr ptr long long ptr str ptr ptr) kernelbase.CreateProcessA
@ stdcall CreateProcessInternalA(ptr str str ptr ptr long long ptr str ptr ptr long) kernelbase.CreateProcessInternalA
@ stdcall CreateProcessInternalW(ptr wstr wstr ptr ptr long long ptr wstr ptr ptr long) kernelbase.CreateProcessInternalW
@ stdcall CreateProcessW(wstr wstr ptr ptr long long ptr wstr ptr ptr) kernelbase.CreateProcessW
@ stdcall -stub -version=0xA00+ CreatePseudoConsole(long long long long ptr)
@ stdcall CreateRemoteThread(long ptr long ptr long long ptr) kernelbase.CreateRemoteThread
@ stdcall -version=0x601+ -stub CreateRemoteThreadEx(long ptr long ptr ptr long ptr ptr) kernelbase.CreateRemoteThreadEx
@ stdcall CreateSemaphoreA(ptr long long str) kernelbase.CreateSemaphoreA
@ stdcall -version=0x600+ CreateSemaphoreExA(ptr long long str long long) kernelbase.CreateSemaphoreExA
@ stdcall -version=0x600+ CreateSemaphoreExW(ptr long long wstr long long) kernelbase.CreateSemaphoreExW
@ stdcall CreateSemaphoreW(ptr long long wstr) kernelbase.CreateSemaphoreW
@ stdcall -i386 CreateSocketHandle() kernelbase.CreateSocketHandle
@ stdcall -version=0x600+ CreateSymbolicLinkA(str str long) kernelbase.CreateSymbolicLinkA
@ stdcall -stub -version=0x600+ CreateSymbolicLinkTransactedA(str str long ptr)
@ stdcall -stub -version=0x600+ CreateSymbolicLinkTransactedW(wstr wstr long ptr)
@ stdcall -version=0x600+ CreateSymbolicLinkW(wstr wstr long)
@ stdcall CreateTapePartition(long long long long)
@ stdcall CreateThread(ptr long ptr long long ptr) kernelbase.CreateThread
@ stdcall -version=0x600+ CreateThreadpool(ptr) kernelbase.CreateThreadpool
@ stdcall -version=0x600+ CreateThreadpoolCleanupGroup() kernelbase.CreateThreadpoolCleanupGroup
@ stdcall -version=0x600+ CreateThreadpoolIo(ptr ptr ptr ptr) kernelbase.CreateThreadpoolIo
@ stdcall -version=0x600+ CreateThreadpoolTimer(ptr ptr ptr) kernelbase.CreateThreadpoolTimer
@ stdcall -version=0x600+ CreateThreadpoolWait(ptr ptr ptr) kernelbase.CreateThreadpoolWait
@ stdcall -version=0x600+ CreateThreadpoolWork(ptr ptr ptr) kernelbase.CreateThreadpoolWork
@ stdcall CreateTimerQueue() kernelbase.CreateTimerQueue
@ stdcall CreateTimerQueueTimer(ptr long ptr ptr long long long) kernelbase.CreateTimerQueueTimer
@ stdcall CreateToolhelp32Snapshot(long long) kernelbase.CreateToolhelp32Snapshot
@ stdcall -version=0x601+ -arch=win64 CreateUmsCompletionList(ptr)
@ stdcall -version=0x601+ -arch=win64 CreateUmsThreadContext(ptr)
@ stdcall CreateWaitableTimerA(ptr long str) kernelbase.CreateWaitableTimerA
@ stdcall -version=0x600+ CreateWaitableTimerExA(ptr str long long)
@ stdcall -version=0x600+ CreateWaitableTimerExW(ptr wstr long long) kernelbase.CreateWaitableTimerExW
@ stdcall CreateWaitableTimerW(ptr long wstr) kernelbase.CreateWaitableTimerW
;@ stdcall -arch=x86_64 CtrlRoutine()
@ stdcall DeactivateActCtx(long ptr) kernelbase.DeactivateActCtx
@ stdcall DebugActiveProcess(long) kernelbase.DebugActiveProcess
@ stdcall DebugActiveProcessStop(long) kernelbase.DebugActiveProcessStop
@ stdcall DebugBreak() ntdll.DbgBreakPoint
@ stdcall DebugBreakProcess(long) kernelbase.DebugBreakProcess
@ stdcall DebugSetProcessKillOnExit(long) kernelbase.DebugSetProcessKillOnExit
@ stdcall DecodePointer(ptr) ntdll.RtlDecodePointer
@ stdcall DecodeSystemPointer(ptr) ntdll.RtlDecodeSystemPointer
@ stdcall DefineDosDeviceA(long str str) kernelbase.DefineDosDeviceA
@ stdcall DefineDosDeviceW(long wstr wstr) kernelbase.DefineDosDeviceW
@ stdcall DelayLoadFailureHook(str str) kernelbase.DelayLoadFailureHook
@ stdcall DeleteAtom(long) 
@ stub -version=0x600+ DeleteBoundaryDescriptor
@ stdcall DeleteCriticalSection(ptr) ntdll.RtlDeleteCriticalSection
@ stdcall DeleteFiber(ptr) kernelbase.DeleteFiber
@ stdcall DeleteFileA(str) kernelbase.DeleteFileA
@ stdcall -version=0x600+ DeleteFileTransactedA(str ptr)
@ stdcall -version=0x600+ DeleteFileTransactedW(wstr ptr)
@ stdcall DeleteFileW(wstr) kernelbase.DeleteFileW
@ stdcall -version=0x600+ DeleteProcThreadAttributeList(ptr) kernelbase.DeleteProcThreadAttributeList
# @ stub -version=0x601+ DisableThreadProfiling
@ stdcall -version=0xA00+ DiscardVirtualMemory(ptr long) kernelbase.DiscardVirtualMemory
@ stdcall DeleteTimerQueue(long) kernelbase.DeleteTimerQueue
@ stdcall DeleteTimerQueueEx(long long) kernelbase.DeleteTimerQueueEx
@ stdcall DeleteTimerQueueTimer(long long long) kernelbase.DeleteTimerQueueTimer
@ stdcall -version=0x601+ -arch=win64 DeleteUmsCompletionList(ptr)
@ stdcall -version=0x601+ -arch=win64 DeleteUmsThreadContext(ptr)
@ stdcall DeleteVolumeMountPointA(str) kernelbase.DeleteVolumeMountPointA ;check
@ stdcall DeleteVolumeMountPointW(wstr) kernelbase.DeleteVolumeMountPointW ;check
@ stdcall -version=0x601+ -arch=win64 DequeueUmsCompletionListItems(ptr long ptr)
@ stdcall DeviceIoControl(long long ptr long ptr long ptr ptr) kernelbase.DeviceIoControl
@ stdcall DisableThreadLibraryCalls(ptr) kernelbase.DisableThreadLibraryCalls
@ stdcall -version=0x600+ DisassociateCurrentThreadFromCallback(ptr) ntdll.TpDisassociateCallback
@ stdcall DisconnectNamedPipe(long) kernelbase.DisconnectNamedPipe
@ stdcall DnsHostnameToComputerNameA(str ptr ptr) kernelbase.DnsHostnameToComputerNameA
@ stdcall DnsHostnameToComputerNameW(wstr ptr ptr) kernelbase.DnsHostnameToComputerNameW
@ stdcall DosDateTimeToFileTime(long long ptr) kernelbase.DosDateTimeToFileTime
@ stdcall DosPathToSessionPathA(long str str) kernelbase.DosPathToSessionPathA
@ stdcall DosPathToSessionPathW(long wstr wstr) kernelbase.DosPathToSessionPathW
@ stdcall DuplicateConsoleHandle(long long long long) kernelbase.DuplicateConsoleHandle
@ stdcall DuplicateHandle(long long long ptr long long long) kernelbase.DuplicateHandle
# @ stub -version=0x601 EnableThreadProfiling
@ stdcall EncodePointer(ptr) ntdll.RtlEncodePointer
@ stdcall EncodeSystemPointer(ptr) ntdll.RtlEncodeSystemPointer
@ stdcall EndUpdateResourceA(long long)
@ stdcall EndUpdateResourceW(long long)
@ stdcall EnterCriticalSection(ptr) ntdll.RtlEnterCriticalSection
@ stdcall EnumCalendarInfoA(ptr long long long) kernelbase.EnumCalendarInfoA
@ stdcall EnumCalendarInfoExA(ptr long long long) kernelbase.EnumCalendarInfoExA
@ stdcall -version=0x600+ EnumCalendarInfoExEx(ptr wstr long wstr long long)
@ stdcall EnumCalendarInfoExW(ptr long long long) kernelbase.EnumCalendarInfoExW
@ stdcall EnumCalendarInfoW(ptr long long long) kernelbase.EnumCalendarInfoW
@ stdcall EnumDateFormatsA(ptr long long) kernelbase.EnumDateFormatsA
@ stdcall EnumDateFormatsExA(ptr long long) kernelbase.EnumDateFormatsExA
@ stdcall -version=0x600+ EnumDateFormatsExEx(ptr wstr long long)
@ stdcall EnumDateFormatsExW(ptr long long) kernelbase.EnumDateFormatsExW
@ stdcall EnumDateFormatsW(ptr long long) kernelbase.EnumDateFormatsW
@ stdcall EnumLanguageGroupLocalesA(ptr long long ptr) kernelbase.EnumLanguageGroupLocalesA
@ stdcall EnumLanguageGroupLocalesW(ptr long long ptr) kernelbase.EnumLanguageGroupLocalesW
@ stdcall EnumResourceLanguagesA(long str str ptr long)
@ stdcall -version=0x600+ EnumResourceLanguagesExA(long str str ptr long long long) kernelbase.EnumResourceLanguagesExA
@ stdcall -version=0x600+ EnumResourceLanguagesExW(long wstr wstr ptr long long long) kernelbase.EnumResourceLanguagesExW
@ stdcall EnumResourceLanguagesW(long wstr wstr ptr long)
@ stdcall EnumResourceNamesA(long str ptr long)
@ stdcall -version=0x600+ EnumResourceNamesExA(long str ptr long long long) kernelbase.EnumResourceNamesExA
@ stdcall -version=0x600+ EnumResourceNamesExW(long wstr ptr long long long) kernelbase.EnumResourceNamesExW
@ stdcall EnumResourceNamesW(long wstr ptr long) kernelbase.EnumResourceNamesW
@ stdcall EnumResourceTypesA(long ptr long)
@ stdcall -version=0x600+ EnumResourceTypesExA(long ptr long long long) kernelbase.EnumResourceTypesExA
@ stdcall -version=0x600+ EnumResourceTypesExW(long ptr long long long) kernelbase.EnumResourceTypesExW
@ stdcall EnumResourceTypesW(long ptr long)
@ stdcall EnumSystemCodePagesA(ptr long) kernelbase.EnumSystemCodePagesA
@ stdcall EnumSystemCodePagesW(ptr long) kernelbase.EnumSystemCodePagesW
@ stdcall EnumSystemFirmwareTables(long ptr long) kernelbase.EnumSystemFirmwareTables
@ stdcall EnumSystemGeoID(long long ptr) kernelbase.EnumSystemGeoID
@ stdcall EnumSystemLanguageGroupsA(ptr long ptr) kernelbase.EnumSystemLanguageGroupsA
@ stdcall EnumSystemLanguageGroupsW(ptr long ptr) kernelbase.EnumSystemLanguageGroupsW
@ stdcall EnumSystemLocalesA(ptr long) kernelbase.EnumSystemLocalesA
@ stdcall -version=0x600+ EnumSystemLocalesEx(ptr long long ptr)
@ stdcall EnumSystemLocalesW(ptr long) kernelbase.EnumSystemLocalesW
@ stdcall EnumTimeFormatsA(ptr long long) kernelbase.EnumTimeFormatsA
@ stdcall -version=0x600+ EnumTimeFormatsEx(ptr wstr long long)
@ stdcall EnumTimeFormatsW(ptr long long) kernelbase.EnumTimeFormatsW
@ stdcall EnumUILanguagesA(ptr long long) kernelbase.EnumUILanguagesA
@ stdcall EnumUILanguagesW(ptr long long) kernelbase.EnumUILanguagesW
@ stdcall EnumerateLocalComputerNamesA(ptr long str ptr) kernelbase.EnumerateLocalComputerNamesA
@ stdcall EnumerateLocalComputerNamesW(ptr long wstr ptr) kernelbase.EnumerateLocalComputerNamesW
@ stdcall -version=0x601 -arch=win64 EnterUmsSchedulingMode(ptr)
@ stdcall EraseTape(ptr long long)
@ stdcall EscapeCommFunction(long long) kernelbase.EscapeCommFunction
@ stdcall -version=0x601 -arch=win64 ExecuteUmsThread(ptr)
@ stdcall ExitProcess(long) kernelbase.ExitProcess
@ stdcall ExitThread(long) kernelbase.ExitThread
@ stdcall ExitVDM(long long) kernelbase.ExitVDM
@ stdcall ExpandEnvironmentStringsA(str ptr long) kernelbase.ExpandEnvironmentStringsA
@ stdcall ExpandEnvironmentStringsW(wstr ptr long) kernelbase.ExpandEnvironmentStringsW
@ stdcall ExpungeConsoleCommandHistoryA(long) kernelbase.ExpungeConsoleCommandHistoryA
@ stdcall ExpungeConsoleCommandHistoryW(long) kernelbase.ExpungeConsoleCommandHistoryW
@ stdcall FatalAppExitA(long str) kernelbase.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernelbase.FatalAppExitW
@ stdcall FatalExit(long) kernelbase.FatalExit
@ stdcall FileTimeToDosDateTime(ptr ptr ptr) kernelbase.FileTimeToDosDateTime
@ stdcall FileTimeToLocalFileTime(ptr ptr) kernelbase.FileTimeToLocalFileTime
@ stdcall FileTimeToSystemTime(ptr ptr) kernelbase.FileTimeToSystemTime
@ stdcall FillConsoleOutputAttribute(long long long long ptr) kernelbase.FillConsoleOutputAttribute
@ stdcall FillConsoleOutputCharacterA(long long long long ptr) kernelbase.FillConsoleOutputCharacterA
@ stdcall FillConsoleOutputCharacterW(long long long long ptr) kernelbase.FillConsoleOutputCharacterW
@ stdcall FindActCtxSectionGuid(long ptr long ptr ptr) kernelbase.FindActCtxSectionGuid
@ stdcall FindActCtxSectionStringA(long ptr long str ptr) kernelbase.FindActCtxSectionStringA
@ stdcall FindActCtxSectionStringW(long ptr long wstr ptr) kernelbase.FindActCtxSectionStringW
@ stdcall FindAtomA(str) 
@ stdcall FindAtomW(wstr)
@ stdcall FindClose(long) kernelbase.FindClose
@ stdcall FindCloseChangeNotification(long) kernelbase.FindCloseChangeNotification
@ stdcall FindFirstChangeNotificationA(str long long) kernelbase.FindFirstChangeNotificationA
@ stdcall FindFirstChangeNotificationW(wstr long long) kernelbase.FindFirstChangeNotificationW
@ stdcall FindFirstFileA(str ptr) kernelbase.FindFirstFileA
@ stdcall FindFirstFileExA(str long ptr long ptr long) kernelbase.FindFirstFileExA
@ stdcall FindFirstFileExW(wstr long ptr long ptr long) kernelbase.FindFirstFileExW
@ stub -version=0x600+ FindFirstFileNameTransactedW
@ stdcall -version=0x600+ FindFirstFileNameW(wstr long ptr ptr) kernelbase.FindFirstFileNameW
@ stdcall -version=0x600+ FindFirstFileTransactedA(str long ptr long ptr long ptr)
@ stdcall -version=0x600+ FindFirstFileTransactedW(wstr long ptr long ptr long ptr)
@ stdcall FindFirstFileW(wstr ptr) kernelbase.FindFirstFileW
@ stub -version=0x600+ FindFirstStreamTransactedW
@ stdcall FindFirstStreamW(wstr ptr ptr long) kernelbase.FindFirstStreamW
@ stdcall FindFirstVolumeA(ptr long) kernelbase.FindFirstVolumeA
@ stdcall FindFirstVolumeMountPointA(str ptr long) kernelbase.FindFirstVolumeMountPointA
@ stdcall FindFirstVolumeMountPointW(wstr ptr long) kernelbase.FindFirstVolumeMountPointW
@ stdcall FindFirstVolumeW(ptr long) kernelbase.FindFirstVolumeW
@ stdcall -stub -version=0x600+ FindNLSString(long long wstr long wstr long ptr)
@ stdcall -stub -version=0x600+ FindNLSStringEx(wstr long wstr long wstr long ptr ptr ptr long)
@ stdcall FindNextChangeNotification(long) kernelbase.FindNextChangeNotification
@ stdcall FindNextFileA(long ptr) kernelbase.FindNextFileA
@ stub -version=0x600+ FindNextFileNameW
@ stdcall FindNextFileW(long ptr) kernelbase.FindNextFileW
@ stdcall FindNextStreamW(ptr ptr) kernelbase.FindNextStreamW
@ stdcall FindNextVolumeA(long ptr long) kernelbase.FindNextVolumeA
@ stdcall FindNextVolumeMountPointA(long str long) kernelbase.FindNextVolumeMountPointA
@ stdcall FindNextVolumeMountPointW(long wstr long) kernelbase.FindNextVolumeMountPointW
@ stdcall FindNextVolumeW(long ptr long) kernelbase.FindNextVolumeW
@ stdcall FindResourceA(long str str)
@ stdcall FindResourceExA(long str str long)
@ stdcall FindResourceExW(long wstr wstr long) kernelbase.FindResourceExW
@ stdcall FindResourceW(long wstr wstr) kernelbase.FindResourceW
@ stdcall -version=0x601+ -stub FindStringOrdinal(long wstr long wstr long long)
@ stdcall FindVolumeClose(ptr) kernelbase.FindVolumeClose
@ stdcall FindVolumeMountPointClose(ptr) kernelbase.FindVolumeMountPointClose
@ stdcall FlsAlloc(ptr) kernelbase.FlsAlloc
@ stdcall FlsFree(long) kernelbase.FlsFree
@ stdcall FlsGetValue(long) kernelbase.FlsGetValue
@ stdcall FlsSetValue(long ptr) kernelbase.FlsSetValue
@ stdcall FlushConsoleInputBuffer(long) kernelbase.FlushConsoleInputBuffer
@ stdcall FlushFileBuffers(long) kernelbase.FlushFileBuffers
@ stdcall FlushInstructionCache(long long long) kernelbase.FlushInstructionCache
@ stub -version=0x600+ FlushProcessWriteBuffers ;ntdll.FlushProcessWriteBuffers
@ stdcall FlushViewOfFile(ptr long) kernelbase.FlushViewOfFile
@ stdcall FoldStringA(long str long ptr long) kernelbase.FoldStringA
@ stdcall FoldStringW(long wstr long ptr long) kernelbase.FoldStringW
@ stdcall FormatMessageA(long ptr long long ptr long ptr) kernelbase.FormatMessageA
@ stdcall FormatMessageW(long ptr long long ptr long ptr) kernelbase.FormatMessageW
@ stdcall FreeConsole() kernelbase.FreeConsole
@ stdcall FreeEnvironmentStringsA(ptr) kernelbase.FreeEnvironmentStringsA
@ stdcall FreeEnvironmentStringsW(ptr) kernelbase.FreeEnvironmentStringsW
@ stdcall FreeLibrary(long) kernelbase.FreeLibrary
@ stdcall FreeLibraryAndExitThread(long long) kernelbase.FreeLibraryAndExitThread
@ stdcall -version=0x600+ FreeLibraryWhenCallbackReturns(ptr ptr) ntdll.TpCallbackUnloadDllOnCompletion
@ stdcall FreeResource(long) kernelbase.FreeResource
@ stdcall FreeUserPhysicalPages(long long long) kernelbase.FreeUserPhysicalPages
@ stdcall GenerateConsoleCtrlEvent(long long) kernelbase.GenerateConsoleCtrlEvent
@ stdcall GetACP() kernelbase.GetACP
@ stdcall -stub -version=0x601+ GetActiveProcessorCount(long)
@ stdcall -stub -version=0x601+ GetActiveProcessorGroupCount()
@ stdcall -version=0x600+ GetApplicationRecoveryCallback(ptr ptr ptr ptr ptr) kernelbase.GetApplicationRecoveryCallback
@ stdcall GetApplicationRestartSettings(long ptr ptr ptr) kernelbase.GetApplicationRestartSettings
@ stdcall GetAtomNameA(long ptr long)
@ stdcall GetAtomNameW(long ptr long)
@ stdcall GetBinaryType(str ptr) kernelbase.GetBinaryTypeA
@ stdcall GetBinaryTypeA(str ptr) kernelbase.GetBinaryTypeA
@ stdcall GetBinaryTypeW(wstr ptr) kernelbase.GetBinaryTypeW
@ stdcall -version=0x501-0x600 GetCPFileNameFromRegistry(long wstr long) kernelbase.GetCPFileNameFromRegistry
@ stdcall GetCPInfo(long ptr) kernelbase.GetCPInfo
@ stdcall GetCPInfoExA(long long ptr) kernelbase.GetCPInfoExA
@ stdcall GetCPInfoExW(long long ptr) kernelbase.GetCPInfoExW
@ stub -version=0x600+ GetCalendarDateFormat
@ stub -version=0x600+ GetCalendarDateFormatEx
@ stub -version=0x600+ GetCalendarDaysInMonth
@ stub -version=0x600+ GetCalendarDifferenceInDays
@ stdcall GetCalendarInfoA(long long long ptr long ptr) kernelbase.GetCalendarInfoA
@ stdcall -version=0x600+ GetCalendarInfoEx(wstr long wstr long wstr long ptr)
@ stdcall GetCalendarInfoW(long long long ptr long ptr) kernelbase.GetCalendarInfoW
@ stub -version=0x600+ GetCalendarMonthsInYear
@ stub -version=0x600+ GetCalendarSupportedDateRange
@ stub -version=0x600+ GetCalendarWeekNumber
@ stdcall GetComPlusPackageInstallStatus() kernelbase.GetComPlusPackageInstallStatus
@ stdcall GetCommConfig(long ptr long) kernelbase.GetCommConfig
@ stdcall GetCommMask(long ptr) kernelbase.GetCommMask
@ stdcall GetCommModemStatus(long ptr) kernelbase.GetCommModemStatus
@ stdcall GetCommProperties(long ptr) kernelbase.GetCommProperties
@ stdcall GetCommState(long ptr) kernelbase.GetCommState
@ stdcall GetCommTimeouts(long ptr) kernelbase.GetCommTimeouts
@ stdcall GetCommandLineA() kernelbase.GetCommandLineA
@ stdcall GetCommandLineW() kernelbase.GetCommandLineW
@ stdcall GetCompressedFileSizeA(long ptr) kernelbase.GetCompressedFileSizeA
@ stub -version=0x600+ GetCompressedFileSizeTransactedA
@ stub -version=0x600+ GetCompressedFileSizeTransactedW
@ stdcall GetCompressedFileSizeW(long ptr) kernelbase.GetCompressedFileSizeW
@ stdcall GetComputerNameA(ptr ptr) kernelbase.GetComputerNameA
@ stdcall GetComputerNameExA(long ptr ptr) kernelbase.GetComputerNameExA
@ stdcall GetComputerNameExW(long ptr ptr) kernelbase.GetComputerNameExW
@ stdcall GetComputerNameW(ptr ptr) kernelbase.GetComputerNameW
@ stdcall GetConsoleAliasA(str str long str) kernelbase.GetConsoleAliasA
@ stdcall GetConsoleAliasExesA(str long) kernelbase.GetConsoleAliasExesA
@ stdcall GetConsoleAliasExesLengthA() kernelbase.GetConsoleAliasExesLengthA
@ stdcall GetConsoleAliasExesLengthW() kernelbase.GetConsoleAliasExesLengthW
@ stdcall GetConsoleAliasExesW(wstr long) kernelbase.GetConsoleAliasExesW
@ stdcall GetConsoleAliasW(wstr ptr long wstr) kernelbase.GetConsoleAliasW
@ stdcall GetConsoleAliasesA(str long str) kernelbase.GetConsoleAliasesA
@ stdcall GetConsoleAliasesLengthA(str) kernelbase.GetConsoleAliasesLengthA
@ stdcall GetConsoleAliasesLengthW(wstr) kernelbase.GetConsoleAliasesLengthW
@ stdcall GetConsoleAliasesW(wstr long wstr) kernelbase.GetConsoleAliasesW
@ stdcall GetConsoleCP() kernelbase.GetConsoleCP
@ stdcall GetConsoleCharType(long long ptr) kernelbase.GetConsoleCharType
@ stdcall GetConsoleCommandHistoryA(long long long) kernelbase.GetConsoleCommandHistoryA
@ stdcall GetConsoleCommandHistoryLengthA(long) kernelbase.GetConsoleCommandHistoryLengthA
@ stdcall GetConsoleCommandHistoryLengthW(long) kernelbase.GetConsoleCommandHistoryLengthW
@ stdcall GetConsoleCommandHistoryW(long long long) kernelbase.GetConsoleCommandHistoryW
@ stdcall GetConsoleCursorInfo(long ptr) kernelbase.GetConsoleCursorInfo
@ stdcall GetConsoleCursorMode(long ptr ptr) kernelbase.GetConsoleCursorMode
@ stdcall GetConsoleDisplayMode(ptr) kernelbase.GetConsoleDisplayMode
@ stdcall GetConsoleFontInfo(long long long ptr) kernelbase.GetConsoleFontInfo
@ stdcall GetConsoleFontSize(long long) kernelbase.GetConsoleFontSize
@ stdcall GetConsoleHardwareState(long long ptr) kernelbase.GetConsoleHardwareState
@ stdcall -version=0x600+ GetConsoleHistoryInfo(ptr) kernelbase.GetConsoleHistoryInfo
@ stdcall GetConsoleInputExeNameA(long ptr) kernelbase.GetConsoleInputExeNameA
@ stdcall GetConsoleInputExeNameW(long ptr) kernelbase.GetConsoleInputExeNameW
@ stdcall GetConsoleInputWaitHandle() kernelbase.GetConsoleInputWaitHandle
@ stdcall GetConsoleKeyboardLayoutNameA(ptr) kernelbase.GetConsoleKeyboardLayoutNameA
@ stdcall GetConsoleKeyboardLayoutNameW(ptr) kernelbase.GetConsoleKeyboardLayoutNameW
@ stdcall GetConsoleMode(long ptr) kernelbase.GetConsoleMode
@ stdcall GetConsoleNlsMode(long ptr) kernelbase.GetConsoleNlsMode
@ stdcall -version=0x600+ GetConsoleOriginalTitleA(ptr long)
@ stdcall -version=0x600+ GetConsoleOriginalTitleW(ptr long)
@ stdcall GetConsoleOutputCP() kernelbase.GetConsoleOutputCP
@ stdcall GetConsoleProcessList(ptr long) kernelbase.GetConsoleProcessList
@ stdcall GetConsoleScreenBufferInfo(long ptr) kernelbase.GetConsoleScreenBufferInfo
@ stdcall -version=0x600+ GetConsoleScreenBufferInfoEx(ptr ptr)
@ stdcall GetConsoleSelectionInfo(ptr) kernelbase.GetConsoleSelectionInfo
@ stdcall GetConsoleTitleA(ptr long) kernelbase.GetConsoleTitleA
@ stdcall GetConsoleTitleW(ptr long) kernelbase.GetConsoleTitleW
@ stdcall GetConsoleWindow() kernelbase.GetConsoleWindow
@ stdcall GetCurrencyFormatA(long long str ptr str long) kernelbase.GetCurrencyFormatA
@ stdcall -version=0x600+ GetCurrencyFormatEx(wstr long wstr ptr wstr long)
@ stdcall GetCurrencyFormatW(long long wstr ptr wstr long) kernelbase.GetCurrencyFormatW
@ stdcall GetCurrentActCtx(ptr) kernelbase.GetCurrentActCtx
@ stdcall GetCurrentConsoleFont(long long ptr) kernelbase.GetCurrentConsoleFont
@ stdcall -version=0x600+ GetCurrentConsoleFontEx(ptr long ptr)
@ stdcall GetCurrentDirectoryA(long ptr) kernelbase.GetCurrentDirectoryA
@ stdcall GetCurrentDirectoryW(long ptr) kernelbase.GetCurrentDirectoryW
@ stdcall -version=0x602+ GetCurrentPackageFamilyName(ptr ptr) kernelbase.GetCurrentPackageFamilyName
@ stdcall -version=0x602+ GetCurrentPackageFullName(ptr ptr) kernelbase.GetCurrentPackageFullName
@ stdcall -version=0x602+ GetCurrentPackageId(ptr ptr) kernelbase.GetCurrentPackageId
@ stdcall -version=0x602+ GetCurrentPackagePath(ptr ptr) kernelbase.GetCurrentPackagePath
@ stdcall -norelay GetCurrentProcess() kernelbase.GetCurrentProcess
@ stdcall -norelay GetCurrentProcessId() kernelbase.GetCurrentProcessId
@ stdcall GetCurrentProcessorNumber() kernelbase.GetCurrentProcessorNumber
@ stub GetCurrentProcessorNumberEx(ptr) ;NTDLL.RtlGetCurrentProcessorNumberEx
@ stdcall -norelay GetCurrentThread() kernelbase.GetCurrentThread
@ stdcall -norelay GetCurrentThreadId() kernelbase.GetCurrentThreadId
@ stdcall -version=0x602+  GetCurrentThreadStackLimits(ptr ptr) kernelbase.GetCurrentThreadStackLimits
@ stdcall -version=0x601+ -arch=win64 GetCurrentUmsThread()
@ stdcall GetDateFormatA(long long ptr str ptr long) kernelbase.GetDateFormatA
@ stdcall -version=0x600+ GetDateFormatEx(wstr long ptr wstr wstr long wstr)
@ stdcall GetDateFormatW(long long ptr wstr ptr long) kernelbase.GetDateFormatW
@ stdcall GetDefaultCommConfigA(str ptr long) kernelbase.GetDefaultCommConfigA
@ stdcall GetDefaultCommConfigW(wstr ptr long) kernelbase.GetDefaultCommConfigW
@ stdcall -version=0x501-0x502 GetDefaultSortkeySize(ptr) kernelbase.GetDefaultSortkeySize
@ stdcall GetDevicePowerState(long ptr) kernelbase.GetDevicePowerState
@ stdcall GetDiskFreeSpaceA(str ptr ptr ptr ptr) kernelbase.GetDiskFreeSpaceA
@ stdcall GetDiskFreeSpaceExA(str ptr ptr ptr) kernelbase.GetDiskFreeSpaceExA
@ stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr) kernelbase.GetDiskFreeSpaceExW
@ stdcall GetDiskFreeSpaceW(wstr ptr ptr ptr ptr) kernelbase.GetDiskFreeSpaceW
@ stdcall GetDllDirectoryA(long ptr) kernelbase.GetDllDirectoryA
@ stdcall GetDllDirectoryW(long ptr) kernelbase.GetDllDirectoryW
@ stdcall GetDriveTypeA(str) kernelbase.GetDriveTypeA
@ stdcall GetDriveTypeW(wstr) kernelbase.GetDriveTypeW
@ stub -version=0x600+ GetDurationFormat
@ stub -version=0x600+ GetDurationFormatEx
@ stdcall -version=0x600+ GetDynamicTimeZoneInformation(ptr) kernelbase.GetDynamicTimeZoneInformation
@ stdcall -stub -version=0x602+ GetDynamicTimeZoneInformationEffectiveYears(ptr ptr ptr)
@ stdcall GetEnvironmentStrings() kernelbase.GetEnvironmentStrings
@ stdcall GetEnvironmentStringsA() kernelbase.GetEnvironmentStringsA
@ stdcall GetEnvironmentStringsW() kernelbase.GetEnvironmentStringsW
@ stdcall GetEnvironmentVariableA(str ptr long) kernelbase.GetEnvironmentVariableA
@ stdcall GetEnvironmentVariableW(wstr ptr long) kernelbase.GetEnvironmentVariableW
@ stdcall -version=0x600+ GetErrorMode() kernelbase.GetErrorMode
@ stdcall GetExitCodeProcess(long ptr) kernelbase.GetExitCodeProcess
@ stdcall GetExitCodeThread(long ptr) kernelbase.GetExitCodeThread
@ stdcall GetExpandedNameA(str ptr) kernelbase.GetExpandedNameA
@ stdcall GetExpandedNameW(wstr ptr) kernelbase.GetExpandedNameW
@ stdcall GetFileAttributesA(str) kernelbase.GetFileAttributesA
@ stdcall GetFileAttributesExA(str long ptr) kernelbase.GetFileAttributesExA
@ stdcall GetFileAttributesExW(wstr long ptr) kernelbase.GetFileAttributesExW
@ stdcall -version=0x600+ GetFileAttributesTransactedA(str long ptr ptr)
@ stdcall -version=0x600+ GetFileAttributesTransactedW(wstr long ptr ptr)
@ stdcall GetFileAttributesW(wstr) kernelbase.GetFileAttributesW
@ stdcall -version=0x600+ GetFileBandwidthReservation(ptr ptr ptr ptr ptr ptr) kernelbase.GetFileBandwidthReservation
@ stdcall GetFileInformationByHandle(long ptr) kernelbase.GetFileInformationByHandle
@ stdcall -version=0x600+ GetFileInformationByHandleEx(ptr long ptr long) kernelbase.GetFileInformationByHandleEx
@ stdcall -version=0x600+ GetFileMUIInfo(long wstr ptr ptr) kernelbase.GetFileMUIInfo
@ stdcall -version=0x600+ GetFileMUIPath(long wstr wstr ptr wstr ptr ptr) kernelbase.GetFileMUIPath
@ stdcall GetFileSize(long ptr) kernelbase.GetFileSize
@ stdcall GetFileSizeEx(long ptr) kernelbase.GetFileSizeEx
@ stdcall GetFileTime(long ptr ptr ptr) kernelbase.GetFileTime
@ stdcall GetFileType(long) kernelbase.GetFileType
@ stdcall -version=0x600+ GetFinalPathNameByHandleA(ptr str long long) kernelbase.GetFinalPathNameByHandleA
@ stdcall -version=0x600+ GetFinalPathNameByHandleW(ptr wstr long long) kernelbase.GetFinalPathNameByHandleW
@ stdcall GetFirmwareEnvironmentVariableA(str str ptr long) kernelbase.GetFirmwareEnvironmentVariableA
@ stdcall -version=0x602+ GetFirmwareEnvironmentVariableExA(str str ptr long long) kernelbase.GetFirmwareEnvironmentVariableExA
@ stdcall -version=0x602+ GetFirmwareEnvironmentVariableExW(wstr wstr ptr long long) kernelbase.GetFirmwareEnvironmentVariableExW
@ stdcall GetFirmwareEnvironmentVariableW(wstr wstr ptr long) kernelbase.GetFirmwareEnvironmentVariableW
@ stdcall -version=0x602+ GetFirmwareType(ptr) kernelbase.GetFirmwareType
@ stdcall GetFullPathNameA(str long ptr ptr) kernelbase.GetFullPathNameA
@ stub -version=0x600+ GetFullPathNameTransactedA
@ stub -version=0x600+ GetFullPathNameTransactedW
@ stdcall GetFullPathNameW(wstr long ptr ptr) kernelbase.GetFullPathNameW
@ stdcall GetGeoInfoA(long long ptr long long) kernelbase.GetGeoInfoA
@ stdcall GetGeoInfoW(long long ptr long long) kernelbase.GetGeoInfoW
@ stdcall -i386 GetHandleContext(long) kernelbase.GetHandleContext
@ stdcall GetHandleInformation(long ptr) kernelbase.GetHandleInformation
@ stdcall GetLargePageMinimum() kernelbase.GetLargePageMinimum
@ stdcall GetLargestConsoleWindowSize(long) kernelbase.GetLargestConsoleWindowSize
@ stdcall GetLastError() kernelbase.GetLastError
@ stdcall -version=0x500-0x502 GetLinguistLangSize(ptr) kernelbase.GetLinguistLangSize
@ stdcall GetLocalTime(ptr) kernelbase.GetLocalTime
@ stdcall GetLocaleInfoA(long long ptr long) kernelbase.GetLocaleInfoA
@ stdcall -version=0x600+ GetLocaleInfoEx(wstr long ptr long) kernelbase.GetLocaleInfoEx
@ stdcall GetLocaleInfoW(long long ptr long) kernelbase.GetLocaleInfoW
@ stdcall -version=0x600+ IsValidLocaleName(wstr) kernelbase.IsValidLocaleName
@ stdcall GetLogicalDriveStringsA(long ptr) kernelbase.GetLogicalDriveStringsA
@ stdcall GetLogicalDriveStringsW(long ptr) kernelbase.GetLogicalDriveStringsW
@ stdcall GetLogicalDrives() kernelbase.GetLogicalDrives
@ stdcall GetLogicalProcessorInformation(ptr ptr) kernelbase.GetLogicalProcessorInformation
@ stdcall -version=0x601+ GetLogicalProcessorInformationEx(long ptr ptr) kernelbase.GetLogicalProcessorInformationEx
@ stdcall GetLongPathNameA(str long long) kernelbase.GetLongPathNameA
@ stub -version=0x600+ GetLongPathNameTransactedA
@ stub -version=0x600+ GetLongPathNameTransactedW
@ stdcall GetLongPathNameW(wstr long long) kernelbase.GetLongPathNameW
@ stdcall GetMailslotInfo(long ptr ptr ptr ptr)
@ stdcall -version=0x601+ GetMaximumProcessorCount(long)
@ stdcall -version=0x601+ GetMaximumProcessorGroupCount()
@ stdcall GetModuleFileNameA(long ptr long) kernelbase.GetModuleFileNameA
@ stdcall GetModuleFileNameW(long ptr long) kernelbase.GetModuleFileNameW
@ stdcall GetModuleHandleA(str) kernelbase.GetModuleHandleA
@ stdcall GetModuleHandleExA(long ptr ptr) kernelbase.GetModuleHandleExA
@ stdcall GetModuleHandleExW(long ptr ptr) kernelbase.GetModuleHandleExW
@ stdcall GetModuleHandleW(wstr) kernelbase.GetModuleHandleW
@ stdcall GetNLSVersion(long long ptr) kernelbase.GetNLSVersion
@ stdcall GetNLSVersionEx(long wstr ptr) kernelbase.GetNLSVersionEx
@ stub -version=0x600+ GetNamedPipeAttribute
@ stub -version=0x600+ GetNamedPipeClientComputerNameA
@ stub -version=0x600+ GetNamedPipeClientComputerNameW
@ stdcall -version=0x600+ GetNamedPipeClientProcessId(ptr ptr) kernelbase.GetNamedPipeClientProcessId
@ stdcall -version=0x600+ -stub GetNamedPipeClientSessionId(long ptr)
@ stdcall GetNamedPipeHandleStateA(long ptr ptr ptr ptr str long) kernelbase.GetNamedPipeHandleStateA
@ stdcall GetNamedPipeHandleStateW(long ptr ptr ptr ptr wstr long) kernelbase.GetNamedPipeHandleStateW
@ stdcall GetNamedPipeInfo(long ptr ptr ptr ptr) kernelbase.GetNamedPipeInfo
@ stdcall -stub -version=0x600+ GetNamedPipeServerProcessId(long ptr)
@ stdcall -stub -version=0x600+ GetNamedPipeServerSessionId(long ptr)
@ stdcall GetNativeSystemInfo(ptr) kernelbase.GetNativeSystemInfo
@ stdcall -version=0x601+ -arch=win64 GetNextUmsListItem(ptr)
@ stdcall GetNextVDMCommand(long) kernelbase.GetNextVDMCommand
@ stdcall -version=0x500-0x502 GetNlsSectionName(long long long str str long) kernelbase.GetNlsSectionName
@ stdcall GetNumaAvailableMemoryNode(long ptr) kernelbase.GetNumaAvailableMemoryNode
@ stdcall -version=0x601+ GetNumaAvailableMemoryNodeEx(long ptr)
@ stdcall GetNumaHighestNodeNumber(ptr) kernelbase.GetNumaHighestNodeNumber
@ stdcall GetNumaNodeProcessorMask(long ptr) kernelbase.GetNumaNodeProcessorMask
@ stdcall -version=0x601+ -stub GetNumaNodeProcessorMaskEx(long ptr)
@ stdcall GetNumaProcessorNode(long ptr) kernelbase.GetNumaProcessorNode
@ stdcall -version=0x601+ GetNumaProcessorNodeEx(ptr ptr)
@ stdcall -version=0x600+ GetNumaProximityNode(long ptr)
@ stdcall -stub -version=0x601+ GetNumaProximityNodeEx(long ptr)
@ stdcall GetNumberFormatA(long long str ptr ptr long) kernelbase.GetNumberFormatA
@ stdcall -version=0x600+ GetNumberFormatEx(wstr long wstr ptr wstr long) kernelbase.GetNumberFormatEx
@ stdcall GetNumberFormatW(long long wstr ptr ptr long) kernelbase.GetNumberFormatW
@ stdcall GetNumberOfConsoleFonts() kernelbase.GetNumberOfConsoleFonts
@ stdcall GetNumberOfConsoleInputEvents(long ptr) kernelbase.GetNumberOfConsoleInputEvents
@ stdcall GetNumberOfConsoleMouseButtons(ptr) kernelbase.GetNumberOfConsoleMouseButtons
@ stdcall GetOEMCP() kernelbase.GetOEMCP
@ stdcall GetOverlappedResult(long ptr ptr long) kernelbase.GetOverlappedResult
@ stdcall -stub -version=0x602+ GetOverlappedResultEx(long ptr ptr long long)
@ stdcall -stub -version=0xA00+ GetUserDefaultGeoName(ptr long)
@ stdcall -version=0x602+ GetPackageFamilyName(long ptr ptr) kernelbase.GetPackageFamilyName
@ stdcall -version=0x602+ GetPackageFullName(long ptr ptr) kernelbase.GetPackageFullName
@ stdcall -version=0x602+ GetPackagesByPackageFamily(wstr ptr ptr ptr ptr) kernelbase.GetPackagesByPackageFamily
@ stdcall -version=0x603+ GetPackagePathByFullName(wstr ptr wstr) kernelbase.GetPackagePathByFullName
@ stdcall -stub -version=0x600+ GetPhysicallyInstalledSystemMemory(ptr) kernelbase.GetPhysicallyInstalledSystemMemory
@ stdcall GetPriorityClass(long) kernelbase.GetPriorityClass
@ stdcall GetPrivateProfileIntA(str str long str) kernelbase.GetPrivateProfileIntA
@ stdcall GetPrivateProfileIntW(wstr wstr long wstr) kernelbase.GetPrivateProfileIntW
@ stdcall GetPrivateProfileSectionA(str ptr long str) kernelbase.GetPrivateProfileSectionA
@ stdcall GetPrivateProfileSectionNamesA(ptr long str) kernelbase.GetPrivateProfileSectionNamesA
@ stdcall GetPrivateProfileSectionNamesW(ptr long wstr) kernelbase.GetPrivateProfileSectionNamesW
@ stdcall GetPrivateProfileSectionW(wstr ptr long wstr) kernelbase.GetPrivateProfileSectionW
@ stdcall GetPrivateProfileStringA(str str str ptr long str) kernelbase.GetPrivateProfileStringA
@ stdcall GetPrivateProfileStringW(wstr wstr wstr ptr long wstr) kernelbase.GetPrivateProfileStringW
@ stdcall GetPrivateProfileStructA(str str ptr long str) kernelbase.GetPrivateProfileStructA
@ stdcall GetPrivateProfileStructW(wstr wstr ptr long wstr) kernelbase.GetPrivateProfileStructW
@ stdcall GetProcAddress(long str) kernelbase.GetProcAddress
@ stdcall GetProcessAffinityMask(long ptr ptr) kernelbase.GetProcessAffinityMask
@ stdcall -version=0x600+ GetProcessDEPPolicy(ptr ptr ptr)
@ stdcall -version=0x601+ GetProcessGroupAffinity(long ptr ptr) kernelbase.GetProcessGroupAffinity
@ stdcall GetProcessHandleCount(long ptr) kernelbase.GetProcessHandleCount
@ stdcall -norelay GetProcessHeap() kernelbase.GetProcessHeap
@ stdcall GetProcessHeaps(long ptr) kernelbase.GetProcessHeaps
@ stdcall GetProcessId(long) kernelbase.GetProcessId
@ stdcall GetProcessIdOfThread(ptr) kernelbase.GetProcessIdOfThread
@ stdcall  -version=0x602+ GetProcessInformation(long long ptr long) kernelbase.GetProcessInformation
@ stdcall GetProcessIoCounters(long ptr) kernelbase.GetProcessIoCounters
@ stdcall -version=0x602+ GetProcessMitigationPolicy(long long ptr long) kernelbase.GetProcessMitigationPolicy
@ stdcall -version=0x601+ GetProcessPreferredUILanguages(long ptr ptr ptr) kernelbase.GetProcessPreferredUILanguages
@ stdcall GetProcessPriorityBoost(long ptr) kernelbase.GetProcessPriorityBoost
@ stdcall GetProcessShutdownParameters(ptr ptr) kernelbase.GetProcessShutdownParameters
@ stdcall GetProcessTimes(long ptr ptr ptr ptr) kernelbase.GetProcessTimes
@ stdcall GetProcessVersion(long) kernelbase.GetProcessVersion
@ stdcall GetProcessWorkingSetSize(long ptr ptr) kernelbase.GetProcessWorkingSetSize
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr long) kernelbase.GetProcessWorkingSetSizeEx
@ stdcall -version=0x600+ GetProductInfo(long long long long ptr) kernelbase.GetProductInfo
@ stdcall GetProfileIntA(str str long) kernelbase.GetProfileIntA
@ stdcall GetProfileIntW(wstr wstr long) kernelbase.GetProfileIntW
@ stdcall GetProfileSectionA(str ptr long) kernelbase.GetProfileSectionA
@ stdcall GetProfileSectionW(wstr ptr long) kernelbase.GetProfileSectionW
@ stdcall GetProfileStringA(str str str ptr long) kernelbase.GetProfileStringA
@ stdcall GetProfileStringW(wstr wstr wstr ptr long) kernelbase.GetProfileStringW
@ stdcall GetQueuedCompletionStatus(long ptr ptr ptr long) kernelbase.GetQueuedCompletionStatus
@ stdcall -version=0x600+ GetQueuedCompletionStatusEx(ptr ptr long ptr long long) kernelbase.GetQueuedCompletionStatusEx
@ stdcall GetShortPathNameA(str ptr long)
@ stdcall GetShortPathNameW(wstr ptr long) kernelbase.GetShortPathNameW
@ stdcall GetStartupInfoA(ptr)
@ stdcall GetStartupInfoW(ptr) kernelbase.GetStartupInfoW
@ stdcall GetStdHandle(long) kernelbase.GetStdHandle
@ stub -version=0x600+ GetStringScripts
@ stdcall GetStringTypeA(long long str long ptr) kernelbase.GetStringTypeA
@ stdcall GetStringTypeExA(long long str long ptr) kernelbase.GetStringTypeExA
@ stdcall GetStringTypeExW(long long wstr long ptr) kernelbase.GetStringTypeExW
@ stdcall GetStringTypeW(long wstr long ptr) kernelbase.GetStringTypeW
@ stdcall -version=0x600+ GetSystemDEPPolicy() kernelbase.GetSystemDEPPolicy
@ stdcall -stub -version=0xA00+ GetSystemCpuSetInformation(ptr long ptr ptr long)
@ stdcall GetSystemDefaultLCID() kernelbase.GetSystemDefaultLCID
@ stdcall GetSystemDefaultLangID() kernelbase.GetSystemDefaultLangID
@ stdcall -stub -version=0x600+ GetSystemDefaultLocaleName(ptr long) kernelbase.GetSystemDefaultLocaleName
@ stdcall GetSystemDefaultUILanguage() kernelbase.GetSystemDefaultUILanguage
@ stdcall GetSystemDirectoryA(ptr long) kernelbase.GetSystemDirectoryA
@ stdcall GetSystemDirectoryW(ptr long) kernelbase.GetSystemDirectoryW
@ stdcall GetSystemFileCacheSize(ptr ptr ptr) kernelbase.GetSystemFileCacheSize
@ stdcall GetSystemFirmwareTable(long long ptr long) kernelbase.GetSystemFirmwareTable
@ stdcall GetSystemInfo(ptr) kernelbase.GetSystemInfo
@ stdcall GetSystemPowerStatus(ptr) kernelbase.GetSystemPowerStatus
@ stdcall -version=0x600+ GetSystemPreferredUILanguages(long ptr wstr ptr) kernelbase.GetSystemPreferredUILanguages
@ stdcall GetSystemRegistryQuota(ptr ptr) kernelbase.GetSystemRegistryQuota
@ stdcall GetSystemTime(ptr) kernelbase.GetSystemTime
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernelbase.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr) kernelbase.GetSystemTimeAsFileTime
@ stdcall -version=0x602+ GetSystemTimePreciseAsFileTime(ptr) kernelbase.GetSystemTimePreciseAsFileTime
@ stdcall GetSystemTimes(ptr ptr ptr) kernelbase.GetSystemTimes
@ stdcall GetSystemWindowsDirectoryA(ptr long) kernelbase.GetSystemWindowsDirectoryA
@ stdcall GetSystemWindowsDirectoryW(ptr long) kernelbase.GetSystemWindowsDirectoryW
@ stdcall GetSystemWow64DirectoryA(ptr long) kernelbase.GetSystemWow64DirectoryA
@ stdcall GetSystemWow64DirectoryW(ptr long) kernelbase.GetSystemWow64DirectoryW
@ stdcall GetTapeParameters(ptr long ptr ptr) kernelbase.GetTapeParameters
@ stdcall GetTapePosition(ptr long ptr ptr ptr) kernelbase.GetTapePosition
@ stdcall GetTapeStatus(ptr) kernelbase.GetTapeStatus
@ stdcall GetTempFileNameA(str str long ptr) kernelbase.GetTempFileNameA
@ stdcall GetTempFileNameW(wstr wstr long ptr) kernelbase.GetTempFileNameW
@ stdcall -stub -version=0xA00+ GetTempPath2A(long ptr)
@ stdcall -stub -version=0xA00+ GetTempPath2W(long ptr)
@ stdcall GetTempPathA(long ptr) kernelbase.GetTempPathA
@ stdcall GetTempPathW(long ptr) kernelbase.GetTempPathW
@ stdcall GetThreadContext(long ptr) kernelbase.GetThreadContext
@ stdcall -version=0xA00+ GetThreadDescription(long ptr) kernelbase.GetThreadDescription
@ stdcall -stub -version=0x600+ GetThreadErrorMode() kernelbase.GetThreadErrorMode
@ stdcall -version=0x601+ GetThreadGroupAffinity(long ptr) kernelbase.GetThreadGroupAffinity
@ stdcall GetThreadIOPendingFlag(long ptr) kernelbase.GetThreadIOPendingFlag
@ stdcall GetThreadId(ptr) kernelbase.GetThreadId
@ stdcall GetThreadLocale() kernelbase.GetThreadLocale
@ stdcall -version=0x600+ GetThreadPreferredUILanguages(long ptr wstr ptr) kernelbase.GetThreadPreferredUILanguages
@ stdcall GetThreadPriority(long) kernelbase.GetThreadPriority
@ stdcall GetThreadPriorityBoost(long ptr) kernelbase.GetThreadPriorityBoost
@ stdcall GetThreadSelectorEntry(long long ptr) kernelbase.GetThreadSelectorEntry
@ stdcall GetThreadTimes(long ptr ptr ptr ptr) kernelbase.GetThreadTimes
@ stdcall -version=0x600+ GetThreadUILanguage() kernelbase.GetThreadUILanguage
@ stdcall GetTickCount() kernelbase.GetTickCount
@ stdcall -version=0x600+ -ret64 GetTickCount64() kernelbase.GetTickCount64
@ stdcall GetTimeFormatA(long long ptr str ptr long) kernelbase.GetTimeFormatA
@ stdcall -version=0x600+ GetTimeFormatEx(wstr long ptr wstr wstr long) kernelbase.GetTimeFormatEx
@ stdcall GetTimeFormatW(long long ptr wstr ptr long) kernelbase.GetTimeFormatW
@ stdcall GetTimeZoneInformation(ptr) kernelbase.GetTimeZoneInformation
@ stdcall -version=0x600+ GetTimeZoneInformationForYear(long ptr ptr) kernelbase.GetTimeZoneInformationForYear
@ stdcall -version=0x600+ GetUILanguageInfo(long wstr wstr ptr ptr) kernelbase.GetUILanguageInfo
@ stdcall GetUserDefaultLCID() kernelbase.GetUserDefaultLCID
@ stdcall GetUserDefaultLangID() kernelbase.GetUserDefaultLangID
@ stdcall -version=0x600+ GetUserDefaultLocaleName(wstr long) kernelbase.GetUserDefaultLocaleName
@ stdcall GetUserDefaultUILanguage() kernelbase.GetUserDefaultUILanguage
@ stdcall GetUserGeoID(long) kernelbase.GetUserGeoID
@ stdcall -version=0x600+ GetUserPreferredUILanguages(long ptr wstr ptr) kernelbase.GetUserPreferredUILanguages
@ stdcall GetVDMCurrentDirectories(long long) kernelbase.GetVDMCurrentDirectories
@ stdcall GetVersion() kernelbase.GetVersion
@ stdcall GetVersionExA(ptr) kernelbase.GetVersionExA
@ stdcall GetVersionExW(ptr) kernelbase.GetVersionExW
@ stdcall GetVolumeInformationA(str ptr long ptr ptr ptr ptr long) kernelbase.GetVolumeInformationA
@ stdcall GetVolumeInformationByHandleW(ptr ptr long ptr ptr ptr ptr long) kernelbase.GetVolumeInformationByHandleW
@ stdcall GetVolumeInformationW(wstr ptr long ptr ptr ptr ptr long) kernelbase.GetVolumeInformationW
@ stdcall GetVolumeNameForVolumeMountPointA(str ptr long) kernelbase.GetVolumeNameForVolumeMountPointA
@ stdcall GetVolumeNameForVolumeMountPointW(wstr ptr long) kernelbase.GetVolumeNameForVolumeMountPointW
@ stdcall GetVolumePathNameA(str ptr long) kernelbase.GetVolumePathNameA
@ stdcall GetVolumePathNameW(wstr ptr long) kernelbase.GetVolumePathNameW
@ stdcall GetVolumePathNamesForVolumeNameA(str str long ptr) kernelbase.GetVolumePathNamesForVolumeNameA
@ stdcall GetVolumePathNamesForVolumeNameW(wstr wstr long ptr) kernelbase.GetVolumePathNamesForVolumeNameW
@ stdcall GetWindowsDirectoryA(ptr long) kernelbase.GetWindowsDirectoryA
@ stdcall GetWindowsDirectoryW(ptr long) kernelbase.GetWindowsDirectoryW
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr) kernelbase.GetWriteWatch
@ stdcall GlobalAddAtomA(str) 
@ stdcall GlobalAddAtomW(wstr)
@ stdcall GlobalAlloc(long long) kernelbase.GlobalAlloc
@ stdcall GlobalCompact(long) kernelbase.GlobalCompact
@ stdcall GlobalDeleteAtom(long)
@ stdcall GlobalFindAtomA(str) 
@ stdcall GlobalFindAtomW(wstr)
@ stdcall GlobalFix(long) kernelbase.GlobalFix
@ stdcall GlobalFlags(long) kernelbase.GlobalFlags
@ stdcall GlobalFree(long) kernelbase.GlobalFree
@ stdcall GlobalGetAtomNameA(long ptr long)
@ stdcall GlobalGetAtomNameW(long ptr long)
@ stdcall GlobalHandle(ptr) kernelbase.GlobalHandle
@ stdcall GlobalLock(long) kernelbase.GlobalLock
@ stdcall GlobalMemoryStatus(ptr) kernelbase.GlobalMemoryStatus
@ stdcall GlobalMemoryStatusEx(ptr) kernelbase.GlobalMemoryStatusEx
@ stdcall GlobalReAlloc(long long long) kernelbase.GlobalReAlloc
@ stdcall GlobalSize(long) kernelbase.GlobalSize
@ stdcall GlobalUnWire(long) kernelbase.GlobalUnWire
@ stdcall GlobalUnfix(long) kernelbase.GlobalUnfix
@ stdcall GlobalUnlock(long) kernelbase.GlobalUnlock
@ stdcall GlobalWire(long) kernelbase.GlobalWire
@ stdcall Heap32First(ptr long long) kernelbase.Heap32First
@ stdcall Heap32ListFirst(long ptr) kernelbase.Heap32ListFirst
@ stdcall Heap32ListNext(long ptr) kernelbase.Heap32ListNext
@ stdcall Heap32Next(ptr) kernelbase.Heap32Next
@ stdcall HeapAlloc(long long long) kernelbase.HeapAlloc
@ stdcall HeapCompact(long long) kernelbase.HeapCompact
@ stdcall HeapCreate(long long long) kernelbase.HeapCreate
@ stdcall -version=0x351-0x502 HeapCreateTagsW(ptr long wstr wstr) kernelbase.HeapCreateTagsW
@ stdcall HeapDestroy(long) kernelbase.HeapDestroy
@ stdcall -version=0x351-0x502 HeapExtend(long long ptr long) kernelbase.HeapExtend
@ stdcall HeapFree(long long long) kernelbase.HeapFree
@ stdcall HeapLock(long) kernelbase.HeapLock
@ stdcall HeapQueryInformation(long long ptr long ptr) kernelbase.HeapQueryInformation
@ stdcall -version=0x351-0x502 HeapQueryTagW(long long long long ptr) kernelbase.HeapQueryTagW
@ stdcall HeapReAlloc(long long ptr long) kernelbase.HeapReAlloc
@ stdcall HeapSetInformation(ptr long ptr long) kernelbase.HeapSetInformation
@ stdcall HeapSize(long long ptr) kernelbase.HeapSize
@ stdcall HeapSummary(long long ptr) kernelbase.HeapSummary
@ stdcall HeapUnlock(long) kernelbase.HeapUnlock
@ stdcall -version=0x351-0x502 HeapUsage(long long long long ptr) kernelbase.HeapUsage
@ stdcall HeapValidate(long long ptr) kernelbase.HeapValidate
@ stdcall HeapWalk(long ptr) kernelbase.HeapWalk
@ stdcall -stub -version=0x600+ IdnToAscii(long wstr long ptr long) kernelbase.IdnToAscii
@ stdcall -stub -version=0x600+ IdnToNameprepUnicode(long wstr long ptr long) kernelbase.IdnToNameprepUnicode
@ stdcall -stub -version=0x600+ IdnToUnicode(long wstr long ptr long) kernelbase.IdnToUnicode
@ stdcall InitAtomTable(long)
@ stdcall -version=0x600+ InitOnceBeginInitialize(ptr long ptr ptr) kernelbase.InitOnceBeginInitialize
@ stdcall -version=0x600+ InitOnceComplete(ptr long ptr) kernelbase.InitOnceComplete
@ stdcall -version=0x600+ InitOnceExecuteOnce(ptr ptr ptr ptr) kernelbase.InitOnceExecuteOnce
@ stdcall -version=0x600+ InitOnceInitialize(ptr) kernelbase.InitOnceInitialize
@ stdcall -version=0x600+ InitializeConditionVariable(ptr) kernelbase.InitializeConditionVariable
@ stdcall InitializeCriticalSection(ptr) kernelbase.InitializeCriticalSection
@ stdcall InitializeCriticalSectionAndSpinCount(ptr long) kernelbase.InitializeCriticalSectionAndSpinCount
@ stdcall -version=0x600+ InitializeCriticalSectionEx(ptr long long) kernelbase.InitializeCriticalSectionEx
@ stdcall -version=0x600+ InitializeProcThreadAttributeList(ptr long long ptr) kernelbase.InitializeProcThreadAttributeList
@ stdcall InitializeSListHead(ptr) kernelbase.InitializeSListHead
@ stdcall -version=0x600+ InitializeSRWLock(ptr) kernelbase.InitializeSRWLock
@ stdcall -arch=i386 -ret64 InterlockedCompareExchange64(ptr double double) kernelbase.InterlockedCompareExchange64
@ stdcall -arch=i386 InterlockedCompareExchange(ptr long long) kernelbase.InterlockedCompareExchange
@ stdcall -arch=i386 InterlockedDecrement(ptr) kernelbase.InterlockedDecrement
@ stdcall -arch=i386 InterlockedExchange(ptr long) kernelbase.InterlockedExchange
@ stdcall -arch=i386 InterlockedExchangeAdd(ptr long) kernelbase.InterlockedExchangeAdd
@ stdcall InterlockedFlushSList(ptr) kernelbase.InterlockedFlushSList
@ stdcall -arch=i386 InterlockedIncrement(ptr) kernelbase.InterlockedIncrement
@ stdcall InterlockedPopEntrySList(ptr) kernelbase.InterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) kernelbase.InterlockedPushEntrySList
@ fastcall -version=0x600+ InterlockedPushListSList(ptr ptr ptr long) kernelbase.InterlockedPushListSList
@ stdcall InvalidateConsoleDIBits(long long) kernelbase.InvalidateConsoleDIBits
@ stdcall IsBadCodePtr(ptr) kernelbase.IsBadCodePtr
@ stdcall IsBadHugeReadPtr(ptr long) kernelbase.IsBadHugeReadPtr
@ stdcall IsBadHugeWritePtr(ptr long) kernelbase.IsBadHugeWritePtr
@ stdcall IsBadReadPtr(ptr long) kernelbase.IsBadReadPtr
@ stdcall IsBadStringPtrA(ptr long) kernelbase.IsBadStringPtrA
@ stdcall IsBadStringPtrW(ptr long) kernelbase.IsBadStringPtrW
@ stdcall IsBadWritePtr(ptr long) kernelbase.IsBadWritePtr
@ stub -version=0x600+ IsCalendarLeapDay
@ stub -version=0x600+ IsCalendarLeapMonth
@ stub -version=0x600+ IsCalendarLeapYear
@ stdcall IsDBCSLeadByte(long) kernelbase.IsDBCSLeadByte
@ stdcall IsDBCSLeadByteEx(long long) kernelbase.IsDBCSLeadByteEx
@ stdcall IsDebuggerPresent() kernelbase.IsDebuggerPresent
@ stdcall IsNLSDefinedString(long long ptr long long) kernelbase.IsNLSDefinedString
@ stdcall -stub -version=0x600+ IsNormalizedString(long wstr long) kernelbase.IsNormalizedString
@ stdcall IsProcessInJob(long long ptr) kernelbase.IsProcessInJob
@ stdcall IsProcessorFeaturePresent(long) kernelbase.IsProcessorFeaturePresent
@ stdcall IsSystemResumeAutomatic() kernelbase.IsSystemResumeAutomatic
@ stdcall -version=0x600+ IsThreadAFiber() kernelbase.IsThreadAFiber
@ stdcall -version=0x600+ IsThreadpoolTimerSet() ntdll.TpIsTimerSet
@ stdcall IsTimeZoneRedirectionEnabled() kernelbase.IsTimeZoneRedirectionEnabled
@ stub -version=0x600+ IsValidCalDateTime
@ stdcall IsValidCodePage(long) kernelbase.IsValidCodePage
@ stdcall IsValidLanguageGroup(long long) kernelbase.IsValidLanguageGroup
@ stdcall IsValidLocale(long long) kernelbase.IsValidLocale
@ stdcall -version=0x501-0x502 IsValidUILanguage(long) kernelbase.IsValidUILanguage
@ stdcall -stub -version=0x602+ IsValidNLSVersion(long wstr ptr)
@ stdcall IsWow64Process(ptr ptr) kernelbase.IsWow64Process
@ stdcall -version=0xA00+ IsWow64Process2(ptr ptr ptr) kernelbase.IsWow64Process2
@ stdcall -version=0x601+ K32EmptyWorkingSet(long) kernelbase.K32EmptyWorkingSet
@ stdcall -version=0x601+ K32EnumDeviceDrivers(ptr long ptr) kernelbase.K32EnumDeviceDrivers
@ stdcall -version=0x601+ K32EnumPageFilesA(ptr ptr) kernelbase.K32EnumPageFilesA
@ stdcall -version=0x601+ K32EnumPageFilesW(ptr ptr) kernelbase.K32EnumPageFilesW
@ stdcall -version=0x601+ K32EnumProcessModules(long ptr long ptr) kernelbase.K32EnumProcessModules
@ stdcall -version=0x601+ K32EnumProcessModulesEx(long ptr long ptr long) kernelbase.K32EnumProcessModulesEx
@ stdcall -version=0x601+ K32EnumProcesses(ptr long ptr) kernelbase.K32EnumProcesses
@ stdcall -version=0x601+ K32GetDeviceDriverBaseNameA(ptr ptr long) kernelbase.K32GetDeviceDriverBaseNameA
@ stdcall -version=0x601+ K32GetDeviceDriverBaseNameW(ptr ptr long) kernelbase.K32GetDeviceDriverBaseNameW
@ stdcall -version=0x601+ K32GetDeviceDriverFileNameA(ptr ptr long) kernelbase.K32GetDeviceDriverFileNameA
@ stdcall -version=0x601+ K32GetDeviceDriverFileNameW(ptr ptr long) kernelbase.K32GetDeviceDriverFileNameW
@ stdcall -version=0x601+ K32GetMappedFileNameA(long ptr ptr long) kernelbase.K32GetMappedFileNameA
@ stdcall -version=0x601+ K32GetMappedFileNameW(long ptr ptr long) kernelbase.K32GetMappedFileNameW
@ stdcall -version=0x601+ K32GetModuleBaseNameA(long long ptr long) kernelbase.K32GetModuleBaseNameA
@ stdcall -version=0x601+ K32GetModuleBaseNameW(long long ptr long) kernelbase.K32GetModuleBaseNameW
@ stdcall -version=0x601+ K32GetModuleFileNameExA(long long ptr long) kernelbase.K32GetModuleFileNameExA
@ stdcall -version=0x601+ K32GetModuleFileNameExW(long long ptr long) kernelbase.K32GetModuleFileNameExW
@ stdcall -version=0x601+ K32GetModuleInformation(long long ptr long) kernelbase.K32GetModuleInformation
@ stdcall -version=0x601+ K32GetPerformanceInfo(ptr long) kernelbase.K32GetPerformanceInfo
@ stdcall -version=0x601+ K32GetProcessImageFileNameA(long ptr long) kernelbase.K32GetProcessImageFileNameA
@ stdcall -version=0x601+ K32GetProcessImageFileNameW(long ptr long) kernelbase.K32GetProcessImageFileNameW
@ stdcall -version=0x601+ K32GetProcessMemoryInfo(long ptr long) kernelbase.K32GetProcessMemoryInfo
@ stdcall -version=0x601+ K32GetWsChanges(long ptr long) kernelbase.K32GetWsChanges
@ stdcall -stub -version=0x601+ K32GetWsChangesEx(long ptr ptr) kernelbase.K32GetWsChangesEx
@ stdcall -version=0x601+ K32InitializeProcessForWsWatch(long) kernelbase.K32InitializeProcessForWsWatch
@ stdcall -version=0x601+ K32QueryWorkingSet(long ptr long) kernelbase.K32QueryWorkingSet
@ stdcall -version=0x601+ K32QueryWorkingSetEx(long ptr long) kernelbase.K32QueryWorkingSetEx
@ stdcall -version=0x600+ LCIDToLocaleName(long wstr long long) kernelbase.LCIDToLocaleName
@ stdcall LCMapStringA(long long str long ptr long) kernelbase.LCMapStringA
@ stdcall -version=0x600+ LCMapStringEx(long long wstr long ptr long ptr ptr long) kernelbase.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernelbase.LCMapStringW
@ stdcall LZClose(long)
@ stdcall LZCloseFile(long)
@ stdcall LZCopy(long long)
@ stdcall LZCreateFileW(ptr long long long ptr)
@ stdcall LZDone()
@ stdcall LZInit(long)
@ stdcall LZOpenFileA(str ptr long) 
@ stdcall LZOpenFileW(wstr ptr long)
@ stdcall LZRead(long str long)
@ stdcall LZSeek(long long long)
@ stdcall LZStart() 
@ stdcall LeaveCriticalSection(ptr) ntdll.RtlLeaveCriticalSection
@ stdcall -version=0x600+ LeaveCriticalSectionWhenCallbackReturns(ptr ptr) ntdll.TpCallbackLeaveCriticalSectionOnCompletion
@ stdcall -version=0x601+ LoadAppInitDlls() kernelbase.LoadAppInitDlls
@ stdcall LoadLibraryA(str) kernelbase.LoadLibraryA
@ stdcall LoadLibraryExA(str long long) kernelbase.LoadLibraryExA
@ stdcall LoadLibraryExW(wstr long long) kernelbase.LoadLibraryExW
@ stdcall LoadLibraryW(wstr) kernelbase.LoadLibraryW
@ stdcall LoadModule(str ptr) kernelbase.LoadModule
@ stdcall -version=0x602+ LoadPackagedLibrary(wstr long) kernelbase.LoadPackagedLibrary
@ stdcall LoadResource(long long) kernelbase.LoadResource
@ stub -version=0x600+ LoadStringBaseExW
@ stub -version=0x600+ LoadStringBaseW
@ stdcall LocalAlloc(long long) kernelbase.LocalAlloc
@ stdcall LocalCompact(long) kernelbase.LocalCompact
@ stdcall LocalFileTimeToFileTime(ptr ptr) kernelbase.LocalFileTimeToFileTime
@ stdcall LocalFlags(long) kernelbase.LocalFlags
@ stdcall LocalFree(long) kernelbase.LocalFree
@ stdcall LocalHandle(ptr) kernelbase.LocalHandle
@ stdcall LocalLock(long) kernelbase.LocalLock
@ stdcall LocalReAlloc(long long long) kernelbase.LocalReAlloc
@ stdcall LocalShrink(long long) kernelbase.LocalShrink
@ stdcall LocalSize(long) kernelbase.LocalSize
@ stdcall LocalUnlock(long) kernelbase.LocalUnlock
@ stdcall -version=0x600+ LocaleNameToLCID(wstr long) kernelbase.LocaleNameToLCID
@ stdcall LockFile(long long long long long) kernelbase.LockFile
@ stdcall LockFileEx(long long long long long ptr) kernelbase.LockFileEx
@ stdcall LockResource(long) kernelbase.LockResource
@ stdcall MapUserPhysicalPages(ptr long ptr) kernelbase.MapUserPhysicalPages
@ stdcall MapUserPhysicalPagesScatter(ptr long ptr) kernelbase.MapUserPhysicalPagesScatter
@ stdcall MapViewOfFile(long long long long long) kernelbase.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernelbase.MapViewOfFileEx
@ stub -version=0x600+ MapViewOfFileExNuma
@ stdcall -stub -version=0x602+ MapViewOfFileFromApp(long long int64 long)
@ stdcall Module32First(long ptr) kernelbase.Module32First
@ stdcall Module32FirstW(long ptr) kernelbase.Module32FirstW
@ stdcall Module32Next(long ptr) kernelbase.Module32Next
@ stdcall Module32NextW(long ptr) kernelbase.Module32NextW
@ stdcall MoveFileA(str str) kernelbase.MoveFileA
@ stdcall MoveFileExA(str str long) kernelbase.MoveFileExA
@ stdcall MoveFileExW(wstr wstr long) kernelbase.MoveFileExW
@ stdcall -version=0x600+ MoveFileTransactedA(str str ptr ptr long ptr)
@ stdcall -version=0x600+ MoveFileTransactedW(wstr wstr ptr ptr long ptr)
@ stdcall MoveFileW(wstr wstr) kernelbase.MoveFileW
@ stdcall MoveFileWithProgressA(str str ptr ptr long) kernelbase.MoveFileWithProgressA
@ stdcall MoveFileWithProgressW(wstr wstr ptr ptr long) kernelbase.MoveFileWithProgressW
@ stdcall MulDiv(long long long) kernelbase.MulDiv
@ stdcall MultiByteToWideChar(long long str long ptr long) kernelbase.MultiByteToWideChar
@ stdcall NeedCurrentDirectoryForExePathA(str) kernelbase.NeedCurrentDirectoryForExePathA
@ stdcall NeedCurrentDirectoryForExePathW(wstr) kernelbase.NeedCurrentDirectoryForExePathW
@ stub -version=0x600+ NlsCheckPolicy
@ stdcall -version=0x500-0x600 NlsConvertIntegerToString(long long long wstr long) kernelbase.NlsConvertIntegerToString
@ stub -version=0x600+ NlsEventDataDescCreate
@ stdcall NlsGetCacheUpdateCount() kernelbase.NlsGetCacheUpdateCount
@ stdcall -version=0x500-0x502 NlsResetProcessLocale() kernelbase.NlsResetProcessLocale
@ stub -version=0x600+ NlsUpdateLocale
@ stub -version=0x600+ NlsUpdateSystemLocale
@ stub -version=0x600+ NlsWriteEtwEvent
@ stdcall -stub -version=0x600+ NormalizeString(long wstr long ptr long) kernelbase.NormalizeString
@ stdcall -stub -version=0x600+ NotifyUILanguageChange(long wstr wstr long ptr) kernelbase.NotifyUILanguageChange
@ stdcall OpenConsoleW(wstr long long long) kernelbase.OpenConsoleW
@ stdcall -version=0x500-0x502 OpenDataFile(long long) kernelbase.OpenDataFile
@ stdcall OpenEventA(long long str) kernelbase.OpenEventA
@ stdcall OpenEventW(long long wstr) kernelbase.OpenEventW
@ stdcall OpenFile(str ptr long) kernelbase.OpenFile
@ stdcall -version=0x600+ OpenFileById(ptr ptr long long ptr long) kernelbase.OpenFileById
@ stdcall OpenFileMappingA(long long str) kernelbase.OpenFileMappingA
@ stdcall OpenFileMappingW(long long wstr) kernelbase.OpenFileMappingW
@ stdcall OpenJobObjectA(long long str) 
@ stdcall OpenJobObjectW(long long wstr)
@ stdcall OpenMutexA(long long str) kernelbase.OpenMutexA
@ stdcall OpenMutexW(long long wstr) kernelbase.OpenMutexW
@ stub -version=0x600+ OpenPrivateNamespaceA
@ stub -version=0x600+ OpenPrivateNamespaceW
@ stdcall OpenProcess(long long long) kernelbase.OpenProcess
@ stdcall -version=0x601+ OpenProcessToken(long long ptr) kernelbase.OpenProcessToken
@ stdcall OpenProfileUserMapping() kernelbase.OpenProfileUserMapping
@ stdcall OpenSemaphoreA(long long str) kernelbase.OpenSemaphoreA
@ stdcall OpenSemaphoreW(long long wstr) kernelbase.OpenSemaphoreW
@ stdcall OpenThread(long long long) kernelbase.OpenThread
@ stdcall -version=0x601+ OpenThreadToken(long long long ptr) kernelbase.OpenThreadToken
@ stdcall OpenWaitableTimerA(long long str) kernelbase.OpenWaitableTimerA
@ stdcall OpenWaitableTimerW(long long wstr) kernelbase.OpenWaitableTimerW
@ stdcall OutputDebugStringA(str) kernelbase.OutputDebugStringA
@ stdcall OutputDebugStringW(wstr) kernelbase.OutputDebugStringW
@ stdcall PeekConsoleInputA(ptr ptr long ptr) kernelbase.PeekConsoleInputA
@ stdcall PeekConsoleInputW(ptr ptr long ptr) kernelbase.PeekConsoleInputW
@ stdcall PeekNamedPipe(long ptr long ptr ptr ptr) kernelbase.PeekNamedPipe
@ stdcall PostQueuedCompletionStatus(long long ptr ptr) kernelbase.PostQueuedCompletionStatus
@ stdcall -version=0x602+ PackageIdFromFullName(wstr long ptr ptr) kernelbase.PackageIdFromFullName
@ stdcall -version=0x601+ PowerClearRequest(long long)
@ stdcall -version=0x601+ PowerCreateRequest(ptr)
@ stdcall -version=0x601+ PowerSetRequest(long long)
@ stdcall PrepareTape(ptr long long) kernelbase.PrepareTape
@ stdcall PrivCopyFileExW(wstr wstr ptr ptr long long) kernelbase.PrivCopyFileExW
@ stdcall PrivMoveFileIdentityW(long long long) kernelbase.PrivMoveFileIdentityW
@ stdcall Process32First(ptr ptr) kernelbase.Process32First
@ stdcall Process32FirstW(ptr ptr) kernelbase.Process32FirstW
@ stdcall Process32Next(ptr ptr) kernelbase.Process32Next
@ stdcall Process32NextW(ptr ptr) kernelbase.Process32NextW
@ stdcall ProcessIdToSessionId(long ptr) kernelbase.ProcessIdToSessionId
@ stdcall PulseEvent(long) kernelbase.PulseEvent
@ stdcall PurgeComm(long long) kernelbase.PurgeComm
@ stdcall -stub -version=0x600+ QueryActCtxSettingsW(long ptr wstr wstr ptr long ptr)
@ stdcall QueryActCtxW(long ptr ptr long ptr long ptr) kernelbase.QueryActCtxW
@ stdcall QueryDepthSList(ptr) ntdll.RtlQueryDepthSList
@ stdcall QueryDosDeviceA(str ptr long) kernelbase.QueryDosDeviceA
@ stdcall QueryDosDeviceW(wstr ptr long) kernelbase.QueryDosDeviceW
@ stdcall -version=0x600+ QueryFullProcessImageNameA(ptr long str ptr) kernelbase.QueryFullProcessImageNameA
@ stdcall -version=0x600+ QueryFullProcessImageNameW(ptr long wstr ptr) kernelbase.QueryFullProcessImageNameW
@ stdcall -version=0x600+ QueryIdleProcessorCycleTime(ptr ptr) kernelbase.QueryIdleProcessorCycleTime
@ stdcall -version=0x601+ QueryIdleProcessorCycleTimeEx(long ptr ptr) kernelbase.QueryIdleProcessorCycleTimeEx
@ stdcall QueryInformationJobObject(long long ptr long ptr)
@ stdcall QueryMemoryResourceNotification(ptr ptr) kernelbase.QueryMemoryResourceNotification
@ stdcall QueryPerformanceCounter(ptr) kernelbase.QueryPerformanceCounter
@ stdcall QueryPerformanceFrequency(ptr) kernelbase.QueryPerformanceFrequency
@ stub -version=0x600+ QueryProcessAffinityUpdateMode
@ stdcall -version=0x600+ QueryProcessCycleTime(long ptr) kernelbase.QueryProcessCycleTime
@ stdcall -version=0x600+ QueryThreadCycleTime(long ptr) kernelbase.QueryThreadCycleTime
@ stdcall -stub -version=0x601+ QueryThreadProfiling(ptr ptr)
@ stdcall -version=0x601+ QueryThreadpoolStackInformation(ptr ptr) kernelbase.QueryThreadpoolStackInformation
@ stdcall -version=0x601+ -arch=win64 QueryUmsThreadInformation(ptr long ptr long ptr)
@ stdcall QueryUnbiasedInterruptTime(ptr); kernelbase.QueryUnbiasedInterruptTimePrecise
@ stdcall QueueUserAPC(ptr long long) kernelbase.QueueUserAPC
@ stdcall QueueUserWorkItem(ptr ptr long) kernelbase.QueueUserWorkItem
@ stdcall -norelay RaiseException(long long long ptr) kernelbase.RaiseException
@ stdcall ReOpenFile(ptr long long long) kernelbase.ReOpenFile
@ stdcall ReadConsoleA(long ptr long ptr ptr) kernelbase.ReadConsoleA
@ stdcall ReadConsoleInputA(long ptr long ptr) kernelbase.ReadConsoleInputA
@ stdcall ReadConsoleInputExA(long ptr long ptr long) kernelbase.ReadConsoleInputExA
@ stdcall ReadConsoleInputExW(long ptr long ptr long) kernelbase.ReadConsoleInputExW
@ stdcall ReadConsoleInputW(long ptr long ptr) kernelbase.ReadConsoleInputW
@ stdcall ReadConsoleOutputA(long ptr long long ptr) kernelbase.ReadConsoleOutputA
@ stdcall ReadConsoleOutputAttribute(long ptr long long ptr) kernelbase.ReadConsoleOutputAttribute
@ stdcall ReadConsoleOutputCharacterA(long ptr long long ptr) kernelbase.ReadConsoleOutputCharacterA
@ stdcall ReadConsoleOutputCharacterW(long ptr long long ptr) kernelbase.ReadConsoleOutputCharacterW
@ stdcall ReadConsoleOutputW(long ptr long long ptr) kernelbase.ReadConsoleOutputW
@ stdcall ReadConsoleW(long ptr long ptr ptr) kernelbase.ReadConsoleW
@ stdcall ReadDirectoryChangesW(long ptr long long long ptr ptr ptr) kernelbase.ReadDirectoryChangesW
@ stdcall ReadFile(long ptr long ptr ptr) kernelbase.ReadFile
@ stdcall ReadFileEx(long ptr long ptr ptr) kernelbase.ReadFileEx
@ stdcall ReadFileScatter(long ptr long ptr ptr) kernelbase.ReadFileScatter
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernelbase.ReadProcessMemory
# @ stub -version=0x601+ ReadThreadProfilingData
@ stdcall -version=0x601+ RegCloseKey(long) kernelbase.RegCloseKey
@ stdcall -version=0x601+ RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumKeyExA
@ stdcall -version=0x601+ RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumKeyExW
@ stdcall -version=0x601+ RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumValueA
@ stdcall -version=0x601+ RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumValueW
@ stdcall -version=0x601+ RegFlushKey(long) kernelbase.RegFlushKey
@ stdcall -version=0x601+ RegCreateKeyExA(long str long ptr long long ptr ptr ptr) kernelbase.RegCreateKeyExA
@ stdcall -version=0x601+ RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) kernelbase.RegCreateKeyExW
@ stdcall -version=0x601+ RegDeleteKeyExA(long str long long) kernelbase.RegDeleteKeyExA
@ stdcall -version=0x601+ RegDeleteKeyExW(long wstr long long) kernelbase.RegDeleteKeyExW
@ stdcall -version=0x601+ RegDeleteTreeA(long str) kernelbase.RegDeleteTreeA
@ stdcall -version=0x601+ RegDeleteTreeW(long wstr) kernelbase.RegDeleteTreeW
@ stdcall -version=0x601+ RegDeleteValueA(long str) kernelbase.RegDeleteValueA
@ stdcall -version=0x601+ RegDeleteValueW(long wstr) kernelbase.RegDeleteValueW
# stub -version=0x601+ RegDisablePredefinedCacheEx
@ stdcall -version=0x601+ RegGetKeySecurity(long long ptr ptr) kernelbase.RegGetKeySecurity
@ stdcall -version=0x601+ RegGetValueA(long str str long ptr ptr ptr) kernelbase.RegGetValueA
@ stdcall -version=0x601+ RegGetValueW(long wstr wstr long ptr ptr ptr) kernelbase.RegGetValueW
# stub -version=0x601+ RegKrnGetGlobalState
# stub -version=0x601+ RegKrnInitialize
@ stdcall -version=0x601+ RegLoadKeyA(long str str) kernelbase.RegLoadKeyA
@ stdcall -version=0x601+ RegLoadKeyW(long wstr wstr) kernelbase.RegLoadKeyW
@ stdcall -version=0x601+ RegLoadMUIStringA(long str str long ptr long str) kernelbase.RegLoadMUIStringA
@ stdcall -version=0x601+ RegLoadMUIStringW(long wstr wstr long ptr long wstr) kernelbase.RegLoadMUIStringW
@ stdcall -version=0x601+ RegNotifyChangeKeyValue(long long long long long) kernelbase.RegNotifyChangeKeyValue
@ stdcall -version=0x601+ RegOpenCurrentUser(long ptr) kernelbase.RegOpenCurrentUser
@ stdcall -version=0x601+ RegOpenKeyExA(long str long long ptr) kernelbase.RegOpenKeyExA
@ stdcall -version=0x601+ RegOpenKeyExW(long wstr long long ptr) kernelbase.RegOpenKeyExW
@ stdcall -version=0x601+ RegOpenUserClassesRoot(ptr long long ptr) kernelbase.RegOpenUserClassesRoot
@ stdcall -version=0x601+ RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.RegQueryInfoKeyA
@ stdcall -version=0x601+ RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.RegQueryInfoKeyW
@ stdcall -version=0x601+ RegQueryValueExA(long str ptr ptr ptr ptr) kernelbase.RegQueryValueExA
@ stdcall -version=0x601+ RegQueryValueExW(long wstr ptr ptr ptr ptr) kernelbase.RegQueryValueExW
@ stdcall -version=0x601+ RegRestoreKeyA(long str long) kernelbase.RegRestoreKeyA
@ stdcall -version=0x601+ RegRestoreKeyW(long wstr long) kernelbase.RegRestoreKeyW
@ stdcall -version=0x601+ RegSaveKeyExA(long str ptr long) kernelbase.RegSaveKeyExA
@ stdcall -version=0x601+ RegSaveKeyExW(long wstr ptr long) kernelbase.RegSaveKeyExW
@ stdcall -version=0x601+ RegSetKeySecurity(long long ptr) kernelbase.RegSetKeySecurity
@ stdcall -version=0x601+ RegSetValueExA(long str long long ptr long) kernelbase.RegSetValueExA
@ stdcall -version=0x601+ RegSetValueExW(long wstr long long ptr long) kernelbase.RegSetValueExW
@ stdcall -version=0x601+ RegUnLoadKeyA(long str) kernelbase.RegUnLoadKeyA
@ stdcall -version=0x601+ RegUnLoadKeyW(long wstr) kernelbase.RegUnLoadKeyW
@ stdcall -version=0x600+ RegisterApplicationRecoveryCallback(ptr ptr long long)
@ stdcall -version=0x600+ RegisterApplicationRestart(wstr long)
@ stdcall RegisterConsoleIME(ptr ptr) kernelbase.RegisterConsoleIME
@ stdcall RegisterConsoleOS2(long) kernelbase.RegisterConsoleOS2
@ stdcall RegisterConsoleVDM(long long long long long long long long long long long) kernelbase.RegisterConsoleVDM
@ stdcall RegisterWaitForInputIdle(ptr) kernelbase.RegisterWaitForInputIdle
@ stdcall RegisterWaitForSingleObject(ptr long ptr ptr long long) kernelbase.RegisterWaitForSingleObject
@ stdcall RegisterWaitForSingleObjectEx(long ptr ptr long long) kernelbase.RegisterWaitForSingleObjectEx
@ stdcall RegisterWowBaseHandlers(long) kernelbase.RegisterWowBaseHandlers
@ stdcall RegisterWowExec(long) kernelbase.RegisterWowExec
@ stdcall ReleaseActCtx(ptr) kernelbase.ReleaseActCtx
@ stdcall ReleaseMutex(long) kernelbase.ReleaseMutex
@ stdcall -version=0x600+ ReleaseMutexWhenCallbackReturns(ptr long) TpCallbackReleaseMutexOnCompletion
@ stdcall -version=0x600+ ReleaseSRWLockExclusive(ptr) ntdll.RtlReleaseSRWLockExclusive
@ stdcall -version=0x600+ ReleaseSRWLockShared(ptr) ntdll.RtlReleaseSRWLockShared
@ stdcall ReleaseSemaphore(long long ptr) kernelbase.ReleaseSemaphore
@ stub -version=0x600+ ReleaseSemaphoreWhenCallbackReturns
@ stdcall RemoveDirectoryA(str) kernelbase.RemoveDirectoryA
@ stdcall -version=0x600+ RemoveDirectoryTransactedA(str ptr)
@ stdcall -version=0x600+ RemoveDirectoryTransactedW(wstr ptr)
@ stdcall RemoveDirectoryW(wstr) kernelbase.RemoveDirectoryW
@ stdcall RemoveLocalAlternateComputerNameA(str long) kernelbase.RemoveLocalAlternateComputerNameA
@ stdcall RemoveLocalAlternateComputerNameW(wstr long) kernelbase.RemoveLocalAlternateComputerNameW
@ stub -version=0x600+ RemoveSecureMemoryCacheCallback
@ stdcall RemoveVectoredContinueHandler(ptr) ntdll.RtlRemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) ntdll.RtlRemoveVectoredExceptionHandler
@ stdcall ReplaceFile(wstr wstr wstr long ptr ptr) kernelbase.ReplaceFileW
@ stdcall ReplaceFileA(str str str long ptr ptr) kernelbase.ReplaceFileA
@ stdcall ReplaceFileW(wstr wstr wstr long ptr ptr) kernelbase.ReplaceFileW
@ stdcall -version=0x600+ RemoveDllDirectory(ptr) kernelbase.RemoveDllDirectory
# @ stub -version=0x600+  RemoveSecureMemoryCacheCallback
@ stub -version=0x600+ ReplacePartitionUnit
@ stdcall RequestDeviceWakeup(long) kernelbase.RequestDeviceWakeup
@ stdcall RequestWakeupLatency(long) kernelbase.RequestWakeupLatency
@ stdcall ResetEvent(long) kernelbase.ResetEvent
@ stdcall ResetWriteWatch(ptr long) kernelbase.ResetWriteWatch
@ stdcall RestoreLastError(long) ntdll.RtlRestoreLastWin32Error
@ stdcall ResumeThread(long) kernelbase.ResumeThread
@ stdcall -arch=x86_64 RtlAddFunctionTable(ptr long long) ntdll.RtlAddFunctionTable
@ stdcall -register RtlCaptureContext(ptr) ntdll.RtlCaptureContext
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr) ntdll.RtlCaptureStackBackTrace
@ stdcall -arch=x86_64 RtlCompareMemory(ptr ptr ptr) ntdll.RtlCompareMemory
@ stdcall -arch=x86_64 RtlCopyMemory(ptr ptr ptr) ntdll.memcpy
@ stdcall -arch=x86_64 RtlDeleteFunctionTable(ptr) ntdll.RtlDeleteFunctionTable
@ stdcall RtlFillMemory(ptr long long) ntdll.RtlFillMemory
@ stdcall -arch=x86_64 RtlInstallFunctionTableCallback(double double long ptr ptr ptr) ntdll.RtlInstallFunctionTableCallback
@ stdcall -arch=x86_64 RtlLookupFunctionEntry(ptr ptr ptr) ntdll.RtlLookupFunctionEntry
@ stdcall RtlMoveMemory(ptr ptr long) ntdll.RtlMoveMemory
@ stdcall -arch=x86_64 RtlPcToFileHeader(ptr ptr) ntdll.RtlPcToFileHeader
@ stdcall -arch=x86_64 RtlRaiseException(ptr) ntdll.RtlRaiseException
@ stdcall -arch=x86_64 RtlRestoreContext(ptr ptr) ntdll.RtlRestoreContext
@ stdcall RtlUnwind(ptr ptr ptr ptr) ntdll.RtlUnwind
@ stdcall -arch=x86_64 RtlUnwindEx(ptr ptr ptr ptr ptr ptr) ntdll.RtlUnwindEx
@ stdcall -arch=x86_64 RtlVirtualUnwind(long int64 int64 ptr ptr ptr ptr ptr) ntdll.RtlVirtualUnwind
@ stub -version=0xA00+ -arch=x86_64 -norelay RtlIsEcCode(ptr)
@ stdcall RtlZeroMemory(ptr long) ntdll.RtlZeroMemory
@ stdcall ScrollConsoleScreenBufferA(long ptr ptr ptr ptr) kernelbase.ScrollConsoleScreenBufferA
@ stdcall ScrollConsoleScreenBufferW(long ptr ptr ptr ptr) kernelbase.ScrollConsoleScreenBufferW
@ stdcall SearchPathA(str str str long ptr ptr) kernelbase.SearchPathA
@ stdcall SearchPathW(wstr wstr wstr long ptr ptr) kernelbase.SearchPathW
@ stdcall -version=0x500-0x502 SetCPGlobal(long) kernelbase.SetCPGlobal
@ stdcall -version=0x602+ SetCachedSigningLevel(ptr long long long) kernelbase.SetCachedSigningLevel
@ stdcall SetCalendarInfoA(long long long str) kernelbase.SetCalendarInfoA
@ stdcall SetCalendarInfoW(long long long wstr) kernelbase.SetCalendarInfoW
@ stdcall SetClientTimeZoneInformation(ptr) kernelbase.SetClientTimeZoneInformation
@ stdcall SetComPlusPackageInstallStatus(ptr) kernelbase.SetComPlusPackageInstallStatus
@ stdcall SetCommBreak(long) kernelbase.SetCommBreak
@ stdcall SetCommConfig(long ptr long) kernelbase.SetCommConfig
@ stdcall SetCommMask(long ptr) kernelbase.SetCommMask
@ stdcall SetCommState(long ptr) kernelbase.SetCommState
@ stdcall SetCommTimeouts(long ptr) kernelbase.SetCommTimeouts
@ stdcall SetComputerNameA(str) kernelbase.SetComputerNameA
@ stdcall SetComputerNameExA(long str) kernelbase.SetComputerNameExA
@ stdcall SetComputerNameExW(long wstr) kernelbase.SetComputerNameExW
@ stdcall SetComputerNameW(wstr) kernelbase.SetComputerNameW
@ stdcall SetConsoleActiveScreenBuffer(long) kernelbase.SetConsoleActiveScreenBuffer
@ stdcall SetConsoleCP(long) kernelbase.SetConsoleCP
@ stdcall -version=0x351-0x502 SetConsoleCommandHistoryMode(long) kernelbase.SetConsoleCommandHistoryMode
@ stdcall SetConsoleCtrlHandler(ptr long) kernelbase.SetConsoleCtrlHandler
@ stdcall SetConsoleCursor(long long) kernelbase.SetConsoleCursor
@ stdcall SetConsoleCursorInfo(long ptr) kernelbase.SetConsoleCursorInfo
@ stdcall SetConsoleCursorMode(long long long) kernelbase.SetConsoleCursorMode
@ stdcall SetConsoleCursorPosition(long long) kernelbase.SetConsoleCursorPosition
@ stdcall SetConsoleDisplayMode(long long ptr) kernelbase.SetConsoleDisplayMode
@ stdcall SetConsoleFont(long long) kernelbase.SetConsoleFont
@ stdcall SetConsoleHardwareState(long long long) kernelbase.SetConsoleHardwareState
@ stdcall -version=0x600+ SetConsoleHistoryInfo(ptr) kernelbase.SetConsoleHistoryInfo
@ stdcall SetConsoleIcon(ptr) kernelbase.SetConsoleIcon
@ stdcall SetConsoleInputExeNameA(ptr) kernelbase.SetConsoleInputExeNameA
@ stdcall SetConsoleInputExeNameW(ptr) kernelbase.SetConsoleInputExeNameW
@ stdcall SetConsoleKeyShortcuts(long long long long) kernelbase.SetConsoleKeyShortcuts
@ stdcall SetConsoleLocalEUDC(long long long long) kernelbase.SetConsoleLocalEUDC
@ stdcall SetConsoleMaximumWindowSize(long long) kernelbase.SetConsoleMaximumWindowSize
@ stdcall SetConsoleMenuClose(long) kernelbase.SetConsoleMenuClose
@ stdcall SetConsoleMode(long long) kernelbase.SetConsoleMode
@ stdcall SetConsoleNlsMode(long long) kernelbase.SetConsoleNlsMode
@ stdcall SetConsoleNumberOfCommandsA(long long) kernelbase.SetConsoleNumberOfCommandsA
@ stdcall SetConsoleNumberOfCommandsW(long long) kernelbase.SetConsoleNumberOfCommandsW
@ stdcall SetConsoleOS2OemFormat(long) kernelbase.SetConsoleOS2OemFormat
@ stdcall SetConsoleOutputCP(long) kernelbase.SetConsoleOutputCP
@ stdcall SetConsolePalette(long long long) kernelbase.SetConsolePalette
@ stdcall -version=0x600+ SetConsoleScreenBufferInfoEx(ptr ptr) kernelbase.SetConsoleScreenBufferInfoEx
@ stdcall SetConsoleScreenBufferSize(long long) kernelbase.SetConsoleScreenBufferSize
@ stdcall SetConsoleTextAttribute(long long) kernelbase.SetConsoleTextAttribute
@ stdcall SetConsoleTitleA(str) kernelbase.SetConsoleTitleA
@ stdcall SetConsoleTitleW(wstr) kernelbase.SetConsoleTitleW
@ stdcall SetConsoleWindowInfo(long long ptr) kernelbase.SetConsoleWindowInfo
@ stdcall SetCriticalSectionSpinCount(ptr long) ntdll.RtlSetCriticalSectionSpinCount
@ stub -version=0x600+ SetCurrentConsoleFontEx
@ stdcall SetCurrentDirectoryA(str) kernelbase.SetCurrentDirectoryA
@ stdcall SetCurrentDirectoryW(wstr) kernelbase.SetCurrentDirectoryW
@ stdcall SetDefaultCommConfigA(str ptr long) kernelbase.SetDefaultCommConfigA
@ stdcall SetDefaultCommConfigW(wstr ptr long) kernelbase.SetDefaultCommConfigW
@ stdcall -version=0x600+ SetDefaultDllDirectories(long) kernelbase.SetDefaultDllDirectories
@ stdcall SetDllDirectoryA(str) kernelbase.SetDllDirectoryA
@ stdcall SetDllDirectoryW(wstr) kernelbase.SetDllDirectoryW
@ stdcall -version=0x600+ SetDynamicTimeZoneInformation(ptr) kernelbase.SetDynamicTimeZoneInformation
@ stdcall SetEndOfFile(long) kernelbase.SetEndOfFile
@ stdcall SetEnvironmentStringsA(ptr) kernelbase.SetEnvironmentStringsA
@ stdcall SetEnvironmentStringsW(ptr) kernelbase.SetEnvironmentStringsW
@ stdcall SetEnvironmentVariableA(str str) kernelbase.SetEnvironmentVariableA
@ stdcall SetEnvironmentVariableW(wstr wstr) kernelbase.SetEnvironmentVariableW
@ stdcall SetErrorMode(long) kernelbase.SetErrorMode
@ stdcall SetEvent(long) kernelbase.SetEvent
@ stdcall -version=0x600+ SetEventWhenCallbackReturns(ptr long) NTDLL.TpCallbackSetEventOnCompletion
@ stdcall SetFileApisToANSI() kernelbase.SetFileApisToANSI
@ stdcall SetFileApisToOEM() kernelbase.SetFileApisToOEM
@ stdcall SetFileAttributesA(str long) kernelbase.SetFileAttributesA
@ stub -version=0x600+ SetFileAttributesTransactedA
@ stub -version=0x600+ SetFileAttributesTransactedW
@ stdcall SetFileAttributesW(wstr long) kernelbase.SetFileAttributesW
@ stdcall -version=0x600+ SetFileBandwidthReservation(ptr long long long ptr ptr) kernelbase.SetFileBandwidthReservation
@ stdcall SetFileCompletionNotificationModes(ptr long) kernelbase.SetFileCompletionNotificationModes
@ stdcall -version=0x600+ SetFileInformationByHandle(long long ptr long) kernelbase.SetFileInformationByHandle
@ stub -version=0x600+ SetFileIoOverlappedRange
@ stdcall SetFilePointer(long long ptr long) kernelbase.SetFilePointer
@ stdcall SetFilePointerEx(long double ptr long) kernelbase.SetFilePointerEx
@ stdcall SetFileShortNameA(long str) kernelbase.SetFileShortNameA
@ stdcall SetFileShortNameW(long wstr) kernelbase.SetFileShortNameW
@ stdcall SetFileTime(long ptr ptr ptr) kernelbase.SetFileTime
@ stdcall SetFileValidData(long double) kernelbase.SetFileValidData
@ stdcall SetFirmwareEnvironmentVariableA(str str ptr long) kernelbase.SetFirmwareEnvironmentVariableA
@ stdcall -version=0x602+ SetFirmwareEnvironmentVariableExA(str str ptr long long) kernelbase.SetFirmwareEnvironmentVariableExA
@ stdcall -version=0x602+ SetFirmwareEnvironmentVariableExW(str str ptr long long) kernelbase.SetFirmwareEnvironmentVariableExW
@ stdcall SetFirmwareEnvironmentVariableW(wstr wstr ptr long) kernelbase.SetFirmwareEnvironmentVariableW
@ stdcall -i386 SetHandleContext(long long) kernelbase.SetHandleContext
@ stdcall SetHandleCount(long) kernelbase.SetHandleCount
@ stdcall SetHandleInformation(long long long) kernelbase.SetHandleInformation
@ stdcall SetInformationJobObject(long long ptr long)
@ stdcall SetLastConsoleEventActive() kernelbase.SetLastConsoleEventActive ; missing in XP SP3
@ stdcall SetLastError(long) ntdll.RtlSetLastWin32Error
@ stdcall SetLocalPrimaryComputerNameA(long long) kernelbase.SetLocalPrimaryComputerNameA ; missing in XP SP3
@ stdcall SetLocalPrimaryComputerNameW(long long) kernelbase.SetLocalPrimaryComputerNameW ; missing in XP SP3
@ stdcall SetLocalTime(ptr) kernelbase.SetLocalTime
@ stdcall SetLocaleInfoA(long long str) kernelbase.SetLocaleInfoA
@ stdcall SetLocaleInfoW(long long wstr) kernelbase.SetLocaleInfoW
@ stdcall SetMailslotInfo(long long) kernelbase.SetMailslotInfo
@ stdcall SetMessageWaitingIndicator(ptr long) kernelbase.SetMessageWaitingIndicator
@ stub -version=0x600+ SetNamedPipeAttribute
@ stdcall SetNamedPipeHandleState(long long long long) kernelbase.SetNamedPipeHandleState
@ stdcall SetPriorityClass(long long) kernelbase.SetPriorityClass
@ stdcall SetProcessAffinityMask(long long) kernelbase.SetProcessAffinityMask
@ stub -version=0x600+ SetProcessAffinityUpdateMode
@ stdcall -stub -version=0xA00+ SetProcessDefaultCpuSets(ptr ptr long)
@ stdcall -version=0x600+ SetProcessDEPPolicy(ptr) kernelbase.SetProcessDEPPolicy
@ stdcall -version=0x602+ SetProcessInformation(long long ptr long) kernelbase.SetProcessInformation
@ stdcall -stub -version=0x602+ SetProcessMitigationPolicy(long ptr long)
@ stdcall -version=0x601+ SetProcessPreferredUILanguages(long ptr ptr) kernelbase.SetProcessPreferredUILanguages
@ stdcall SetProcessPriorityBoost(long long) kernelbase.SetProcessPriorityBoost
@ stdcall SetProcessShutdownParameters(long long) kernelbase.SetProcessShutdownParameters
@ stdcall SetProcessWorkingSetSize(long long long) kernelbase.SetProcessWorkingSetSize
@ stdcall SetProcessWorkingSetSizeEx(long long long long) kernelbase.SetProcessWorkingSetSizeEx
@ stdcall -stub -version=0x600+ SetSearchPathMode(long)
@ stdcall SetStdHandle(long long) kernelbase.SetStdHandle
@ stub -version=0x600+ SetStdHandleEx
@ stdcall SetSystemFileCacheSize(long long long) kernelbase.SetSystemFileCacheSize
@ stdcall SetSystemPowerState(long long) kernelbase.SetSystemPowerState
@ stdcall SetSystemTime(ptr) kernelbase.SetSystemTime
@ stdcall SetSystemTimeAdjustment(long long) kernelbase.SetSystemTimeAdjustment
@ stdcall SetTapeParameters(ptr long ptr) kernelbase.SetTapeParameters
@ stdcall SetTapePosition(ptr long long long long long) kernelbase.SetTapePosition
@ stdcall SetTermsrvAppInstallMode(long) kernelbase.SetTermsrvAppInstallMode
@ stdcall SetThreadAffinityMask(long long) kernelbase.SetThreadAffinityMask
@ stdcall SetThreadContext(long ptr) kernelbase.SetThreadContext
@ stdcall -version=0xA00+ SetThreadDescription(ptr wstr) kernelbase.SetThreadDescription
@ stdcall -version=0x600+ SetThreadErrorMode(long ptr) kernelbase.SetThreadErrorMode
@ stdcall SetThreadExecutionState(long) kernelbase.SetThreadExecutionState
@ stdcall -version=0x601+ SetThreadGroupAffinity(long ptr ptr) kernelbase.SetThreadGroupAffinity
@ stdcall SetThreadIdealProcessor(long long) kernelbase.SetThreadIdealProcessor
@ stdcall -version=0x601+ SetThreadIdealProcessorEx(long ptr ptr) kernelbase.SetThreadIdealProcessorEx
@ stdcall -version=0x602+ SetThreadInformation(long long ptr long) kernelbase.SetThreadInformation
@ stdcall SetThreadLocale(long) kernelbase.SetThreadLocale
@ stdcall -version=0x600+ SetThreadPreferredUILanguages(long wstr ptr) kernelbase.SetThreadPreferredUILanguages
@ stdcall SetThreadPriority(long long) kernelbase.SetThreadPriority
@ stdcall SetThreadPriorityBoost(long long) kernelbase.SetThreadPriorityBoost
@ stub -version=0xA00+ SetThreadSelectedCpuSets
@ stdcall SetThreadStackGuarantee(ptr) kernelbase.SetThreadStackGuarantee
@ stdcall -version=0x601+ SetThreadToken(ptr ptr) kernelbase.SetThreadToken
@ stdcall SetThreadUILanguage(long) kernelbase.SetThreadUILanguage
@ stdcall -version=0x601+ SetThreadpoolStackInformation(ptr ptr) kernelbase.SetThreadpoolStackInformation
@ stdcall -version=0x600+ SetThreadpoolThreadMaximum(ptr long) NTDLL.TpSetPoolMaxThreads
@ stdcall -version=0x600+ SetThreadpoolThreadMinimum(ptr long) NTDLL.TpSetPoolMinThreads
@ stdcall -version=0x600+ SetThreadpoolTimer(ptr ptr long long) NTDLL.TpSetTimer
@ stdcall -version=0x600+ SetThreadpoolWait(ptr long ptr) NTDLL.TpSetWait
@ stdcall SetTimeZoneInformation(ptr) kernelbase.SetTimeZoneInformation
@ stdcall SetTimerQueueTimer(long ptr ptr long long long) kernelbase.SetTimerQueueTimer
@ stdcall -version=0x601+ -arch=win64 SetUmsThreadInformation(ptr long ptr long)
@ stdcall SetUnhandledExceptionFilter(ptr) kernelbase.SetUnhandledExceptionFilter
@ stdcall SetUserGeoID(long) kernelbase.SetUserGeoID
@ stdcall -stub -version=0xA00+ SetUserGeoName(wstr)
@ stdcall SetVDMCurrentDirectories(long long) kernelbase.SetVDMCurrentDirectories
@ stdcall SetVolumeLabelA(str str) kernelbase.SetVolumeLabelA
@ stdcall SetVolumeLabelW(wstr wstr) kernelbase.SetVolumeLabelW
@ stdcall SetVolumeMountPointA(str str) kernelbase.SetVolumeMountPointA
@ stdcall SetVolumeMountPointW(wstr wstr) kernelbase.SetVolumeMountPointW
@ stdcall SetWaitableTimer(long ptr long ptr ptr long) kernelbase.SetWaitableTimer
@ stdcall -version=0x601+ SetWaitableTimerEx(long ptr long ptr ptr ptr long) kernelbase.SetWaitableTimerEx
@ stdcall SetupComm(long long long) kernelbase.SetupComm
@ stdcall ShowConsoleCursor(long long) kernelbase.ShowConsoleCursor
@ stdcall SignalObjectAndWait(long long long long) kernelbase.SignalObjectAndWait
@ stdcall SizeofResource(long long) kernelbase.SizeofResource
@ stdcall Sleep(long) kernelbase.Sleep
@ stdcall -version=0x600+ SleepConditionVariableCS(ptr ptr long) kernelbase.SleepConditionVariableCS
@ stdcall -version=0x600+ SleepConditionVariableSRW(ptr ptr long long) kernelbase.SleepConditionVariableSRW
@ stdcall SleepEx(long long) kernelbase.SleepEx
@ stdcall -version=0x600+ StartThreadpoolIo(ptr) NTDLL.TpStartAsyncIoOperation
@ stdcall -version=0x600+ SubmitThreadpoolWork(ptr)  NTDLL.TpPostWork
@ stdcall SuspendThread(long) kernelbase.SuspendThread
@ stdcall SwitchToFiber(ptr) kernelbase.SwitchToFiber
@ stdcall SwitchToThread() kernelbase.SwitchToThread
@ stdcall SystemTimeToFileTime(ptr ptr) kernelbase.SystemTimeToFileTime
@ stdcall SystemTimeToTzSpecificLocalTime(ptr ptr ptr) kernelbase.SystemTimeToTzSpecificLocalTime
@ stdcall TerminateJobObject(ptr long)
@ stdcall TerminateProcess(ptr long) kernelbase.TerminateProcess
@ stdcall TerminateThread(ptr long) kernelbase.TerminateThread
@ stdcall TermsrvAppInstallMode() kernelbase.TermsrvAppInstallMode
@ stdcall Thread32First(long ptr) kernelbase.Thread32First
@ stdcall Thread32Next(long ptr) kernelbase.Thread32Next
@ stdcall RaiseFailFastException(ptr ptr long) kernelbase.RaiseFailFastException
@ stdcall TlsAlloc() kernelbase.TlsAlloc
@ stdcall TlsFree(long) kernelbase.TlsFree
@ stdcall -norelay TlsGetValue(long) kernelbase.TlsGetValue
@ stdcall -norelay TlsSetValue(long ptr) kernelbase.TlsSetValue
@ stdcall Toolhelp32ReadProcessMemory(long ptr ptr long ptr) kernelbase.Toolhelp32ReadProcessMemory
@ stdcall TransactNamedPipe(long ptr long ptr long ptr ptr) kernelbase.TransactNamedPipe
@ stdcall TransmitCommChar(long long) kernelbase.TransmitCommChar
@ stdcall -version=0x601+ TryAcquireSRWLockExclusive(ptr) NTDLL.RtlTryAcquireSRWLockExclusive
@ stdcall -version=0x601+ TryAcquireSRWLockShared(ptr) NTDLL.RtlTryAcquireSRWLockShared
@ stdcall TryEnterCriticalSection(ptr) ntdll.RtlTryEnterCriticalSection
@ stdcall TrySubmitThreadpoolCallback(ptr ptr ptr) kernelbase.TrySubmitThreadpoolCallback
@ stdcall TzSpecificLocalTimeToSystemTime(ptr ptr ptr) kernelbase.TzSpecificLocalTimeToSystemTime
@ stdcall UTRegister(long str str str ptr ptr ptr) kernelbase.UTRegister
@ stdcall UTUnRegister(long) kernelbase.UTUnRegister
@ stdcall UnhandledExceptionFilter(ptr) kernelbase.UnhandledExceptionFilter
@ stdcall UnlockFile(long long long long long) kernelbase.UnlockFile
@ stdcall UnlockFileEx(long long long long ptr) kernelbase.UnlockFileEx
@ stdcall UnmapViewOfFile(ptr) kernelbase.UnmapViewOfFile
@ stub -version=0x600+ UnregisterApplicationRecoveryCallback 
@ stdcall -version=0x600+ UnregisterApplicationRestart()
@ stdcall UnregisterConsoleIME() kernelbase.UnregisterConsoleIME
@ stdcall UnregisterWait(long) kernelbase.UnregisterWait
@ stdcall UnregisterWaitEx(long long) kernelbase.UnregisterWaitEx
@ stub -version=0x600+ UpdateCalendarDayOfWeek
@ stdcall -version=0x600+ UpdateProcThreadAttribute(ptr long ptr ptr ptr ptr ptr) kernelbase.UpdateProcThreadAttribute
@ stdcall UpdateResourceA(long str str long ptr long)
@ stdcall UpdateResourceW(long wstr wstr long ptr long)
@ stdcall VDMConsoleOperation(long long) kernelbase.VDMConsoleOperation
@ stdcall VDMOperationStarted(long) kernelbase.VDMOperationStarted
@ stdcall -version=0x500-0x502 ValidateLCType(long long ptr ptr) kernelbase.ValidateLCType
@ stdcall -version=0x500-0x502 ValidateLocale(long) kernelbase.ValidateLocale
@ stdcall VerLanguageNameA(long str long) kernelbase.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernelbase.VerLanguageNameW
@ stdcall -ret64 VerSetConditionMask(long long long long) ntdll.VerSetConditionMask
@ stdcall VerifyConsoleIoHandle(long) kernelbase.VerifyConsoleIoHandle
@ stub -version=0x600+ VerifyScripts
@ stdcall VerifyVersionInfoA(long long double)
@ stdcall VerifyVersionInfoW(long long double) kernelbase.VerifyVersionInfoW
@ stdcall VirtualAlloc(ptr long long long) kernelbase.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernelbase.VirtualAllocEx
@ stdcall -version=0xA00+ VirtualAlloc2(long ptr long long long ptr long) kernelbase.VirtualAlloc2
@ stub -version=0x600+ VirtualAllocExNuma
@ stdcall VirtualFree(ptr long long) kernelbase.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernelbase.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernelbase.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernelbase.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernelbase.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernelbase.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernelbase.VirtualQueryEx
@ stdcall VirtualUnlock(ptr long) kernelbase.VirtualUnlock
@ stdcall WTSGetActiveConsoleSessionId() kernelbase.WTSGetActiveConsoleSessionId
@ stdcall WaitCommEvent(long ptr ptr) kernelbase.WaitCommEvent
@ stdcall WaitForDebugEvent(ptr long) kernelbase.WaitForDebugEvent
@ stdcall -version=0xA00+ WaitForDebugEventEx(ptr long) kernelbase.WaitForDebugEventEx
@ stdcall WaitForMultipleObjects(long ptr long long) kernelbase.WaitForMultipleObjects
@ stdcall WaitForMultipleObjectsEx(long ptr long long long) kernelbase.WaitForMultipleObjectsEx
@ stdcall WaitForSingleObject(long long) kernelbase.WaitForSingleObject
@ stdcall WaitForSingleObjectEx(long long long) kernelbase.WaitForSingleObjectEx
@ stdcall -version=0x600+ WaitForThreadpoolIoCallbacks(ptr long) NTDLL.TpWaitForIoCompletion
@ stdcall -version=0x600+ WaitForThreadpoolTimerCallbacks(ptr long) NTDLL.TpWaitForTimer
@ stdcall -version=0x600+ WaitForThreadpoolWaitCallbacks(ptr long) NTDLL.TpWaitForWait
@ stdcall -version=0x600+ WaitForThreadpoolWorkCallbacks(ptr long) NTDLL.TpWaitForWork
@ stdcall WaitNamedPipeA(str long) kernelbase.WaitNamedPipeA
@ stdcall WaitNamedPipeW(wstr long) kernelbase.WaitNamedPipeW
@ stdcall -version=0x600+ WakeAllConditionVariable(ptr) ntdll.RtlWakeAllConditionVariable
@ stdcall -version=0x600+ WakeConditionVariable(ptr) ntdll.RtlWakeConditionVariable
@ stdcall -version=0x600+ WerGetFlags(ptr ptr) kernelbase.WerGetFlags
@ stdcall -version=0x600+ WerRegisterFile(wstr long long) kernelbase.WerRegisterFile
@ stdcall -version=0x600+ WerRegisterMemoryBlock(ptr long) kernelbase.WerRegisterMemoryBlock
@ stdcall -version=0x601+ WerRegisterRuntimeExceptionModule(wstr ptr) kernelbase.WerRegisterRuntimeExceptionModule
@ stdcall -version=0x600+ WerSetFlags(long) kernelbase.WerSetFlags
@ stdcall -version=0x600+ WerUnregisterFile(wstr) kernelbase.WerUnregisterFile
@ stdcall -version=0x600+ WerUnregisterMemoryBlock(ptr) kernelbase.WerUnregisterMemoryBlock
@ stdcall -version=0x601+ WerUnregisterRuntimeExceptionModule(wstr ptr) kernelbase.WerUnregisterRuntimeExceptionModule
@ stdcall WideCharToMultiByte(long long wstr long ptr long ptr ptr) kernelbase.WideCharToMultiByte
@ stdcall WinExec(str long) kernelbase.WinExec
@ stdcall Wow64DisableWow64FsRedirection(ptr) kernelbase.Wow64DisableWow64FsRedirection
@ stdcall Wow64EnableWow64FsRedirection(long) kernelbase.Wow64EnableWow64FsRedirection
@ stdcall -version=0x600+ Wow64GetThreadContext(long ptr) kernelbase.Wow64GetThreadContext
@ stdcall Wow64RevertWow64FsRedirection(ptr) kernelbase.Wow64RevertWow64FsRedirection
@ stdcall -version=0x600+ Wow64SetThreadContext(long ptr) kernelbase.Wow64SetThreadContext
@ stub -version=0x600+ Wow64SuspendThread
@ stdcall WriteConsoleA(long ptr long ptr ptr) kernelbase.WriteConsoleA
@ stdcall WriteConsoleInputA(long ptr long ptr) kernelbase.WriteConsoleInputA
@ stdcall WriteConsoleInputVDMA(long long long long) kernelbase.WriteConsoleInputVDMA
@ stdcall WriteConsoleInputVDMW(long long long long) kernelbase.WriteConsoleInputVDMW
@ stdcall WriteConsoleInputW(long ptr long ptr) kernelbase.WriteConsoleInputW
@ stdcall WriteConsoleOutputA(long ptr long long ptr) kernelbase.WriteConsoleOutputA
@ stdcall WriteConsoleOutputAttribute(long ptr long long ptr) kernelbase.WriteConsoleOutputAttribute
@ stdcall WriteConsoleOutputCharacterA(long ptr long long ptr) kernelbase.WriteConsoleOutputCharacterA
@ stdcall WriteConsoleOutputCharacterW(long ptr long long ptr) kernelbase.WriteConsoleOutputCharacterW
@ stdcall WriteConsoleOutputW(long ptr long long ptr) kernelbase.WriteConsoleOutputW
@ stdcall WriteConsoleW(long ptr long ptr ptr) kernelbase.WriteConsoleW
@ stdcall WriteFile(long ptr long ptr ptr) kernelbase.WriteFile
@ stdcall WriteFileEx(long ptr long ptr ptr) kernelbase.WriteFileEx
@ stdcall WriteFileGather(long ptr long ptr ptr) kernelbase.WriteFileGather
@ stdcall WritePrivateProfileSectionA(str str str) kernelbase.WritePrivateProfileSectionA
@ stdcall WritePrivateProfileSectionW(wstr wstr wstr) kernelbase.WritePrivateProfileSectionW
@ stdcall WritePrivateProfileStringA(str str str str) kernelbase.WritePrivateProfileStringA
@ stdcall WritePrivateProfileStringW(wstr wstr wstr wstr) kernelbase.WritePrivateProfileStringW
@ stdcall WritePrivateProfileStructA(str str ptr long str) kernelbase.WritePrivateProfileStructA
@ stdcall WritePrivateProfileStructW(wstr wstr ptr long wstr) kernelbase.WritePrivateProfileStructW
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernelbase.WriteProcessMemory
@ stdcall WriteProfileSectionA(str str) kernelbase.WriteProfileSectionA
@ stdcall WriteProfileSectionW(str str) kernelbase.WriteProfileSectionW
@ stdcall WriteProfileStringA(str str str) kernelbase.WriteProfileStringA
@ stdcall WriteProfileStringW(wstr wstr wstr) kernelbase.WriteProfileStringW
@ stdcall WriteTapemark(ptr long long long) kernelbase.WriteTapemark
@ stdcall ZombifyActCtx(ptr) kernelbase.ZombifyActCtx
@ stdcall -arch=x86_64,arm64 __C_specific_handler() ntdll.__C_specific_handler
@ stdcall -arch=x86_64,arm64 __chkstk() ntdll.__chkstk
;@ stdcall -arch=x86_64 __misaligned_access() ntdll.__misaligned_access
@ stdcall _hread(long ptr long) kernelbase._hread
@ stdcall _hwrite(long ptr long) kernelbase._hwrite
@ stdcall _lclose(long) kernelbase._lclose
@ stdcall _lcreat(str long) kernelbase._lcreat
@ stdcall _llseek(long long long) kernelbase._llseek
@ stdcall -arch=x86_64,arm64 _local_unwind() ntdll._local_unwind
@ stdcall _lopen(str long) kernelbase._lopen
@ stdcall _lread(long ptr long) kernelbase._hread
@ stdcall _lwrite(long ptr long) kernelbase._hwrite
@ stdcall lstrcat(str str) kernelbase.lstrcatA
@ stdcall lstrcatA(str str) kernelbase.lstrcatA
@ stdcall lstrcatW(wstr wstr) kernelbase.lstrcatW
@ stdcall lstrcmp(str str) kernelbase.lstrcmpA
@ stdcall lstrcmpA(str str) kernelbase.lstrcmpA
@ stdcall lstrcmpW(wstr wstr) kernelbase.lstrcmpW
@ stdcall lstrcmpi(str str) kernelbase.lstrcmpiA 
@ stdcall lstrcmpiA(str str) kernelbase.lstrcmpiA
@ stdcall lstrcmpiW(wstr wstr) kernelbase.lstrcmpiW
@ stdcall lstrcpy(ptr str) kernelbase.lstrcpyA
@ stdcall lstrcpyA(ptr str) kernelbase.lstrcpyA
@ stdcall lstrcpyW(ptr wstr) kernelbase.lstrcpyW
@ stdcall lstrcpyn(ptr str long) lstrcpynA
@ stdcall lstrcpynA(ptr str long) kernelbase.lstrcpynA
@ stdcall lstrcpynW(ptr wstr long) kernelbase.lstrcpynW
@ stdcall lstrlen(str) kernelbase.lstrlen
@ stdcall lstrlenA(str) kernelbase.lstrlenA
@ stdcall lstrlenW(wstr) kernelbase.lstrlenW
;@ stdcall -arch=x86_64 uaw_lstrcmpW(wstr wstr)
;@ stdcall -arch=x86_64 uaw_lstrcmpiW(wstr wstr)
;@ stdcall -arch=x86_64 uaw_lstrlenW(wstr)
;@ stdcall -arch=x86_64 uaw_wcschr(wstr long)
;@ stdcall -arch=x86_64 uaw_wcscpy(ptr wstr)
;@ stdcall -arch=x86_64 uaw_wcsicmp(wstr wstr)
;@ stdcall -arch=x86_64 uaw_wcslen(wstr)
;@ stdcall -arch=x86_64 uaw_wcsrchr(wstr long)


@ stdcall -stub ResolveLocaleName(wstr ptr long)

@ stdcall -version=0x601+ -arch=i386,x86_64 LocateXStateFeature(ptr long ptr) kernelbase.LocateXStateFeature
@ stdcall -arch=i386,x86_64 GetXStateFeaturesMask(ptr ptr) kernelbase.GetXStateFeaturesMask
@ stdcall -ret64 -arch=i386,x86_64 GetEnabledXStateFeatures() kernelbase.GetEnabledXStateFeatures
@ stdcall -arch=i386,x86_64 SetXStateFeaturesMask(ptr int64) kernelbase.SetXStateFeaturesMask
@ stdcall -version=0x601+ InitializeContext(ptr long ptr ptr) kernelbase.InitializeContext
@ stdcall -stub GetThreadIdealProcessorEx(long ptr)
@ stdcall GetCurrentApplicationUserModelId(ptr wstr) kernelbase.GetCurrentApplicationUserModelId

@ stdcall -version=0xA00+ InitializeContext2(ptr long ptr ptr int64) kernelbase.InitializeContext2
@ stdcall SetUserDefaultGeoName(wstr) kernelbase.SetUserDefaultGeoName


@ stdcall -version=0x603+ WaitOnAddress(ptr ptr long long) kernelbase.WaitOnAddress
@ stdcall -version=0x603+ WakeByAddressAll(ptr) kernelbase.WakeByAddressAll
@ stdcall -version=0x603+ WakeByAddressSingle(ptr) kernelbase.WakeByAddressSingle
