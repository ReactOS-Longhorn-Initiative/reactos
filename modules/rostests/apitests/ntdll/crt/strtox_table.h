/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared declarations for string-to-integer conversion test table
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include <stdlib.h>

#define F_NEG       0x001   /* Result is negative */
#define F_NODIGITS  0x002   /* No valid digits parsed, endptr = start */
#define F_OVF_S32   0x004   /* Magnitude overflows signed 32-bit range */
#define F_OVF_U32   0x008   /* Magnitude overflows unsigned 32-bit range */
#define F_OVF_S64   0x010   /* Magnitude overflows signed 64-bit range */
#define F_OVF_U64   0x020   /* Magnitude overflows unsigned 64-bit range */

typedef struct
{
    int line;
    int radix;
    const char* string;
    __int64 value;
    int endptr_offset;
    unsigned int flags;
} strtox_entry;

typedef struct
{
    int line;
    int radix;
    const wchar_t* string;
    __int64 value;
    int endptr_offset;
    unsigned int flags;
} wcstox_entry;

extern const strtox_entry g_strtox_table[];
extern const size_t g_strtox_table_length;

extern const wcstox_entry g_wcstox_table[];
extern const size_t g_wcstox_table_length;

