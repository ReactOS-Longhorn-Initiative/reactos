/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for swscanf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_swscanf_s)(const wchar_t*, const wchar_t*, ...);
PFN_swscanf_s p_swscanf_s;

#define p__stscanf p_swscanf_s
#define USE_WIDECHAR
#define IS_SECURE_API
#include "scanf_formats.h"

#define ok_sscanf_(File, Line, ExpectedRet, ExpectedContent, Src, Format) \
    do { \
        wchar_t buf[17]; \
        memset(buf, 0xAA, sizeof(buf)); \
        buf[16] = 0; \
        int ret = p_sscanf(Src, Format, buf); \
        ok_(File, Line)(ret == ExpectedRet, "sscanf returned %d, expected %d\n", ret, ExpectedRet); \
        ok_(File, Line)(memcmp(buf, ExpectedContent, sizeof(ExpectedContent) - 1) == 0, "sscanf content mismatch. Got \"%s\"\n", buf); \
    } while (0)

#define ok_sscanf(ExpectedRet, ExpectedContent, Src, Format) \
    ok_sscanf_(__FILE__, __LINE__, ExpectedRet, ExpectedContent, Src, Format)


START_TEST(swscanf_s)
{
    p_swscanf_s = (PFN_swscanf_s)load_function("swscanf_s");
    if (!p_swscanf_s)
    {
        skip("swscanf_s is not available\n");
        return;
    }

    int int_val;
    __int64 int64_val;
    wchar_t char_buf[64];

#if 0
    int ret;
    int excpt_code;
    excpt_code = 0;
    __try
    {
        p_swscanf_s(NULL, NULL);
    }
    __except(1)
    {
        excpt_code = _exception_code();
    }
    ok(excpt_code == 0xC0000005, "swscanf_s did not raise an exception when called with NULL parameters\n");

    excpt_code = 0;
    __try
    {
        ret = p_swscanf_s(NULL, L"");
    }
    __except(1)
    {
        excpt_code = _exception_code();
    }
    ok(excpt_code == 0xC0000005, "swscanf_s did not raise an exception when called with NULL parameters\n");
#endif

    /* Basic int format */
    ok_eq_int(p_swscanf_s(L"123", L"%d", &int_val), 1);
    ok_eq_int(int_val, 123);

    ok_eq_int(p_swscanf_s(L"123", L"%ld", &int_val), 1);
    ok_eq_int(p_swscanf_s(L"1231234567890", L"%lld", &int64_val), 1);
    ok_eq_int64(int64_val, 1231234567890ll);
    double dbl_val;
    ok_eq_int(p_swscanf_s(L"1.23", L"%f", &dbl_val), 0);

    ok_eq_int(p_swscanf_s(L"0023", L"%*d", 4, &int_val), 0);


    /* Incomplete specifier */
    ok_eq_int(p_swscanf_s(L"1", L"%3%c", char_buf), 0);
    ok_eq_int(p_swscanf_s(L"1", L"%l%c", char_buf), 0);
    #ifdef _WIN64
        ok_eq_int(p_swscanf_s(L"1", L"%c%l", char_buf), 0);
    #else
        ok_eq_int(p_swscanf_s(L"1", L"%c%l", char_buf), 1);
    #endif

    ok_eq_int(p_swscanf_s(L"0d23", L"%l%d", &int_val), 0);

    /* %d without a number parsedd */
    ok_eq_int(p_swscanf_s(L"", L"%d", &int_val), 0); // different to sscanf, which returns -1 here
    ok_eq_int(p_swscanf_s(L" ", L"%d", &int_val), 0); // different to sscanf, which returns -1 here
    ok_eq_int(p_swscanf_s(L" ab", L"%d", &int_val), 0);
    ok_eq_int(p_swscanf_s(L" ab", L"%d", &int_val), 0);

    /* Basic int format plus invalid format */
    ok_eq_int(p_swscanf_s(L"123", L"%d%y", &int_val), 1);
    ok_eq_int(int_val, 123);
    ok_eq_int(p_swscanf_s(L"", L"%d%y", &int_val), 0);
    ok_eq_int(int_val, 123);

    Test_scanf_format();



}
