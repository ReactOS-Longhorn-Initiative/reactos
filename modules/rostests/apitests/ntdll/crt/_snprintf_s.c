/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _snprintf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <stdio.h>

typedef int (__cdecl *PFN__snprintf_s)(char*, size_t, size_t, const char*, ...);

#define ok_snprintf_s_(File, Line, ExpectedRet, ExpectedContent, BufSize, MaxCount, Format, ...) \
    do { \
        char buf[64]; \
        int ret; \
        memset(buf, 0xAA, sizeof(buf)); \
        buf[63] = 0;\
        ret = p_snprintf_s(buf, BufSize, MaxCount, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "_snprintf_s returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - 1) == 0, "_snprintf_s content mismatch. Got \"%s\"\n", buf); \
    } while (0)

#define ok_snprintf_s(ExpectedRet, ExpectedContent, BufSize, MaxCount, Format, ...) \
    ok_snprintf_s_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, BufSize, MaxCount, Format, ##__VA_ARGS__)

START_TEST(_snprintf_s)
{
    int ret;

    PFN__snprintf_s p_snprintf_s = (PFN__snprintf_s)load_function("_snprintf_s");
    if (!p_snprintf_s)
    {
        skip("_snprintf_s is not available\n");
        return;
    }

    /* NULL buffer with zero size and count */
    ret = p_snprintf_s(NULL, 0, 0, "Hello");
    ok_eq_int(ret, 0);

    /* NULL buffer with non-zero size */
    ret = p_snprintf_s(NULL, 1, 0, "Hello");
    ok(ret == -1, "_snprintf_s NULL buf returned %d, expected 0\n", ret);

    /* NULL buffer with non-zero count */
    ret = p_snprintf_s(NULL, 0, 100, "Hello");
    ok_eq_int(ret, -1);

    /* NULL buffer with zero count and null format string */
    ret = p_snprintf_s(NULL, 0, 0, NULL);
    ok_eq_int(ret, -1);

    /* Buffer size 0 with empty format */
    ok_snprintf_s(-1, "\xAA", 0, 0, "");
    ok_snprintf_s(-1, "\xAA", 0, 63, "");

    /* Buffer size 0 with non-empty format */
    ok_snprintf_s(-1, "\xAA", 0, 0, "Hello");
    ok_snprintf_s(-1, "\xAA", 0, 63, "Hello");

    /* Buffer count 0 with empty format */
    ok_snprintf_s(0, "\0\xAA", 64, 0, "");

    /* Buffer count 0 with non-empty format */
    ok_snprintf_s(-1, "\0\xAA", 64, 0, "Hello");

    /* Buffer size 1 with empty format */
    ok_snprintf_s(0, "\0\xAA", 1, 63, "");

    /* Buffer size 1 with non-empty format */
    ok_snprintf_s(-1, "\0\xAA", 1, 63, "Hello");

    /* NULL format string */
    ok_snprintf_s(-1, "\xAA", 64, 63, NULL);

    /* Empty format string */
    ok_snprintf_s(0, "\0\xAA", 63, 63, "");

    /* Basic with large buffer */
    ok_snprintf_s(5, "Hello\0\xAA", 64, 63, "Hello");

    /* Exact buffer size (5 chars + null = 6) */
    ok_snprintf_s(5, "Hello\0\xAA", 6, 63, "Hello");

    /* Buffer too small */
    ok_snprintf_s(-1, "\0ell\0\xAA", 5, 63, "Hello");

    /* _TRUNCATE with sufficient buffer */
    ok_snprintf_s(5, "Hello\0\xAA", 64, _TRUNCATE, "Hello");

    /* _TRUNCATE with small buffer - truncates and null-terminates */
    ok_snprintf_s(-1, "Hel\0\xAA", 4, _TRUNCATE, "Hello");

    /*  */
    ok_snprintf_s(-1, "\0el\0\xAA", 4, _TRUNCATE - 1, "Hello");
#ifdef _WIN64
    ok_snprintf_s(-1, "\0el\0\xAA", 4, 0xFFFFFFFF, "Hello");
#endif

    /* _TRUNCATE with buffer size 1 - writes only null */
    ok_snprintf_s(-1, "\0\xAA", 1, _TRUNCATE, "Hello");

    /* _TRUNCATE with buffer size 2 - writes 1 char + null */
    ok_snprintf_s(-1, "H\0\xAA", 2, _TRUNCATE, "Hello");

    /* maxCount exceeded without _TRUNCATE: truncates to maxCount */
    ok_snprintf_s(-1, "Hel\0\xAA", 64, 3, "Hello");
    ok_snprintf_s(-1, "Hel\0\xAA", 4, 3, "Hello");
    ok_snprintf_s(-1, "\0e\0\xAA", 3, 4, "Hello");
    ok_snprintf_s(-1, "\0e\0\xAA", 3, 3, "Hello");

    /* Argument passing */
    ok_snprintf_s(4, "x=42\0\xAA", 64, 63, "%s=%d", "x", 42);

    /* Argument passing with _TRUNCATE and small buffer */
    ok_snprintf_s(-1, "x=\0\xAA", 3, _TRUNCATE, "%s=%d", "x", 42);
}
