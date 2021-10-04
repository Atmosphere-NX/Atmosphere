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
#include "lm_remote_log_service.hpp"
#include "impl/lm_log_packet_header.hpp"
#include "impl/lm_log_packet_transmitter.hpp"

namespace ams::diag::impl {

    void ReplaceDefaultLogObserver(LogObserver observer);
    void ResetDefaultLogObserver();

    void GetProcessNamePointer(const char **out, size_t *out_len);

}

namespace ams::lm {

    namespace {

        constinit sf::SharedPointer<ILogger> g_logger = nullptr;

        constexpr inline size_t TransmissionBufferSize  = 1_KB;
        constexpr inline size_t TransmissionBufferAlign = alignof(impl::LogPacketHeader);

        constinit os::SdkMutex g_transmission_buffer_mutex;

        alignas(TransmissionBufferAlign) constinit char g_transmission_buffer[TransmissionBufferSize];

        using PushTextFunction = void (impl::LogPacketTransmitter::*)(const char *, size_t);

        bool LogPacketTransmitterFlushFunction(const u8 *data, size_t size) {
            const Result result = g_logger->Log(sf::InAutoSelectBuffer(data, size));
            R_ABORT_UNLESS(result);
            return R_SUCCEEDED(result);
        }

        void InvokePushTextWithUtf8Sanitizing(impl::LogPacketTransmitter &transmitter, PushTextFunction push_func, const char *str) {
            /* Get the string length. */
            const auto len = std::strlen(str);

            if (len == 0 || util::VerifyUtf8String(str, len)) {
                (transmitter.*push_func)(str, len);
            } else {
                (transmitter.*push_func)("(Invalid UTF8 string)", sizeof("(Invalid UTF8 string)") - 1);
            }
        }

        void LogManagerLogObserver(const diag::LogMetaData &meta, const diag::LogBody &body, void *) {
            /* Check pre-conditions. */
            AMS_ASSERT(!meta.use_default_locale_charset);

            /* Acquire access to the transmission buffer. */
            std::scoped_lock lk(g_transmission_buffer_mutex);

            /* Create transmitter. */
            impl::LogPacketTransmitter transmitter(g_transmission_buffer, TransmissionBufferSize, LogPacketTransmitterFlushFunction, static_cast<u8>(meta.severity), static_cast<u8>(meta.verbosity), 0, body.is_head, body.is_tail);

            /* Push head-only logs. */
            if (body.is_head) {
                transmitter.PushUserSystemClock(os::GetSystemTick().ToTimeSpan().GetSeconds());
                transmitter.PushLineNumber(meta.source_info.line_number);
                InvokePushTextWithUtf8Sanitizing(transmitter, &impl::LogPacketTransmitter::PushFileName, meta.source_info.file_name);
                InvokePushTextWithUtf8Sanitizing(transmitter, &impl::LogPacketTransmitter::PushFunctionName, meta.source_info.function_name);
                InvokePushTextWithUtf8Sanitizing(transmitter, &impl::LogPacketTransmitter::PushModuleName, meta.module_name);
                InvokePushTextWithUtf8Sanitizing(transmitter, &impl::LogPacketTransmitter::PushThreadName, os::GetThreadNamePointer(os::GetCurrentThread()));

                const char *process_name;
                size_t process_name_len;
                diag::impl::GetProcessNamePointer(std::addressof(process_name), std::addressof(process_name_len));

                transmitter.PushProcessName(process_name, process_name_len);
            }

            /* Push the actual log. */
            transmitter.PushTextLog(body.message, body.message_size);
        }

    }

    void Initialize() {
        AMS_ABORT_UNLESS(g_logger == nullptr);

        /* Create the logger. */
        {
            auto service = CreateLogService();
            R_ABORT_UNLESS(service->OpenLogger(std::addressof(g_logger), sf::ClientProcessId{}));
        }

        /* Replace the default log observer. */
        diag::impl::ReplaceDefaultLogObserver(LogManagerLogObserver);
    }

    void Finalize() {
        AMS_ABORT_UNLESS(g_logger != nullptr);

        /* Reset the default log observer. */
        diag::impl::ResetDefaultLogObserver();

        /* Destroy the logger. */
        g_logger = nullptr;
    }

    void SetDestination(u32 destination) {
        g_logger->SetDestination(destination);
    }

}
