/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/filename.c
 * PURPOSE:         Directory functions
 * PROGRAMMERS:     Ariadne (ariadne@xs4all.nl)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

/***********************************************************************
 *           GetTempFileNameA   (KERNEL32.@)
 */
UINT WINAPI
GetTempFileNameA(IN LPCSTR lpPathName,
                 IN LPCSTR lpPrefixString,
                 IN UINT uUnique,
                 OUT LPSTR lpTempFileName)
{
    UINT ID;
    NTSTATUS Status;
    LPWSTR lpTempFileNameW;
    PUNICODE_STRING lpPathNameW;
    ANSI_STRING TempFileNameStringA;
    UNICODE_STRING lpPrefixStringW, TempFileNameStringW;

    /* Convert strings */
    lpPathNameW = Basep8BitStringToStaticUnicodeString(lpPathName);
    if (!lpPathNameW)
    {
        return 0;
    }

    if (!Basep8BitStringToDynamicUnicodeString(&lpPrefixStringW, lpPrefixString))
    {
        return 0;
    }

    lpTempFileNameW = RtlAllocateHeap(RtlGetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (!lpTempFileNameW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RtlFreeUnicodeString(&lpPrefixStringW);
        return 0;
    }

    /* Call Unicode */
    ID = GetTempFileNameW(lpPathNameW->Buffer, lpPrefixStringW.Buffer, uUnique, lpTempFileNameW);
    if (ID)
    {
        RtlInitUnicodeString(&TempFileNameStringW, lpTempFileNameW);
        TempFileNameStringA.Buffer = lpTempFileName;
        TempFileNameStringA.MaximumLength = MAX_PATH;

        Status = BasepUnicodeStringTo8BitString(&TempFileNameStringA, &TempFileNameStringW, FALSE);
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            ID = 0;
        }
    }

    /* Cleanup */
    RtlFreeUnicodeString(&lpPrefixStringW);
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpTempFileNameW);
    return ID;
 }

 /***********************************************************************
  *           GetTempFileNameW   (KERNEL32.@)
  */
