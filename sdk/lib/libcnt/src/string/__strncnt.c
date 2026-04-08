/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __strncnt
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

size_t
__cdecl
__strncnt(
    _In_reads_or_z_(_Count) char const* String,
    _In_ size_t Count)
{
    size_t i;

    for (i = 0; (i < Count) && (String[i] != '\0'); i++)
    {
        /* nothing */
    }

    return i;
}
