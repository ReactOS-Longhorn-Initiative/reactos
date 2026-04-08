/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _swprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__swprintf)(wchar_t*, const wchar_t*, ...);
PFN__swprintf p__swprintf;

#define USE_WIDECHAR
#include "printf_formats.h"

#define ok_swprintf_(File, Line, ExpectedRet, ExpectedContent, Format, ...) \
    do { \
        wchar_t buf[64]; \
        int ret; \
        memset(buf, 0xAA, sizeof(buf)); \
        ret = p__swprintf(buf, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "_swprintf returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - sizeof(wchar_t)) == 0, "_swprintf content mismatch\n"); \
    } while (0)

#define ok_swprintf(ExpectedRet, ExpectedContent, Format, ...) \
    ok_swprintf_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, Format, ##__VA_ARGS__)

START_TEST(_swprintf)
{
    p__swprintf = (PFN__swprintf)load_function("_swprintf");
    if (!p__swprintf)
        return;

    /* Basic string */
    ok_swprintf(5, L"Hello\0\xAAAA", L"Hello");

    /* Empty format string */
    ok_swprintf(0, L"\0\xAAAA", L"");

    /* Argument passing - single wide string */
    ok_swprintf(5, L"World\0\xAAAA", L"%s", L"World");

    /* Argument passing - single integer */
    ok_swprintf(2, L"42\0\xAAAA", L"%d", 42);

    /* Argument passing - multiple arguments */
    ok_swprintf(4, L"x=42\0\xAAAA", L"%s=%d", L"x", 42);

    /* Longer literal text */
    ok_swprintf(10, L"ABCDEFGHIJ\0\xAAAA", L"ABCDEFGHIJ");

    /* Single character output */
    ok_swprintf(1, L"A\0\xAAAA", L"A");

    Test_printf_formats();
}
