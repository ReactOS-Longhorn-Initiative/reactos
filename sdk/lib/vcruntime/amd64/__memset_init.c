/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __memset_init for x64 (determines the best memset implementation to use at runtime)
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <intrin.h>
#include <windef.h>
#include <winbase.h>

void* __memset_init(void*, int, size_t);
void* __memset_sse2(void*, int, size_t);
void* __memset_erms(void*, int, size_t);
void* __memset_stosb(void*, int, size_t);
void* __memset_stosq(void*, int, size_t);
void* __memset_c(void*, int, size_t);

/* The global function pointer */
void* (*__memset_fnptr)(void* _Dst, int _Val, size_t _Size) = __memset_init;

void*
__memset_init(
    _Out_writes_bytes_all_(_Size) void* _Dst,
    _In_ int _Val,
    _In_ size_t _Size)
{
    int cpuInfo[4];

    /* Check for ERMS support (CPUID leaf 7, sub-leaf 0, EBX bit 9) */
    __cpuidex(cpuInfo, 7, 0);
    if ((cpuInfo[1] & (1 << 9)) != 0)
    {
        /* Check for FSRM support (CPUID leaf 7, sub-leaf 0, EBX bit 10) */
        if ((cpuInfo[1] & (1 << 10)) != 0)
        {
            /* Always use 'rep stosb' directly */
            __memset_fnptr = __memset_stosb;
        }
        else
        {
            /* This one uses 'rep stosb' for larger operations, and SSE for shorter */
            __memset_fnptr = __memset_erms;
        }
    }
    else
    {
        /* Use the default SSE2 implementation (never AVX) */
        __memset_fnptr = __memset_sse2;
    }

    return memset(_Dst, _Val, _Size);
}
