/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of memset
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

#ifdef _MSC_VER
#pragma function(memset)
#endif

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("memset")))
#endif
void*
__cdecl
memset(
    _Out_writes_bytes_all_(Size) void* const Dst,
    _In_ int const Val,
    _In_ size_t const Size)
{
    unsigned char* byte_ptr = (unsigned char*)Dst;
    unsigned char byte = (unsigned char)Val;
    size_t bytes_remaining = Size;
    if (bytes_remaining == 0)
        return Dst;

    /* Align to word boundary */
    size_t align_mask = sizeof(size_t) - 1;
    size_t unalignment = (uintptr_t)byte_ptr & align_mask;
    size_t unaligned_bytes = (sizeof(size_t) - unalignment) & align_mask;

    if (unaligned_bytes > bytes_remaining)
        unaligned_bytes = bytes_remaining;

    bytes_remaining -= unaligned_bytes;

    while (unaligned_bytes)
    {
        *byte_ptr++ = byte;
        unaligned_bytes--;
    }

    /* Build repeated byte pattern */
    size_t word = byte * (size_t)0x0101010101010101ULL;

    /* Set word-sized chunks */
    size_t* word_ptr = (size_t*)byte_ptr;
    size_t words_remaining = bytes_remaining / sizeof(size_t);

    while (words_remaining)
    {
        *word_ptr++ = word;
        words_remaining--;
    }

    /* Set remaining bytes */
    byte_ptr = (unsigned char*)word_ptr;
    bytes_remaining %= sizeof(size_t);

    while (bytes_remaining)
    {
        *byte_ptr++ = byte;
        bytes_remaining--;
    }

    return Dst;
}
