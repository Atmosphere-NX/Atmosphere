/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <mesosphere.hpp>
#include "kern_debug_log_impl.hpp"

namespace ams::kern {
    #pragma GCC push_options
    #pragma GCC optimize ("-Os")

    namespace {

        /* Useful definitions for our VSNPrintf implementation. */
        enum FormatSpecifierFlag : u32 {
            FormatSpecifierFlag_None         = 0,
            FormatSpecifierFlag_EmptySign    = (1 << 0),
            FormatSpecifierFlag_ForceSign    = (1 << 1),
            FormatSpecifierFlag_Hash         = (1 << 2),
            FormatSpecifierFlag_LeftJustify  = (1 << 3),
            FormatSpecifierFlag_ZeroPad      = (1 << 4),
            FormatSpecifierFlag_Char         = (1 << 5),
            FormatSpecifierFlag_Short        = (1 << 6),
            FormatSpecifierFlag_Long         = (1 << 7),
            FormatSpecifierFlag_LongLong     = (1 << 8),
            FormatSpecifierFlag_Uppercase    = (1 << 9),
            FormatSpecifierFlag_HasPrecision = (1 << 10),
        };

        using FormatSpecifierFlagStorage = std::underlying_type<FormatSpecifierFlag>::type;

        constexpr ALWAYS_INLINE bool IsDigit(char c) {
            return '0' <= c && c <= '9';
        }

        constexpr ALWAYS_INLINE u32 ParseU32(const char *&str) {
            u32 value = 0;
            do {
                value = (value * 10) + static_cast<u32>(*(str++) - '0');
            } while (IsDigit(*str));
            return value;
        }

        constexpr ALWAYS_INLINE size_t Strnlen(const char *str, size_t max) {
            const char *cur = str;
            while (*cur && max--) {
                cur++;
            }
            return static_cast<size_t>(cur - str);
        }

