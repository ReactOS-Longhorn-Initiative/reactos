#include <stdint.h>
#include <stdlib.h>

#ifdef USE_WIDECHAR
  typedef wchar_t tchar_t;
  #define _T(x) L##x
  #define _NT(x) x
  #define _tcslen wcslen
  #define _tcscmp wcscmp
  #define _tcsncmp wcsncmp
  #define _tcschr wcschr
  #define _tcscat wcscat
  #define _tcsncat wcsncat
  #define TSTRFMT "%ws"
  #define ok_eq_tstr ok_eq_wstr
#else
  typedef char tchar_t;
  #define _T(x) x
  #define _NT(x) L##x
  #define _tcslen strlen
  #define _tcscmp strcmp
  #define _tcsncmp strncmp
  #define _tcschr strchr
  #define _tcscat strcat
  #define _tcsncat strncat
  #define TSTRFMT "%s"
  #define ok_eq_tstr ok_eq_str
#endif

#ifdef IS_SECURE_API
#define SEC_API_BUFSIZE_ARG(buf) , _countof(buf)
#else
#define SEC_API_BUFSIZE_ARG(buf)
#endif


#define wcscpy_s(dest, dest_size, src) wcsncpy(dest, src, dest_size)
#pragma warning(disable: 4996) // disable MSVC warning about using wcscpy_s in a test, we don't care about buffer overflows in tests

static void fillbuf(tchar_t* buf, size_t size, tchar_t fill)
{
    memset(buf, 0x0, size);
    for (size_t i = 0; i < size / sizeof(tchar_t); i++)
    {
        buf[i] = fill;
    }
}

