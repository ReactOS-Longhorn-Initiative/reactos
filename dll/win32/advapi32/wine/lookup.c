
/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     See COPYING in the top level directory
 * WINE COPYRIGHT:
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
 * Copyright 2006 Robert Reif
 * Copyright 2006 Hervé Poussineau
 */

#include <advapi32.h>

#include <sddl.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static
DWORD
GetUnicodeEnvironmentSize(
    PVOID pEnvironment)
{
    INT Length, TotalLength = 0;
    PWCHAR Ptr;

    if (pEnvironment == NULL)
        return 0;

    Ptr = (PWCHAR)pEnvironment;
    while (*Ptr != UNICODE_NULL)
    {
        Length = wcslen(Ptr) + 1;
        TotalLength += Length;
        Ptr = Ptr + Length;
    }

    return (TotalLength + 1) * sizeof(WCHAR);
}


static
DWORD
GetAnsiEnvironmentSize(
    PVOID pEnvironment)
{
    INT Length, TotalLength = 0;
    PCHAR Ptr;

    if (pEnvironment == NULL)
        return 0;

    Ptr = (PCHAR)pEnvironment;
    while (*Ptr != ANSI_NULL)
    {
        Length = strlen(Ptr) + 1;
        TotalLength += Length;
        Ptr = Ptr + Length;
    }

    return TotalLength + 1;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CreateProcessWithLogonW(
    _In_ LPCWSTR lpUsername,
    _In_opt_ LPCWSTR lpDomain,
    _In_ LPCWSTR lpPassword,
    _In_ DWORD dwLogonFlags,
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation)
{
    LPWSTR pszStringBinding = NULL;
    handle_t hBinding = NULL;
    SECL_REQUEST Request;
    SECL_RESPONSE Response;
    RPC_STATUS Status;

    TRACE("CreateProcessWithLogonW(%s %s %s 0x%08x %s %s 0x%08x %p %s %p %p)\n", debugstr_w(lpUsername), debugstr_w(lpDomain),
    debugstr_w(lpPassword), dwLogonFlags, debugstr_w(lpApplicationName),
    debugstr_w(lpCommandLine), dwCreationFlags, lpEnvironment, debugstr_w(lpCurrentDirectory),
    lpStartupInfo, lpProcessInformation);

    Status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      NULL,
                                      L"\\pipe\\seclogon",
                                      NULL,
                                      &pszStringBinding);
    if (Status != RPC_S_OK)
    {
        WARN("RpcStringBindingCompose returned 0x%x\n", Status);
        SetLastError(Status);
        return FALSE;
    }

    /* Set the binding handle that will be used to bind to the server. */
    Status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (Status != RPC_S_OK)
    {
        WARN("RpcBindingFromStringBinding returned 0x%x\n", Status);
    }

    Status = RpcStringFreeW(&pszStringBinding);
    if (Status != RPC_S_OK)
    {
        WARN("RpcStringFree returned 0x%x\n", Status);
    }

    Request.Username = (LPWSTR)lpUsername;
    Request.Domain = (LPWSTR)lpDomain;
    Request.Password = (LPWSTR)lpPassword;
    Request.ApplicationName = (LPWSTR)lpApplicationName;
    Request.CommandLine = (LPWSTR)lpCommandLine;
    Request.CurrentDirectory = (LPWSTR)lpCurrentDirectory;

    if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT)
        Request.dwEnvironmentSize = GetUnicodeEnvironmentSize(lpEnvironment);
    else
        Request.dwEnvironmentSize = GetAnsiEnvironmentSize(lpEnvironment);
    Request.Environment = lpEnvironment;

    TRACE("Request.dwEnvironmentSize %lu\n", Request.dwEnvironmentSize);
    TRACE("Request.Environment %p\n", Request.Environment);

    Request.dwLogonFlags = dwLogonFlags;
    Request.dwCreationFlags = dwCreationFlags;

    Request.dwProcessId = GetCurrentProcessId();
    TRACE("Request.dwProcessId %lu\n", Request.dwProcessId);

    Response.hProcess = 0;
    Response.hThread = 0;
    Response.dwProcessId = 0;
    Response.dwThreadId = 0;
    Response.dwError = ERROR_SUCCESS;

    RpcTryExcept
    {
        SeclCreateProcessWithLogonW(hBinding, &Request, &Response);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        WARN("Exception: %lx\n", RpcExceptionCode());
    }
    RpcEndExcept;

    if (hBinding)
    {
        Status = RpcBindingFree(&hBinding);
        if (Status != RPC_S_OK)
        {
            WARN("RpcBindingFree returned 0x%x\n", Status);
        }

        hBinding = NULL;
    }

    TRACE("Response.hProcess %p\n", Response.hProcess);
    TRACE("Response.hThread %p\n", Response.hThread);
    TRACE("Response.dwProcessId %lu\n", Response.dwProcessId);
    TRACE("Response.dwThreadId %lu\n", Response.dwThreadId);
    TRACE("Response.dwError %lu\n", Response.dwError);
    if (Response.dwError != ERROR_SUCCESS)
        SetLastError(Response.dwError);

    TRACE("CreateProcessWithLogonW() done\n");

    return (Response.dwError == ERROR_SUCCESS);
}


/**********************************************************************
 * LookupPrivilegeDisplayNameA			EXPORTED
 *
 * @unimplemented
 */