        ALWAYS_INLINE void VSNPrintfImpl(char * const dst, const size_t dst_size, const char *format, ::std::va_list vl) {
            size_t dst_index = 0;

            auto WriteCharacter = [dst, dst_size, &dst_index](char c) ALWAYS_INLINE_LAMBDA {
                if (dst_index < dst_size) {
                    dst[dst_index++] = c;
                }
            };

            /* Loop over every character in the string, looking for format specifiers. */
            while (*format) {
                if (const char c = *(format++); c != '%') {
                    WriteCharacter(c);
                    continue;
                }

                /* We have to parse a format specifier. */
                /* Start by parsing flags. */
                FormatSpecifierFlagStorage flags = FormatSpecifierFlag_None;
                auto SetFlag   = [&flags](FormatSpecifierFlag f) ALWAYS_INLINE_LAMBDA { flags |= f; };
                auto ClearFlag = [&flags](FormatSpecifierFlag f) ALWAYS_INLINE_LAMBDA { flags &= ~f; };
                auto HasFlag   = [&flags](FormatSpecifierFlag f) ALWAYS_INLINE_LAMBDA { return (flags & f) != 0; };
                {
                    bool parsed_flags = false;
                    while (!parsed_flags) {
                        switch (*format) {
                            case ' ': SetFlag(FormatSpecifierFlag_EmptySign);   format++; break;
                            case '+': SetFlag(FormatSpecifierFlag_ForceSign);   format++; break;
                            case '#': SetFlag(FormatSpecifierFlag_Hash);        format++; break;
                            case '-': SetFlag(FormatSpecifierFlag_LeftJustify); format++; break;
                            case '0': SetFlag(FormatSpecifierFlag_ZeroPad);     format++; break;
                            default:
                                parsed_flags = true;
                                break;
                        }
                    }
                }

                /* Next, parse width. */
                u32 width = 0;
                if (IsDigit(*format)) {
                    /* Integer width. */
                    width = ParseU32(format);
                } else if (*format == '*') {
                    /* Dynamic width. */
                    const int _width = va_arg(vl, int);
                    if (_width >= 0) {
                        width = static_cast<u32>(_width);
                    } else {
                        SetFlag(FormatSpecifierFlag_LeftJustify);
                        width = static_cast<u32>(-_width);
                    }
                    format++;
                }

                /* Next, parse precision if present. */
                u32 precision = 0;
                if (*format == '.') {
                    SetFlag(FormatSpecifierFlag_HasPrecision);
                    format++;

                    if (IsDigit(*format)) {
                        /* Integer precision. */
                        precision = ParseU32(format);
                    } else if (*format == '*') {
                        /* Dynamic precision. */
                        const int _precision = va_arg(vl, int);
                        if (_precision > 0) {
                            precision = static_cast<u32>(_precision);
                        }
                        format++;
                    }
                }

                /* Parse length. */
                constexpr bool SizeIsLong    = sizeof(size_t)    == sizeof(long);
                constexpr bool IntMaxIsLong  = sizeof(intmax_t)  == sizeof(long);
                constexpr bool PtrDiffIsLong = sizeof(ptrdiff_t) == sizeof(long);
                switch (*format) {
                    case 'z':
                        SetFlag(SizeIsLong    ? FormatSpecifierFlag_Long : FormatSpecifierFlag_LongLong);
                        format++;
                        break;
                    case 'j':
                        SetFlag(IntMaxIsLong  ? FormatSpecifierFlag_Long : FormatSpecifierFlag_LongLong);
                        format++;
                        break;
                    case 't':
                        SetFlag(PtrDiffIsLong ? FormatSpecifierFlag_Long : FormatSpecifierFlag_LongLong);
                        format++;
                        break;
                    case 'h':
                        SetFlag(FormatSpecifierFlag_Short);
                        format++;
                        if (*format == 'h') {
                            SetFlag(FormatSpecifierFlag_Char);
                            format++;
                        }
                        break;
                    case 'l':
                        SetFlag(FormatSpecifierFlag_Long);
                        format++;
                        if (*format == 'l') {
                            SetFlag(FormatSpecifierFlag_LongLong);
                            format++;
                        }
                        break;
                    default:
                        break;
                }

                const char specifier = *(format++);
                switch (specifier) {
                    case 'p':
                        if constexpr (sizeof(uintptr_t) == sizeof(long long)) {
                            SetFlag(FormatSpecifierFlag_LongLong);
                        } else {
                            SetFlag(FormatSpecifierFlag_Long);
                        }
                        SetFlag(FormatSpecifierFlag_Hash);
                        [[fallthrough]];
                    case 'd':
                    case 'i':
                    case 'u':
                    case 'b':
                    case 'o':
                    case 'x':
                    case 'X':
                        {
                            /* Determine the base to print with. */
                            u32 base;
                            switch (specifier) {
                                case 'b':
                                    base = 2;
                                    break;
                                case 'o':
                                    base = 8;
                                    break;
                                case 'X':
                                    SetFlag(FormatSpecifierFlag_Uppercase);
                                    [[fallthrough]];
                                case 'p':
                                case 'x':
                                    base = 16;
                                    break;
                                default:
                                    base = 10;
                                    ClearFlag(FormatSpecifierFlag_Hash);
                                    break;
                            }

                            /* Precision implies no zero-padding. */
                            if (HasFlag(FormatSpecifierFlag_HasPrecision)) {
                                ClearFlag(FormatSpecifierFlag_ZeroPad);
                            }

                            /* Unsigned types don't get signs. */
                            const bool is_unsigned = base != 10 || specifier == 'u';
                            if (is_unsigned) {
                                ClearFlag(FormatSpecifierFlag_EmptySign);
                                ClearFlag(FormatSpecifierFlag_ForceSign);
                            }

                            auto PrintInteger = [&](bool negative, uintmax_t value) {
                                constexpr size_t BufferSize = 64; /* Binary digits for 64-bit numbers may use 64 digits. */
                                char buf[BufferSize];
                                size_t len = 0;

                                /* No hash flag for zero. */
                                if (value == 0) {
                                    ClearFlag(FormatSpecifierFlag_Hash);
                                }

                                if (!HasFlag(FormatSpecifierFlag_HasPrecision) || value != 0) {
                                    do {
                                        const char digit = static_cast<char>(value % base);
                                        buf[len++] = (digit < 10) ? ('0' + digit) : ((HasFlag(FormatSpecifierFlag_Uppercase) ? 'A' : 'a') + digit - 10);
                                        value /= base;
                                    } while (value);
                                }

                                /* Determine our prefix length. */
                                size_t prefix_len = 0;
                                const bool has_sign = negative || HasFlag(FormatSpecifierFlag_ForceSign) || HasFlag(FormatSpecifierFlag_EmptySign);
                                if (has_sign) {
                                    prefix_len++;
                                }
                                if (HasFlag(FormatSpecifierFlag_Hash)) {
                                    prefix_len += (base != 8) ? 2 : 1;
                                }

                                /* Determine zero-padding count. */
                                size_t num_zeroes = (len < precision) ? precision - len : 0;
                                if (!HasFlag(FormatSpecifierFlag_LeftJustify) && HasFlag(FormatSpecifierFlag_ZeroPad)) {
                                    num_zeroes = (len + prefix_len < width) ? width - len - prefix_len : 0;
                                }

                                /* Print out left padding. */
                                if (!HasFlag(FormatSpecifierFlag_LeftJustify)) {
                                    for (size_t i = len + prefix_len + num_zeroes; i < static_cast<size_t>(width); i++) {
                                        WriteCharacter(' ');
                                    }
                                }

                                /* Print out sign. */
                                if (negative) {
                                    WriteCharacter('-');
                                } else if (HasFlag(FormatSpecifierFlag_ForceSign)) {
                                    WriteCharacter('+');
                                } else if (HasFlag(FormatSpecifierFlag_EmptySign)) {
                                    WriteCharacter(' ');
                                }

                                /* Print out base prefix. */
                                if (HasFlag(FormatSpecifierFlag_Hash)) {
                                    WriteCharacter('0');
                                    if (base == 2) {
                                        WriteCharacter('b');
                                    } else if (base == 16) {
                                        WriteCharacter('x');
                                    }
                                }

                                /* Print out zeroes. */
                                for (size_t i = 0; i < num_zeroes; i++) {
                                    WriteCharacter('0');
                                }

                                /* Print out digits. */
                                for (size_t i = 0; i < len; i++) {
                                    WriteCharacter(buf[len - 1 - i]);
                                }

                                /* Print out right padding. */
                                if (HasFlag(FormatSpecifierFlag_LeftJustify)) {
                                    for (size_t i = len + prefix_len + num_zeroes; i < static_cast<size_t>(width); i++) {
                                        WriteCharacter(' ');
                                    }
                                }
                            };

                            /* Output the integer. */
                            if (is_unsigned) {
                                uintmax_t n = 0;
                                if (HasFlag(FormatSpecifierFlag_LongLong)) {
                                    n = static_cast<unsigned long long>(va_arg(vl, unsigned long long));
                                } else if (HasFlag(FormatSpecifierFlag_Long)) {
                                    n = static_cast<unsigned long>(va_arg(vl, unsigned long));
                                } else if (HasFlag(FormatSpecifierFlag_Char)) {
                                    n = static_cast<unsigned char>(va_arg(vl, unsigned int));
                                } else if (HasFlag(FormatSpecifierFlag_Short)) {
                                    n = static_cast<unsigned short>(va_arg(vl, unsigned int));
                                } else {
                                    n = static_cast<unsigned int>(va_arg(vl, unsigned int));
                                }
                                if (specifier == 'p' && n == 0) {
                                    WriteCharacter('(');
                                    WriteCharacter('n');
                                    WriteCharacter('i');
                                    WriteCharacter('l');
                                    WriteCharacter(')');
                                } else {
                                    PrintInteger(false, n);
                                }
                            } else {
                                intmax_t n = 0;
                                if (HasFlag(FormatSpecifierFlag_LongLong)) {
                                    n = static_cast<signed long long>(va_arg(vl, signed long long));
                                } else if (HasFlag(FormatSpecifierFlag_Long)) {
                                    n = static_cast<signed long>(va_arg(vl, signed long));
                                } else if (HasFlag(FormatSpecifierFlag_Char)) {
                                    n = static_cast<signed char>(va_arg(vl, signed int));
                                } else if (HasFlag(FormatSpecifierFlag_Short)) {
                                    n = static_cast<signed short>(va_arg(vl, signed int));
                                } else {
                                    n = static_cast<signed int>(va_arg(vl, signed int));
                                }
                                const bool negative = n < 0;
                                const uintmax_t u = (negative) ? static_cast<uintmax_t>(-n) : static_cast<uintmax_t>(n);
                                PrintInteger(negative, u);
                            }
                        }
                        break;
                    case 'c':
                        {
                            size_t len = 1;
                            if (!HasFlag(FormatSpecifierFlag_LeftJustify)) {
                                while (len++ < width) {
                                    WriteCharacter(' ');
                                }
                            }
                            WriteCharacter(static_cast<char>(va_arg(vl, int)));
                            if (HasFlag(FormatSpecifierFlag_LeftJustify)) {
                                while (len++ < width) {
                                    WriteCharacter(' ');
                                }
                            }
                        }
                        break;
                    case 's':
                        {
                            const char *str = va_arg(vl, char *);
                            if (str == nullptr) {
                                str = "(null)";
                            }

                            size_t len = Strnlen(str, precision > 0 ? precision : std::numeric_limits<size_t>::max());
                            if (HasFlag(FormatSpecifierFlag_HasPrecision)) {
                                len = (len < precision) ? len : precision;
                            }
                            if (!HasFlag(FormatSpecifierFlag_LeftJustify)) {
                                while (len++ < width) {
                                    WriteCharacter(' ');
                                }
                            }
                            while (*str && (!HasFlag(FormatSpecifierFlag_HasPrecision) || (precision--) != 0)) {
                                WriteCharacter(*(str++));
                            }
                            if (HasFlag(FormatSpecifierFlag_LeftJustify)) {
                                while (len++ < width) {
                                    WriteCharacter(' ');
                                }
                            }
                        }
                        break;
                    case '%':
                    default:
                        WriteCharacter(specifier);
                        break;
                }
            }

            /* Ensure null termination. */
            WriteCharacter('\0');
            dst[dst_size - 1] = '\0';
        }

