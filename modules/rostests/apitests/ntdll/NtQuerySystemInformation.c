/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtQuerySystemInformation
 * COPYRIGHT:   Copyright 2019 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"

LARGE_INTEGER g_CurrentSystemTime;

void Test_SystemBasicInformation(void) // 0
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    ULONG ReturnLength;

    memset(&BasicInfo, 0xCC, sizeof(BasicInfo));

    Status = NtQuerySystemInformation(SystemBasicInformation, &BasicInfo, sizeof(BasicInfo), &ReturnLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, sizeof(BasicInfo));
    if (!NT_SUCCESS(Status))
    {
        skip("NtQuerySystemInformation failed with status 0x%08x\n", Status);
        return;
    }

    ok_eq_ulong(BasicInfo.Reserved, 0);
    ok((BasicInfo.TimerResolution >= 100000) && (BasicInfo.TimerResolution <= 2100000), "TimerResolution out of expected range: %lu\n", BasicInfo.TimerResolution);
    ok_eq_ulong(BasicInfo.PageSize, PAGE_SIZE);
    ok((BasicInfo.NumberOfPhysicalPages > 0) && (BasicInfo.NumberOfPhysicalPages <= 0x2000000), "NumberOfPhysicalPages out of expected range: %lu\n", BasicInfo.NumberOfPhysicalPages);
    ok((BasicInfo.LowestPhysicalPageNumber < BasicInfo.NumberOfPhysicalPages), "LowestPhysicalPageNumber (0x%lx) >= NumberOfPhysicalPages (0x%lx)\n", BasicInfo.LowestPhysicalPageNumber, BasicInfo.NumberOfPhysicalPages);
    ok(BasicInfo.HighestPhysicalPageNumber >= BasicInfo.NumberOfPhysicalPages, "HighestPhysicalPageNumber (0x%lx) < NumberOfPhysicalPages (0x%lx)\n", BasicInfo.HighestPhysicalPageNumber, BasicInfo.NumberOfPhysicalPages);
    ok(BasicInfo.HighestPhysicalPageNumber <= 0x2000000, "HighestPhysicalPageNumber (0x%lx) > 0x200000\n", BasicInfo.HighestPhysicalPageNumber);
    ok_eq_ulong(BasicInfo.AllocationGranularity, 64 * 1024);
    ok_eq_ulongptr(BasicInfo.MinimumUserModeAddress, 0x10000);
#ifdef _WIN64
    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
        ok_eq_ulongptr(BasicInfo.MaximumUserModeAddress, 0x7FFFFFFEFFFF);
    else
        ok_eq_ulongptr(BasicInfo.MaximumUserModeAddress, 0x07FFFFFEFFFF);
#else
    ok_eq_ulongptr(BasicInfo.MaximumUserModeAddress, 0x7FFEFFFF);
#endif
    ok_eq_ulongptr(BasicInfo.ActiveProcessorsAffinityMask, ((KAFFINITY)1 << BasicInfo.NumberOfProcessors) - 1);
    ok((BasicInfo.NumberOfProcessors > 0) && (BasicInfo.NumberOfProcessors <= sizeof(KAFFINITY) * 8), "NumberOfProcessors out of expected range: %lu\n", BasicInfo.NumberOfProcessors);
}

void Test_SystemEmulationBasicInformation(void) // 62
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo, EmulationInfo;
    ULONG ReturnLength;

    Status = NtQuerySystemInformation(SystemBasicInformation, &BasicInfo, sizeof(BasicInfo), &ReturnLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("NtQuerySystemInformation failed with status 0x%08x\n", Status);
        return;
    }

    Status = NtQuerySystemInformation(SystemEmulationBasicInformation, &EmulationInfo, sizeof(EmulationInfo), &ReturnLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, sizeof(EmulationInfo));
    if (!NT_SUCCESS(Status))
    {
        skip("NtQuerySystemInformation failed with status 0x%08x\n", Status);
        return;
    }

    ok_eq_ulong(EmulationInfo.Reserved, BasicInfo.Reserved);
    ok_eq_ulong(EmulationInfo.TimerResolution, BasicInfo.TimerResolution);
    ok_eq_ulong(EmulationInfo.PageSize, BasicInfo.PageSize);
    ok_eq_ulong(EmulationInfo.NumberOfPhysicalPages, BasicInfo.NumberOfPhysicalPages);
    ok_eq_ulong(EmulationInfo.LowestPhysicalPageNumber, BasicInfo.LowestPhysicalPageNumber);
    ok_eq_ulong(EmulationInfo.HighestPhysicalPageNumber, BasicInfo.HighestPhysicalPageNumber);
    ok_eq_ulong(EmulationInfo.AllocationGranularity, BasicInfo.AllocationGranularity);
    ok_eq_ulongptr(EmulationInfo.MinimumUserModeAddress, BasicInfo.MinimumUserModeAddress);
    ok_eq_ulongptr(EmulationInfo.MaximumUserModeAddress, BasicInfo.MaximumUserModeAddress);
    ok_eq_ulongptr(EmulationInfo.ActiveProcessorsAffinityMask, BasicInfo.ActiveProcessorsAffinityMask);
    ok_eq_ulong(EmulationInfo.NumberOfProcessors, BasicInfo.NumberOfProcessors);
}

