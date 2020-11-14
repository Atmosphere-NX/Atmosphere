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

    namespace {

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
        ::ams::util::TVSNPrintf(dst, dst_size, format, vl);
    }

    Result KDebugLog::PrintUserString(ams::kern::svc::KUserPointer<const char *> user_str, size_t len) {
        /* If printing is enabled, print the user string. */
        #if defined(MESOSPHERE_ENABLE_DEBUG_PRINT)
            if (KTargetSystem::IsDebugLoggingEnabled()) {
                KScopedInterruptDisable di;
                KScopedSpinLock lk(g_debug_log_lock);

                R_TRY(PutUserString(user_str, len));
            }
        #else
            MESOSPHERE_UNUSED(user_str, len);
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