        KSpinLock g_debug_log_lock;
        bool      g_initialized_impl;

        /* NOTE: Nintendo's print buffer is size 0x100. */
        char g_print_buffer[0x400];

        void PutString(const char *str) {
            /* Only print if the implementation is initialized. */
            if (!g_initialized_impl) {
                return;
            }

            while (*str) {
                /* Get a character. */
                const char c = *(str++);

                /* Print the character. */
                if (c == '\n') {
                    KDebugLogImpl::PutChar('\r');
                }
                KDebugLogImpl::PutChar(c);
            }

            KDebugLogImpl::Flush();
        }

        #if defined(MESOSPHERE_ENABLE_DEBUG_PRINT)

        Result PutUserString(ams::kern::svc::KUserPointer<const char *> user_str, size_t len) {
            /* Only print if the implementation is initialized. */
            if (!g_initialized_impl) {
                return ResultSuccess();
            }

            for (size_t i = 0; i < len; ++i) {
                /* Get a character. */
                char c;
                R_TRY(user_str.CopyArrayElementTo(std::addressof(c), i));

                /* Print the character. */
                if (c == '\n') {
                    KDebugLogImpl::PutChar('\r');
                }
                KDebugLogImpl::PutChar(c);
            }

            KDebugLogImpl::Flush();

            return ResultSuccess();
        }

