/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/sid.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"

#include <ndk/sefuncs.h>

#define NDEBUG
#include <debug.h>

PSID
WINAPI
GetUserSid(HANDLE hToken)
{
    PTOKEN_USER UserBuffer;
    PTOKEN_USER TempBuffer;
    NTSTATUS Status;
    ULONG Length;
    PSID pSid;

    Length = 256;
    UserBuffer = LocalAlloc(LPTR, Length);
    if (UserBuffer == NULL)
    {
        return NULL;
    }

    Status = NtQueryInformationToken(hToken,
                                     TokenUser,
                                     (PVOID)UserBuffer,
                                     Length,
                                     &Length);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        TempBuffer = LocalReAlloc(UserBuffer, Length, LMEM_MOVEABLE);
        if (TempBuffer == NULL)
        {
            LocalFree(UserBuffer);
            return NULL;
        }

        UserBuffer = TempBuffer;
        Status = NtQueryInformationToken(hToken,
                                         TokenUser,
                                         (PVOID)UserBuffer,
                                         Length,
                                         &Length);
    }

    if (!NT_SUCCESS(Status))
    {
        LocalFree(UserBuffer);
        return NULL;
    }

        Length = RtlLengthSid(UserBuffer->User.Sid);

    pSid = LocalAlloc(LPTR, Length);
    if (pSid == NULL)
    {
        LocalFree(UserBuffer);
        return NULL;
    }

    Status = RtlCopySid(Length, pSid, UserBuffer->User.Sid);

    LocalFree(UserBuffer);

    if (!NT_SUCCESS(Status))
    {
        LocalFree(pSid);
        return NULL;
    }

    return pSid;
}

BOOL
GetUserSidStringFromToken(HANDLE hToken,
                          PUNICODE_STRING SidString)
{
    PTOKEN_USER UserBuffer, nsb;
    ULONG Length;
    NTSTATUS Status;

    Length = 256;
    UserBuffer = LocalAlloc(LPTR, Length);
    if (UserBuffer == NULL)
        return FALSE;

    Status = NtQueryInformationToken(hToken,
                                     TokenUser,
                                     (PVOID)UserBuffer,
                                     Length,
                                     &Length);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        nsb = LocalReAlloc(UserBuffer,
                           Length,
                           LMEM_MOVEABLE);
        if (nsb == NULL)
        {
            LocalFree(UserBuffer);
            return FALSE;
        }

        UserBuffer = nsb;
        Status = NtQueryInformationToken(hToken,
                                         TokenUser,
                                         (PVOID)UserBuffer,
                                         Length,
                                         &Length);
    }

    if (!NT_SUCCESS (Status))
    {
        LocalFree(UserBuffer);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    DPRINT("SidLength: %lu\n", RtlLengthSid (UserBuffer->User.Sid));

    Status = RtlConvertSidToUnicodeString(SidString,
                                          UserBuffer->User.Sid,
                                          TRUE);

    LocalFree(UserBuffer);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    DPRINT("SidString.Length: %lu\n", SidString->Length);
    DPRINT("SidString.MaximumLength: %lu\n", SidString->MaximumLength);
    DPRINT("SidString: '%wZ'\n", SidString);

    return TRUE;
}


/******************************************************************
 *    DeriveAppContainerSidFromAppContainerName (USERENV.@)
 *
 * Creates a SID for an app container based on the app container name.
 *
 * PARAMS
 *  pszAppContainerName [In]  App container name
 *  ppsidAppContainerSid [Out] Pointer to receive the app container SID
 *
 * RETURNS
 *  HRESULT indicating success or failure
 */
HRESULT WINAPI
DeriveAppContainerSidFromAppContainerName(
    IN PCWSTR pszAppContainerName,
    OUT PSID *ppsidAppContainerSid)
{

    /* This code is also just insanely borked. */
    UNICODE_STRING AppContainerName;
    PSID AppContainerSid = NULL;

    if (!pszAppContainerName || !ppsidAppContainerSid)
    {
        return E_INVALIDARG;
    }

    *ppsidAppContainerSid = NULL;

    /* Initialize the unicode string for the app container name */
    RtlInitUnicodeString(&AppContainerName, pszAppContainerName);

    /* Create a simple app container SID manually */
    /* App container SIDs have the format: S-1-15-2-<hash> where:
     * S-1-15 is the APPLICATION_PACKAGE_AUTHORITY  
     * 2 is the SECURITY_APP_PACKAGE_BASE_RID
     * <hash> is derived from the app container name
     */
    SID_IDENTIFIER_AUTHORITY AppPackageAuthority = { SECURITY_APP_PACKAGE_AUTHORITY };
    DWORD Hash = 0;
    PCWSTR p = pszAppContainerName;

    /* Simple hash calculation from the app container name */
    while (*p)
    {
        WCHAR c = *p;
        /* Convert to lowercase manually to avoid dependency on CRT */
        if (c >= L'A' && c <= L'Z')
            c = c - L'A' + L'a';
        Hash = Hash * 37 + (DWORD)c;
        p++;
    }

    /* Allocate and initialize the SID */
    if (!AllocateAndInitializeSid(&AppPackageAuthority,
                                  2,  /* SubAuthorityCount */
                                  SECURITY_APP_PACKAGE_BASE_RID,
                                  Hash,
                                  0, 0, 0, 0, 0, 0,
                                  &AppContainerSid))
    {
        DWORD dwError = GetLastError();
        DPRINT1("AllocateAndInitializeSid failed with error %lu\n", dwError);
        return HRESULT_FROM_WIN32(dwError);
    }

    *ppsidAppContainerSid = AppContainerSid;
    return S_OK;
}
