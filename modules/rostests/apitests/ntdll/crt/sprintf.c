/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for sprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_sprintf)(char*, const char*, ...);
PFN_sprintf p_sprintf;

#include "printf_formats.h"

#define ok_sprintf_(File, Line, ExpectedRet, ExpectedContent, Format, ...) \
    do { \
        char buf[64]; \
        int ret; \
        memset(buf, 0xAA, sizeof(buf)); \
        ret = p_sprintf(buf, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "sprintf returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - 1) == 0, "sprintf content mismatch. Got \"%s\"\n", buf); \
    } while (0)

#define ok_sprintf(ExpectedRet, ExpectedContent, Format, ...) \
    ok_sprintf_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, Format, ##__VA_ARGS__)

START_TEST(sprintf)
{
    p_sprintf = (PFN_sprintf)load_function("sprintf");
    if (!p_sprintf)
    {
        skip("sprintf is not available\n");
        return;
    }

    /* Basic string */
    ok_sprintf(5, "Hello\0\xAA", "Hello");

    /* Empty format string */
    ok_sprintf(0, "\0\xAA", "");

    /* NULL format string */
    ok_sprintf(-1, "\xAA", NULL);

    /* Argument passing - single string */
    ok_sprintf(5, "World\0\xAA", "%s", "World");

///
    /* Prefix parsing error handling */
    ok_sprintf(4, "I33d\0\xAA", "%hI33d", 123);
    //ok_sprintf(5, "World\0\xAA", "%hI33d", 123);
///

    /* Argument passing - single integer */
    ok_sprintf(2, "42\0\xAA", "%d", 42);

    /* Argument passing - multiple arguments */
    ok_sprintf(4, "x=42\0\xAA", "%s=%d", "x", 42);

    /* Longer literal text */
    ok_sprintf(10, "ABCDEFGHIJ\0\xAA", "ABCDEFGHIJ");

    /* Single character output */
    ok_sprintf(1, "A\0\xAA", "A");

    ok_sprintf(4, "%%A%\0\xAA", "%%%%A%%");
    ok_sprintf(4, "%%d%\0\xAA", "%%%%d%%");
    ok_sprintf(6, "112233\0\xAA", "1%d2%d3%d", 1, 2, 3);

    /* NULL buffer */
    int ret = p_sprintf(NULL, "Test");
    ok_eq_int(ret, -1);

    Test_printf_formats();
}
