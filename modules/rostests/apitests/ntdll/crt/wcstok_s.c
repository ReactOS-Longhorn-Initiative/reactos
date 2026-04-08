/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcstok_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef wchar_t* (__cdecl *PFN_wcstok_s)(wchar_t* _String, const wchar_t* _Delimiter, wchar_t** _Context);

START_TEST(wcstok_s)
{
    PFN_wcstok_s p_wcstok_s = (PFN_wcstok_s)load_function("wcstok_s");
    wchar_t buf[64];
    wchar_t *ctx;
    wchar_t *ret;

    if (GetNTVersion() < _WIN32_WINNT_WINBLUE)
    {
        ok_eq_pointer(p_wcstok_s, NULL);
        return;
    }

    if (p_wcstok_s == NULL)
    {
        ok(FALSE, "wcstok_s is not available\n");
        return;
    }

    /* Basic tokenization with single delimiter */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, L"Hello World Foo\0", 16 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L" ", &ctx);
    ok(ret == buf, "wcstok_s first returned %p, expected %p\n", ret, buf);
    ok(memcmp(ret, L"Hello\0", 6 * sizeof(wchar_t)) == 0, "wcstok_s first token mismatch\n");

    ret = p_wcstok_s(NULL, L" ", &ctx);
    ok(ret == buf + 6, "wcstok_s second returned %p, expected %p\n", ret, buf + 6);
    ok(memcmp(ret, L"World\0", 6 * sizeof(wchar_t)) == 0, "wcstok_s second token mismatch\n");

    ret = p_wcstok_s(NULL, L" ", &ctx);
    ok(ret == buf + 12, "wcstok_s third returned %p, expected %p\n", ret, buf + 12);
    ok(memcmp(ret, L"Foo\0", 4 * sizeof(wchar_t)) == 0, "wcstok_s third token mismatch\n");

    ret = p_wcstok_s(NULL, L" ", &ctx);
    ok(ret == NULL, "wcstok_s end returned %p, expected NULL\n", ret);

    /* Verify buffer: delimiters replaced with null */
    ok(memcmp(buf, L"Hello\0World\0Foo\0", 16 * sizeof(wchar_t)) == 0,
       "wcstok_s buffer content mismatch\n");
    ok(buf[16] == (wchar_t)0xAAAA, "wcstok_s wrote past end\n");

    /* Multiple delimiters */
    memcpy(buf, L"a,b;c.d\0", 8 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L",;.", &ctx);
    ok((ret == buf) && (memcmp(ret, L"a\0", 2 * sizeof(wchar_t)) == 0), "wcstok_s multi first mismatch\n");

    ret = p_wcstok_s(NULL, L",;.", &ctx);
    ok((ret == buf + 2) && (memcmp(ret, L"b\0", 2 * sizeof(wchar_t)) == 0), "wcstok_s multi second mismatch\n");

    ret = p_wcstok_s(NULL, L",;.", &ctx);
    ok((ret == buf + 4) && (memcmp(ret, L"c\0", 2 * sizeof(wchar_t)) == 0), "wcstok_s multi third mismatch\n");

    ret = p_wcstok_s(NULL, L",;.", &ctx);
    ok((ret == buf + 6) && (memcmp(ret, L"d\0", 2 * sizeof(wchar_t)) == 0), "wcstok_s multi fourth mismatch\n");

    ret = p_wcstok_s(NULL, L",;.", &ctx);
    ok(ret == NULL, "wcstok_s multi end returned %p, expected NULL\n", ret);

    /* Leading delimiters are skipped */
    memcpy(buf, L",,hello,,world,,\0", 17 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L",", &ctx);
    ok((ret != NULL) && (memcmp(ret, L"hello\0", 6 * sizeof(wchar_t)) == 0), "wcstok_s leading first mismatch\n");

    ret = p_wcstok_s(NULL, L",", &ctx);
    ok((ret != NULL) && (memcmp(ret, L"world\0", 6 * sizeof(wchar_t)) == 0), "wcstok_s leading second mismatch\n");

    ret = p_wcstok_s(NULL, L",", &ctx);
    ok(ret == NULL, "wcstok_s leading end returned %p, expected NULL\n", ret);

    /* String is only delimiters */
    memcpy(buf, L",,,,\0", 5 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L",", &ctx);
    ok(ret == NULL, "wcstok_s all-delim returned %p, expected NULL\n", ret);

    /* Empty string */
    buf[0] = L'\0';
    ret = p_wcstok_s(buf, L",", &ctx);
    ok(ret == NULL, "wcstok_s empty returned %p, expected NULL\n", ret);

    /* Empty delimiter set - entire string is one token */
    memcpy(buf, L"hello\0", 6 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L"", &ctx);
    ok((ret == buf) && (memcmp(ret, L"hello\0", 6 * sizeof(wchar_t)) == 0), "wcstok_s empty delim mismatch\n");

    ret = p_wcstok_s(NULL, L"", &ctx);
    ok(ret == NULL, "wcstok_s empty delim end returned %p, expected NULL\n", ret);

    /* Single char string, not a delimiter */
    memcpy(buf, L"x\0", 2 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L",", &ctx);
    ok((ret == buf) && (memcmp(ret, L"x\0", 2 * sizeof(wchar_t)) == 0), "wcstok_s single char mismatch\n");

    ret = p_wcstok_s(NULL, L",", &ctx);
    ok(ret == NULL, "wcstok_s single char end returned %p, expected NULL\n", ret);

    /* Single char string, is a delimiter */
    memcpy(buf, L",\0", 2 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L",", &ctx);
    ok(ret == NULL, "wcstok_s single delim returned %p, expected NULL\n", ret);

    /* Change delimiter between calls */
    memcpy(buf, L"a,b;c\0", 6 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L",", &ctx);
    ok((ret == buf) && (memcmp(ret, L"a\0", 2 * sizeof(wchar_t)) == 0), "wcstok_s change first mismatch\n");

    ret = p_wcstok_s(NULL, L";", &ctx);
    ok((ret == buf + 2) && (memcmp(ret, L"b\0", 2 * sizeof(wchar_t)) == 0), "wcstok_s change second mismatch\n");

    ret = p_wcstok_s(NULL, L";", &ctx);
    ok((ret == buf + 4) && (memcmp(ret, L"c\0", 2 * sizeof(wchar_t)) == 0), "wcstok_s change third mismatch\n");

    ret = p_wcstok_s(NULL, L";", &ctx);
    ok(ret == NULL, "wcstok_s change end returned %p, expected NULL\n", ret);

    /* High Unicode values as delimiters */
    memcpy(buf, L"abc\xFF00" L"def\xFFFE" L"ghi\0", 12 * sizeof(wchar_t));
    ret = p_wcstok_s(buf, L"\xFF00\xFFFE", &ctx);
    ok((ret == buf) && (memcmp(ret, L"abc\0", 4 * sizeof(wchar_t)) == 0), "wcstok_s high first mismatch\n");

    ret = p_wcstok_s(NULL, L"\xFF00\xFFFE", &ctx);
    ok((ret != NULL) && (memcmp(ret, L"def\0", 4 * sizeof(wchar_t)) == 0), "wcstok_s high second mismatch\n");

    ret = p_wcstok_s(NULL, L"\xFF00\xFFFE", &ctx);
    ok((ret != NULL) && (memcmp(ret, L"ghi\0", 4 * sizeof(wchar_t)) == 0), "wcstok_s high third mismatch\n");

    ret = p_wcstok_s(NULL, L"\xFF00\xFFFE", &ctx);
    ok(ret == NULL, "wcstok_s high end returned %p, expected NULL\n", ret);
}
