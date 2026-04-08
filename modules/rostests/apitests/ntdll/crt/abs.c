/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for abs
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <limits.h>

typedef int (__cdecl *PFN_abs)(int _Number);

START_TEST(abs)
{
    PFN_abs p_abs = (PFN_abs)load_function("abs");

    /* Test zero */
    ok_eq_int(p_abs(0), 0);

    /* Test positive values */
    ok_eq_int(p_abs(1), 1);
    ok_eq_int(p_abs(42), 42);
    ok_eq_int(p_abs(INT_MAX), INT_MAX);

    /* Test negative values */
    ok_eq_int(p_abs(-1), 1);
    ok_eq_int(p_abs(-42), 42);
    ok_eq_int(p_abs(-INT_MAX), INT_MAX);

    /* Test INT_MIN - this is undefined behavior in C, but ntdll returns INT_MIN */
    ok_eq_int(p_abs(INT_MIN), INT_MIN);
}
