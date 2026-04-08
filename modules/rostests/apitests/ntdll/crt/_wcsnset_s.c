/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wcsnset_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN__wcsnset_s)(wchar_t* _Destination, size_t _SizeInWords, wchar_t _Value, size_t _MaxCount);

START_TEST(_wcsnset_s)
{
    PFN__wcsnset_s p_wcsnset_s = (PFN__wcsnset_s)load_function("_wcsnset_s");
    wchar_t buf[64];
    errno_t ret;

    if (p_wcsnset_s == NULL)
    {
        skip("Function _wcsnset_s not available\n");
        return;
    }

    /* Basic: count < string length */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsnset_s(buf, sizeof(buf) / sizeof(wchar_t), L'X', 3);
    ok(ret == 0, "_wcsnset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"XXXlo\0\xAAAA", 7 * sizeof(wchar_t)) == 0, "_wcsnset_s basic content mismatch\n");

    /* Count == string length */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsnset_s(buf, sizeof(buf) / sizeof(wchar_t), L'X', 5);
    ok(ret == 0, "_wcsnset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"XXXXX\0\xAAAA", 7 * sizeof(wchar_t)) == 0, "_wcsnset_s full content mismatch\n");

    /* Count > string length - stops at null */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"Hi\0", 3 * sizeof(wchar_t));
    ret = p_wcsnset_s(buf, sizeof(buf) / sizeof(wchar_t), L'X', 10);
    ok(ret == 0, "_wcsnset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"XX\0\xAAAA", 4 * sizeof(wchar_t)) == 0, "_wcsnset_s overflow content mismatch\n");

    /* Count of zero */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsnset_s(buf, sizeof(buf) / sizeof(wchar_t), L'X', 0);
    ok(ret == 0, "_wcsnset_s(count=0) returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"Hello\0\xAAAA", 7 * sizeof(wchar_t)) == 0, "_wcsnset_s(count=0) content mismatch\n");

    /* Empty string with count > 0 */
    memset(buf, 0xAA, sizeof(buf));
    buf[0] = L'\0';
    ret = p_wcsnset_s(buf, sizeof(buf) / sizeof(wchar_t), L'X', 5);
    ok(ret == 0, "_wcsnset_s empty returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"\0\xAAAA", 2 * sizeof(wchar_t)) == 0, "_wcsnset_s empty content mismatch\n");

    /* Exact buffer size */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"ABC\0", 4 * sizeof(wchar_t));
    ret = p_wcsnset_s(buf, 4, L'Z', 2);
    ok(ret == 0, "_wcsnset_s exact returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"ZZC\0\xAAAA", 5 * sizeof(wchar_t)) == 0, "_wcsnset_s exact content mismatch\n");

    /* High Unicode value */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"AB\0", 3 * sizeof(wchar_t));
    ret = p_wcsnset_s(buf, sizeof(buf) / sizeof(wchar_t), (wchar_t)0xFFFE, 1);
    ok(ret == 0, "_wcsnset_s high returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"\xFFFE" L"B\0\xAAAA", 4 * sizeof(wchar_t)) == 0, "_wcsnset_s high content mismatch\n");

    /* Zero size */
    memset(buf, 0xAA, sizeof(buf));
    ret = p_wcsnset_s(buf, 0, L'X', 5);
    ok(ret == EINVAL, "_wcsnset_s(size=0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(buf[0] == (wchar_t)0xAAAA, "_wcsnset_s(size=0): buf modified\n");

    /* No null terminator within size */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"ABCDEF", 6 * sizeof(wchar_t));
    ret = p_wcsnset_s(buf, 3, L'X', 5);
    ok(ret == EINVAL, "_wcsnset_s no-null returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(memcmp(buf, L"\0XCDEF\xAAAA", 7 * sizeof(wchar_t)) == 0, "_wcsnset_s no-null content mismatch\n");

    /* NULL string */
    ret = p_wcsnset_s(NULL, 10, L'X', 5);
    ok(ret == EINVAL, "_wcsnset_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* NULL string and zero size */
    ret = p_wcsnset_s(NULL, 0, L'X', 5);
    ok(ret == EINVAL, "_wcsnset_s(NULL, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* NULL string with zero count */
    ret = p_wcsnset_s(NULL, 10, L'X', 0);
    ok(ret == EINVAL, "_wcsnset_s(NULL, 10, X, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
}
