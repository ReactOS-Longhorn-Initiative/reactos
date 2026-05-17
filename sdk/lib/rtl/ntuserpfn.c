/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NtUser PFN registration
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtl.h>
#include <ndk/rtlfuncs.h>

#define NDEBUG
#include <debug.h>

typedef struct _NTUSER_PFN
{
    PVOID ProcsA[RTLP_NUMBER_OF_NTUSER_AW_PFNS];
    PVOID ProcsW[RTLP_NUMBER_OF_NTUSER_AW_PFNS];
    PVOID ProcsX[RTLP_NUMBER_OF_NTUSER_X_PFNS];
} NTUSER_PFN, *PNTUSER_PFN;

const NTUSER_PFN RtlpNtUserPfns = { 0 };

extern const NTUSER_PFN NtDllUserStubs;

static BOOLEAN NtUserPfnsInitialized;

NTSTATUS
NTAPI
RtlInitializeNtUserPfn(
    _In_reads_bytes_(SizeOfProcsA) const PVOID* ProcsA,
    _In_ SIZE_T SizeOfProcsA,
    _In_reads_bytes_(SizeOfProcsW) const PVOID* ProcsW,
    _In_ SIZE_T SizeOfProcsW,
    _In_reads_bytes_(SizeOfProcsX) const PVOID* ProcsX,
    _In_ SIZE_T SizeOfProcsX)
{
    ULONG OldProtect;
    PVOID BaseAddress;
    SIZE_T RegionSize;
    NTSTATUS Status;

    /* Validate parameters */
    if ((SizeOfProcsA != sizeof(RtlpNtUserPfns.ProcsA)) ||
        (SizeOfProcsW != sizeof(RtlpNtUserPfns.ProcsW)) ||
        (SizeOfProcsX != sizeof(RtlpNtUserPfns.ProcsX)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if already initialized */
    if (NtUserPfnsInitialized != FALSE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Change page protection, so we can set up the PFNs */
    BaseAddress = (PVOID)&RtlpNtUserPfns;
    RegionSize = sizeof(RtlpNtUserPfns);
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &BaseAddress,
                                    &RegionSize,
                                    PAGE_READWRITE,
                                    &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RtlCopyMemory((PVOID)RtlpNtUserPfns.ProcsA, ProcsA, SizeOfProcsA);
    RtlCopyMemory((PVOID)RtlpNtUserPfns.ProcsW, ProcsW, SizeOfProcsW);
    RtlCopyMemory((PVOID)RtlpNtUserPfns.ProcsX, ProcsX, SizeOfProcsX);

    /* Restore original page protection */
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &BaseAddress,
                                    &RegionSize,
                                    OldProtect,
                                    &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    NtUserPfnsInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS
WINAPI
RtlRetrieveNtUserPfn(
    _Out_ const PVOID** ProcsA,
    _Out_ const PVOID** ProcsW,
    _Out_ const PVOID** ProcsX)
{
    if (NtUserPfnsInitialized == FALSE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ProcsA = NtDllUserStubs.ProcsA;
    *ProcsW = NtDllUserStubs.ProcsW;
    *ProcsX = NtDllUserStubs.ProcsX;

    return STATUS_SUCCESS;
}
