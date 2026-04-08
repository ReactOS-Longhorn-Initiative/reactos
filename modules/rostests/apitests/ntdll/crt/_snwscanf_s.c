/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _snwscanf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__snwscanf_s)(const wchar_t*, size_t, const wchar_t*, ...);

START_TEST(_snwscanf_s)
{
    PFN__snwscanf_s p_snwscanf_s = (PFN__snwscanf_s)load_function("_snwscanf_s");
    int val;
    wchar_t wstr[32];
    int ret;

    if (!p_snwscanf_s)
        return;

    /* Basic integer scan */
    val = 0xDEAD;
    ret = p_snwscanf_s(L"42", 10, L"%d", &val);
    ok(ret == 1, "_snwscanf_s basic int returned %d, expected 1\n", ret);
    ok(val == 42, "_snwscanf_s basic int val = %d, expected 42\n", val);

    /* Integer scan with count limiting */
    val = 0xDEAD;
    ret = p_snwscanf_s(L"12345", 3, L"%d", &val);
    ok(ret == 1, "_snwscanf_s count=3 returned %d, expected 1\n", ret);
    ok(val == 123, "_snwscanf_s count=3 val = %d, expected 123\n", val);

    /* Integer scan with count=1 */
    val = 0xDEAD;
    ret = p_snwscanf_s(L"42", 1, L"%d", &val);
    ok(ret == 1, "_snwscanf_s count=1 returned %d, expected 1\n", ret);
    ok(val == 4, "_snwscanf_s count=1 val = %d, expected 4\n", val);

    /* String scan with buffer size argument */
    memset(wstr, 0xAA, sizeof(wstr));
    ret = p_snwscanf_s(L"hello", 10, L"%s", wstr, (unsigned)(sizeof(wstr) / sizeof(wchar_t)));
    ok(ret == 1, "_snwscanf_s string returned %d, expected 1\n", ret);
    ok(memcmp(wstr, L"hello", sizeof(L"hello")) == 0, "_snwscanf_s string content mismatch\n");

    /* Multiple fields */
    val = 0xDEAD;
    memset(wstr, 0xAA, sizeof(wstr));
    ret = p_snwscanf_s(L"42 hello", 20, L"%d %s", &val, wstr, (unsigned)(sizeof(wstr) / sizeof(wchar_t)));
    ok(ret == 2, "_snwscanf_s multi returned %d, expected 2\n", ret);
    ok(val == 42, "_snwscanf_s multi val = %d, expected 42\n", val);
    ok(memcmp(wstr, L"hello", sizeof(L"hello")) == 0, "_snwscanf_s multi wstr content mismatch\n");

    /* Count limits scan to partial integer */
    val = 0xDEAD;
    ret = p_snwscanf_s(L"99bottles", 2, L"%d", &val);
    ok(ret == 1, "_snwscanf_s partial returned %d, expected 1\n", ret);
    ok(val == 99, "_snwscanf_s partial val = %d, expected 99\n", val);

    /* Count limits string scan */
    memset(wstr, 0xAA, sizeof(wstr));
    ret = p_snwscanf_s(L"hello", 3, L"%s", wstr, (unsigned)(sizeof(wstr) / sizeof(wchar_t)));
    ok(ret == 1, "_snwscanf_s str count=3 returned %d, expected 1\n", ret);
    ok(memcmp(wstr, L"hel", 3 * sizeof(wchar_t)) == 0, "_snwscanf_s str count=3 content mismatch\n");
    ok(wstr[3] == L'\0', "_snwscanf_s str count=3 null mismatch\n");

    /* Negative integer */
    val = 0xDEAD;
    ret = p_snwscanf_s(L"-7", 10, L"%d", &val);
    ok(ret == 1, "_snwscanf_s negative returned %d, expected 1\n", ret);
    ok(val == -7, "_snwscanf_s negative val = %d, expected -7\n", val);

    /* No matching input */
    val = 0xDEAD;
    ret = p_snwscanf_s(L"abc", 10, L"%d", &val);
    ok(ret == 0, "_snwscanf_s no match returned %d, expected 0\n", ret);
}
