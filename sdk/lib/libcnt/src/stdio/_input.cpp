/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _tcsinput template, which is the core of all printf-like functions
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

enum API_FLAGS : uint32_t
{
    NORMAL = 0x00,
    SECURE_API = 0x01, // The API is a secure API (e.g., swscanf_s)
};

enum FMT_FLAGS : uint32_t
{
    NONE = 0x00,

    /* Size prefix flags */
    SIZE_h          = 0x000100, // 'h' - short: 16 bit integer / char: 8 bit character
    SIZE_l          = 0x000200, // 'l' - long: no effect on integers, for 'c'/'s' it means wide char
    SIZE_ll         = 0x000400, // 'll' - long long: 64 bit integer (behaves slightly different to I64!)
    SIZE_w          = 0x000800, // 'w' - Windows-specific wide char (wchar_t)
    SIZE_I32        = 0x001000, // 'I32' - Windows-specific 32-bit integer
    SIZE_I64        = 0x002000, // 'I64' - Windows-specific 64-bit integer
    SIZE_PTR        = 0x004000, // 'I' / 't' / 'z' - pointer-sized integer
    SIZE_hh         = 0x008000, // 'hh' - char integer modifier

    /* Other flags */
    SIZE_CHAR_OTHER = 0x010000, // Invert the character size (used for %C/%S)
    SIGNED          = 0x020000, // Signed format specifier (e.g., %d or %i instead of %u, %o or %x)
    UPPERCASE       = 0x040000, // Uppercase format specifier (e.g., %X instead of %x)
    NULL_TERMINATE  = 0x080000, // Null-terminate the output (used for %s)

    /* Combined masks */
    SIZE_64_MASK  = SIZE_ll | SIZE_I64,
};

/* Implement binary operators on FMT_FLAGS */
inline FMT_FLAGS operator|=(FMT_FLAGS& a, FMT_FLAGS b) { a = FMT_FLAGS(uint32_t(a) | uint32_t(b)); return a; }
inline FMT_FLAGS operator|(FMT_FLAGS a, FMT_FLAGS b) { return FMT_FLAGS(uint32_t(a) | uint32_t(b)); }
inline FMT_FLAGS operator&=(FMT_FLAGS& a, FMT_FLAGS b) { a = FMT_FLAGS(uint32_t(a) & uint32_t(b)); return a; }
inline FMT_FLAGS operator&(FMT_FLAGS a, FMT_FLAGS b) { return FMT_FLAGS(uint32_t(a) & uint32_t(b)); }
inline FMT_FLAGS operator~(FMT_FLAGS a) { return FMT_FLAGS(~uint32_t(a)); }

template<typename TCHAR> static inline bool _tisspace(TCHAR ch);

template<> inline bool _tisspace<char>(char ch)
{
    return isspace(static_cast<unsigned char>(ch)) != 0;
}

template<> inline bool _tisspace<wchar_t>(wchar_t ch)
{
    return iswspace(ch) != 0;
}

template<typename TCHAR> static inline bool _tisxdigit(TCHAR ch);

template<> inline bool _tisxdigit<char>(char ch)
{
    return isxdigit(static_cast<unsigned char>(ch)) != 0;
}

template<> inline bool _tisxdigit<wchar_t>(wchar_t ch)
{
    return iswxdigit(ch) != 0;
}

template<typename TCHAR>
static inline unsigned
hex_value(TCHAR ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return static_cast<unsigned>(ch - '0');
    if ((ch >= 'a') && (ch <= 'f'))
        return static_cast<unsigned>(ch - 'a' + 10);
    return static_cast<unsigned>(ch - 'A' + 10);
}

static inline long _tcstol(const char* str, char** endptr, int base)
{
    return strtol(str, endptr, base);
}

static inline long _tcstol(const wchar_t* str, wchar_t** endptr, int base)
{
    return wcstol(str, endptr, base);
}

static inline unsigned long _tcstoul(const char* str, char** endptr, int base)
{
    return strtoul(str, endptr, base);
}

static inline unsigned long _tcstoul(const wchar_t* str, wchar_t** endptr, int base)
{
    return wcstoul(str, endptr, base);
}

