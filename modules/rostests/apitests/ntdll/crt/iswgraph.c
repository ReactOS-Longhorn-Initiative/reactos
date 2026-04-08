/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswgraph
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswgraph)(wint_t c);

int __cdecl expected_iswgraph(wint_t c)
{
    if ((c >= 'A') && (c <= 'Z')) return 0x100 | _UPPER;
    if ((c >= 'a') && (c <= 'z')) return 0x100 | _LOWER;
    if ((c >= '0') && (c <= '9')) return _DIGIT;
    if ((c >= '\x21') && (c <= '\x2F')) return _PUNCT;
    if ((c >= '\x3A') && (c <= '\x40')) return _PUNCT;
    if ((c >= '\x5B') && (c <= '\x60')) return _PUNCT;
    if ((c >= '\x7B') && (c <= '\x7E')) return _PUNCT;
    if ((c >= 0xA1) && (c <= 0xB1)) return _PUNCT;
    if ((c >= 0xB2) && (c <= 0xB3)) return _PUNCT | _DIGIT;
    if ((c >= 0xB4) && (c <= 0xB8)) return _PUNCT;
    if (c == 0xB9) return _PUNCT | _DIGIT;
    if ((c >= 0xBA) && (c <= 0xBF)) return _PUNCT;
    if ((c >= 0xC0) && (c <= 0xD6)) return 0x100 | _UPPER;
    if (c == 0xD7) return _PUNCT;
    if ((c >= 0xD8) && (c <= 0xDE)) return 0x100 | _UPPER;
    if ((c >= 0xDF) && (c <= 0xF6)) return 0x100 | _LOWER;
    if (c == 0xF7) return _PUNCT;
    if ((c >= 0xF8) && (c <= 0xFF)) return 0x100 | _LOWER;
    return 0;
}

START_TEST(iswgraph)
{
    PFN_iswgraph p_iswgraph = load_function("iswgraph");

    if (p_iswgraph == NULL)
    {
        skip("Function iswgraph not available\n");
        return;
    }

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswgraph(c);
        int ret = p_iswgraph(c);
        ok(ret == expected, "iswgraph(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}