static void Test_scanf_format_c(void)
{
    tchar_t buf[16];

    /* One character */
    //ok_sscanf(1, "A\xAA", "AB", "%c");
    //ok_sscanf(1, " \xAA", " AB", "%c");

    /* Test different flags with 'c' */
    ok_eq_int(p__stscanf(_T("AB"), _T("%-c"), buf, _countof(buf)), 0);
    ok_eq_int(p__stscanf(_T("AB"), _T("%+c"), buf, _countof(buf)), 0);
    ok_eq_int(p__stscanf(_T("AB"), _T("% c"), buf, _countof(buf)), 0);
    ok_eq_int(p__stscanf(_T("AB"), _T("%0c"), buf, _countof(buf)), 0);
    ok_eq_int(p__stscanf(_T("AB"), _T("%#c"), buf, _countof(buf)), 0);

    /* Field width */
    fillbuf(buf, sizeof(buf), 0xAA);
    buf[5] = 0;
    ok_eq_int(p__stscanf(_T("\tA BC"), _T("%4c"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("\tA B\xAA"));

    /* Field width starts with 0 */
    fillbuf(buf, sizeof(buf), 0xAA);
    buf[3] = 0;
    ok_eq_int(p__stscanf(_T("\tA BC"), _T("%01c"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("\t\xAA\xAA"));

    /* Indirect field width (invalid) */
    ok_eq_int(p__stscanf(_T("ABC"), _T("%*c"), 3, buf, _countof(buf)), 0);

    /* Precision (invalid) */
    ok_eq_int(p__stscanf(_T("ABCD"), _T("%.2c"), buf, _countof(buf)), 0);

    /* Field width and precision (invalid) */
    ok_eq_int(p__stscanf(_T("ABCDE"), _T("%4.2c"), buf, _countof(buf)), 0);

    /* Different size prefixes */
    fillbuf(buf, sizeof(buf), 0xAA);
    buf[3] = 0;
    ok_eq_int(p__stscanf(_T("AB"), _T("%hc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("A\xAA\xAA"));
    ok_eq_int(p__stscanf(_T("BC"), _T("%hhc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("B\xAA\xAA"));

    wchar_t wbuf[3] = { 0xAA, 0xAA, 0x00 };
    ok_eq_int(p__stscanf(_T("CD"), _T("%wc"), wbuf, _countof(wbuf)), 1);
#if defined(IS_SECURE_API) && !defined(USE_WIDECHAR)
    // sscanf_s is severely broken and returns the low 16 bits of a stack variable instead of the character
    ok(wbuf[0] != 0xAA, "Expected wbuf[0] to be overwritten with something\n");
    ok_eq_wchar(wbuf[1], 0xAA);
#else
    ok_eq_wstr(wbuf, L"C\xAA");
#endif

    wbuf[0] = 0xAA;
    ok_eq_int(p__stscanf(_T("\x8A!"), _T("%lc"), wbuf, _countof(wbuf)), 1);
#if defined(IS_SECURE_API) && !defined(USE_WIDECHAR)
    // sscanf_s is severely broken and returns the low 16 bits of a stack variable instead of the character
    ok(wbuf[0] != 0xAA, "Expected wbuf[0] to be overwritten with something\n");
    ok_eq_wchar(wbuf[1], 0xAA);
#elif defined(USE_WIDECHAR)
    ok_eq_wstr(wbuf, L"\x8A\xAA");
#else
    ok_eq_wstr(wbuf, L"\u0160\xAA");
#endif

    fillbuf(buf, 3 * sizeof(tchar_t), 0xAA);
    ok_eq_int(p__stscanf(_T("AB"), _T("%llc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("A\xAA\xAA"));
    fillbuf(buf, 3 * sizeof(tchar_t), 0xAA);
    ok_eq_int(p__stscanf(_T("AB"), _T("%jc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("A\xAA\xAA"));
    ok_eq_int(p__stscanf(_T("AB"), _T("%Lc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("A\xAA\xAA"));
    ok_eq_int(p__stscanf(_T("A"), _T("%I32c"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("A\xAA\xAA"));
    ok_eq_int(p__stscanf(_T("A"), _T("%I64c"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("A\xAA\xAA"));

    /* F (far), N (near) and L (long double) are valid, but ignored */
    fillbuf(buf, 3 * sizeof(tchar_t), 0xAA);
    ok_eq_int(p__stscanf(_T("F*"), _T("%Fc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("F\xAA\xAA"));
    ok_eq_int(p__stscanf(_T("N*"), _T("%Nc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("N\xAA\xAA"));
    ok_eq_int(p__stscanf(_T("L*"), _T("%Lc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("L\xAA\xAA"));

    /* Invalid size prefixes */
    ok_eq_int(p__stscanf(_T("A"), _T("%zc"), buf, _countof(buf)), 0);
    ok_eq_int(p__stscanf(_T("A"), _T("%tc"), buf, _countof(buf)), 0);
    ok_eq_int(p__stscanf(_T("A"), _T("%Ic"), buf, _countof(buf)), 0);

    /* Multiple size prefixes */
    fillbuf(buf, 3 * sizeof(tchar_t), 0xAA);
    ok_eq_int(p__stscanf(_T("hw*"), _T("%2hwc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("hw\xAA"));
    ok_eq_int(p__stscanf(_T("wh*"), _T("%2whc"), buf, _countof(buf)), 1);
    ok_eq_tstr(buf, _T("wh\xAA"));
}

static void Test_scanf_format_C(void)
{
#if defined(IS_SECURE_API) && !defined(USE_WIDECHAR)
    // TODO: sscanf_s is severely broken and returns the low 16 bits of a stack variable instead of the character, so we can't test this
#elif defined(USE_WIDECHAR)
    // TODO like below, but with wchar_t and %C
#else
    wchar_t buf[16];
    
    wcscpy_s(buf, sizeof(buf) / sizeof(buf[0]), L"\xAAAA\xAAAA");
    ok_eq_int(p__stscanf(_T("\x8A"), "%C", buf), 1);
    ok_eq_wstr(buf, L"\x0160\xAAAA");
    wcscpy_s(buf, sizeof(buf) / sizeof(buf[0]), L"\xAAAA\xAAAA");
    ok_eq_int(p__stscanf(_T("\x8A"), "%hC", buf), 1);
    ok_eq_wstr(buf, L"\xAA8A\xAAAA");
    wcscpy_s(buf, sizeof(buf) / sizeof(buf[0]), L"\xAAAA\xAAAA");
    ok_eq_int(p__stscanf(_T("\x54"), "%hC", buf), 1);
    ok_eq_wstr(buf, L"\xAA54\xAAAA");
#endif
}

static void Test_scanf_format_d(void)
{
    unsigned char buf[10];

    /* Test different flags with 'd' - all invalid for scanf */
    ok_eq_int(p__stscanf(_T("123"), _T("%-d"), &buf), 0);
    ok_eq_int(p__stscanf(_T("123"), _T("%+d"), &buf), 0);
    ok_eq_int(p__stscanf(_T("123"), _T("% d"), &buf), 0);
    ok_eq_int(p__stscanf(_T("123"), _T("%0d"), &buf), 0);
    ok_eq_int(p__stscanf(_T("123"), _T("%#d"), &buf), 0);

    /* Suppress assignment */
    ok_eq_int(p__stscanf(_T("123"), _T("%*d"), 3, &buf), 0);

    /* Precision (invalid) */
    ok_eq_int(p__stscanf(_T("123"), _T("%.2d"), &buf), 0);

    /* Field width and precision (invalid) */
    ok_eq_int(p__stscanf(_T("12345"), _T("%4.2d"), &buf), 0);

    /* Basic decimal parsing */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("123"), _T("%d"), &buf), 1);
    ok_eq_int(*(int*)buf, 123);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-456"), _T("%d"), &buf), 1);
    ok_eq_int(*(int*)buf, -456);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("+789"), _T("%d"), &buf), 1);
    ok_eq_int(*(int*)buf, 789);
    ok_eq_uchar(buf[4], 0xAA);

    /* Octal prefix is not treated specially for %d */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("010"), _T("%d"), &buf), 1);
    ok_eq_int(*(int*)buf, 10);
    ok_eq_uchar(buf[4], 0xAA);

    /* Hex prefix is not treated specially for %d */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("0x10"), _T("%d"), &buf), 1);
    ok_eq_int(*(int*)buf, 0);
    ok_eq_uchar(buf[4], 0xAA);

    /* Field width */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567"), _T("%4d"), &buf), 1);
    ok_eq_int(*(int*)buf, -123);
    ok_eq_uchar(buf[4], 0xAA);

    /* Field width starting with 0 */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-012345"), _T("%04d"), &buf), 1);
    ok_eq_int(*(int*)buf, -12);
    ok_eq_uchar(buf[4], 0xAA);

    /* Size prefix 'h' - reads into short */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234"), _T("%hd"), &buf), 1);
    ok_eq_int(*(short*)buf, -1234);
    ok_eq_uchar(buf[2], 0xAA);

    /* Size prefix 'hh' - reads into int */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-12"), _T("%hhd"), &buf), 1);
    ok_eq_int(*(int*)buf, -12);
    ok_eq_uchar(buf[5], 0xAA);

    /* Size prefix 'll' - reads into long long */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%lld"), &buf), 1);
    ok_eq_int64(*(long long*)buf, -1234567890LL);
    ok_eq_uchar(buf[8], 0xAA);

    /* Size prefix 'L' is ignored - reads into int */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-17"), _T("%Ld"), &buf), 1);
    ok_eq_int(*(int*)buf, -17);
    ok_eq_uchar(buf[5], 0xAA);

    /* Size prefix 'I32' - reads into 32-bit int */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-123456"), _T("%I32d"), &buf), 1);
    ok_eq_int(*(int*)buf, -123456);
    ok_eq_uchar(buf[4], 0xAA);

    /* Size prefix 'I64' - reads into 64-bit int */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%I64d"), &buf), 1);
    ok_eq_int64(*(long long*)buf, -1234567890LL);
    ok_eq_uchar(buf[8], 0xAA);

    /* Size prefix 'I' - reads into pointer size int */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%Id"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, -1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    /* Size prefix 'z' - reads into pointer size int */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%zd"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, -1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    /* Size prefix 't' - reads into pointer size int */
    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%td"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, -1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);
}

static void Test_scanf_format_D(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("123"), _T("%D"), &buf), 1);
    ok_eq_int(*(int*)buf, 123);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-456"), _T("%D"), &buf), 1);
    ok_eq_int(*(int*)buf, -456);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-17"), _T("%LD"), &buf), 1);
    ok_eq_int(*(int*)buf, -17);
    ok_eq_uchar(buf[4], 0xAA);
}

static void Test_scanf_format_i(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("123"), _T("%i"), &buf), 1);
    ok_eq_int(*(int*)buf, 123);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-456"), _T("%i"), &buf), 1);
    ok_eq_int(*(int*)buf, -456);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("010"), _T("%i"), &buf), 1);
    ok_eq_int(*(int*)buf, 8);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("0x10"), _T("%i"), &buf), 1);
    ok_eq_int(*(int*)buf, 16);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234"), _T("%hi"), &buf), 1);
    ok_eq_int(*(short*)buf, -1234);
    ok_eq_uchar(buf[2], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%lli"), &buf), 1);
    ok_eq_int64(*(long long*)buf, -1234567890LL);
    ok_eq_uchar(buf[8], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-17"), _T("%Li"), &buf), 1);
    ok_eq_int(*(int*)buf, -17);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-123456"), _T("%I32i"), &buf), 1);
    ok_eq_int(*(int*)buf, -123456);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%I64i"), &buf), 1);
    ok_eq_int64(*(long long*)buf, -1234567890LL);
    ok_eq_uchar(buf[8], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%Ii"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, -1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%zi"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, -1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("-1234567890"), _T("%ti"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, -1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);
}

static void Test_scanf_format_I(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("123"), _T("%I"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 123);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("010"), _T("%I"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 8);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("0x10"), _T("%I"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 16);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);
}

static void Test_scanf_format_o(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("377"), _T("%o"), &buf), 1);
    ok_eq_int(*(int*)buf, 255);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("377"), _T("%Lo"), &buf), 1);
    ok_eq_int(*(int*)buf, 255);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("377"), _T("%ho"), &buf), 1);
    ok_eq_int(*(short*)buf, 255);
    ok_eq_uchar(buf[2], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("177777"), _T("%llo"), &buf), 1);
    ok_eq_int64(*(long long*)buf, 65535);
    ok_eq_uchar(buf[8], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("377"), _T("%I32o"), &buf), 1);
    ok_eq_int(*(int*)buf, 255);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("177777"), _T("%I64o"), &buf), 1);
    ok_eq_int64(*(long long*)buf, 65535);
    ok_eq_uchar(buf[8], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("177777"), _T("%Io"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 65535);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("177777"), _T("%zo"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 65535);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("177777"), _T("%to"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 65535);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);
}

static void Test_scanf_format_O(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("377"), _T("%O"), &buf), 1);
    ok_eq_int(*(int*)buf, 255);
    ok_eq_uchar(buf[4], 0xAA);
}

static void Test_scanf_format_u(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("123456"), _T("%u"), &buf), 1);
    ok_eq_int(*(int*)buf, 123456);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("123456"), _T("%Lu"), &buf), 1);
    ok_eq_int(*(int*)buf, 123456);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234"), _T("%hu"), &buf), 1);
    ok_eq_int(*(short*)buf, 1234);
    ok_eq_uchar(buf[2], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234567890"), _T("%llu"), &buf), 1);
    ok_eq_int64(*(long long*)buf, 1234567890LL);
    ok_eq_uchar(buf[8], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("123456"), _T("%I32u"), &buf), 1);
    ok_eq_int(*(int*)buf, 123456);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234567890"), _T("%I64u"), &buf), 1);
    ok_eq_int64(*(long long*)buf, 1234567890LL);
    ok_eq_uchar(buf[8], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234567890"), _T("%Iu"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234567890"), _T("%zu"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234567890"), _T("%tu"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 1234567890);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);
}

static void Test_scanf_format_U(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("123456"), _T("%U"), &buf), 1);
    ok_eq_int(*(int*)buf, 123456);
    ok_eq_uchar(buf[4], 0xAA);
}

static void Test_scanf_format_x(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("7B"), _T("%x"), &buf), 1);
    ok_eq_int(*(int*)buf, 123);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("0x10"), _T("%x"), &buf), 1);
    ok_eq_int(*(int*)buf, 16);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("7B"), _T("%hx"), &buf), 1);
    ok_eq_int(*(short*)buf, 123);
    ok_eq_uchar(buf[2], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234ABCD"), _T("%llx"), &buf), 1);
    ok_eq_int64(*(long long*)buf, 0x1234ABCD);
    ok_eq_uchar(buf[8], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("7B"), _T("%I32x"), &buf), 1);
    ok_eq_int(*(int*)buf, 123);
    ok_eq_uchar(buf[4], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234ABCD"), _T("%I64x"), &buf), 1);
    ok_eq_int64(*(long long*)buf, 0x1234ABCD);
    ok_eq_uchar(buf[8], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234ABCD"), _T("%Ix"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 0x1234ABCD);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234ABCD"), _T("%zx"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 0x1234ABCD);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("1234ABCD"), _T("%tx"), &buf), 1);
    ok_eq_int64(*(intptr_t*)buf, 0x1234ABCD);
    ok_eq_uchar(buf[sizeof(intptr_t)], 0xAA);
}

