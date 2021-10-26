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
#include <mesosphere.hpp>
#include "kern_debug_log_impl.hpp"

namespace ams::kern {

    namespace {

        constinit KSpinLock g_debug_log_lock;
        constinit bool      g_initialized_impl;

        /* NOTE: Nintendo's print buffer is size 0x100. */
        constinit char g_print_buffer[0x400];

        void PutString(const char *str) {
            /* Only print if the implementation is initialized. */
            if (AMS_UNLIKELY(!g_initialized_impl)) {
                return;
            }

            #if defined(MESOSPHERE_DEBUG_LOG_USE_SEMIHOSTING)
            KDebugLogImpl::PutStringBySemihosting(str);
            #else
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
            #endif
        }

        #if defined(MESOSPHERE_ENABLE_DEBUG_PRINT)

        Result PutUserString(ams::kern::svc::KUserPointer<const char *> user_str, size_t len) {
            /* Only print if the implementation is initialized. */
            if (!g_initialized_impl) {
                return ResultSuccess();
            }

            #if defined(MESOSPHERE_DEBUG_LOG_USE_SEMIHOSTING)
            /* TODO: should we do this properly? */
            KDebugLogImpl::PutStringBySemihosting(user_str.GetUnsafePointer());
            MESOSPHERE_UNUSED(len);
            #else
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
            #endif

            return ResultSuccess();
        }

        #endif

        ALWAYS_INLINE void FormatU64(char * const dst, u64 value) {
            /* Adjust, so that we can print the value backwards. */
            char *cur = dst + 2 * sizeof(value);

            /* Format the value in (as %016lx) */
            while (cur > dst) {
                /* Extract the digit. */
                const auto digit = value & 0xF;
                value >>= 4;

                *(--cur) = (digit <= 9) ? ('0' + digit) : ('a' + digit - 10);
            }
        }

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

    void KDebugLog::LogException(const char *str) {
        if (KTargetSystem::IsDebugLoggingEnabled()) {
            /* Get the current program ID. */
            /* NOTE: Nintendo does this after printing the string, */
            /* but it seems wise to avoid holding the lock/disabling interrupts */
            /* for longer than is strictly necessary. */
            char suffix[18];
            if (const auto *cur_process = GetCurrentProcessPointer(); AMS_LIKELY(cur_process != nullptr)) {
                FormatU64(suffix, cur_process->GetProgramId());
                suffix[16] = '\n';
                suffix[17] = '\x00';
            } else {
                suffix[0] = '\n';
                suffix[1] = '\x00';
            }

            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_debug_log_lock);

            /* Log the string. */
            PutString(str);

            /* Log the program id (and newline) suffix. */
            PutString(suffix);
        }
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
