/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared test table for string-to-integer conversion functions
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 *
 * Define USE_WIDECHAR before including for wchar_t version.
 */

#include <stdlib.h>
#include "strtox_table.h"

#ifdef USE_WIDECHAR
  #define _T(x) L##x
  #define TABLE_NAME      g_wcstox_table
  #define TABLE_LEN_NAME  g_wcstox_table_length
  #define ENTRY_TYPE      wcstox_entry
#else
  #define _T(x) x
  #define TABLE_NAME      g_strtox_table
  #define TABLE_LEN_NAME  g_strtox_table_length
  #define ENTRY_TYPE      strtox_entry
#endif

#define E(...) { __LINE__, __VA_ARGS__ }

const ENTRY_TYPE TABLE_NAME[] =
{
    /* rdx string                         value                        endptr  flags */

    /* Basic decimal */
    E( 10, _T("0"),                       0,                           1,      0 ),
    E( 10, _T("1"),                       1,                           1,      0 ),
    E( 10, _T("-1"),                      -1,                          2,      F_NEG ),
    E( 10, _T("123"),                     123,                         3,      0 ),
    E( 10, _T("-456"),                    -456,                        4,      F_NEG ),
    E( 10, _T("+789"),                    789,                         4,      0 ),
    E( 10, _T("+0"),                      0,                           2,      0 ),
    E( 10, _T("-0"),                      -0,                          2,      F_NEG ),
    E( 10, _T("0099"),                    99,                          4,      0 ),
    E( 10, _T("077"),                     77,                          3,      0 ),

    /* Leading whitespace */
    E( 10, _T("  \t\n 42"),               42,                          7,      0 ),
    E( 10, _T(" +42"),                    42,                          4,      0 ),
    E( 10, _T(" -42"),                    -42,                         4,      F_NEG ),

    /* Trailing non-digit characters */
    E( 10, _T("123abc"),                  123,                         3,      0 ),

    /* No valid digits */
    E( 10, _T(""),                        0,                           0,      F_NODIGITS ),
    E( 10, _T("abc"),                     0,                           0,      F_NODIGITS ),
    E( 10, _T("   "),                     0,                           0,      F_NODIGITS ),
    E( 10, _T("+"),                       0,                           0,      F_NODIGITS ),
    E( 10, _T("-"),                       0,                           0,      F_NODIGITS ),

    /* Hex and octal with base 10 */
    E( 10, _T("0x1A"),                    0,                           1,      0 ),
    E( 10, _T("077"),                     77,                          3,      0 ),

    /* Signed 32-bit boundaries */
    E( 10, _T("2147483647"),              2147483647LL,                10,     0 ),
    E( 10, _T("2147483648"),              2147483648LL,                10,     F_OVF_S32 ),
    E( 10, _T("-2147483647"),             -2147483647LL,               11,     F_NEG ),
    E( 10, _T("-2147483648"),             -2147483648LL,               11,     F_NEG ),
    E( 10, _T("-2147483649"),             -2147483649LL,               11,     F_NEG | F_OVF_S32 ),
    E( 10, _T("-9999999999"),             -9999999999,                 11,     F_NEG | F_OVF_S32 | F_OVF_U32),

    /* Unsigned 32-bit boundaries */
    E( 10, _T("4294967295"),              4294967295LL,                10,     F_OVF_S32 ),
    E( 10, _T("4294967296"),              4294967296LL,                10,     F_OVF_S32 | F_OVF_U32 ),
    E( 10, _T("3000000000"),              3000000000LL,                10,     F_OVF_S32 ),

    /* Signed 64-bit boundaries */
    E( 10, _T("9223372036854775807"),     9223372036854775807LL,       19,     F_OVF_S32 | F_OVF_U32 ),
    E( 10, _T("9223372036854775808"),     9223372036854775808ULL,      19,     F_OVF_S32 | F_OVF_U32 | F_OVF_S64 ),
    E( 10, _T("-9223372036854775807"),    -9223372036854775807LL,      20,     F_NEG | F_OVF_S32 | F_OVF_U32 ),
    E( 10, _T("-9223372036854775808"),    -9223372036854775808LL,      20,     F_NEG | F_OVF_S32 | F_OVF_U32 ),
    E( 10, _T("-9223372036854775809"),    9223372036854775807ULL,      20,     F_NEG | F_OVF_S32 | F_OVF_U32 | F_OVF_S64 ),

    /* Unsigned 64-bit boundaries */
    E( 10, _T("18446744073709551615"),    18446744073709551615ULL,     20,     F_OVF_S32 | F_OVF_U32 | F_OVF_S64 ),
    E( 10, _T("18446744073709551616"),    0,                           20,     F_OVF_S32 | F_OVF_U32 | F_OVF_S64 | F_OVF_U64 ),
    E( 10, _T("99999999999999999999999"), -159383553,                  23,     F_OVF_S32 | F_OVF_U32 | F_OVF_S64 | F_OVF_U64 ),

    /* Negative values for unsigned wrapping tests */
    E( 10, _T("-4294967295"),             -4294967295LL,               11,     F_NEG | F_OVF_S32 ),
    E( 10, _T("-4294967296"),             -4294967296LL,               11,     F_NEG | F_OVF_S32 | F_OVF_U32 ),

    /* Hex with explicit radix 16 */
    E( 16, _T("ff"),                      255,                         2,      0 ),
    E( 16, _T("FF"),                      255,                         2,      0 ),
    E( 16, _T("0xFF"),                    255,                         4,      0 ),
    E( 16, _T("0XFF"),                    255,                         4,      0 ),
    E( 16, _T("-0xff"),                   -255,                        5,      F_NEG ),
    E( 16, _T("7FFFFFFF"),                0x7FFFFFFFLL,                8,      0 ),
    E( 16, _T("80000000"),                0x80000000LL,                8,      F_OVF_S32 ),
    E( 16, _T("FFFFFFFF"),                0xFFFFFFFFLL,                8,      F_OVF_S32 ),
    E( 16, _T("100000000"),               0x100000000LL,               9,      F_OVF_S32 | F_OVF_U32 ),
    E( 16, _T("DEADBEEF"),                0xDEADBEEFLL,                8,      F_OVF_S32 ),
    E( 16, _T("0xZZ"),                    0,                           0,      F_NODIGITS ),

    /* Octal with explicit radix 8 */
    E( 8,  _T("077"),                     63,                          3,      0 ),
    E( 8,  _T("0377"),                    255,                         4,      0 ),

    /* Auto-detect radix (radix = 0) */
    E( 0,  _T("123"),                     123,                         3,      0 ),
    E( 0,  _T("0x1A"),                    26,                          4,      0 ),
    E( 0,  _T("0X1A"),                    26,                          4,      0 ),
    E( 0,  _T("-0x1A"),                   -26,                         5,      F_NEG ),
    E( 0,  _T("077"),                     63,                          3,      0 ),
    E( 0,  _T("-077"),                    -63,                         4,      F_NEG ),
    E( 0,  _T("0"),                       0,                           1,      0 ),
    E( 0,  _T("00"),                      0,                           2,      0 ),
    E( 0,  _T("0x"),                      0,                           0,      F_NODIGITS ),

    /* Binary (radix 2) */
    E( 2,  _T("1010"),                    10,                          4,      0 ),
    E( 2,  _T("11111111"),                255,                         8,      0 ),

    /* Base 36 */
    E( 36, _T("z"),                       35,                          1,      0 ),
    E( 36, _T("Z"),                       35,                          1,      0 ),
    E( 36, _T("10"),                      36,                          2,      0 ),

    /* Invalid radix */
    E( 1, _T("10"),                       0,                           0,      0 ),
    E( 37, _T("10"),                      0,                           0,      0 ),
    E( -1, _T("10"),                      0,                           0,      0 ),
    E( -10, _T("10"),                     0,                           0,      0 ),

#ifdef USE_WIDECHAR
    /* ntdll's wide functions recognize non-western Unicode digits.
       Each entry uses the digit sequence "1234567890" in the respective script. */
    E( 10, L"\x0661\x0662\x0663\x0664\x0665\x0666\x0667\x0668\x0669\x0660", 1234567890LL, 10, 0 ),   /* Arabic-Indic */
    E( 10, L"\x06F1\x06F2\x06F3\x06F4\x06F5\x06F6\x06F7\x06F8\x06F9\x06F0", 1234567890LL, 10, 0 ),   /* Extended Arabic-Indic */
    E( 10, L"\x0967\x0968\x0969\x096A\x096B\x096C\x096D\x096E\x096F\x0966", 1234567890LL, 10, 0 ),   /* Devanagari */
    E( 10, L"\x09E7\x09E8\x09E9\x09EA\x09EB\x09EC\x09ED\x09EE\x09EF\x09E6", 1234567890LL, 10, 0 ),   /* Bengali */
    E( 10, L"\x0A67\x0A68\x0A69\x0A6A\x0A6B\x0A6C\x0A6D\x0A6E\x0A6F\x0A66", 1234567890LL, 10, 0 ),   /* Gurmukhi */
    E( 10, L"\x0AE7\x0AE8\x0AE9\x0AEA\x0AEB\x0AEC\x0AED\x0AEE\x0AEF\x0AE6", 1234567890LL, 10, 0 ),   /* Gujarati */
    E( 10, L"\x0B67\x0B68\x0B69\x0B6A\x0B6B\x0B6C\x0B6D\x0B6E\x0B6F\x0B66", 1234567890LL, 10, 0 ),   /* Oriya */
    E( 10, L"\x0C67\x0C68\x0C69\x0C6A\x0C6B\x0C6C\x0C6D\x0C6E\x0C6F\x0C66", 1234567890LL, 10, 0 ),   /* Telugu */
    E( 10, L"\x0CE7\x0CE8\x0CE9\x0CEA\x0CEB\x0CEC\x0CED\x0CEE\x0CEF\x0CE6", 1234567890LL, 10, 0 ),   /* Kannada */
    E( 10, L"\x0D67\x0D68\x0D69\x0D6A\x0D6B\x0D6C\x0D6D\x0D6E\x0D6F\x0D66", 1234567890LL, 10, 0 ),   /* Malayalam */
    E( 10, L"\x0E51\x0E52\x0E53\x0E54\x0E55\x0E56\x0E57\x0E58\x0E59\x0E50", 1234567890LL, 10, 0 ),   /* Thai */
    E( 10, L"\x0ED1\x0ED2\x0ED3\x0ED4\x0ED5\x0ED6\x0ED7\x0ED8\x0ED9\x0ED0", 1234567890LL, 10, 0 ),   /* Lao */
    E( 10, L"\x0F21\x0F22\x0F23\x0F24\x0F25\x0F26\x0F27\x0F28\x0F29\x0F20", 1234567890LL, 10, 0 ),   /* Tibetan */
    E( 10, L"\x1041\x1042\x1043\x1044\x1045\x1046\x1047\x1048\x1049\x1040", 1234567890LL, 10, 0 ),   /* Myanmar */
    E( 10, L"\x17E1\x17E2\x17E3\x17E4\x17E5\x17E6\x17E7\x17E8\x17E9\x17E0", 1234567890LL, 10, 0 ),   /* Khmer */
    E( 10, L"\x1811\x1812\x1813\x1814\x1815\x1816\x1817\x1818\x1819\x1810", 1234567890LL, 10, 0 ),   /* Mongolian */
    E( 10, L"\xFF11\xFF12\xFF13\xFF14\xFF15\xFF16\xFF17\xFF18\xFF19\xFF10", 1234567890LL, 10, 0 ),   /* Fullwidth */

    /* Mixed digits from different scripts (Thai 1, Mongolian 2, ASCII 3) */
    E( 10, L"\x0E51\x1812" L"3",         123,                         3,      0 ),

    /* Non-western digits in octal (Thai "01234567" = octal 01234567 = decimal 342391) */
    E( 8,  L"\x0E50\x0E51\x0E52\x0E53\x0E54\x0E55\x0E56\x0E57", 342391LL, 8, 0 ),
    E( 0,  L"\x0E50\x0E51\x0E52\x0E53\x0E54\x0E55\x0E56\x0E57", 342391LL, 8, 0 ),

    /* Non-western digits in hex (Fullwidth "1a" with ASCII hex letter) */
    E( 16, L"\xFF11" L"a",                0x1A,                          2,      0 ),
    E( 0,  L"\x1810X\xFF11" L"a",         0x1A,                          4,      0 ),

    /* Non-western digits stop at 8/9 in octal */
    E( 8,  L"\x0E51\x0E52\x0E58",        10,                          2,      0 ),   /* Thai "128" in octal: stops at 8 */

    /* Mixed ASCII + Mongolian zero */
    E( 10, L"12\x1810" L"34",             12034,                       5,      0 ),
#endif
};

const size_t TABLE_LEN_NAME = sizeof(TABLE_NAME) / sizeof(TABLE_NAME[0]);
