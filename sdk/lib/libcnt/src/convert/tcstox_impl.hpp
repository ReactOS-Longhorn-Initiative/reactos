/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Template implementation of string-to-integer conversion
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include <stdlib.h>
#include <limits.h>
#include <errno.h>

template<typename TChar>
static
int
_get_digit(
    TChar const _Ch,
    int const _Radix)
{
    int digit;

    if ((_Ch >= '0') && (_Ch <= '9'))
        digit = _Ch - '0';
    else if ((_Ch >= 'A') && (_Ch <= 'Z'))
        digit = _Ch - 'A' + 10;
    else if ((_Ch >= 'a') && (_Ch <= 'z'))
        digit = _Ch - 'a' + 10;
    else if (sizeof(TChar) > 1)
    {
        /* Unicode digit ranges recognized by ntdll's wide functions */
        if ((_Ch >= 0x0660) && (_Ch <= 0x0669))         /* Arabic-Indic */
            digit = _Ch - 0x0660;
        else if ((_Ch >= 0x06F0) && (_Ch <= 0x06F9))     /* Extended Arabic-Indic */
            digit = _Ch - 0x06F0;
        else if ((_Ch >= 0x0966) && (_Ch <= 0x096F))     /* Devanagari */
            digit = _Ch - 0x0966;
        else if ((_Ch >= 0x09E6) && (_Ch <= 0x09EF))     /* Bengali */
            digit = _Ch - 0x09E6;
        else if ((_Ch >= 0x0A66) && (_Ch <= 0x0A6F))     /* Gurmukhi */
            digit = _Ch - 0x0A66;
        else if ((_Ch >= 0x0AE6) && (_Ch <= 0x0AEF))     /* Gujarati */
            digit = _Ch - 0x0AE6;
        else if ((_Ch >= 0x0B66) && (_Ch <= 0x0B6F))     /* Oriya */
            digit = _Ch - 0x0B66;
        else if ((_Ch >= 0x0C66) && (_Ch <= 0x0C6F))     /* Telugu */
            digit = _Ch - 0x0C66;
        else if ((_Ch >= 0x0CE6) && (_Ch <= 0x0CEF))     /* Kannada */
            digit = _Ch - 0x0CE6;
        else if ((_Ch >= 0x0D66) && (_Ch <= 0x0D6F))     /* Malayalam */
            digit = _Ch - 0x0D66;
        else if ((_Ch >= 0x0E50) && (_Ch <= 0x0E59))     /* Thai */
            digit = _Ch - 0x0E50;
        else if ((_Ch >= 0x0ED0) && (_Ch <= 0x0ED9))     /* Lao */
            digit = _Ch - 0x0ED0;
        else if ((_Ch >= 0x0F20) && (_Ch <= 0x0F29))     /* Tibetan */
            digit = _Ch - 0x0F20;
        else if ((_Ch >= 0x1040) && (_Ch <= 0x1049))     /* Myanmar */
            digit = _Ch - 0x1040;
        else if ((_Ch >= 0x17E0) && (_Ch <= 0x17E9))     /* Khmer */
            digit = _Ch - 0x17E0;
        else if ((_Ch >= 0x1810) && (_Ch <= 0x1819))     /* Mongolian */
            digit = _Ch - 0x1810;
        else if ((_Ch >= 0xFF10) && (_Ch <= 0xFF19))     /* Fullwidth */
            digit = _Ch - 0xFF10;
        else
            return -1;
    }
    else
        return -1;

    if (digit >= _Radix)
        return -1;

    return digit;
}

template<typename TChar>
static
bool
_is_space(
    TChar const Chr)
{
    return ((Chr == ' ') ||
            ((Chr >= 9) && (Chr <= 13)) ||
            ((sizeof(Chr) == sizeof(wchar_t)) && (Chr == 0x00A0)));
}

