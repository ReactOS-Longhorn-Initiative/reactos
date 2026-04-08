/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wcsnicmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__wcsnicmp)(const wchar_t* _String1, const wchar_t* _String2, size_t _MaxCount);

START_TEST(_wcsnicmp)
{
    PFN__wcsnicmp p_wcsnicmp = (PFN__wcsnicmp)load_function("_wcsnicmp");
    int ret;

    /* Equal strings, count covers all */
    ret = p_wcsnicmp(L"Hello", L"Hello", 10);
    ok(ret == 0, "_wcsnicmp(\"Hello\", \"Hello\", 10) = %d, expected 0\n", ret);

    /* Case-insensitive equality, count covers all */
    ret = p_wcsnicmp(L"Hello", L"HELLO", 10);
    ok(ret == 0, "_wcsnicmp(\"Hello\", \"HELLO\", 10) = %d, expected 0\n", ret);

    ret = p_wcsnicmp(L"HELLO", L"hello", 5);
    ok(ret == 0, "_wcsnicmp(\"HELLO\", \"hello\", 5) = %d, expected 0\n", ret);

    /* Count = 0 always returns 0 */
    ret = p_wcsnicmp(L"ABC", L"XYZ", 0);
    ok(ret == 0, "_wcsnicmp(\"ABC\", \"XYZ\", 0) = %d, expected 0\n", ret);

    /* Difference within count */
    ret = p_wcsnicmp(L"ABC", L"abd", 3);
    ok(ret < 0, "_wcsnicmp(\"ABC\", \"abd\", 3) = %d, expected < 0\n", ret);

    ret = p_wcsnicmp(L"ABD", L"abc", 3);
    ok(ret > 0, "_wcsnicmp(\"ABD\", \"abc\", 3) = %d, expected > 0\n", ret);

    /* Difference beyond count - should be equal */
    ret = p_wcsnicmp(L"ABCX", L"abcy", 3);
    ok(ret == 0, "_wcsnicmp beyond-count = %d, expected 0\n", ret);

    /* First string is prefix, count covers both */
    ret = p_wcsnicmp(L"Hello", L"HELLO WORLD", 11);
    ok(ret < 0, "_wcsnicmp prefix = %d, expected < 0\n", ret);

    /* First string is prefix, count limited to common part */
    ret = p_wcsnicmp(L"Hello", L"HELLO WORLD", 5);
    ok(ret == 0, "_wcsnicmp prefix limited = %d, expected 0\n", ret);

    /* Empty strings */
    ret = p_wcsnicmp(L"", L"", 5);
    ok(ret == 0, "_wcsnicmp(\"\", \"\", 5) = %d, expected 0\n", ret);

    /* Empty vs non-empty */
    ret = p_wcsnicmp(L"", L"A", 5);
    ok(ret < 0, "_wcsnicmp(\"\", \"A\", 5) = %d, expected < 0\n", ret);

    ret = p_wcsnicmp(L"A", L"", 5);
    ok(ret > 0, "_wcsnicmp(\"A\", \"\", 5) = %d, expected > 0\n", ret);

    /* Empty vs non-empty, count = 0 */
    ret = p_wcsnicmp(L"", L"A", 0);
    ok(ret == 0, "_wcsnicmp(\"\", \"A\", 0) = %d, expected 0\n", ret);

    /* Case-insensitive single character */
    ret = p_wcsnicmp(L"A", L"a", 1);
    ok(ret == 0, "_wcsnicmp(\"A\", \"a\", 1) = %d, expected 0\n", ret);

    ret = p_wcsnicmp(L"Z", L"z", 1);
    ok(ret == 0, "_wcsnicmp(\"Z\", \"z\", 1) = %d, expected 0\n", ret);

    /* Verify exact difference value */
    ret = p_wcsnicmp(L"b", L"a", 1);
    ok(ret == 1, "_wcsnicmp(\"b\", \"a\", 1) = %d, expected 1\n", ret);

    ret = p_wcsnicmp(L"a", L"b", 1);
    ok(ret == -1, "_wcsnicmp(\"a\", \"b\", 1) = %d, expected -1\n", ret);

    /* Unsigned comparison: high values unchanged */
    ret = p_wcsnicmp(L"\x8000", L"\x7FFF", 1);
    ok(ret > 0, "_wcsnicmp(\"\\x8000\", \"\\x7FFF\", 1) = %d, expected > 0\n", ret);

    ret = p_wcsnicmp(L"\x7FFF", L"\x8000", 1);
    ok(ret < 0, "_wcsnicmp(\"\\x7FFF\", \"\\x8000\", 1) = %d, expected < 0\n", ret);

    /* Count = 1 on longer strings, first char equal (case-insensitive) */
    ret = p_wcsnicmp(L"ABCDEF", L"axyz", 1);
    ok(ret == 0, "_wcsnicmp count=1 same-first = %d, expected 0\n", ret);

    /* Null terminator encountered before count */
    ret = p_wcsnicmp(L"AB", L"ab", 100);
    ok(ret == 0, "_wcsnicmp short-strings-big-count = %d, expected 0\n", ret);

    /* Mixed case comparison */
    ret = p_wcsnicmp(L"aBcDeF", L"AbCdEf", 6);
    ok(ret == 0, "_wcsnicmp mixed case = %d, expected 0\n", ret);

    /* Characters just outside A-Z range are not lowered */
    ret = p_wcsnicmp(L"@", L"`", 1);
    ok(ret < 0, "_wcsnicmp(\"@\", \"`\", 1) = %d, expected < 0\n", ret);

    ret = p_wcsnicmp(L"[", L"{", 1);
    ok(ret < 0, "_wcsnicmp(\"[\", \"{\", 1) = %d, expected < 0\n", ret);

    /* '_' (0x5F) is between upper case 'A'-'Z' (0x41-0x5A) and lower case
       'a'-'z' (0x61-0x7A). CRT uses lower case folding, so 'A' becomes 'a'
       (0x61), which is greater than '_' (0x5F). This is the opposite of Rtl
       functions that use upper case folding. */
    ret = p_wcsnicmp(L"_", L"A", 1);
    ok(ret < 0, "_wcsnicmp(\"_\", \"A\", 1) = %d, expected < 0\n", ret);
    ret = p_wcsnicmp(L"A", L"_", 1);
    ok(ret > 0, "_wcsnicmp(\"A\", \"_\", 1) = %d, expected > 0\n", ret);
    ret = p_wcsnicmp(L"_", L"a", 1);
    ok(ret < 0, "_wcsnicmp(\"_\", \"a\", 1) = %d, expected < 0\n", ret);
    ret = p_wcsnicmp(L"a", L"_", 1);
    ok(ret > 0, "_wcsnicmp(\"a\", \"_\", 1) = %d, expected > 0\n", ret);
}