static inline __int64 _tcstoll(const char* str, char** endptr, int base)
{
    return _strtoi64(str, endptr, base);
}

static inline __int64 _tcstoll(const wchar_t* str, wchar_t** endptr, int base)
{
    return _wcstoi64(str, endptr, base);
}

static inline unsigned __int64 _tcstoull(const char* str, char** endptr, int base)
{
    return _strtoui64(str, endptr, base);
}

static inline unsigned __int64 _tcstoull(const wchar_t* str, wchar_t** endptr, int base)
{
    return _wcstoui64(str, endptr, base);
}

template<typename TCHAR>
static bool
parse_scanset(const TCHAR*& Format, TCHAR Terminator, bool& Negate, bool& AllowRanges)
{
    const TCHAR* spec = Format;

    Negate = false;
    AllowRanges = (Terminator == ']');

    if (Terminator == ']' && *spec == '^')
    {
        Negate = true;
        spec++;
    }

    if (*spec == 0)
    {
        return false;
    }

    while ((*spec != 0) && (*spec != Terminator))
    {
        spec++;
    }

    if (*spec != Terminator)
    {
        return false;
    }

    Format = spec;
    return true;
}

template<typename TCHAR>
static bool
is_in_scanset(TCHAR Char, const TCHAR* SetStart, const TCHAR* SetEnd, bool Negate, bool AllowRanges)
{
    bool matches = false;

    for (const TCHAR* it = SetStart; it < SetEnd; it++)
    {
        if (AllowRanges && (it + 2 < SetEnd) && (it[1] == '-') && (it[0] <= it[2]))
        {
            if ((Char >= it[0]) && (Char <= it[2]))
            {
                matches = true;
                break;
            }
            it += 2;
            continue;
        }

        if (Char == *it)
        {
            matches = true;
            break;
        }
    }

    return Negate ? !matches : matches;
}

template<typename TCHAR>
static inline
bool
is_wide_char_format(FMT_FLAGS Flags)
{
    /* In ansi APIs, the "h" prefix forces ansi format even when combined with "w"/"l". */
    if ((sizeof(TCHAR) == sizeof(char)) && (Flags & FMT_FLAGS::SIZE_h))
    {
        return false;
    }

    /* Otherwise if "w" or "l" were specified, this is wide char format */
    if (Flags & (FMT_FLAGS::SIZE_w | FMT_FLAGS::SIZE_l))
    {
        return true;
    }

    /* Check for wide-char specifier (wide char API with "%c"/"%s" or ansi API with "%C"/"%S") */
    if ((sizeof(TCHAR) == sizeof(wchar_t)) ^ ((Flags & FMT_FLAGS::SIZE_CHAR_OTHER) != 0))
    {
        /* This is wide char, unless "h" was specified. */
        return !(Flags & FMT_FLAGS::SIZE_h);
    }

    return false;
}

template<typename TCHAR>
static void
copy_tchars(TCHAR* Dest, const TCHAR* Src, size_t CountOfChars)
{
    for (size_t i = 0; i < CountOfChars; i++)
    {
        Dest[i] = Src[i];
        if (Src[i] == 0)
        {
            return;
        }
    }
}

static inline void
copy_achars(char* Dest, const char* Src, size_t CountOfChars)
{
    copy_tchars(Dest, Src, CountOfChars);
}

static inline void
copy_achars(char* Dest, const wchar_t* Src, size_t CountOfChars)
{
    const char* src = reinterpret_cast<const char*>(Src);
    for (size_t i = 0; i < CountOfChars; i++)
    {
        if (src[i] == 0)
            break;
        Dest[i] = src[i];
    }
}

static inline void
copy_wchars(wchar_t* Dest, const char* Src, size_t CountOfChars)
{
    mbstowcs(Dest, Src, CountOfChars);
}

static inline void
copy_wchars(wchar_t* Dest, const wchar_t* Src, size_t CountOfChars)
{
    copy_tchars(Dest, Src, CountOfChars);
}

static inline size_t
get_secure_buffer_count(uint32_t ApiFlags, va_list& Argptr)
{
    if ((ApiFlags & API_FLAGS::SECURE_API) == 0)
    {
        return static_cast<size_t>(-1);
    }

    return va_arg(Argptr, size_t);
}

