/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for bsearch_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>

typedef void* (__cdecl *PFN_bsearch_s)(const void*, const void*, rsize_t, rsize_t,
    int (__cdecl *)(void*, const void*, const void*), void*);

static int __cdecl compare_int_s(void* context, const void* a, const void* b)
{
    int va = *(const int*)a;
    int vb = *(const int*)b;
    (void)context;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static int __cdecl compare_int_ctx(void* context, const void* a, const void* b)
{
    int va = *(const int*)a;
    int vb = *(const int*)b;
    /* Use context as a call counter */
    (*(int*)context)++;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

START_TEST(bsearch_s)
{
    PFN_bsearch_s p_bsearch_s = (PFN_bsearch_s)load_function("bsearch_s");
    void *ret;
    int key;

    if (p_bsearch_s == NULL)
    {
        skip("Function bsearch_s not available\n");
        return;
    }

    /* Sorted array of ints */
    {
        const int arr[] = { 1, 3, 5, 7, 9, 11, 13, 15 };
        const size_t count = sizeof(arr) / sizeof(arr[0]);

        /* Find first element */
        key = 1;
        ret = p_bsearch_s(&key, arr, count, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[0], "bsearch_s(1) = %p, expected %p\n", ret, &arr[0]);

        /* Find middle element */
        key = 7;
        ret = p_bsearch_s(&key, arr, count, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[3], "bsearch_s(7) = %p, expected %p\n", ret, &arr[3]);

        /* Find last element */
        key = 15;
        ret = p_bsearch_s(&key, arr, count, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[7], "bsearch_s(15) = %p, expected %p\n", ret, &arr[7]);

        /* Element not found - below range */
        key = 0;
        ret = p_bsearch_s(&key, arr, count, sizeof(int), compare_int_s, NULL);
        ok(ret == NULL, "bsearch_s(0) = %p, expected NULL\n", ret);

        /* Element not found - above range */
        key = 20;
        ret = p_bsearch_s(&key, arr, count, sizeof(int), compare_int_s, NULL);
        ok(ret == NULL, "bsearch_s(20) = %p, expected NULL\n", ret);

        /* Element not found - in gap */
        key = 4;
        ret = p_bsearch_s(&key, arr, count, sizeof(int), compare_int_s, NULL);
        ok(ret == NULL, "bsearch_s(4) = %p, expected NULL\n", ret);
    }

    /* Single element array */
    {
        const int arr[] = { 42 };

        key = 42;
        ret = p_bsearch_s(&key, arr, 1, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[0], "bsearch_s single found = %p, expected %p\n", ret, &arr[0]);

        key = 99;
        ret = p_bsearch_s(&key, arr, 1, sizeof(int), compare_int_s, NULL);
        ok(ret == NULL, "bsearch_s single not found = %p, expected NULL\n", ret);
    }

    /* Two element array */
    {
        const int arr[] = { 10, 20 };

        key = 10;
        ret = p_bsearch_s(&key, arr, 2, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[0], "bsearch_s two first = %p, expected %p\n", ret, &arr[0]);

        key = 20;
        ret = p_bsearch_s(&key, arr, 2, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[1], "bsearch_s two second = %p, expected %p\n", ret, &arr[1]);

        key = 15;
        ret = p_bsearch_s(&key, arr, 2, sizeof(int), compare_int_s, NULL);
        ok(ret == NULL, "bsearch_s two gap = %p, expected NULL\n", ret);
    }

    /* Zero element count - returns NULL */
    {
        const int arr[] = { 1 };

        key = 1;
        ret = p_bsearch_s(&key, arr, 0, sizeof(int), compare_int_s, NULL);
        ok(ret == NULL, "bsearch_s zero count = %p, expected NULL\n", ret);
    }

    /* Context pointer is passed through */
    {
        const int arr[] = { 1, 3, 5, 7, 9 };
        int call_count = 0;

        key = 5;
        ret = p_bsearch_s(&key, arr, 5, sizeof(int), compare_int_ctx, &call_count);
        ok(ret == &arr[2], "bsearch_s ctx found = %p, expected %p\n", ret, &arr[2]);
        ok(call_count > 0, "bsearch_s ctx: compare not called\n");

        call_count = 0;
        key = 4;
        ret = p_bsearch_s(&key, arr, 5, sizeof(int), compare_int_ctx, &call_count);
        ok(ret == NULL, "bsearch_s ctx not found = %p, expected NULL\n", ret);
        ok(call_count > 0, "bsearch_s ctx miss: compare not called\n");
    }

    /* Large array */
    {
        int arr[256];
        for (int i = 0; i < 256; i++)
            arr[i] = i * 3;

        key = 0;
        ret = p_bsearch_s(&key, arr, 256, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[0], "bsearch_s large first = %p, expected %p\n", ret, &arr[0]);

        key = 255 * 3;
        ret = p_bsearch_s(&key, arr, 256, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[255], "bsearch_s large last = %p, expected %p\n", ret, &arr[255]);

        key = 128 * 3;
        ret = p_bsearch_s(&key, arr, 256, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[128], "bsearch_s large mid = %p, expected %p\n", ret, &arr[128]);

        key = 1;
        ret = p_bsearch_s(&key, arr, 256, sizeof(int), compare_int_s, NULL);
        ok(ret == NULL, "bsearch_s large gap = %p, expected NULL\n", ret);
    }

    /* Negative values */
    {
        const int arr[] = { -10, -5, 0, 5, 10 };

        key = -5;
        ret = p_bsearch_s(&key, arr, 5, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[1], "bsearch_s neg = %p, expected %p\n", ret, &arr[1]);

        key = 0;
        ret = p_bsearch_s(&key, arr, 5, sizeof(int), compare_int_s, NULL);
        ok(ret == &arr[2], "bsearch_s zero = %p, expected %p\n", ret, &arr[2]);
    }

    // TODO: invalid parameters, e.g., NULL key, NULL array, size 0, element size 0, etc. and possibly set errno to EINVAL
}
