/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Template implementation of integer-to-string conversion
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

extern "C" void __cdecl _invalid_parameter_noinfo(void);

#define _UNBOUNDED_BUFFER_SIZE (static_cast<size_t>(-1))

template <typename T> struct __xtostr_type_traits;
template <> struct __xtostr_type_traits<int32_t > { enum { is_signed = true  }; using utype = uint32_t; };
template <> struct __xtostr_type_traits<uint32_t> { enum { is_signed = false }; using utype = uint32_t; };
template <> struct __xtostr_type_traits<int64_t > { enum { is_signed = true  }; using utype = uint64_t; };
template <> struct __xtostr_type_traits<uint64_t> { enum { is_signed = false }; using utype = uint64_t; };

/*
 * Core conversion: takes an unsigned value (which may represent a
 * negative signed value cast to unsigned), a sign flag, a buffer,
 * a buffer size, and a radix.
 *
 * When _Negative is true, the value is first negated in the unsigned
 * domain to recover the magnitude before generating digits.
 * When _Negative is false, the value is used as-is.
 */
template<typename TUINT, typename TCHAR>
static
errno_t
_utotcs_impl(
    TUINT const _Value,
    bool const _Negative,
    TCHAR* const _Buffer,
    size_t const _BufferCount,
    int const _Radix)
{
    TCHAR temp[65];
    size_t len = 0;
    size_t total_len;
    TCHAR* p;
    TUINT uval;

    /* Avoid template explosion by limiting the implementation to unsigned types */
    static_assert(__xtostr_type_traits<TUINT>::is_signed == false, "TUINT must be unsigned");

    /* The value that is passed in is a potentially negative value, cast to an unsigned type.
       If _negative is true, we need to invert the sign before generating the digits. */
    if (_Negative)
    {
        uval = static_cast<uint64_t>(static_cast<TUINT>(0) - _Value);
    }
    else
    {
        uval = _Value;
    }

    /* Generate digits in reverse order */
    if (uval == 0)
    {
        temp[len++] = (TCHAR)'0';
    }
    else
    {
        while (uval != 0)
        {
            uint32_t digit = (uint32_t)(uval % (TUINT)_Radix);
            temp[len++] = (TCHAR)((digit < 10) ? ('0' + digit) : ('a' + digit - 10));
            uval /= (TUINT)_Radix;
        }
    }

    /* Make sure the buffer is large enough */
    total_len = len + (_Negative ? 1 : 0);
    if (total_len >= _BufferCount)
    {
        _Buffer[0] = (TCHAR)'\0';
        _invalid_parameter_noinfo();
        //_invalid_parameter(L"", __FUNCTIONW__, __FILEW__, __LINE__, 0);
        //_invalid_parameter(NULL, _CRT_WIDE("x"), _CRT_WIDE(__FILE__), __LINE__, 0);
        return ERANGE;
    }

    /* Write sign */
    p = _Buffer;
    if (_Negative)
        *p++ = (TCHAR)'-';

    /* Write digits in correct order */
    while (len > 0)
        *p++ = temp[--len];

    *p = (TCHAR)'\0';
    return 0;
}

template<typename TINT, typename TCHAR>
static
TCHAR*
_xtotcs(
    TINT const _Value,
    TCHAR* const _Buffer,
    int const _Radix)
{
    using TUINT = typename __xtostr_type_traits<TINT>::utype;

    /* Prevent crash/hang. Windows doesn't do this, but better safe than sorry! */
    if (_Radix < 2)
    {
        _Buffer[0] = (TCHAR)'\0';
        return _Buffer;
    }

    bool negative = __xtostr_type_traits<TINT>::is_signed && (_Value < 0) && (_Radix == 10);
    _utotcs_impl<TUINT, TCHAR>(_Value, negative, _Buffer, _UNBOUNDED_BUFFER_SIZE, _Radix);
    return _Buffer;
}

template<typename TINT, typename TCHAR>
static
errno_t
_xtotcs_s(
    TINT const _Value,
    TCHAR* const _Buffer,
    size_t const _BufferCount,
    int const _Radix)
{
    if ((_Buffer == NULL) || (_BufferCount == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    bool negative = __xtostr_type_traits<TINT>::is_signed && (_Value < 0) && (_Radix == 10);
    if (_BufferCount <= (size_t)(negative ? 2 : 1))
    {
        _Buffer[0] = (TCHAR)'\0';
        _invalid_parameter_noinfo();
        return ERANGE;
    }

    if ((_Radix < 2) || (_Radix > 36))
    {
        _Buffer[0] = (TCHAR)'\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Generate digits */
    using TUINT = typename __xtostr_type_traits<TINT>::utype;
    return _utotcs_impl<TUINT, TCHAR>(_Value, negative, _Buffer, _BufferCount, _Radix);
}
