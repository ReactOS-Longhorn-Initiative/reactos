/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _snwprintf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <stdio.h>

typedef int (__cdecl *PFN__snwprintf_s)(wchar_t*, size_t, size_t, const wchar_t*, ...);

#define ok_snwprintf_s_(File, Line, ExpectedRet, ExpectedContent, BufSize, MaxCount, Format, ...) \
    do { \
        wchar_t buf[64]; \
        int ret; \
        memset(buf, 0xAA, sizeof(buf)); \
        ret = p_snwprintf_s(buf, BufSize, MaxCount, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "_snwprintf_s returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - sizeof(wchar_t)) == 0, "_snwprintf_s content mismatch\n"); \
    } while (0)

#define ok_snwprintf_s(ExpectedRet, ExpectedContent, BufSize, MaxCount, Format, ...) \
    ok_snwprintf_s_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, BufSize, MaxCount, Format, ##__VA_ARGS__)

START_TEST(_snwprintf_s)
{
    PFN__snwprintf_s p_snwprintf_s = (PFN__snwprintf_s)load_function("_snwprintf_s");
    int ret;

    if (!p_snwprintf_s)
        return;

    /* Basic with large buffer and large maxCount */
    ok_snwprintf_s(5, L"Hello\0\xAAAA", 64, 63, L"Hello");

    /* Exact buffer size (bufCount=6, maxCount=5) */
    ok_snwprintf_s(5, L"Hello\0\xAAAA", 6, 5, L"Hello");

    /* _TRUNCATE with sufficient buffer */
    ok_snwprintf_s(5, L"Hello\0\xAAAA", 64, _TRUNCATE, L"Hello");

    /* _TRUNCATE with small buffer - truncates and null-terminates */
    ok_snwprintf_s(-1, L"Hel\0\xAAAA", 4, _TRUNCATE, L"Hello");

    /* _TRUNCATE with buffer size 1 - writes only null */
    ok_snwprintf_s(-1, L"\0\xAAAA", 1, _TRUNCATE, L"Hello");

    /* _TRUNCATE with buffer size 2 - writes 1 char + null */
    ok_snwprintf_s(-1, L"H\0\xAAAA", 2, _TRUNCATE, L"Hello");

    /* maxCount exceeded without _TRUNCATE: truncates to maxCount */
    ok_snwprintf_s(-1, L"Hel\0\xAAAA", 64, 3, L"Hello");

    /* Empty format string */
    ok_snwprintf_s(0, L"\0\xAAAA", 64, 63, L"");

    /* NULL buffer with zero size */
    ret = p_snwprintf_s(NULL, 0, 0, L"Hello");
    ok(ret == 0, "_snwprintf_s NULL buf returned %d, expected 0\n", ret);

    /* Argument passing */
    ok_snwprintf_s(4, L"x=42\0\xAAAA", 64, 63, L"%s=%d", L"x", 42);

    /* Argument passing with _TRUNCATE and small buffer */
    ok_snwprintf_s(-1, L"x=\0\xAAAA", 3, _TRUNCATE, L"%s=%d", L"x", 42);
}
