/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of memmove
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("memmove")))
#endif
void*
__cdecl
memmove(
    _Out_writes_bytes_all_opt_(Size) void* const Dst,
    _In_reads_bytes_opt_(Size) const void* const Src,
    _In_ size_t const Size)
{
    unsigned char* dest = (unsigned char*)Dst;
    const unsigned char* src = (const unsigned char*)Src;
    size_t remaining = Size;

    if ((Size == 0) || (Dst == Src))
        return Dst;

    /* Check for overlap */
    if ((dest > src) && (dest < (src + Size)))
    {
        /* Copy backwards */
        dest += Size;
        src += Size;
        while (remaining--)
        {
            *(--dest) = *(--src);
        }
    }
    else
    {
        /* Copy forwards */
        while (remaining--) 
        {
            *dest++ = *src++;
        }
    }

    return Dst;
}
