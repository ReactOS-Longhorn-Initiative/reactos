/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _tcsoutput template, which is the core of all printf-like functions
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_output.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <intrin.h>
#include <string.h>
#include <minmax.h>
#include "../ntrtl.h"

#define UINT64_MAX_DIGITS 20 /* 18446744073709551615 */

enum FMT_FLAGS : uint32_t
{
    NONE = 0x00,

    /* Formatting flags */
    ALIGN_LEFT      = 0x000001, // '-' - Left-align the output within the field width
    FORCE_SIGN      = 0x000002, // '+' - Always prefix numeric output with a sign ('+' or '-')
    FORCE_SIGNSPC   = 0x000004, // ' ' - Prefix numeric output with a space if no sign is going to be written
    PAD_ZERO        = 0x000008, // '0' - Pad the field with leading zeros instead of spaces
    SPECIAL         = 0x000010, // '#' - Alternate form: for 'o', prefix with '0'; for 'x'/'X', prefix with '0x'/'0X'; etc.

    /* Size prefix flags */
    SIZE_h          = 0x000100, // 'h' - short: 16 bit integer / char: 8 bit character
    SIZE_l          = 0x000200, // 'l' - long: no effect on integers, for 'c'/'s' it means wide char
    SIZE_ll         = 0x000400, // 'll' - long long: 64 bit integer (behaves slightly different to I64!)
    SIZE_w          = 0x000800, // 'w' - Windows-specific wide char (wchar_t)
    SIZE_I32        = 0x001000, // 'I32' - Windows-specific 32-bit integer
    SIZE_I64        = 0x002000, // 'I64' - Windows-specific 64-bit integer
#ifdef _WIN64
    SIZE_PTR        = SIZE_I64, // 't' (ptrdiff_t) / 'z' (size_t) / 'I': 64 bit on 64 bit systems
#else
    SIZE_PTR        = 0x000000, // 't' (ptrdiff_t) / 'z' (size_t) / 'I': 32 bit (ignored) on 32 bit systems
#endif

    /* Other flags */
    SIZE_CHAR_OTHER = 0x010000, // Invert the character size (used for %C/%S)
    SIGNED          = 0x020000, // Signed format specifier (e.g., %d or %i instead of %u, %o or %x)
    UPPERCASE       = 0x040000, // Uppercase format specifier (e.g., %X instead of %x)

    /* Combined masks */
    SIZE_64_MASK  = SIZE_ll | SIZE_I64, // 
};

/* Implement binary operators on FMT_FLAGS */
inline FMT_FLAGS operator|=(FMT_FLAGS& a, FMT_FLAGS b) { a = FMT_FLAGS(uint32_t(a) | uint32_t(b)); return a; }
inline FMT_FLAGS operator|(FMT_FLAGS a, FMT_FLAGS b) { return FMT_FLAGS(uint32_t(a) | uint32_t(b)); }
inline FMT_FLAGS operator&=(FMT_FLAGS& a, FMT_FLAGS b) { a = FMT_FLAGS(uint32_t(a) & uint32_t(b)); return a; }
inline FMT_FLAGS operator&(FMT_FLAGS a, FMT_FLAGS b) { return FMT_FLAGS(uint32_t(a) & uint32_t(b)); }
inline FMT_FLAGS operator~(FMT_FLAGS a) { return FMT_FLAGS(~uint32_t(a)); }

struct FMT_OPTIONS
{
    FMT_FLAGS flags = FMT_FLAGS::NONE;
    int width = 0;
    int precision = -1;
};

template<typename TCHAR>
class OUTBUF
{
public: // FIXME
    TCHAR* const Buffer;
    size_t BufferSizeInElems;
    size_t CurrentPosition;

public:

    OUTBUF(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ TCHAR* const _Buffer,
        _In_ size_t const _BufferCount)
        : Buffer(_Buffer), BufferSizeInElems(_BufferCount), CurrentPosition(0)
    {
    }

    inline TCHAR& operator[](size_t index)
    {
        if (index >= BufferSizeInElems)
        {
            __int2c();
        }

        return Buffer[index];
    }

    inline void add_count(size_t count)
    {
        /* We check for overflow and cap the size at UINT_MAX to prevent
           the return value to become positive after an INT_MAX overflow */
        size_t new_pos = CurrentPosition + count;
        if ((new_pos < CurrentPosition) || (new_pos > UINT_MAX))
        {
            CurrentPosition = UINT_MAX; // Cap at max to prevent overflow
        }
        else
        {
            CurrentPosition += count;
        }
    }
};

