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

void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value) {
    #if defined(AMS_ENABLE_DETAILED_ASSERTIONS)
    {
        AMS_LOG("Abort Called\n");
        AMS_LOG("    Location:   %s:%d\n", file, line);
        AMS_LOG("    Function:   %s\n", func);
        AMS_LOG("    Expression: %s\n", expr);
        AMS_LOG("    Value:      %016" PRIx64 "\n", value);
        AMS_LOG("\n");
    }
    #else
    AMS_UNUSED(file, line, func, expr, value);
    #endif
    AbortImpl();
}

void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) {
    #if defined(AMS_ENABLE_DETAILED_ASSERTIONS)
    {
        AMS_LOG("Abort Called\n");
        AMS_LOG("    Location:   %s:%d\n", file, line);
        AMS_LOG("    Function:   %s\n", func);
        AMS_LOG("    Expression: %s\n", expr);
        AMS_LOG("    Value:      %016" PRIx64 "\n", value);
        AMS_LOG("\n");
        {
            ::std::va_list vl;
            va_start(vl, format);
            AMS_VLOG(format, vl);
            va_end(vl);
            AMS_LOG("\n");
        }
    }
    #else
    AMS_UNUSED(file, line, func, expr, value, format);
    #endif
    AbortImpl();
}

void AssertionFailureImpl(const char *file, int line, const char *func, const char *expr, u64 value) {
    #if defined(AMS_ENABLE_DETAILED_ASSERTIONS)
    {
        AMS_LOG("Assertion Failure\n");
        AMS_LOG("    Location:   %s:%d\n", file, line);
        AMS_LOG("    Function:   %s\n", func);
        AMS_LOG("    Expression: %s\n", expr);
        AMS_LOG("    Value:      %016" PRIx64 "\n", value);
        AMS_LOG("\n");
    }
    #else
    AMS_UNUSED(file, line, func, expr, value);
    #endif
    AbortImpl();
}

void AssertionFailureImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) {
    #if defined(AMS_ENABLE_DETAILED_ASSERTIONS)
    {
        AMS_LOG("Assertion Failure\n");
        AMS_LOG("    Location:   %s:%d\n", file, line);
        AMS_LOG("    Function:   %s\n", func);
        AMS_LOG("    Expression: %s\n", expr);
        AMS_LOG("    Value:      %016" PRIx64 "\n", value);
        AMS_LOG("\n");
        {
            ::std::va_list vl;
            va_start(vl, format);
            AMS_VLOG(format, vl);
            va_end(vl);
            AMS_LOG("\n");
        }
    }
    #else
    AMS_UNUSED(file, line, func, expr, value, format);
    #endif
    AbortImpl();
}
