/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wcsicmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__wcsicmp)(const wchar_t* _String1, const wchar_t* _String2);

START_TEST(_wcsicmp)
{
    PFN__wcsicmp p_wcsicmp = (PFN__wcsicmp)load_function("_wcsicmp");
    int ret;

    /* Equal strings */
    ret = p_wcsicmp(L"Hello", L"Hello");
    ok(ret == 0, "_wcsicmp(\"Hello\", \"Hello\") = %d, expected 0\n", ret);

    /* Case-insensitive equality */
    ret = p_wcsicmp(L"Hello", L"HELLO");
    ok(ret == 0, "_wcsicmp(\"Hello\", \"HELLO\") = %d, expected 0\n", ret);

    ret = p_wcsicmp(L"HELLO", L"hello");
    ok(ret == 0, "_wcsicmp(\"HELLO\", \"hello\") = %d, expected 0\n", ret);

    ret = p_wcsicmp(L"hello", L"HELLO");
    ok(ret == 0, "_wcsicmp(\"hello\", \"HELLO\") = %d, expected 0\n", ret);

    /* Empty strings */
    ret = p_wcsicmp(L"", L"");
    ok(ret == 0, "_wcsicmp(\"\", \"\") = %d, expected 0\n", ret);

    /* First string less than second (case-insensitive) */
    ret = p_wcsicmp(L"ABC", L"abd");
    ok(ret < 0, "_wcsicmp(\"ABC\", \"abd\") = %d, expected < 0\n", ret);

    /* First string greater than second (case-insensitive) */
    ret = p_wcsicmp(L"ABD", L"abc");
    ok(ret > 0, "_wcsicmp(\"ABD\", \"abc\") = %d, expected > 0\n", ret);

    /* First string is prefix of second */
    ret = p_wcsicmp(L"Hello", L"Hello World");
    ok(ret < 0, "_wcsicmp prefix = %d, expected < 0\n", ret);

    /* Second string is prefix of first */
    ret = p_wcsicmp(L"Hello World", L"Hello");
    ok(ret > 0, "_wcsicmp prefix2 = %d, expected > 0\n", ret);

    /* Empty vs non-empty */
    ret = p_wcsicmp(L"", L"A");
    ok(ret < 0, "_wcsicmp(\"\", \"A\") = %d, expected < 0\n", ret);

    ret = p_wcsicmp(L"A", L"");
    ok(ret > 0, "_wcsicmp(\"A\", \"\") = %d, expected > 0\n", ret);

    /* Single character case-insensitive */
    ret = p_wcsicmp(L"A", L"a");
    ok(ret == 0, "_wcsicmp(\"A\", \"a\") = %d, expected 0\n", ret);

    ret = p_wcsicmp(L"a", L"A");
    ok(ret == 0, "_wcsicmp(\"a\", \"A\") = %d, expected 0\n", ret);

    ret = p_wcsicmp(L"Z", L"z");
    ok(ret == 0, "_wcsicmp(\"Z\", \"z\") = %d, expected 0\n", ret);

    /* Verify exact difference value */
    ret = p_wcsicmp(L"b", L"a");
    ok(ret == 1, "_wcsicmp(\"b\", \"a\") = %d, expected 1\n", ret);

    ret = p_wcsicmp(L"a", L"b");
    ok(ret == -1, "_wcsicmp(\"a\", \"b\") = %d, expected -1\n", ret);

    ret = p_wcsicmp(L"B", L"A");
    ok(ret == 1, "_wcsicmp(\"B\", \"A\") = %d, expected 1\n", ret);

    /* Only ASCII letters are lowered, high values are unchanged */
    ret = p_wcsicmp(L"\xFFFF", L"\x0001");
    ok(ret == 0xFFFE, "_wcsicmp(\"\\xFFFF\", \"\\x0001\") = %d, expected 65534\n", ret);

    ret = p_wcsicmp(L"\x8000", L"\x7FFF");
    ok(ret > 0, "_wcsicmp(\"\\x8000\", \"\\x7FFF\") = %d, expected > 0\n", ret);

    /* Mixed case in the middle */
    ret = p_wcsicmp(L"aBcDeF", L"AbCdEf");
    ok(ret == 0, "_wcsicmp mixed case = %d, expected 0\n", ret);

    /* Difference in the middle, case-insensitive */
    ret = p_wcsicmp(L"ABCXEF", L"abcdef");
    ok(ret > 0, "_wcsicmp mid-diff = %d, expected > 0\n", ret);

    ret = p_wcsicmp(L"abcdef", L"ABCXEF");
    ok(ret < 0, "_wcsicmp mid-diff2 = %d, expected < 0\n", ret);

    /* Characters just outside A-Z range are not lowered */
    ret = p_wcsicmp(L"@", L"`");
    ok(ret < 0, "_wcsicmp(\"@\", \"`\") = %d, expected < 0\n", ret);

    ret = p_wcsicmp(L"[", L"{");
    ok(ret < 0, "_wcsicmp(\"[\", \"{\") = %d, expected < 0\n", ret);

    /* '_' (0x5F) is between upper case 'A'-'Z' (0x41-0x5A) and lower case
       'a'-'z' (0x61-0x7A). CRT uses lower case folding, so 'A' becomes 'a'
       (0x61), which is greater than '_' (0x5F). This is the opposite of Rtl
       functions that use upper case folding. */
    ret = p_wcsicmp(L"_", L"A");
    ok(ret < 0, "_wcsicmp(\"_\", \"A\") = %d, expected < 0\n", ret);
    ret = p_wcsicmp(L"A", L"_");
    ok(ret > 0, "_wcsicmp(\"A\", \"_\") = %d, expected > 0\n", ret);
    ret = p_wcsicmp(L"_", L"a");
    ok(ret < 0, "_wcsicmp(\"_\", \"a\") = %d, expected < 0\n", ret);
    ret = p_wcsicmp(L"a", L"_");
    ok(ret > 0, "_wcsicmp(\"a\", \"_\") = %d, expected > 0\n", ret);
}
