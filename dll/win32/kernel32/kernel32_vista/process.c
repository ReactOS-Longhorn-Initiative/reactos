
#include <k32_vista.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(wine);

#ifndef __REACTOS__
/***********************************************************************
 *           CmdBatNotification   (KERNEL32.@)
 *
 * Notifies the system that a batch file has started or finished.
 *
 * PARAMS
 *  bBatchRunning [I]  TRUE if a batch file has started or 
 *                     FALSE if a batch file has finished executing.
 *
 * RETURNS
 *  Unknown.
 */
BOOL WINAPI CmdBatNotification( BOOL bBatchRunning )
{
    FIXME("%d\n", bBatchRunning);
    return FALSE;
}
#endif

/***********************************************************************
 *           RegisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRestart(PCWSTR pwzCommandLine, DWORD dwFlags)
{
    FIXME("(%s,%ld)\n", debugstr_w(pwzCommandLine), dwFlags);

    return S_OK;
}


/**********************************************************************
 *           ApplicationRecoveryFinished     (KERNEL32.@)
 */
VOID WINAPI ApplicationRecoveryFinished(BOOL success)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/**********************************************************************
 *           ApplicationRecoveryInProgress     (KERNEL32.@)
 */
HRESULT WINAPI ApplicationRecoveryInProgress(PBOOL canceled)
{
    FIXME(":%p stub\n", canceled);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return E_FAIL;
}

/**********************************************************************
 *           RegisterApplicationRecoveryCallback     (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRecoveryCallback(APPLICATION_RECOVERY_CALLBACK callback, PVOID param, DWORD pingint, DWORD flags)
{
    FIXME("%p, %p, %ld, %ld: stub, faking success\n", callback, param, pingint, flags);
    return S_OK;
}





























 /* I dont like this code seem IDA-ish ill use it for now but im not gonna commit it back to master */
BOOL WINAPI GetProcessGroupAffinity(HANDLE hProcess, PUSHORT GroupCount, PUSHORT GroupArray)
{
   PROCESS_INFORMATION ProcessInfo;
   NTSTATUS Status;
   *GroupCount = 1;
   if(!GroupCount)
   {
     SetLastError(ERROR_INSUFFICIENT_BUFFER);
	 return FALSE;
   }
   Status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &ProcessInfo, sizeof(PROCESS_INFORMATION), NULL);
   if(Status < 0)
   {
	 BaseSetLastNTError(Status);
     return FALSE;
   }
   GroupArray[0] = 1;
   return TRUE;
}
BOOL WINAPI
SetThreadGroupAffinity(HANDLE hThread, const GROUP_AFFINITY *GroupAffinity, PGROUP_AFFINITY PreviousGroupAffinity)
{
		DWORD_PTR ProcessAffinityMask;
		DWORD_PTR SystemAffinityMask;
		DWORD Pid;
		HANDLE hProcess;
		if(PreviousGroupAffinity)
		{
			Pid = GetProcessIdOfThread(hThread);
		    hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pid);
			if(!hProcess)
				return FALSE;
			GetProcessAffinityMask(hProcess, &ProcessAffinityMask, &SystemAffinityMask);
			PreviousGroupAffinity->Mask = ProcessAffinityMask;
			CloseHandle(hProcess);
		}
		return SetThreadAffinityMask(hThread, GroupAffinity->Mask);
}
BOOL WINAPI
GetThreadGroupAffinity(HANDLE hThread, PGROUP_AFFINITY GroupAffinity)
{
		DWORD_PTR ProcessAffinityMask;
		DWORD_PTR SystemAffinityMask;
		DWORD Pid;
		HANDLE hProcess;
			Pid = GetProcessIdOfThread(hThread);
		    hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pid);
			if(!hProcess)
				return FALSE;
			if(!GetProcessAffinityMask(hProcess, &ProcessAffinityMask, &SystemAffinityMask))
				return FALSE;
			GroupAffinity->Mask = ProcessAffinityMask;
			GroupAffinity->Group = 0;
                        CloseHandle(hProcess);
		return TRUE;
}
BOOL WINAPI
GetThreadIdealProcessorEx(HANDLE hThread, PPROCESSOR_NUMBER lpIdealProcessor)
{
		lpIdealProcessor->Number = SetThreadIdealProcessor(hThread, MAXIMUM_PROCESSORS);
		lpIdealProcessor->Group = 0;
		if(lpIdealProcessor->Number == -1)
			return FALSE;
		else
			return TRUE;
}
BOOL WINAPI
SetThreadIdealProcessorEx(HANDLE hThread, PPROCESSOR_NUMBER lpIdealProcessor,
		PPROCESSOR_NUMBER lpPreviousIdealProcessor)
{
	lpPreviousIdealProcessor->Number = SetThreadIdealProcessor(hThread, lpIdealProcessor->Number);
        lpPreviousIdealProcessor->Group = 0;
	lpIdealProcessor->Group = 0;
	if(lpPreviousIdealProcessor->Number == -1)
		return FALSE;
	else
	return TRUE;
}
BOOL
WINAPI
GetLogicalProcessorInformation(OUT PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
                               IN OUT PDWORD ReturnLength);
