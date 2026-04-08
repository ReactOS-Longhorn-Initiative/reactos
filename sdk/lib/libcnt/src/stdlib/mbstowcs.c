/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of mbstowcs
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include "../ntrtl.h"

size_t
__cdecl
mbstowcs(
    _Out_writes_opt_z_(_MaxCount) wchar_t* const _Dest,
    _In_z_ char const* const _Source,
    _In_ size_t const _MaxCount)
{
    NTSTATUS status;
    ULONG size;

    if (_Source == NULL)
        return (size_t)-1;

    size = (ULONG)strlen(_Source);
    if (_Dest == NULL)
    {
        RtlMultiByteToUnicodeSize(&size, _Source, size);
        return size / sizeof(wchar_t);
    }

    status = RtlMultiByteToUnicodeN(_Dest,
                                    (ULONG)_MaxCount * sizeof(wchar_t),
                                    &size,
                                    _Source,
                                    size);
    if (!NT_SUCCESS(status))
        return (size_t)-1;

    return size / sizeof(wchar_t);
}