static void Test_scanf_format_X(void)
{
    unsigned char buf[16];

    memset(buf, 0xAA, sizeof(buf));
    ok_eq_int(p__stscanf(_T("7B"), _T("%X"), &buf), 1);
    ok_eq_int(*(int*)buf, 123);
    ok_eq_uchar(buf[4], 0xAA);
}

static void Test_scanf_format_p(void)
{
    void* ptr_val;

    ptr_val = (void*)(uintptr_t)0xAAAAAAAA;
    ok_eq_int(p__stscanf(_T("1234"), _T("%p"), &ptr_val), 1);
    ok_eq_ptr(ptr_val, (void*)(uintptr_t)0x1234);

    ptr_val = (void*)(uintptr_t)0xAAAAAAAA;
    ok_eq_int(p__stscanf(_T("0x1234"), _T("%p"), &ptr_val), 1);
    ok_eq_ptr(ptr_val, (void*)(uintptr_t)0x0);
}

static void Test_scanf_format_P(void)
{
    void* ptr_val;

    ptr_val = (void*)(uintptr_t)0xAAAAAAAA;
    ok_eq_int(p__stscanf(_T("1234"), _T("%P"), &ptr_val), 1);
    ok_eq_ptr(ptr_val, (void*)(uintptr_t)0x1234);

    ptr_val = (void*)(uintptr_t)0xAAAAAAAA;
    ok_eq_int(p__stscanf(_T("0x1234"), _T("%P"), &ptr_val), 1);
    ok_eq_ptr(ptr_val, (void*)(uintptr_t)0x0);
}