template<typename TCHAR>
static
bool
capture_characters(const TCHAR*& Src,
                   size_t CountOfChars,
                   FMT_FLAGS fmt_flags,
                   va_list& Argptr,
                   uint32_t ApiFlags,
                   bool NullTerminate,
                   bool& InputDepleted)
{
    size_t copy_count = 0;
    while ((copy_count < CountOfChars) && (Src[copy_count] != 0))
    {
        copy_count++;
    }

    if (copy_count == 0)
    {
        InputDepleted = true;
        return false;
    }

    void* dst = va_arg(Argptr, void*);
    size_t capacity = get_secure_buffer_count(ApiFlags, Argptr);

    if (capacity != static_cast<size_t>(-1))
    {
        size_t required = copy_count + (NullTerminate ? 1 : 0);
        if (capacity < required)
        {
            return false;
        }
    }

    if (is_wide_char_format<TCHAR>(fmt_flags))
    {
        copy_wchars(static_cast<wchar_t*>(dst), Src, copy_count);
        if (NullTerminate)
        {
            static_cast<wchar_t*>(dst)[copy_count] = 0;
        }
    }
    else
    {
        copy_achars(static_cast<char*>(dst), Src, copy_count);
        if (NullTerminate)
        {
            static_cast<char*>(dst)[copy_count] = 0;
        }
    }

    Src += copy_count;
    return true;
}

template<typename TCHAR>
static inline bool
capture_string(const TCHAR*& Src,
               int field_width,
               FMT_FLAGS fmt_flags,
               va_list& Argptr,
               uint32_t ApiFlags,
               bool& InputDepleted)
{
    while (_tisspace(*Src))
    {
        Src++;
    }

    if (*Src == 0)
    {
        InputDepleted = true;
        return false;
    }

    size_t length = 0;

    if (field_width >= 0)
    {
        while ((length < static_cast<size_t>(field_width)) && !_tisspace(Src[length]) && (Src[length] != 0))
        {
            length++;
        }
    }
    else
    {
        while (!_tisspace(Src[length]) && (Src[length] != 0))
        {
            length++;
        }
    }

    if (length == 0)
    {
        return false;
    }

    return capture_characters(Src, length, fmt_flags, Argptr, ApiFlags, true, InputDepleted);
}

template<typename TCHAR>
static void
store_integer(unsigned __int64 Value, FMT_FLAGS fmt_flags, va_list& Argptr)
{
    if (fmt_flags & FMT_FLAGS::SIGNED)
    {
        __int64 signed_value = static_cast<__int64>(Value);
        if (fmt_flags & FMT_FLAGS::SIZE_64_MASK)
        {
            *va_arg(Argptr, __int64*) = signed_value;
        }
        else if (fmt_flags & FMT_FLAGS::SIZE_PTR)
        {
            *va_arg(Argptr, intptr_t*) = static_cast<intptr_t>(signed_value);
        }
        else if ((fmt_flags & FMT_FLAGS::SIZE_h) && !(fmt_flags & FMT_FLAGS::SIZE_hh))
        {
            *va_arg(Argptr, short*) = static_cast<short>(signed_value);
        }
        else
        {
            *va_arg(Argptr, int*) = static_cast<int>(signed_value);
        }
    }
    else
    {
        if (fmt_flags & FMT_FLAGS::SIZE_64_MASK)
        {
            *va_arg(Argptr, unsigned __int64*) = Value;
        }
        else if (fmt_flags & FMT_FLAGS::SIZE_PTR)
        {
            *va_arg(Argptr, uintptr_t*) = static_cast<uintptr_t>(Value);
        }
        else if ((fmt_flags & FMT_FLAGS::SIZE_h) && !(fmt_flags & FMT_FLAGS::SIZE_hh))
        {
            *va_arg(Argptr, unsigned short*) = static_cast<unsigned short>(Value);
        }
        else
        {
            *va_arg(Argptr, unsigned int*) = static_cast<unsigned int>(Value);
        }
    }
}