/* For parsing 'Z' and 'wZ' */
struct NT_STRING
{
    uint16_t Length;
    uint16_t MaximumLength;
    const void* Buffer;
};

template<typename TCHAR>
static inline
bool
is_wide_char_format(FMT_FLAGS Flags)
{
    /* For wide char API, the "h" prefix always specifies ansi format */
    if ((sizeof(TCHAR) == sizeof(wchar_t)) && (Flags & FMT_FLAGS::SIZE_h))
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

static inline
bool
is_64bit_integer_format(FMT_FLAGS Flags)
{
    return (Flags & FMT_FLAGS::SIZE_64_MASK);
}

template<typename TCHAR>
static
void
_tcsoutput_tstring(
    _Inout_ OUTBUF<TCHAR>& OutputBuffer,
    _In_ const TCHAR* String,
    _In_ size_t const CharsToWrite)
{
    if (OutputBuffer.CurrentPosition < OutputBuffer.BufferSizeInElems)
    {
        size_t maxcount = min(CharsToWrite, OutputBuffer.BufferSizeInElems - OutputBuffer.CurrentPosition);
        for (size_t i = 0; i < maxcount; i++)
        {
            OutputBuffer[OutputBuffer.CurrentPosition + i] = String[i];
        }
    }

    OutputBuffer.add_count(CharsToWrite);
}

static
void
_tcsoutput_astring(
    _Inout_ OUTBUF<char>& OutputBuffer,
    _In_ const char* String,
    _In_ size_t const CharsToWrite)
{
    _tcsoutput_tstring(OutputBuffer, String, CharsToWrite);
}

static
void
_tcsoutput_astring(
    _Inout_ OUTBUF<wchar_t>& OutputBuffer,
    _In_ const char* String,
    _In_ size_t const CharsToWrite)
{
    if (OutputBuffer.CurrentPosition < OutputBuffer.BufferSizeInElems)
    {
        wchar_t* dest = OutputBuffer.Buffer + OutputBuffer.CurrentPosition;
        size_t maxcount = min(CharsToWrite, OutputBuffer.BufferSizeInElems - OutputBuffer.CurrentPosition);
        //RtlMultiByteToUnicodeN(dest, (ULONG)maxcount, NULL, String, (ULONG)CharsToWrite);
        mbstowcs(dest, String, maxcount);
    }

    OutputBuffer.add_count(CharsToWrite);
}

static
void
_tcsoutput_wstring(
    _Inout_ OUTBUF<char>& OutputBuffer,
    _In_ const wchar_t* String,
    _In_ size_t const CharsToWrite)
{
    if (OutputBuffer.CurrentPosition < OutputBuffer.BufferSizeInElems)
    {
        char* dest = OutputBuffer.Buffer + OutputBuffer.CurrentPosition;
        size_t maxcount = min(CharsToWrite, OutputBuffer.BufferSizeInElems - OutputBuffer.CurrentPosition);
        //RtlUnicodeToMultiByteN(dest, (ULONG)maxcount, NULL, String, (ULONG)CharsToWrite);
        wcstombs(dest, String, maxcount);
    }

    OutputBuffer.add_count(CharsToWrite);
}

static
void
_tcsoutput_wstring(
    _Inout_ OUTBUF<wchar_t>& OutputBuffer,
    _In_ const wchar_t* String,
    _In_ size_t const CharsToWrite)
{
    _tcsoutput_tstring(OutputBuffer, String, CharsToWrite);
}

template<typename TCHAR>
static
void
_tcsoutput_ascii_chars(
    _Inout_ OUTBUF<TCHAR>& OutputBuffer,
    _In_ char const Char,
    _In_ size_t const CharsToWrite)
{
    if (OutputBuffer.CurrentPosition < OutputBuffer.BufferSizeInElems)
    {
        size_t maxcount = min(CharsToWrite, OutputBuffer.BufferSizeInElems - OutputBuffer.CurrentPosition);
        for (size_t i = 0; i < maxcount; i++)
        {
            OutputBuffer[OutputBuffer.CurrentPosition + i] = Char;
        }
    }

    OutputBuffer.add_count(CharsToWrite);
}

static
void
format_number_with_options(
    _Out_z_cap_(UINT64_MAX_DIGITS + 1) char OutputString[UINT64_MAX_DIGITS + 1],
    _In_ FMT_OPTIONS& fmt_opts,
    _In_ uint32_t Radix,
    _Inout_ char const*& prefix,
    _Inout_ va_list& argptr)
{
    if (is_64bit_integer_format(fmt_opts.flags))
    {
        int64_t value = va_arg(argptr, int64_t);
        if ((fmt_opts.flags & FMT_FLAGS::SIGNED))
        {
            if (value < 0)
            {
                prefix = "-";
                value = -value;
            }
            else if ((fmt_opts.flags & FMT_FLAGS::FORCE_SIGN))
            {
                prefix = "+";
            }
            else if ((fmt_opts.flags & FMT_FLAGS::FORCE_SIGNSPC))
            {
                prefix = " ";
            }
        }
        _ui64toa(value, OutputString, Radix);
    }
    else
    {
        int32_t value = va_arg(argptr, int32_t);

        if ((fmt_opts.flags & FMT_FLAGS::SIGNED))
        {
            /* Both h and hh prefixes result in signed 16 bit integers */
            if (fmt_opts.flags & FMT_FLAGS::SIZE_h)
            {
                value = (int16_t)value;
            }

            if (value < 0)
            {
                prefix = "-";
                value = -value;
            }
            else if ((fmt_opts.flags & FMT_FLAGS::FORCE_SIGN))
            {
                prefix = "+";
            }
            else if ((fmt_opts.flags & FMT_FLAGS::FORCE_SIGNSPC))
            {
                prefix = " ";
            }
        }
        else
        {
            /* Both h and hh prefixes result in unsigned 16 bit integers */
            if (fmt_opts.flags & FMT_FLAGS::SIZE_h)
            {
                value = (uint16_t)value;
            }
        }
        _ultoa(value, OutputString, Radix);
    }

    /* For numbers pecision is capped at 512 */
    fmt_opts.precision = min(fmt_opts.precision, 512);

    /* If the precision is explicitly zero and the value is zero, the output should be empty */
    if ((fmt_opts.precision == 0) && (OutputString[0] == '0') && (OutputString[1] == '\0'))
    {
        OutputString[0] = '\0';
    }

    if (fmt_opts.flags & FMT_FLAGS::UPPERCASE)
    {
        _strupr(OutputString);
    }
}

template<typename TCHAR>
static inline
FMT_OPTIONS
parse_format_options(
    _Inout_ const TCHAR*& Format,
    _Inout_ va_list& ArgList)
{
    FMT_OPTIONS fmt_opts;
    TCHAR chr = *Format;

    /* Handle format flags */
    for (; true; chr = *(++Format))
    {
        switch (chr)
        {
            case '-': fmt_opts.flags |= FMT_FLAGS::ALIGN_LEFT; continue;
            case '+': fmt_opts.flags |= FMT_FLAGS::FORCE_SIGN; continue;
            case ' ': fmt_opts.flags |= FMT_FLAGS::FORCE_SIGNSPC; continue;
            case '0': fmt_opts.flags |= FMT_FLAGS::PAD_ZERO; continue;
            case '#': fmt_opts.flags |= FMT_FLAGS::SPECIAL; continue;
            default: break;
        }
        break;
    }

    /* Handle indirect field width modifier */
    if (chr == '*')
    {
        fmt_opts.width = va_arg(ArgList, int);
        if (fmt_opts.width < 0)
        {
            fmt_opts.flags |= FMT_FLAGS::ALIGN_LEFT;
            fmt_opts.width = -fmt_opts.width;
        }

        chr = *(++Format);
    }

    /* Handle direct field width modifier (it 'stacks' on top of the indirect one) */
    while ((chr >= '0') && (chr <= '9'))
    {
        fmt_opts.width = fmt_opts.width * 10 + (chr - '0');
        chr = *(++Format);
    }

    /* Handle precision modifier */
    if (chr == '.')
    {
        fmt_opts.precision = 0; // Precision specified, but no value yet

        chr = *(++Format);

        /* Handle indirect precision modifier  */
        if (chr == '*')
        {
            fmt_opts.precision = va_arg(ArgList, int);
            chr = *(++Format);
        }

        /* Handle direct precision modifier (it 'stacks' on top of the indirect one) */
        while (chr >= '0' && chr <= '9')
        {
            fmt_opts.precision = fmt_opts.precision * 10 + (chr - '0');
            chr = *(++Format);
        }
    }

    /* Handle argument size prefix: 'h', 'l', 'll', 'j', 'w', 'I', 'I32', 'I64' */
    for (; true; chr = *(++Format))
    {
        switch (chr)
        {
            case 'h':
                fmt_opts.flags |= FMT_FLAGS::SIZE_h;
                continue;

            case 'l':
                /* Check if this is 'll' */
                if (Format[1] == 'l')
                {
                    Format++;
                    fmt_opts.flags |= FMT_FLAGS::SIZE_ll;
                }
                else
                {
                    fmt_opts.flags |= FMT_FLAGS::SIZE_l;
                }
                continue;

            case 'j':
                fmt_opts.flags |= FMT_FLAGS::SIZE_I64;
                continue;

            case 'w':
                fmt_opts.flags |= FMT_FLAGS::SIZE_w;
                continue;

            case 'I':
                if ((Format[1] == '3') && (Format[2] == '2'))
                {
                    Format += 2;
                    fmt_opts.flags |= FMT_FLAGS::SIZE_I32;
                    fmt_opts.flags &= ~FMT_FLAGS::SIZE_I64; // Clear I32 if it was set before
                }
                else if ((Format[1] == '6') && (Format[2] == '4'))
                {
                    Format += 2;
                    fmt_opts.flags |= FMT_FLAGS::SIZE_I64;
                    fmt_opts.flags &= ~FMT_FLAGS::SIZE_I32; // Clear I32 if it was set before
                }
                else
                {
                    /* Parse the 'I' below */
                    break;
                }
                continue;

            case 'F': // far pointer (legacy, ignored)
            case 'N': // near pointer (legacy, ignored)
            case 'L': // long double (standard, but we don't support it, so ignore)
                continue;

            default:
                break;
        }
        break;
    }

    /* Handle pointer size prefixes (only valid with certain formats and must come last) */
    if ((chr == 'I') || (chr == 'z') || (chr == 't'))
    {
        if ((Format[1] == 'd') ||
            (Format[1] == 'i') ||
            (Format[1] == 'o') ||
            (Format[1] == 'u') ||
            (Format[1] == 'x') ||
            (Format[1] == 'X'))
        {
            fmt_opts.flags |= FMT_FLAGS::SIZE_PTR;
            chr = *(++Format);
        }
    }

    return fmt_opts;
}

template<typename TCHAR>
int
_tcsoutput(
    _Out_writes_opt_(BufferCount) _Post_maybez_ TCHAR* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Printf_format_string_ const TCHAR* const Format,
    va_list const ArgList)
{
    OUTBUF<TCHAR> output(Buffer, BufferCount);
    const TCHAR* format = Format;
    const TCHAR* start = Format;
    va_list argptr = ArgList;

    if (Format == nullptr)
    {
        return -1;
    }

    if ((Buffer == nullptr) && (BufferCount != 0))
    {
        return -1;
    }

    while (true)
    {
        /* Scan format characters until we find the end or a '%' */
        while ((*format != '\0') && (*format != '%'))
        {
            format++;
        }

        /* Check if we have a string to output */
        if (format > start)
        {
            /* Output the format characters up to this point */
            _tcsoutput_tstring(output, start, (format - start));
        }

        /* Make sure the buffer isn't too large to add another field
           (which is limited to INT_MAX characters) */
        if (output.CurrentPosition > INT_MAX)
        {
            return INT_MIN;
        }

        /* Check for end of format string */
        if (*format == 0)
        {
            break;
        }

        /* Skip the '%' */
        format++;

        /* Parse format options (passes format and argptr by reference!) */
        FMT_OPTIONS fmt_opts = parse_format_options(format, argptr);

        /* String buffers for formatting of numbers and 'c'/'C' */
        char char_buf[UINT64_MAX_DIGITS + 1];
        wchar_t wchar_buf[2];
        const void* string_ptr;
        size_t string_length;
        bool string_is_wide = false;
        const char* prefix = nullptr;

        /* Parse format specifier */
        TCHAR format_char = *format;
        switch (format_char)
        {
            case '%':
                char_buf[0] = '%';
                char_buf[1] = '\0';
                string_ptr = char_buf;
                string_length = 1;
                break;

            case 'C':
                fmt_opts.flags |= FMT_FLAGS::SIZE_CHAR_OTHER;
                __fallthrough;
            case 'c':
                /* Check what kind of character we should expect */
                string_is_wide = is_wide_char_format<TCHAR>(fmt_opts.flags);
                if (string_is_wide)
                {
                    wchar_buf[0] = (wchar_t)va_arg(argptr, int); // char is promoted to int in varargs
                    wchar_buf[1] = 0;
                    string_ptr = wchar_buf;
                }
                else
                {
                    char_buf[0] = (char)va_arg(argptr, int); // char is promoted to int in varargs
                    char_buf[1] = 0;
                    string_ptr = char_buf;
                }
                string_length = 1;

                /* Disable precision */
                fmt_opts.precision = -1;
                break;

            case 'd':
            case 'i':
                fmt_opts.flags |= FMT_FLAGS::SIGNED;
                __fallthrough;
            case 'u':
                format_number_with_options(char_buf, fmt_opts, 10, prefix, argptr);
                string_ptr = char_buf;
                string_length = strlen(char_buf);
                break;

            case 'o':
                format_number_with_options(char_buf, fmt_opts, 8, prefix, argptr);
                string_ptr = char_buf;
                string_length = strlen(char_buf);
                if ((fmt_opts.flags & FMT_FLAGS::SPECIAL) && (char_buf[0] != '0'))
                {
                    prefix = "0";
                    /* For octal the leading 0 counts into the 'precision' length, so if
                       we add it with a prefix, counter it by decreasing precision by 1. */
                    if (fmt_opts.precision > 1)
                    {
                        fmt_opts.precision--;
                    }
                }
                break;

            case 'p':
                /* The '%p' specifier prints a pointer-sized, uppercase hexadecimal value. */
                fmt_opts.flags |= FMT_FLAGS::SIZE_PTR;
                fmt_opts.precision = (int)(sizeof(void*) * 2);
                __fallthrough;
            case 'X':
                fmt_opts.flags |= FMT_FLAGS::UPPERCASE;
                __fallthrough;
            case 'x':
                format_number_with_options(char_buf, fmt_opts, 16, prefix, argptr);
                string_ptr = char_buf;
                string_length = strlen(char_buf);

                /* Only add 0x/0X prefix for non-empty numeric output. If the
                   value is 0 (numeric string is "0") the prefix
                   should not be added. Also, when precision == 0 the
                   numeric output is empty and no prefix should be added. */
                if ((fmt_opts.flags & FMT_FLAGS::SPECIAL) &&
                    (char_buf[0] != '\0') &&
                    (char_buf[0] != '0'))
                {
                    prefix = (fmt_opts.flags & FMT_FLAGS::UPPERCASE) ? "0X" : "0x";
                }
                break;

            case 'S':
                fmt_opts.flags |= FMT_FLAGS::SIZE_CHAR_OTHER;
                __fallthrough;
            case 's':
                /* Argument is either 'const char*' or `const wchar*' */
                string_ptr = va_arg(argptr, const void*);
                if (string_ptr == nullptr)
                {
                    string_ptr = "(null)";
                    string_length = 6;
                    string_is_wide = false;
                }
                else
                {
                    string_is_wide = is_wide_char_format<TCHAR>(fmt_opts.flags);
                    if (string_is_wide)
                    {
                        string_length = wcslen((const wchar_t*)string_ptr);
                    }
                    else
                    {
                        string_length = strlen((const char*)string_ptr);
                    }
                }
                if (fmt_opts.precision >= 0)
                {
                    string_length = min(string_length, (size_t)fmt_opts.precision);
                    fmt_opts.precision = -1;
                }
                break;

            case 'Z':
            {
                /* Pointer to NT-style string (ANSI_STRING or UNICODE_STRING) */
                const NT_STRING* nt_string = va_arg(argptr, const NT_STRING*);
                if ((nt_string == nullptr) || (nt_string->Buffer == nullptr))
                {
                    string_ptr = "(null)";
                    string_length = 6;
                    string_is_wide = false;
                }
                else
                {
                    string_ptr = nt_string->Buffer;
                    string_is_wide = (fmt_opts.flags & FMT_FLAGS::SIZE_w);
                    if (string_is_wide)
                    {
                        string_length = nt_string->Length / sizeof(wchar_t);
                    }
                    else
                    {
                        string_length = nt_string->Length / sizeof(char);
                    }
                }
                break;
            }

            default:
            {
                /* Unsupported format specifier, continue writing with the current char */
                start = format;
                continue;
            }
        } // end switch

        /* If we got here, we have a string to output! */
        size_t field_length = string_length;
        if (prefix != nullptr)
        {
            field_length += strlen(prefix);
        }

        /* Handle precision */
        size_t precision_padding = 0;
        if (fmt_opts.precision >= 0)
        {
            if (fmt_opts.precision > (int)string_length)
            {
                precision_padding = fmt_opts.precision - string_length;
                field_length += precision_padding;
            }

            /* If precision is specified, '0' flag is ignored */
            fmt_opts.flags &= ~FMT_FLAGS::PAD_ZERO;
        }

        /* Calculate required padding */
        size_t field_width_padding = 0;
        if (fmt_opts.width > 0 && (size_t)fmt_opts.width > field_length)
        {
            field_width_padding = (size_t)fmt_opts.width - field_length;
        }

        /* Output left space padding */
        if ((field_width_padding != 0) &&
            !(fmt_opts.flags & FMT_FLAGS::ALIGN_LEFT) &&
            !(fmt_opts.flags & FMT_FLAGS::PAD_ZERO))
        {
            _tcsoutput_ascii_chars(output, ' ', field_width_padding);
        }

        /* Output optional prefix */
        if (prefix != nullptr)
        {
            size_t prefixLen = strlen(prefix);
            _tcsoutput_astring(output, prefix, prefixLen);
        }

        /* Output 0 padding for alignment */
        if (!(fmt_opts.flags & FMT_FLAGS::ALIGN_LEFT) &&
            (fmt_opts.flags & FMT_FLAGS::PAD_ZERO))
        {
            _tcsoutput_ascii_chars(output, '0', field_width_padding);
        }

        /* Output 0 padding for precision */
        if (precision_padding > 0)
        {
            _tcsoutput_ascii_chars(output, '0', precision_padding);
        }

        /* Output the string itself */
        if (string_is_wide)
        {
            _tcsoutput_wstring(output, (const wchar_t*)string_ptr, string_length);
        }
        else
        {
            _tcsoutput_astring(output, (const char*)string_ptr, string_length);
        }

        /* Output right space padding */
        if ((fmt_opts.flags & FMT_FLAGS::ALIGN_LEFT))
        {
            _tcsoutput_ascii_chars(output, ' ', field_width_padding);
        }

        if (output.CurrentPosition > INT_MAX)
        {
            break;
        }

        /* We have completed the format specifier, start fresh at the next character */
        start = ++format;
    }

    /* Terminate the output, if enough space is available */
    if (output.CurrentPosition < BufferCount)
    {
       Buffer[output.CurrentPosition] = 0;
    }
    else
    {
        /* Otherwise check for buffer overflow */
        if ((output.CurrentPosition > BufferCount) && (Buffer != nullptr))
        {
            return -1;
        }
    }
 
    return (int)output.CurrentPosition;
}

template<typename TCHAR>
int
_tcsoutput_s(
    _Out_writes_opt_(_BufferCount) _Post_maybez_ TCHAR* const Buffer,
    _In_ size_t const BufferCount,
    _In_ size_t const MaxCount,
    _In_z_ _Printf_format_string_ const TCHAR* const Format,
    va_list const ArgList)
{
    if (Format == nullptr)
    {
        return -1;
    }

    // Check if we have a valid buffer
    if (Buffer == nullptr)
    {
        return ((BufferCount == 0) && (MaxCount == 0)) ? 0 : -1;
    }

    if (BufferCount == 0)
    {
        return -1;
    }

    // Calculate the maximum number of characters to write.
    // Include space for the null terminator, if the buffer is large enough to hold it.
    // Otherwise, we will null terminate ourselves later. DO NOT USE min HERE!
    size_t maxcount = (MaxCount < BufferCount) ? (MaxCount + 1) : BufferCount;

    // Call the main function to do the dirty work
    int result = _tcsoutput<TCHAR>(Buffer, maxcount, Format, ArgList);

    // Check for failure (buffer overflow or other error)
    if ((result < 0) || ((size_t)result >= maxcount))
    {
        // Always null-terminate at the end of the buffer
        Buffer[maxcount - 1] = 0;

        // If MaxCount did not specify _TRUNCATE and the output was not null-terminated
        // due to the buffer size being too small, set the entire string to empty.
        if ((MaxCount != _TRUNCATE) && (BufferCount <= MaxCount))
        {
            Buffer[0] = 0;
        }
        return -1;
    }

    return result;
}

// Explicitly instantiate the templates for char and wchar_t
template int _tcsoutput<char>(char*, size_t, const char*, va_list);
template int _tcsoutput<wchar_t>(wchar_t*, size_t, const wchar_t*, va_list);

template int _tcsoutput_s<char>(char*, size_t, size_t, const char*, va_list);
template int _tcsoutput_s<wchar_t>(wchar_t*, size_t, size_t, const wchar_t*, va_list);
