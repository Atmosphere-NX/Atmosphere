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

namespace ams::lm::detail {

    namespace {

        os::SdkMutex g_log_observer_lock;
        u8 g_packet_transmitter_buffer[0x400];

        bool LogPacketTransmitterFlushFunction(const void *data, size_t data_size) {
            /* TODO: lm OpenLogger, Log, SetDestination libnx bindings... */
            return true;
        }

        constexpr const char DefaultInvalidString[] = "(Invalid UTF8 string)";

    }

    void LogManagerLogObserver(const diag::LogMetaData &log_metadata, const diag::LogBody &log_body, void *user_data) {
        AMS_ASSERT(!log_metadata.use_default_locale_charset);
        std::scoped_lock lk(g_log_observer_lock);

        LogPacketTransmitter packet_transmitter(g_packet_transmitter_buffer, sizeof(g_packet_transmitter_buffer), LogPacketTransmitterFlushFunction, log_metadata.log_severity, log_metadata.verbosity, 0, log_body.log_is_head, log_body.log_is_tail);
        if (log_body.log_is_head) {
            /* Push time. */
            auto system_tick = os::GetSystemTick();
            auto tick_ts = os::ConvertToTimeSpan(system_tick);
            packet_transmitter.PushUserSystemClock(static_cast<u64>(tick_ts.GetSeconds()));

            /* Push line number. */
            auto line_number = log_metadata.source_info.line_number;
            packet_transmitter.PushLineNumber(line_number);

            /* Push file name. */
            auto file_name = log_metadata.source_info.file_name;
            auto file_name_len = strlen(file_name);
            if (file_name /* && !util::VerifyUtf8String(file_name, file_name_len) */) {
                file_name = DefaultInvalidString;
                file_name_len = strlen(DefaultInvalidString);
            }
            packet_transmitter.PushFileName(file_name, file_name_len);

            /* Push function name. */
            auto function_name = log_metadata.source_info.function_name;
            auto function_name_len = strlen(function_name);
            if (function_name /* && !util::VerifyUtf8String(function_name, function_name_len) */) {
                function_name = DefaultInvalidString;
                function_name_len = strlen(DefaultInvalidString);
            }
            packet_transmitter.PushFunctionName(function_name, function_name_len);

            /* Push module name. */
            auto module_name = log_metadata.module_name;
            auto module_name_len = strlen(module_name);
            if (module_name /* && !util::VerifyUtf8String(module_name, module_name_len) */) {
                module_name = DefaultInvalidString;
                module_name_len = strlen(DefaultInvalidString);
            }
            packet_transmitter.PushModuleName(module_name, module_name_len);

            /* Push thread name. */
            auto thread_name = os::GetThreadNamePointer(os::GetCurrentThread());
            auto thread_name_len = strlen(thread_name);
            if (thread_name /* && !util::VerifyUtf8String(thread_name, thread_name_len) */) {
                thread_name = DefaultInvalidString;
                thread_name_len = strlen(DefaultInvalidString);
            }
            packet_transmitter.PushThreadName(thread_name, thread_name_len);

            /* TODO: Push process name. */
            /*
            const char *process_name;
            size_t process_name_len;
            diag::detail::GetProcessNamePointer(std::addressof(process_name), std::addressof(process_name_len));
            packet_transmitter.PushProcessName(process_name, process_name_len);
            */
        }

        /* Push text log. */
        packet_transmitter.PushTextLog(log_body.log_text, log_body.log_text_length);

        /* Packet transmitter's destructor flushes everything automatically. */
    }

}
