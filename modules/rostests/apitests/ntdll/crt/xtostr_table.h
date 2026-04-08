/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared declarations for integer-to-string conversion test table
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include <stdlib.h>

/* Flags indicating which function categories an entry applies to */
#define XF_I32  0x001   /* Valid for _itoa/_itoa_s/_itow/_itow_s (value fits in int) */
#define XF_U32  0x002   /* Valid for _ultoa/_ultoa_s/_ultow/_ultow_s (value fits in unsigned long) */
#define XF_I64  0x004   /* Valid for _i64toa/_i64toa_s/_i64tow/_i64tow_s (value fits in __int64) */
#define XF_U64  0x008   /* Valid for _ui64toa/_ui64toa_s/_ui64tow/_ui64tow_s */
#define XF_ALL  (XF_I32 | XF_U32 | XF_I64 | XF_U64)

typedef struct
{
    int line;
    int radix;
    unsigned __int64 value;
    const char* expected;
    unsigned int flags;
} xtoa_entry;

typedef struct
{
    int line;
    int radix;
    unsigned __int64 value;
    const wchar_t* expected;
    unsigned int flags;
} xtow_entry;

extern const xtoa_entry g_xtoa_table[];
extern const size_t g_xtoa_table_length;

extern const xtow_entry g_xtow_table[];
extern const size_t g_xtow_table_length;
