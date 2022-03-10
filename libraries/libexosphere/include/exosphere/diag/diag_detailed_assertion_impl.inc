/*
 * Copyright (c) Atmosphère-NX
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

void AbortImpl(const char *expr, const char *func, const char *file, int line) {
    #if defined(AMS_ENABLE_DETAILED_ASSERTIONS)
    {
        AMS_LOG("Abort Called\n");
        AMS_LOG("    Location:   %s:%d\n", file, line);
        AMS_LOG("    Function:   %s\n", func);
        AMS_LOG("    Expression: %s\n", expr);
        AMS_LOG("\n");
    }
    #else
    AMS_UNUSED(file, line, func, expr);
    #endif
    AbortImpl();
}

void AbortImpl(const char *expr, const char *func, const char *file, int line, const char *format, ...) {
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
    AMS_UNUSED(file, line, func, expr, format);
    #endif
    AbortImpl();
}

void AbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const char *format, ...) {
    #if defined(AMS_ENABLE_DETAILED_ASSERTIONS)
    {
        AMS_LOG("Abort Called\n");
        AMS_LOG("    Location:   %s:%d\n", file, line);
        AMS_LOG("    Function:   %s\n", func);
        AMS_LOG("    Expression: %s\n", expr);
        AMS_LOG("    Result:     0x%08" PRIx32 "\n", result->GetValue());
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
    AMS_UNUSED(file, line, func, expr, result, format);
    #endif
    AbortImpl();
}

void OnAssertionFailure(AssertionType type, const char *expr, const char *func, const char *file, int line) {
    #if defined(AMS_ENABLE_DETAILED_ASSERTIONS)
    {
        AMS_LOG("Assertion Failure\n");
        AMS_LOG("    Location:   %s:%d\n", file, line);
        AMS_LOG("    Function:   %s\n", func);
        AMS_LOG("    Expression: %s\n", expr);
        AMS_LOG("    Value:      %016" PRIx64 "\n", value);
        AMS_LOG("\n");

        AMS_UNUSED(type);
    }
    #else
    AMS_UNUSED(type, expr, func, file, line);
    #endif
    AbortImpl();
}

void OnAssertionFailure(AssertionType type, const char *expr, const char *func, const char *file, int line, const char *format, ...) {
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

        AMS_UNUSED(type);
    }
    #else
    AMS_UNUSED(type, expr, func, file, line, format);
    #endif
    AbortImpl();
}
