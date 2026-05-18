/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     KSE 'VersionLie' shim implementation
 * COPYRIGHT:   Copyright 2020
 */

#include <ntoskrnl.h>
#include <initguid.h>

//#define NDEBUG
#include <debug.h>

extern KSE_SHIM KmVlShim;

/* Generic version info to use with the shims */
typedef struct VersionLieInfo
{
    DWORD FullVersion;
    WORD dwMajorVersion;
    WORD dwMinorVersion;
    DWORD dwBuildNumber;
    WORD dwPlatformId;
    WORD wServicePackMajor;
    WORD wServicePackMinor;
} VersionLieInfo;

VersionLieInfo g_Win95 = { 0xC3B60004, 4, 0, 950, VER_PLATFORM_WIN32_WINDOWS, 0, 0 };
VersionLieInfo g_WinNT4SP5 = { 0x05650004, 4, 0, 1381, VER_PLATFORM_WIN32_NT, 5, 0 };
VersionLieInfo g_Win98 = { 0xC0000A04, 4, 10, 0x040A08AE, VER_PLATFORM_WIN32_WINDOWS, 0, 0 };

VersionLieInfo g_Win2000 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win2000SP1 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_Win2000SP2 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 2, 0 };
VersionLieInfo g_Win2000SP3 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 3, 0 };

VersionLieInfo g_WinXP = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_WinXPSP1 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_WinXPSP2 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 2, 0 };
VersionLieInfo g_WinXPSP3 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 3, 0 };

VersionLieInfo g_Win2k3RTM = { 0x0ece0205, 5, 2, 3790, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win2k3SP1 = { 0x0ece0205, 5, 2, 3790, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_Win2k3SP2 = { 0x0ece0205, 5, 2, 3790, VER_PLATFORM_WIN32_NT, 2, 0 };

VersionLieInfo g_WinVistaRTM = { 0x17700006, 6, 0, 6000, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_WinVistaSP1 = { 0x17710006, 6, 0, 6001, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_WinVistaSP2 = { 0x17720006, 6, 0, 6002, VER_PLATFORM_WIN32_NT, 2, 0 };

VersionLieInfo g_Win7RTM = { 0x1db00106, 6, 1, 7600, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win7SP1 = { 0x1db10106, 6, 1, 7601, VER_PLATFORM_WIN32_NT, 1, 0 };

VersionLieInfo g_Win8RTM = { 0x23f00206, 6, 2, 9200, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win81RTM = { 0x25800306, 6, 3, 9600, VER_PLATFORM_WIN32_NT, 0, 0 };

VersionLieInfo g_Win10RTM = { 0x47ba000a, 10, 0, 18362, VER_PLATFORM_WIN32_NT, 0, 0 };

VersionLieInfo g_Win2k16RTM = { 0x3fab000a, 10, 0, 16299, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win2k19RTM = { 0x4563000a, 10, 0, 17763, VER_PLATFORM_WIN32_NT, 0, 0 };

static BOOLEAN FakeVersion(PRTL_OSVERSIONINFOEXW pResult, VersionLieInfo* pFake)
{
    if (pResult->dwOSVersionInfoSize == sizeof(PRTL_OSVERSIONINFOW) || pResult->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
    {
        RtlGetVersion((PRTL_OSVERSIONINFOW)pResult);

        pResult->dwMajorVersion = pFake->dwMajorVersion;
        pResult->dwMinorVersion = pFake->dwMinorVersion;
        pResult->dwBuildNumber = pFake->dwBuildNumber;
        pResult->dwPlatformId = pFake->dwPlatformId;
        if (pResult->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
        {
            pResult->wServicePackMajor = pFake->wServicePackMajor;
            pResult->wServicePackMinor = pFake->wServicePackMinor;

        }
        return TRUE;
    }
    return FALSE;
}

static BOOLEAN NTAPI
ShimPsGetVersion(OUT PULONG MajorVersion OPTIONAL,
             OUT PULONG MinorVersion OPTIONAL,
             OUT PULONG BuildNumber  OPTIONAL,
             OUT PUNICODE_STRING CSDVersion OPTIONAL)
{
    if (MajorVersion) *MajorVersion = g_Win2k3SP2.dwMajorVersion;
    if (MinorVersion) *MinorVersion = g_Win2k3SP2.dwMinorVersion;
    if (BuildNumber ) *BuildNumber  = g_Win2k3SP2.dwBuildNumber;

    return PsGetVersion(NULL, NULL, NULL, CSDVersion);
}

static NTSTATUS NTAPI
ShimRtlGetVersion(
    OUT PRTL_OSVERSIONINFOW lpVersionInformation)
{
    FakeVersion((PRTL_OSVERSIONINFOEXW)lpVersionInformation, &g_Win2k3SP2);
    return STATUS_SUCCESS;
}

static KSE_HOOK KmVlShimHooksNT[] = {
    { KseHookFunction, { "PsGetVersion" }, (PVOID)(ULONG_PTR)ShimPsGetVersion },
    { KseHookFunction, { "RtlGetVersion" }, (PVOID)(ULONG_PTR)ShimRtlGetVersion },
    //{ KseHookFunction, { "RtlVerifyVersionInfo" }, (PVOID)(ULONG_PTR)ShimRtlVerifyVersionInfo },
    { KseHookInvalid },
};
static KSE_HOOK_COLLECTION KmVlShimCollections[] = {
    { KseCollectionNtExport, NULL, KmVlShimHooksNT },
    { KseCollectionInvalid },
};

DEFINE_GUID(KmVlShimGuid, 0x4923cb7f, 0x461f, 0x42cd, 0xbb, 0x37, 0xc7, 0x74, 0xb1, 0x81, 0x0b, 0x20);

KSE_SHIM KmVlShim = { sizeof(KSE_SHIM), &KmVlShimGuid, L"VersionLie", NULL, NULL, NULL, KmVlShimCollections };

NTSTATUS
NTAPI
KseVersionLieInitialize(VOID)
{
    NTSTATUS Status;

    Status = KseRegisterShim(&KmVlShim, NULL, 0);
    if (NT_SUCCESS(Status))
        DPRINT("KSE: version lie shim registered.\n");

    return Status;
}
