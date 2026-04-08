/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for sscanf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_sscanf_s)(const char*, const char*, ...);
PFN_sscanf_s p_sscanf_s;

#define p__stscanf p_sscanf_s
#define IS_SECURE_API
#include "scanf_formats.h"

#define ok_sscanf_(File, Line, ExpectedRet, ExpectedContent, Src, Format) \
    do { \
        char buf[17]; \
        memset(buf, 0xAA, sizeof(buf)); \
        buf[16] = 0; \
        int ret = p_sscanf_s(Src, Format, buf); \
        ok_(File, Line)(ret == ExpectedRet, "sscanf returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - 1) == 0, "sscanf content mismatch. Got \"%s\"\n", buf); \
    } while (0)

#define ok_sscanf(ExpectedRet, ExpectedContent, Src, Format) \
    ok_sscanf_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, Src, Format)


START_TEST(sscanf_s)
{
    p_sscanf_s = (PFN_sscanf_s)load_function("sscanf_s");
    if (!p_sscanf_s)
    {
        skip("sscanf_s is not available\n");
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
        p_sscanf_s(NULL, NULL);
    }
    __except(1)
    {
        excpt_code = _exception_code();
    }
    ok(excpt_code == 0xC0000005, "sscanf did not raise an exception when called with NULL parameters\n");

    excpt_code = 0;
    __try
    {
        ret = p_sscanf_s(NULL, "");
    }
    __except(1)
    {
        excpt_code = _exception_code();
    }
    ok(excpt_code == 0xC0000005, "sscanf did not raise an exception when called with NULL parameters\n");
#endif

    /* Basic int format */
    ok_eq_int(p_sscanf_s("123", "%d", &int_val), 1);
    ok_eq_int(int_val, 123);

    ok_eq_int(p_sscanf_s("123", "%ld", &int_val), 1);
    ok_eq_int(p_sscanf_s("1231234567890", "%lld", &int64_val), 1);
    ok_eq_int64(int64_val, 1231234567890ll);
    double dbl_val;
    ok_eq_int(p_sscanf_s("1.23", "%f", &dbl_val), 0);

    ok_eq_int(p_sscanf_s("0023", "%*d", 4, &int_val), 0);


    /* Incomplete specifier */
    ok_eq_int(p_sscanf_s("1", "%3%c", char_buf, 1), 0);
    ok_eq_int(p_sscanf_s("1", "%l%c", char_buf, 1), 0);
    ok_eq_int(p_sscanf_s("1", "%c%l", char_buf, 1), 1);

    /* Multiple specifiers */
    ok_eq_int(p_sscanf_s("0d23", "%l%d", &int_val), 0);

    /* %d without a number parsed */
    ok_eq_int(p_sscanf_s("", "%d", &int_val), -1);
    ok_eq_int(p_sscanf_s(" ", "%d", &int_val), -1);
    ok_eq_int(p_sscanf_s(" ab", "%d", &int_val), 0);
    ok_eq_int(p_sscanf_s(" ab", "%d", &int_val), 0);

    ok_eq_int(p_sscanf_s("123", "%d%d", &int_val, &int_val), 1);
    ok_eq_int(int_val, 123);

    /* Basic int format plus invalid format */
    ok_eq_int(p_sscanf_s("123", "%d%y", &int_val), 1);
    ok_eq_int(int_val, 123);
    ok_eq_int(p_sscanf_s("", "%d%y", &int_val), -1);
    ok_eq_int(int_val, 123);

    /* sscanf_s is severely broken and returns the low 16 bits of a stack variable instead of the character, so we can't test this */
#if 0
    wchar_t wbuf[4] = {0};
    ok_eq_int(p_sscanf_s("\x33\x34", "%C", wbuf, 1), 1);
    ok((wbuf[0] & 0xF) == 0, "Expected wbuf to be aligned to 16 (leaked stack address), got 0x%04x\n", wbuf[0]);
    ok_eq_int(p_sscanf_s("\x33\x34", "%wc", wbuf, 1), 1);
    ok((wbuf[0] & 0xF) == 0, "Expected wbuf to be aligned to 16 (leaked stack address), got 0x%04x\n", wbuf[0]);
    ok_eq_int(p_sscanf_s("\x33XY", "%S", wbuf, _countof(wbuf)), 1);
    ok((wbuf[0] & 0xF) == 0, "Expected wbuf to be aligned to 16 (leaked stack address), got 0x%04x\n", wbuf[0]);
    ok(wbuf[1] == wbuf[0], "Expected wbuf to be aligned to 16 (leaked stack address), got 0x%04x\n", wbuf[1]);
    ok(wbuf[2] == wbuf[0], "Expected wbuf to be aligned to 16 (leaked stack address), got 0x%04x\n", wbuf[2]);
#endif

    Test_scanf_format();

}