void ValidateProcessInfo(
    PSYSTEM_PROCESS_INFORMATION ProcessInfo,
    BOOL IsExtended)
{
    ok_eq_pointer(ALIGN_UP_POINTER_BY(ProcessInfo, sizeof(ULONG64)), ProcessInfo);

    ok(ProcessInfo->NumberOfThreads > 0, "Process with zero threads found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->WorkingSetPrivateSize.QuadPart > 0, "Process with zero WorkingSetPrivateSize found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->HardFaultCount > 0, "Process with zero HardFaultCount found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->NumberOfThreadsHighWatermark > 0, "Process with zero NumberOfThreadsHighWatermark found: %wZ\n", &ProcessInfo->ImageName);
    if (is_reactos() || (GetNTVersion() >= _WIN32_WINNT_WIN7))
        ok(ProcessInfo->CycleTime > 0, "Process with zero CycleTime found: %wZ\n", &ProcessInfo->ImageName);
    //else
      //  ok(ProcessInfo->CycleTime == 0, "Process with non-zero CycleTime found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->UserTime.QuadPart > 0, "Process with zero UserTime found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->KernelTime.QuadPart > 0, "Process with zero KernelTime found: %wZ\n", &ProcessInfo->ImageName);
    ok((ProcessInfo->BasePriority >= 0) && (ProcessInfo->BasePriority <= 31), "Process with invalid BasePriority %u found: %wZ\n", ProcessInfo->BasePriority, &ProcessInfo->ImageName);
    //ok(ProcessInfo->HandleCount > 0, "Process with zero HandleCount found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->SessionId != 0, "Process with zero SessionId found: %wZ\n", &ProcessInfo->ImageName);
    ok(ProcessInfo->PageFaultCount > 0, "Process with zero PageFaultCount found: %wZ\n", &ProcessInfo->ImageName);
    ok(ProcessInfo->PeakWorkingSetSize > 0, "Process with zero PeakWorkingSetSize found: %wZ\n", &ProcessInfo->ImageName);
    ok(ProcessInfo->WorkingSetSize > 0, "Process with zero WorkingSetSize found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->QuotaPeakNonPagedPoolUsage > 0, "Process with zero QuotaPeakNonPagedPoolUsage found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->QuotaNonPagedPoolUsage > 0, "Process with zero QuotaNonPagedPoolUsage found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->ReadOperationCount.QuadPart > 0, "Process with zero ReadOperationCount found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->WriteOperationCount.QuadPart > 0, "Process with zero WriteOperationCount found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->OtherOperationCount.QuadPart > 0, "Process with zero OtherOperationCount found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->ReadTransferCount.QuadPart > 0, "Process with zero ReadTransferCount found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->WriteTransferCount.QuadPart > 0, "Process with zero WriteTransferCount found: %wZ\n", &ProcessInfo->ImageName);
    //ok(ProcessInfo->OtherTransferCount.QuadPart > 0, "Process with zero OtherTransferCount found: %wZ\n", &ProcessInfo->ImageName);

    if (ProcessInfo->UniqueProcessId == UlongToHandle(0))
    {
        ok(ProcessInfo->ImageName.Length == 0, "Idle process with non-empty ImageName found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->InheritedFromUniqueProcessId == 0, "Idle process with non-zero InheritedFromUniqueProcessId found\n");
        ok(ProcessInfo->HandleCount == 0, "Idle process with non-zero HandleCount found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->QuotaPeakPagedPoolUsage == 0, "Idle process with non-zero QuotaPeakPagedPoolUsage found\n");
        ok(ProcessInfo->QuotaPagedPoolUsage == 0, "Idle process with non-zero QuotaPagedPoolUsage found: %wZ\n", &ProcessInfo->ImageName);
    }
    else if (ProcessInfo->UniqueProcessId == UlongToHandle(4))
    {
        static const UNICODE_STRING SystemString = RTL_CONSTANT_STRING(L"System");
        ok(RtlEqualUnicodeString(&ProcessInfo->ImageName, &SystemString, TRUE), "Process with PID 4 has incorrect ImageName: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->CreateTime.QuadPart > g_CurrentSystemTime.QuadPart - /* 365 days */ 10000000LL * 60 * 60 * 24 * 365, "Process with CreateTime too far in the past found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->CreateTime.QuadPart <= g_CurrentSystemTime.QuadPart, "Process with CreateTime in the future found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->InheritedFromUniqueProcessId == 0, "System process with non-zero InheritedFromUniqueProcessId found\n");
        ok(ProcessInfo->QuotaPeakPagedPoolUsage == 0, "System process with non-zero QuotaPeakPagedPoolUsage found\n");
        ok(ProcessInfo->QuotaPagedPoolUsage == 0, "System process with non-zero QuotaPagedPoolUsage found\n");
        ok(ProcessInfo->VirtualSize > 0, "System process with zero VirtualSize found\n");
        ok(ProcessInfo->PeakVirtualSize > 0, "System process with zero PeakVirtualSize found\n");
        ok(ProcessInfo->PrivatePageCount > 0, "System process with zero PrivatePageCount found\n");
    }
    else
    {
        ok(ProcessInfo->ImageName.Length > 0, "Process with empty ImageName found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->CreateTime.QuadPart > g_CurrentSystemTime.QuadPart - /* 365 days */ 10000000LL * 60 * 60 * 24 * 365, "Process with CreateTime too far in the past found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->CreateTime.QuadPart <= g_CurrentSystemTime.QuadPart, "Process with CreateTime in the future found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->InheritedFromUniqueProcessId != 0, "Process with zero InheritedFromUniqueProcessId found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->QuotaPeakPagedPoolUsage > 0, "Process with zero QuotaPeakPagedPoolUsage found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->QuotaPagedPoolUsage > 0, "Process with zero QuotaPagedPoolUsage found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->VirtualSize > 0, "Process with zero VirtualSize found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->PeakVirtualSize > 0, "Process with zero PeakVirtualSize found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->PagefileUsage > 0, "Process with zero PagefileUsage found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->PeakPagefileUsage > 0, "Process with zero PeakPagefileUsage found: %wZ\n", &ProcessInfo->ImageName);
        ok(ProcessInfo->PrivatePageCount > 0, "Process with zero PrivatePageCount found: %wZ\n", &ProcessInfo->ImageName);
    }
}

void ValidateThreadInfo(
    ULONG Line,
    PSYSTEM_THREAD_INFORMATION ThreadInfo,
    PSYSTEM_PROCESS_INFORMATION ProcessInfo)
{
    ok_eq_pointer(ALIGN_UP_POINTER_BY(ThreadInfo, sizeof(ULONG64)), ThreadInfo);

    ok(ThreadInfo->ClientId.UniqueProcess == ProcessInfo->UniqueProcessId, "Thread with mismatched UniqueProcess found in process %wZ\n", &ProcessInfo->ImageName);
    ok(ThreadInfo->Priority >= 0, "Thread with negative Priority found in process %wZ\n", &ProcessInfo->ImageName);
    ok(ThreadInfo->BasePriority >= 0, "Thread with negative BasePriority found in process %wZ\n", &ProcessInfo->ImageName);
    ok(ThreadInfo->ContextSwitches >= 0, "Thread with negative ContextSwitches found in process %wZ\n", &ProcessInfo->ImageName);
    ok(ThreadInfo->ThreadState >= 0, "Thread with negative ThreadState found in process %wZ\n", &ProcessInfo->ImageName);
    ok(ThreadInfo->WaitReason >= 0, "Thread with negative WaitReason found in process %wZ\n", &ProcessInfo->ImageName);
    //ok_eq_ulong(ThreadInfo->PadPadAlignment, 0);
    //ok(ThreadInfo->WaitTime == 0, "Idle process thread with non-zero WaitTime found\n");

    if (ProcessInfo->UniqueProcessId == UlongToHandle(0))
    {
        ok(ThreadInfo->CreateTime.QuadPart == 0, "Idle process thread with non-zero CreateTime found\n");
        ok(ThreadInfo->KernelTime.QuadPart > 0, "Idle process thread with zero KernelTime found\n");
        ok(ThreadInfo->UserTime.QuadPart == 0, "Idle process thread with non-zero UserTime found\n");
        ok(ThreadInfo->ClientId.UniqueThread == 0, "Idle process thread with non-zero UniqueThread found\n");
        ok(ThreadInfo->StartAddress != NULL, "Idle process thread with NULL StartAddress found\n");
    }
    else if (ProcessInfo->UniqueProcessId == UlongToHandle(4))
    {
        ok(ThreadInfo->ClientId.UniqueThread != 0, "System process thread with zero UniqueThread found\n");
        ok(ThreadInfo->WaitTime != 0, "System process thread with non-zero WaitTime found\n");
        ok(ThreadInfo->StartAddress != NULL, "System process thread with NULL StartAddress found\n");
    }
    else
    {
        ok(ThreadInfo->CreateTime.QuadPart >= ProcessInfo->CreateTime.QuadPart, "Thread with CreateTime before process CreateTime found in process %wZ\n", &ProcessInfo->ImageName);
        ok(ThreadInfo->CreateTime.QuadPart <= g_CurrentSystemTime.QuadPart, "Thread with CreateTime in the future found: %wZ\n", &ProcessInfo->ImageName);
        ok(ThreadInfo->ClientId.UniqueThread != 0, "Thread with zero UniqueThread found in process %wZ\n", &ProcessInfo->ImageName);
        ok(ThreadInfo->WaitTime != 0, "Thread with zero WaitTime found in process %wZ\n", &ProcessInfo->ImageName);
    }
}

void Test_SystemProcessInformation(void) // 5
{
    NTSTATUS Status;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;
    PVOID InfoBuffer, InfoEnd;
    PVOID CurrentInfo;
    ULONG ReturnLength;

    Status = NtQuerySystemInformation(SystemProcessInformation, NULL, 0, &ReturnLength);
    ok_eq_hex(Status, STATUS_INFO_LENGTH_MISMATCH);
    ok(ReturnLength > sizeof(*ProcessInfo), "ReturnLength too small: %lu\n", ReturnLength);

    InfoBuffer = HeapAlloc(GetProcessHeap(), 0, ReturnLength * 2);
    if (!InfoBuffer)
    {
        skip("HeapAlloc failed\n");
        return;
    }

    Status = NtQuerySystemInformation(SystemProcessInformation, InfoBuffer, ReturnLength * 2, &ReturnLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("NtQuerySystemInformation failed with status 0x%08x\n", Status);
        HeapFree(GetProcessHeap(), 0, InfoBuffer);
        return;
    }

    /* Iterate over the extended process information */
    InfoEnd = (PBYTE)InfoBuffer + ReturnLength;
    CurrentInfo = InfoBuffer;
    while (CurrentInfo < InfoEnd)
    {
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)CurrentInfo;

        ValidateProcessInfo(ProcessInfo, FALSE);
        ok(ProcessInfo->PageDirectoryBase == 0, "Process with non-zero PageDirectoryBase found: %wZ\n", &ProcessInfo->ImageName);

        /* Iterate over the threads of the current process */
        PSYSTEM_THREAD_INFORMATION ThreadInfoBase = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        for (ULONG i = 0; i < ProcessInfo->NumberOfThreads; i++)
        {
            PSYSTEM_THREAD_INFORMATION ThreadInfo = &ThreadInfoBase[i];

            ValidateThreadInfo(__LINE__, ThreadInfo, ProcessInfo);
        }

        if (ProcessInfo->NextEntryOffset == 0)
            break;
        CurrentInfo = (PBYTE)CurrentInfo + ProcessInfo->NextEntryOffset;
    }

    HeapFree(GetProcessHeap(), 0, InfoBuffer);
}

void Test_SystemExtendedProcessInformation(void) // 57
{
    NTSTATUS Status;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    ULONG ReturnLength;
    PVOID InfoBuffer, InfoEnd;
    PVOID CurrentInfo;

    Status = NtQuerySystemInformation(SystemExtendedProcessInformation, NULL, 0, &ReturnLength);
    if (!is_reactos() && GetNTVersion() < _WIN32_WINNT_VISTA)
    {
        ok_hex(Status, STATUS_INVALID_INFO_CLASS);
        skip("SystemExtendedProcessInformation is not supported on this OS version\n");
        return;
    }

    if (is_reactos() && (Status == STATUS_INVALID_INFO_CLASS))
    {
        skip("SystemExtendedProcessInformation is not implemented in ReactOSn\n");
        return;
    }

    ok_eq_hex(Status, STATUS_INFO_LENGTH_MISMATCH);
    ok(ReturnLength > sizeof(*ProcessInfo), "ReturnLength too small: %lu\n", ReturnLength);

    InfoBuffer = HeapAlloc(GetProcessHeap(), 0, ReturnLength * 2);
    if (!InfoBuffer)
    {
        skip("HeapAlloc failed\n");
        return;
    }

    Status = NtQuerySystemInformation(SystemExtendedProcessInformation, InfoBuffer, ReturnLength * 2, &ReturnLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("NtQuerySystemInformation failed with status 0x%08x\n", Status);
        HeapFree(GetProcessHeap(), 0, InfoBuffer);
        return;
    }

    /* Iterate over the extended process information */
    InfoEnd = (PBYTE)InfoBuffer + ReturnLength;
    CurrentInfo = InfoBuffer;
    while (CurrentInfo < InfoEnd)
    {
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)CurrentInfo;

        ValidateProcessInfo(ProcessInfo, TRUE);
        if ((ProcessInfo->UniqueProcessId == 0) && (GetNTVersion() >= _WIN32_WINNT_WIN7))
        {
            ok(ProcessInfo->PageDirectoryBase == 0, "Idle process with non-zero PageDirectoryBase found\n");
        }
        else
        {
            ok(ProcessInfo->PageDirectoryBase != 0, "Process with zero PageDirectoryBase found: %wZ\n", &ProcessInfo->ImageName);
        }

        /* Iterate over the threads of the current process */
        PSYSTEM_EXTENDED_THREAD_INFORMATION ThreadInfoBase = (PSYSTEM_EXTENDED_THREAD_INFORMATION)(ProcessInfo + 1);
        for (ULONG i = 0; i < ProcessInfo->NumberOfThreads; i++)
        {
            PSYSTEM_EXTENDED_THREAD_INFORMATION ThreadInfoEx = &ThreadInfoBase[i];

            ValidateThreadInfo(__LINE__, &ThreadInfoEx->ThreadInfo, ProcessInfo);

            //ok(ThreadInfoEx->StackBase != NULL, "Thread with NULL StackBase found in process %wZ\n", &ProcessInfo->ImageName);
            ok(ThreadInfoEx->StackLimit != NULL, "Thread with NULL StackLimit found in process %wZ\n", &ProcessInfo->ImageName);
            ok(ThreadInfoEx->Win32StartAddress != NULL, "Thread with NULL Win32StartAddress found in process %wZ\n", &ProcessInfo->ImageName);
            //ok(ThreadInfoEx->TebBase != NULL, "Thread with NULL TebBase found in process %wZ\n", &ProcessInfo->ImageName);
            ok_eq_ulongptr(ThreadInfoEx->Reserved2, 0);
            ok_eq_ulongptr(ThreadInfoEx->Reserved3, 0);
            ok_eq_ulongptr(ThreadInfoEx->Reserved4, 0);
        }

        if (ProcessInfo->NextEntryOffset == 0)
            break;
        CurrentInfo = (PBYTE)CurrentInfo + ProcessInfo->NextEntryOffset;
    }

    HeapFree(GetProcessHeap(), 0, InfoBuffer);
}


START_TEST(NtQuerySystemInformation)
{
    NTSTATUS Status;

    Status = NtQuerySystemInformation(0, NULL, 0, NULL);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    Status = NtQuerySystemInformation(0x80000000, NULL, 0, NULL);
    ok_hex(Status, STATUS_INVALID_INFO_CLASS);

    /* Get current system time for comparison */
    NtQuerySystemTime(&g_CurrentSystemTime);

    Test_SystemBasicInformation();
    Test_SystemEmulationBasicInformation();
    Test_SystemProcessInformation();
    Test_SystemExtendedProcessInformation();
}