BOOL
WINAPI
LookupPrivilegeDisplayNameA(LPCSTR lpSystemName,
                            LPCSTR lpName,
                            LPSTR lpDisplayName,
                            LPDWORD cchDisplayName,
                            LPDWORD lpLanguageId)
{
    UNICODE_STRING lpSystemNameW;
    UNICODE_STRING lpNameW;
    BOOL ret;
    DWORD wLen = 0;

    TRACE("%s %s %p %p %p\n", debugstr_a(lpSystemName), debugstr_a(lpName), lpName, cchDisplayName, lpLanguageId);

    RtlCreateUnicodeStringFromAsciiz(&lpSystemNameW, lpSystemName);
    RtlCreateUnicodeStringFromAsciiz(&lpNameW, lpName);
    ret = LookupPrivilegeDisplayNameW(lpSystemNameW.Buffer, lpNameW.Buffer, NULL, &wLen, lpLanguageId);
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        LPWSTR lpDisplayNameW = HeapAlloc(GetProcessHeap(), 0, wLen * sizeof(WCHAR));

        ret = LookupPrivilegeDisplayNameW(lpSystemNameW.Buffer, lpNameW.Buffer, lpDisplayNameW,
                                          &wLen, lpLanguageId);
        if (ret)
        {
            unsigned int len = WideCharToMultiByte(CP_ACP, 0, lpDisplayNameW, -1, lpDisplayName,
                                                   *cchDisplayName, NULL, NULL);

            if (len == 0)
            {
                /* WideCharToMultiByte failed */
                ret = FALSE;
            }
            else if (len > *cchDisplayName)
            {
                *cchDisplayName = len;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                ret = FALSE;
            }
            else
            {
                /* WideCharToMultiByte succeeded, output length needs to be
                 * length not including NULL terminator
                 */
                *cchDisplayName = len - 1;
            }
        }
        HeapFree(GetProcessHeap(), 0, lpDisplayNameW);
    }
    RtlFreeUnicodeString(&lpSystemNameW);
    RtlFreeUnicodeString(&lpNameW);
    return ret;
}

/**********************************************************************
 * LookupPrivilegeNameA				EXPORTED
 *
 * @implemented
 */
BOOL
WINAPI
LookupPrivilegeNameA(LPCSTR lpSystemName,
                     PLUID lpLuid,
                     LPSTR lpName,
                     LPDWORD cchName)
{
    UNICODE_STRING lpSystemNameW;
    BOOL ret;
    DWORD wLen = 0;

    TRACE("%s %p %p %p\n", debugstr_a(lpSystemName), lpLuid, lpName, cchName);

    RtlCreateUnicodeStringFromAsciiz(&lpSystemNameW, lpSystemName);
    ret = LookupPrivilegeNameW(lpSystemNameW.Buffer, lpLuid, NULL, &wLen);
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        LPWSTR lpNameW = HeapAlloc(GetProcessHeap(), 0, wLen * sizeof(WCHAR));

        ret = LookupPrivilegeNameW(lpSystemNameW.Buffer, lpLuid, lpNameW,
         &wLen);
        if (ret)
        {
            /* Windows crashes if cchName is NULL, so will I */
            unsigned int len = WideCharToMultiByte(CP_ACP, 0, lpNameW, -1, lpName,
             *cchName, NULL, NULL);

            if (len == 0)
            {
                /* WideCharToMultiByte failed */
                ret = FALSE;
            }
            else if (len > *cchName)
            {
                *cchName = len;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                ret = FALSE;
            }
            else
            {
                /* WideCharToMultiByte succeeded, output length needs to be
                 * length not including NULL terminator
                 */
                *cchName = len - 1;
            }
        }
        HeapFree(GetProcessHeap(), 0, lpNameW);
    }
    RtlFreeUnicodeString(&lpSystemNameW);
    return ret;
}


/******************************************************************************
 * LookupAccountNameA [ADVAPI32.@]
 *
 * @implemented
 */
BOOL
WINAPI
LookupAccountNameA(LPCSTR SystemName,
                   LPCSTR AccountName,
                   PSID Sid,
                   LPDWORD SidLength,
                   LPSTR ReferencedDomainName,
                   LPDWORD hReferencedDomainNameLength,
                   PSID_NAME_USE SidNameUse)
{
    BOOL ret;
    UNICODE_STRING lpSystemW;
    UNICODE_STRING lpAccountW;
    LPWSTR lpReferencedDomainNameW = NULL;

    RtlCreateUnicodeStringFromAsciiz(&lpSystemW, SystemName);
    RtlCreateUnicodeStringFromAsciiz(&lpAccountW, AccountName);

    if (ReferencedDomainName)
        lpReferencedDomainNameW = HeapAlloc(GetProcessHeap(),
                                            0,
                                            *hReferencedDomainNameLength * sizeof(WCHAR));

    ret = LookupAccountNameW(lpSystemW.Buffer,
                             lpAccountW.Buffer,
                             Sid,
                             SidLength,
                             lpReferencedDomainNameW,
                             hReferencedDomainNameLength,
                             SidNameUse);

    if (ret && lpReferencedDomainNameW)
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpReferencedDomainNameW,
                            *hReferencedDomainNameLength + 1,
                            ReferencedDomainName,
                            *hReferencedDomainNameLength + 1,
                            NULL,
                            NULL);
    }

    RtlFreeUnicodeString(&lpSystemW);
    RtlFreeUnicodeString(&lpAccountW);
    HeapFree(GetProcessHeap(), 0, lpReferencedDomainNameW);

    return ret;
}