UINT WINAPI
GetTempFileNameW(IN LPCWSTR lpPathName,
                 IN LPCWSTR lpPrefixString,
                 IN UINT uUnique,
                 OUT LPWSTR lpTempFileName)
{
    PUCHAR Let;
    HANDLE TempFile;
    UINT ID, Num = 0;
    UCHAR IDString[5];
    WCHAR * TempFileName;
    BASE_API_MESSAGE ApiMessage;
    PBASE_GET_TEMP_FILE GetTempFile = &ApiMessage.Data.GetTempFileRequest;
    DWORD FileAttributes, LastError;
    UNICODE_STRING PathNameString, PrefixString;
    static const WCHAR Ext[] = { L'.', 't', 'm', 'p', UNICODE_NULL };

    RtlInitUnicodeString(&PathNameString, lpPathName);
    if (PathNameString.Length == 0 || PathNameString.Buffer[(PathNameString.Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        PathNameString.Length += sizeof(WCHAR);
    }

    /* lpTempFileName must be able to contain: PathName, Prefix (3), number(4), .tmp(4) & \0(1)
     * See: http://msdn.microsoft.com/en-us/library/aa364991%28v=vs.85%29.aspx
     */
    if (PathNameString.Length > (MAX_PATH - 3 - 4 - 4 - 1) * sizeof(WCHAR))
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return 0;
    }

    /* If PathName and TempFileName aren't the same buffer, move PathName to TempFileName */
    if (lpPathName != lpTempFileName)
    {
        memmove(lpTempFileName, PathNameString.Buffer, PathNameString.Length);
    }

    /* PathName MUST BE a path. Check it */
    lpTempFileName[(PathNameString.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    FileAttributes = GetFileAttributesW(lpTempFileName);
    if (FileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        /* Append a '\' if necessary */
        lpTempFileName[(PathNameString.Length / sizeof(WCHAR)) - 1] = L'\\';
        lpTempFileName[PathNameString.Length / sizeof(WCHAR)] = UNICODE_NULL;
        FileAttributes = GetFileAttributesW(lpTempFileName);
        if (FileAttributes == INVALID_FILE_ATTRIBUTES)
        {
            SetLastError(ERROR_DIRECTORY);
            return 0;
        }
    }
    if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        SetLastError(ERROR_DIRECTORY);
        return 0;
    }

    /* Make sure not to mix path & prefix */
    lpTempFileName[(PathNameString.Length / sizeof(WCHAR)) - 1] = L'\\';
    RtlInitUnicodeString(&PrefixString, lpPrefixString);
    if (PrefixString.Length > 3 * sizeof(WCHAR))
    {
        PrefixString.Length = 3 * sizeof(WCHAR);
    }

    /* Append prefix to path */
    TempFileName = lpTempFileName + PathNameString.Length / sizeof(WCHAR);
    memmove(TempFileName, PrefixString.Buffer, PrefixString.Length);
    TempFileName += PrefixString.Length / sizeof(WCHAR);

    /* Then, generate filename */
    do
    {
        /* If user didn't gave any ID, ask Csrss to give one */
        if (!uUnique)
        {
            CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                NULL,
                                CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepGetTempFile),
                                sizeof(*GetTempFile));
            if (GetTempFile->UniqueID == 0)
            {
                Num++;
                continue;
            }

            ID = GetTempFile->UniqueID;
        }
        else
        {
            ID = uUnique;
        }

        /* Convert that ID to wchar */
        RtlIntegerToChar(ID, 0x10, sizeof(IDString), (PCHAR)IDString);
        Let = IDString;
        do
        {
            *(TempFileName++) = RtlAnsiCharToUnicodeChar(&Let);
        } while (*Let != 0);

        /* Append extension & UNICODE_NULL */
        memmove(TempFileName, Ext, sizeof(Ext));

        /* If user provided its ID, just return */
        if (uUnique)
        {
            return uUnique;
        }

        /* Then, try to create file */
        if (!RtlIsDosDeviceName_U(lpTempFileName))
        {
            TempFile = CreateFileW(lpTempFileName,
                                   GENERIC_READ,
                                   0,
                                   NULL,
                                   CREATE_NEW,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
            if (TempFile != INVALID_HANDLE_VALUE)
            {
                NtClose(TempFile);
                DPRINT("Temp file: %S\n", lpTempFileName);
                return ID;
            }

            LastError = GetLastError();
            /* There is no need to recover from those errors, they would hit next step */
            if (LastError == ERROR_INVALID_PARAMETER || LastError == ERROR_CANNOT_MAKE ||
                LastError == ERROR_WRITE_PROTECT || LastError == ERROR_NETWORK_ACCESS_DENIED ||
                LastError == ERROR_DISK_FULL || LastError == ERROR_INVALID_NAME ||
                LastError == ERROR_BAD_PATHNAME || LastError == ERROR_NO_INHERITANCE ||
                LastError == ERROR_DISK_CORRUPT ||
                (LastError == ERROR_ACCESS_DENIED && NtCurrentTeb()->LastStatusValue != STATUS_FILE_IS_A_DIRECTORY))
            {
                break;
            }
        }
        Num++;
    } while (Num & 0xFFFF);

    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetFileShortNameW(
    HANDLE hFile,
    LPCWSTR lpShortName)
{
    NTSTATUS Status;
    ULONG NeededSize;
    UNICODE_STRING ShortName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_NAME_INFORMATION FileNameInfo;

    if(IsConsoleHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(!lpShortName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlInitUnicodeString(&ShortName, lpShortName);

    NeededSize = sizeof(FILE_NAME_INFORMATION) + ShortName.Length + sizeof(WCHAR);
    if(!(FileNameInfo = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, NeededSize)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    FileNameInfo->FileNameLength = ShortName.Length;
    RtlCopyMemory(FileNameInfo->FileName, ShortName.Buffer, ShortName.Length);

    Status = NtSetInformationFile(hFile,
                                  &IoStatusBlock, //out
                                  FileNameInfo,
                                  NeededSize,
                                  FileShortNameInformation);

    RtlFreeHeap(RtlGetProcessHeap(), 0, FileNameInfo);
    if(!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetFileShortNameA(
    HANDLE hFile,
    LPCSTR lpShortName)
{
    PWCHAR ShortNameW;

    if(IsConsoleHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(!lpShortName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(ShortNameW = FilenameA2W(lpShortName, FALSE)))
        return FALSE;

    return SetFileShortNameW(hFile, ShortNameW);
}


/*
 * @implemented
 */
BOOL
WINAPI
CheckNameLegalDOS8Dot3W(
    LPCWSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PBOOL pbNameContainsSpaces OPTIONAL,
    PBOOL pbNameLegal
    )
{
    UNICODE_STRING Name;
    ANSI_STRING AnsiName;
    BOOLEAN NameContainsSpaces;

    if(lpName == NULL ||
       (lpOemName == NULL && OemNameSize != 0) ||
       pbNameLegal == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

    if(lpOemName != NULL)
    {
      AnsiName.Buffer = lpOemName;
      AnsiName.MaximumLength = (USHORT)OemNameSize * sizeof(CHAR);
      AnsiName.Length = 0;
    }

    RtlInitUnicodeString(&Name, lpName);

    *pbNameLegal = RtlIsNameLegalDOS8Dot3(&Name,
                                          (lpOemName ? &AnsiName : NULL),
                                          &NameContainsSpaces);
    if (*pbNameLegal && pbNameContainsSpaces)
        *pbNameContainsSpaces = NameContainsSpaces;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
CheckNameLegalDOS8Dot3A(
    LPCSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PBOOL pbNameContainsSpaces OPTIONAL,
    PBOOL pbNameLegal
    )
{
    UNICODE_STRING Name;
    ANSI_STRING AnsiName, AnsiInputName;
    NTSTATUS Status;
    BOOLEAN NameContainsSpaces;

    if(lpName == NULL ||
       (lpOemName == NULL && OemNameSize != 0) ||
       pbNameLegal == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

    if(lpOemName != NULL)
    {
      AnsiName.Buffer = lpOemName;
      AnsiName.MaximumLength = (USHORT)OemNameSize * sizeof(CHAR);
      AnsiName.Length = 0;
    }

    RtlInitAnsiString(&AnsiInputName, (LPSTR)lpName);
    if(bIsFileApiAnsi)
      Status = RtlAnsiStringToUnicodeString(&Name, &AnsiInputName, TRUE);
    else
      Status = RtlOemStringToUnicodeString(&Name, &AnsiInputName, TRUE);

    if(!NT_SUCCESS(Status))
    {
      BaseSetLastNTError(Status);
      return FALSE;
    }

    *pbNameLegal = RtlIsNameLegalDOS8Dot3(&Name,
                                          (lpOemName ? &AnsiName : NULL),
                                          &NameContainsSpaces);
    if (*pbNameLegal && pbNameContainsSpaces)
        *pbNameContainsSpaces = NameContainsSpaces;

    RtlFreeUnicodeString(&Name);

    return TRUE;
}

// TODO: let's find a better spot...
DEBUG_CHANNEL(wine);

/**************************************************************************
 *           MoveFileTransactedA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileTransactedA(const char *source, const char *dest, LPPROGRESS_ROUTINE progress, void *data, DWORD flags, HANDLE handle)
{
    FIXME("(%s, %s, %p, %p, %ld, %p) semi-stub\n", debugstr_a(source), debugstr_a(dest), progress, data, flags, handle);

    return MoveFileWithProgressA(source, dest, progress, data, flags);
}

/**************************************************************************
 *           MoveFileTransactedW   (KERNEL32.@)
 */
BOOL WINAPI MoveFileTransactedW(const WCHAR *source, const WCHAR *dest, LPPROGRESS_ROUTINE progress, void *data, DWORD flags, HANDLE handle)
{
    FIXME("(%s, %s, %p, %p, %ld, %p) semi-stub\n", debugstr_w(source), debugstr_w(dest), progress, data, flags, handle);

    return MoveFileWithProgressW(source, dest, progress, data, flags);
}

/*************************************************************************
 *           CreateFileTransactedA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileTransactedA( LPCSTR name, DWORD access, DWORD sharing,
                                                       LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                                       DWORD attributes, HANDLE template,
                                                       HANDLE transaction, PUSHORT version,
                                                       PVOID param )
{
    FIXME("(%s %lx %lx %p %lx %lx %p %p %p %p): semi-stub\n", debugstr_a(name), access, sharing, sa,
           creation, attributes, template, transaction, version, param);
    return CreateFileA(name, access, sharing, sa, creation, attributes, template);
}

/*************************************************************************
 *           CreateFileTransactedW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileTransactedW( LPCWSTR name, DWORD access, DWORD sharing,
                                                       LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                                       DWORD attributes, HANDLE template, HANDLE transaction,
                                                       PUSHORT version, PVOID param )
{
    FIXME("(%s %lx %lx %p %lx %lx %p %p %p %p): semi-stub\n", debugstr_w(name), access, sharing, sa,
           creation, attributes, template, transaction, version, param);
    return CreateFileW(name, access, sharing, sa, creation, attributes, template);
}

/***********************************************************************
 *           CreateDirectoryTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryTransactedA(LPCSTR template, LPCSTR path, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): semi-stub\n", debugstr_a(template), debugstr_a(path), sa, transaction);
    return CreateDirectoryExA(template, path, sa);
}

/***********************************************************************
 *           CreateDirectoryTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryTransactedW(LPCWSTR template, LPCWSTR path, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): semi-stub\n", debugstr_w(template), debugstr_w(path), sa, transaction);
    return CreateDirectoryExW(template, path, sa);
}

/***********************************************************************
 *           DeleteFileTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileTransactedA(LPCSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(path), transaction);
    return DeleteFileA(path);
}

/***********************************************************************
 *           DeleteFileTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileTransactedW(LPCWSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(path), transaction);
    return DeleteFileW(path);
}

/******************************************************************************
 *           FindFirstFileTransactedA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileTransactedA( LPCSTR filename, FINDEX_INFO_LEVELS level,
                                                          LPVOID data, FINDEX_SEARCH_OPS search_op,
                                                          LPVOID filter, DWORD flags, HANDLE transaction )
{
    FIXME("(%s %d %p %d %p %lx %p): semi-stub\n", debugstr_a(filename), level, data, search_op, filter, flags, transaction);
    return FindFirstFileExA(filename, level, data, search_op, filter, flags);
}

/******************************************************************************
 *           FindFirstFileTransactedW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileTransactedW( LPCWSTR filename, FINDEX_INFO_LEVELS level,
                                                          LPVOID data, FINDEX_SEARCH_OPS search_op,
                                                          LPVOID filter, DWORD flags, HANDLE transaction )
{
    FIXME("(%s %d %p %d %p %lx %p): semi-stub\n", debugstr_w(filename), level, data, search_op, filter, flags, transaction);
    return FindFirstFileExW(filename, level, data, search_op, filter, flags);
}


/**************************************************************************
 *           GetFileAttributesTransactedA   (KERNEL32.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesTransactedA(LPCSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(name), transaction);
    return GetFileAttributesExA(name, level, ptr);
}

/**************************************************************************
 *           GetFileAttributesTransactedW   (KERNEL32.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesTransactedW(LPCWSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(name), transaction);
    return GetFileAttributesExW(name, level, ptr);
}

/***********************************************************************
 *           RemoveDirectoryTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDirectoryTransactedA(LPCSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(path), transaction);
    return RemoveDirectoryA(path);
}

/***********************************************************************
 *           RemoveDirectoryTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDirectoryTransactedW(LPCWSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(path), transaction);
    return RemoveDirectoryW(path);
}

/*************************************************************************
 *           CreateHardLinkTransactedA   (KERNEL32.@)
 */
BOOL WINAPI CreateHardLinkTransactedA(LPCSTR link, LPCSTR target, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): stub\n", debugstr_a(link), debugstr_a(target), sa, transaction);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*************************************************************************
 *           CreateHardLinkTransactedW   (KERNEL32.@)
 */
BOOL WINAPI CreateHardLinkTransactedW(LPCWSTR link, LPCWSTR target, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): stub\n", debugstr_w(link), debugstr_w(target), sa, transaction);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
