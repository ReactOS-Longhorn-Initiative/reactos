/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _snprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <limits.h>

typedef int (__cdecl *PFN__snprintf)(char*, size_t, const char*, ...);

#define ok_snprintf_(File, Line, ExpectedRet, ExpectedContent, Count, Format, ...) \
    do { \
        char buf[64]; \
        int ret; \
        memset(buf, 0xAA, sizeof(buf)); \
        ret = p_snprintf(buf, Count, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "_snprintf returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - 1) == 0, "_snprintf content mismatch\n"); \
    } while (0)

#define ok_snprintf(ExpectedRet, ExpectedContent, Count, Format, ...) \
    ok_snprintf_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, Count, Format, ##__VA_ARGS__)

START_TEST(_snprintf)
{
    PFN__snprintf p_snprintf = (PFN__snprintf)load_function("_snprintf");
    if (!p_snprintf)
    {
        skip("_snprintf is not available\n");
        return;
    }

    /* Basic: len < count - null terminated */
    ok_snprintf(5, "Hello\0\xAA", 64, "Hello");

    /* Exact buffer: len < count (5 chars + null fits in 6) */
    ok_snprintf(5, "Hello\0\xAA", 6, "Hello");

    /* len == count: no null terminator, returns len */
    ok_snprintf(5, "Hello\xAA", 5, "Hello");

    /* len > count: truncated, no null, returns -1 */
    ok_snprintf(-1, "Hel\xAA", 3, "Hello");

    /* len > count: only 1 char fits */
    ok_snprintf(-1, "H\xAA", 1, "Hello");

    /* Count = 0 with non-empty format: no chars written, returns -1 */
    ok_snprintf(-1, "\xAA", 0, "Hello");

    /* Empty format with count = 1 */
    ok_snprintf(0, "\0\xAA", 1, "");

    /* Empty format with count = 0 */
    ok_snprintf(0, "\xAA", 0, "");

    /* NULL format string */
    ok_snprintf(-1, "\xAA", 64, NULL);

    /* Argument passing - multiple arguments */
    ok_snprintf(4, "x=42\0\xAA", 64, "%s=%d", "x", 42);

    /* Argument passing truncated */
    ok_snprintf(-1, "x=4\xAA", 3, "%s=%d", "x", 42);

    /* Truncated invalid format */
    ok_snprintf(-1, "t", 1, "%tp,%d");

    /* NULL buffer with non-zero count */
    int ret = p_snprintf(NULL, 10, "Test");
    ok_eq_int(ret, -1);

    /* NULL buffer with zero count */
    ret = p_snprintf(NULL, 0, "Test");
    ok_eq_int(ret, 4);

    /* Number precision is capped at 512 */
    ret = p_snprintf(NULL, 0, "%.*d", 1024, 1);
    ok_eq_int(ret, 512);
    ret = p_snprintf(NULL, 0, "%#.*o", 1024, 1);
    ok_eq_int(ret, 512);
    ret = p_snprintf(NULL, 0, "%#.*x", 1024, 1);
    ok_eq_int(ret, 514);

    /* String precision (i.e. max string length) is not capped */
    char buf[1024 + 1];
    memset(buf, 'X', sizeof(buf));
    buf[1024] = '\0';
    ret = p_snprintf(NULL, 0, "%.*s", 1024, buf);
    ok_eq_int(ret, 1024);

#if 0
    /* Try maximum sized format */
    //ret = p_snprintf(NULL, 0, "%*d", INT_MAX, 1);
    //ok_eq_int(ret, INT_MAX);

    /* If the format string causes an integer overflow, the result is INT_MIN */
    ret = p_snprintf(NULL, 0, "%*d~", INT_MAX, 1);
    ok_eq_int(ret, INT_MIN);
    ret = p_snprintf(NULL, 0, "%*d1234567890", INT_MAX, 1);
    ok_eq_int(ret, INT_MIN);

    /* If a format specifier causes the integer overflow the result is INT_MIN + number of chars - 1 */
    ret = p_snprintf(NULL, 0, "1%*d", INT_MAX, 1);
    ok_eq_int(ret, INT_MIN + 1 - 1);
    ret = p_snprintf(NULL, 0, "1234567890%*d", INT_MAX, 1);
    ok_eq_int(ret, INT_MIN + 10 - 1);


    ret = p_snprintf(NULL, 0, "%*d%*d", INT_MAX, 1, INT_MAX - 1, 1);
    ok_eq_int(ret, -3);

    ret = p_snprintf(NULL, 0, "%*d%*d", INT_MAX, 1, 1000, 1);
    ok_eq_int(ret, INT_MIN + 1000 - 1);

    ret = p_snprintf(NULL, 0, "%*d%*d --- Test 12345678 ---", INT_MAX, 1, 1000, 1);
    ok_eq_int(ret, INT_MIN + 1000 - 1);

    /* Try to exceed an int as return value */
    ret = p_snprintf(NULL, 0, "Test %*d Test", INT_MAX, 1);
    ok_eq_int(ret, INT_MIN + 4);
    ret = p_snprintf(NULL, 0, "%*d - 1234567890 - %*d%", INT_MAX - 1, 1, INT_MAX);
    ok_eq_int(ret, INT_MIN);
#endif
#if 0
    ret = p_snprintf(NULL, 0, "%*d123%%", INT_MAX, 1);
    ok_eq_int(ret, INT_MIN);

    /* Try to exceed an int as return value, so it wraps into positive again */
    //ret = p_snprintf(NULL, 0, "%*d%*d%*d", INT_MAX, 1, INT_MAX, 1, 123, 1);
    //ok_eq_int(ret, -2);

#endif

    /* Corner case: wrap around from INT_MAX to 0 in 1 field (INT_MAX + 2 + INT_MAX) */
    //ret = p_snprintf(NULL, 0, "%*d%.*d", INT_MAX, 1, INT_MAX, 1);
    //ok_eq_int(ret, -2);

}
