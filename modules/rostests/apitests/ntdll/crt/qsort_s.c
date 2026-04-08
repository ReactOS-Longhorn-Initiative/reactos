/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for qsort_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <string.h>

typedef void (__cdecl *PFN_qsort_s)(void*, rsize_t, rsize_t,
    int (__cdecl *)(void*, const void*, const void*), void*);

static int __cdecl compare_char_s(void* context, const void* a, const void* b)
{
    unsigned char va = *(const unsigned char*)a;
    unsigned char vb = *(const unsigned char*)b;
    (void)context;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static int __cdecl compare_key_only_s(void* context, const void* a, const void* b)
{
    unsigned char va = ((const unsigned char*)a)[0];
    unsigned char vb = ((const unsigned char*)b)[0];
    (void)context;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static int __cdecl compare_char_ctx(void* context, const void* a, const void* b)
{
    unsigned char va = *(const unsigned char*)a;
    unsigned char vb = *(const unsigned char*)b;
    (*(int*)context)++;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

START_TEST(qsort_s)
{
    PFN_qsort_s p_qsort_s = (PFN_qsort_s)load_function("qsort_s");

    if (GetNTVersion() < _WIN32_WINNT_WINBLUE)
    {
        ok_eq_pointer(p_qsort_s, NULL);
        return;
    }

    if (p_qsort_s == NULL)
    {
        ok(FALSE, "qsort_s is not available\n");
        return;
    }

    /* === Unique char arrays at various sizes === */

    /* Zero count - no crash */
    {
        char arr[] = "X";
        p_qsort_s(arr, 0, 1, compare_char_s, NULL);
        ok(arr[0] == 'X', "qsort_s 0-count modified\n");
    }

    /* Single element */
    {
        char arr[] = "X";
        p_qsort_s(arr, 1, 1, compare_char_s, NULL);
        ok(arr[0] == 'X', "qsort_s 1-count modified\n");
    }

    /* 2 elements: swap needed */
    {
        char arr[] = "BA";
        p_qsort_s(arr, 2, 1, compare_char_s, NULL);
        ok(memcmp(arr, "AB", 2) == 0, "qsort_s 2-swap failed\n");
    }

    /* 3 elements: all 6 permutations */
    {
        char arr1[] = "ABC"; p_qsort_s(arr1, 3, 1, compare_char_s, NULL);
        ok(memcmp(arr1, "ABC", 3) == 0, "qsort_s 3-ABC failed\n");

        char arr2[] = "ACB"; p_qsort_s(arr2, 3, 1, compare_char_s, NULL);
        ok(memcmp(arr2, "ABC", 3) == 0, "qsort_s 3-ACB failed\n");

        char arr3[] = "BAC"; p_qsort_s(arr3, 3, 1, compare_char_s, NULL);
        ok(memcmp(arr3, "ABC", 3) == 0, "qsort_s 3-BAC failed\n");

        char arr4[] = "BCA"; p_qsort_s(arr4, 3, 1, compare_char_s, NULL);
        ok(memcmp(arr4, "ABC", 3) == 0, "qsort_s 3-BCA failed\n");

        char arr5[] = "CAB"; p_qsort_s(arr5, 3, 1, compare_char_s, NULL);
        ok(memcmp(arr5, "ABC", 3) == 0, "qsort_s 3-CAB failed\n");

        char arr6[] = "CBA"; p_qsort_s(arr6, 3, 1, compare_char_s, NULL);
        ok(memcmp(arr6, "ABC", 3) == 0, "qsort_s 3-CBA failed\n");
    }

    /* 3 elements: all same */
    {
        char arr[] = "AAA";
        p_qsort_s(arr, 3, 1, compare_char_s, NULL);
        ok(memcmp(arr, "AAA", 3) == 0, "qsort_s 3-allsame failed\n");
    }

    /* 3 elements: duplicates */
    {
        char arr[] = "ABA";
        p_qsort_s(arr, 3, 1, compare_char_s, NULL);
        ok(memcmp(arr, "AAB", 3) == 0, "qsort_s 3-dup failed\n");
    }

    /* 8 elements: reverse (at CUTOFF=8, uses shortsort) */
    {
        char arr[] = "HGFEDCBA";
        p_qsort_s(arr, 8, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGH", 8) == 0, "qsort_s 8-reverse failed\n");
    }

    /* 8 elements: already sorted */
    {
        char arr[] = "ABCDEFGH";
        p_qsort_s(arr, 8, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGH", 8) == 0, "qsort_s 8-sorted failed\n");
    }

    /* 8 elements: random pattern */
    {
        char arr[] = "DBHFAECG";
        p_qsort_s(arr, 8, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGH", 8) == 0, "qsort_s 8-random failed\n");
    }

    /* 8 elements: all same */
    {
        char arr[] = "XXXXXXXX";
        p_qsort_s(arr, 8, 1, compare_char_s, NULL);
        ok(memcmp(arr, "XXXXXXXX", 8) == 0, "qsort_s 8-allsame failed\n");
    }

    /* 8 elements: duplicates */
    {
        char arr[] = "ABBAABBA";
        p_qsort_s(arr, 8, 1, compare_char_s, NULL);
        ok(memcmp(arr, "AAAABBBB", 8) == 0, "qsort_s 8-dup failed\n");
    }

    /* 9 elements: reverse (just past CUTOFF, quicksort path) */
    {
        char arr[] = "IHGFEDCBA";
        p_qsort_s(arr, 9, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGHI", 9) == 0, "qsort_s 9-reverse failed\n");
    }

    /* 9 elements: already sorted */
    {
        char arr[] = "ABCDEFGHI";
        p_qsort_s(arr, 9, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGHI", 9) == 0, "qsort_s 9-sorted failed\n");
    }

    /* 9 elements: random pattern */
    {
        char arr[] = "EIGBFDHCA";
        p_qsort_s(arr, 9, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGHI", 9) == 0, "qsort_s 9-random failed\n");
    }

    /* 9 elements: all same */
    {
        char arr[] = "AAAAAAAAA";
        p_qsort_s(arr, 9, 1, compare_char_s, NULL);
        ok(memcmp(arr, "AAAAAAAAA", 9) == 0, "qsort_s 9-allsame failed\n");
    }

    /* 9 elements: duplicates */
    {
        char arr[] = "CBAABCABC";
        p_qsort_s(arr, 9, 1, compare_char_s, NULL);
        ok(memcmp(arr, "AAABBBCCC", 9) == 0, "qsort_s 9-dup failed\n");
    }

    /* 16 elements: reverse */
    {
        char arr[] = "PONMLKJIHGFEDCBA";
        p_qsort_s(arr, 16, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGHIJKLMNOP", 16) == 0, "qsort_s 16-reverse failed\n");
    }

    /* 16 elements: already sorted */
    {
        char arr[] = "ABCDEFGHIJKLMNOP";
        p_qsort_s(arr, 16, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGHIJKLMNOP", 16) == 0, "qsort_s 16-sorted failed\n");
    }

    /* 16 elements: random pattern */
    {
        char arr[] = "HDALFPBJNECKGIOM";
        p_qsort_s(arr, 16, 1, compare_char_s, NULL);
        ok(memcmp(arr, "ABCDEFGHIJKLMNOP", 16) == 0, "qsort_s 16-random failed\n");
    }

    /* 16 elements: duplicates */
    {
        char arr[] = "DCBADCBADCBADCBA";
        p_qsort_s(arr, 16, 1, compare_char_s, NULL);
        ok(memcmp(arr, "AAAABBBBCCCCDDDD", 16) == 0, "qsort_s 16-dup failed\n");
    }

    /* Large unique char array */
    {
        char arr[256];
        char expected[256];
        for (int i = 0; i < 256; i++)
        {
            arr[i] = (char)(255 - i);
            expected[i] = (char)i;
        }
        p_qsort_s(arr, 256, 1, compare_char_s, NULL);
        ok(memcmp(arr, expected, 256) == 0, "qsort_s 256-reverse failed\n");
    }

    /* === Stability tests === */

    /* 3 elements: all equal */
    {
        unsigned char arr[] = { 5,0, 5,1, 5,2 };
        const unsigned char exp[] = { 5,1, 5,2, 5,0 };
        p_qsort_s(arr, 3, 2, compare_key_only_s, NULL);
        ok(memcmp(arr, exp, sizeof(exp)) == 0,
            "qsort_s stab-3: {%d,%d,%d}, expected {1,2,0}\n",
            arr[1], arr[3], arr[5]);
    }

    /* 8 elements: all equal keys */
    {
        unsigned char arr[] = { 1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7 };
        const unsigned char exp[] = { 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7, 1,0 };
        p_qsort_s(arr, 8, 2, compare_key_only_s, NULL);
        ok(memcmp(arr, exp, sizeof(exp)) == 0,
            "qsort_s stab-8: {%d,%d,%d,%d,%d,%d,%d,%d}\n",
            arr[1], arr[3], arr[5], arr[7], arr[9], arr[11], arr[13], arr[15]);
    }

    /* 8 elements: two groups of equal keys */
    {
        unsigned char arr[] = { 2,0, 1,1, 2,2, 1,3, 2,4, 1,5, 2,6, 1,7 };
        const unsigned char exp[] = { 1,1, 1,5, 1,3, 1,7, 2,4, 2,6, 2,2, 2,0 };
        p_qsort_s(arr, 8, 2, compare_key_only_s, NULL);
        ok(memcmp(arr, exp, sizeof(exp)) == 0,
            "qsort_s stab-8-groups: {%d:%d, %d:%d, %d:%d, %d:%d, %d:%d, %d:%d, %d:%d, %d:%d}\n",
            arr[0],arr[1], arr[2],arr[3], arr[4],arr[5], arr[6],arr[7],
            arr[8],arr[9], arr[10],arr[11], arr[12],arr[13], arr[14],arr[15]);
    }

    /* === Context pointer pass-through === */
    {
        char arr[] = "EDCBA";
        int call_count = 0;
        p_qsort_s(arr, 5, 1, compare_char_ctx, &call_count);
        ok(memcmp(arr, "ABCDE", 5) == 0, "qsort_s ctx failed\n");
        ok(call_count > 0, "qsort_s ctx: compare not called\n");
    }
}
