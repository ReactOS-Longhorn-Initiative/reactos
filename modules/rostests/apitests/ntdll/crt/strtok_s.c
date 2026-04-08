/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strtok_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef char* (__cdecl *PFN_strtok_s)(char* _String, const char* _Delimiter, char** _Context);

START_TEST(strtok_s)
{
    PFN_strtok_s p_strtok_s = (PFN_strtok_s)load_function("strtok_s");
    char buf[64];
    char *ctx;
    char *ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_strtok_s, NULL);
        return;
    }

    if (p_strtok_s == NULL)
    {
        ok(FALSE, "strtok_s is not available\n");
        return;
    }

    /* Basic tokenization with single delimiter */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "Hello World Foo\0", 16);
    ret = p_strtok_s(buf, " ", &ctx);
    ok(ret == buf, "strtok_s first returned %p, expected %p\n", ret, buf);
    ok(memcmp(ret, "Hello\0", 6) == 0, "strtok_s first token mismatch\n");

    ret = p_strtok_s(NULL, " ", &ctx);
    ok(ret == buf + 6, "strtok_s second returned %p, expected %p\n", ret, buf + 6);
    ok(memcmp(ret, "World\0", 6) == 0, "strtok_s second token mismatch\n");

    ret = p_strtok_s(NULL, " ", &ctx);
    ok(ret == buf + 12, "strtok_s third returned %p, expected %p\n", ret, buf + 12);
    ok(memcmp(ret, "Foo\0", 4) == 0, "strtok_s third token mismatch\n");

    ret = p_strtok_s(NULL, " ", &ctx);
    ok(ret == NULL, "strtok_s end returned %p, expected NULL\n", ret);

    /* Verify buffer: delimiters replaced with null */
    ok(memcmp(buf, "Hello\0World\0Foo\0\xAA", 17) == 0, "strtok_s buffer content mismatch\n");

    /* Multiple delimiters */
    memcpy(buf, "a,b;c.d\0", 8);
    ret = p_strtok_s(buf, ",;.", &ctx);
    ok((ret == buf) && (memcmp(ret, "a\0", 2) == 0), "strtok_s multi-delim first mismatch\n");

    ret = p_strtok_s(NULL, ",;.", &ctx);
    ok((ret == buf + 2) && (memcmp(ret, "b\0", 2) == 0), "strtok_s multi-delim second mismatch\n");

    ret = p_strtok_s(NULL, ",;.", &ctx);
    ok((ret == buf + 4) && (memcmp(ret, "c\0", 2) == 0), "strtok_s multi-delim third mismatch\n");

    ret = p_strtok_s(NULL, ",;.", &ctx);
    ok((ret == buf + 6) && (memcmp(ret, "d\0", 2) == 0), "strtok_s multi-delim fourth mismatch\n");

    ret = p_strtok_s(NULL, ",;.", &ctx);
    ok(ret == NULL, "strtok_s multi-delim end returned %p, expected NULL\n", ret);

    /* Leading delimiters are skipped */
    memcpy(buf, ",,hello,,world,,\0", 17);
    ret = p_strtok_s(buf, ",", &ctx);
    ok((ret != NULL) && (memcmp(ret, "hello\0", 6) == 0), "strtok_s leading delim first mismatch\n");

    ret = p_strtok_s(NULL, ",", &ctx);
    ok((ret != NULL) && (memcmp(ret, "world\0", 6) == 0), "strtok_s leading delim second mismatch\n");

    ret = p_strtok_s(NULL, ",", &ctx);
    ok(ret == NULL, "strtok_s leading delim end returned %p, expected NULL\n", ret);

    /* String is only delimiters */
    memcpy(buf, ",,,,\0", 5);
    ret = p_strtok_s(buf, ",", &ctx);
    ok(ret == NULL, "strtok_s all-delim returned %p, expected NULL\n", ret);

    /* Empty string */
    buf[0] = '\0';
    ret = p_strtok_s(buf, ",", &ctx);
    ok(ret == NULL, "strtok_s empty returned %p, expected NULL\n", ret);

    /* Empty delimiter set - entire string is one token */
    memcpy(buf, "hello\0", 6);
    ret = p_strtok_s(buf, "", &ctx);
    ok((ret == buf) && (memcmp(ret, "hello\0", 6) == 0), "strtok_s empty delim mismatch\n");

    ret = p_strtok_s(NULL, "", &ctx);
    ok(ret == NULL, "strtok_s empty delim end returned %p, expected NULL\n", ret);

    /* Single char string, not a delimiter */
    memcpy(buf, "x\0", 2);
    ret = p_strtok_s(buf, ",", &ctx);
    ok((ret == buf) && (memcmp(ret, "x\0", 2) == 0), "strtok_s single char mismatch\n");

    ret = p_strtok_s(NULL, ",", &ctx);
    ok(ret == NULL, "strtok_s single char end returned %p, expected NULL\n", ret);

    /* Single char string, is a delimiter */
    memcpy(buf, ",\0", 2);
    ret = p_strtok_s(buf, ",", &ctx);
    ok(ret == NULL, "strtok_s single delim returned %p, expected NULL\n", ret);

    /* Change delimiter between calls */
    memcpy(buf, "a,b;c\0", 6);
    ret = p_strtok_s(buf, ",", &ctx);
    ok((ret == buf) && (memcmp(ret, "a\0", 2) == 0), "strtok_s change delim first mismatch\n");

    ret = p_strtok_s(NULL, ";", &ctx);
    ok((ret == buf + 2) && (memcmp(ret, "b\0", 2) == 0), "strtok_s change delim second mismatch\n");

    ret = p_strtok_s(NULL, ";", &ctx);
    ok((ret == buf + 4) && (memcmp(ret, "c\0", 2) == 0), "strtok_s change delim third mismatch\n");

    ret = p_strtok_s(NULL, ";", &ctx);
    ok(ret == NULL, "strtok_s change delim end returned %p, expected NULL\n", ret);

    /* High byte values as delimiters */
    memcpy(buf, "abc\x80" "def\xFE" "ghi\0", 12);
    ret = p_strtok_s(buf, "\x80\xFE", &ctx);
    ok((ret == buf) && (memcmp(ret, "abc\0", 4) == 0), "strtok_s high delim first mismatch\n");

    ret = p_strtok_s(NULL, "\x80\xFE", &ctx);
    ok((ret != NULL) && (memcmp(ret, "def\0", 4) == 0), "strtok_s high delim second mismatch\n");

    ret = p_strtok_s(NULL, "\x80\xFE", &ctx);
    ok((ret != NULL) && (memcmp(ret, "ghi\0", 4) == 0), "strtok_s high delim third mismatch\n");

    ret = p_strtok_s(NULL, "\x80\xFE", &ctx);
    ok(ret == NULL, "strtok_s high delim end returned %p, expected NULL\n", ret);
}
