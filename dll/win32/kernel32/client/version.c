/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/version.c
 * PURPOSE:         Version functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
                    Ged Murphy (gedmurphy@reactos.org)
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
GetVersion(VOID)
{
    PPEB Peb = NtCurrentPeb();

    return (DWORD)( ((Peb->OSPlatformId ^ 2) << 30) |
                     (Peb->OSBuildNumber     << 16) |
                     (Peb->OSMinorVersion    << 8 ) |
                      Peb->OSMajorVersion );
}


BOOLEAN WINAPI RtlGetProductInfo(DWORD dwOSMajorVersion, DWORD dwOSMinorVersion, DWORD dwSpMajorVersion,
                                 DWORD dwSpMinorVersion, PDWORD pdwReturnedProductType);
            
/***********************************************************************
 *         GetProductInfo   (kernelbase.@)
 */
BOOL WINAPI GetProductInfo( DWORD os_major, DWORD os_minor,
                                              DWORD sp_major, DWORD sp_minor, DWORD *type )
{
    return RtlGetProductInfo( os_major, os_minor, sp_major, sp_minor, type );
}

#define DONT_RESOLVE_DLL_REFERENCES         0x00000001
#define LOAD_LIBRARY_AS_DATAFILE            0x00000002
// reserved for internal LOAD_PACKAGED_LIBRARY: 0x00000004
#define LOAD_WITH_ALTERED_SEARCH_PATH       0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL        0x00000010
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE      0x00000020
#define LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE  0x00000040
#define LOAD_LIBRARY_REQUIRE_SIGNED_TARGET  0x00000080
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#define LOAD_LIBRARY_SEARCH_USER_DIRS       0x00000400
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000


static DWORD default_search_flags;  /* default flags set by LdrSetDefaultDllDirectories */
BOOL WINAPI SetDefaultDllDirectories(DWORD DirectoryFlags)
{
 
    /* LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR doesn't make sense in default dirs */
    const ULONG load_library_search_flags = (LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                             LOAD_LIBRARY_SEARCH_USER_DIRS |
                                             LOAD_LIBRARY_SEARCH_SYSTEM32 |
                                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

    if (!DirectoryFlags || (DirectoryFlags & ~load_library_search_flags)) return STATUS_INVALID_PARAMETER;
    default_search_flags = DirectoryFlags;
    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetVersionExW(IN LPOSVERSIONINFOW lpVersionInformation)
{
    NTSTATUS Status;
    LPOSVERSIONINFOEXW lpVersionInformationEx;

    if ((lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW)) &&
        (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)lpVersionInformation);
    if (Status == STATUS_SUCCESS)
    {
        if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
        {
            lpVersionInformationEx = (PVOID)lpVersionInformation;
            lpVersionInformationEx->wReserved = 0;
        }

        return TRUE;
    }

    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVersionExA(IN LPOSVERSIONINFOA lpVersionInformation)
{
    OSVERSIONINFOEXW VersionInformation;
    LPOSVERSIONINFOEXA lpVersionInformationEx;
    UNICODE_STRING CsdVersionW;
    NTSTATUS Status;
    ANSI_STRING CsdVersionA;

    if ((lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA)) &&
        (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA)))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    if (!GetVersionExW((LPOSVERSIONINFOW)&VersionInformation)) return FALSE;

    /* Copy back fields that match both supported structures */
    lpVersionInformation->dwMajorVersion = VersionInformation.dwMajorVersion;
    lpVersionInformation->dwMinorVersion = VersionInformation.dwMinorVersion;
    lpVersionInformation->dwBuildNumber = VersionInformation.dwBuildNumber;
    lpVersionInformation->dwPlatformId = VersionInformation.dwPlatformId;

    if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA))
    {
        lpVersionInformationEx = (PVOID)lpVersionInformation;
        lpVersionInformationEx->wServicePackMajor = VersionInformation.wServicePackMajor;
        lpVersionInformationEx->wServicePackMinor = VersionInformation.wServicePackMinor;
        lpVersionInformationEx->wSuiteMask = VersionInformation.wSuiteMask;
        lpVersionInformationEx->wProductType = VersionInformation.wProductType;
        lpVersionInformationEx->wReserved = VersionInformation.wReserved;
    }

    /* Convert the CSD string */
    RtlInitEmptyAnsiString(&CsdVersionA,
                           lpVersionInformation->szCSDVersion,
                           sizeof(lpVersionInformation->szCSDVersion));
    RtlInitUnicodeString(&CsdVersionW, VersionInformation.szCSDVersion);
    Status = RtlUnicodeStringToAnsiString(&CsdVersionA, &CsdVersionW, FALSE);
    return (NT_SUCCESS(Status));
}

/*
 * @implemented
 */
BOOL
WINAPI
VerifyVersionInfoW(IN LPOSVERSIONINFOEXW lpVersionInformation,
                   IN DWORD dwTypeMask,
                   IN DWORDLONG dwlConditionMask)
{
    NTSTATUS Status;

    Status = RtlVerifyVersionInfo((PRTL_OSVERSIONINFOEXW)lpVersionInformation,
                                  dwTypeMask,
                                  dwlConditionMask);
    switch (Status)
    {
        case STATUS_INVALID_PARAMETER:
            SetLastError(ERROR_BAD_ARGUMENTS);
            return FALSE;

        case STATUS_REVISION_MISMATCH:
            if (lpVersionInformation)
            {
                DPRINT1("VerifyVersionInfo -- Version mismatch(%d.%d.%d:%d)\n",
                        (dwTypeMask & VER_MAJORVERSION) ? lpVersionInformation->dwMajorVersion : -1,
                        (dwTypeMask & VER_MINORVERSION) ? lpVersionInformation->dwMinorVersion : -1,
                        (dwTypeMask & VER_BUILDNUMBER) ? lpVersionInformation->dwBuildNumber : -1,
                        (dwTypeMask & VER_PLATFORMID) ? lpVersionInformation->dwPlatformId : -1);
            }
            else
            {
                DPRINT1("VerifyVersionInfo -- Version mismatch(NULL)\n");
            }
            SetLastError(ERROR_OLD_WIN_VERSION);
            return FALSE;

        default:
            /* RtlVerifyVersionInfo shouldn't report any other failure code! */
            ASSERT(NT_SUCCESS(Status));
            return TRUE;
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
VerifyVersionInfoA(IN LPOSVERSIONINFOEXA lpVersionInformation,
                   IN DWORD dwTypeMask,
                   IN DWORDLONG dwlConditionMask)
{
    OSVERSIONINFOEXW viex;

    /* NOTE: szCSDVersion is ignored, we don't need to convert it to Unicode */
    viex.dwOSVersionInfoSize = sizeof(viex);
    viex.dwMajorVersion = lpVersionInformation->dwMajorVersion;
    viex.dwMinorVersion = lpVersionInformation->dwMinorVersion;
    viex.dwBuildNumber = lpVersionInformation->dwBuildNumber;
    viex.dwPlatformId = lpVersionInformation->dwPlatformId;
    viex.wServicePackMajor = lpVersionInformation->wServicePackMajor;
    viex.wServicePackMinor = lpVersionInformation->wServicePackMinor;
    viex.wSuiteMask = lpVersionInformation->wSuiteMask;
    viex.wProductType = lpVersionInformation->wProductType;
    viex.wReserved = lpVersionInformation->wReserved;
    return VerifyVersionInfoW(&viex, dwTypeMask, dwlConditionMask);
}
