/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcsncpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef wchar_t* (__cdecl *PFN_wcsncpy)(wchar_t* _Destination, const wchar_t* _Source, size_t _Count);

START_TEST(wcsncpy)
{
    PFN_wcsncpy p_wcsncpy = (PFN_wcsncpy)load_function("wcsncpy");
    wchar_t dst[64];
    wchar_t *ret;

    /* Source shorter than count - pads with nulls */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy(dst, L"Hi", 5);
    ok(ret == dst, "wcsncpy returned %p, expected %p\n", ret, dst);
    ok(dst[0] == L'H', "wcsncpy dst[0] = 0x%04x\n", (unsigned)dst[0]);
    ok(dst[1] == L'i', "wcsncpy dst[1] = 0x%04x\n", (unsigned)dst[1]);
    ok(dst[2] == L'\0', "wcsncpy dst[2] = 0x%04x, expected 0\n", (unsigned)dst[2]);
    ok(dst[3] == L'\0', "wcsncpy dst[3] = 0x%04x, expected 0 (padding)\n", (unsigned)dst[3]);
    ok(dst[4] == L'\0', "wcsncpy dst[4] = 0x%04x, expected 0 (padding)\n", (unsigned)dst[4]);
    ok(dst[5] == (wchar_t)0xAAAA, "wcsncpy wrote past count\n");

    /* Source longer than count - no null terminator */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy(dst, L"Hello World", 5);
    ok(ret == dst, "wcsncpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, L"Hello", 5 * sizeof(wchar_t)) == 0, "wcsncpy content mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcsncpy wrote past count\n");

    /* Source exactly count length - no null terminator */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy(dst, L"ABCDE", 5);
    ok(ret == dst, "wcsncpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, L"ABCDE", 5 * sizeof(wchar_t)) == 0, "wcsncpy exact content mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcsncpy exact wrote past count\n");

    /* Count of zero */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy(dst, L"Hello", 0);
    ok(ret == dst, "wcsncpy(count=0) returned %p, expected %p\n", ret, dst);
    ok(dst[0] == (wchar_t)0xAAAA, "wcsncpy(count=0) modified dst\n");

    /* Empty source with count > 0 */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy(dst, L"", 3);
    ok(ret == dst, "wcsncpy returned %p, expected %p\n", ret, dst);
    ok(dst[0] == L'\0', "wcsncpy empty dst[0] = 0x%04x\n", (unsigned)dst[0]);
    ok(dst[1] == L'\0', "wcsncpy empty dst[1] = 0x%04x\n", (unsigned)dst[1]);
    ok(dst[2] == L'\0', "wcsncpy empty dst[2] = 0x%04x\n", (unsigned)dst[2]);
    ok(dst[3] == (wchar_t)0xAAAA, "wcsncpy empty wrote past count\n");

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsncpy(dst, dst, 5);
    ok(ret == dst, "wcsncpy self returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, L"Hello", 5 * sizeof(wchar_t)) == 0, "wcsncpy self content mismatch\n");

    /* Overlapping: src at positive offset from dst */
    memcpy(dst, L"Hello World\0", 12 * sizeof(wchar_t));
    ret = p_wcsncpy(dst, dst + 6, 5);
    ok(ret == dst, "wcsncpy fwd-overlap returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, L"World", 5 * sizeof(wchar_t)) == 0, "wcsncpy fwd-overlap content mismatch\n");

    /* Overlapping: dst at positive offset from src */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsncpy(dst + 2, dst, 8);
    ok(ret == dst + 2, "wcsncpy bwd-overlap returned %p, expected dst + 2 (%p)\n", ret, dst + 2);
    ok(memcmp(dst, L"HeHeHeHeHe\xAAAA\xAAAA", 12 * sizeof(wchar_t)) == 0, "wcsncpy bwd-overlap content mismatch\n");
}
