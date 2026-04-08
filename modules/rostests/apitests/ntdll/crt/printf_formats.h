#include <stdint.h>
#include <stdlib.h>

#define TCHAR _TCHAR_
#ifdef USE_WIDECHAR
  typedef wchar_t TCHAR;
  #define _T(x) L##x
  #define _NT(x) x
  #define p__stprintf p__swprintf
  #define _tcslen wcslen
  #define _tcscmp wcscmp
  #define _tcschr wcschr
  #define TSTRFMT "%ws"
#else
  typedef char TCHAR;
  #define _T(x) x
  #define _NT(x) L##x
  #define p__stprintf p_sprintf
  #define _tcslen strlen
  #define _tcscmp strcmp
  #define _tcschr strchr
  #define TSTRFMT "%s"
#endif

#define ok_sprintf_format_(File, Line, ExpectedContent, Format, ...) \
    do { \
        TCHAR buf[64]; \
        int ret = p__stprintf(buf, Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == _tcslen(buf), "format \"" TSTRFMT "\": returned %d, expected %d\n", Format, ret, _tcslen(buf)); \
        ok_(File, Line)(!_tcscmp(buf, ExpectedContent), "content mismatch for format \"" TSTRFMT "\": got \"" TSTRFMT "\", expected \"" TSTRFMT "\"\n", Format, buf, ExpectedContent); \
    } while (0)

