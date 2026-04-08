/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for sscanf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_sscanf)(const char*, const char*, ...);
PFN_sscanf p_sscanf;

#define p__stscanf p_sscanf
#include "scanf_formats.h"

#define ok_sscanf_(File, Line, ExpectedRet, ExpectedContent, Src, Format) \
    do { \
        char buf[17]; \
        memset(buf, 0xAA, sizeof(buf)); \
        buf[16] = 0; \
        int ret = p_sscanf(Src, Format, buf); \
        ok_(File, Line)(ret == ExpectedRet, "sscanf returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - 1) == 0, "sscanf content mismatch. Got \"%s\"\n", buf); \
    } while (0)

#define ok_sscanf(ExpectedRet, ExpectedContent, Src, Format) \
    ok_sscanf_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, Src, Format)


START_TEST(sscanf)
{
    p_sscanf = (PFN_sscanf)load_function("sscanf");
    if (!p_sscanf)
    {
        skip("sscanf is not available\n");
        return;
    }

    int int_val;
    __int64 int64_val;
    char char_buf[64];

#if 0
    int ret;
    int excpt_code;
    excpt_code = 0;
    __try
    {
        p_sscanf(NULL, NULL);
    }
    __except(1)
    {
        excpt_code = _exception_code();
    }
    ok(excpt_code == 0xC0000005, "sscanf did not raise an exception when called with NULL parameters\n");

    excpt_code = 0;
    __try
    {
        ret = p_sscanf(NULL, "");
    }
    __except(1)
    {
        excpt_code = _exception_code();
    }
    ok(excpt_code == 0xC0000005, "sscanf did not raise an exception when called with NULL parameters\n");
#endif

    /* Basic int format */
    ok_eq_int(p_sscanf("123", "%d", &int_val), 1);
    ok_eq_int(int_val, 123);

    ok_eq_int(p_sscanf("123", "%ld", &int_val), 1);
    ok_eq_int(p_sscanf("1231234567890", "%lld", &int64_val), 1);
    ok_eq_int64(int64_val, 1231234567890ll);
    double dbl_val;
    ok_eq_int(p_sscanf("1.23", "%f", &dbl_val), 0);

    ok_eq_int(p_sscanf("0023", "%*d", 4, &int_val), 0);


    /* Incomplete specifier */
    ok_eq_int(p_sscanf("1", "%3%c", char_buf), 0);
    ok_eq_int(p_sscanf("1", "%l%c", char_buf), 0);
    ok_eq_int(p_sscanf("1", "%c%l", char_buf), 1);

    /* Multiple specifiers */
    ok_eq_int(p_sscanf("0d23", "%l%d", &int_val), 0);

    /* %d without a number parsed */
    ok_eq_int(p_sscanf("", "%d", &int_val), -1);
    ok_eq_int(p_sscanf(" ", "%d", &int_val), -1);
    ok_eq_int(p_sscanf(" ab", "%d", &int_val), 0);
    ok_eq_int(p_sscanf(" ab", "%d", &int_val), 0);

    ok_eq_int(p_sscanf("123", "%d%d", &int_val, &int_val), 1);
    ok_eq_int(int_val, 123);

    /* Basic int format plus invalid format */
    ok_eq_int(p_sscanf("123", "%d%y", &int_val), 1);
    ok_eq_int(int_val, 123);
    ok_eq_int(p_sscanf("", "%d%y", &int_val), -1);
    ok_eq_int(int_val, 123);

    Test_scanf_format();

}
