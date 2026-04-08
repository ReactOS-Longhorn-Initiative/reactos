/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wctomb
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include "../ntrtl.h"

int
__cdecl
wctomb(
    _Out_writes_opt_z_(MB_LEN_MAX) char* const _MbCh,
    _In_ wchar_t _WCh)
{
    NTSTATUS status;
    ULONG size;

    if (_MbCh == NULL)
        return 0;

    status = RtlUnicodeToMultiByteN(_MbCh, 1, &size, &_WCh, sizeof(WCHAR));
    if (!NT_SUCCESS(status))
        return -1;

    return size;
}
