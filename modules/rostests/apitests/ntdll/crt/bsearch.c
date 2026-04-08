/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for bsearch
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>

typedef void* (__cdecl *PFN_bsearch)(const void*, const void*, size_t, size_t,
    int (__cdecl *)(const void*, const void*));

static int __cdecl compare_int(const void* a, const void* b)
{
    int va = *(const int*)a;
    int vb = *(const int*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static int __cdecl compare_char(const void* a, const void* b)
{
    unsigned char va = *(const unsigned char*)a;
    unsigned char vb = *(const unsigned char*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

START_TEST(bsearch)
{
    PFN_bsearch p_bsearch = (PFN_bsearch)load_function("bsearch");
    void *ret;
    int key;

    /* Sorted array of ints */
    {
        const int arr[] = { 1, 3, 5, 7, 9, 11, 13, 15 };
        const size_t count = sizeof(arr) / sizeof(arr[0]);

        /* Find first element */
        key = 1;
        ret = p_bsearch(&key, arr, count, sizeof(int), compare_int);
        ok(ret == &arr[0], "bsearch(1) = %p, expected %p\n", ret, &arr[0]);

        /* Find middle element */
        key = 7;
        ret = p_bsearch(&key, arr, count, sizeof(int), compare_int);
        ok(ret == &arr[3], "bsearch(7) = %p, expected %p\n", ret, &arr[3]);

        /* Find last element */
        key = 15;
        ret = p_bsearch(&key, arr, count, sizeof(int), compare_int);
        ok(ret == &arr[7], "bsearch(15) = %p, expected %p\n", ret, &arr[7]);

        /* Element not found - below range */
        key = 0;
        ret = p_bsearch(&key, arr, count, sizeof(int), compare_int);
        ok(ret == NULL, "bsearch(0) = %p, expected NULL\n", ret);

        /* Element not found - above range */
        key = 20;
        ret = p_bsearch(&key, arr, count, sizeof(int), compare_int);
        ok(ret == NULL, "bsearch(20) = %p, expected NULL\n", ret);

        /* Element not found - in gap */
        key = 4;
        ret = p_bsearch(&key, arr, count, sizeof(int), compare_int);
        ok(ret == NULL, "bsearch(4) = %p, expected NULL\n", ret);
    }

    /* Single element array */
    {
        const int arr[] = { 42 };

        key = 42;
        ret = p_bsearch(&key, arr, 1, sizeof(int), compare_int);
        ok(ret == &arr[0], "bsearch single found = %p, expected %p\n", ret, &arr[0]);

        key = 99;
        ret = p_bsearch(&key, arr, 1, sizeof(int), compare_int);
        ok(ret == NULL, "bsearch single not found = %p, expected NULL\n", ret);
    }

    /* Two element array */
    {
        const int arr[] = { 10, 20 };

        key = 10;
        ret = p_bsearch(&key, arr, 2, sizeof(int), compare_int);
        ok(ret == &arr[0], "bsearch two first = %p, expected %p\n", ret, &arr[0]);

        key = 20;
        ret = p_bsearch(&key, arr, 2, sizeof(int), compare_int);
        ok(ret == &arr[1], "bsearch two second = %p, expected %p\n", ret, &arr[1]);

        key = 15;
        ret = p_bsearch(&key, arr, 2, sizeof(int), compare_int);
        ok(ret == NULL, "bsearch two gap = %p, expected NULL\n", ret);
    }

    /* Zero element count - returns NULL */
    {
        const int arr[] = { 1 };

        key = 1;
        ret = p_bsearch(&key, arr, 0, sizeof(int), compare_int);
        ok(ret == NULL, "bsearch zero count = %p, expected NULL\n", ret);
    }

    /* Large array - all elements findable */
    {
        int arr[256];
        for (int i = 0; i < 256; i++)
            arr[i] = i * 3;

        key = 0;
        ret = p_bsearch(&key, arr, 256, sizeof(int), compare_int);
        ok(ret == &arr[0], "bsearch large first = %p, expected %p\n", ret, &arr[0]);

        key = 255 * 3;
        ret = p_bsearch(&key, arr, 256, sizeof(int), compare_int);
        ok(ret == &arr[255], "bsearch large last = %p, expected %p\n", ret, &arr[255]);

        key = 128 * 3;
        ret = p_bsearch(&key, arr, 256, sizeof(int), compare_int);
        ok(ret == &arr[128], "bsearch large mid = %p, expected %p\n", ret, &arr[128]);

        key = 1;
        ret = p_bsearch(&key, arr, 256, sizeof(int), compare_int);
        ok(ret == NULL, "bsearch large gap = %p, expected NULL\n", ret);
    }

    /* Negative values */
    {
        const int arr[] = { -10, -5, 0, 5, 10 };

        key = -5;
        ret = p_bsearch(&key, arr, 5, sizeof(int), compare_int);
        ok(ret == &arr[1], "bsearch neg = %p, expected %p\n", ret, &arr[1]);

        key = 0;
        ret = p_bsearch(&key, arr, 5, sizeof(int), compare_int);
        ok(ret == &arr[2], "bsearch zero = %p, expected %p\n", ret, &arr[2]);
    }

    /* Array of chars with sizeof 1 */
    {
        const unsigned char arr[] = { 'A', 'B', 'C', 'D', 'E' };
        unsigned char ckey;

        ckey = 'C';
        ret = p_bsearch(&ckey, arr, 5, 1, compare_char);
        ok(ret == &arr[2], "bsearch char = %p, expected %p\n", ret, &arr[2]);

        ckey = 'Z';
        ret = p_bsearch(&ckey, arr, 5, 1, compare_char);
        ok(ret == NULL, "bsearch char miss = %p, expected NULL\n", ret);
    }
}
