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
#include <stratosphere.hpp>

namespace ams {

    extern ncm::ProgramId CurrentProgramId;

}

namespace ams::diag {

    namespace {

        inline NORETURN void AbortWithValue(u64 debug) {
            /* Just perform a data abort. */
            register u64 addr __asm__("x27") = FatalErrorContext::StdAbortMagicAddress;
            register u64 val __asm__("x28")  = FatalErrorContext::StdAbortMagicValue;
            while (true) {
                __asm__ __volatile__ (
                    "mov x0, %[debug]\n"
                    "str %[val], [%[addr]]\n"
                    :
                    : [debug]"r"(debug), [val]"r"(val), [addr]"r"(addr)
                    : "x0"
                );
            }
            __builtin_unreachable();
        }

        inline void DebugLog(const char *format, ...) __attribute__((format(printf, 1, 2)));

#ifdef AMS_ENABLE_DETAILED_ASSERTIONS
        os::Mutex g_debug_log_lock(true);
        char g_debug_buffer[0x400];

        void DebugLogImpl(const char *format, ::std::va_list vl) {
            std::scoped_lock lk(g_debug_log_lock);

            std::vsnprintf(g_debug_buffer, sizeof(g_debug_buffer), format, vl);

            svc::OutputDebugString(g_debug_buffer, strlen(g_debug_buffer));
        }

        void DebugLog(const char *format, ...) {
            ::std::va_list vl;
            va_start(vl, format);
            DebugLogImpl(format, vl);
            va_end(vl);
        }

#else
        void DebugLog(const char *format, ...)  { /* ... */ }
#endif

    }

    NORETURN WEAK_SYMBOL void AssertionFailureImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) {
        DebugLog("%016lx: Assertion Failure\n", static_cast<u64>(ams::CurrentProgramId));
        DebugLog("        Location:   %s:%d\n", file, line);
        DebugLog("        Function:   %s\n", func);
        DebugLog("        Expression: %s\n", expr);
        DebugLog("        Value:      %016" PRIx64 "\n", value);
        DebugLog("\n");
#ifdef AMS_ENABLE_DETAILED_ASSERTIONS
        {
            ::std::va_list vl;
            va_start(vl, format);
            DebugLogImpl(format, vl);
            va_end(vl);
        }
#endif
        DebugLog("\n");

        AbortWithValue(value);
    }

    NORETURN WEAK_SYMBOL void AssertionFailureImpl(const char *file, int line, const char *func, const char *expr, u64 value) {
        DebugLog("%016lx: Assertion Failure\n", static_cast<u64>(ams::CurrentProgramId));
        DebugLog("        Location:   %s:%d\n", file, line);
        DebugLog("        Function:   %s\n", func);
        DebugLog("        Expression: %s\n", expr);
        DebugLog("        Value:      %016" PRIx64 "\n", value);
        DebugLog("\n");
        DebugLog("\n");

        AbortWithValue(value);
    }

    NORETURN WEAK_SYMBOL void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) {
        DebugLog("%016lx: Abort Called\n", static_cast<u64>(ams::CurrentProgramId));
        DebugLog("        Location:   %s:%d\n", file, line);
        DebugLog("        Function:   %s\n", func);
        DebugLog("        Expression: %s\n", expr);
        DebugLog("        Value:      %016" PRIx64 "\n", value);
        DebugLog("\n");
#ifdef AMS_ENABLE_DETAILED_ASSERTIONS
        {
            ::std::va_list vl;
            va_start(vl, format);
            DebugLogImpl(format, vl);
            va_end(vl);
        }
#endif
        DebugLog("\n");

        AbortWithValue(value);
    }

    NORETURN WEAK_SYMBOL void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value) {
        DebugLog("%016lx: Abort Called\n", static_cast<u64>(ams::CurrentProgramId));
        DebugLog("        Location:   %s:%d\n", file, line);
        DebugLog("        Function:   %s\n", func);
        DebugLog("        Expression: %s\n", expr);
        DebugLog("        Value:      %016" PRIx64 "\n", value);
        DebugLog("\n");
        DebugLog("\n");

        AbortWithValue(value);
    }

    NORETURN WEAK_SYMBOL void AbortImpl() {
        AbortWithValue(0);
    }

}