template<typename TCHAR>
static bool
capture_integer(const TCHAR*& buf,
                int field_width,
                int base,
                FMT_FLAGS fmt_flags,
                va_list& Argptr,
                bool& InputDepleted)
{
    while (_tisspace(*buf))
    {
        buf++;
    }

    if (*buf == 0)
    {
        InputDepleted = true;
        return false;
    }

    TCHAR* endptr = nullptr;
    unsigned __int64 value;

    if (field_width < 0)
    {
        if (fmt_flags & FMT_FLAGS::SIGNED)
        {
            value = static_cast<unsigned __int64>(_tcstoll(buf, &endptr, base));
        }
        else
        {
            value = _tcstoull(buf, &endptr, base);
        }

        if (endptr == buf)
        {
            return false;
        }

        buf = endptr;
        store_integer<TCHAR>(value, fmt_flags, Argptr);
        return true;
    }

    const TCHAR* token_end = buf;
    for (int i = 0; (i < field_width) && (*token_end != 0); i++)
    {
        token_end++;
    }

    size_t token_len = static_cast<size_t>(token_end - buf);
    TCHAR token[128];

    if (token_len >= (sizeof(token) / sizeof(token[0])))
    {
        token_len = (sizeof(token) / sizeof(token[0])) - 1;
    }

    copy_tchars(token, buf, token_len);
    token[token_len] = 0;

    if (fmt_flags & FMT_FLAGS::SIGNED)
    {
        value = static_cast<unsigned __int64>(_tcstoll(token, &endptr, base));
    }
    else
    {
        value = _tcstoull(token, &endptr, base);
    }

    if (endptr == token)
    {
        return false;
    }

    buf += (endptr - token);
    store_integer<TCHAR>(value, fmt_flags, Argptr);
    return true;
}

template<typename TCHAR>
static bool
capture_pointer(const TCHAR*& buf, int field_width, va_list& Argptr, bool& InputDepleted)
{
    while (_tisspace(*buf))
    {
        buf++;
    }

    if (*buf == 0)
    {
        InputDepleted = true;
        return false;
    }

    uint64_t value = 0;
    size_t consumed = 0;
    const TCHAR* it = buf;

    while (*it != 0)
    {
        if ((field_width >= 0) && (consumed >= static_cast<size_t>(field_width)))
        {
            break;
        }

        if (!_tisxdigit(*it))
        {
            break;
        }

        value = (value << 4) | hex_value(*it);
        it++;
        consumed++;
    }

    if (consumed == 0)
    {
        return false;
    }

    *va_arg(Argptr, void**) = reinterpret_cast<void*>(static_cast<uintptr_t>(value));
    buf = it;
    return true;
}

template<typename TCHAR>
static void
capture_count(const TCHAR* BufferStart, const TCHAR* Current, FMT_FLAGS fmt_flags, va_list& Argptr)
{
    size_t count = static_cast<size_t>(Current - BufferStart);

    if (fmt_flags & FMT_FLAGS::SIZE_64_MASK)
    {
        *va_arg(Argptr, __int64*) = static_cast<__int64>(count);
    }
    else if (fmt_flags & FMT_FLAGS::SIZE_PTR)
    {
        *va_arg(Argptr, intptr_t*) = static_cast<intptr_t>(count);
    }
    else if ((fmt_flags & FMT_FLAGS::SIZE_h) && !(fmt_flags & FMT_FLAGS::SIZE_hh))
    {
        *va_arg(Argptr, short*) = static_cast<short>(count);
    }
    else
    {
        *va_arg(Argptr, int*) = static_cast<int>(count);
    }
}

template<typename CHAR_TYPE, typename TCHAR>
static inline void
store_output_char(CHAR_TYPE* Dest, size_t Index, TCHAR Value)
{
    Dest[Index] = static_cast<CHAR_TYPE>(Value);
}

