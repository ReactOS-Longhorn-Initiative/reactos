/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _snscanf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__snscanf_s)(const char*, size_t, const char*, ...);

START_TEST(_snscanf_s)
{
    PFN__snscanf_s p_snscanf_s = (PFN__snscanf_s)load_function("_snscanf_s");
    int val;
    char str[32];
    int ret;

    if (!p_snscanf_s)
        return;

    /* Basic integer scan */
    val = 0xDEAD;
    ret = p_snscanf_s("42", 10, "%d", &val);
    ok(ret == 1, "_snscanf_s basic int returned %d, expected 1\n", ret);
    ok(val == 42, "_snscanf_s basic int val = %d, expected 42\n", val);

    /* Integer scan with count limiting */
    val = 0xDEAD;
    ret = p_snscanf_s("12345", 3, "%d", &val);
    ok(ret == 1, "_snscanf_s count=3 returned %d, expected 1\n", ret);
    ok(val == 123, "_snscanf_s count=3 val = %d, expected 123\n", val);

    /* Integer scan with count=1 */
    val = 0xDEAD;
    ret = p_snscanf_s("42", 1, "%d", &val);
    ok(ret == 1, "_snscanf_s count=1 returned %d, expected 1\n", ret);
    ok(val == 4, "_snscanf_s count=1 val = %d, expected 4\n", val);

    /* String scan with buffer size argument */
    memset(str, 0xAA, sizeof(str));
    ret = p_snscanf_s("hello", 10, "%s", str, (unsigned)sizeof(str));
    ok(ret == 1, "_snscanf_s string returned %d, expected 1\n", ret);
    ok(memcmp(str, "hello", 6) == 0, "_snscanf_s string content mismatch\n");

    /* Multiple fields */
    val = 0xDEAD;
    memset(str, 0xAA, sizeof(str));
    ret = p_snscanf_s("42 hello", 20, "%d %s", &val, str, (unsigned)sizeof(str));
    ok(ret == 2, "_snscanf_s multi returned %d, expected 2\n", ret);
    ok(val == 42, "_snscanf_s multi val = %d, expected 42\n", val);
    ok(memcmp(str, "hello", 6) == 0, "_snscanf_s multi str content mismatch\n");

    /* Count limits scan to partial integer */
    val = 0xDEAD;
    ret = p_snscanf_s("99bottles", 2, "%d", &val);
    ok(ret == 1, "_snscanf_s partial returned %d, expected 1\n", ret);
    ok(val == 99, "_snscanf_s partial val = %d, expected 99\n", val);

    /* Count limits string scan */
    memset(str, 0xAA, sizeof(str));
    ret = p_snscanf_s("hello", 3, "%s", str, (unsigned)sizeof(str));
    ok(ret == 1, "_snscanf_s str count=3 returned %d, expected 1\n", ret);
    ok(memcmp(str, "hel\0\xAA", 5) == 0, "_snscanf_s str count=3 content mismatch\n");

    /* Negative integer */
    val = 0xDEAD;
    ret = p_snscanf_s("-7", 10, "%d", &val);
    ok(ret == 1, "_snscanf_s negative returned %d, expected 1\n", ret);
    ok(val == -7, "_snscanf_s negative val = %d, expected -7\n", val);

    /* No matching input */
    val = 0xDEAD;
    ret = p_snscanf_s("abc", 10, "%d", &val);
    ok(ret == 0, "_snscanf_s no match returned %d, expected 0\n", ret);
}