template<typename TResult, typename TChar>
static inline
TResult
_tcstox_impl(
    TChar const* const _String,
    TChar** const _EndPtr,
    int _Radix,
    bool const _IsSigned)
{
    typedef unsigned __int64 TUnsigned;

    TChar const* p = _String;
    bool negative = false;
    TUnsigned result = 0;
    TChar const* num_start;
    bool overflow = false;

    /* Reject invalid radix values */
    if ((_Radix != 0) && ((_Radix < 2) || (_Radix > 36)))
    {
        if (_EndPtr != NULL)
        {
            *_EndPtr = (TChar*)_String;
        }
        return 0;
    }

    /* Skip leading whitespace */
    while (_is_space(*p))
    {
        p++;
    }

    /* Handle sign */
    if (*p == '+')
    {
        p++;
    }
    else if (*p == '-')
    {
        negative = true;
        p++;
    }

    /* Detect radix from prefix */
    if (_Radix == 0)
    {
        if (_get_digit(*p, 10) == 0)
        {
            if ((p[1] == 'x') || (p[1] == 'X'))
            {
                _Radix = 16;
                p += 2;
            }
            else
            {
                _Radix = 8;
            }
        }
        else
        {
            _Radix = 10;
        }
    }
    else if (_Radix == 16)
    {
        /* Skip optional 0x prefix for hex */
        if ((_get_digit(*p, 10) == 0) && ((p[1] == 'x') || (p[1] == 'X')))
        {
            p += 2;
        }
    }

    num_start = p;

    /* Determine the overflow limit */
    TUnsigned max_value;
    if (_IsSigned)
    {
        if (sizeof(TResult) == 8)
            max_value = negative ? (TUnsigned)_I64_MAX + 1 : (TUnsigned)_I64_MAX;
        else
            max_value = negative ? (TUnsigned)LONG_MAX + 1 : (TUnsigned)LONG_MAX;
    }
    else
    {
        if (sizeof(TResult) == 8)
            max_value = _UI64_MAX;
        else
            max_value = ULONG_MAX;
    }

    TUnsigned overflow_check = max_value / (TUnsigned)_Radix;

    /* Convert digits */
    for (;;)
    {
        int digit = _get_digit(*p, _Radix);
        if (digit < 0)
            break;

        if ((result > overflow_check) ||
            ((result == overflow_check) && ((TUnsigned)digit > max_value % (TUnsigned)_Radix)))
        {
            overflow = true;
        }

        result = result * (TUnsigned)_Radix + (TUnsigned)digit;
        p++;
    }

    /* If no digits were consumed, set endptr to original string */
    if (p == num_start)
    {
        if (_EndPtr != NULL)
        {
            *_EndPtr = (TChar*)_String;
        }
        return 0;
    }

    if (_EndPtr != NULL)
    {
        *_EndPtr = (TChar*)p;
    }

    if (overflow)
    {
        *_errno() = ERANGE;
        if (_IsSigned)
        {
            if (sizeof(TResult) == 8)
                return (TResult)(negative ? _I64_MIN : _I64_MAX);
            else
                return (TResult)(negative ? LONG_MIN : LONG_MAX);
        }
        else
        {
            if (negative)
                return (TResult)(0 - max_value);
            return (TResult)max_value;
        }
    }

    if (negative)
        return (TResult)(0 - result);

    return (TResult)result;
}

/* Simplified version for atoi/_wtoi (no endptr, no radix, no saturation) */
template<typename TResult, typename TChar>
static inline
TResult
_atox_nosat_impl(
    TChar const* const _String)
{
    typedef unsigned __int64 TUnsigned;

    TChar const* p = _String;
    bool negative = false;
    TUnsigned result = 0;

    /* Skip leading whitespace */
    while (_is_space(*p))
    {
        p++;
    }

    /* Handle sign */
    if (*p == '+')
    {
        p++;
    }
    else if (*p == '-')
    {
        negative = true;
        p++;
    }

    /* Convert digits */
    for (;;)
    {
        int digit = _get_digit(*p, 10);
        if (digit < 0)
            break;

        result = result * 10 + (TUnsigned)digit;
        p++;
    }

    if (negative)
        return (TResult)(0 - result);

    return (TResult)result;
}