template<typename TCHAR>
static bool
capture_scanset(const TCHAR*& Buf,
               const TCHAR*& Format,
               int FieldWidth,
               TCHAR Terminator,
               FMT_FLAGS fmt_flags,
               va_list& Argptr,
               uint32_t ApiFlags,
               bool& InputDepleted)
{
    bool negate = false;
    bool allow_ranges = false;
    const TCHAR* set_start = Format + 1;
    const TCHAR* set_end = set_start;

    if (!parse_scanset(set_end, Terminator, negate, allow_ranges))
    {
        return false;
    }

    if ((Terminator == '}') && (*set_start == '^'))
    {
        return false;
    }

    void* dst = va_arg(Argptr, void*);
    size_t capacity = get_secure_buffer_count(ApiFlags, Argptr);
    size_t length = 0;
    size_t max_capture = (FieldWidth >= 0) ? static_cast<size_t>(FieldWidth) : static_cast<size_t>(-1);

    if (Terminator == '}')
    {
        max_capture = 1;
    }

    if (is_wide_char_format<TCHAR>(fmt_flags))
    {
        wchar_t* out = static_cast<wchar_t*>(dst);
        size_t max_out = (capacity == static_cast<size_t>(-1)) ? max_capture : ((capacity > 0) ? capacity - 1 : 0);

        while ((Buf[length] != 0) && (length < max_capture) && (length < max_out) &&
               is_in_scanset(Buf[length], set_start, set_end, negate, allow_ranges))
        {
            store_output_char(out, length, Buf[length]);
            length++;
        }

        if (length == 0)
        {
            InputDepleted = (*Buf == 0);
            return false;
        }

        out[length] = 0;
    }
    else
    {
        char* out = static_cast<char*>(dst);
        size_t max_out = (capacity == static_cast<size_t>(-1)) ? max_capture : ((capacity > 0) ? capacity - 1 : 0);

        while ((Buf[length] != 0) && (length < max_capture) && (length < max_out) &&
               is_in_scanset(Buf[length], set_start, set_end, negate, allow_ranges))
        {
            store_output_char(out, length, Buf[length]);
            length++;
        }

        if (length == 0)
        {
            InputDepleted = (*Buf == 0);
            return false;
        }

        out[length] = 0;
    }

    Buf += length;
    Format = set_end;
    return true;
}


