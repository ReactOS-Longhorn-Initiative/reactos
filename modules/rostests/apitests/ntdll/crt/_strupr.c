/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _strupr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <windows.h>

typedef char* (__cdecl *PFN__strupr)(char* _String);

static char expected_strupr(char ch)
{
    if (ch >= 'a' && ch <= 'z')
        return ch + ('A' - 'a');
    return ch;
}

START_TEST(_strupr)
{
    PFN__strupr p_strupr = (PFN__strupr)load_function("_strupr");
    char buffer[4];
    char *result;
    DWORD excpt_code;

    /* Test NULL parameter handling - _strupr calls _invalid_parameter which only debug-prints and then returns */
    excpt_code = 0;
    _SEH2_TRY
    {
        result = p_strupr(NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        excpt_code = GetExceptionCode();
        result = (void*)(ULONG_PTR)0xdeaddead;
    }
    _SEH2_END;
    ok(excpt_code == 0,
       "_strupr(NULL) exception code = 0x%lx, expected 0\n",
       excpt_code);
    ok(result == NULL, "_strupr(NULL) returned %p, expected NULL\n", result);

    /* Test a basic longer string with weird characters */
    char test_str[] = "Hello, World! 123 \t\n\x80\xAB\xFE~`@#$%^&*()";
    char expected_str[] = "HELLO, WORLD! 123 \t\n\x80\xAB\xFE~`@#$%^&*()";
    result = p_strupr(test_str);
    ok(result == test_str, "_strupr returned %p, expected %p\n", result, test_str);
    ok(strcmp(test_str, expected_str) == 0,
        "_strupr result = \"%s\", expected \"%s\"\n", test_str, expected_str);

    /* Test all chars from 0x01 to 0xFF individually */
    for (unsigned int c = 1; c <= 0xFF; c++)
    {
        char expected_ch = expected_strupr((char)c);
        buffer[0] = (char)c;
        buffer[1] = '\0';
        result = p_strupr(buffer);
        ok(result == buffer,
           "_strupr(0x%02x) returned %p, expected %p\n", c, result, buffer);
        ok(buffer[0] == expected_ch,
           "_strupr(0x%02x): result = 0x%02x, expected 0x%02x\n",
           c, (unsigned char)buffer[0], (unsigned char)expected_ch);
    }
}
