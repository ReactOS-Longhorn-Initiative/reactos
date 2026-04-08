/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for sprintf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <limits.h>

typedef int (__cdecl *PFN_sprintf_s)(char*, size_t, const char*, ...);

#define ok_sprintf_s_(File, Line, ExpectedRet, ExpectedContent, BufSize, Format, ...) \
    do { \
        char buf[64]; \
        int ret; \
        memset(buf, 0xAA, sizeof(buf)); \
        buf[63] = 0;\
        ret = p_sprintf_s(buf, BufSize, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "sprintf_s returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - 1) == 0, "sprintf_s content mismatch. Got \"%s\"\n", buf); \
    } while (0)

#define ok_sprintf_s(ExpectedRet, ExpectedContent, BufSize, Format, ...) \
    ok_sprintf_s_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, BufSize, Format, ##__VA_ARGS__)

START_TEST(sprintf_s)
{
    int ret;

    PFN_sprintf_s p_sprintf_s = (PFN_sprintf_s)load_function("sprintf_s");
    if (!p_sprintf_s)
    {
        skip("sprintf_s is not available\n");
        return;
    }

    /* NULL buffer with zero count */
    ret = p_sprintf_s(NULL, 0, "test");
    ok_eq_int(ret, -1);

    /* NULL buffer with non-zero count */
    ret = p_sprintf_s(NULL, 10, "test");
    ok_eq_int(ret, -1);

    /* NULL buffer with zero count and null format string */
    ret = p_sprintf_s(NULL, 0, NULL);
    ok_eq_int(ret, -1);

    /* Buffer size 0 with empty format */
    ok_sprintf_s(-1, "\xAA", 0, "");

    /* Buffer size 0 with non-empty format */
    ok_sprintf_s(-1, "\xAA", 0, "Hello");

    /* Buffer size 1 with empty format */
    ok_sprintf_s(0, "\0\xAA", 1, "");

    /* Buffer size 1 with non-empty format */
    ok_sprintf_s(-1, "\0\xAA", 1, "Hello");

    /* NULL format string */
    ok_sprintf_s(-1, "\xAA", 63, NULL);

    /* Empty format string */
    ok_sprintf_s(0, "\0\xAA", 63, "");

    /* Basic with large buffer */
    ok_sprintf_s(5, "Hello\0\xAA", 63, "Hello");

    /* Exact buffer size (5 chars + null = 6) */
    ok_sprintf_s(5, "Hello\0\xAA", 6, "Hello");

    /* Buffer too small */
    ok_sprintf_s(-1, "\0ell\0\xAA", 5, "Hello");



}
