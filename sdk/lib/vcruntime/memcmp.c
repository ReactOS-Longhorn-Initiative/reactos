/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of memcmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("memcmp")))
#endif
_Check_return_
int
__cdecl
memcmp(
    _In_reads_bytes_(Size) const void* const Buf1,
    _In_reads_bytes_(Size) const void* const Buf2,
    _In_ size_t const Size)
{
    const unsigned char* p1 = (const unsigned char*)Buf1;
    const unsigned char* p2 = (const unsigned char*)Buf2;
    size_t remaining = Size;

    while (remaining--)
    {
        unsigned char chr1 = *p1++;
        unsigned char chr2 = *p2++;
        if (chr1 != chr2)
        {
            return (chr1 > chr2) ? 1 : -1;
        }
    }

    /* This is a match */
    return 0;
}
