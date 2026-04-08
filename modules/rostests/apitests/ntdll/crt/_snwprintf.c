/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _snwprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__snwprintf)(wchar_t*, size_t, const wchar_t*, ...);

#define ok_snwprintf_(File, Line, ExpectedRet, ExpectedContent, Count, Format, ...) \
    do { \
        wchar_t buf[64]; \
        int ret; \
        memset(buf, 0xAA, sizeof(buf)); \
        ret = p_snwprintf(buf, Count, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "_snwprintf returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - sizeof(wchar_t)) == 0, "_snwprintf content mismatch\n"); \
    } while (0)

#define ok_snwprintf(ExpectedRet, ExpectedContent, Count, Format, ...) \
    ok_snwprintf_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, Count, Format, ##__VA_ARGS__)

START_TEST(_snwprintf)
{
    PFN__snwprintf p_snwprintf = (PFN__snwprintf)load_function("_snwprintf");

    if (!p_snwprintf)
        return;

    /* Basic: len < count - null terminated */
    ok_snwprintf(5, L"Hello\0\xAAAA", 64, L"Hello");

    /* Exact buffer: len < count (5 chars + null fits in 6) */
    ok_snwprintf(5, L"Hello\0\xAAAA", 6, L"Hello");

    /* len == count: no null terminator, returns len */
    ok_snwprintf(5, L"Hello\xAAAA", 5, L"Hello");

    /* len > count: truncated, no null, returns -1 */
    ok_snwprintf(-1, L"Hel\xAAAA", 3, L"Hello");

    /* len > count: only 1 char fits */
    ok_snwprintf(-1, L"H\xAAAA", 1, L"Hello");

    /* Empty format with large buffer */
    ok_snwprintf(0, L"\0\xAAAA", 64, L"");

    /* Count = 0 with non-empty format */
    ok_snwprintf(-1, L"\xAAAA", 0, L"Hello");

    /* Argument passing - multiple arguments */
    ok_snwprintf(4, L"x=42\0\xAAAA", 64, L"%s=%d", L"x", 42);

    /* Argument passing truncated */
    ok_snwprintf(-1, L"x=4\xAAAA", 3, L"%s=%d", L"x", 42);
}