static void Test_scanf_format_S(void)
{
#if defined(IS_SECURE_API) && !defined(USE_WIDECHAR)
    // sscanf_s is severely broken and returns the low 16 bits of a stack variable instead of the character, so we can't test this
#elif defined(USE_WIDECHAR)
    char char_buf[16];

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("AB CD"), _T("%S"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_int(char_buf[0], 'A');
    ok_eq_uchar((unsigned char)char_buf[1], 0xAA);

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("AB CD"), _T("AB %S"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_int(char_buf[0], 'C');
    ok_eq_uchar((unsigned char)char_buf[1], 0xAA);
#else
    wchar_t wchar_buf[16];

    memset(wchar_buf, 0xAA, sizeof(wchar_buf));
    ok_eq_int(p__stscanf(_T("AB CD"), _T("%S"), wchar_buf SEC_API_BUFSIZE_ARG(wchar_buf)), 1);
    ok_eq_int(wchar_buf[0], L'A');
    ok_eq_int(wchar_buf[1], L'B');

    memset(wchar_buf, 0xAA, sizeof(wchar_buf));
    ok_eq_int(p__stscanf(_T("AB CD"), _T("AB %S"), wchar_buf SEC_API_BUFSIZE_ARG(wchar_buf)), 1);
    ok_eq_int(wchar_buf[0], L'C');
    ok_eq_int(wchar_buf[1], L'D');
#endif
}

static void Test_scanf_format_s(void)
{
    tchar_t char_buf[32];

    /* Test basic string parsing */
    ok_eq_int(p__stscanf(_T("Hello, World!"), _T("%s"), char_buf, _countof(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("Hello,"));
    ok_eq_int(p__stscanf(_T("Hello, World!"), _T(" \tHello,\n %s"), char_buf, _countof(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("World!"));

    /* Test different flags with 's' */
    ok_eq_int(p__stscanf(_T("Test"), _T("%-s"), char_buf, _countof(char_buf)), 0);
    ok_eq_int(p__stscanf(_T("Test"), _T("%+s"), char_buf, _countof(char_buf)), 0);
    ok_eq_int(p__stscanf(_T("Test"), _T("% s"), char_buf, _countof(char_buf)), 0);
    ok_eq_int(p__stscanf(_T("Test"), _T("%0s"), char_buf, _countof(char_buf)), 0);
    ok_eq_int(p__stscanf(_T("Test"), _T("%#s"), char_buf, _countof(char_buf)), 0);

    /* Field width */
    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("Hello, World!"), _T("%4s"), char_buf, _countof(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("Hell"));
    ok_eq_int(p__stscanf(_T("Hello, World!"), _T("%8s"), char_buf, _countof(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("Hello,"));

    /* Field width starts with 0 */
    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("\tAB C"), _T("%01s"), char_buf, _countof(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("A"));

    /* Indirect field width (invalid) */
    ok_eq_int(p__stscanf(_T("ABC"), _T("%*s"), 3, char_buf), 0);

    /* Precision (invalid) */
    ok_eq_int(p__stscanf(_T("ABCD"), _T("%.2s"), char_buf, _countof(char_buf)), 0);

    /* Field width and precision (invalid) */
    ok_eq_int(p__stscanf(_T("ABCDE"), _T("%4.2s"), char_buf, _countof(char_buf)), 0);
}

static void Test_scanf_format_n(void)
{
    unsigned char num_buf[16];
    tchar_t char_buf[8];
    int count;

    memset(num_buf, 0xAA, sizeof(num_buf));
    ok_eq_int(p__stscanf(_T("ABC"), _T("%n"), num_buf), 0);
    ok_eq_int(*(int*)num_buf, 0);
    ok_eq_uchar(num_buf[4], 0xAA);

    memset(num_buf, 0xAA, sizeof(num_buf));
    ok_eq_int(p__stscanf(_T("ABC"), _T("%hn"), num_buf), 0);
    ok_eq_int(*(short*)num_buf, 0);
    ok_eq_uchar(num_buf[2], 0xAA);

    memset(num_buf, 0xAA, sizeof(num_buf));
    ok_eq_int(p__stscanf(_T("ABC"), _T("%lln"), num_buf), 0);
    ok_eq_int64(*(long long*)num_buf, 0);
    ok_eq_uchar(num_buf[8], 0xAA);

    count = -1;
    ok_eq_int(p__stscanf(_T("ABCD"), _T("ABC%n"), &count), 0);
    ok_eq_int(count, 3);

    /* String plus count */
    fillbuf(char_buf, sizeof(char_buf), 0xAA);
    ok_eq_int(p__stscanf(_T("ABCD"), _T("%3c%n"), char_buf SEC_API_BUFSIZE_ARG(char_buf), &count), 1);
    ok_eq_int(count, 3);
    ok_eq_int(_tcsncmp(char_buf, _T("ABC\xAA"), 4), 0);

    /* Number plus count */
    memset(num_buf, 0xAA, sizeof(num_buf));
    ok_eq_int(p__stscanf(_T("-123x"), _T("%d%n"), num_buf, &count), 1);
    ok_eq_int(*(int*)num_buf, -123);
    ok_eq_uchar(num_buf[4], 0xAA);
    ok_eq_int(count, 4);
}

static void Test_scanf_format_scanset(void)
{
    tchar_t char_buf[32];

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("cabXYZabc"), _T("%[abc]"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("cab"));

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("XYZcbade"), _T("%[^abc]"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("XYZ"));

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("abcXYZ"), _T("%[a-z]"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("abc"));

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("XYZabc"), _T("%[^a-z]"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_tstr(char_buf, _T("XYZ"));

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("abcXYZ"), _T("%{abc}"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_int(char_buf[0], _T('a'));

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("XYZabc"), _T("%{XYZ}"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_int(char_buf[0], _T('X'));

    memset(char_buf, 0xAA, sizeof(char_buf));
    ok_eq_int(p__stscanf(_T("abcXYZ"), _T("%{a-z}"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 1);
    ok_eq_int(char_buf[0], _T('a'));

    /* Negation doesn't work here */
    ok_eq_int(p__stscanf(_T("abcXYZ"), _T("%{^abc}"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 0);
    ok_eq_int(p__stscanf(_T("XYZabc"), _T("%{^a-z}"), char_buf SEC_API_BUFSIZE_ARG(char_buf)), 0);
}

static void Test_scanf_format(void)
{
    tchar_t buf[16];
    int ret;

    /* Test different format characters */
    for (int c = 1; c <= ((sizeof(tchar_t) == 2) ? 0xFFFF : 0xFF); c++)
    {
        tchar_t fmt[4] = { '%', (tchar_t)c, '\0' };

        /* Check valid format specifiers */
        if (_tcschr(_T("CDIOPSUXcdiopsux"), (tchar_t)c))
        {
            ret = p__stscanf(_T("0"), fmt, buf, sizeof(buf));
            ok(ret == 1, "Format '%s' (0x%02x): Expected 1, got %d\n", fmt, c, ret);

            /* Test different modifiers */
            for (int m = 1; m <= 0xFF; m++)
            {
                tchar_t fmt_mod[8] = { '%', (tchar_t)m, (tchar_t)c, '\0' };

                if (_tcschr(_T("CDIOPSUXcdiopsux123456789FLNhjlw"), (tchar_t)m))
                {
                    ret = p__stscanf(_T("0"), fmt_mod, buf, sizeof(buf));
                    ok(ret == 1, "Format '%s' (0x%02x): Expected 1, got %d\n", fmt_mod, c, ret);
                    continue;
                }

                /* 't' and 'z' are allowed for certain format specifiers */
                if (_tcschr(_T("Xdioux"), (tchar_t)c) && _tcschr(_T("tz"), (tchar_t)m))
                {
                    ret = p__stscanf(_T("0"), fmt_mod, buf, sizeof(buf));
                    ok(ret == 1, "Format '%s' (0x%02x): Expected 1, got %d\n", fmt_mod, c, ret);
                    continue;
                }

                if (_tcschr(_T("[{"), (tchar_t)m))
                {
                    continue;
                }

                ret = p__stscanf(_T("0"), fmt_mod, buf, sizeof(buf));
                ok(ret == 0, "Format '%s' (0x%02x): Expected 0, got %d\n", fmt_mod, c, ret);
            }
            continue;
        }

        if (_tcschr(_T("[{"), (tchar_t)c))
        {
            continue;
        }

        ret = p__stscanf(_T("0"), fmt, buf, _countof(buf));
        ok(ret == 0, "Format '%s' (0x%02x): Expected 0, got %d\n", fmt, c, ret);
        ret = p__stscanf(_T("0x0"), fmt, buf, _countof(buf));
        ok(ret == 0, "Format '%s' (0x%02x): Expected 0, got %d\n", fmt, c, ret);

        if (c == 'n')
        {
            ret = p__stscanf(_T(""), fmt, buf, _countof(buf));
            ok(ret == 0, "Format '%s' (0x%02x): Expected 0, got %d\n", fmt, c, ret);
            continue;
        }

        ret = p__stscanf(_T(""), fmt, buf, _countof(buf));
#ifdef USE_WIDECHAR
        ok(ret == 0, "Format '%s' (0x%02x): Expected 0, got %d\n", fmt, c, ret);
#else
        ok(ret == -1, "Format '%s' (0x%02x): Expected -1, got %d\n", fmt, c, ret);
#endif
    }

    /* Test different format modifier combinations */
    const tchar_t format_specifier[] = _T("CDIOPSUXcdiopsux");
    for (int s = 0; s < _countof(format_specifier) - 1; s++)
    {
        const tchar_t* modifiers[] = { _T(""), _T("h"), _T("hh"), _T("l"), _T("ll"), _T("j"), _T("L"), _T("I"), _T("I32"), _T("I64"), _T("t"), _T("z")};
        for (int m1 = 0; m1 < sizeof(modifiers) / sizeof(modifiers[0]); m1++)
        {
            for (int m2 = 0; m2 < sizeof(modifiers) / sizeof(modifiers[0]); m2++)
            {
                tchar_t fmt[16] = { '%', '\0' };
                int expected = 1;

                _tcscat(fmt, modifiers[m1]);
                _tcscat(fmt, modifiers[m2]);
                _tcsncat(fmt, &format_specifier[s], 1);

                ret = p__stscanf(_T("0"), fmt, buf, _countof(buf));

                if ((_tcscmp(modifiers[m1], _T("t")) == 0) ||
                    (_tcscmp(modifiers[m1], _T("z")) == 0))
                {
                    if (modifiers[m2][0] == '\0')
                    {
                        if (!_tcschr(_T("Xdioux"), format_specifier[s]))
                        {
                            expected = 0;
                        }
                    }
                    else
                    {
                        expected = 0;
                    }
                }

                if ((_tcscmp(modifiers[m2], _T("t")) == 0) ||
                    (_tcscmp(modifiers[m2], _T("z")) == 0))
                {
                    /* If the first one is 'I', the following ones are ignored */
                    if ((_tcscmp(modifiers[m1], _T("I")) == 0))
                    {
                        expected = 1;
                    }
                    else if (!_tcschr(_T("Xdioux"), format_specifier[s]))
                    {
                        expected = 0;
                    }
                }

                ok(ret == expected, "Format '" TSTRFMT "': Expected %d, got %d\n", fmt, expected, ret);
            }
        }
    }

    /* 'I' Can act as a size prefix */
    intptr_t ptr_val = -1;
    ok_eq_int(p__stscanf(_T("deadf00d"), _T("%Ix"), &ptr_val), 1);
    ok(ptr_val == (intptr_t)0xdeadf00d, "Expected 0xdeadf00d, got 0x%Ix\n", ptr_val);

    /* 'I' can also act as a format specifier itself, if it is not followed by a valid format specifier */
    ok_eq_int(p__stscanf(_T("1234567890"), _T("%Iy#+"), &ptr_val), 1);
    ok(ptr_val == (intptr_t)1234567890, "Expected 0x1234567890, got 0x%Ix\n", ptr_val);

    /* Test NULL buffer, but only for secure API, otherwise it would crash */
#ifdef IS_SECURE_API
    ok_eq_int(p__stscanf(NULL, NULL), -1);
#endif

    /* Test NULL format string */
    ok_eq_int(p__stscanf(_T("a"), NULL), -1);

    /* Empty format string returns nothing */
    ok_eq_int(p__stscanf(_T("0"), _T(""), buf), 0);

    /* Test empty buffer */
    ok_eq_int(p__stscanf(_T(""), _T(""), buf), 0);
    ok_eq_int(p__stscanf(_T(""), _T(" "), buf), 0);
    int count1 = -1, count2 = -1;
    ok_eq_int(p__stscanf(_T(""), _T("%n"), &count1), 0);
    ok_eq_int(count1, 0);
    count1 = -1;
    ok_eq_int(p__stscanf(_T(""), _T(" %n"), &count1), 0);
    ok_eq_int(count1, 0);
    count1 = -1;
    ok_eq_int(p__stscanf(_T(""), _T("%n "), &count1), 0);
    ok_eq_int(count1, 0);
    count1 = -1;
    count2 = -1;
    ok_eq_int(p__stscanf(_T(""), _T(" %n%n "), &count1, &count2), 0);
    ok_eq_int(count1, 0);
    ok_eq_int(count2, 0);
#ifdef USE_WIDECHAR
    ok_eq_int(p__stscanf(_T(""), _T("a"), buf), 0);
    ok_eq_int(p__stscanf(_T(""), _T("%"), buf), 0);
    ok_eq_int(p__stscanf(_T(""), _T("%%"), buf), 0);
    ok_eq_int(p__stscanf(_T(""), _T("%c"), buf, 1), 0);
#else
    ok_eq_int(p__stscanf(_T(""), _T("a"), buf), -1);
    ok_eq_int(p__stscanf(_T(""), _T("%"), buf), -1);
    ok_eq_int(p__stscanf(_T(""), _T("%%"), buf), -1);
    ok_eq_int(p__stscanf(_T(""), _T("%c"), buf, 1), -1);
#endif

    /* Test white space */
    int int_val = -1;
    ok_eq_int(p__stscanf(_T(" \t 432"), _T(" %d"), &int_val), 1);
    ok_eq_int(int_val, 432);
    ok_eq_int(p__stscanf(_T("143"), _T(" %d"), &int_val), 1);
    ok_eq_int(int_val, 143);
    ok_eq_int(p__stscanf(_T(" \t 163"), _T("%d"), &int_val), 1);
    ok_eq_int(int_val, 163);

    /* Test double percent */
    ok_eq_int(p__stscanf(_T("%123"), _T("%%%d"), &int_val), 1);
    ok_eq_int(int_val, 123);
    ok_eq_int(p__stscanf(_T("%"), _T("%%"), buf), 0);




    ok_eq_int(p__stscanf(_T("0"), _T("%04x"), buf), 1);
    ok_eq_int(p__stscanf(_T("0000"), _T("%4s"), buf, _countof(buf)), 1);
    ok_eq_int(p__stscanf(_T("0"), _T("%4s"), buf, _countof(buf)), 1);

    Test_scanf_format_c();
    Test_scanf_format_C();
    Test_scanf_format_d();
    Test_scanf_format_D();
    Test_scanf_format_i();
    Test_scanf_format_I();
    Test_scanf_format_o();
    Test_scanf_format_O();
    Test_scanf_format_p();
    Test_scanf_format_P();
    Test_scanf_format_S();
    Test_scanf_format_s();
    Test_scanf_format_n();
    Test_scanf_format_u();
    Test_scanf_format_U();
    Test_scanf_format_x();
    Test_scanf_format_X();
    Test_scanf_format_scanset();

}
