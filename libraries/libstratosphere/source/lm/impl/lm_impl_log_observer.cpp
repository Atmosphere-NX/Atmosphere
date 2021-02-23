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

namespace ams::lm::impl {

    namespace {

        os::SdkMutex g_log_observer_lock;
        u8 g_packet_transmitter_buffer[0x400];

        bool LogPacketTransmitterFlushFunction(const u8 *log_data, size_t log_data_size) {
            /* TODO: libnx bindings. */
            return true;
        }

        constexpr const char DefaultInvalidString[] = "(Invalid UTF8 string)";

    }

    void LogManagerLogObserver(const diag::LogMetaData &log_metadata, const diag::LogBody &log_body, void *user_data) {
        AMS_ASSERT(!log_metadata.use_default_locale_charset);
        std::scoped_lock lk(g_log_observer_lock);

        LogPacketTransmitter packet_transmitter(g_packet_transmitter_buffer, sizeof(g_packet_transmitter_buffer), LogPacketTransmitterFlushFunction, log_metadata.log_severity, log_metadata.verbosity, 0, log_body.is_head, log_body.is_tail);
        if (log_body.is_head) {
            /* Push time. */
            packet_transmitter.PushUserSystemClock(static_cast<u64>(os::ConvertToTimeSpan(os::GetSystemTick()).GetSeconds()));

            /* Push line number. */
            packet_transmitter.PushLineNumber(log_metadata.source_info.line_number);

            /* Push file name. */
            if (std::strlen(log_metadata.source_info.file_name) /* && !util::VerifyUtf8String(log_metadata.source_info.file_name, std::strlen(log_metadata.source_info.file_name)) */) {
                packet_transmitter.PushFileName(DefaultInvalidString, __builtin_strlen(DefaultInvalidString));
            }
            else {
                packet_transmitter.PushFileName(log_metadata.source_info.file_name, std::strlen(log_metadata.source_info.file_name));
            }

            /* Push function name. */
            if (std::strlen(log_metadata.source_info.function_name) /* && !util::VerifyUtf8String(log_metadata.source_info.function_name, std::strlen(log_metadata.source_info.function_name)) */) {
                packet_transmitter.PushFunctionName(DefaultInvalidString, __builtin_strlen(DefaultInvalidString));
            }
            else {
                packet_transmitter.PushFunctionName(log_metadata.source_info.function_name, std::strlen(log_metadata.source_info.function_name));
            }

            /* Push module name. */
            if (std::strlen(log_metadata.module_name) /* && !util::VerifyUtf8String(log_metadata.module_name, std::strlen(log_metadata.module_name)) */) {
                packet_transmitter.PushModuleName(DefaultInvalidString, __builtin_strlen(DefaultInvalidString));
            }
            else {
                packet_transmitter.PushModuleName(log_metadata.module_name, std::strlen(log_metadata.module_name));
            }

            /* Push thread name. */
            const auto thread_name = os::GetThreadNamePointer(os::GetCurrentThread());
            if (std::strlen(thread_name) /* && !util::VerifyUtf8String(thread_name, std::strlen(thread_name)) */) {
                packet_transmitter.PushThreadName(DefaultInvalidString, __builtin_strlen(DefaultInvalidString));
            }
            else {
                packet_transmitter.PushThreadName(thread_name, std::strlen(thread_name));
            }

            /* TODO: Push process name. */
            /*
            const char *process_name;
            size_t process_name_len;
            diag::impl::GetProcessNamePointer(std::addressof(process_name), std::addressof(process_name_len));
            packet_transmitter.PushProcessName(process_name, process_name_len);
            */
        }

        /* Push text log. */
        packet_transmitter.PushTextLog(log_body.text, log_body.text_length);

        /* Packet transmitter's destructor flushes everything automatically. */
    }

}
