/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of mbtowc
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include "../ntrtl.h"

int
__cdecl
mbtowc(
    _Pre_notnull_ _Post_z_ wchar_t* _DstCh,
    _In_reads_or_z_opt_(_SrcSizeInBytes) char const* _SrcCh,
    _In_ size_t _SrcSizeInBytes)
{
    NTSTATUS status;
    ULONG size;

    if (_SrcCh == NULL)
        return 0;

    status = RtlMultiByteToUnicodeN(_DstCh,
                                    sizeof(wchar_t),
                                    &size,
                                    _SrcCh,
                                    (ULONG)_SrcSizeInBytes);
    if (!NT_SUCCESS(status))
        return -1;

    return size;
}
