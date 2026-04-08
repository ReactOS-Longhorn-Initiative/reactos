/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for qsort
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <string.h>

typedef void (__cdecl *PFN_qsort)(void*, size_t, size_t,
    int (__cdecl *)(const void*, const void*));

static int __cdecl compare_char(const void* a, const void* b)
{
    unsigned char va = *(const unsigned char*)a;
    unsigned char vb = *(const unsigned char*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

/* Compare only byte[0] of a 2-byte element.
   Byte[1] tracks the original index so we can verify
   the exact permutation produced for equal keys. */
static int __cdecl compare_key_only(const void* a, const void* b)
{
    unsigned char va = ((const unsigned char*)a)[0];
    unsigned char vb = ((const unsigned char*)b)[0];
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

START_TEST(qsort)
{
    PFN_qsort p_qsort = (PFN_qsort)load_function("qsort");

    /* === Unique char arrays at various sizes === */

    /* Zero count and zero size - no crash */
    {
        char arr[] = "X";
        p_qsort(arr, 0, 0, compare_char);
        ok(arr[0] == 'X', "qsort 0-count modified\n");
    }

    /* Non-zero count and zero size - no crash */
    {
        char arr[] = "YXZ";
        p_qsort(arr, 3, 0, compare_char);
        ok(arr[0] == 'Y', "qsort 0-count modified\n");
    }

    /* Zero count - no crash */
    {
        char arr[] = "X";
        p_qsort(arr, 0, 1, compare_char);
        ok(arr[0] == 'X', "qsort 0-count modified\n");
    }

    /* Single element */
    {
        char arr[] = "X";
        p_qsort(arr, 1, 1, compare_char);
        ok(arr[0] == 'X', "qsort 1-count modified\n");
    }

    /* 2 elements: already sorted */
    {
        char arr[] = "AB";
        p_qsort(arr, 2, 1, compare_char);
        ok(memcmp(arr, "AB", 2) == 0, "qsort 2-sorted failed\n");
    }

    /* 2 elements: swap needed */
    {
        char arr[] = "BA";
        p_qsort(arr, 2, 1, compare_char);
        ok(memcmp(arr, "AB", 2) == 0, "qsort 2-swap failed\n");
    }

    /* 3 elements: all 6 permutations */
    {
        char arr1[] = "ABC"; p_qsort(arr1, 3, 1, compare_char);
        ok(memcmp(arr1, "ABC", 3) == 0, "qsort 3-ABC failed\n");

        char arr2[] = "ACB"; p_qsort(arr2, 3, 1, compare_char);
        ok(memcmp(arr2, "ABC", 3) == 0, "qsort 3-ACB failed\n");

        char arr3[] = "BAC"; p_qsort(arr3, 3, 1, compare_char);
        ok(memcmp(arr3, "ABC", 3) == 0, "qsort 3-BAC failed\n");

        char arr4[] = "BCA"; p_qsort(arr4, 3, 1, compare_char);
        ok(memcmp(arr4, "ABC", 3) == 0, "qsort 3-BCA failed\n");

        char arr5[] = "CAB"; p_qsort(arr5, 3, 1, compare_char);
        ok(memcmp(arr5, "ABC", 3) == 0, "qsort 3-CAB failed\n");

        char arr6[] = "CBA"; p_qsort(arr6, 3, 1, compare_char);
        ok(memcmp(arr6, "ABC", 3) == 0, "qsort 3-CBA failed\n");
    }

    /* 3 elements: all same */
    {
        char arr[] = "AAA";
        p_qsort(arr, 3, 1, compare_char);
        ok(memcmp(arr, "AAA", 3) == 0, "qsort 3-allsame failed\n");
    }

    /* 3 elements: duplicates */
    {
        char arr[] = "ABA";
        p_qsort(arr, 3, 1, compare_char);
        ok(memcmp(arr, "AAB", 3) == 0, "qsort 3-dup failed\n");
    }

    /* 8 elements: reverse (at CUTOFF=8, uses shortsort) */
    {
        char arr[] = "HGFEDCBA";
        p_qsort(arr, 8, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGH", 8) == 0, "qsort 8-reverse failed\n");
    }

    /* 8 elements: already sorted */
    {
        char arr[] = "ABCDEFGH";
        p_qsort(arr, 8, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGH", 8) == 0, "qsort 8-sorted failed\n");
    }

    /* 8 elements: random pattern */
    {
        char arr[] = "DBHFAECG";
        p_qsort(arr, 8, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGH", 8) == 0, "qsort 8-random failed\n");
    }

    /* 8 elements: rotated */
    {
        char arr[] = "EFGHABCD";
        p_qsort(arr, 8, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGH", 8) == 0, "qsort 8-rotated failed\n");
    }

    /* 8 elements: all same */
    {
        char arr[] = "XXXXXXXX";
        p_qsort(arr, 8, 1, compare_char);
        ok(memcmp(arr, "XXXXXXXX", 8) == 0, "qsort 8-allsame failed\n");
    }

    /* 8 elements: duplicates */
    {
        char arr[] = "ABBAABBA";
        p_qsort(arr, 8, 1, compare_char);
        ok(memcmp(arr, "AAAABBBB", 8) == 0, "qsort 8-dup failed\n");
    }

    /* 9 elements: reverse (just past CUTOFF, uses quicksort path) */
    {
        char arr[] = "IHGFEDCBA";
        p_qsort(arr, 9, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGHI", 9) == 0, "qsort 9-reverse failed\n");
    }

    /* 9 elements: already sorted */
    {
        char arr[] = "ABCDEFGHI";
        p_qsort(arr, 9, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGHI", 9) == 0, "qsort 9-sorted failed\n");
    }

    /* 9 elements: random pattern */
    {
        char arr[] = "EIGBFDHCA";
        p_qsort(arr, 9, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGHI", 9) == 0, "qsort 9-random failed\n");
    }

    /* 9 elements: all same */
    {
        char arr[] = "AAAAAAAAA";
        p_qsort(arr, 9, 1, compare_char);
        ok(memcmp(arr, "AAAAAAAAA", 9) == 0, "qsort 9-allsame failed\n");
    }

    /* 9 elements: duplicates */
    {
        char arr[] = "CBAABCABC";
        p_qsort(arr, 9, 1, compare_char);
        ok(memcmp(arr, "AAABBBCCC", 9) == 0, "qsort 9-dup failed\n");
    }

    /* 16 elements: reverse */
    {
        char arr[] = "PONMLKJIHGFEDCBA";
        p_qsort(arr, 16, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGHIJKLMNOP", 16) == 0, "qsort 16-reverse failed\n");
    }

    /* 16 elements: already sorted */
    {
        char arr[] = "ABCDEFGHIJKLMNOP";
        p_qsort(arr, 16, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGHIJKLMNOP", 16) == 0, "qsort 16-sorted failed\n");
    }

    /* 16 elements: random pattern */
    {
        char arr[] = "HDALFPBJNECKGIOM";
        p_qsort(arr, 16, 1, compare_char);
        ok(memcmp(arr, "ABCDEFGHIJKLMNOP", 16) == 0, "qsort 16-random failed\n");
    }

    /* 16 elements: duplicates */
    {
        char arr[] = "DCBADCBADCBADCBA";
        p_qsort(arr, 16, 1, compare_char);
        ok(memcmp(arr, "AAAABBBBCCCCDDDD", 16) == 0, "qsort 16-dup failed\n");
    }

    /* Large unique char array: 256 elements reversed */
    {
        char arr[256];
        char expected[256];
        for (int i = 0; i < 256; i++)
        {
            arr[i] = (char)(255 - i);
            expected[i] = (char)i;
        }
        p_qsort(arr, 256, 1, compare_char);
        ok(memcmp(arr, expected, 256) == 0, "qsort 256-reverse failed\n");
    }

    /* === Stability tests using 2-byte (key, index) elements ===
       UCRT shortsort is a selection sort that reverses equal elements.
       This is NOT the same as insertion sort. These tests verify the
       exact permutation matches ntdll. */

    /* 2 elements: equal keys - selection sort swaps max(=first) with hi */
    {
        unsigned char arr[] = { 5,0, 5,1 };
        const unsigned char exp[] = { 5,1, 5,0 };
        p_qsort(arr, 2, 2, compare_key_only);
        ok(memcmp(arr, exp, sizeof(exp)) == 0,
            "qsort stab-2: {%d,%d}, expected {1,0}\n", arr[1], arr[3]);
    }

    /* 3 elements: all equal */
    {
        unsigned char arr[] = { 5,0, 5,1, 5,2 };
        const unsigned char exp[] = { 5,1, 5,2, 5,0 };
        p_qsort(arr, 3, 2, compare_key_only);
        ok(memcmp(arr, exp, sizeof(exp)) == 0,
            "qsort stab-3: {%d,%d,%d}, expected {1,2,0}\n",
            arr[1], arr[3], arr[5]);
    }

    /* 3 elements: two equal + one smaller */
    {
        unsigned char arr[] = { 5,0, 3,1, 5,2 };
        const unsigned char exp[] = { 3,1, 5,2, 5,0 };
        p_qsort(arr, 3, 2, compare_key_only);
        ok(memcmp(arr, exp, sizeof(exp)) == 0,
            "qsort stab-3b: idx={%d,%d,%d}, expected {1,2,0}\n",
            arr[1], arr[3], arr[5]);
    }

    /* 8 elements: all equal keys */
    {
        unsigned char arr[] = { 1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7 };
        const unsigned char exp[] = { 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7, 1,0 };
        p_qsort(arr, 8, 2, compare_key_only);
        ok(memcmp(arr, exp, sizeof(exp)) == 0,
            "qsort stab-8: {%d,%d,%d,%d,%d,%d,%d,%d}\n",
            arr[1], arr[3], arr[5], arr[7], arr[9], arr[11], arr[13], arr[15]);
    }

    /* 8 elements: two groups of equal keys */
    {
        unsigned char arr[] = { 2,0, 1,1, 2,2, 1,3, 2,4, 1,5, 2,6, 1,7 };
        const unsigned char exp[] = { 1,1, 1,5, 1,3, 1,7, 2,4, 2,6, 2,2, 2,0 };
        p_qsort(arr, 8, 2, compare_key_only);
        ok(memcmp(arr, exp, sizeof(exp)) == 0,
            "qsort stab-8-groups: {%d:%d, %d:%d, %d:%d, %d:%d, %d:%d, %d:%d, %d:%d, %d:%d}\n",
            arr[0],arr[1], arr[2],arr[3], arr[4],arr[5], arr[6],arr[7],
            arr[8],arr[9], arr[10],arr[11], arr[12],arr[13], arr[14],arr[15]);
    }
}
