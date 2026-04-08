/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Template implementation of qsort / qsort_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include <search.h>

#define CUTOFF 8
#define STKSIZ (8 * sizeof(void*) - 2)

static
void
_swap(char* A, char* B, size_t Width)
{
    if (A != B)
    {
        while (Width--)
        {
            char const temp = *A;
            *A++ = *B;
            *B++ = temp;
        }
    }
}

template<typename TCompare>
static
void
_shortsort_impl(
    char* Lo,
    char* Hi,
    size_t const Width,
    TCompare const Compare)
{
    while (Hi > Lo)
    {
        char* max = Lo;
        for (char* p = Lo + Width; p <= Hi; p += Width)
        {
            if (Compare(p, max) > 0)
            {
                max = p;
            }
        }

        _swap(max, Hi, Width);
        Hi -= Width;
    }
}

template<typename TCompare>
static
void
_qsort_impl(
    void* const Base,
    size_t const NumOfElements,
    size_t const SizeOfElements,
    TCompare const Compare)
{
    char* lostk[STKSIZ];
    char* histk[STKSIZ];
    int stkptr = 0;
    char* lo;
    char* hi;

    /* One element is always sorted */
    if (NumOfElements < 2)
    {
        return;
    }

    /* If size is 0 there is nothing to sort */
    if (SizeOfElements == 0)
    {
        return;
    }

    lo = static_cast<char*>(Base);
    hi = static_cast<char*>(Base) + SizeOfElements * (NumOfElements - 1);

recurse:
    {
        size_t size = static_cast<size_t>(hi - lo) / SizeOfElements + 1;

        if (size <= CUTOFF)
        {
            _shortsort_impl(lo, hi, SizeOfElements, Compare);
        }
        else
        {
            char* mid = lo + (size / 2) * SizeOfElements;

            /* Median-of-three pivot selection */
            if (Compare(lo, mid) > 0)
                _swap(lo, mid, SizeOfElements);
            if (Compare(lo, hi) > 0)
                _swap(lo, hi, SizeOfElements);
            if (Compare(mid, hi) > 0)
                _swap(mid, hi, SizeOfElements);

            char* loguy = lo;
            char* higuy = hi;

            for (;;)
            {
                /* Scan right, skipping past mid to avoid self-comparison */
                if (mid > loguy)
                {
                    do { loguy += SizeOfElements; }
                    while ((loguy < mid) && (Compare(loguy, mid) <= 0));
                }
                if (mid <= loguy)
                {
                    do { loguy += SizeOfElements; }
                    while ((loguy <= hi) && (Compare(loguy, mid) <= 0));
                }

                /* Scan left */
                do { higuy -= SizeOfElements; }
                while ((higuy > mid) && (Compare(higuy, mid) > 0));

                if (higuy < loguy)
                    break;

                _swap(loguy, higuy, SizeOfElements);

                /* If the pivot was moved, follow it */
                if (mid == higuy)
                    mid = loguy;
            }

            /* Shrink the equal-to-pivot region */
            higuy += SizeOfElements;
            if (mid < higuy)
            {
                do { higuy -= SizeOfElements; }
                while ((higuy > mid) && (Compare(higuy, mid) == 0));
            }
            if (mid >= higuy)
            {
                do { higuy -= SizeOfElements; }
                while ((higuy > lo) && (Compare(higuy, mid) == 0));
            }

            /* Sort the smaller partition first to minimize stack usage */
            if ((higuy - lo) >= (hi - loguy))
            {
                if (lo < higuy)
                {
                    lostk[stkptr] = lo;
                    histk[stkptr] = higuy;
                    ++stkptr;
                }

                if (loguy < hi)
                {
                    lo = loguy;
                    goto recurse;
                }
            }
            else
            {
                if (loguy < hi)
                {
                    lostk[stkptr] = loguy;
                    histk[stkptr] = hi;
                    ++stkptr;
                }

                if (lo < higuy)
                {
                    hi = higuy;
                    goto recurse;
                }
            }
        }

        --stkptr;
        if (stkptr >= 0)
        {
            lo = lostk[stkptr];
            hi = histk[stkptr];
            goto recurse;
        }
    }
}
