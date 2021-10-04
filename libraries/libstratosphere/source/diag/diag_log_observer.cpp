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
#include <stratosphere.hpp>
#include "diag_log_impl.hpp"
#include "impl/diag_observer_manager.hpp"
#include "impl/diag_print_debug_string.hpp"

namespace ams::diag {

    namespace impl {

        namespace {

            constexpr inline size_t DecorationStringLengthMax = 0x61;

            constexpr inline const char *EscapeSequencesForSeverity[] = {
                "\x1B[90m",         /* Dark Gray (Trace) */
                nullptr,            /* None (Info) */
                "\x1B[33m",         /* Yellow (Warn) */
                "\x1B[31m",         /* Red (Error) */
                "\x1B[41m\x1B[37m", /* White-on-red (Fatal) */
            };

            constexpr inline const char EscapeSequenceReset[] = "\x1B[0m";

            constexpr inline size_t PrintBufferLength = DecorationStringLengthMax + impl::DebugPrintBufferLength + 1;

            constinit os::SdkMutex g_print_buffer_mutex;
            constinit char g_print_buffer[PrintBufferLength];

            inline void GetCurrentTime(int *h, int *m, int *s, int *ms) {
                /* Get the current time. */
                const auto cur_time = os::GetSystemTick().ToTimeSpan();

                /* Extract fields. */
                const s64 hours        = cur_time.GetHours();
                const s64 minutes      = cur_time.GetMinutes();
                const s64 seconds      = cur_time.GetSeconds();
                const s64 milliseconds = cur_time.GetMilliSeconds();

                /* Set out fields. */
                *h  = static_cast<int>(hours);
                *m  = static_cast<int>(minutes - hours * 60);
                *s  = static_cast<int>(seconds - minutes * 60);
                *ms = static_cast<int>(milliseconds - seconds * 1000);
            }

            void TentativeDefaultLogObserver(const LogMetaData &meta, const LogBody &body, void *) {
                /* Acquire access to the print buffer */
                std::scoped_lock lk(g_print_buffer_mutex);

                /* Get the escape sequence. */
                const char *escape = nullptr;
                if (LogSeverity_Trace <= meta.severity && meta.severity <= LogSeverity_Fatal) {
                    escape = EscapeSequencesForSeverity[meta.severity];
                }

                /* Declare message variables. */
                const char *msg = nullptr;
                size_t msg_size = 0;

                /* Handle structured logs. */
                const bool structured = meta.module_name != nullptr && std::strlen(meta.module_name) >= 2;
                if (escape || structured) {
                    /* Insert timestamp, if head. */
                    if (structured && body.is_head) {
                        /* Get current timestamp. */
                        int hours, minutes, seconds, milliseconds;
                        GetCurrentTime(std::addressof(hours), std::addressof(minutes), std::addressof(seconds), std::addressof(milliseconds));

                        /* Print the timestamp/header. */
                        msg_size += util::SNPrintf(g_print_buffer + msg_size, PrintBufferLength - msg_size, "%s%d:%02d:%02d.%03d [%-5.63s] ", escape ? escape : "", hours, minutes, seconds, milliseconds, meta.module_name[0] == '$' ? meta.module_name + 1 : meta.module_name + 0);

                        AMS_AUDIT(msg_size <= DecorationStringLengthMax);
                    } else if (escape) {
                        msg_size += util::SNPrintf(g_print_buffer + msg_size, PrintBufferLength - msg_size, "%s", escape);
                    }

                    /* Determine maximum remaining size. */
                    const size_t max_msg_size = PrintBufferLength - msg_size - (escape ? sizeof(EscapeSequenceReset) - 1 : 0);

                    /* Determine printable size. */
                    size_t printable_size = std::min<size_t>(body.message_size, max_msg_size);

                    /* Determine newline status. */
                    bool new_line = false;
                    if (body.message_size > 0 && body.message[body.message_size - 1] == '\n') {
                        --printable_size;
                        new_line = true;
                    }

                    /* Print the messsage. */
                    msg_size += util::SNPrintf(g_print_buffer + msg_size, PrintBufferLength - msg_size, "%.*s%s%s", static_cast<int>(printable_size), body.message, escape ? EscapeSequenceReset : "", new_line ? "\n" : "");

                    /* Set the message. */
                    msg = g_print_buffer;
                } else {
                    /* Use the body's message directly. */
                    msg      = body.message;
                    msg_size = body.message_size;
                }

                /* Print the string. */
                impl::PrintDebugString(msg, msg_size);
            }

            struct LogObserverContext {
                const LogMetaData &meta;
                const LogBody &body;
            };

            using LogObserverManager = ObserverManagerWithDefaultHolder<LogObserverHolder, LogObserverContext>;

            constinit LogObserverManager g_log_observer_manager(::ams::diag::InitializeLogObserverHolder, TentativeDefaultLogObserver, nullptr);

        }

        void CallAllLogObserver(const LogMetaData &meta, const LogBody &body) {
            /* Create context. */
            const LogObserverContext context = { .meta = meta, .body = body };

            /* Invoke the log observer. */
            g_log_observer_manager.InvokeAllObserver(context, [] (const LogObserverHolder &holder, const LogObserverContext &context) {
                holder.log_observer(context.meta, context.body, holder.arg);
            });
        }

        void ReplaceDefaultLogObserver(LogObserver observer) {
            /* Get the default observer. */
            auto *default_holder = std::addressof(g_log_observer_manager.GetDefaultObserverHolder());

            /* Unregister, replace, and re-register. */
            UnregisterLogObserver(default_holder);
            InitializeLogObserverHolder(default_holder, observer, nullptr);
            RegisterLogObserver(default_holder);
        }

        void ResetDefaultLogObserver() {
            /* Restore the default observer. */
            ReplaceDefaultLogObserver(TentativeDefaultLogObserver);
        }

    }

    void RegisterLogObserver(LogObserverHolder *holder) {
        impl::g_log_observer_manager.RegisterObserver(holder);
    }

    void UnregisterLogObserver(LogObserverHolder *holder) {
        impl::g_log_observer_manager.UnregisterObserver(holder);
    }

}