        #endif

    }


    #pragma GCC pop_options

    void KDebugLog::Initialize() {
        if (KTargetSystem::IsDebugLoggingEnabled()) {
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_debug_log_lock);

            if (!g_initialized_impl) {
                g_initialized_impl = KDebugLogImpl::Initialize();
            }
        }
    }

    void KDebugLog::Printf(const char *format, ...) {
        if (KTargetSystem::IsDebugLoggingEnabled()) {
            ::std::va_list vl;
            va_start(vl, format);
            VPrintf(format, vl);
            va_end(vl);
        }
    }

    void KDebugLog::VPrintf(const char *format, ::std::va_list vl) {
        if (KTargetSystem::IsDebugLoggingEnabled()) {
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_debug_log_lock);

            VSNPrintf(g_print_buffer, util::size(g_print_buffer), format, vl);
            PutString(g_print_buffer);
        }
    }

    void KDebugLog::VSNPrintf(char *dst, const size_t dst_size, const char *format, ::std::va_list vl) {
        VSNPrintfImpl(dst, dst_size, format, vl);
    }

    Result KDebugLog::PrintUserString(ams::kern::svc::KUserPointer<const char *> user_str, size_t len) {
        /* If printing is enabled, print the user string. */
        #if defined(MESOSPHERE_ENABLE_DEBUG_PRINT)
            if (KTargetSystem::IsDebugLoggingEnabled()) {
                KScopedInterruptDisable di;
                KScopedSpinLock lk(g_debug_log_lock);

                R_TRY(PutUserString(user_str, len));
            }
        #endif

        return ResultSuccess();
    }

    void KDebugLog::Save() {
        if (KTargetSystem::IsDebugLoggingEnabled()) {
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_debug_log_lock);

            KDebugLogImpl::Save();
        }
    }

    void KDebugLog::Restore() {
        if (KTargetSystem::IsDebugLoggingEnabled()) {
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_debug_log_lock);

            KDebugLogImpl::Restore();
        }
    }

}