#define ok_sprintf_format(ExpectedContent, Format, ...) \
    ok_sprintf_format_(__FILE__, __LINE__, ExpectedContent, Format, ##__VA_ARGS__)

typedef struct _ANSI_STRING {
  unsigned short Length;
  unsigned short MaximumLength;
  const char*  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _UNICODE_STRING {
  unsigned short Length;
  unsigned short MaximumLength;
  const wchar_t*  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef size_t (*PFN_wcstombs)(char *mbstr, const wchar_t *wcstr, size_t count);
typedef size_t (*PFN_mbstowcs)(wchar_t *pwcs, const char *s, size_t n);

// TODO:
// - unify
// - move wZ to Z
// - test negative precision
// -add tests for precision and indirect precision for all formats
// - width/precision integer overflow tests

/***********************************************
 * 'c' - TCHAR character                       *
 ***********************************************/
static void Test_printf_format_c(void)
{
    ok_sprintf_format(_T("&"), _T("%c"), _T('&'));
    ok_sprintf_format(_T("\xFF"), _T("%c"), _T('\xFF'));
#ifdef USE_WIDECHAR
    ok_sprintf_format(_T("\uFFFF"), _T("%c"), _T('\uFFFF'));
#endif

    /* 'c' with flags */
    ok_sprintf_format(_T("&"), _T("%-c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%+c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%0c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("% c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%#c"), _T('&'));

    /* 'c' with flags and width */
    ok_sprintf_format(_T("  &"), _T("%3c"), _T('&'));
    ok_sprintf_format(_T("&  "), _T("%-3c"), _T('&'));
    ok_sprintf_format(_T("  &"), _T("%+3c"), _T('&'));
    ok_sprintf_format(_T("00&"), _T("%03c"), _T('&'));
    ok_sprintf_format(_T("  &"), _T("% 3c"), _T('&'));
    ok_sprintf_format(_T("  &"), _T("%#3c"), _T('&'));
    ok_sprintf_format(_T("&  "), _T("%-+0 #3c"), _T('&'));

    /* 'c' with width of 0 (no padding expected) */
    ok_sprintf_format(_T("&"), _T("%0c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%-0c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%00c"), _T('&'));

    /* 'c' with width of 1 (no padding expected) */
    ok_sprintf_format(_T("&"), _T("%1c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%-1c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%01c"), _T('&'));

    /* 'c' with flags and indirect width ('*') */
    ok_sprintf_format(_T("  &"), _T("%*c"), 3, _T('&'));
    ok_sprintf_format(_T("&  "), _T("%-*c"), 3, _T('&'));
    ok_sprintf_format(_T("  &"), _T("%+*c"), 3, _T('&'));
    ok_sprintf_format(_T("00&"), _T("%0*c"), 3, _T('&'));
    ok_sprintf_format(_T("  &"), _T("% *c"), 3, _T('&'));
    ok_sprintf_format(_T("  &"), _T("%#*c"), 3, _T('&'));

    /* 'c' with flags and indirect width of 0 */
    ok_sprintf_format(_T("&"), _T("%*c"), 0, _T('&'));
    ok_sprintf_format(_T("&"), _T("%-*c"), 0, _T('&'));
    ok_sprintf_format(_T("&"), _T("%+*c"), 0, _T('&'));
    ok_sprintf_format(_T("&"), _T("%0*c"), 0, _T('&'));
    ok_sprintf_format(_T("&"), _T("% *c"), 0, _T('&'));
    ok_sprintf_format(_T("&"), _T("%#*c"), 0, _T('&'));
    ok_sprintf_format(_T("&"), _T("%*c"), 0, _T('&'));
    ok_sprintf_format(_T(" &"), _T("%*2c"), 0, _T('&'));
    ok_sprintf_format(_T("&  "), _T("%-+0 #*c"), 3, _T('&'));

    /* 'c' with negative indirect width (acts as left-justified) */
    ok_sprintf_format(_T("&  "), _T("%*c"), -3, _T('&'));
    ok_sprintf_format(_T("&  "), _T("%-*c"), -3, _T('&'));
    ok_sprintf_format(_T("&  "), _T("%+*c"), -3, _T('&'));
    ok_sprintf_format(_T("&  "), _T("%0*c"), -3, _T('&'));
    ok_sprintf_format(_T("&  "), _T("% *c"), -3, _T('&'));
    ok_sprintf_format(_T("&  "), _T("%#*c"), -3, _T('&'));
    ok_sprintf_format(_T("&  "), _T("%-+0 #*c"), -3, _T('&'));

    /* 'c' with flags, indirect width and width digit (*1 + "2" = 12) */
    ok_sprintf_format(_T("           &"), _T("%*2c"), 1, _T('&'));
    ok_sprintf_format(_T("&           "), _T("%-*2c"), 1, _T('&'));
    ok_sprintf_format(_T("           &"), _T("%+*2c"), 1, _T('&'));
    ok_sprintf_format(_T("00000000000&"), _T("%0*2c"), 1, _T('&'));
    ok_sprintf_format(_T("           &"), _T("% *2c"), 1, _T('&'));
    ok_sprintf_format(_T("           &"), _T("%#*2c"), 1, _T('&'));

    /* 'c' with negative indirect width and width digit (*-1 + "2" = -12) */
    ok_sprintf_format(_T("&           "), _T("%*2c"), -1, _T('&'));
    ok_sprintf_format(_T("&           "), _T("%0*2c"), -1, _T('&'));

    /* 'c' with invalid indirect width use */
    ok_sprintf_format(_T("*c"), _T("%**c"), 1, 2, _T('&'));
    ok_sprintf_format(_T(" #3c"), _T("%-+*0 #3c"), 2, _T('&'));
    ok_sprintf_format(_T("*c"), _T("%3*c"), 3, _T('&'));

    /* 'c' with precision (ignored for %c) */
    ok_sprintf_format(_T("&"), _T("%.c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%.0c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%.3c"), _T('&'));
    ok_sprintf_format(_T("&"), _T("%.*c"), 3, _T('&'));
    ok_sprintf_format(_T("  &"), _T("%3.5c"), _T('&'));
    ok_sprintf_format(_T("  &"), _T("%*.*c"), 3, 5, _T('&'));

    /* 'c' with different size prefixes (plus %d to validate pramaters are parsed correctly) */
    ok_sprintf_format(_T("&,-1"), _T("%hhc,%d"), _T('&'), -1);
    ok_sprintf_format(_T("&,-1"), _T("%hc,%d"), _T('&'), -1);
    ok_sprintf_format(_T("&,-1"), _T("%lc,%d"), L'&', -1);
    ok_sprintf_format(_T("&,-1"), _T("%llc,%d"), _T('&'), -1);
    ok_sprintf_format(_T("&,-1"), _T("%jc,%d"), _T('&'), -1);
    ok_sprintf_format(_T("&,-1"), _T("%wc,%d"), L'&', -1);
    ok_sprintf_format(_T("&,-1"), _T("%I32c,%d"), _T('&'), -1);
    ok_sprintf_format(_T("&,-1"), _T("%I64c,%d"), _T('&'), -1);

#ifdef USE_WIDECHAR
    ok_sprintf_format(_T("\x01"), _T("%hc"), L'\x0101');
    ok_sprintf_format(_T("\x0101"), _T("%wc"), L'\x0101');
#else
    ok_sprintf_format(_T("\01"), _T("%hc"), L'\x0101');
    ok_sprintf_format(_T("a"), _T("%wc"), L'\x0101');
#endif

    /* 'c' with multiple size prefixes */
#ifdef USE_WIDECHAR
    ok_sprintf_format(_T("\x01"), _T("%whc"), L'\x0101'); // w + h -> h
    ok_sprintf_format(_T("\x01"), _T("%hwc"), L'\x0101'); // h + w -> h
#else
    ok_sprintf_format(_T("a"), _T("%whc"), L'\x0101'); // w + h -> w
    ok_sprintf_format(_T("a"), _T("%hwc"), L'\x0101'); // h + w -> w
#endif

    /* 'c' with invalid size prefixes */
    ok_sprintf_format(_T("Ic,-1"), _T("%Ic,%d"), -1);
    ok_sprintf_format(_T("tc,-1"), _T("%tc,%d"), -1);
    ok_sprintf_format(_T("zc,-1"), _T("%zc,%d"), -1);

    /* 'c' adjacent to %% */
    ok_sprintf_format(_T("%&"), _T("%%%c"), _T('&'));
    ok_sprintf_format(_T("&%"), _T("%c%%"), _T('&'));
}

static void Test_printf_format_C(void)
{
    /* 'C' - "inversed" TCHAR character */
#ifdef USE_WIDECHAR
    ok_sprintf_format(_T("\x01"), _T("%C"), L'\x0101');
    ok_sprintf_format(_T("\x0101"), _T("%wC"), L'\x0101');
    ok_sprintf_format(_T("\x0101"), _T("%lC"), L'\x0101');
    ok_sprintf_format(_T("\x01"), _T("%llC"), L'\x0101');
    ok_sprintf_format(_T("\x01"), _T("%hC"), L'\x0101');
    ok_sprintf_format(_T("\x01"), _T("%whC"), L'\x0101'); // w + h -> h
    ok_sprintf_format(_T("\x01"), _T("%hwC"), L'\x0101'); // h + w -> h
#else
    ok_sprintf_format(_T("a"), _T("%C"), L'\x0101');
    ok_sprintf_format(_T("a"), _T("%wC"), L'\x0101');
    ok_sprintf_format(_T("a"), _T("%lC"), L'\x0101');
    ok_sprintf_format(_T("a"), _T("%llC"), L'\x0101');
    ok_sprintf_format(_T("\x01"), _T("%hC"), L'\x0101');
    ok_sprintf_format(_T("a"), _T("%whC"), L'\x0101'); // w + h -> w
    ok_sprintf_format(_T("a"), _T("%hwC"), L'\x0101'); // h + w -> w
#endif
}

/***********************************************
 * 'd' - Decimal number                        *
 ***********************************************/
static void Test_printf_format_d(void)
{
    ok_sprintf_format(_T("0"), _T("%d"), 0);
    ok_sprintf_format(_T("123"), _T("%d"), 123);
    ok_sprintf_format(_T("-123"), _T("%d"), -123);
    ok_sprintf_format(_T("-2147483648"), _T("%d"), INT_MIN);
    ok_sprintf_format(_T("2147483647"), _T("%d"), INT_MAX);
    ok_sprintf_format(_T("-9223372036854775808"), _T("%lld"), INT64_MIN);
    ok_sprintf_format(_T("9223372036854775807"), _T("%lld"), INT64_MAX);

    /* 'd' with flags */
    ok_sprintf_format(_T("0"),    _T("%-d"), 0);
    ok_sprintf_format(_T("123"),  _T("%-d"), 123);
    ok_sprintf_format(_T("-123"), _T("%-d"), -123);
    ok_sprintf_format(_T("+0"),   _T("%+d"), 0);
    ok_sprintf_format(_T("+123"), _T("%+d"), 123);
    ok_sprintf_format(_T("-123"), _T("%+d"), -123);
    ok_sprintf_format(_T("0"),    _T("%0d"), 0);
    ok_sprintf_format(_T("123"),  _T("%0d"), 123);
    ok_sprintf_format(_T("-123"), _T("%0d"), -123);
    ok_sprintf_format(_T(" 0"),   _T("% d"), 0);
    ok_sprintf_format(_T(" 123"), _T("% d"), 123);
    ok_sprintf_format(_T("-123"), _T("% d"), -123);
    ok_sprintf_format(_T("0"),    _T("%#d"), 0);
    ok_sprintf_format(_T("123"),  _T("%#d"), 123);
    ok_sprintf_format(_T("-123"), _T("%#d"), -123);

    /* 'd' with flag combinations */
    ok_sprintf_format(_T("+123"), _T("%+ d"), 123);
    ok_sprintf_format(_T("+123"), _T("% +d"), 123);
    ok_sprintf_format(_T("+123"), _T("%+-d"), 123);
    ok_sprintf_format(_T("+123"), _T("%-+d"), 123);
    ok_sprintf_format(_T("+123"), _T("%+0d"), 123);
    ok_sprintf_format(_T("123"), _T("%#0d"), 123);
    ok_sprintf_format(_T(" 123"), _T("%# d"), 123);
    ok_sprintf_format(_T("+123"), _T("%#+d"), 123);
    ok_sprintf_format(_T("+123"), _T("%+#d"), 123);
    ok_sprintf_format(_T("+123"), _T("%0+d"), 123);

    /* 'd' with flags and width */
    ok_sprintf_format(_T("    0"), _T("%5d"), 0);
    ok_sprintf_format(_T("  123"), _T("%5d"), 123);
    ok_sprintf_format(_T(" -123"), _T("%5d"), -123);
    ok_sprintf_format(_T("0    "), _T("%-5d"), 0);
    ok_sprintf_format(_T("123  "), _T("%-5d"), 123);
    ok_sprintf_format(_T("-123 "), _T("%-5d"), -123);
    ok_sprintf_format(_T("   +0"), _T("%+5d"), 0);
    ok_sprintf_format(_T(" +123"), _T("%+5d"), 123);
    ok_sprintf_format(_T(" -123"), _T("%+5d"), -123);
    ok_sprintf_format(_T("00000"), _T("%05d"), 0);
    ok_sprintf_format(_T("00123"), _T("%05d"), 123);
    ok_sprintf_format(_T("-0123"), _T("%05d"), -123);
    ok_sprintf_format(_T("+0000"), _T("%+05d"), 0);
    ok_sprintf_format(_T("+0123"), _T("%+05d"), 123);
    ok_sprintf_format(_T("-0123"), _T("%+05d"), -123);
    ok_sprintf_format(_T("    0"), _T("% 5d"), 0);
    ok_sprintf_format(_T("  123"), _T("% 5d"), 123);
    ok_sprintf_format(_T(" -123"), _T("% 5d"), -123);
    ok_sprintf_format(_T("    0"), _T("%#5d"), 0);
    ok_sprintf_format(_T("  123"), _T("%#5d"), 123);
    ok_sprintf_format(_T(" -123"), _T("%#5d"), -123);

    ok_sprintf_format(_T("+0123"), _T("%+05d"), 123);
    ok_sprintf_format(_T("+0   "), _T("%-+0 #5d"), 0);
    ok_sprintf_format(_T("+123 "), _T("%-+0 #5d"), 123);
    ok_sprintf_format(_T("-123 "), _T("%-+0 #5d"), -123);

    /* 'd' with flags and indirect width ('*') */
    ok_sprintf_format(_T("    0"), _T("%*d"), 5, 0);
    ok_sprintf_format(_T("  123"), _T("%*d"), 5, 123);
    ok_sprintf_format(_T(" -123"), _T("%*d"), 5, -123);
    ok_sprintf_format(_T("0    "), _T("%-*d"), 5, 0);
    ok_sprintf_format(_T("123  "), _T("%-*d"), 5, 123);
    ok_sprintf_format(_T("-123 "), _T("%-*d"), 5, -123);
    ok_sprintf_format(_T("   +0"), _T("%+*d"), 5, 0);
    ok_sprintf_format(_T(" +123"), _T("%+*d"), 5, 123);
    ok_sprintf_format(_T(" -123"), _T("%+*d"), 5, -123);
    ok_sprintf_format(_T("00000"), _T("%0*d"), 5, 0);
    ok_sprintf_format(_T("00123"), _T("%0*d"), 5, 123);
    ok_sprintf_format(_T("-0123"), _T("%0*d"), 5, -123);
    ok_sprintf_format(_T("    0"), _T("% *d"), 5, 0);
    ok_sprintf_format(_T("  123"), _T("% *d"), 5, 123);
    ok_sprintf_format(_T(" -123"), _T("% *d"), 5, -123);
    ok_sprintf_format(_T("    0"), _T("%#*d"), 5, 0);
    ok_sprintf_format(_T("  123"), _T("%#*d"), 5, 123);
    ok_sprintf_format(_T(" -123"), _T("%#*d"), 5, -123);

    /* 'd' with flags and indirect width of 0 */
    ok_sprintf_format(_T("0"), _T("%*d"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%*d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%*d"), 0, -123);
    ok_sprintf_format(_T("0"), _T("%-*d"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%-*d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%-*d"), 0, -123);
    ok_sprintf_format(_T("+0"), _T("%+*d"), 0, 0);
    ok_sprintf_format(_T("+123"), _T("%+*d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%+*d"), 0, -123);
    ok_sprintf_format(_T("0"), _T("%0*d"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%0*d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%0*d"), 0, -123);
    ok_sprintf_format(_T(" 0"), _T("% *d"), 0, 0);
    ok_sprintf_format(_T(" 123"), _T("% *d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("% *d"), 0, -123);
    ok_sprintf_format(_T("0"), _T("%#*d"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%#*d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%#*d"), 0, -123);

    ok_sprintf_format(_T("+0   "), _T("%-+0 #*d"), 5, 0);
    ok_sprintf_format(_T("+123 "), _T("%-+0 #*d"), 5, 123);
    ok_sprintf_format(_T("-123 "), _T("%-+0 #*d"), 5, -123);

    /* 'd' with negative indirect width (acts as left-justified) */
    ok_sprintf_format(_T("0    "), _T("%*d"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%*d"), -5, 123);
    ok_sprintf_format(_T("-123 "), _T("%*d"), -5, -123);
    ok_sprintf_format(_T("0    "), _T("%-*d"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%-*d"), -5, 123);
    ok_sprintf_format(_T("-123 "), _T("%-*d"), -5, -123);
    ok_sprintf_format(_T("+0   "), _T("%+*d"), -5, 0);
    ok_sprintf_format(_T("+123 "), _T("%+*d"), -5, 123);
    ok_sprintf_format(_T("-123 "), _T("%+*d"), -5, -123);
    ok_sprintf_format(_T("0    "), _T("%0*d"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%0*d"), -5, 123);
    ok_sprintf_format(_T("-123 "), _T("%0*d"), -5, -123);
    ok_sprintf_format(_T(" 0   "), _T("% *d"), -5, 0);
    ok_sprintf_format(_T(" 123 "), _T("% *d"), -5, 123);
    ok_sprintf_format(_T("-123 "), _T("% *d"), -5, -123);
    ok_sprintf_format(_T("0    "), _T("%#*d"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%#*d"), -5, 123);
    ok_sprintf_format(_T("-123 "), _T("%#*d"), -5, -123);
    ok_sprintf_format(_T("+0   "), _T("%-+0 #*d"), -5, 0);
    ok_sprintf_format(_T("+123 "), _T("%-+0 #*d"), -5, 123);
    ok_sprintf_format(_T("-123 "), _T("%-+0 #*d"), -5, -123);

    /* 'd' with flags, indirect width and width digit (*1 + "0" = 10) */
    ok_sprintf_format(_T("         0"), _T("%*0d"), 1, 0);
    ok_sprintf_format(_T("       123"), _T("%*0d"), 1, 123);
    ok_sprintf_format(_T("      -123"), _T("%*0d"), 1, -123);
    ok_sprintf_format(_T("0         "), _T("%-*0d"), 1, 0);
    ok_sprintf_format(_T("123       "), _T("%-*0d"), 1, 123);
    ok_sprintf_format(_T("-123      "), _T("%-*0d"), 1, -123);
    ok_sprintf_format(_T("        +0"), _T("%+*0d"), 1, 0);
    ok_sprintf_format(_T("      +123"), _T("%+*0d"), 1, 123);
    ok_sprintf_format(_T("      -123"), _T("%+*0d"), 1, -123);
    ok_sprintf_format(_T("0000000000"), _T("%0*0d"), 1, 0);
    ok_sprintf_format(_T("0000000123"), _T("%0*0d"), 1, 123);
    ok_sprintf_format(_T("-000000123"), _T("%0*0d"), 1, -123);
    ok_sprintf_format(_T("         0"), _T("% *0d"), 1, 0);
    ok_sprintf_format(_T("       123"), _T("% *0d"), 1, 123);
    ok_sprintf_format(_T("      -123"), _T("% *0d"), 1, -123);
    ok_sprintf_format(_T("         0"), _T("%#*0d"), 1, 0);
    ok_sprintf_format(_T("       123"), _T("%#*0d"), 1, 123);
    ok_sprintf_format(_T("      -123"), _T("%#*0d"), 1, -123);

    /* 'd' with flags, indirect width 0 and width digit 0 (*0 + "0" = 0) */
    ok_sprintf_format(_T("0"), _T("%*0d"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%*0d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%*0d"), 0, -123);
    ok_sprintf_format(_T("0"), _T("%-*0d"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%-*0d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%-*0d"), 0, -123);
    ok_sprintf_format(_T("+0"), _T("%+*0d"), 0, 0);
    ok_sprintf_format(_T("+123"), _T("%+*0d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%+*0d"), 0, -123);
    ok_sprintf_format(_T("0"), _T("%0*0d"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%0*0d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%0*0d"), 0, -123);
    ok_sprintf_format(_T(" 0"), _T("% *0d"), 0, 0);
    ok_sprintf_format(_T(" 123"), _T("% *0d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("% *0d"), 0, -123);
    ok_sprintf_format(_T("0"), _T("%#*0d"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%#*0d"), 0, 123);
    ok_sprintf_format(_T("-123"), _T("%#*0d"), 0, -123);

    /* 'd' with negative indirect width and width digit (*-1 + "2" = -12) */
    ok_sprintf_format(_T("123         "), _T("%*2d"), -1, 123);
    ok_sprintf_format(_T("-123        "), _T("%*2d"), -1, -123);
    ok_sprintf_format(_T("123         "), _T("%0*2d"), -1, 123);
    ok_sprintf_format(_T("-123        "), _T("%0*2d"), -1, -123);

    /* 'd' with invalid indirect width use */
    ok_sprintf_format(_T("*d"), _T("%**d"), 1, 2, 123);
    ok_sprintf_format(_T(" #3d"), _T("%-+*0 #3d"), 2, 123);
    ok_sprintf_format(_T("*d"), _T("%3*d"), 3, 123);

    /* 'd' with flags and precision */
    ok_sprintf_format(_T("00000"),  _T("%.5d"), 0);
    ok_sprintf_format(_T("00123"),  _T("%.5d"), 123);
    ok_sprintf_format(_T("-00123"), _T("%.5d"), -123);
    ok_sprintf_format(_T("00000"),  _T("%-.5d"), 0);
    ok_sprintf_format(_T("00123"),  _T("%-.5d"), 123);
    ok_sprintf_format(_T("-00123"), _T("%-.5d"), -123);
    ok_sprintf_format(_T("+00000"), _T("%+.5d"), 0);
    ok_sprintf_format(_T("+00123"), _T("%+.5d"), 123);
    ok_sprintf_format(_T("-00123"), _T("%+.5d"), -123);
    ok_sprintf_format(_T("00000"),  _T("%0.5d"), 0);
    ok_sprintf_format(_T("00123"),  _T("%0.5d"), 123);
    ok_sprintf_format(_T("-00123"), _T("%0.5d"), -123);
    ok_sprintf_format(_T("+00000"), _T("%+0.5d"), 0);
    ok_sprintf_format(_T("+00123"), _T("%+0.5d"), 123);
    ok_sprintf_format(_T("-00123"), _T("%+0.5d"), -123);
    ok_sprintf_format(_T(" 00000"), _T("% .5d"), 0);
    ok_sprintf_format(_T(" 00123"), _T("% .5d"), 123);
    ok_sprintf_format(_T("-00123"), _T("% .5d"), -123);
    ok_sprintf_format(_T("00000"),  _T("%#.5d"), 0);
    ok_sprintf_format(_T("00123"),  _T("%#.5d"), 123);
    ok_sprintf_format(_T("-00123"), _T("%#.5d"), -123);

    /* 'd' with flags and small precision */
    ok_sprintf_format(_T("00"),  _T("%.2d"), 0);
    ok_sprintf_format(_T("123"),  _T("%.2d"), 123);
    ok_sprintf_format(_T("-123"), _T("%.2d"), -123);
    ok_sprintf_format(_T("00"),  _T("%-.2d"), 0);
    ok_sprintf_format(_T("123"),  _T("%-.2d"), 123);
    ok_sprintf_format(_T("-123"), _T("%-.2d"), -123);
    ok_sprintf_format(_T("+00"), _T("%+.2d"), 0);
    ok_sprintf_format(_T("+123"), _T("%+.2d"), 123);
    ok_sprintf_format(_T("-123"), _T("%+.2d"), -123);
    ok_sprintf_format(_T("00"),  _T("%0.2d"), 0);
    ok_sprintf_format(_T("123"),  _T("%0.2d"), 123);
    ok_sprintf_format(_T("-123"), _T("%0.2d"), -123);
    ok_sprintf_format(_T("+00"), _T("%+0.2d"), 0);
    ok_sprintf_format(_T("+123"), _T("%+0.2d"), 123);
    ok_sprintf_format(_T("-123"), _T("%+0.2d"), -123);
    ok_sprintf_format(_T(" 00"), _T("% .2d"), 0);
    ok_sprintf_format(_T(" 123"), _T("% .2d"), 123);
    ok_sprintf_format(_T("-123"), _T("% .2d"), -123);
    ok_sprintf_format(_T("00"),  _T("%#.2d"), 0);
    ok_sprintf_format(_T("123"),  _T("%#.2d"), 123);
    ok_sprintf_format(_T("-123"), _T("%#.2d"), -123);

    /* 'd' with flags, indirect precision and precision digit */
    ok_sprintf_format(_T("0000000000"),  _T("%.*0d"), 1, 0);
    ok_sprintf_format(_T("0000000123"),  _T("%.*0d"), 1, 123);
    ok_sprintf_format(_T("-0000000123"), _T("%.*0d"), 1, -123);
    ok_sprintf_format(_T("0000000000"),  _T("%-.*0d"), 1, 0);
    ok_sprintf_format(_T("0000000123"),  _T("%-.*0d"), 1, 123);
    ok_sprintf_format(_T("-0000000123"), _T("%-.*0d"), 1, -123);
    ok_sprintf_format(_T("+0000000000"), _T("%+.*0d"), 1, 0);
    ok_sprintf_format(_T("+0000000123"), _T("%+.*0d"), 1, 123);
    ok_sprintf_format(_T("-0000000123"), _T("%+.*0d"), 1, -123);
    ok_sprintf_format(_T("0000000000"),  _T("%0.*0d"), 1, 0);
    ok_sprintf_format(_T("0000000123"),  _T("%0.*0d"), 1, 123);
    ok_sprintf_format(_T("-0000000123"), _T("%0.*0d"), 1, -123);
    ok_sprintf_format(_T("+0000000000"), _T("%+0.*0d"), 1, 0);
    ok_sprintf_format(_T("+0000000123"), _T("%+0.*0d"), 1, 123);
    ok_sprintf_format(_T("-0000000123"), _T("%+0.*0d"), 1, -123);
    ok_sprintf_format(_T(" 0000000000"), _T("% .*0d"), 1, 0);
    ok_sprintf_format(_T(" 0000000123"), _T("% .*0d"), 1, 123);
    ok_sprintf_format(_T("-0000000123"), _T("% .*0d"), 1, -123);
    ok_sprintf_format(_T("0000000000"),  _T("%#.*0d"), 1, 0);
    ok_sprintf_format(_T("0000000123"),  _T("%#.*0d"), 1, 123);
    ok_sprintf_format(_T("-0000000123"), _T("%#.*0d"), 1, -123);

    /* 'd' with flags and precision 0 */
    ok_sprintf_format(_T(""),     _T("%.0d"), 0);
    ok_sprintf_format(_T("123"),  _T("%.0d"), 123);
    ok_sprintf_format(_T("-123"), _T("%.0d"), -123);
    ok_sprintf_format(_T(""),     _T("%-.0d"), 0);
    ok_sprintf_format(_T("123"),  _T("%-.0d"), 123);
    ok_sprintf_format(_T("-123"), _T("%-.0d"), -123);
    ok_sprintf_format(_T("+"),    _T("%+.0d"), 0);
    ok_sprintf_format(_T("+123"), _T("%+.0d"), 123);
    ok_sprintf_format(_T("-123"), _T("%+.0d"), -123);
    ok_sprintf_format(_T(""),     _T("%0.0d"), 0);
    ok_sprintf_format(_T("123"),  _T("%0.0d"), 123);
    ok_sprintf_format(_T("-123"), _T("%0.0d"), -123);
    ok_sprintf_format(_T("+"),    _T("%+0.0d"), 0);
    ok_sprintf_format(_T("+123"), _T("%+0.0d"), 123);
    ok_sprintf_format(_T("-123"), _T("%+0.0d"), -123);
    ok_sprintf_format(_T(" "),    _T("% .0d"), 0);
    ok_sprintf_format(_T(" 123"), _T("% .0d"), 123);
    ok_sprintf_format(_T("-123"), _T("% .0d"), -123);
    ok_sprintf_format(_T(""),     _T("%#.0d"), 0);
    ok_sprintf_format(_T("123"),  _T("%#.0d"), 123);
    ok_sprintf_format(_T("-123"), _T("%#.0d"), -123);

    /* '-' sign in precision is invalid */
    ok_sprintf_format(_T("-5d"),  _T("%.-5d"), 0);

    /* Negative precision is ignored */
    ok_sprintf_format(_T("0"),  _T("%.*d"), -1, 0);
    ok_sprintf_format(_T("0"),  _T("%.4294967294d"), 0);
    ok_sprintf_format(_T("0"),  _T("%.*0d"), -1, 0);
    ok_sprintf_format(_T("0"),  _T("%.*0d"), 429496729, 0);

    /* 'd' with width and precision */
    ok_sprintf_format(_T("  00000"), _T("%7.5d"), 0);
    ok_sprintf_format(_T("  00123"), _T("%7.5d"), 123);
    ok_sprintf_format(_T(" -00123"), _T("%7.5d"), -123);
    ok_sprintf_format(_T("00000  "), _T("%-7.5d"), 0);
    ok_sprintf_format(_T("00123  "), _T("%-7.5d"), 123);
    ok_sprintf_format(_T("-00123 "), _T("%-7.5d"), -123);
    ok_sprintf_format(_T(" +00000"), _T("%+7.5d"), 0);
    ok_sprintf_format(_T(" +00123"), _T("%+7.5d"), 123);
    ok_sprintf_format(_T(" -00123"), _T("%+7.5d"), -123);
    ok_sprintf_format(_T("  00000"), _T("%07.5d"), 0);
    ok_sprintf_format(_T("  00123"), _T("%07.5d"), 123);
    ok_sprintf_format(_T(" -00123"), _T("%07.5d"), -123);
    ok_sprintf_format(_T(" +00000"), _T("%+07.5d"), 0);
    ok_sprintf_format(_T(" +00123"), _T("%+07.5d"), 123);
    ok_sprintf_format(_T(" -00123"), _T("%+07.5d"), -123);
    ok_sprintf_format(_T("  00000"), _T("% 7.5d"), 0);
    ok_sprintf_format(_T("  00123"), _T("% 7.5d"), 123);
    ok_sprintf_format(_T(" -00123"), _T("% 7.5d"), -123);
    ok_sprintf_format(_T("  00000"), _T("%#7.5d"), 0);
    ok_sprintf_format(_T("  00123"), _T("%#7.5d"), 123);
    ok_sprintf_format(_T(" -00123"), _T("%#7.5d"), -123);

    /* 'd' with indirect width and indirect precision */
    ok_sprintf_format(_T("  00000"), _T("%*.*d"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("%*.*d"), 7, 5, 123);
    ok_sprintf_format(_T(" -00123"), _T("%*.*d"), 7, 5, -123);
    ok_sprintf_format(_T("00000  "), _T("%-*.*d"), 7, 5, 0);
    ok_sprintf_format(_T("00123  "), _T("%-*.*d"), 7, 5, 123);
    ok_sprintf_format(_T("-00123 "), _T("%-*.*d"), 7, 5, -123);
    ok_sprintf_format(_T(" +00000"), _T("%+*.*d"), 7, 5, 0);
    ok_sprintf_format(_T(" +00123"), _T("%+*.*d"), 7, 5, 123);
    ok_sprintf_format(_T(" -00123"), _T("%+*.*d"), 7, 5, -123);
    ok_sprintf_format(_T("  00000"), _T("%0*.*d"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("%0*.*d"), 7, 5, 123);
    ok_sprintf_format(_T(" -00123"), _T("%0*.*d"), 7, 5, -123);
    ok_sprintf_format(_T(" +00000"), _T("%+0*.*d"), 7, 5, 0);
    ok_sprintf_format(_T(" +00123"), _T("%+0*.*d"), 7, 5, 123);
    ok_sprintf_format(_T(" -00123"), _T("%+0*.*d"), 7, 5, -123);
    ok_sprintf_format(_T("  00000"), _T("% *.*d"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("% *.*d"), 7, 5, 123);
    ok_sprintf_format(_T(" -00123"), _T("% *.*d"), 7, 5, -123);
    ok_sprintf_format(_T("  00000"), _T("%#*.*d"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("%#*.*d"), 7, 5, 123);
    ok_sprintf_format(_T(" -00123"), _T("%#*.*d"), 7, 5, -123);

    /* 'd' with different size prefixes (plus %d to validate pramaters are parsed correctly) */
    ok_sprintf_format(_T("21605,-1"), _T("%hhd,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"), _T("%hd,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%ld,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Ld,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lld,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%wd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32d,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64d,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%td,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%td,%d"), -12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%zd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%zd,%d"), -12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Id,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%Id,%d"), -12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%td,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%td,%d"), -1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%zd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%zd,%d"), -1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Id,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%Id,%d"), -1234567890, -1);
#endif

    /* 'd' - with multiple size prefixes */
    ok_sprintf_format(_T("21605,-1"), _T("%hhhhd,%d"), 0xFFFF0000 + 21605, -1); // hh + hh -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhhd,%d"), 0xFFFF0000 + 21605, -1); // hh + h -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhld,%d"), 0xFFFF0000 + 21605, -1); // hh + l -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhLd,%d"), 0xFFFF0000 + 21605, -1); // hh + L -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhlld,%d"), 12341234567890LL, -1); // hh + ll -> 32
    ok_sprintf_format(_T("21605,-1"), _T("%hhLd,%d"), 0xFFFF0000 + 21605, -1); // hh + L -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhjd,%d"), 12341234567890LL, -1); // hh + j -> 64
    ok_sprintf_format(_T("21605,-1"), _T("%hhwd,%d"), 0xFFFF0000 + 21605, -1); // hh + w -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhI32d,%d"), 0xFFFF0000 + 21605, -1); // hh + I32 -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhI64d,%d"), 12341234567890LL, -1); // hh + I64 -> 64
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhtd,%d"), 12341234567890LL, -1); // hh + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhzd,%d"), 12341234567890LL, -1); // hh + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhId,%d"), 12341234567890LL, -1); // hh + z -> 64
#else
    ok_sprintf_format(_T("21605,-1"), _T("%hhtd,%d"), 0xFFFF0000 + 21605, -1); // hh + t -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhzd,%d"), 0xFFFF0000 + 21605, -1); // hh + z -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhId,%d"), 0xFFFF0000 + 21605, -1); // hh + z -> 16
#endif

    ok_sprintf_format(_T("21605,-1"), _T("%hhhd,%d"), 0xFFFF0000 + 21605, -1); // hh + hh + h -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhd,%d"), 0xFFFF0000 + 21605, -1); // hh + hh + l -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hld,%d"), 0xFFFF0000 + 21605, -1); // h + l -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hLd,%d"), 0xFFFF0000 + 21605, -1); // h + L -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hlld,%d"), 12341234567890LL, -1); // h + ll -> 64
    ok_sprintf_format(_T("21605,-1"), _T("%hLd,%d"), 0xFFFF0000 + 21605, -1); // h + L -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hjd,%d"), 12341234567890LL, -1); // h + j -> 64
    ok_sprintf_format(_T("21605,-1"), _T("%hwd,%d"), 0xFFFF0000 + 21605, -1); // h + w -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hI32d,%d"), 0xFFFF0000 + 21605, -1); // h + I32 -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hI64d,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%htd,%d"), 12341234567890LL, -1); // h + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hzd,%d"), 12341234567890LL, -1); // h + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hId,%d"), 12341234567890LL, -1); // h + z -> 64
#else
    ok_sprintf_format(_T("21605,-1"), _T("%htd,%d"), 0xFFFF0000 + 21605, -1); // h + t -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hzd,%d"), 0xFFFF0000 + 21605, -1); // h + z -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hId,%d"), 0xFFFF0000 + 21605, -1); // h + z -> 16
#endif

    ok_sprintf_format(_T("21605,-1"), _T("%lhhd,%d"), 0xFFFF0000 + 21605, -1); // l + hh -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%lhd,%d"), 0xFFFF0000 + 21605, -1); // l + h -> 16
    ok_sprintf_format(_T("1234567890,-1"), _T("%lLd,%d"), 1234567890, -1); // l + L -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llld,%d"), 12341234567890LL, -1); // l + ll -> 64 (actually ll + l)
    ok_sprintf_format(_T("12341234567890,-1"), _T("%ljd,%d"), 12341234567890LL, -1); // l + j -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%lwd,%d"), 1234567890, -1); // l + w -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%lI32d,%d"), 1234567890, -1); // l + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lI64d,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%ltd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lzd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lId,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%ltd,%d"), 1234567890, -1); // l + t -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%lzd,%d"), 1234567890, -1); // l + z -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%lId,%d"), 1234567890, -1); // l + t -> 32
#endif

    ok_sprintf_format(_T("12341234567890,-1"), _T("%llhhd,%d"), 12341234567890LL, -1); // ll + hh -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llhd,%d"), 12341234567890LL, -1); // ll + h -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llld,%d"), 12341234567890LL, -1); // ll + l -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llLd,%d"), 12341234567890LL, -1); // ll + L -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lllld,%d"), 12341234567890LL, -1); // ll + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lljd,%d"), 12341234567890LL, -1); // ll + j -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llwd,%d"), 12341234567890LL, -1); // ll + w -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llI32d,%d"), 12341234567890LL, -1); // ll + I32 -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llI64d,%d"), 12341234567890LL, -1); // ll + I64 -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lltd,%d"), 12341234567890LL, -1); // ll + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llzd,%d"), 12341234567890LL, -1); // ll + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llId,%d"), 12341234567890LL, -1); // ll + t -> 64

    ok_sprintf_format(_T("12341234567890,-1"), _T("%jhhd,%d"), 12341234567890LL, -1); // j + hh -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jhd,%d"), 12341234567890LL, -1); // j + h -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jld,%d"), 12341234567890LL, -1); // j + l -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jLd,%d"), 12341234567890LL, -1); // j + L -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jlld,%d"), 12341234567890LL, -1); // j + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jjd,%d"), 12341234567890LL, -1); // j + j -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jwd,%d"), 12341234567890LL, -1); // j + w -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%jI32d,%d"), 1234567890, -1); // j + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jI64d,%d"), 12341234567890LL, -1); // j + I64 -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jtd,%d"), 12341234567890LL, -1); // j + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jzd,%d"), 12341234567890LL, -1); // j + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jId,%d"), 12341234567890LL, -1); // j + t -> 64

    ok_sprintf_format(_T("21605,-1"), _T("%whhd,%d"), 0xFFFF0000 + 21605, -1); // w + hh -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%whd,%d"), 0xFFFF0000 + 21605, -1); // w + h -> 16
    ok_sprintf_format(_T("1234567890,-1"), _T("%wld,%d"), 1234567890, -1); // w + l -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%wLd,%d"), 1234567890, -1); // w + L -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%wlld,%d"), 12341234567890LL, -1); // w + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%wjd,%d"), 12341234567890LL, -1); // w + j -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%wwd,%d"), 1234567890, -1); // w + w -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%wI32d,%d"), 1234567890, -1); // w + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%wI64d,%d"), 12341234567890LL, -1); // w + I64 -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%wtd,%d"), 1234567890, -1); // w + t -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%wzd,%d"), 1234567890, -1); // w + z -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%wId,%d"), 1234567890, -1); // w + t -> 32

    ok_sprintf_format(_T("21605,-1"), _T("%I32hhd,%d"), 0xFFFF0000 + 21605, -1); // I32 + hh -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%I32hd,%d"), 0xFFFF0000 + 21605, -1); // I32 + h -> 16
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32ld,%d"), 1234567890, -1); // I32 + l -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32Ld,%d"), 1234567890, -1); // I32 + l -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I32lld,%d"), 12341234567890LL, -1); // I32 + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I32jd,%d"), 12341234567890LL, -1); // I32 + j -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32wd,%d"), 1234567890, -1); // I32 + w -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32I32d,%d"), 1234567890, -1); // I32 + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I32I64d,%d"), 12341234567890LL, -1); // I32 + I64 -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32td,%d"), 1234567890, -1); // I32 + t -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32zd,%d"), 1234567890, -1); // I32 + z -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32Id,%d"), 1234567890, -1); // I32 + t -> 32

    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64hhd,%d"), 12341234567890LL, -1); // I64 + hh -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64hd,%d"), 12341234567890LL, -1); // I64 + h -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64ld,%d"), 12341234567890LL, -1); // I64 + l -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64Ld,%d"), 12341234567890LL, -1); // I64 + L -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64lld,%d"), 12341234567890LL, -1); // I64 + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64jd,%d"), 12341234567890LL, -1); // I64 + j -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64wd,%d"), 12341234567890LL, -1); // I64 + w -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%I64I32d,%d"), 1234567890, -1); // I64 + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64I64d,%d"), 12341234567890LL, -1); // I64 + I64 -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64td,%d"), 12341234567890LL, -1); // I64 + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64zd,%d"), 12341234567890LL, -1); // I64 + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64Id,%d"), 12341234567890LL, -1); // I64 + t -> 64

    /* 'F' and 'N' size prefixes (far/near), that are ignored */
    ok_sprintf_format(_T("21605,-1"), _T("%Fhhd,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"), _T("%Fhd,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Fld,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Flld,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Fjd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Fwd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%FI32d,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%FI64d,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Ftd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%Ftd,%d"), -12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Fzd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%Fzd,%d"), -12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%FId,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%FId,%d"), -12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%Ftd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%Ftd,%d"), -1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Fzd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%Fzd,%d"), -1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%FId,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%FId,%d"), -1234567890, -1);
#endif

    ok_sprintf_format(_T("21605,-1"), _T("%Nhhd,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"), _T("%Nhd,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Nld,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Nlld,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Njd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Nwd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%NI32d,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%NI64d,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Ntd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%Ntd,%d"), -12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Nzd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%Nzd,%d"), -12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%NId,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%NId,%d"), -12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%Ntd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%Ntd,%d"), -1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Nzd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%Nzd,%d"), -1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%NId,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%NId,%d"), -1234567890, -1);
#endif

    /* 'L' (long double) is also ignored */
    ok_sprintf_format(_T("21605,-1"), _T("%Lhhd,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"), _T("%Lhd,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Lld,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Llld,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Ljd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Lwd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%LI32d,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%LI64d,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Ltd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%Ltd,%d"), -12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Lzd,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%Lzd,%d"), -12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%LId,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("-12341234567890,-1"), _T("%LId,%d"), -12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%Ltd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%Ltd,%d"), -1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Lzd,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%Lzd,%d"), -1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%LId,%d"), 1234567890, -1);
    ok_sprintf_format(_T("-1234567890,-1"), _T("%LId,%d"), -1234567890, -1);
#endif

    /* 't', 'z' and 'I' are special and cannot be followed by another size prefix */
    ok_sprintf_format(_T("thhd,-1"), _T("%thhd,%d"), -1);
    ok_sprintf_format(_T("thd,-1"), _T("%thd,%d"), -1);
    ok_sprintf_format(_T("tld,-1"), _T("%tld,%d"), -1);
    ok_sprintf_format(_T("tlld,-1"), _T("%tlld,%d"), -1);
    ok_sprintf_format(_T("tjd,-1"), _T("%tjd,%d"), -1);
    ok_sprintf_format(_T("twd,-1"), _T("%twd,%d"), -1);
    ok_sprintf_format(_T("tzd,-1"), _T("%tzd,%d"), -1);
    ok_sprintf_format(_T("ttd,-1"), _T("%ttd,%d"), -1);
    ok_sprintf_format(_T("tI32d,-1"), _T("%tI32d,%d"), -1);
    ok_sprintf_format(_T("tI64d,-1"), _T("%tI64d,%d"), -1);

    ok_sprintf_format(_T("zhhd,-1"), _T("%zhhd,%d"), -1);
    ok_sprintf_format(_T("zhd,-1"), _T("%zhd,%d"), -1);
    ok_sprintf_format(_T("zld,-1"), _T("%zld,%d"), -1);
    ok_sprintf_format(_T("zlld,-1"), _T("%zlld,%d"), -1);
    ok_sprintf_format(_T("zjd,-1"), _T("%zjd,%d"), -1);
    ok_sprintf_format(_T("zwd,-1"), _T("%zwd,%d"), -1);
    ok_sprintf_format(_T("zzd,-1"), _T("%zzd,%d"), -1);
    ok_sprintf_format(_T("ztd,-1"), _T("%ztd,%d"), -1);
    ok_sprintf_format(_T("zI32d,-1"), _T("%zI32d,%d"), -1);
    ok_sprintf_format(_T("zI64d,-1"), _T("%zI64d,%d"), -1);

    ok_sprintf_format(_T("Ihhd,-1"), _T("%Ihhd,%d"), -1);
    ok_sprintf_format(_T("Ihd,-1"), _T("%Ihd,%d"), -1);
    ok_sprintf_format(_T("Ild,-1"), _T("%Ild,%d"), -1);
    ok_sprintf_format(_T("Illd,-1"), _T("%Illd,%d"), -1);
    ok_sprintf_format(_T("Ijd,-1"), _T("%Ijd,%d"), -1);
    ok_sprintf_format(_T("Iwd,-1"), _T("%Iwd,%d"), -1);
    ok_sprintf_format(_T("Izd,-1"), _T("%Izd,%d"), -1);
    ok_sprintf_format(_T("Itd,-1"), _T("%Itd,%d"), -1);
    ok_sprintf_format(_T("II32d,-1"), _T("%II32d,%d"), -1);
    ok_sprintf_format(_T("II64d,-1"), _T("%II64d,%d"), -1);

    /* 'd' adjacent to %% */
    ok_sprintf_format(_T("%1234567890"), _T("%%%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890%"), _T("%d%%"), 1234567890, -1);

    /* 'u' with misc modifiers */
    ok_sprintf_format(_T("+123"), _T("%+*d"), 4, 123);
    ok_sprintf_format(_T("+123"), _T("%+.*d"), 0, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*d"), 4, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*d"), -4, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*d"), 0, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*d"), 1, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*d"), -1, 123);
    ok_sprintf_format(_T("+00123"), _T("%0+6d"), 123);
    ok_sprintf_format(_T("+00123"), _T("%+06d"), 123);
    ok_sprintf_format(_T("zhd,-1"), _T("%hzhd,%d"), -1);
    ok_sprintf_format(_T("thd,-1"), _T("%hthd,%d"), -1);
    ok_sprintf_format(_T("123,-1"), _T("%hI32hd,%d"), 123, -1);
    ok_sprintf_format(_T("123,-1"), _T("%hhhjllld,%d"), (int64_t)123, -1);
}

/***********************************************
 * 'i' - Decimal number                        *
 ***********************************************/
static void Test_printf_format_i(void)
{
    ok_sprintf_format(_T("123"), _T("%i"), 123);
    ok_sprintf_format(_T("-123"), _T("%i"), -123);

    /* 'i' - Decimal number */
    ok_sprintf_format(_T("   123"), _T("%6i"), 123);
    ok_sprintf_format(_T("123   "), _T("%-6i"), 123);
    ok_sprintf_format(_T("+123"), _T("%+i"), 123);
    ok_sprintf_format(_T(" 123"), _T("% i"), 123);
    ok_sprintf_format(_T("00123"), _T("%05i"), 123);
    ok_sprintf_format(_T("+0123"), _T("%+05i"), 123);
    ok_sprintf_format(_T("-0123"), _T("%05i"), -123);
    ok_sprintf_format(_T("0"), _T("%i"), 0);
    ok_sprintf_format(_T("-1"), _T("%i"), -1);
    ok_sprintf_format(_T("2147483647"), _T("%i"), 2147483647);
    ok_sprintf_format(_T("-2147483648"), _T("%i"), (int)0x80000000);
    ok_sprintf_format(_T("123"), _T("%.3i"), 123);
    ok_sprintf_format(_T("00123"), _T("%.5i"), 123);
    ok_sprintf_format(_T("   123"), _T("%6.3i"), 123);
    ok_sprintf_format(_T("   123"), _T("%06.3i"), 123);
    ok_sprintf_format(_T(""), _T("%.0i"), 0);
    ok_sprintf_format(_T("123"), _T("%*i"), 3, 123);
    ok_sprintf_format(_T("123   "), _T("%-*i"), 6, 123);
    ok_sprintf_format(_T("00123"), _T("%0*i"), 5, 123);
    ok_sprintf_format(_T("+123"), _T("%+*i"), 4, 123);
    ok_sprintf_format(_T("   123"), _T("%*.*i"), 6, 3, 123);
    ok_sprintf_format(_T("   123"), _T("%0*.*i"), 6, 3, 123);
    ok_sprintf_format(_T("123"), _T("%hhi"), (signed char)123);
    ok_sprintf_format(_T("123"), _T("%hi"), (short)123);
    ok_sprintf_format(_T("123"), _T("%li"), (long)123);
    ok_sprintf_format(_T("123"), _T("%lli"), (long long)123);
    ok_sprintf_format(_T("123"), _T("%ji"), (intmax_t)123);
    ok_sprintf_format(_T("123"), _T("%zi"), (size_t)123);
    ok_sprintf_format(_T("123"), _T("%ti"), (ptrdiff_t)123);
    ok_sprintf_format(_T("123"), _T("%I32i"), (int)123);
    ok_sprintf_format(_T("123"), _T("%I64i"), (long long)123);

    /* 'i' with all flag/width/precision/size/invalid combinations */
    ok_sprintf_format(_T("+123"), _T("%+i"), 123);
    ok_sprintf_format(_T(" 123"), _T("% i"), 123);
    ok_sprintf_format(_T("+123"), _T("%+ i"), 123);
    ok_sprintf_format(_T("+123"), _T("% +i"), 123);
    ok_sprintf_format(_T("+123"), _T("%+-i"), 123);
    ok_sprintf_format(_T("+123"), _T("%-+i"), 123);
    ok_sprintf_format(_T("+123"), _T("%+0i"), 123);
    ok_sprintf_format(_T("+00123"), _T("%+06i"), 123);
    ok_sprintf_format(_T("+123"), _T("%+*i"), 4, 123);
    ok_sprintf_format(_T("+123"), _T("%+*i"), -4, 123);
    ok_sprintf_format(_T("+123"), _T("%+.*i"), 0, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*i"), 4, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*i"), -4, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*i"), 0, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*i"), 1, 123);
    ok_sprintf_format(_T("+123"), _T("%+0*i"), -1, 123);
    ok_sprintf_format(_T("123"), _T("%#i"), 123); /* # ignored */
}

/***********************************************
 * 'u' - Unsigned number                       *
 ***********************************************/
static void Test_printf_format_u(void)
{
    ok_sprintf_format(_T("0"), _T("%u"), 0);
    ok_sprintf_format(_T("123"), _T("%u"), 123);
    ok_sprintf_format(_T("4294967173"), _T("%u"), -123);
    ok_sprintf_format(_T("2147483648"), _T("%u"), INT_MIN);
    ok_sprintf_format(_T("2147483647"), _T("%u"), INT_MAX);
    ok_sprintf_format(_T("9223372036854775808"), _T("%llu"), INT64_MIN);
    ok_sprintf_format(_T("9223372036854775807"), _T("%llu"), INT64_MAX);
    ok_sprintf_format(_T("18446744073709551615"), _T("%llu"), UINT64_MAX);

    /* 'u' with flags */
    ok_sprintf_format(_T("0"),    _T("%-u"), 0);
    ok_sprintf_format(_T("123"),  _T("%-u"), 123);
    ok_sprintf_format(_T("0"),   _T("%+u"), 0);
    ok_sprintf_format(_T("123"), _T("%+u"), 123);
    ok_sprintf_format(_T("0"),    _T("%0u"), 0);
    ok_sprintf_format(_T("123"),  _T("%0u"), 123);
    ok_sprintf_format(_T("0"),   _T("% u"), 0);
    ok_sprintf_format(_T("123"), _T("% u"), 123);
    ok_sprintf_format(_T("0"),    _T("%#u"), 0);
    ok_sprintf_format(_T("123"),  _T("%#u"), 123);

    /* 'u' with flag combinations */
    ok_sprintf_format(_T("123"), _T("%+-u"), 123);
    ok_sprintf_format(_T("123"), _T("%-+u"), 123);
    ok_sprintf_format(_T("123"), _T("%+0u"), 123);
    ok_sprintf_format(_T("123"), _T("%#0u"), 123);
    ok_sprintf_format(_T("123"), _T("%# u"), 123);
    ok_sprintf_format(_T("123"), _T("%#+u"), 123);
    ok_sprintf_format(_T("123"), _T("%+#u"), 123);
    ok_sprintf_format(_T("123"), _T("%0+u"), 123);
    ok_sprintf_format(_T("123"), _T("%+ u"), 123);
    ok_sprintf_format(_T("123"), _T("% +u"), 123);

    /* 'u' with flags and width */
    ok_sprintf_format(_T("    0"), _T("%5u"), 0);
    ok_sprintf_format(_T("  123"), _T("%5u"), 123);
    ok_sprintf_format(_T("0    "), _T("%-5u"), 0);
    ok_sprintf_format(_T("123  "), _T("%-5u"), 123);
    ok_sprintf_format(_T("    0"), _T("%+5u"), 0);
    ok_sprintf_format(_T("  123"), _T("%+5u"), 123);
    ok_sprintf_format(_T("00000"), _T("%05u"), 0);
    ok_sprintf_format(_T("00123"), _T("%05u"), 123);
    ok_sprintf_format(_T("00000"), _T("%+05u"), 0);
    ok_sprintf_format(_T("00123"), _T("%+05u"), 123);
    ok_sprintf_format(_T("    0"), _T("% 5u"), 0);
    ok_sprintf_format(_T("  123"), _T("% 5u"), 123);
    ok_sprintf_format(_T("    0"), _T("%#5u"), 0);
    ok_sprintf_format(_T("  123"), _T("%#5u"), 123);

    ok_sprintf_format(_T("00123"), _T("%+05u"), 123);
    ok_sprintf_format(_T("0    "), _T("%-+0 #5u"), 0);
    ok_sprintf_format(_T("123  "), _T("%-+0 #5u"), 123);

    /* 'u' with flags and indirect width ('*') */
    ok_sprintf_format(_T("    0"), _T("%*u"), 5, 0);
    ok_sprintf_format(_T("  123"), _T("%*u"), 5, 123);
    ok_sprintf_format(_T("0    "), _T("%-*u"), 5, 0);
    ok_sprintf_format(_T("123  "), _T("%-*u"), 5, 123);
    ok_sprintf_format(_T("    0"), _T("%+*u"), 5, 0);
    ok_sprintf_format(_T("  123"), _T("%+*u"), 5, 123);
    ok_sprintf_format(_T("00000"), _T("%0*u"), 5, 0);
    ok_sprintf_format(_T("00123"), _T("%0*u"), 5, 123);
    ok_sprintf_format(_T("    0"), _T("% *u"), 5, 0);
    ok_sprintf_format(_T("  123"), _T("% *u"), 5, 123);
    ok_sprintf_format(_T("    0"), _T("%#*u"), 5, 0);
    ok_sprintf_format(_T("  123"), _T("%#*u"), 5, 123);

    /* 'u' with flags and indirect width of 0 */
    ok_sprintf_format(_T("0"), _T("%*u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%*u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%-*u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%-*u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%+*u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%+*u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%0*u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%0*u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("% *u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("% *u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%#*u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%#*u"), 0, 123);

    ok_sprintf_format(_T("0    "), _T("%-+0 #*u"), 5, 0);
    ok_sprintf_format(_T("123  "), _T("%-+0 #*u"), 5, 123);

    /* 'u' with negative indirect width (acts as left-justified) */
    ok_sprintf_format(_T("0    "), _T("%*u"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%*u"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%-*u"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%-*u"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%+*u"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%+*u"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%0*u"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%0*u"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("% *u"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("% *u"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%#*u"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%#*u"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%-+0 #*u"), -5, 0);
    ok_sprintf_format(_T("123  "), _T("%-+0 #*u"), -5, 123);

    /* 'u' with flags, indirect width and width digit (*1 + "0" = 10) */
    ok_sprintf_format(_T("         0"), _T("%*0u"), 1, 0);
    ok_sprintf_format(_T("       123"), _T("%*0u"), 1, 123);
    ok_sprintf_format(_T("0         "), _T("%-*0u"), 1, 0);
    ok_sprintf_format(_T("123       "), _T("%-*0u"), 1, 123);
    ok_sprintf_format(_T("         0"), _T("%+*0u"), 1, 0);
    ok_sprintf_format(_T("       123"), _T("%+*0u"), 1, 123);
    ok_sprintf_format(_T("0000000000"), _T("%0*0u"), 1, 0);
    ok_sprintf_format(_T("0000000123"), _T("%0*0u"), 1, 123);
    ok_sprintf_format(_T("         0"), _T("% *0u"), 1, 0);
    ok_sprintf_format(_T("       123"), _T("% *0u"), 1, 123);
    ok_sprintf_format(_T("         0"), _T("%#*0u"), 1, 0);
    ok_sprintf_format(_T("       123"), _T("%#*0u"), 1, 123);

    /* 'u' with flags, indirect width 0 and width digit 0 (*0 + "0" = 0) */
    ok_sprintf_format(_T("0"), _T("%*0u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%*0u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%-*0u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%-*0u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%+*0u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%+*0u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%0*0u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%0*0u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("% *0u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("% *0u"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%#*0u"), 0, 0);
    ok_sprintf_format(_T("123"), _T("%#*0u"), 0, 123);

    /* 'u' with negative indirect width and width digit (*-1 + "2" = -12) */
    ok_sprintf_format(_T("123         "), _T("%*2u"), -1, 123);
    ok_sprintf_format(_T("123         "), _T("%0*2u"), -1, 123);

    /* 'u' with invalid indirect width use */
    ok_sprintf_format(_T("*u"), _T("%**u"), 1, 2, 123);
    ok_sprintf_format(_T(" #3u"), _T("%-+*0 #3u"), 2, 123);
    ok_sprintf_format(_T("*u"), _T("%3*u"), 3, 123);

    /* 'u' with flags and precision */
    ok_sprintf_format(_T("00000"),  _T("%.5u"), 0);
    ok_sprintf_format(_T("00123"),  _T("%.5u"), 123);
    ok_sprintf_format(_T("00000"),  _T("%-.5u"), 0);
    ok_sprintf_format(_T("00123"),  _T("%-.5u"), 123);
    ok_sprintf_format(_T("00000"), _T("%+.5u"), 0);
    ok_sprintf_format(_T("00123"), _T("%+.5u"), 123);
    ok_sprintf_format(_T("00000"),  _T("%0.5u"), 0);
    ok_sprintf_format(_T("00123"),  _T("%0.5u"), 123);
    ok_sprintf_format(_T("00000"), _T("%+0.5u"), 0);
    ok_sprintf_format(_T("00123"), _T("%+0.5u"), 123);
    ok_sprintf_format(_T("00000"), _T("% .5u"), 0);
    ok_sprintf_format(_T("00123"), _T("% .5u"), 123);
    ok_sprintf_format(_T("00000"),  _T("%#.5u"), 0);
    ok_sprintf_format(_T("00123"),  _T("%#.5u"), 123);

    /* 'u' with flags and small precision */
    ok_sprintf_format(_T("00"),  _T("%.2u"), 0);
    ok_sprintf_format(_T("123"),  _T("%.2u"), 123);
    ok_sprintf_format(_T("00"),  _T("%-.2u"), 0);
    ok_sprintf_format(_T("123"),  _T("%-.2u"), 123);
    ok_sprintf_format(_T("00"), _T("%+.2u"), 0);
    ok_sprintf_format(_T("123"), _T("%+.2u"), 123);
    ok_sprintf_format(_T("00"),  _T("%0.2u"), 0);
    ok_sprintf_format(_T("123"),  _T("%0.2u"), 123);
    ok_sprintf_format(_T("00"), _T("%+0.2u"), 0);
    ok_sprintf_format(_T("123"), _T("%+0.2u"), 123);
    ok_sprintf_format(_T("00"), _T("% .2u"), 0);
    ok_sprintf_format(_T("123"), _T("% .2u"), 123);
    ok_sprintf_format(_T("00"),  _T("%#.2u"), 0);
    ok_sprintf_format(_T("123"),  _T("%#.2u"), 123);

    /* 'u' with flags, indirect precision and precision digit */
    ok_sprintf_format(_T("0000000000"),  _T("%.*0u"), 1, 0);
    ok_sprintf_format(_T("0000000123"),  _T("%.*0u"), 1, 123);
    ok_sprintf_format(_T("0000000000"),  _T("%-.*0u"), 1, 0);
    ok_sprintf_format(_T("0000000123"),  _T("%-.*0u"), 1, 123);
    ok_sprintf_format(_T("0000000000"), _T("%+.*0u"), 1, 0);
    ok_sprintf_format(_T("0000000123"), _T("%+.*0u"), 1, 123);
    ok_sprintf_format(_T("0000000000"),  _T("%0.*0u"), 1, 0);
    ok_sprintf_format(_T("0000000123"),  _T("%0.*0u"), 1, 123);
    ok_sprintf_format(_T("0000000000"), _T("%+0.*0u"), 1, 0);
    ok_sprintf_format(_T("0000000123"), _T("%+0.*0u"), 1, 123);
    ok_sprintf_format(_T("0000000000"), _T("% .*0u"), 1, 0);
    ok_sprintf_format(_T("0000000123"), _T("% .*0u"), 1, 123);
    ok_sprintf_format(_T("0000000000"),  _T("%#.*0u"), 1, 0);
    ok_sprintf_format(_T("0000000123"),  _T("%#.*0u"), 1, 123);

    /* 'u' with flags and precision 0 */
    ok_sprintf_format(_T(""),     _T("%.0u"), 0);
    ok_sprintf_format(_T("123"),  _T("%.0u"), 123);
    ok_sprintf_format(_T(""),     _T("%-.0u"), 0);
    ok_sprintf_format(_T("123"),  _T("%-.0u"), 123);
    ok_sprintf_format(_T(""),    _T("%+.0u"), 0);
    ok_sprintf_format(_T("123"), _T("%+.0u"), 123);
    ok_sprintf_format(_T(""),     _T("%0.0u"), 0);
    ok_sprintf_format(_T("123"),  _T("%0.0u"), 123);
    ok_sprintf_format(_T(""),    _T("%+0.0u"), 0);
    ok_sprintf_format(_T("123"), _T("%+0.0u"), 123);
    ok_sprintf_format(_T(""),    _T("% .0u"), 0);
    ok_sprintf_format(_T("123"), _T("% .0u"), 123);
    ok_sprintf_format(_T(""),     _T("%#.0u"), 0);
    ok_sprintf_format(_T("123"),  _T("%#.0u"), 123);

    /* '-' sign in precision is invalid */
    ok_sprintf_format(_T("-5u"),  _T("%.-5u"), 0);

    /* Negative precision is ignored */
    ok_sprintf_format(_T("0"),  _T("%.*u"), -1, 0);
    ok_sprintf_format(_T("0"),  _T("%.4294967294u"), 0);
    ok_sprintf_format(_T("0"),  _T("%.*0u"), -1, 0);
    ok_sprintf_format(_T("0"),  _T("%.*0u"), 429496729, 0);

    /* 'u' with width and precision */
    ok_sprintf_format(_T("  00000"), _T("%7.5u"), 0);
    ok_sprintf_format(_T("  00123"), _T("%7.5u"), 123);
    ok_sprintf_format(_T("00000  "), _T("%-7.5u"), 0);
    ok_sprintf_format(_T("00123  "), _T("%-7.5u"), 123);
    ok_sprintf_format(_T("  00000"), _T("%+7.5u"), 0);
    ok_sprintf_format(_T("  00123"), _T("%+7.5u"), 123);
    ok_sprintf_format(_T("  00000"), _T("%07.5u"), 0);
    ok_sprintf_format(_T("  00123"), _T("%07.5u"), 123);
    ok_sprintf_format(_T("  00000"), _T("%+07.5u"), 0);
    ok_sprintf_format(_T("  00123"), _T("%+07.5u"), 123);
    ok_sprintf_format(_T("  00000"), _T("% 7.5u"), 0);
    ok_sprintf_format(_T("  00123"), _T("% 7.5u"), 123);
    ok_sprintf_format(_T("  00000"), _T("%#7.5u"), 0);
    ok_sprintf_format(_T("  00123"), _T("%#7.5u"), 123);

    /* 'u' with indirect width and indirect precision */
    ok_sprintf_format(_T("  00000"), _T("%*.*u"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("%*.*u"), 7, 5, 123);
    ok_sprintf_format(_T("00000  "), _T("%-*.*u"), 7, 5, 0);
    ok_sprintf_format(_T("00123  "), _T("%-*.*u"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("%+*.*u"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("%+*.*u"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("%0*.*u"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("%0*.*u"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("%+0*.*u"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("%+0*.*u"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("% *.*u"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("% *.*u"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("%#*.*u"), 7, 5, 0);
    ok_sprintf_format(_T("  00123"), _T("%#*.*u"), 7, 5, 123);

    /* 'u' with different size prefixes (plus %d to validate pramaters are parsed correctly) */
    ok_sprintf_format(_T("21605,-1"),           _T("%hhu,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"),           _T("%hu,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("1234567890,-1"),      _T("%lu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"),      _T("%Lu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"),  _T("%lld,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"),  _T("%ju,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("1234567890,-1"),      _T("%wu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"),      _T("%I32u,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"),  _T("%I64u,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"),  _T("%tu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"),  _T("%zu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"),  _T("%Iu,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"),      _T("%tu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"),      _T("%zu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"),      _T("%Iu,%d"), 1234567890, -1);
#endif

    /* 'u' with multiple size prefixes */
    ok_sprintf_format(_T("21605,-1"), _T("%hhhhu,%d"), 0xFFFF0000 + 21605, -1); // hh + hh -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhhu,%d"), 0xFFFF0000 + 21605, -1); // hh + h -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhlu,%d"), 0xFFFF0000 + 21605, -1); // hh + l -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhLu,%d"), 0xFFFF0000 + 21605, -1); // hh + L -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhllu,%d"), 12341234567890LL, -1); // hh + ll -> 32
    ok_sprintf_format(_T("21605,-1"), _T("%hhLu,%d"), 0xFFFF0000 + 21605, -1); // hh + L -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhju,%d"), 12341234567890LL, -1); // hh + j -> 64
    ok_sprintf_format(_T("21605,-1"), _T("%hhwu,%d"), 0xFFFF0000 + 21605, -1); // hh + w -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhI32u,%d"), 0xFFFF0000 + 21605, -1); // hh + I32 -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhI64u,%d"), 12341234567890LL, -1); // hh + I64 -> 64
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhtu,%d"), 12341234567890LL, -1); // hh + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhzu,%d"), 12341234567890LL, -1); // hh + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hhIu,%d"), 12341234567890LL, -1); // hh + z -> 64
#else
    ok_sprintf_format(_T("21605,-1"), _T("%hhtu,%d"), 0xFFFF0000 + 21605, -1); // hh + t -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhzu,%d"), 0xFFFF0000 + 21605, -1); // hh + z -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhIu,%d"), 0xFFFF0000 + 21605, -1); // hh + z -> 16
#endif

    ok_sprintf_format(_T("21605,-1"), _T("%hhhu,%d"), 0xFFFF0000 + 21605, -1); // hh + hh + h -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hhu,%d"), 0xFFFF0000 + 21605, -1); // hh + hh + l -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hlu,%d"), 0xFFFF0000 + 21605, -1); // h + l -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hLu,%d"), 0xFFFF0000 + 21605, -1); // h + L -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hllu,%d"), 12341234567890LL, -1); // h + ll -> 64
    ok_sprintf_format(_T("21605,-1"), _T("%hLu,%d"), 0xFFFF0000 + 21605, -1); // h + L -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hju,%d"), 12341234567890LL, -1); // h + j -> 64
    ok_sprintf_format(_T("21605,-1"), _T("%hwu,%d"), 0xFFFF0000 + 21605, -1); // h + w -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hI32u,%d"), 0xFFFF0000 + 21605, -1); // h + I32 -> 16
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hI64u,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%htu,%d"), 12341234567890LL, -1); // h + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hzu,%d"), 12341234567890LL, -1); // h + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%hIu,%d"), 12341234567890LL, -1); // h + z -> 64
#else
    ok_sprintf_format(_T("21605,-1"), _T("%htu,%d"), 0xFFFF0000 + 21605, -1); // h + t -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hzu,%d"), 0xFFFF0000 + 21605, -1); // h + z -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%hIu,%d"), 0xFFFF0000 + 21605, -1); // h + z -> 16
#endif

    ok_sprintf_format(_T("21605,-1"), _T("%lhhu,%d"), 0xFFFF0000 + 21605, -1); // l + hh -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%lhu,%d"), 0xFFFF0000 + 21605, -1); // l + h -> 16
    ok_sprintf_format(_T("1234567890,-1"), _T("%lLu,%d"), 1234567890, -1); // l + L -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lllu,%d"), 12341234567890LL, -1); // l + ll -> 64 (actually ll + l)
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lju,%d"), 12341234567890LL, -1); // l + j -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%lwu,%d"), 1234567890, -1); // l + w -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%lI32u,%d"), 1234567890, -1); // l + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lI64u,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%ltu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lzu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lIu,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%ltu,%d"), 1234567890, -1); // l + t -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%lzu,%d"), 1234567890, -1); // l + z -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%lIu,%d"), 1234567890, -1); // l + t -> 32
#endif

    ok_sprintf_format(_T("12341234567890,-1"), _T("%llhhu,%d"), 12341234567890LL, -1); // ll + hh -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llhu,%d"), 12341234567890LL, -1); // ll + h -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lllu,%d"), 12341234567890LL, -1); // ll + l -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llLu,%d"), 12341234567890LL, -1); // ll + L -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llllu,%d"), 12341234567890LL, -1); // ll + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llju,%d"), 12341234567890LL, -1); // ll + j -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llwu,%d"), 12341234567890LL, -1); // ll + w -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llI32u,%d"), 12341234567890LL, -1); // ll + I32 -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llI64u,%d"), 12341234567890LL, -1); // ll + I64 -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%lltu,%d"), 12341234567890LL, -1); // ll + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llzu,%d"), 12341234567890LL, -1); // ll + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%llIu,%d"), 12341234567890LL, -1); // ll + t -> 64

    ok_sprintf_format(_T("12341234567890,-1"), _T("%jhhu,%d"), 12341234567890LL, -1); // j + hh -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jhu,%d"), 12341234567890LL, -1); // j + h -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jlu,%d"), 12341234567890LL, -1); // j + l -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jLu,%d"), 12341234567890LL, -1); // j + L -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jllu,%d"), 12341234567890LL, -1); // j + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jju,%d"), 12341234567890LL, -1); // j + j -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jwu,%d"), 12341234567890LL, -1); // j + w -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%jI32u,%d"), 1234567890, -1); // j + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jI64u,%d"), 12341234567890LL, -1); // j + I64 -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jtu,%d"), 12341234567890LL, -1); // j + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jzu,%d"), 12341234567890LL, -1); // j + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%jIu,%d"), 12341234567890LL, -1); // j + t -> 64

    ok_sprintf_format(_T("21605,-1"), _T("%whhu,%d"), 0xFFFF0000 + 21605, -1); // w + hh -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%whu,%d"), 0xFFFF0000 + 21605, -1); // w + h -> 16
    ok_sprintf_format(_T("1234567890,-1"), _T("%wlu,%d"), 1234567890, -1); // w + l -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%wLu,%d"), 1234567890, -1); // w + L -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%wllu,%d"), 12341234567890LL, -1); // w + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%wju,%d"), 12341234567890LL, -1); // w + j -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%wwu,%d"), 1234567890, -1); // w + w -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%wI32u,%d"), 1234567890, -1); // w + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%wI64u,%d"), 12341234567890LL, -1); // w + I64 -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%wtu,%d"), 1234567890, -1); // w + t -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%wzu,%d"), 1234567890, -1); // w + z -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%wIu,%d"), 1234567890, -1); // w + t -> 32

    ok_sprintf_format(_T("21605,-1"), _T("%I32hhu,%d"), 0xFFFF0000 + 21605, -1); // I32 + hh -> 16
    ok_sprintf_format(_T("21605,-1"), _T("%I32hu,%d"), 0xFFFF0000 + 21605, -1); // I32 + h -> 16
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32lu,%d"), 1234567890, -1); // I32 + l -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32Lu,%d"), 1234567890, -1); // I32 + l -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I32llu,%d"), 12341234567890LL, -1); // I32 + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I32ju,%d"), 12341234567890LL, -1); // I32 + j -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32wu,%d"), 1234567890, -1); // I32 + w -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32I32u,%d"), 1234567890, -1); // I32 + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I32I64u,%d"), 12341234567890LL, -1); // I32 + I64 -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32tu,%d"), 1234567890, -1); // I32 + t -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32zu,%d"), 1234567890, -1); // I32 + z -> 32
    ok_sprintf_format(_T("1234567890,-1"), _T("%I32Iu,%d"), 1234567890, -1); // I32 + t -> 32

    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64hhu,%d"), 12341234567890LL, -1); // I64 + hh -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64hu,%d"), 12341234567890LL, -1); // I64 + h -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64lu,%d"), 12341234567890LL, -1); // I64 + l -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64Lu,%d"), 12341234567890LL, -1); // I64 + L -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64llu,%d"), 12341234567890LL, -1); // I64 + ll -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64ju,%d"), 12341234567890LL, -1); // I64 + j -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64wu,%d"), 12341234567890LL, -1); // I64 + w -> 64
    ok_sprintf_format(_T("1234567890,-1"), _T("%I64I32u,%d"), 1234567890, -1); // I64 + I32 -> 32
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64I64u,%d"), 12341234567890LL, -1); // I64 + I64 -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64tu,%d"), 12341234567890LL, -1); // I64 + t -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64zu,%d"), 12341234567890LL, -1); // I64 + z -> 64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%I64Iu,%d"), 12341234567890LL, -1); // I64 + t -> 64

    /* 'F' and 'N' size prefixes (far/near), that are ignored */
    ok_sprintf_format(_T("21605,-1"), _T("%Fhhu,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"), _T("%Fhu,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Flu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Fllu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Fju,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Fwu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%FI32u,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%FI64u,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Ftu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Fzu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%FIu,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%Ftu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Fzu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%FIu,%d"), 1234567890, -1);
#endif

    ok_sprintf_format(_T("21605,-1"), _T("%Nhhu,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"), _T("%Nhu,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Nlu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Nllu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Nju,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Nwu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%NI32u,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%NI64u,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Ntu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Nzu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%NIu,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%Ntu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Nzu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%NIu,%d"), 1234567890, -1);
#endif

    /* 'L' (long double) is also ignored */
    ok_sprintf_format(_T("21605,-1"), _T("%Lhhu,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"), _T("%Lhu,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Llu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Lllu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Lju,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Lwu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%LI32u,%d"), 1234567890, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%LI64u,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Ltu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%Lzu,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("12341234567890,-1"), _T("%LIu,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("1234567890,-1"), _T("%Ltu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%Lzu,%d"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890,-1"), _T("%LIu,%d"), 1234567890, -1);
#endif

    /* 't', 'z' and 'I' are special and cannot be followed by another size prefix */
    ok_sprintf_format(_T("thhu,-1"), _T("%thhu,%d"), -1);
    ok_sprintf_format(_T("thu,-1"), _T("%thu,%d"), -1);
    ok_sprintf_format(_T("tlu,-1"), _T("%tlu,%d"), -1);
    ok_sprintf_format(_T("tllu,-1"), _T("%tllu,%d"), -1);
    ok_sprintf_format(_T("tju,-1"), _T("%tju,%d"), -1);
    ok_sprintf_format(_T("twu,-1"), _T("%twu,%d"), -1);
    ok_sprintf_format(_T("tzu,-1"), _T("%tzu,%d"), -1);
    ok_sprintf_format(_T("ttu,-1"), _T("%ttu,%d"), -1);
    ok_sprintf_format(_T("tI32u,-1"), _T("%tI32u,%d"), -1);
    ok_sprintf_format(_T("tI64u,-1"), _T("%tI64u,%d"), -1);

    ok_sprintf_format(_T("zhhu,-1"), _T("%zhhu,%d"), -1);
    ok_sprintf_format(_T("zhu,-1"), _T("%zhu,%d"), -1);
    ok_sprintf_format(_T("zlu,-1"), _T("%zlu,%d"), -1);
    ok_sprintf_format(_T("zllu,-1"), _T("%zllu,%d"), -1);
    ok_sprintf_format(_T("zju,-1"), _T("%zju,%d"), -1);
    ok_sprintf_format(_T("zwu,-1"), _T("%zwu,%d"), -1);
    ok_sprintf_format(_T("zzu,-1"), _T("%zzu,%d"), -1);
    ok_sprintf_format(_T("ztu,-1"), _T("%ztu,%d"), -1);
    ok_sprintf_format(_T("zI32u,-1"), _T("%zI32u,%d"), -1);
    ok_sprintf_format(_T("zI64u,-1"), _T("%zI64u,%d"), -1);

    ok_sprintf_format(_T("Ihhu,-1"), _T("%Ihhu,%d"), -1);
    ok_sprintf_format(_T("Ihu,-1"), _T("%Ihu,%d"), -1);
    ok_sprintf_format(_T("Ilu,-1"), _T("%Ilu,%d"), -1);
    ok_sprintf_format(_T("Illu,-1"), _T("%Illu,%d"), -1);
    ok_sprintf_format(_T("Iju,-1"), _T("%Iju,%d"), -1);
    ok_sprintf_format(_T("Iwu,-1"), _T("%Iwu,%d"), -1);
    ok_sprintf_format(_T("Izu,-1"), _T("%Izu,%d"), -1);
    ok_sprintf_format(_T("Itu,-1"), _T("%Itu,%d"), -1);
    ok_sprintf_format(_T("II32u,-1"), _T("%II32u,%d"), -1);
    ok_sprintf_format(_T("II64u,-1"), _T("%II64u,%d"), -1);

    /* 'u' adjacent to %% */
    ok_sprintf_format(_T("%1234567890"), _T("%%%u"), 1234567890, -1);
    ok_sprintf_format(_T("1234567890%"), _T("%u%%"), 1234567890, -1);

    /* 'u' with misc modifiers */
    ok_sprintf_format(_T("123"), _T("%+.*u"), 0, 123);
    ok_sprintf_format(_T("0123"), _T("%+0*u"), 4, 123);
    ok_sprintf_format(_T("123 "), _T("%+0*u"), -4, 123);
    ok_sprintf_format(_T("123"), _T("%+0*u"), 0, 123);
    ok_sprintf_format(_T("123"), _T("%+0*u"), 1, 123);
    ok_sprintf_format(_T("123"), _T("%+0*u"), -1, 123);
    ok_sprintf_format(_T("000123"), _T("%0+6u"), 123);
    ok_sprintf_format(_T("000123"), _T("%+06u"), 123);
    ok_sprintf_format(_T("zhu,-1"), _T("%hzhu,%d"), -1);
    ok_sprintf_format(_T("thu,-1"), _T("%hthu,%d"), -1);
    ok_sprintf_format(_T("123,-1"), _T("%hI32hu,%d"), (size_t)123, -1);
    ok_sprintf_format(_T("123,-1"), _T("%hhhjlllu,%d"), (int64_t)123, -1);
}

/***********************************************
 * 'o' - Octal number                          *
 ***********************************************/
static void Test_printf_format_o(void)
{
    ok_sprintf_format(_T("372"), _T("%o"), 0372);
    ok_sprintf_format(_T("37777777406"), _T("%o"), -0372);

    /* 'o' with flags */
    ok_sprintf_format(_T("372"), _T("%-o"), 0372);
    ok_sprintf_format(_T("372"), _T("%+o"), 0372);
    ok_sprintf_format(_T("37777777406"), _T("%+o"), -0372);
    ok_sprintf_format(_T("372"), _T("%0o"), 0372);
    ok_sprintf_format(_T("372"), _T("% o"), 0372);
    ok_sprintf_format(_T("37777777406"), _T("% o"), -0372);
    ok_sprintf_format(_T("0372"), _T("%#o"), 0372);

    /* 'o' with flags and width */
    ok_sprintf_format(_T("  372"), _T("%5o"), 0372);
    ok_sprintf_format(_T("372  "), _T("%-5o"), 0372);
    ok_sprintf_format(_T("  372"), _T("%+5o"), 0372);
    ok_sprintf_format(_T("00372"), _T("%05o"), 0372);
    ok_sprintf_format(_T("00372"), _T("%+05o"), 0372);
    ok_sprintf_format(_T("  372"), _T("% 5o"), 0372);
    ok_sprintf_format(_T(" 0372"), _T("%#5o"), 0372);
    ok_sprintf_format(_T("0372 "), _T("%-+0 #5o"), 0372);

    /* 'o' with flags and indirect width ('*') */
    ok_sprintf_format(_T("  372"), _T("%*o"), 5, 0372);
    ok_sprintf_format(_T("372  "), _T("%-*o"), 5, 0372);
    ok_sprintf_format(_T("  372"), _T("%+*o"), 5, 0372);
    ok_sprintf_format(_T("00372"), _T("%0*o"), 5, 0372);
    ok_sprintf_format(_T("  372"), _T("% *o"), 5, 0372);
    ok_sprintf_format(_T(" 0372"), _T("%#*o"), 5, 0372);
    ok_sprintf_format(_T("372"), _T("%*o"), 0, 0372);
    ok_sprintf_format(_T("  372"), _T("%*5o"), 0, 0372);
    ok_sprintf_format(_T("0372 "), _T("%-+0 #*o"), 5, 0372);

    /* 'o' with negative indirect width (acts as left-justified) */
    /* 'o' with flags, indirect width and width digit (*1 + "2" = 12) */
    /* 'o' with negative indirect width and width digit (*-1 + "2" = -12) */
    /* 'o' with invalid indirect width use */
    /* 'o' with different size prefixes (plus %u to validate pramaters are parsed correctly) */
    /* 'o' with invalid size prefixes */
    /* 'o' with precision (ignored for %c) */
    /* 'o' with width of 1 (no padding expected) */
    /* 'o' adjacent to %% */


    /* 'o' - Octal number */
    ok_sprintf_format(_T("000372"), _T("%06o"), 0372);
    ok_sprintf_format(_T("372   "), _T("%-6o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%06o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%0*o"), 6, 0372);
    ok_sprintf_format(_T("372"), _T("%.3o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%.6o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%#06o"), 0372);
    ok_sprintf_format(_T("0372"), _T("%#o"), 0372);
    ok_sprintf_format(_T("0"), _T("%o"), 0);
    ok_sprintf_format(_T(""), _T("%.0o"), 0);
    ok_sprintf_format(_T("0"), _T("%#o"), 0);
    ok_sprintf_format(_T("  0"), _T("%#3o"), 0);
    ok_sprintf_format(_T("000"), _T("%#03o"), 0);
    ok_sprintf_format(_T("000"), _T("%#.3o"), 0);
    ok_sprintf_format(_T("0"), _T("%#.0o"), 0);
    ok_sprintf_format(_T("0"), _T("%#.1o"), 0);
    ok_sprintf_format(_T("00"), _T("%#.2o"), 0);
    ok_sprintf_format(_T("01"), _T("%#.0o"), 1);
    ok_sprintf_format(_T("01"), _T("%#.1o"), 1);
    ok_sprintf_format(_T("01"), _T("%#.2o"), 1);
    ok_sprintf_format(_T("012"), _T("%#.3o"), 012);
    ok_sprintf_format(_T("012"), _T("%#.0o"), 012);
    ok_sprintf_format(_T(""), _T("%.0o"), 0);
    ok_sprintf_format(_T("0"), _T("%#.0o"), 0);
    ok_sprintf_format(_T("372"), _T("%ho"), (unsigned short)0372);
    ok_sprintf_format(_T("372"), _T("%hho"), (unsigned char)0372);
    ok_sprintf_format(_T("372"), _T("%lo"), (unsigned long)0372);
    ok_sprintf_format(_T("372"), _T("%llo"), (unsigned long long)0372);
    ok_sprintf_format(_T("372"), _T("%jo"), (uintmax_t)0372);
    ok_sprintf_format(_T("372"), _T("%zo"), (size_t)0372);
    ok_sprintf_format(_T("372"), _T("%to"), (ptrdiff_t)0372);
    ok_sprintf_format(_T("372"), _T("%I32o"), (unsigned int)0372);
    ok_sprintf_format(_T("372"), _T("%I64o"), (unsigned long long)0372);

    /* 'o' - All flag/width/precision/size/invalid combinations */
    ok_sprintf_format(_T("0372"), _T("%#o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%06o"), 0372);
    ok_sprintf_format(_T("372   "), _T("%-6o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%06o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%0*o"), 6, 0372);
    ok_sprintf_format(_T("372"), _T("%.3o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%.6o"), 0372);
    ok_sprintf_format(_T("000372"), _T("%#06o"), 0372);
    ok_sprintf_format(_T("0"), _T("%o"), 0);
    ok_sprintf_format(_T("0"), _T("%#o"), 0);
    ok_sprintf_format(_T(""), _T("%.0o"), 0);
    ok_sprintf_format(_T("0"), _T("%#.0o"), 0);
    ok_sprintf_format(_T("    0372"), _T("%#8.4o"), 0372);
    ok_sprintf_format(_T("    0372"), _T("%#08.4o"), 0372);
    ok_sprintf_format(_T("0372    "), _T("%-#8o"), 0372);
    ok_sprintf_format(_T("0372    "), _T("%#-8o"), 0372);
    ok_sprintf_format(_T("00000372"), _T("%#08o"), 0372);
    ok_sprintf_format(_T("0372    "), _T("%#-08o"), 0372);
}

/***********************************************
 * 'x' - Hexadecimal number                    *
 ***********************************************/
static void Test_printf_format_x(void)
{
    ok_sprintf_format(_T("0"), _T("%x"), 0);
    ok_sprintf_format(_T("7b"), _T("%x"), 123);
    ok_sprintf_format(_T("ffffff85"), _T("%x"), -123);
    ok_sprintf_format(_T("80000000"), _T("%x"), INT_MIN);
    ok_sprintf_format(_T("7fffffff"), _T("%x"), INT_MAX);
    ok_sprintf_format(_T("8000000000000000"), _T("%llx"), INT64_MIN);
    ok_sprintf_format(_T("7fffffffffffffff"), _T("%llx"), INT64_MAX);
    ok_sprintf_format(_T("ffffffffffffffff"), _T("%llx"), UINT64_MAX);

    /* 'x' with flags */
    ok_sprintf_format(_T("0"),    _T("%-x"), 0);
    ok_sprintf_format(_T("7b"),  _T("%-x"), 123);
    ok_sprintf_format(_T("0"),   _T("%+x"), 0);
    ok_sprintf_format(_T("7b"), _T("%+x"), 123);
    ok_sprintf_format(_T("0"),    _T("%0x"), 0);
    ok_sprintf_format(_T("7b"),  _T("%0x"), 123);
    ok_sprintf_format(_T("0"),   _T("% x"), 0);
    ok_sprintf_format(_T("7b"), _T("% x"), 123);
    ok_sprintf_format(_T("0"),    _T("%#x"), 0);
    ok_sprintf_format(_T("0x7b"),  _T("%#x"), 123);

    /* 'x' with flag combinations */
    ok_sprintf_format(_T("7b"), _T("%+-x"), 123);
    ok_sprintf_format(_T("7b"), _T("%-+x"), 123);
    ok_sprintf_format(_T("7b"), _T("%+0x"), 123);
    ok_sprintf_format(_T("0x7b"), _T("%#0x"), 123);
    ok_sprintf_format(_T("0x7b"), _T("%# x"), 123);
    ok_sprintf_format(_T("0x7b"), _T("%#+x"), 123);
    ok_sprintf_format(_T("0x7b"), _T("%+#x"), 123);
    ok_sprintf_format(_T("7b"), _T("%0+x"), 123);
    ok_sprintf_format(_T("7b"), _T("%+ x"), 123);
    ok_sprintf_format(_T("7b"), _T("% +x"), 123);

    /* 'x' with flags and width */
    ok_sprintf_format(_T("    0"), _T("%5x"), 0);
    ok_sprintf_format(_T("   7b"), _T("%5x"), 123);
    ok_sprintf_format(_T("0    "), _T("%-5x"), 0);
    ok_sprintf_format(_T("7b   "), _T("%-5x"), 123);
    ok_sprintf_format(_T("    0"), _T("%+5x"), 0);
    ok_sprintf_format(_T("   7b"), _T("%+5x"), 123);
    ok_sprintf_format(_T("00000"), _T("%05x"), 0);
    ok_sprintf_format(_T("0007b"), _T("%05x"), 123);
    ok_sprintf_format(_T("00000"), _T("%+05x"), 0);
    ok_sprintf_format(_T("0007b"), _T("%+05x"), 123);
    ok_sprintf_format(_T("    0"), _T("% 5x"), 0);
    ok_sprintf_format(_T("   7b"), _T("% 5x"), 123);
    ok_sprintf_format(_T("    0"), _T("%#5x"), 0);
    ok_sprintf_format(_T(" 0x7b"), _T("%#5x"), 123);

    ok_sprintf_format(_T("0007b"), _T("%+05x"), 123);
    ok_sprintf_format(_T("0    "), _T("%-+0 #5x"), 0);
    ok_sprintf_format(_T("0x7b "), _T("%-+0 #5x"), 123);

    /* 'x' with flags and indirect width ('*') */
    ok_sprintf_format(_T("    0"), _T("%*x"), 5, 0);
    ok_sprintf_format(_T("   7b"), _T("%*x"), 5, 123);
    ok_sprintf_format(_T("0    "), _T("%-*x"), 5, 0);
    ok_sprintf_format(_T("7b   "), _T("%-*x"), 5, 123);
    ok_sprintf_format(_T("    0"), _T("%+*x"), 5, 0);
    ok_sprintf_format(_T("   7b"), _T("%+*x"), 5, 123);
    ok_sprintf_format(_T("00000"), _T("%0*x"), 5, 0);
    ok_sprintf_format(_T("0007b"), _T("%0*x"), 5, 123);
    ok_sprintf_format(_T("    0"), _T("% *x"), 5, 0);
    ok_sprintf_format(_T("   7b"), _T("% *x"), 5, 123);
    ok_sprintf_format(_T("    0"), _T("%#*x"), 5, 0);
    ok_sprintf_format(_T(" 0x7b"), _T("%#*x"), 5, 123);

    /* 'x' with flags and indirect width of 0 */
    ok_sprintf_format(_T("0"), _T("%*x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("%*x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%-*x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("%-*x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%+*x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("%+*x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%0*x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("%0*x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("% *x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("% *x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%#*x"), 0, 0);
    ok_sprintf_format(_T("0x7b"), _T("%#*x"), 0, 123);

    ok_sprintf_format(_T("0    "), _T("%-+0 #*x"), 5, 0);
    ok_sprintf_format(_T("0x7b "), _T("%-+0 #*x"), 5, 123);

    /* 'x' with negative indirect width (acts as left-justified) */
    ok_sprintf_format(_T("0    "), _T("%*x"), -5, 0);
    ok_sprintf_format(_T("7b   "), _T("%*x"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%-*x"), -5, 0);
    ok_sprintf_format(_T("7b   "), _T("%-*x"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%+*x"), -5, 0);
    ok_sprintf_format(_T("7b   "), _T("%+*x"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%0*x"), -5, 0);
    ok_sprintf_format(_T("7b   "), _T("%0*x"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("% *x"), -5, 0);
    ok_sprintf_format(_T("7b   "), _T("% *x"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%#*x"), -5, 0);
    ok_sprintf_format(_T("0x7b "), _T("%#*x"), -5, 123);
    ok_sprintf_format(_T("0    "), _T("%-+0 #*x"), -5, 0);
    ok_sprintf_format(_T("0x7b "), _T("%-+0 #*x"), -5, 123);

    /* 'x' with flags, indirect width and width digit (*1 + "0" = 10) */
    ok_sprintf_format(_T("         0"), _T("%*0x"), 1, 0);
    ok_sprintf_format(_T("        7b"), _T("%*0x"), 1, 123);
    ok_sprintf_format(_T("0         "), _T("%-*0x"), 1, 0);
    ok_sprintf_format(_T("7b        "), _T("%-*0x"), 1, 123);
    ok_sprintf_format(_T("         0"), _T("%+*0x"), 1, 0);
    ok_sprintf_format(_T("        7b"), _T("%+*0x"), 1, 123);
    ok_sprintf_format(_T("0000000000"), _T("%0*0x"), 1, 0);
    ok_sprintf_format(_T("000000007b"), _T("%0*0x"), 1, 123);
    ok_sprintf_format(_T("         0"), _T("% *0x"), 1, 0);
    ok_sprintf_format(_T("        7b"), _T("% *0x"), 1, 123);
    ok_sprintf_format(_T("         0"), _T("%#*0x"), 1, 0);
    ok_sprintf_format(_T("      0x7b"), _T("%#*0x"), 1, 123);

    /* 'x' with flags, indirect width 0 and width digit 0 (*0 + "0" = 0) */
    ok_sprintf_format(_T("0"), _T("%*0x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("%*0x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%-*0x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("%-*0x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%+*0x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("%+*0x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%0*0x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("%0*0x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("% *0x"), 0, 0);
    ok_sprintf_format(_T("7b"), _T("% *0x"), 0, 123);
    ok_sprintf_format(_T("0"), _T("%#*0x"), 0, 0);
    ok_sprintf_format(_T("0x7b"), _T("%#*0x"), 0, 123);

    /* 'x' with negative indirect width and width digit (*-1 + "2" = -12) */
    ok_sprintf_format(_T("7b          "), _T("%*2x"), -1, 123);
    ok_sprintf_format(_T("7b          "), _T("%0*2x"), -1, 123);

    /* 'x' with invalid indirect width use */
    ok_sprintf_format(_T("*x"), _T("%**x"), 1, 2, 123);
    ok_sprintf_format(_T(" #3x"), _T("%-+*0 #3x"), 2, 123);
    ok_sprintf_format(_T("*x"), _T("%3*x"), 3, 123);

    /* 'x' with flags and precision */
    ok_sprintf_format(_T("00000"),  _T("%.5x"), 0);
    ok_sprintf_format(_T("0007b"),  _T("%.5x"), 123);
    ok_sprintf_format(_T("00000"),  _T("%-.5x"), 0);
    ok_sprintf_format(_T("0007b"),  _T("%-.5x"), 123);
    ok_sprintf_format(_T("00000"), _T("%+.5x"), 0);
    ok_sprintf_format(_T("0007b"), _T("%+.5x"), 123);
    ok_sprintf_format(_T("00000"),  _T("%0.5x"), 0);
    ok_sprintf_format(_T("0007b"),  _T("%0.5x"), 123);
    ok_sprintf_format(_T("00000"), _T("%+0.5x"), 0);
    ok_sprintf_format(_T("0007b"), _T("%+0.5x"), 123);
    ok_sprintf_format(_T("00000"), _T("% .5x"), 0);
    ok_sprintf_format(_T("0007b"), _T("% .5x"), 123);
    ok_sprintf_format(_T("00000"),  _T("%#.5x"), 0);
    ok_sprintf_format(_T("0x0007b"),  _T("%#.5x"), 123);

    /* 'x' with flags and small precision */
    ok_sprintf_format(_T("00"),  _T("%.2x"), 0);
    ok_sprintf_format(_T("7b"),  _T("%.2x"), 123);
    ok_sprintf_format(_T("00"),  _T("%-.2x"), 0);
    ok_sprintf_format(_T("7b"),  _T("%-.2x"), 123);
    ok_sprintf_format(_T("00"), _T("%+.2x"), 0);
    ok_sprintf_format(_T("7b"), _T("%+.2x"), 123);
    ok_sprintf_format(_T("00"),  _T("%0.2x"), 0);
    ok_sprintf_format(_T("7b"),  _T("%0.2x"), 123);
    ok_sprintf_format(_T("00"), _T("%+0.2x"), 0);
    ok_sprintf_format(_T("7b"), _T("%+0.2x"), 123);
    ok_sprintf_format(_T("00"), _T("% .2x"), 0);
    ok_sprintf_format(_T("7b"), _T("% .2x"), 123);
    ok_sprintf_format(_T("00"),  _T("%#.2x"), 0);
    ok_sprintf_format(_T("0x7b"),  _T("%#.2x"), 123);

    /* 'x' with flags, indirect precision and precision digit */
    ok_sprintf_format(_T("0000000000"),  _T("%.*0x"), 1, 0);
    ok_sprintf_format(_T("000000007b"),  _T("%.*0x"), 1, 123);
    ok_sprintf_format(_T("0000000000"),  _T("%-.*0x"), 1, 0);
    ok_sprintf_format(_T("000000007b"),  _T("%-.*0x"), 1, 123);
    ok_sprintf_format(_T("0000000000"), _T("%+.*0x"), 1, 0);
    ok_sprintf_format(_T("000000007b"), _T("%+.*0x"), 1, 123);
    ok_sprintf_format(_T("0000000000"),  _T("%0.*0x"), 1, 0);
    ok_sprintf_format(_T("000000007b"),  _T("%0.*0x"), 1, 123);
    ok_sprintf_format(_T("0000000000"), _T("%+0.*0x"), 1, 0);
    ok_sprintf_format(_T("000000007b"), _T("%+0.*0x"), 1, 123);
    ok_sprintf_format(_T("0000000000"), _T("% .*0x"), 1, 0);
    ok_sprintf_format(_T("000000007b"), _T("% .*0x"), 1, 123);
    ok_sprintf_format(_T("0000000000"),  _T("%#.*0x"), 1, 0);
    ok_sprintf_format(_T("0x000000007b"),  _T("%#.*0x"), 1, 123);

    /* 'x' with flags and precision 0 */
    ok_sprintf_format(_T(""),     _T("%.0x"), 0);
    ok_sprintf_format(_T("7b"),  _T("%.0x"), 123);
    ok_sprintf_format(_T(""),     _T("%-.0x"), 0);
    ok_sprintf_format(_T("7b"),  _T("%-.0x"), 123);
    ok_sprintf_format(_T(""),    _T("%+.0x"), 0);
    ok_sprintf_format(_T("7b"), _T("%+.0x"), 123);
    ok_sprintf_format(_T(""),     _T("%0.0x"), 0);
    ok_sprintf_format(_T("7b"),  _T("%0.0x"), 123);
    ok_sprintf_format(_T(""),    _T("%+0.0x"), 0);
    ok_sprintf_format(_T("7b"), _T("%+0.0x"), 123);
    ok_sprintf_format(_T(""),    _T("% .0x"), 0);
    ok_sprintf_format(_T("7b"), _T("% .0x"), 123);
    ok_sprintf_format(_T(""),     _T("%#.0x"), 0);
    ok_sprintf_format(_T("0x7b"),  _T("%#.0x"), 123);

    /* '-' sign in precision is invalid */
    ok_sprintf_format(_T("-5x"),  _T("%.-5x"), 0);

    /* Negative precision is ignored */
    ok_sprintf_format(_T("0"),  _T("%.*x"), -1, 0);
    ok_sprintf_format(_T("0"),  _T("%.4294967294x"), 0);
    ok_sprintf_format(_T("0"),  _T("%.*0x"), -1, 0);
    ok_sprintf_format(_T("0"),  _T("%.*0x"), 429496729, 0);

    /* 'x' with width and precision */
    ok_sprintf_format(_T("  00000"), _T("%7.5x"), 0);
    ok_sprintf_format(_T("  0007b"), _T("%7.5x"), 123);
    ok_sprintf_format(_T("00000  "), _T("%-7.5x"), 0);
    ok_sprintf_format(_T("0007b  "), _T("%-7.5x"), 123);
    ok_sprintf_format(_T("  00000"), _T("%+7.5x"), 0);
    ok_sprintf_format(_T("  0007b"), _T("%+7.5x"), 123);
    ok_sprintf_format(_T("  00000"), _T("%07.5x"), 0);
    ok_sprintf_format(_T("  0007b"), _T("%07.5x"), 123);
    ok_sprintf_format(_T("  00000"), _T("%+07.5x"), 0);
    ok_sprintf_format(_T("  0007b"), _T("%+07.5x"), 123);
    ok_sprintf_format(_T("  00000"), _T("% 7.5x"), 0);
    ok_sprintf_format(_T("  0007b"), _T("% 7.5x"), 123);
    ok_sprintf_format(_T("  00000"), _T("%#7.5x"), 0);
    ok_sprintf_format(_T("0x0007b"), _T("%#7.5x"), 123);

    /* 'x' with indirect width and indirect precision */
    ok_sprintf_format(_T("  00000"), _T("%*.*x"), 7, 5, 0);
    ok_sprintf_format(_T("  0007b"), _T("%*.*x"), 7, 5, 123);
    ok_sprintf_format(_T("00000  "), _T("%-*.*x"), 7, 5, 0);
    ok_sprintf_format(_T("0007b  "), _T("%-*.*x"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("%+*.*x"), 7, 5, 0);
    ok_sprintf_format(_T("  0007b"), _T("%+*.*x"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("%0*.*x"), 7, 5, 0);
    ok_sprintf_format(_T("  0007b"), _T("%0*.*x"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("%+0*.*x"), 7, 5, 0);
    ok_sprintf_format(_T("  0007b"), _T("%+0*.*x"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("% *.*x"), 7, 5, 0);
    ok_sprintf_format(_T("  0007b"), _T("% *.*x"), 7, 5, 123);
    ok_sprintf_format(_T("  00000"), _T("%#*.*x"), 7, 5, 0);
    ok_sprintf_format(_T("0x0007b"), _T("%#*.*x"), 7, 5, 123);

    /* 'x' with different size prefixes (plus %d to validate pramaters are parsed correctly) */
    ok_sprintf_format(_T("5465,-1"),           _T("%hhx,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("5465,-1"),           _T("%hx,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("499602d2,-1"),      _T("%lx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"),      _T("%Lx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"),  _T("%llx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"),  _T("%jx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("499602d2,-1"),      _T("%wx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"),      _T("%I32x,%d"), 1234567890, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"),  _T("%I64x,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("b396ae70ad2,-1"),  _T("%tx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"),  _T("%zx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"),  _T("%Ix,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("499602d2,-1"),      _T("%tx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"),      _T("%zx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"),      _T("%Ix,%d"), 1234567890, -1);
#endif

    /* 'x' with multiple size prefixes */
    ok_sprintf_format(_T("5465,-1"), _T("%hhhhx,%d"), 0xFFFF0000 + 21605, -1); // hh + hh -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hhhx,%d"), 0xFFFF0000 + 21605, -1); // hh + h -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hhlx,%d"), 0xFFFF0000 + 21605, -1); // hh + l -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hhLx,%d"), 0xFFFF0000 + 21605, -1); // hh + L -> 16
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hhllx,%d"), 12341234567890LL, -1); // hh + ll -> 32
    ok_sprintf_format(_T("5465,-1"), _T("%hhLx,%d"), 0xFFFF0000 + 21605, -1); // hh + L -> 16
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hhjx,%d"), 12341234567890LL, -1); // hh + j -> 64
    ok_sprintf_format(_T("5465,-1"), _T("%hhwx,%d"), 0xFFFF0000 + 21605, -1); // hh + w -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hhI32x,%d"), 0xFFFF0000 + 21605, -1); // hh + I32 -> 16
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hhI64x,%d"), 12341234567890LL, -1); // hh + I64 -> 64
#ifdef _WIN64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hhtx,%d"), 12341234567890LL, -1); // hh + t -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hhzx,%d"), 12341234567890LL, -1); // hh + z -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hhIx,%d"), 12341234567890LL, -1); // hh + z -> 64
#else
    ok_sprintf_format(_T("5465,-1"), _T("%hhtx,%d"), 0xFFFF0000 + 21605, -1); // hh + t -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hhzx,%d"), 0xFFFF0000 + 21605, -1); // hh + z -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hhIx,%d"), 0xFFFF0000 + 21605, -1); // hh + z -> 16
#endif

    ok_sprintf_format(_T("5465,-1"), _T("%hhhx,%d"), 0xFFFF0000 + 21605, -1); // hh + hh + h -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hhx,%d"), 0xFFFF0000 + 21605, -1); // hh + hh + l -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hlx,%d"), 0xFFFF0000 + 21605, -1); // h + l -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hLx,%d"), 0xFFFF0000 + 21605, -1); // h + L -> 16
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hllx,%d"), 12341234567890LL, -1); // h + ll -> 64
    ok_sprintf_format(_T("5465,-1"), _T("%hLx,%d"), 0xFFFF0000 + 21605, -1); // h + L -> 16
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hjx,%d"), 12341234567890LL, -1); // h + j -> 64
    ok_sprintf_format(_T("5465,-1"), _T("%hwx,%d"), 0xFFFF0000 + 21605, -1); // h + w -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hI32x,%d"), 0xFFFF0000 + 21605, -1); // h + I32 -> 16
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hI64x,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%htx,%d"), 12341234567890LL, -1); // h + t -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hzx,%d"), 12341234567890LL, -1); // h + z -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%hIx,%d"), 12341234567890LL, -1); // h + z -> 64
#else
    ok_sprintf_format(_T("5465,-1"), _T("%htx,%d"), 0xFFFF0000 + 21605, -1); // h + t -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hzx,%d"), 0xFFFF0000 + 21605, -1); // h + z -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%hIx,%d"), 0xFFFF0000 + 21605, -1); // h + z -> 16
#endif

    ok_sprintf_format(_T("5465,-1"), _T("%lhhx,%d"), 0xFFFF0000 + 21605, -1); // l + hh -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%lhx,%d"), 0xFFFF0000 + 21605, -1); // l + h -> 16
    ok_sprintf_format(_T("499602d2,-1"), _T("%lLx,%d"), 1234567890, -1); // l + L -> 32
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%lllx,%d"), 12341234567890LL, -1); // l + ll -> 64 (actually ll + l)
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%ljx,%d"), 12341234567890LL, -1); // l + j -> 64
    ok_sprintf_format(_T("499602d2,-1"), _T("%lwx,%d"), 1234567890, -1); // l + w -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%lI32x,%d"), 1234567890, -1); // l + I32 -> 32
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%lI64x,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%ltx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%lzx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%lIx,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("499602d2,-1"), _T("%ltx,%d"), 1234567890, -1); // l + t -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%lzx,%d"), 1234567890, -1); // l + z -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%lIx,%d"), 1234567890, -1); // l + t -> 32
#endif

    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llhhx,%d"), 12341234567890LL, -1); // ll + hh -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llhx,%d"), 12341234567890LL, -1); // ll + h -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%lllx,%d"), 12341234567890LL, -1); // ll + l -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llLx,%d"), 12341234567890LL, -1); // ll + L -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llllx,%d"), 12341234567890LL, -1); // ll + ll -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%lljx,%d"), 12341234567890LL, -1); // ll + j -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llwx,%d"), 12341234567890LL, -1); // ll + w -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llI32x,%d"), 12341234567890LL, -1); // ll + I32 -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llI64x,%d"), 12341234567890LL, -1); // ll + I64 -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%lltx,%d"), 12341234567890LL, -1); // ll + t -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llzx,%d"), 12341234567890LL, -1); // ll + z -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%llIx,%d"), 12341234567890LL, -1); // ll + t -> 64

    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jhhx,%d"), 12341234567890LL, -1); // j + hh -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jhx,%d"), 12341234567890LL, -1); // j + h -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jlx,%d"), 12341234567890LL, -1); // j + l -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jLx,%d"), 12341234567890LL, -1); // j + L -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jllx,%d"), 12341234567890LL, -1); // j + ll -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jjx,%d"), 12341234567890LL, -1); // j + j -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jwx,%d"), 12341234567890LL, -1); // j + w -> 64
    ok_sprintf_format(_T("499602d2,-1"), _T("%jI32x,%d"), 1234567890, -1); // j + I32 -> 32
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jI64x,%d"), 12341234567890LL, -1); // j + I64 -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jtx,%d"), 12341234567890LL, -1); // j + t -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jzx,%d"), 12341234567890LL, -1); // j + z -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%jIx,%d"), 12341234567890LL, -1); // j + t -> 64

    ok_sprintf_format(_T("5465,-1"), _T("%whhx,%d"), 0xFFFF0000 + 21605, -1); // w + hh -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%whx,%d"), 0xFFFF0000 + 21605, -1); // w + h -> 16
    ok_sprintf_format(_T("499602d2,-1"), _T("%wlx,%d"), 1234567890, -1); // w + l -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%wLx,%d"), 1234567890, -1); // w + L -> 32
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%wllx,%d"), 12341234567890LL, -1); // w + ll -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%wjx,%d"), 12341234567890LL, -1); // w + j -> 64
    ok_sprintf_format(_T("499602d2,-1"), _T("%wwx,%d"), 1234567890, -1); // w + w -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%wI32x,%d"), 1234567890, -1); // w + I32 -> 32
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%wI64x,%d"), 12341234567890LL, -1); // w + I64 -> 64
    ok_sprintf_format(_T("499602d2,-1"), _T("%wtx,%d"), 1234567890, -1); // w + t -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%wzx,%d"), 1234567890, -1); // w + z -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%wIx,%d"), 1234567890, -1); // w + t -> 32

    ok_sprintf_format(_T("5465,-1"), _T("%I32hhx,%d"), 0xFFFF0000 + 21605, -1); // I32 + hh -> 16
    ok_sprintf_format(_T("5465,-1"), _T("%I32hx,%d"), 0xFFFF0000 + 21605, -1); // I32 + h -> 16
    ok_sprintf_format(_T("499602d2,-1"), _T("%I32lx,%d"), 1234567890, -1); // I32 + l -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%I32Lx,%d"), 1234567890, -1); // I32 + l -> 32
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I32llx,%d"), 12341234567890LL, -1); // I32 + ll -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I32jx,%d"), 12341234567890LL, -1); // I32 + j -> 64
    ok_sprintf_format(_T("499602d2,-1"), _T("%I32wx,%d"), 1234567890, -1); // I32 + w -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%I32I32x,%d"), 1234567890, -1); // I32 + I32 -> 32
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I32I64x,%d"), 12341234567890LL, -1); // I32 + I64 -> 64
    ok_sprintf_format(_T("499602d2,-1"), _T("%I32tx,%d"), 1234567890, -1); // I32 + t -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%I32zx,%d"), 1234567890, -1); // I32 + z -> 32
    ok_sprintf_format(_T("499602d2,-1"), _T("%I32Ix,%d"), 1234567890, -1); // I32 + t -> 32

    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64hhx,%d"), 12341234567890LL, -1); // I64 + hh -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64hx,%d"), 12341234567890LL, -1); // I64 + h -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64lx,%d"), 12341234567890LL, -1); // I64 + l -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64Lx,%d"), 12341234567890LL, -1); // I64 + L -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64llx,%d"), 12341234567890LL, -1); // I64 + ll -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64jx,%d"), 12341234567890LL, -1); // I64 + j -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64wx,%d"), 12341234567890LL, -1); // I64 + w -> 64
    ok_sprintf_format(_T("499602d2,-1"), _T("%I64I32x,%d"), 1234567890, -1); // I64 + I32 -> 32
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64I64x,%d"), 12341234567890LL, -1); // I64 + I64 -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64tx,%d"), 12341234567890LL, -1); // I64 + t -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64zx,%d"), 12341234567890LL, -1); // I64 + z -> 64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%I64Ix,%d"), 12341234567890LL, -1); // I64 + t -> 64

    /* 'F' and 'N' size prefixes (far/near), that are ignored */
    ok_sprintf_format(_T("5465,-1"), _T("%Fhhx,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("5465,-1"), _T("%Fhx,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Flx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Fllx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Fjx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Fwx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%FI32x,%d"), 1234567890, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%FI64x,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Ftx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Fzx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%FIx,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("499602d2,-1"), _T("%Ftx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Fzx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%FIx,%d"), 1234567890, -1);
#endif

    ok_sprintf_format(_T("5465,-1"), _T("%Nhhx,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("5465,-1"), _T("%Nhx,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Nlx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Nllx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Njx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Nwx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%NI32x,%d"), 1234567890, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%NI64x,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Ntx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Nzx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%NIx,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("499602d2,-1"), _T("%Ntx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Nzx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%NIx,%d"), 1234567890, -1);
#endif

    /* 'L' (long double) is also ignored */
    ok_sprintf_format(_T("5465,-1"), _T("%Lhhx,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("5465,-1"), _T("%Lhx,%d"), 0xFFFF0000 + 21605, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Llx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Lllx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Ljx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Lwx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%LI32x,%d"), 1234567890, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%LI64x,%d"), 12341234567890LL, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Ltx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%Lzx,%d"), 12341234567890LL, -1);
    ok_sprintf_format(_T("b396ae70ad2,-1"), _T("%LIx,%d"), 12341234567890LL, -1);
#else
    ok_sprintf_format(_T("499602d2,-1"), _T("%Ltx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%Lzx,%d"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2,-1"), _T("%LIx,%d"), 1234567890, -1);
#endif

    /* 't', 'z' and 'I' are special and cannot be followed by another size prefix */
    ok_sprintf_format(_T("thhx,-1"), _T("%thhx,%d"), -1);
    ok_sprintf_format(_T("thx,-1"), _T("%thx,%d"), -1);
    ok_sprintf_format(_T("tlx,-1"), _T("%tlx,%d"), -1);
    ok_sprintf_format(_T("tllx,-1"), _T("%tllx,%d"), -1);
    ok_sprintf_format(_T("tjx,-1"), _T("%tjx,%d"), -1);
    ok_sprintf_format(_T("twx,-1"), _T("%twx,%d"), -1);
    ok_sprintf_format(_T("tzx,-1"), _T("%tzx,%d"), -1);
    ok_sprintf_format(_T("ttx,-1"), _T("%ttx,%d"), -1);
    ok_sprintf_format(_T("tI32x,-1"), _T("%tI32x,%d"), -1);
    ok_sprintf_format(_T("tI64x,-1"), _T("%tI64x,%d"), -1);

    ok_sprintf_format(_T("zhhx,-1"), _T("%zhhx,%d"), -1);
    ok_sprintf_format(_T("zhx,-1"), _T("%zhx,%d"), -1);
    ok_sprintf_format(_T("zlx,-1"), _T("%zlx,%d"), -1);
    ok_sprintf_format(_T("zllx,-1"), _T("%zllx,%d"), -1);
    ok_sprintf_format(_T("zjx,-1"), _T("%zjx,%d"), -1);
    ok_sprintf_format(_T("zwx,-1"), _T("%zwx,%d"), -1);
    ok_sprintf_format(_T("zzx,-1"), _T("%zzx,%d"), -1);
    ok_sprintf_format(_T("ztx,-1"), _T("%ztx,%d"), -1);
    ok_sprintf_format(_T("zI32x,-1"), _T("%zI32x,%d"), -1);
    ok_sprintf_format(_T("zI64x,-1"), _T("%zI64x,%d"), -1);

    ok_sprintf_format(_T("Ihhx,-1"), _T("%Ihhx,%d"), -1);
    ok_sprintf_format(_T("Ihx,-1"), _T("%Ihx,%d"), -1);
    ok_sprintf_format(_T("Ilx,-1"), _T("%Ilx,%d"), -1);
    ok_sprintf_format(_T("Illx,-1"), _T("%Illx,%d"), -1);
    ok_sprintf_format(_T("Ijx,-1"), _T("%Ijx,%d"), -1);
    ok_sprintf_format(_T("Iwx,-1"), _T("%Iwx,%d"), -1);
    ok_sprintf_format(_T("Izx,-1"), _T("%Izx,%d"), -1);
    ok_sprintf_format(_T("Itx,-1"), _T("%Itx,%d"), -1);
    ok_sprintf_format(_T("II32x,-1"), _T("%II32x,%d"), -1);
    ok_sprintf_format(_T("II64x,-1"), _T("%II64x,%d"), -1);

    /* 'x' adjacent to %% */
    ok_sprintf_format(_T("%499602d2"), _T("%%%x"), 1234567890, -1);
    ok_sprintf_format(_T("499602d2%"), _T("%x%%"), 1234567890, -1);

    /* 'x' with misc modifiers */
    ok_sprintf_format(_T("7b"), _T("%+.*x"), 0, 123);
    ok_sprintf_format(_T("007b"), _T("%+0*x"), 4, 123);
    ok_sprintf_format(_T("7b  "), _T("%+0*x"), -4, 123);
    ok_sprintf_format(_T("7b"), _T("%+0*x"), 0, 123);
    ok_sprintf_format(_T("7b"), _T("%+0*x"), 1, 123);
    ok_sprintf_format(_T("7b"), _T("%+0*x"), -1, 123);
    ok_sprintf_format(_T("00007b"), _T("%0+6x"), 123);
    ok_sprintf_format(_T("00007b"), _T("%+06x"), 123);
    ok_sprintf_format(_T("zhx,-1"), _T("%hzhx,%d"), -1);
    ok_sprintf_format(_T("thx,-1"), _T("%hthx,%d"), -1);
    ok_sprintf_format(_T("7b,-1"), _T("%hI32hx,%d"), (size_t)123, -1);
    ok_sprintf_format(_T("7b,-1"), _T("%hhhjlllx,%d"), (int64_t)123, -1);
}

/***********************************************
 * 'X' - Uppercase hexadecimal number          *
 ***********************************************/
static void Test_printf_format_X(void)
{
    ok_sprintf_format(_T("DEADBE0F"), _T("%X"), 0xDEADBE0F);
    ok_sprintf_format(_T("215241F1"), _T("%X"), -(int)0xDEADBE0F);
    ok_sprintf_format(_T("BEEF    "), _T("%-8X"), 0xBEEF);
    ok_sprintf_format(_T("0XDEADBE0F"), _T("%#X"), 0xDEADBE0F);
    
}

/***********************************************
 * 'p' - Pointer                               *
 ***********************************************/
static void Test_printf_format_p(void)
{
#ifdef _WIN64
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%p"), NULL);
#else
    ok_sprintf_format(_T("DEADBEEF"), _T("%p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%p"), NULL);
#endif

    /* 'p' with flags */
#ifdef _WIN64
    ok_sprintf_format(_T("0000000000000000"),   _T("%-p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"),   _T("%-p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"),   _T("%+p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"),   _T("%+p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"),   _T("%0p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"),   _T("%0p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"),   _T("% p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"),   _T("% p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"),   _T("%#p"), 0LL);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF"), _T("%#p"), 0xCAFEBABEDEADBEEFull);
#else
    ok_sprintf_format(_T("00000000"),   _T("%-p"), 0);
    ok_sprintf_format(_T("DEADBEEF"),   _T("%-p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"),   _T("%+p"), 0);
    ok_sprintf_format(_T("DEADBEEF"),   _T("%+p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"),   _T("%0p"), 0);
    ok_sprintf_format(_T("DEADBEEF"),   _T("%0p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"),   _T("% p"), 0);
    ok_sprintf_format(_T("DEADBEEF"),   _T("% p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"),   _T("%#p"), 0);
    ok_sprintf_format(_T("0XDEADBEEF"), _T("%#p"), 0xDEADBEEF);
#endif

    /* 'x' with flag combinations */
#ifdef _WIN64
    ok_sprintf_format(_T("0000000000000000"), _T("%+-p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%+-p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%-+p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%-+p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%+0p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%+0p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%#0p"), 0LL);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF"), _T("%#0p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%# p"), 0LL);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF"), _T("%# p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%#+p"), 0LL);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF"), _T("%#+p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%+#p"), 0LL);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF"), _T("%+#p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%0+p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%0+p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("%+ p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%+ p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000"), _T("% +p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("% +p"), 0xCAFEBABEDEADBEEFull);
#else
    ok_sprintf_format(_T("00000000"), _T("%+-p"), 0);
    ok_sprintf_format(_T("DEADBEEF"), _T("%+-p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%-+p"), 0);
    ok_sprintf_format(_T("DEADBEEF"), _T("%-+p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%+0p"), 0);
    ok_sprintf_format(_T("DEADBEEF"), _T("%+0p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%#0p"), 0);
    ok_sprintf_format(_T("0XDEADBEEF"), _T("%#0p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%# p"), 0);
    ok_sprintf_format(_T("0XDEADBEEF"), _T("%# p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%#+p"), 0);
    ok_sprintf_format(_T("0XDEADBEEF"), _T("%#+p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%+#p"), 0);
    ok_sprintf_format(_T("0XDEADBEEF"), _T("%+#p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%0+p"), 0);
    ok_sprintf_format(_T("DEADBEEF"), _T("%0+p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("%+ p"), 0);
    ok_sprintf_format(_T("DEADBEEF"), _T("%+ p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000"), _T("% +p"), 0);
    ok_sprintf_format(_T("DEADBEEF"), _T("% +p"), 0xDEADBEEF);
#endif

    /* 'p' with flags and width */
#ifdef _WIN64
    ok_sprintf_format(_T("   0000000000000000"), _T("%19p"), 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%19p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000   "), _T("%-19p"), 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF   "), _T("%-19p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%+19p"), 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%+19p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%019p"), 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%019p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%+019p"), 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%+019p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("% 19p"), 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("% 19p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%#19p"), 0LL);
    ok_sprintf_format(_T(" 0XCAFEBABEDEADBEEF"), _T("%#19p"), 0xCAFEBABEDEADBEEFull);

    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%+019p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000   "), _T("%-+0 #19p"), 0LL);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF "), _T("%-+0 #19p"), 0xCAFEBABEDEADBEEFull);
#else
    ok_sprintf_format(_T("   00000000"), _T("%11p"), 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("%11p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000   "), _T("%-11p"), 0);
    ok_sprintf_format(_T("DEADBEEF   "), _T("%-11p"), 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("%+11p"), 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("%+11p"), 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("%011p"), 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("%011p"), 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("%+011p"), 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("%+011p"), 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("% 11p"), 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("% 11p"), 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("%#11p"), 0);
    ok_sprintf_format(_T(" 0XDEADBEEF"), _T("%#11p"), 0xDEADBEEF);

    ok_sprintf_format(_T("   DEADBEEF"), _T("%+011p"), 0xDEADBEEF);
    ok_sprintf_format(_T("00000000   "), _T("%-+0 #11p"), 0);
    ok_sprintf_format(_T("0XDEADBEEF "), _T("%-+0 #11p"), 0xDEADBEEF);
#endif

    /* 'p' with flags and indirect width ('*') */
#ifdef _WIN64
    ok_sprintf_format(_T("   0000000000000000"), _T("%*p"), 19, 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%*p"), 19, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000   "), _T("%-*p"), 19, 0LL);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF   "), _T("%-*p"), 19, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%+*p"), 19, 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%+*p"), 19, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%0*p"), 19, 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%0*p"), 19, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%+0*p"), 19, 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%+0*p"), 19, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("% *p"), 19, 0LL);
    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("% *p"), 19, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%#*p"), 19, 0LL);
    ok_sprintf_format(_T(" 0XCAFEBABEDEADBEEF"), _T("%#*p"), 19, 0xCAFEBABEDEADBEEFull);

    ok_sprintf_format(_T("   CAFEBABEDEADBEEF"), _T("%+0*p"), 19, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0000000000000000   "), _T("%-+0 #*p"), 19, 0LL);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF "), _T("%-+0 #*p"), 19, 0xCAFEBABEDEADBEEFull);
#else
    ok_sprintf_format(_T("   00000000"), _T("%*p"), 11, 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("%*p"), 11, 0xDEADBEEF);
    ok_sprintf_format(_T("00000000   "), _T("%-*p"), 11, 0);
    ok_sprintf_format(_T("DEADBEEF   "), _T("%-*p"), 11, 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("%+*p"), 11, 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("%+*p"), 11, 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("%0*p"), 11, 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("%0*p"), 11, 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("%+0*p"), 11, 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("%+0*p"), 11, 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("% *p"), 11, 0);
    ok_sprintf_format(_T("   DEADBEEF"), _T("% *p"), 11, 0xDEADBEEF);
    ok_sprintf_format(_T("   00000000"), _T("%#*p"), 11, 0);
    ok_sprintf_format(_T(" 0XDEADBEEF"), _T("%#*p"), 11, 0xDEADBEEF);

    ok_sprintf_format(_T("   DEADBEEF"), _T("%+0*p"), 11, 0xDEADBEEF);
    ok_sprintf_format(_T("00000000   "), _T("%-+0 #*p"), 11, 0);
    ok_sprintf_format(_T("0XDEADBEEF "), _T("%-+0 #*p"), 11, 0xDEADBEEF);
#endif

    /* 'x' with flags and indirect width of 0 */
#ifdef _WIN64
#else
#endif

    /* 'p' with negative indirect width (acts as left-justified) */
#ifdef _WIN64
#else
#endif

    /* 'p' with flags, indirect width and width digit (*1 + "9" = 19) */
#ifdef _WIN64
#else
#endif

    /* 'p' with negative indirect width and width digit (*-1 + "2" = -12) */
#ifdef _WIN64
#else
#endif

    /* 'p' with invalid indirect width use */
#ifdef _WIN64
#else
#endif

    /* 'x' with flags and precision */
#ifdef _WIN64
#else
#endif

    /* 'x' with flags and small precision */
#ifdef _WIN64
#else
#endif

    /* 'x' with flags, indirect precision and precision digit */
#ifdef _WIN64
#else
#endif

    /* 'x' with flags and precision 0 */
#ifdef _WIN64
#else
#endif

    /* '-' sign in precision is invalid */
#ifdef _WIN64
#else
#endif

    /* Negative precision is ignored */
#ifdef _WIN64
#else
#endif

    /* 'x' with width and precision */
#ifdef _WIN64
    ok_sprintf_format(_T("   0000000000000000"), _T("%19.9p"), 0LL);
    ok_sprintf_format(_T("   0000DEAD0000BEEF"), _T("%19.9p"), 0x0000DEAD0000BEEFull);
    ok_sprintf_format(_T("0000000000000000   "), _T("%-19.9p"), 0LL);
    ok_sprintf_format(_T("0000DEAD0000BEEF   "), _T("%-19.9p"), 0x0000DEAD0000BEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%+19.9p"), 0LL);
    ok_sprintf_format(_T("   0000DEAD0000BEEF"), _T("%+19.9p"), 0x0000DEAD0000BEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%019.9p"), 0LL);
    ok_sprintf_format(_T("   0000DEAD0000BEEF"), _T("%019.9p"), 0x0000DEAD0000BEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%+019.9p"), 0LL);
    ok_sprintf_format(_T("   0000DEAD0000BEEF"), _T("%+019.9p"), 0x0000DEAD0000BEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("% 19.9p"), 0LL);
    ok_sprintf_format(_T("   0000DEAD0000BEEF"), _T("% 19.9p"), 0x0000DEAD0000BEEFull);
    ok_sprintf_format(_T("   0000000000000000"), _T("%#19.9p"), 0LL);
    ok_sprintf_format(_T(" 0X0000DEAD0000BEEF"), _T("%#19.9p"), 0x0000DEAD0000BEEFull);

    ok_sprintf_format(_T("    0000DEAD0000BEEF"), _T("%20.18llp"), 0x0000DEAD0000BEEFull);
    ok_sprintf_format(_T("    0000DEAD0000BEEF"), _T("%20.18jp"), 0x0000DEAD0000BEEFull);
#else
    ok_sprintf_format(_T("   00000000"), _T("%11.9p"), 0);
    ok_sprintf_format(_T("   0000BEEF"), _T("%11.9p"), 0x0000BEEF);
    ok_sprintf_format(_T("00000000   "), _T("%-11.9p"), 0);
    ok_sprintf_format(_T("0000BEEF   "), _T("%-11.9p"), 0x0000BEEF);
    ok_sprintf_format(_T("   00000000"), _T("%+11.9p"), 0);
    ok_sprintf_format(_T("   0000BEEF"), _T("%+11.9p"), 0x0000BEEF);
    ok_sprintf_format(_T("   00000000"), _T("%011.9p"), 0);
    ok_sprintf_format(_T("   0000BEEF"), _T("%011.9p"), 0x0000BEEF);
    ok_sprintf_format(_T("   00000000"), _T("%+011.9p"), 0);
    ok_sprintf_format(_T("   0000BEEF"), _T("%+011.9p"), 0x0000BEEF);
    ok_sprintf_format(_T("   00000000"), _T("% 11.9p"), 0);
    ok_sprintf_format(_T("   0000BEEF"), _T("% 11.9p"), 0x0000BEEF);
    ok_sprintf_format(_T("   00000000"), _T("%#11.9p"), 0);
    ok_sprintf_format(_T(" 0X0000BEEF"), _T("%#11.9p"), 0x0000BEEF);

    ok_sprintf_format(_T("        DEAD0000BEEF"), _T("%20.18llp"), 0x0000DEAD0000BEEFull);
    ok_sprintf_format(_T("        DEAD0000BEEF"), _T("%20.18jp"), 0x0000DEAD0000BEEFull);
#endif

    /* 'p' with indirect width and indirect precision */
#ifdef _WIN64
#else
#endif

    /* 'p' with different size prefixes (plus %d to validate pramaters are parsed correctly) */
#ifdef _WIN64
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%hhp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%hp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%lp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%Lp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%llp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%jp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%wp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%I32p,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%I64p,%d"), 0x0000DEAD0000BEEFull, -1);
#else
    ok_sprintf_format(_T("0000BEEF,-1"),         _T("%hhp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("0000BEEF,-1"),         _T("%hp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEADBEEF,-1"),         _T("%lp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEADBEEF,-1"),         _T("%Lp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEAD0000BEEF,-1"),     _T("%llp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("DEAD0000BEEF,-1"),     _T("%jp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("DEADBEEF,-1"),         _T("%wp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEADBEEF,-1"),         _T("%I32p,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEAD0000BEEF,-1"),     _T("%I64p,%d"), 0x0000DEAD0000BEEFull, -1);
#endif

    /* 'p' with invalid size prefixes */
    ok_sprintf_format(_T("tp,-1"), _T("%tp,%d"), -1);
    ok_sprintf_format(_T("zp,-1"), _T("%zp,%d"), -1);
    ok_sprintf_format(_T("Ip,-1"), _T("%Ip,%d"), -1);

    /* 'p' with multiple size prefixes */
#ifdef _WIN64
#else
    ok_sprintf_format(_T("CAFEBABEDEADBEEF,-1"), _T("%lllp,%d"), 0xCAFEBABEDEADBEEFull, -1);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF,-1"), _T("%I64lp,%d"), 0xCAFEBABEDEADBEEFull, -1);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF,-1"), _T("%hhhjlllp,%d"), 0xCAFEBABEDEADBEEFull, -1);


#endif

    /* 'p' adjacent to %% */
#ifdef _WIN64
    ok_sprintf_format(_T("%CAFEBABEDEADBEEF"), _T("%%%p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF%"), _T("%p%%"), 0xCAFEBABEDEADBEEFull);
#else
    ok_sprintf_format(_T("%DEADBEEF"), _T("%%%p"), 0xDEADBEEF);
    ok_sprintf_format(_T("DEADBEEF%"), _T("%p%%"), 0xDEADBEEF);
#endif

    /* 'p' with misc modifiers */
#ifdef _WIN64
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%+.*p"), 0, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("  CAFEBABEDEADBEEF"), _T("%+0*p"), 18, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF  "), _T("%+0*p"), -18, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%+0*p"), 0, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%+0*p"), 1, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%+0*p"), -1, 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("  CAFEBABEDEADBEEF"), _T("%0+18p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("  CAFEBABEDEADBEEF"), _T("%+018p"), 0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF,-1"), _T("%hI32hp,%d"), 0xCAFEBABEDEADBEEFull, -1);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF,-1"), _T("%hhhjlllp,%d"), 0xCAFEBABEDEADBEEFull, -1);
#else
    ok_sprintf_format(_T("DEADBEEF"), _T("%+.*p"), 0, 0xDEADBEEF);
    ok_sprintf_format(_T("  DEADBEEF"), _T("%+0*p"), 10, 0xDEADBEEF);
    ok_sprintf_format(_T("DEADBEEF  "), _T("%+0*p"), -10, 0xDEADBEEF);
    ok_sprintf_format(_T("DEADBEEF"), _T("%+0*p"), 0, 0xDEADBEEF);
    ok_sprintf_format(_T("DEADBEEF"), _T("%+0*p"), 1, 0xDEADBEEF);
    ok_sprintf_format(_T("DEADBEEF"), _T("%+0*p"), -1, 0xDEADBEEF);
    ok_sprintf_format(_T("  DEADBEEF"), _T("%0+10p"), 0xDEADBEEF);
    ok_sprintf_format(_T("  DEADBEEF"), _T("%+010p"), 0xDEADBEEF);
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%hI32hp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF,-1"), _T("%hhhjlllp,%d"), 0xCAFEBABEDEADBEEFull, -1);
#endif
    ok_sprintf_format(_T("zhp,-1"), _T("%hzhp,%d"), -1);
    ok_sprintf_format(_T("thp,-1"), _T("%hthp,%d"), -1);
 
//-----
    /* 'p' with precision (ignored for %c) */
    /* 'p' with width of 1 (no padding expected) */
    /* 'p' adjacent to %% */

    /* 'p' - Pointer */
#ifdef _WIN64
    ok_sprintf_format(_T("0000000000001234"), _T("%p"), (void*)(uintptr_t)0x1234);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF"), _T("%08p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T(" CAFEBABEDEADBEEF"), _T("%17p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF "), _T("%-17p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T(" CAFEBABEDEADBEEF"), _T("%017p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF"), _T("%#p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF  "), _T("%#0-20p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("  0XCAFEBABEDEADBEEF"), _T("%#020p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
#else
    ok_sprintf_format(_T("00001234"), _T("%p"), (void*)0x1234);
    ok_sprintf_format(_T("DEADBEEF"), _T("%4p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T(" DEADBEEF"), _T("%9p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("DEADBEEF "), _T("%-9p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T(" DEADBEEF"), _T("%09p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("0XDEADBEEF"), _T("%#p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("0XDEADBEEF  "), _T("%#0-12p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("  0XDEADBEEF"), _T("%#012p"), (void*)0xDEADBEEF);
#endif

    /* 'p' - All flag/width/precision/invalid combinations */
#ifdef _WIN64
    ok_sprintf_format(_T("  CAFEBABEDEADBEEF"), _T("%18p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF  "), _T("%-18p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF"), _T("%#p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("0XCAFEBABEDEADBEEF  "), _T("%#-20p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
    ok_sprintf_format(_T("  0XCAFEBABEDEADBEEF"), _T("%#20p"), (void*)(uintptr_t)0xCAFEBABEDEADBEEFull);
#else
    ok_sprintf_format(_T("  DEADBEEF"), _T("%10p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("DEADBEEF  "), _T("%-10p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("0XDEADBEEF"), _T("%#p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("0XDEADBEEF  "), _T("%#-12p"), (void*)0xDEADBEEF);
    ok_sprintf_format(_T("  0XDEADBEEF"), _T("%#12p"), (void*)0xDEADBEEF);

    /* 'p' with different size prefixes */
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%hp,%d"), (void*)0xDEADBEEF, -1);
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%hhp,%d"), (void*)0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEADBEEF,-1"), _T("%lp,%d"), (void*)0xDEADBEEF, -1);
    ok_sprintf_format(_T("CAFEBABEDEADBEEF,-1"), _T("%llp,%d"), (uint64_t)0xCAFEBABEDEADBEEFull, -1);
    ok_sprintf_format(_T("FEBABEDEADBEEF,-1"), _T("%llp,%d"), (uint64_t)0x00FEBABEDEADBEEFull, -1);
    ok_sprintf_format(_T("FEBABEDEADBEEF,-1"), _T("%jp,%d"), (uintmax_t)0x00FEBABEDEADBEEFull, -1);
    ok_sprintf_format(_T("DEADBEEF,-1"), _T("%wp,%d"), (void*)0xDEADBEEF, -1);

    /* 'p' with invalid size prefixes */
    ok_sprintf_format(_T("tp,-1"), _T("%tp,%d"), -1);
    ok_sprintf_format(_T("zp,-1"), _T("%zp,%d"), -1);
#endif
}

/***********************************************
 * 's' - String                                *
 ***********************************************/
static void Test_printf_format_s(void)
{
    ok_sprintf_format(_T("Hello"), _T("%s"), _T("Hello"));

    /* 's' with flags */
    ok_sprintf_format(_T("Hello"), _T("%-s"), _T("Hello"));
    ok_sprintf_format(_T("Hello"), _T("%+s"), _T("Hello"));
    ok_sprintf_format(_T("Hello"), _T("%0s"), _T("Hello"));
    ok_sprintf_format(_T("Hello"), _T("% s"), _T("Hello"));
    ok_sprintf_format(_T("Hello"), _T("%#s"), _T("Hello"));

    /* 's' with flags and width */
    ok_sprintf_format(_T("  Hello"), _T("%7s"), _T("Hello"));
    ok_sprintf_format(_T("Hello  "), _T("%-7s"), _T("Hello"));
    ok_sprintf_format(_T("  Hello"), _T("%+7s"), _T("Hello"));
    ok_sprintf_format(_T("00Hello"), _T("%07s"), _T("Hello"));
    ok_sprintf_format(_T("00Hello"), _T("%+07s"), _T("Hello"));
    ok_sprintf_format(_T("  Hello"), _T("% 7s"), _T("Hello"));
    ok_sprintf_format(_T("  Hello"), _T("%#7s"), _T("Hello"));
    ok_sprintf_format(_T("Hello  "), _T("%-+0 #7s"), _T("Hello"));

    /* 's with flags and indirect width ('*') */
    /* 's' with negative indirect width (acts as left-justified) */
    /* 's' with flags, indirect width and width digit (*1 + "2" = 12) */
    /* 's' with negative indirect width and width digit (*-1 + "2" = -12) */
    /* 's' with invalid indirect width use */

    /* 's' with different size prefixes (plus %u to validate pramaters are parsed correctly) */
    ok_sprintf_format(_T("Hello"), _T("%hs"), "Hello");
    ok_sprintf_format(_T("Hello"), _T("%ws"), L"Hello");
#ifdef USE_WIDECHAR
    ok_sprintf_format(_T("Hello"), _T("%hws"), "Hello");
    ok_sprintf_format(_T("Hello"), _T("%whs"), "Hello");
#else
    ok_sprintf_format(_T("Hello"), _T("%hws"), L"Hello");
    ok_sprintf_format(_T("Hello"), _T("%whs"), L"Hello");
#endif

    /* 's' with invalid size prefixes */

    /* 's' with precision */
    ok_sprintf_format(_T(""), _T("%.s"), _T("Hello"));
    ok_sprintf_format(_T(""), _T("%.0s"), _T("Hello"));
    ok_sprintf_format(_T("Hel"), _T("%.3s"), _T("Hello"));
    ok_sprintf_format(_T("Hello"), _T("%.7s"), _T("Hello"));
    ok_sprintf_format(_T("Hel"), _T("%.*s"), 3, _T("Hello"));
    ok_sprintf_format(_T("Hell"), _T("%3.4s"), _T("Hello"));
    ok_sprintf_format(_T("    Hel"), _T("%*.*s"), 7, 3, _T("Hello"));
    ok_sprintf_format(_T("  Hello"), _T("%*.*0s"), 7, 1, _T("Hello"));

    /* 's' with width of 1 (no padding expected) */
    /* 's' adjacent to %% */

    /* 's' and 'S' - String */
    ok_sprintf_format(_T("   Hello"), _T("%8s"), _T("Hello"));
    ok_sprintf_format(_T("Hello   "), _T("%-8s"), _T("Hello"));
    ok_sprintf_format(_T("Hel"), _T("%.3s"), _T("Hello"));
    ok_sprintf_format(_T("   Hel"), _T("%6.3s"), _T("Hello"));
    ok_sprintf_format(_T(""), _T("%.0s"), _T("Hello"));
    ok_sprintf_format(_T(""), _T("%s"), _T(""));
    ok_sprintf_format(_T("(null)"), _T("%s"), NULL);
    ok_sprintf_format(_T("(n"), _T("%.2s"), NULL);

    /* 's' and 'S' - All flag/width/precision/invalid combinations */
    ok_sprintf_format(_T("     Hello"), _T("%10.10s"), _T("Hello"));
    ok_sprintf_format(_T("     Hello"), _T("%10.5s"), _T("Hello"));
    ok_sprintf_format(_T("Hello     "), _T("%-10.5s"), _T("Hello"));
    ok_sprintf_format(_T("Hel"), _T("%.3s"), _T("Hello"));
    ok_sprintf_format(_T("   Hel"), _T("%6.3s"), _T("Hello"));
    ok_sprintf_format(_T(""), _T("%.0s"), _T("Hello"));
}

/***********************************************
 * 'S' - String                                *
 ***********************************************/
static void Test_printf_format_S(void)
{
    /* 'S' with flags */
    /* 'S' with flags and width */
    /* 'S' with flags and indirect width ('*') */
    /* 'S' with negative indirect width (acts as left-justified) */
    /* 'S' with flags, indirect width and width digit (*1 + "2" = 12) */
    /* 'S' with negative indirect width and width digit (*-1 + "2" = -12) */
    /* 'S' with invalid indirect width use */
    /* 'S' with different size prefixes (plus %u to validate pramaters are parsed correctly) */
    /* 'S' with invalid size prefixes */
    /* 'S' with precision (ignored for %c) */
    /* 'S' with width of 1 (no padding expected) */
    /* 'S' adjacent to %% */

#ifdef USE_WIDECHAR
    ok_sprintf_format(_T("Hello"), _T("%S"), "Hello");
    ok_sprintf_format(_T("   Hello"), _T("%8S"), "Hello");
    ok_sprintf_format(_T("Hel"), _T("%.3S"), "Hello");
    ok_sprintf_format(_T(""), _T("%.0S"), "Hello");
    ok_sprintf_format(_T("(null)"), _T("%S"), NULL);
    ok_sprintf_format(_T("   Hel"), _T("%6.3S"), "Hello");
#else
    ok_sprintf_format(_T("Hello"), _T("%S"), L"Hello");
    ok_sprintf_format(_T("   Hello"), _T("%8S"), L"Hello");
    ok_sprintf_format(_T("Hel"), _T("%.3S"), L"Hello");
    ok_sprintf_format(_T(""), _T("%.0S"), L"Hello");
    ok_sprintf_format(_T("(null)"), _T("%S"), NULL);
    ok_sprintf_format(_T("   Hel"), _T("%6.3S"), L"Hello");
#endif
}

/***********************************************
 * 'Z' - Counted string                        *
 ***********************************************/
static void Test_printf_format_Z(void)
{
    ANSI_STRING astr = { 5, 6, "Hello" };
    ANSI_STRING astr_empty = { 0, 1, "" };
    ANSI_STRING astr_null = { 1, 1, NULL };

    ok_sprintf_format(_T("Hello"), _T("%Z"), &astr);
    ok_sprintf_format(_T(""), _T("%Z"), &astr_empty);
    ok_sprintf_format(_T("(null)"), _T("%Z"), NULL);
    ok_sprintf_format(_T("(null)"), _T("%Z"), &astr_null);

    /* 'Z' with flags */
    ok_sprintf_format(_T("Hello"), _T("%-Z"), &astr);
    ok_sprintf_format(_T("Hello"), _T("%+Z"), &astr);
    ok_sprintf_format(_T("Hello"), _T("%0Z"), &astr);
    ok_sprintf_format(_T("Hello"), _T("% Z"), &astr);
    ok_sprintf_format(_T("Hello"), _T("%#Z"), &astr);
    ok_sprintf_format(_T(""), _T("%#Z"), &astr_empty);
    ok_sprintf_format(_T("(null)"), _T("%#Z"), NULL);

    /* 'Z' with flags and width */
    ok_sprintf_format(_T("  Hello"), _T("%7Z"), &astr);
    ok_sprintf_format(_T("Hello  "), _T("%-7Z"), &astr);
    ok_sprintf_format(_T("  Hello"), _T("%+7Z"), &astr);
    ok_sprintf_format(_T("00Hello"), _T("%07Z"), &astr);
    ok_sprintf_format(_T("00Hello"), _T("%+07Z"), &astr);
    ok_sprintf_format(_T("  Hello"), _T("% 7Z"), &astr);
    ok_sprintf_format(_T("  Hello"), _T("%#7Z"), &astr);
    ok_sprintf_format(_T("Hello  "), _T("%-+0 #7Z"), &astr);

    ok_sprintf_format(_T(" (null)"), _T("%7Z"), &astr_null);
    ok_sprintf_format(_T("(null) "), _T("%-7Z"), &astr_null);
    ok_sprintf_format(_T(" (null)"), _T("%+7Z"), &astr_null);
    ok_sprintf_format(_T("0(null)"), _T("%07Z"), &astr_null);
    ok_sprintf_format(_T("0(null)"), _T("%+07Z"), &astr_null);
    ok_sprintf_format(_T(" (null)"), _T("% 7Z"), &astr_null);
    ok_sprintf_format(_T(" (null)"), _T("%#7Z"), &astr_null);
    ok_sprintf_format(_T("(null) "), _T("% -+0 #7Z"), &astr_null);

    /* 'Z' with flags and indirect width ('*') */
    /* 'Z' with negative indirect width (acts as left-justified) */
    /* 'Z' with flags, indirect width and width digit (*1 + "2" = 12) */
    /* 'Z' with negative indirect width and width digit (*-1 + "2" = -12) */
    /* 'Z' with invalid indirect width use */
    /* 'Z' with different size prefixes (plus %u to validate pramaters are parsed correctly) */
    /* 'Z' with invalid size prefixes */
    /* 'Z' with precision (ignored for %c) */
    /* 'Z' with width of 1 (no padding expected) */
    /* 'Z' adjacent to %% */

    /* 'Z' - Counted string */

    /* 'Z' - All flag/width/precision/invalid combinations */
    ok_sprintf_format(_T("   Hello"), _T("%8Z"), &astr);
    ok_sprintf_format(_T("Hello   "), _T("%-8Z"), &astr);
    ok_sprintf_format(_T("Hello"), _T("%.3Z"), &astr);
    ok_sprintf_format(_T(" Hello"), _T("%6.3Z"), &astr);
    ok_sprintf_format(_T("Hello"), _T("%.0Z"), &astr);
}

/***********************************************
 * 'wZ' - Counted unicode string               *
 ***********************************************/
static void Test_printf_format_wZ(void)
{
    UNICODE_STRING ustr = { 5 * sizeof(wchar_t), 6 * sizeof(wchar_t), L"Hello" };
    UNICODE_STRING ustr_empty = { 0, 1, L"" };

    ok_sprintf_format(_T("Hello"), _T("%wZ"), &ustr);
    ok_sprintf_format(_T(""), _T("%wZ"), &ustr_empty);
    ok_sprintf_format(_T("(null)"), _T("%wZ"), NULL);

    /* 'wZ' with flags */
    ok_sprintf_format(_T("Hello"), _T("%-wZ"), &ustr);
    ok_sprintf_format(_T("Hello"), _T("%+wZ"), &ustr);
    ok_sprintf_format(_T("Hello"), _T("%0wZ"), &ustr);
    ok_sprintf_format(_T("Hello"), _T("% wZ"), &ustr);
    ok_sprintf_format(_T("Hello"), _T("%#wZ"), &ustr);
    ok_sprintf_format(_T(""), _T("%#wZ"), &ustr_empty);
    ok_sprintf_format(_T("(null)"), _T("%#wZ"), NULL);

    /* 'wZ' with flags and width */
    ok_sprintf_format(_T("  Hello"), _T("%7wZ"), &ustr);
    ok_sprintf_format(_T("Hello  "), _T("%-7wZ"), &ustr);
    ok_sprintf_format(_T("  Hello"), _T("%+7wZ"), &ustr);
    ok_sprintf_format(_T("00Hello"), _T("%07wZ"), &ustr);
    ok_sprintf_format(_T("00Hello"), _T("%+07wZ"), &ustr);
    ok_sprintf_format(_T("  Hello"), _T("% 7wZ"), &ustr);
    ok_sprintf_format(_T("  Hello"), _T("%#7wZ"), &ustr);
    ok_sprintf_format(_T("Hello  "), _T("%-+0 #7wZ"), &ustr);

    /* 'wZ' with flags and indirect width ('*') */
    /* 'wZ' with negative indirect width (acts as left-justified) */
    /* 'wZ' with flags, indirect width and width digit (*1 + "2" = 12) */
    /* 'wZ' with negative indirect width and width digit (*-1 + "2" = -12) */
    /* 'wZ' with invalid indirect width use */
    /* 'wZ' with different size prefixes (plus %u to validate pramaters are parsed correctly) */
    /* 'wZ' with invalid size prefixes */
    /* 'wZ' with precision (ignored for %c) */
    /* 'wZ' with width of 1 (no padding expected) */
    /* 'wZ' adjacent to %% */

    ok_sprintf_format(_T("  Hello"), _T("%7whhI64Z"), &ustr);


    /* 'wZ' - All flag/width/precision/invalid combinations */
    ok_sprintf_format(_T("   Hello"), _T("%8wZ"), &ustr);
    ok_sprintf_format(_T("Hello   "), _T("%-8wZ"), &ustr);
    ok_sprintf_format(_T("Hello"), _T("%.3wZ"), &ustr);
    ok_sprintf_format(_T(" Hello"), _T("%6.3wZ"), &ustr);
    ok_sprintf_format(_T("Hello"), _T("%.0wZ"), &ustr);
}

static void Test_printf_formats(void)
{
    /* No format specifier */
    ok_sprintf_format(_T(""), _T(""));

    /* Multiple % */
    ok_sprintf_format(_T("%"), _T("%%"));
    ok_sprintf_format(_T("%"), _T("%%%"));
    ok_sprintf_format(_T("%"), _T("%%%"));

    /* Multiple % with flags / modifiers */
    ok_sprintf_format(_T("%!"), _T("% %!"));
    ok_sprintf_format(_T("%!"), _T("%0%!"));
    ok_sprintf_format(_T("%!"), _T("%0ll%!"));

    /* Supported formats */
    ok_sprintf_format(_T("x"), _T("%c"), _T('x'));
    ok_sprintf_format(_T("x"), _T("%C"), _NT('x'));
    ok_sprintf_format(_T("-12345"), _T("%d"), -12345);
    ok_sprintf_format(_T("-12345"), _T("%i"), -12345);
    ok_sprintf_format(_T("12345"), _T("%o"), 012345);
#ifdef _WIN64
    ok_sprintf_format(_T("0000000000012345"), _T("%p"), (void*)0x12345ull);
#else
    ok_sprintf_format(_T("00012345"), _T("%p"), (void*)0x12345);
#endif
    ok_sprintf_format(_T("test"), _T("%s"), _T("test"));
    ok_sprintf_format(_T("test"), _T("%S"), _NT("test"));
    ok_sprintf_format(_T("12345"), _T("%u"), 12345);
    ok_sprintf_format(_T("1234abc"), _T("%x"), 0x1234ABC);
    ok_sprintf_format(_T("1234ABC"), _T("%X"), 0x1234ABC);
    ANSI_STRING astr = { 5, 6, "Hello" };
    ok_sprintf_format(_T("Hello"), _T("%Z"), &astr);
                          
    /* Unsupported formats */
    ok_sprintf_format(_T("a"), _T("%a"), 7.39185);
    ok_sprintf_format(_T("A"), _T("%A"), 7.39185);
    ok_sprintf_format(_T("e"), _T("%e"), 7.39185);
    ok_sprintf_format(_T("E"), _T("%E"), 7.39185);
    ok_sprintf_format(_T("f"), _T("%f"), 7.39185);
    ok_sprintf_format(_T("g"), _T("%g"), 7.39185);
    ok_sprintf_format(_T("G"), _T("%G"), 7.39185);
    ok_sprintf_format(_T("123n,-1"), _T("123%n,%d"), -1);
    ok_sprintf_format(_T("P"), _T("%P"), NULL);

    /* Flags and size prefixes without a valid format specifier get ignored */
    ok_sprintf_format(_T("?"), _T("% ?"), NULL);
    ok_sprintf_format(_T("?"), _T("%#?"), NULL);
    ok_sprintf_format(_T("?"), _T("%*?"), NULL);
    ok_sprintf_format(_T("?"), _T("%+?"), NULL);
    ok_sprintf_format(_T("?"), _T("%-?"), NULL);
    ok_sprintf_format(_T("?"), _T("%.?"), NULL);
    ok_sprintf_format(_T("?"), _T("%0?"), NULL);
    ok_sprintf_format(_T("?"), _T("%1?"), NULL);
    ok_sprintf_format(_T("?"), _T("%2?"), NULL);
    ok_sprintf_format(_T("?"), _T("%3?"), NULL);
    ok_sprintf_format(_T("?"), _T("%4?"), NULL);
    ok_sprintf_format(_T("?"), _T("%5?"), NULL);
    ok_sprintf_format(_T("?"), _T("%6?"), NULL);
    ok_sprintf_format(_T("?"), _T("%7?"), NULL);
    ok_sprintf_format(_T("?"), _T("%8?"), NULL);
    ok_sprintf_format(_T("?"), _T("%9?"), NULL);
    ok_sprintf_format(_T("?"), _T("%F?"), NULL);
    ok_sprintf_format(_T("?"), _T("%L?"), NULL);
    ok_sprintf_format(_T("?"), _T("%N?"), NULL);
    ok_sprintf_format(_T("?"), _T("%h?"), NULL);
    ok_sprintf_format(_T("?"), _T("%j?"), NULL);
    ok_sprintf_format(_T("?"), _T("%l?"), NULL);

    /* The 'hh' size prefix behaves like the 'h' prefix */
    ok_sprintf_format(_T("x,-1"),      _T("%hhc,%d"), _T('x'), -1);
    ok_sprintf_format(_T("x,-1"),       _T("%hc,%d"), _T('x'), -1);
    ok_sprintf_format(_T("21605,-1"),  _T("%hhd,%d"), 0x1DAB0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"),   _T("%hd,%d"), 0x1DAB0000 + 21605, -1);
    ok_sprintf_format(_T("-21605,-1"), _T("%hhd,%d"), 0x1DAB0000 + (uint16_t)-21605, -1);
    ok_sprintf_format(_T("-21605,-1"),  _T("%hd,%d"), 0x1DAB0000 + (uint16_t)-21605, -1);
    ok_sprintf_format(_T("21605,-1"),  _T("%hhu,%d"), 0x1DAB0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"),   _T("%hu,%d"), 0x1DAB0000 + 21605, -1);
    ok_sprintf_format(_T("21605,-1"),  _T("%hho,%d"), 0x1DAB0000 + 021605, -1);
    ok_sprintf_format(_T("21605,-1"),   _T("%ho,%d"), 0x1DAB0000 + 021605, -1);
    ok_sprintf_format(_T("16a5,-1"),   _T("%hhx,%d"), 0x1DAB0000 + 0x16A5, -1);
    ok_sprintf_format(_T("16a5,-1"),    _T("%hx,%d"), 0x1DAB0000 + 0x16A5, -1);
    ok_sprintf_format(_T("test,-1"),   _T("%hhs,%d"), "test", -1);
    ok_sprintf_format(_T("test,-1"),    _T("%hs,%d"), "test", -1);

    /* Both "w" and "l" work on*/


    /* 'F' (far) is a size prefix, that is ignored */
    ok_sprintf_format(_T(""), _T("%F"), 0, 0);
    ok_sprintf_format(_T("+d"), _T("%F+d"), 0, 0);
    ok_sprintf_format(_T("12345678,-1"), _T("%Fd,%d"), 12345678, -1);
    ok_sprintf_format(_T("24910,-1"), _T("%hFd,%d"), 12345678, -1);
    ok_sprintf_format(_T("123412345678,-1"), _T("%llFd,%d"), 123412345678, -1);
    ok_sprintf_format(_T("12345678,-1"), _T("%Fx,%d"), 0x12345678, -1);
    ok_sprintf_format(_T("5678,-1"), _T("%hFx,%d"), 0x12345678, -1);
    ok_sprintf_format(_T("123412345678,-1"), _T("%llFx,%d"), 0x123412345678, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%Fp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%llFp,%d"), 0x0000DEAD0000BEEFull, -1);
#else
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%Fp,%d"), 0x0000BEEF, -1);
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%hFp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEAD0000BEEF,-1"), _T("%llFp,%d"), 0x0000DEAD0000BEEFull, -1);
#endif
    ok_sprintf_format(_T("test"), _T("%Fs"), _T("test"));
    ok_sprintf_format(_T("test"), _T("%FS"), _NT("test"));

    /* 'N' (near) is a size prefix, that is ignored */
    ok_sprintf_format(_T(""), _T("%N"), 0, 0);
    ok_sprintf_format(_T("+d"), _T("%N+d"), 0, 0);
    ok_sprintf_format(_T("12345678,-1"), _T("%Nd,%d"), 12345678, -1);
    ok_sprintf_format(_T("24910,-1"), _T("%hNd,%d"), 12345678, -1);
    ok_sprintf_format(_T("123412345678,-1"), _T("%llNd,%d"), 123412345678, -1);
    ok_sprintf_format(_T("12345678,-1"), _T("%Nx,%d"), 0x12345678, -1);
    ok_sprintf_format(_T("5678,-1"), _T("%hNx,%d"), 0x12345678, -1);
    ok_sprintf_format(_T("123412345678,-1"), _T("%llNx,%d"), 0x123412345678, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%Np,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%llNp,%d"), 0x0000DEAD0000BEEFull, -1);
#else
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%Np,%d"), 0x0000BEEF, -1);
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%hNp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEAD0000BEEF,-1"), _T("%llNp,%d"), 0x0000DEAD0000BEEFull, -1);
#endif
    ok_sprintf_format(_T("test"), _T("%Ns"), _T("test"));
    ok_sprintf_format(_T("test"), _T("%NS"), _NT("test"));

    /* 'L' (long double) is a size prefix, that is ignored */
    ok_sprintf_format(_T(""), _T("%L"), 0, 0);
    ok_sprintf_format(_T("+d"), _T("%L+d"), 0, 0);
    ok_sprintf_format(_T("12345678,-1"), _T("%Ld,%d"), 12345678, -1);
    ok_sprintf_format(_T("24910,-1"), _T("%hLd,%d"), 12345678, -1);
    ok_sprintf_format(_T("123412345678,-1"), _T("%llLd,%d"), 123412345678, -1);
    ok_sprintf_format(_T("12345678,-1"), _T("%Lx,%d"), 0x12345678, -1);
    ok_sprintf_format(_T("5678,-1"), _T("%hLx,%d"), 0x12345678, -1);
    ok_sprintf_format(_T("123412345678,-1"), _T("%llLx,%d"), 0x123412345678, -1);
#ifdef _WIN64
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%Lp,%d"), 0x0000DEAD0000BEEFull, -1);
    ok_sprintf_format(_T("0000DEAD0000BEEF,-1"), _T("%llLp,%d"), 0x0000DEAD0000BEEFull, -1);
#else
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%Lp,%d"), 0x0000BEEF, -1);
    ok_sprintf_format(_T("0000BEEF,-1"), _T("%hLp,%d"), 0xDEADBEEF, -1);
    ok_sprintf_format(_T("DEAD0000BEEF,-1"), _T("%llLp,%d"), 0x0000DEAD0000BEEFull, -1);
#endif
    ok_sprintf_format(_T("test"), _T("%Ls"), _T("test"));
    ok_sprintf_format(_T("test"), _T("%LS"), _NT("test"));

/*
    Size prefixes can be combined, and the resulting size is determined by the combination of prefixes used.
    The rules for determining the resulting size are as follows:

    Size combination matrix:

        h   l   L   ll  j   w   I32 I64 z   t
    ---------------------------------------------
    h   16  16  16  64  64  16  16  64  16* 16*
    l   16  32  32  ?   64  32  32  64  32* 32*
    ll  64  64  64  64  64  64  64  64  64  64
    j   64  64  64  64  64  64  32  64  64  64
    w   16  32  32  64  64  32  32  64  32  32
    I32 16  32  32  64  64  32  32  64  32  32
    I64 64  64  64  64  64  64  32  64  64  64
    z   ---------------------------------------
    t   ---------------------------------------

    - ll -> 64 bit
    - I64 -> 64 bit, unless overwritten by I32, then 32 bit
    - j -> 64 bit, unless overwritten by I32, then 32 bit
 */
    const TCHAR* size_prefixes[] = { _T("h"), _T("hh"), _T("w"), _T("l"), _T("L"), _T("ll"), _T("j"), _T("w"), _T("I32"), _T("I64"), _T("I"), _T("z"), _T("t"), _T("N"), _T("F") };
    for (int first = 0; first < _countof(size_prefixes); first++)
    {
        for (int second = 0; second < _countof(size_prefixes); second++)
        {
            TCHAR fmt_X[16];
            TCHAR fmt_c[16];
            int int_size = 32;
            int is_wide_char = sizeof(TCHAR) == 2;
            p__stprintf(fmt_X, _T("%%%s%sX,%%d"), size_prefixes[first], size_prefixes[second]);
            p__stprintf(fmt_c, _T("%%%s%sc,%%d"), size_prefixes[first], size_prefixes[second]);

            /* If the first prefix is "I", "t" or "z", the format is invalid */
            if (!_tcscmp(size_prefixes[first], _T("I")) || !_tcscmp(size_prefixes[first], _T("t")) || !_tcscmp(size_prefixes[first], _T("z")))
            {
                TCHAR expected[16];
                p__stprintf(expected, _T("%s%sX,-1"), size_prefixes[first], size_prefixes[second]);
                ok_sprintf_format(expected, fmt_X, -1);
                continue;
            }
#ifdef _WIN64
            /* 64 bit only: if the second prefix is "I", "t" or "z", the format is 64 bits, ignored on 32 bit system */
            else if (!_tcscmp(size_prefixes[second], _T("I")) || !_tcscmp(size_prefixes[second], _T("t")) || !_tcscmp(size_prefixes[second], _T("z")))
            {
                int_size = 64;
            }
#endif
            /* If either prefix is "ll", the format is 64 bits */
            else if (!_tcscmp(size_prefixes[first], _T("ll")) || !_tcscmp(size_prefixes[second], _T("ll")))
            {
                int_size = 64;
            }

            /* Special case: both prefixes are "l", which gets interpreted as "ll" */
            else if (!_tcscmp(size_prefixes[first], _T("l")) && !_tcscmp(size_prefixes[second], _T("l")))
            {
                int_size = 64;
            }

            /* If the second prefix is "I64" or "j", the format is 64 bits */
            else if (!_tcscmp(size_prefixes[second], _T("I64")) || !_tcscmp(size_prefixes[second], _T("j")))
            {
                int_size = 64;
            }

            /* If the first prefix is "I64" or "j" and the second is not "I32", the format is 64 bits */
            else if ((!_tcscmp(size_prefixes[first], _T("I64")) || !_tcscmp(size_prefixes[first], _T("j"))) &&
                     _tcscmp(size_prefixes[second], _T("I32")))
            {
                int_size = 64;
            }

            /* If either prefix is "h" or "hh", the format is 16 bits */
            else if (!_tcscmp(size_prefixes[first], _T("h")) || !_tcscmp(size_prefixes[second], _T("h")) ||
                     !_tcscmp(size_prefixes[first], _T("hh")) || !_tcscmp(size_prefixes[second], _T("hh")))
            {
                int_size = 16;
            }

            /* Validate the integer size */
            if (int_size == 64)
            {
                ok_sprintf_format(_T("1234567890ABCDEF,-1"), fmt_X, 0x1234567890ABCDEFull, -1);
            }
            else if (int_size == 16)
            {
                ok_sprintf_format(_T("5678,-1"), fmt_X, 0x12345678, -1);
            }
            else
            {
                ok_sprintf_format(_T("ABCDEF01,-1"), fmt_X, (intptr_t)(int)0xABCDEF01, -1);
            }

#ifdef USE_WIDECHAR
            /* If either prefix is "h", but not both, the char format is narrow */
            if (!_tcscmp(size_prefixes[first], _T("h")) || !_tcscmp(size_prefixes[second], _T("h")) ||
                !_tcscmp(size_prefixes[first], _T("hh")) || !_tcscmp(size_prefixes[second], _T("hh")))
            {
                is_wide_char = 0;
            }
#else
            /* If either prefix is "w", the char format is wide */
            if (!_tcscmp(size_prefixes[first], _T("w")) || !_tcscmp(size_prefixes[second], _T("w")))
            {
                is_wide_char = 1;
            }

            /* If either prefix is "l", but not both, the char format is wide */
            if (!_tcscmp(size_prefixes[first], _T("l")) ^ !_tcscmp(size_prefixes[second], _T("l")))
            {
                is_wide_char = 1;
            }
#endif

            /* Validate the char format */
            if (!_tcscmp(size_prefixes[second], _T("I")) || !_tcscmp(size_prefixes[second], _T("t")) || !_tcscmp(size_prefixes[second], _T("z")))
            {
                /* "I", "t" and "z" are invalid with "c" */
                TCHAR expected[16];
                p__stprintf(expected, _T("%sc,-1"), size_prefixes[second]);
                ok_sprintf_format(expected, fmt_c, -1);
                continue;
            }
            else if (is_wide_char)
            {
#ifdef USE_WIDECHAR
                ok_sprintf_format(L"\x0101,-1", fmt_c, L'\x0101', -1);
#else
                ok_sprintf_format("a,-1", fmt_c, L'\x0101', -1);
#endif
            }
            else
            {
#ifdef USE_WIDECHAR
                ok_sprintf_format(L"\x0001,-1", fmt_c, L'\x0101', -1);
#else
                ok_sprintf_format(_T("\x01,-1"), fmt_c, L'\x0101', -1);
#endif
            }
        }
    }

    /* Prefix parsing error handling */
    ok_sprintf_format(_T("I33d"), _T("%hI33d"), NULL);
    ok_sprintf_format(_T("I65d"), _T("%hI65d"), NULL);

    /* Test all invalid format characters */
    for (int c = 0; c <= ((sizeof(TCHAR) == 2) ? 0xFFFF : 0xFF); c++)
    {
        /* Skip valid format specifiers (see above) */
        if (_tcschr(_T("cCdiopsSuxXZ%"), (TCHAR)c))
            continue;

        TCHAR fmt[4] = { '%', (TCHAR)c, '!', '\0' };

        /* Modifiers*/
        if (_tcschr(_T("FLNhjlw #*+-.0123456789"), (TCHAR)c))
        {
            ok_sprintf_format(_T("!"), fmt, NULL, NULL, NULL);
            continue;
        }

        TCHAR expected[4] = { (TCHAR)c, '!', '\0', 0 };
        ok_sprintf_format(expected, fmt, NULL, NULL, NULL);
    }

    /* Test all invalid flag characters */
    for (int c = 0; c <= ((sizeof(TCHAR) == 2) ? 0xFFFF : 0xFF); c++)
    {
        /* Skip valid formats */
        if (_tcschr(_T("cCdinopsSuxXZwZ%"), (TCHAR)c))
            continue;

        /* Skip valid flags and modifiers */
        if (_tcschr(_T("FLNhjl #*+-.0123456789"), (TCHAR)c))
            continue;

        TCHAR fmt[4] = { '%', (TCHAR)c, 'c', '\0' };
        TCHAR expected[4] = { (TCHAR)c, 'c', '\0', 0 };
        ok_sprintf_format(expected, fmt, NULL, NULL, NULL);
    }

    /* Test char<->wchar_t conversion */
    if (sizeof(TCHAR) == sizeof(wchar_t))
    {
        PFN_mbstowcs p_mbstowcs = load_function("mbstowcs");
        for (int i = 1; i <= 0xFF; i++)
        {
            TCHAR buf[4];
            wchar_t expected[4];

            p_mbstowcs(expected, (char*)&i, 2);

            int ret = p__stprintf(buf, _T("%C"), i);
            ok(ret == 1, "[%d]: Expected ret 1, got %d\n", i, ret);
            ok(buf[0] == expected[0], "[%d]: Expected '0x%x', got '0x%x'\n", i, i, buf[0]);

            char src[2] = { (char)i, '\0' };
            ret = p__stprintf(buf, _T("%S"), src);
            ok(ret == 1, "[%d]: Expected ret 1, got %d\n", i, ret);
            ok(buf[0] == expected[0], "[%d]: Expected '0x%x', got '0x%x'\n", i, i, buf[0]);
        }
    }
    else
    {
        PFN_wcstombs p_wcstombs = load_function("wcstombs");
        for (int i = 1; i <= 0xFFFF; i++)
        {
            TCHAR buf[4];
            char expected[4];

            p_wcstombs(expected, (wchar_t*)&i, 2);

            int ret = p__stprintf(buf, _T("%C"), i);
            ok(ret == 1, "[%d]: Expected ret 1, got %d\n", i, ret);
            ok(buf[0] == expected[0], "[%d]: Expected '0x%x', got '0x%x'\n", i, i, buf[0]);

            wchar_t src[2] = { (wchar_t)i, L'\0' };
            ret = p__stprintf(buf, _T("%S"), src);
            ok(ret == 1, "[%d]: Expected ret 1, got %d\n", i, ret);
            ok(buf[0] == expected[0], "[%d]: Expected '0x%x', got '0x%x'\n", i, i, buf[0]);
        }
    }

    /* Extensive tests for each format specifier */
    Test_printf_format_c();
    Test_printf_format_C();
    Test_printf_format_d();
    Test_printf_format_i();
    Test_printf_format_u();
    Test_printf_format_o();
    Test_printf_format_x();
    Test_printf_format_X();
    Test_printf_format_p();
    Test_printf_format_s();
    Test_printf_format_S();
    Test_printf_format_Z();
    Test_printf_format_wZ();
}