BOOL WINAPI
GetLogicalProcessorInformationEx(LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer, PDWORD ReturnedLength)
{
 PSYSTEM_LOGICAL_PROCESSOR_INFORMATION BufferClassic;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Ptr;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX NewBuffer;
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Temp;
	SYSTEM_INFO SysInfo;
	DWORD ReturnedLengthClassic;
	DWORD RequiredLength_All;
	DWORD Offset;
#ifdef _X86_
	long ActiveProcessorMask;
#else
	__int64 ActiveProcessorMask;
#endif
	BOOL StructisCopied;
	BOOL BufferTooSmall = FALSE;
	if(!ReturnedLength || RelationshipType < 0 || (RelationshipType > 7 && RelationshipType != RelationAll))
	{
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
    if(Buffer == NULL)
	{
		BufferTooSmall = TRUE;
	}
    BufferClassic = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, sizeof(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION));
	ReturnedLengthClassic = 0;
	while(!GetLogicalProcessorInformation(BufferClassic, &ReturnedLengthClassic))
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
		RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, BufferClassic);
		BufferClassic = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, ReturnedLengthClassic);
		}
		else
        {
		    return FALSE;
        }
	}
	GetSystemInfo(&SysInfo);
	ActiveProcessorMask = SysInfo.dwActiveProcessorMask;
	Ptr = BufferClassic;
	NewBuffer = Buffer;
	Offset = 0;
	RequiredLength_All = 0;
	//NewOffset = 0;
	while (Offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= ReturnedLengthClassic)
	{
		StructisCopied = FALSE;
		switch(Ptr->Relationship)
		{
			case RelationNumaNode:
		     if(RelationshipType == RelationNumaNode || RelationshipType == RelationAll)
		     {
				 StructisCopied = TRUE;
				 Temp.Relationship = RelationNumaNode;
				 #ifdef _X86_
				 Temp.Size = 76;
				 RequiredLength_All += 76;
			     #else
				 Temp.Size = 80;
				 RequiredLength_All += 80;
				 #endif
				 Temp.NumaNode.NodeNumber = Ptr->NumaNode.NodeNumber;
				 Temp.NumaNode.GroupMask.Mask = (KAFFINITY) ActiveProcessorMask;
				 Temp.NumaNode.GroupMask.Group = 0;
				 if(RequiredLength_All > *ReturnedLength || BufferTooSmall)
				 BufferTooSmall = TRUE;
			     else
				 *NewBuffer = Temp;
			 }
			 break;
			 case RelationProcessorCore:
			 case RelationProcessorPackage:
			 if(RelationshipType == RelationProcessorCore || RelationshipType == RelationAll || RelationshipType == RelationProcessorPackage)
			 {
				 StructisCopied = TRUE;
				 if(Ptr->Relationship == RelationProcessorCore)
				 {
                 Temp.Relationship = RelationProcessorCore;
                 }
                 else
                 {
				 Temp.Relationship = RelationProcessorPackage;
                 }
                 #ifdef _X86_
				 Temp.Size = 76;
				 RequiredLength_All += 76;
			     #else
				 Temp.Size = 80;
				 RequiredLength_All += 80;
				 #endif
				 Temp.Processor.Flags = Ptr->ProcessorCore.Flags;
				// Temp.Processor.EfficiencyClass = 0;
				 //Temp.Processor.GroupCount = 1;
				 Temp.Processor.GroupMask->Mask =(KAFFINITY) ActiveProcessorMask;
				 Temp.Processor.GroupMask->Group = 0;
				 if(RequiredLength_All > *ReturnedLength || BufferTooSmall)
				 BufferTooSmall = TRUE;
			     else
				 *NewBuffer = Temp;
			 }
			 break;
			 case RelationCache:
			 if(RelationshipType == RelationCache || RelationshipType == RelationAll)
			 {
				 StructisCopied = TRUE;
				 Temp.Relationship = RelationCache;
				 Temp.Cache.Level = Ptr->Cache.Level;
				 #ifdef _X86_
				 Temp.Size = 76; //52
				 RequiredLength_All += 76;
				 #else
				 Temp.Size = 80;
				 RequiredLength_All += 80;
                 #endif
				 Temp.Cache.Associativity = Ptr->Cache.Associativity;
				 Temp.Cache.LineSize = Ptr->Cache.LineSize;
				 Temp.Cache.CacheSize = Ptr->Cache.Size;
				 Temp.Cache.Type = Ptr->Cache.Type;
				// Temp.Cache.GroupCount = 1;
				 Temp.Cache.GroupMask.Mask = (KAFFINITY) ActiveProcessorMask;
				 Temp.Cache.GroupMask.Group = 0;
				 if(RequiredLength_All > *ReturnedLength || BufferTooSmall)
				 BufferTooSmall = TRUE;
			     else
				 *NewBuffer = Temp;
			 }
             case RelationGroup:
                break;
             case RelationAll:
                break;
			 break;
		}
		Offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		//NewOffset = RequiredLength + Offset;
       // to make sure it doesn't advance the SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pointer
                              // if a SYSTEM_LOGICAL_PROCESSOR_INFORMATION struct contains irrelevant information
		if (StructisCopied && !BufferTooSmall)
		NewBuffer++;
		Ptr++;
	}
	RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, BufferClassic);
	if(RelationshipType == RelationGroup || RelationshipType == RelationAll)
	{
		Temp.Relationship = RelationGroup;
		#ifdef _X86_
		Temp.Size = 76;
		RequiredLength_All += 76;
	    #else
		Temp.Size = 80;
		RequiredLength_All += 80;
		#endif
		Temp.Group.MaximumGroupCount = 1;
		Temp.Group.ActiveGroupCount = 1;
		Temp.Group.GroupInfo->MaximumProcessorCount = SysInfo.dwNumberOfProcessors;
		Temp.Group.GroupInfo->ActiveProcessorCount = SysInfo.dwNumberOfProcessors;
		Temp.Group.GroupInfo->ActiveProcessorMask = (KAFFINITY) ActiveProcessorMask;
		if(RequiredLength_All > *ReturnedLength || BufferTooSmall)
		BufferTooSmall = TRUE;
	    else
		*NewBuffer = Temp;
	}
	  *ReturnedLength = RequiredLength_All;
	   if(BufferTooSmall)
	   {
		   RtlSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
		   return FALSE;
	   }
	   return TRUE;
}



static SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *get_logical_processor_info(void)
{
    DWORD size = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    GetLogicalProcessorInformationEx( RelationGroup, NULL, &size );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return NULL;
    if (!(info = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;
    if (!GetLogicalProcessorInformationEx( RelationGroup, info, &size ))
    {
        HeapFree( GetProcessHeap(), 0, info );
        return NULL;
    }
    return info;
}


/***********************************************************************
 *           GetActiveProcessorGroupCount (KERNEL32.@)
 */
WORD WINAPI GetActiveProcessorGroupCount(void)
{
    WORD groups;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    TRACE("()\n");

    if (!(info = get_logical_processor_info())) return 0;

    groups = info->Group.ActiveGroupCount;

    HeapFree(GetProcessHeap(), 0, info);
    return groups;
}

/***********************************************************************
 *           GetActiveProcessorCount (KERNEL32.@)
 */
DWORD WINAPI GetActiveProcessorCount(WORD group)
{
    DWORD cpus = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    TRACE("(0x%x)\n", group);

    if (!(info = get_logical_processor_info())) return 0;

    if (group == ALL_PROCESSOR_GROUPS)
    {
        for (group = 0; group < info->Group.ActiveGroupCount; group++)
            cpus += info->Group.GroupInfo[group].ActiveProcessorCount;
    }
    else
    {
        if (group < info->Group.ActiveGroupCount)
            cpus = info->Group.GroupInfo[group].ActiveProcessorCount;
    }

    HeapFree(GetProcessHeap(), 0, info);
    return cpus;
}

/***********************************************************************
 *           GetMaximumProcessorCount (KERNEL32.@)
 */
DWORD WINAPI GetMaximumProcessorCount(WORD group)
{
    DWORD cpus = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    TRACE("(0x%x)\n", group);

    if (!(info = get_logical_processor_info())) return 0;

    if (group == ALL_PROCESSOR_GROUPS)
    {
        for (group = 0; group < info->Group.MaximumGroupCount; group++)
            cpus += info->Group.GroupInfo[group].MaximumProcessorCount;
    }
    else
    {
        if (group < info->Group.MaximumGroupCount)
            cpus = info->Group.GroupInfo[group].MaximumProcessorCount;
    }

    HeapFree(GetProcessHeap(), 0, info);
    return cpus;
}

/***********************************************************************
 *           GetMaximumProcessorGroupCount (KERNEL32.@)
 */
WORD WINAPI GetMaximumProcessorGroupCount(void)
{
    WORD groups;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    TRACE("()\n");

    if (!(info = get_logical_processor_info())) return 0;

    groups = info->Group.MaximumGroupCount;

    HeapFree(GetProcessHeap(), 0, info);
    return groups;
}


#ifndef __REACTOS__
/**********************************************************************
 *           GetNumaNodeProcessorMask     (KERNEL32.@)
 */
BOOL WINAPI GetNumaNodeProcessorMask(UCHAR node, PULONGLONG mask)
{
    FIXME("(%c %p): stub\n", node, mask);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaAvailableMemoryNode     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNode(UCHAR node, PULONGLONG available_bytes)
{
    FIXME("(%c %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

/**********************************************************************
 *           GetNumaAvailableMemoryNodeEx     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNodeEx(USHORT node, PULONGLONG available_bytes)
{
    FIXME("(%hu %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#ifndef __REACTOS__
/***********************************************************************
 *           GetNumaProcessorNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNode(UCHAR processor, PUCHAR node)
{
    TRACE("(%d, %p)\n", processor, node);

    if (processor < GetActiveProcessorCount(0))
    {
        *node = 0;
        return TRUE;
    }

    *node = 0xFF;
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}
#endif

/***********************************************************************
 *           GetNumaProcessorNodeEx (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNodeEx(PPROCESSOR_NUMBER processor, PUSHORT node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProximityNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProximityNode(ULONG  proximity_id, PUCHAR node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *           UnregisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI UnregisterApplicationRestart(void)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return S_OK;
}

/***********************************************************************
 *           CreateUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsCompletionList(PUMS_COMPLETION_LIST *list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           CreateUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsThreadContext(PUMS_CONTEXT *ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsCompletionList(PUMS_COMPLETION_LIST list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsThreadContext(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DequeueUmsCompletionListItems   (KERNEL32.@)
 */
BOOL WINAPI DequeueUmsCompletionListItems(void *list, DWORD timeout, PUMS_CONTEXT *ctx)
{
    FIXME( "%p,%08lx,%p: stub\n", list, timeout, ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           EnterUmsSchedulingMode   (KERNEL32.@)
 */
BOOL WINAPI EnterUmsSchedulingMode(UMS_SCHEDULER_STARTUP_INFO *info)
{
    FIXME( "%p: stub\n", info );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           ExecuteUmsThread   (KERNEL32.@)
 */
BOOL WINAPI ExecuteUmsThread(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetCurrentUmsThread   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetCurrentUmsThread(void)
{
    FIXME( "stub\n" );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetNextUmsListItem   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetNextUmsListItem(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return NULL;
}

/***********************************************************************
 *           GetUmsCompletionListEvent   (KERNEL32.@)
 */
BOOL WINAPI GetUmsCompletionListEvent(PUMS_COMPLETION_LIST list, HANDLE *event)
{
    FIXME( "%p,%p: stub\n", list, event );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           QueryUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI QueryUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                       void *buf, ULONG length, ULONG *ret_length)
{
    FIXME( "%p,%08x,%p,%08lx,%p: stub\n", ctx, class, buf, length, ret_length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           SetUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI SetUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                     void *buf, ULONG length)
{
    FIXME( "%p,%08x,%p,%08lx: stub\n", ctx, class, buf, length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           UmsThreadYield   (KERNEL32.@)
 */
BOOL WINAPI UmsThreadYield(void *param)
{
    FIXME( "%p: stub\n", param );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}