template<typename TCHAR>
int
_tcsinput_internal(
    _In_reads_(_BufferCount) _Pre_z_ const TCHAR* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Scanf_s_format_string_ const TCHAR* const Format,
    va_list const ArgList,
    uint32_t Flags)
{
    const TCHAR* buf = Buffer;
    const TCHAR* format = Format;
    va_list args = ArgList;
    int field_width;
    int count = 0;

    if ((Buffer == nullptr) || (Format == nullptr))
    {
        //_invalid_parameter(0,0,0,0);
        return -1;
    }

    while (true)
    {
        /* Handle chars other than '%' first */
        if (*format != '%')
        {
            /* Check for whitespace skipping */
            if (_tisspace(*format))
            {
                /* Skip white space in the format string */
                while (_tisspace(*format))
                {
                    format++;
                }

                /* Skip white space in the buffer */
                while (_tisspace(*buf))
                {
                    buf++;
                }

                continue;
            }

            /* If the format string ends here, we are done */
            if (*format == '\0')
            {
                return count;
            }

            /* Bail out on non-matching chars */
            if (*format != *buf)
            {
                goto fail_path;
            }

            format++;
            buf++;
            continue;
        }

        /* Check for "%%" */
        if (format[1] == '%')
        {
            /* This should match a single '%' character in the buffer */
            if (*buf != '%')
            {
                goto fail_path;
            }

            /* Skip the next character and continue */
            format += 2;
            buf++;
            continue;
        }

        format++;
        field_width = -1;
        FMT_FLAGS fmt_flags = FMT_FLAGS::NONE;

        if ((*format == '-') || (*format == '+') || (*format == '#') || (*format == '.') || (*format == '*'))
        {
            goto fail_path;
        }

        /* Handle field width modifier */
        if ((*format >= '0') && (*format <= '9'))
        {
            field_width = 0;
            while ((*format >= '0') && (*format <= '9'))
            {
                field_width = field_width * 10 + (*format - '0');
                format++;
            }

            if (field_width == 0)
            {
                goto fail_path;
            }
        }

        bool invalid_modifier = false;
        bool had_tz_modifier = false;
        bool tz_after_I = false;

        /* Handle size prefix */
        for (; true; format++)
        {
            switch (*format)
            {
                case 'h':
                    if (fmt_flags & FMT_FLAGS::SIZE_h)
                        fmt_flags |= FMT_FLAGS::SIZE_hh;
                    fmt_flags |= FMT_FLAGS::SIZE_h;
                    continue;

                case 'l':
                    if (format[1] == 'l')
                    {
                        format++;
                        fmt_flags |= FMT_FLAGS::SIZE_ll;
                    }
                    else
                    {
                        fmt_flags |= FMT_FLAGS::SIZE_l;
                    }
                    continue;

                case 'w':
                    fmt_flags |= FMT_FLAGS::SIZE_w;
                    continue;

                case 'z':
                case 't':
                    had_tz_modifier = true;
                    if ((fmt_flags & FMT_FLAGS::SIZE_PTR))
                    {
                        tz_after_I = true;
                        continue;
                    }
                    if (fmt_flags != FMT_FLAGS::NONE)
                    {
                        continue;
                    }

                    /* Validate the following format character */
                    if ((format[1] != 'd') && (format[1] != 'i') && (format[1] != 'o') &&
                        (format[1] != 'u') && (format[1] != 'x') && (format[1] != 'X'))
                    {
                        invalid_modifier = true;
                        break;
                    }

                    fmt_flags |= FMT_FLAGS::SIZE_PTR;
                    continue;

                case 'I':
                    if ((format[1] == '3') && (format[2] == '2'))
                    {
                        format += 2;
                        fmt_flags |= FMT_FLAGS::SIZE_I32;
                        continue;
                    }
                    if ((format[1] == '6') && (format[2] == '4'))
                    {
                        format += 2;
                        fmt_flags |= FMT_FLAGS::SIZE_I64;
                        continue;
                    }

                    /* Validate the following format character */
                    if ((format[1] != 'd') && (format[1] != 'i') && (format[1] != 'o') &&
                        (format[1] != 'u') && (format[1] != 'x') && (format[1] != 'X') &&
                        (format[1] != 't') && (format[1] != 'z'))
                    {
                        break;
                    }

                    fmt_flags |= FMT_FLAGS::SIZE_PTR;
                    continue;

                case 'F': // far pointer (legacy, ignored)
                case 'N': // near pointer (legacy, ignored)
                case 'L': // long double (standard, but we don't support it, so ignore)
                case 'j': // intmax_t (standard, but we don't support it, so ignore)
                    continue;

                default:
                    break;
            }

            break;
        }

        if (invalid_modifier)
        {
            goto fail_path;
        }

        if (*format == '\0')
        {
#ifdef _WIN64
            if ((Flags & API_FLAGS::SECURE_API) && (sizeof(TCHAR) == sizeof(wchar_t)))
            {
                return 0;
            }
#endif
            goto fail_path;
        }

        if (had_tz_modifier && !tz_after_I && (*format != 'd') && (*format != 'i') &&
            (*format != 'o') && (*format != 'u') && (*format != 'x') && (*format != 'X'))
        {
            goto fail_path;
        }

        if (tz_after_I)
        {
            fmt_flags &= ~FMT_FLAGS::SIZE_PTR;
        }

        bool input_depleted = false;
        bool assigned = false;

        switch (*format)
        {
            case 'C':
                fmt_flags |= FMT_FLAGS::SIZE_CHAR_OTHER;
                __fallthrough;
            case 'c':
                if (fmt_flags & FMT_FLAGS::SIZE_PTR)
                {
                    goto fail_path;
                }
                assigned = capture_characters(buf,
                                              field_width >= 0 ? static_cast<size_t>(field_width) : 1,
                                              fmt_flags,
                                              args,
                                              Flags,
                                              false,
                                              input_depleted);
                break;

            case 'S':
                fmt_flags |= FMT_FLAGS::SIZE_CHAR_OTHER;
                __fallthrough;
            case 's':
                if (fmt_flags & FMT_FLAGS::SIZE_PTR)
                {
                    goto fail_path;
                }
                assigned = capture_string(buf, field_width, fmt_flags | FMT_FLAGS::NULL_TERMINATE, args, Flags, input_depleted);
                break;

            /* "%d" / "%D" are integers with base 10 */
            case 'D': // synonym for "%ld" (ignored)
                __fallthrough;
            case 'd':
                assigned = capture_integer(buf, field_width, 10, fmt_flags | FMT_FLAGS::SIGNED, args, input_depleted);
                break;

            /* "%i" / "%I" are integers with autodetected base (0 prefix for octal, 0x for hex) */
            case 'I':
                assigned = capture_integer(buf, field_width, 0, fmt_flags | FMT_FLAGS::SIGNED | FMT_FLAGS::SIZE_PTR, args, input_depleted);
                break;

            case 'i':
                assigned = capture_integer(buf, field_width, 0, fmt_flags | FMT_FLAGS::SIGNED, args, input_depleted);
                break;

            case 'U':
                __fallthrough;
            case 'u':
                assigned = capture_integer(buf, field_width, 10, fmt_flags, args, input_depleted);
                break;

            case 'O':
                __fallthrough;
            case 'o':
                assigned = capture_integer(buf, field_width, 8, fmt_flags, args, input_depleted);
                break;

            case 'X':
                __fallthrough;
            case 'x':
                assigned = capture_integer(buf, field_width, 16, fmt_flags, args, input_depleted);
                break;

            case 'n':
                capture_count(Buffer, buf, fmt_flags, args);
                format++;
                continue;

            case 'p':
            case 'P':
                assigned = capture_pointer(buf, field_width, args, input_depleted);
                break;

            case '[':
                if (fmt_flags & FMT_FLAGS::SIZE_PTR)
                {
                    goto fail_path;
                }
                assigned = capture_scanset(buf, format, field_width, static_cast<TCHAR>(']'), fmt_flags, args, Flags, input_depleted);
                break;

            case '{':
                if (fmt_flags & FMT_FLAGS::SIZE_PTR)
                {
                    goto fail_path;
                }
                assigned = capture_scanset(buf, format, field_width, static_cast<TCHAR>('}'), fmt_flags, args, Flags, input_depleted);
                break;

            default:
                /* Invalid format specifier, bail out */
                goto fail_path;
        }

        if (!assigned)
        {
            if ((count == 0) && input_depleted)
            {
                if ((Flags & API_FLAGS::SECURE_API) && (sizeof(TCHAR) == sizeof(wchar_t)))
                {
                    return 0;
                }
                return -1;
            }
            return count;
        }

        /* Successfully parsed a field */
        format++;
        count++;
    }


fail_path:

    if ((count == 0) && (*buf == 0) && (sizeof(TCHAR) == sizeof(char)))
    {
        return -1;
    }
    return count;
}

