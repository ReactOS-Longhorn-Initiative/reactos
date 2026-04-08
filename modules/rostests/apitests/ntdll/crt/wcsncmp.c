/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcsncmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_wcsncmp)(const wchar_t* _String1, const wchar_t* _String2, size_t _MaxCount);

START_TEST(wcsncmp)
{
    PFN_wcsncmp p_wcsncmp = (PFN_wcsncmp)load_function("wcsncmp");
    int ret;

    /* Equal strings, count covers all */
    ret = p_wcsncmp(L"Hello", L"Hello", 10);
    ok(ret == 0, "wcsncmp(\"Hello\", \"Hello\", 10) = %d, expected 0\n", ret);

    /* Equal strings, count is exact length */
    ret = p_wcsncmp(L"Hello", L"Hello", 5);
    ok(ret == 0, "wcsncmp(\"Hello\", \"Hello\", 5) = %d, expected 0\n", ret);

    /* Count = 0 always returns 0 */
    ret = p_wcsncmp(L"ABC", L"XYZ", 0);
    ok(ret == 0, "wcsncmp(\"ABC\", \"XYZ\", 0) = %d, expected 0\n", ret);

    /* Difference within count */
    ret = p_wcsncmp(L"ABC", L"ABD", 3);
    ok(ret < 0, "wcsncmp(\"ABC\", \"ABD\", 3) = %d, expected < 0\n", ret);

    ret = p_wcsncmp(L"ABD", L"ABC", 3);
    ok(ret > 0, "wcsncmp(\"ABD\", \"ABC\", 3) = %d, expected > 0\n", ret);

    /* Difference beyond count - should be equal */
    ret = p_wcsncmp(L"ABCX", L"ABCY", 3);
    ok(ret == 0, "wcsncmp beyond-count = %d, expected 0\n", ret);

    /* First string is prefix, count covers both */
    ret = p_wcsncmp(L"Hello", L"Hello World", 11);
    ok(ret < 0, "wcsncmp prefix = %d, expected < 0\n", ret);

    /* First string is prefix, count limited to common part */
    ret = p_wcsncmp(L"Hello", L"Hello World", 5);
    ok(ret == 0, "wcsncmp prefix limited = %d, expected 0\n", ret);

    /* Empty strings */
    ret = p_wcsncmp(L"", L"", 5);
    ok(ret == 0, "wcsncmp(\"\", \"\", 5) = %d, expected 0\n", ret);

    /* Empty vs non-empty */
    ret = p_wcsncmp(L"", L"A", 5);
    ok(ret < 0, "wcsncmp(\"\", \"A\", 5) = %d, expected < 0\n", ret);

    ret = p_wcsncmp(L"A", L"", 5);
    ok(ret > 0, "wcsncmp(\"A\", \"\", 5) = %d, expected > 0\n", ret);

    /* Empty vs non-empty, count = 0 */
    ret = p_wcsncmp(L"", L"A", 0);
    ok(ret == 0, "wcsncmp(\"\", \"A\", 0) = %d, expected 0\n", ret);

    /* Unsigned comparison: 0x8000 > 0x7FFF */
    ret = p_wcsncmp(L"\x8000", L"\x7FFF", 1);
    ok(ret > 0, "wcsncmp(\"\\x8000\", \"\\x7FFF\", 1) = %d, expected > 0\n", ret);

    ret = p_wcsncmp(L"\x7FFF", L"\x8000", 1);
    ok(ret < 0, "wcsncmp(\"\\x7FFF\", \"\\x8000\", 1) = %d, expected < 0\n", ret);

    /* Verify exact difference value */
    ret = p_wcsncmp(L"B", L"A", 1);
    ok(ret == 1, "wcsncmp(\"B\", \"A\", 1) = %d, expected 1\n", ret);

    ret = p_wcsncmp(L"A", L"B", 1);
    ok(ret == -1, "wcsncmp(\"A\", \"B\", 1) = %d, expected -1\n", ret);

    /* Count = 1 on longer strings, first char equal */
    ret = p_wcsncmp(L"ABCDEF", L"AXYZ", 1);
    ok(ret == 0, "wcsncmp count=1 same-first = %d, expected 0\n", ret);

    /* Null terminator encountered before count */
    ret = p_wcsncmp(L"AB", L"AB", 100);
    ok(ret == 0, "wcsncmp short-strings-big-count = %d, expected 0\n", ret);
}
