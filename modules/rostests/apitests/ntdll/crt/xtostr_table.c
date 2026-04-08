/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared test table for integer-to-string conversion functions
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 *
 * Define USE_WIDECHAR before including for wchar_t version.
 */

#include <stdlib.h>
#include "xtostr_table.h"

#ifdef USE_WIDECHAR
  #define _T(x) L##x
  #define TABLE_NAME      g_xtow_table
  #define TABLE_LEN_NAME  g_xtow_table_length
  #define ENTRY_TYPE      xtow_entry
#else
  #define _T(x) x
  #define TABLE_NAME      g_xtoa_table
  #define TABLE_LEN_NAME  g_xtoa_table_length
  #define ENTRY_TYPE      xtoa_entry
#endif

#define E(...) { __LINE__, __VA_ARGS__ }

const ENTRY_TYPE TABLE_NAME[] =
{
    /* rdx value                             expected                               flags */

    /* ===== Zero ===== */
    E( 10, 0,                                _T("0"),                               XF_ALL ),
    E( 8,  0,                                _T("0"),                               XF_ALL ),
    E( 16, 0,                                _T("0"),                               XF_ALL ),
    E( 2,  0,                                _T("0"),                               XF_ALL ),
    E( 36, 0,                                _T("0"),                               XF_ALL ),

    /* ===== Small positive values (all functions) ===== */
    E( 10, 1,                                _T("1"),                               XF_ALL ),
    E( 10, 9,                                _T("9"),                               XF_ALL ),
    E( 10, 10,                               _T("10"),                              XF_ALL ),
    E( 10, 99,                               _T("99"),                              XF_ALL ),
    E( 10, 100,                              _T("100"),                             XF_ALL ),
    E( 10, 123,                              _T("123"),                             XF_ALL ),
    E( 10, 255,                              _T("255"),                             XF_ALL ),
    E( 10, 1000,                             _T("1000"),                            XF_ALL ),

    /* ===== Hex (positive values, all functions) ===== */
    E( 16, 0,                                _T("0"),                               XF_ALL ),
    E( 16, 1,                                _T("1"),                               XF_ALL ),
    E( 16, 10,                               _T("a"),                               XF_ALL ),
    E( 16, 15,                               _T("f"),                               XF_ALL ),
    E( 16, 16,                               _T("10"),                              XF_ALL ),
    E( 16, 255,                              _T("ff"),                              XF_ALL ),
    E( 16, 256,                              _T("100"),                             XF_ALL ),
    E( 16, 0xDEAD,                           _T("dead"),                            XF_ALL ),
    E( 16, 65535,                            _T("ffff"),                            XF_ALL ),

    /* ===== Octal (positive values, all functions) ===== */
    E( 8,  1,                                _T("1"),                               XF_ALL ),
    E( 8,  7,                                _T("7"),                               XF_ALL ),
    E( 8,  8,                                _T("10"),                              XF_ALL ),
    E( 8,  63,                               _T("77"),                              XF_ALL ),
    E( 8,  64,                               _T("100"),                             XF_ALL ),
    E( 8,  255,                              _T("377"),                             XF_ALL ),

    /* ===== Binary ===== */
    E( 2,  1,                                _T("1"),                               XF_ALL ),
    E( 2,  2,                                _T("10"),                              XF_ALL ),
    E( 2,  10,                               _T("1010"),                            XF_ALL ),
    E( 2,  255,                              _T("11111111"),                         XF_ALL ),

    /* ===== Base 36 ===== */
    E( 36, 1,                                _T("1"),                               XF_ALL ),
    E( 36, 10,                               _T("a"),                               XF_ALL ),
    E( 36, 35,                               _T("z"),                               XF_ALL ),
    E( 36, 36,                               _T("10"),                              XF_ALL ),
    E( 36, 255,                              _T("73"),                              XF_ALL ),

    /* ===== Signed 32-bit negative values, decimal (only _itoa, _i64toa) ===== */
    E( 10, (unsigned __int64)(__int64)-1,    _T("-1"),                              XF_I32 | XF_I64 ),
    E( 10, (unsigned __int64)(__int64)-9,    _T("-9"),                              XF_I32 | XF_I64 ),
    E( 10, (unsigned __int64)(__int64)-10,   _T("-10"),                             XF_I32 | XF_I64 ),
    E( 10, (unsigned __int64)(__int64)-123,  _T("-123"),                            XF_I32 | XF_I64 ),
    E( 10, (unsigned __int64)(__int64)-456,  _T("-456"),                            XF_I32 | XF_I64 ),
    E( 10, (unsigned __int64)(__int64)-1000, _T("-1000"),                           XF_I32 | XF_I64 ),

    /* ===== Signed 32-bit negative values, hex (treated as unsigned bits) ===== */
    /* _itoa(-1, buf, 16) -> "ffffffff", _i64toa(-1, buf, 16) -> "ffffffffffffffff" */
    /* These need separate entries for 32-bit vs 64-bit signed */

    /* 32-bit signed negative in hex/octal/binary (bit pattern = unsigned 32-bit) */
    E( 16, (unsigned __int64)(unsigned int)-1,    _T("ffffffff"),                   XF_I32 ),
    E( 16, (unsigned __int64)(unsigned int)-256,  _T("ffffff00"),                   XF_I32 ),
    E( 8,  (unsigned __int64)(unsigned int)-1,    _T("37777777777"),                XF_I32 ),
    E( 2,  (unsigned __int64)(unsigned int)-1,    _T("11111111111111111111111111111111"), XF_I32 ),

    /* 64-bit signed negative in hex/octal/binary (bit pattern = unsigned 64-bit) */
    E( 16, (unsigned __int64)(__int64)-1,         _T("ffffffffffffffff"),           XF_I64 ),
    E( 16, (unsigned __int64)(__int64)-256,       _T("ffffffffffffff00"),           XF_I64 ),
    E( 8,  (unsigned __int64)(__int64)-1,         _T("1777777777777777777777"),     XF_I64 ),

    /* ===== Signed 32-bit boundary values ===== */
    E( 10, 2147483647ULL,                    _T("2147483647"),                      XF_I32 | XF_U32 | XF_I64 | XF_U64 ),
    E( 16, 0x7FFFFFFFULL,                    _T("7fffffff"),                        XF_I32 | XF_U32 | XF_I64 | XF_U64 ),
    E( 10, (unsigned __int64)(__int64)-2147483647, _T("-2147483647"),               XF_I32 | XF_I64 ),
    E( 10, (unsigned __int64)(__int64)(-2147483647 - 1), _T("-2147483648"),         XF_I32 | XF_I64 ),

    /* ===== Unsigned 32-bit boundary values ===== */
    E( 10, 2147483648ULL,                    _T("2147483648"),                      XF_U32 | XF_I64 | XF_U64 ),
    E( 10, 4294967295ULL,                    _T("4294967295"),                      XF_U32 | XF_I64 | XF_U64 ),
    E( 16, 0x80000000ULL,                    _T("80000000"),                        XF_U32 | XF_I64 | XF_U64 ),
    E( 16, 0xFFFFFFFFULL,                    _T("ffffffff"),                        XF_U32 | XF_I64 | XF_U64 ),
    E( 16, 0xDEADBEEFULL,                   _T("deadbeef"),                        XF_U32 | XF_I64 | XF_U64 ),

    /* ===== Signed 64-bit boundary values ===== */
    E( 10, 9223372036854775807ULL,           _T("9223372036854775807"),             XF_I64 | XF_U64 ),
    E( 16, 0x7FFFFFFFFFFFFFFFULL,            _T("7fffffffffffffff"),               XF_I64 | XF_U64 ),
    E( 10, (unsigned __int64)(-9223372036854775807LL), _T("-9223372036854775807"),  XF_I64 ),
    E( 10, (unsigned __int64)(-9223372036854775807LL - 1), _T("-9223372036854775808"), XF_I64 ),

    /* ===== Unsigned 64-bit boundary values ===== */
    E( 10, 9223372036854775808ULL,           _T("9223372036854775808"),             XF_U64 ),
    E( 10, 18446744073709551615ULL,          _T("18446744073709551615"),            XF_U64 ),
    E( 16, 0x8000000000000000ULL,            _T("8000000000000000"),               XF_U64 ),
    E( 16, 0xFFFFFFFFFFFFFFFFULL,            _T("ffffffffffffffff"),               XF_U64 ),
    E( 16, 0xDEADBEEFCAFEBABEULL,           _T("deadbeefcafebabe"),               XF_U64 ),

    /* ===== Values > 32 bits, positive (only 64-bit functions) ===== */
    E( 10, 4294967296ULL,                    _T("4294967296"),                      XF_I64 | XF_U64 ),
    E( 10, 10000000000ULL,                   _T("10000000000"),                     XF_I64 | XF_U64 ),
    E( 16, 0x100000000ULL,                   _T("100000000"),                      XF_I64 | XF_U64 ),
    E( 8,  0x100000000ULL,                   _T("40000000000"),                    XF_I64 | XF_U64 ),

    /* ===== Other bases ===== */
    E( 3,  100,                              _T("10201"),                           XF_ALL ),
    E( 5,  100,                              _T("400"),                             XF_ALL ),
    E( 7,  100,                              _T("202"),                             XF_ALL ),

    /* ===== Invalid radix returns empty string ===== */
    E( 1,  123,                              _T(""),                                XF_ALL ),
    E( 37, 123,                              _T("3c"),                              XF_ALL ),
    E( 0,  123,                              _T(""),                                XF_ALL ),
    E( -1, 123,                              _T(""),                                XF_ALL ),
};

const size_t TABLE_LEN_NAME = sizeof(TABLE_NAME) / sizeof(TABLE_NAME[0]);