template<typename TCHAR>
int
_tcsinput(
    _In_reads_(_BufferCount) _Pre_z_ const TCHAR* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Scanf_s_format_string_ const TCHAR* const Format,
    va_list const ArgList)
{
    int result;

    if ((Buffer == NULL) || (Format == NULL))
    {
        return -1;
    }

    if (Format[0] == 0)
    {
        return 0;
    }

    result = _tcsinput_internal<TCHAR>(Buffer, BufferCount, Format, ArgList, API_FLAGS::NORMAL);

    return result;
}

template<typename TCHAR>
int
_tcsinput_s(
    _In_reads_(_BufferCount) _Pre_z_ const TCHAR* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Scanf_s_format_string_ const TCHAR* const Format,
    va_list const ArgList)
{
    int result;

    if ((Buffer == NULL) || (Format == NULL))
    {
        return -1;
    }

    result = _tcsinput_internal<TCHAR>(Buffer, BufferCount, Format, ArgList, API_FLAGS::SECURE_API);

    return result;
}

// Explicitly instantiate the templates for char and wchar_t
template int _tcsinput<char>(const char* const Buffer, size_t const BufferCount, const char* const Format, va_list const ArgList);
template int _tcsinput<wchar_t>(const wchar_t* const Buffer, size_t const BufferCount, const wchar_t* const Format, va_list const ArgList);
template int _tcsinput_s<char>(const char* const Buffer, size_t const BufferCount, const char* const Format, va_list const ArgList);
template int _tcsinput_s<wchar_t>(const wchar_t* const Buffer, size_t const BufferCount, const wchar_t* const Format, va_list const ArgList);

