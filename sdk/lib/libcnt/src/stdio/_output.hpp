/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header for _tcsoutput template, implemented in _output.cpp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>

template<typename TCHAR>
int
_tcsoutput(
    _Out_writes_opt_(_BufferCount) _Post_maybez_ TCHAR* Buffer,
    _In_ size_t BufferCount,
    _In_z_ _Printf_format_string_ const TCHAR* Format,
    va_list ArgList);

template<typename TCHAR>
int
_tcsoutput_s(
    _Out_writes_opt_(_BufferCount) _Post_maybez_ TCHAR* Buffer,
    _In_ size_t BufferCount,
    _In_ size_t MaxCount,
    _In_z_ _Printf_format_string_ const TCHAR* Format,
    va_list ArgList);
