/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wcsupr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <windows.h>

typedef wchar_t* (__cdecl *PFN__wcsupr)(wchar_t* _String);

static wchar_t expected_wcsupr(wchar_t ch)
{
    if (ch >= 'a' && ch <= 'z')
        return ch + ('A' - 'a');
    return ch;
}

START_TEST(_wcsupr)
{
    PFN__wcsupr p_wcsupr = (PFN__wcsupr)load_function("_wcsupr");
    wchar_t buffer[4];
    wchar_t *result;
    DWORD excpt_code;

    /* Test NULL parameter handling - _wcsupr has no NULL check, expect STATUS_ACCESS_VIOLATION */
    excpt_code = 0;
    _SEH2_TRY
    {
        result = p_wcsupr(NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        excpt_code = GetExceptionCode();
        result = (void*)(ULONG_PTR)0xdeaddead;
    }
    _SEH2_END;
    ok(excpt_code == STATUS_ACCESS_VIOLATION,
       "_wcsupr(NULL) exception code = 0x%lx, expected STATUS_ACCESS_VIOLATIONx\n",
       excpt_code);

    /* Test a basic longer string with weird characters */
    wchar_t test_str[] = L"Hello, World! 123 \t\n\x80\xAB\xFE~`@#$%^&*()";
    const wchar_t expected_str[] = L"HELLO, WORLD! 123 \t\n\x80\xAB\xFE~`@#$%^&*()";
    result = p_wcsupr(test_str);
    ok(result == test_str, "_wcsupr returned %p, expected %p\n", result, test_str);
    ok(wcscmp(test_str, expected_str) == 0,
        "_wcsupr result = \"%S\", expected \"%S\"\n", test_str, expected_str);

    /* Test all chars from 0x01 to 0xFF individually */
    for (unsigned int c = 1; c <= 0xFFFF; c++)
    {
        wchar_t expected_ch = expected_wcsupr((wchar_t)c);
        buffer[0] = (wchar_t)c;
        buffer[1] = '\0';
        result = p_wcsupr(buffer);
        ok(result == buffer,
           "_wcsupr(0x%02x) returned %p, expected %p\n", c, result, buffer);
        ok(buffer[0] == expected_ch,
           "_wcsupr(0x%02x): result = 0x%02x, expected 0x%02x\n",
           c, (unsigned char)buffer[0], (unsigned char)expected_ch);
    }
}
