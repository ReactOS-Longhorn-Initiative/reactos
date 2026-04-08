/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for towupper
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

#include <minwindef.h>

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(
    _In_ WCHAR SourceCharacter);

typedef wint_t (*PFN_towupper)(wint_t c);

wint_t __cdecl expected_towupper(wint_t c)
{
    return RtlUpcaseUnicodeChar((WCHAR)c);
}

START_TEST(towupper)
{
    PFN_towupper p_towupper = load_function("towupper");

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        wint_t expected = expected_towupper(c);
        wint_t ret = p_towupper(c);
        ok(ret == expected, "towupper(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}
