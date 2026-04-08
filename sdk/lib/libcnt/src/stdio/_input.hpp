/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header for _tcsinput template, implemented in _input.cpp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>

template<typename TCHAR>
int
_tcsinput(
    _In_reads_(_BufferCount) _Pre_z_ const TCHAR* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Scanf_s_format_string_ const TCHAR* const Format,
    va_list const ArgList);

template<typename TCHAR>
int
_tcsinput_s(
    _In_reads_(_BufferCount) _Pre_z_ const TCHAR* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Scanf_s_format_string_ const TCHAR* const Format,
    va_list const ArgList);
