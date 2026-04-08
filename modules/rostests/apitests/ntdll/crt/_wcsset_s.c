/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wcsset_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN__wcsset_s)(wchar_t* _Destination, size_t _SizeInWords, wchar_t _Value);

START_TEST(_wcsset_s)
{
    PFN__wcsset_s p_wcsset_s = (PFN__wcsset_s)load_function("_wcsset_s");
    wchar_t buf[64];
    errno_t ret;

    if (p_wcsset_s == NULL)
    {
        skip("Function _wcsset_s not available\n");
        return;
    }

    /* Basic: fill entire string */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsset_s(buf, sizeof(buf) / sizeof(wchar_t), L'X');
    ok(ret == 0, "_wcsset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"XXXXX\0\xAAAA", 7 * sizeof(wchar_t)) == 0, "_wcsset_s basic content mismatch\n");
    /* Single character string */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"A\0", 2 * sizeof(wchar_t));
    ret = p_wcsset_s(buf, sizeof(buf) / sizeof(wchar_t), L'Z');
    ok(ret == 0, "_wcsset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"Z\0\xAAAA", 3 * sizeof(wchar_t)) == 0, "_wcsset_s single char content mismatch\n");

    /* Empty string */
    memset(buf, 0xAA, sizeof(buf));
    buf[0] = L'\0';
    ret = p_wcsset_s(buf, sizeof(buf) / sizeof(wchar_t), L'X');
    ok(ret == 0, "_wcsset_s empty returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"\0\xAAAA", 2 * sizeof(wchar_t)) == 0, "_wcsset_s empty content mismatch\n");

    /* Exact size */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"ABC\0", 4 * sizeof(wchar_t));
    ret = p_wcsset_s(buf, 4, L'Y');
    ok(ret == 0, "_wcsset_s exact returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"YYY\0\xAAAA", 5 * sizeof(wchar_t)) == 0, "_wcsset_s exact content mismatch\n");

    /* Fill with null character */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsset_s(buf, sizeof(buf) / sizeof(wchar_t), L'\0');
    ok(ret == 0, "_wcsset_s fill-null returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"\0\0\0\0\0\0\xAAAA", 7 * sizeof(wchar_t)) == 0, "_wcsset_s fill-null content mismatch\n");

    /* High Unicode value */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"AB\0", 3 * sizeof(wchar_t));
    ret = p_wcsset_s(buf, sizeof(buf) / sizeof(wchar_t), (wchar_t)0xFFFE);
    ok(ret == 0, "_wcsset_s high unicode returned %d, expected 0\n", ret);
    ok(memcmp(buf, L"\xFFFE\xFFFE\0\xAAAA", 4 * sizeof(wchar_t)) == 0, "_wcsset_s high unicode content mismatch\n");

    /* Zero size */
    memset(buf, 0xAA, sizeof(buf));
    ret = p_wcsset_s(buf, 0, L'X');
    ok(ret == EINVAL, "_wcsset_s(size=0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(buf[0] == (wchar_t)0xAAAA, "_wcsset_s(size=0): buf modified\n");

    /* No null terminator within size */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"ABCDEF", 6 * sizeof(wchar_t));
    ret = p_wcsset_s(buf, 3, L'X');
    ok(ret == EINVAL, "_wcsset_s no-null returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(memcmp(buf, L"\0XCDEF\xAAAA", 7 * sizeof(wchar_t)) == 0, "_wcsset_s truncated content mismatch\n");

    /* NULL string */
    ret = p_wcsset_s(NULL, 10, L'X');
    ok(ret == EINVAL, "_wcsset_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* NULL string and zero size */
    ret = p_wcsset_s(NULL, 0, L'X');
    ok(ret == EINVAL, "_wcsset_s(NULL, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
}
