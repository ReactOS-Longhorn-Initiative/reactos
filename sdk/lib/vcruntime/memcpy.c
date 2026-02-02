/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of memcpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("memcpy")))
#endif
_Post_equal_to_(_Dst)
_At_buffer_((unsigned char*)_Dst, _Iter_, _Size,
    _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == ((unsigned char*)_Src)[_Iter_]))
void*
__cdecl
memcpy(
    _Out_writes_bytes_all_(Size) void* const Dst,
    _In_reads_bytes_(Size) const void* const Src,
    _In_ size_t const Size)
{
    unsigned char* dest = (unsigned char*)Dst;
    const unsigned char* src = (const unsigned char*)Src;

    if ((Size == 0) || (Dst == Src))
        return Dst;

    /* Copy forwards */
    size_t remaining = Size;
    while (remaining--)
    {
        *dest++ = *src++;
    }

    return Dst;
}
