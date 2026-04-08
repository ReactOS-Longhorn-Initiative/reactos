/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for swprintf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_swprintf_s)(wchar_t*, size_t, const wchar_t*, ...);

#define ok_swprintf_s_(File, Line, ExpectedRet, ExpectedContent, BufSize, Format, ...) \
    do { \
        wchar_t buf[64]; \
        int ret; \
        memset(buf, 0xAA, sizeof(buf)); \
        ret = p_swprintf_s(buf, BufSize, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "swprintf_s returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - sizeof(wchar_t)) == 0, "swprintf_s content mismatch\n"); \
    } while (0)

#define ok_swprintf_s(ExpectedRet, ExpectedContent, BufSize, Format, ...) \
    ok_swprintf_s_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, BufSize, Format, ##__VA_ARGS__)

START_TEST(swprintf_s)
{
    PFN_swprintf_s p_swprintf_s = (PFN_swprintf_s)load_function("swprintf_s");
    wchar_t buf[64];
    int ret;

    if (!p_swprintf_s)
        return;

    /* Basic with large buffer */
    ok_swprintf_s(5, L"Hello\0\xAAAA", 64, L"Hello");

    /* Exact buffer size (5 chars + null = 6 wchar_t) */
    ok_swprintf_s(5, L"Hello\0\xAAAA", 6, L"Hello");

    /* Buffer too small */
    ok_swprintf_s(-1, L"\0", 3, L"Hello");

    /* Buffer size 1 with non-empty format */
    ok_swprintf_s(-1, L"\0", 1, L"Hello");

    /* Empty format string */
    ok_swprintf_s(0, L"\0\xAAAA", 64, L"");

    /* Buffer size 1 with empty format */
    ok_swprintf_s(0, L"\0\xAAAA", 1, L"");

    /* Argument passing - multiple arguments */
    ok_swprintf_s(4, L"x=42\0\xAAAA", 64, L"%s=%d", L"x", 42);

    /* NULL buffer */
    ret = p_swprintf_s(NULL, 10, L"Hello");
    ok(ret == -1, "swprintf_s NULL buf returned %d, expected -1\n", ret);

    /* Zero buffer count */
    ret = p_swprintf_s(buf, 0, L"Hello");
    ok(ret == -1, "swprintf_s zero count returned %d, expected -1\n", ret);
}
