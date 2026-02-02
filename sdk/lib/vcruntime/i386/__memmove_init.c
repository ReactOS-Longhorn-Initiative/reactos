/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __memset_init for x86 (determines the best memset implementation to use at runtime)
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <intrin.h>

void* __memmove_init(void*, const void*, size_t);
void* __memmove_erms(void*, const void*, size_t);
void* __memmove_fsrm(void*, const void*, size_t);
void* __memmove_x86(void*, const void*, size_t);
void* _memmove_movsb(void*, const void*, size_t);

/* The global function pointer */
void* (*__memmove_fnptr)(void* Dst, const void* Src, size_t Size) = __memmove_init;

void*
__memmove_init(
    _Out_writes_bytes_all_(Size) void* Dst,
    _In_ const void* Src,
    _In_ size_t Size)
{
    int cpuInfo[4];

    /* Check for ERMS support (CPUID leaf 7, sub-leaf 0, EBX bit 9) */
    __cpuidex(cpuInfo, 7, 0);
    if ((cpuInfo[1] & (1 << 9)) != 0)
    {
        /* Check for FSRM support (CPUID leaf 7, sub-leaf 0, EBX bit 10) */
        if ((cpuInfo[1] & (1 << 10)) != 0)
        {
            /* Use 'rep stosb' directly for not too small sizes */
            __memmove_fnptr = __memmove_fsrm;
        }
        else
        {
            /* Use 'rep stosb' for larger sizes */
            __memmove_fnptr = __memmove_erms;
        }
    }
    else
    {
        /* Use the default x86 implementation (never SSE, etc.) */
        __memmove_fnptr = __memmove_x86;
    }

    /* Now that the implementation is set, call memmove to satisfy the initial request */
    return memmove(Dst, Src, Size);
}
