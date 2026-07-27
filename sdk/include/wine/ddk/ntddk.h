

#pragma once

/* User-mode-callable subset of Wine's <ddk/ntddk.h>. Everything here is
 * exported by ntdll; the kernel-only bits of the real DDK header are not
 * reproduced. */
void      WINAPI RtlUpperString(STRING*, const STRING*);
LONG      WINAPI RtlCompareString(const STRING*,const STRING*,BOOLEAN);
void      WINAPI RtlCopyString(STRING*,const STRING*);
BOOLEAN   WINAPI RtlEqualString(const STRING*,const STRING*,BOOLEAN);
void      WINAPI RtlMapGenericMask(ACCESS_MASK*,const GENERIC_MAPPING*);
BOOLEAN   WINAPI RtlPrefixUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  WINAPI RtlUpcaseUnicodeString(UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
CHAR      WINAPI RtlUpperChar(CHAR);

#ifndef _NTDDK_FILE_VALID_DATA_LENGTH_INFORMATION_DEFINED
#define _NTDDK_FILE_VALID_DATA_LENGTH_INFORMATION_DEFINED
typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION
{
    LARGE_INTEGER ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;
#endif

typedef struct _PROCESS_ACCESS_TOKEN
{
    HANDLE Token;
    HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;
