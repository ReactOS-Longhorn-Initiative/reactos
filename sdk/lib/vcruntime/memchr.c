/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of memchr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
*/

#include <vcruntime_string.h>

_NODISCARD
_Check_return_
void*
__cdecl
memchr(
    _In_reads_bytes_opt_(MaxCount) const void* const Buf,
    _In_ int const Val,
    _In_ size_t const MaxCount)
{
    const unsigned char* ptr = (const unsigned char*)Buf;
    unsigned char chr = (unsigned char)Val;

    size_t remaining = MaxCount;
    while (remaining--)
    {
        if (*ptr == chr)
            return (void*)ptr;
        ptr++;
    }

    return NULL;
}
