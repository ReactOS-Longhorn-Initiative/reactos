/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcscmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_wcscmp)(const wchar_t* _String1, const wchar_t* _String2);

START_TEST(wcscmp)
{
    PFN_wcscmp p_wcscmp = (PFN_wcscmp)load_function("wcscmp");
    int ret;

    /* Equal strings */
    ret = p_wcscmp(L"Hello", L"Hello");
    ok(ret == 0, "wcscmp(\"Hello\", \"Hello\") = %d, expected 0\n", ret);

    /* Empty strings */
    ret = p_wcscmp(L"", L"");
    ok(ret == 0, "wcscmp(\"\", \"\") = %d, expected 0\n", ret);

    /* First string less than second */
    ret = p_wcscmp(L"ABC", L"ABD");
    ok(ret < 0, "wcscmp(\"ABC\", \"ABD\") = %d, expected < 0\n", ret);

    /* First string greater than second */
    ret = p_wcscmp(L"ABD", L"ABC");
    ok(ret > 0, "wcscmp(\"ABD\", \"ABC\") = %d, expected > 0\n", ret);

    /* First string is prefix of second */
    ret = p_wcscmp(L"Hello", L"Hello World");
    ok(ret < 0, "wcscmp prefix = %d, expected < 0\n", ret);

    /* Second string is prefix of first */
    ret = p_wcscmp(L"Hello World", L"Hello");
    ok(ret > 0, "wcscmp prefix2 = %d, expected > 0\n", ret);

    /* Empty vs non-empty */
    ret = p_wcscmp(L"", L"A");
    ok(ret < 0, "wcscmp(\"\", \"A\") = %d, expected < 0\n", ret);

    ret = p_wcscmp(L"A", L"");
    ok(ret > 0, "wcscmp(\"A\", \"\") = %d, expected > 0\n", ret);

    /* Single character */
    ret = p_wcscmp(L"A", L"A");
    ok(ret == 0, "wcscmp(\"A\", \"A\") = %d, expected 0\n", ret);

    ret = p_wcscmp(L"A", L"B");
    ok(ret < 0, "wcscmp(\"A\", \"B\") = %d, expected < 0\n", ret);

    ret = p_wcscmp(L"B", L"A");
    ok(ret > 0, "wcscmp(\"B\", \"A\") = %d, expected > 0\n", ret);

    /* Comparison must be unsigned: 0x8000 > 0x7FFF */
    ret = p_wcscmp(L"\x8000", L"\x7FFF");
    ok(ret > 0, "wcscmp(\"\\x8000\", \"\\x7FFF\") = %d, expected > 0\n", ret);

    ret = p_wcscmp(L"\x7FFF", L"\x8000");
    ok(ret < 0, "wcscmp(\"\\x7FFF\", \"\\x8000\") = %d, expected < 0\n", ret);

    /* High values: 0xFFFF vs 0x0001 */
    ret = p_wcscmp(L"\xFFFF", L"\x0001");
    ok(ret > 0, "wcscmp(\"\\xFFFF\", \"\\x0001\") = %d, expected > 0\n", ret);

    ret = p_wcscmp(L"\x0001", L"\xFFFF");
    ok(ret < 0, "wcscmp(\"\\x0001\", \"\\xFFFF\") = %d, expected < 0\n", ret);

    /* Difference in the middle */
    ret = p_wcscmp(L"ABCXEF", L"ABCDEF");
    ok(ret > 0, "wcscmp mid-diff = %d, expected > 0\n", ret);

    /* Verify exact difference value */
    ret = p_wcscmp(L"B", L"A");
    ok(ret == 1, "wcscmp(\"B\", \"A\") = %d, expected 1\n", ret);

    ret = p_wcscmp(L"A", L"B");
    ok(ret == -1, "wcscmp(\"A\", \"B\") = %d, expected -1\n", ret);

    ret = p_wcscmp(L"\xFFFF", L"\x0001");
    ok(ret == 1, "wcscmp(\"\\xFFFF\", \"\\x0001\") = %d, expected 1\n", ret);
}
