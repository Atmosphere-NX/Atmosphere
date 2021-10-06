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
#include "lm_logger_impl.hpp"
#include "lm_event_log_transmitter.hpp"
#include "lm_log_buffer.hpp"
#include "lm_log_packet_parser.hpp"
#include "lm_log_getter_impl.hpp"
#include "../impl/lm_log_packet_header.hpp"

namespace ams::lm::srv {

    bool IsFlushAvailable();

    bool g_is_logging_to_custom_sink = false;

    namespace {

        constinit u32 g_log_destination = lm::LogDestination_TargetManager;

        bool SetProcessId(const sf::InAutoSelectBuffer &message, u64 process_id) {
            /* Check the message. */
            AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(message.GetPointer()), alignof(impl::LogPacketHeader)));

            /* Get a modifiable copy of the header. */
            auto *header = const_cast<impl::LogPacketHeader *>(reinterpret_cast<const impl::LogPacketHeader *>(message.GetPointer()));

            /* Check that the message size is correct. */
            if (impl::LogPacketHeaderSize + header->GetPayloadSize() != message.GetSize()) {
                return false;
            }

            /* Set the header's process id. */
            header->SetProcessId(process_id);

            return true;
        }

        void PutLogToTargetManager(const sf::InAutoSelectBuffer &message) {
            /* Try to push the message. */
            bool success;
            if (IsFlushAvailable()) {
                success = LogBuffer::GetDefaultInstance().Push(message.GetPointer(), message.GetSize());
            } else {
                success = LogBuffer::GetDefaultInstance().TryPush(message.GetPointer(), message.GetSize());
            }

            /* If we fail, increment dropped packet count. */
            if (!success) {
                EventLogTransmitter::GetDefaultInstance().IncreaseLogPacketDropCount();
            }
        }

        void PutLogToUart(const sf::InAutoSelectBuffer &message) {
            #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
            {
                /* Get header. */
                auto *data     = message.GetPointer();
                auto data_size = message.GetSize();
                const auto *header = reinterpret_cast<const impl::LogPacketHeader *>(data);

                /* Get the module name. */
                char module_name[0x10] = {};
                LogPacketParser::ParseModuleName(module_name, sizeof(module_name), data, data_size);

                /* Create log metadata. */
                const diag::LogMetaData log_meta = {
                    .module_name = module_name,
                    .severity    = static_cast<diag::LogSeverity>(header->GetSeverity()),
                    .verbosity   = header->GetVerbosity(),
                };

                LogPacketParser::ParseTextLogWithContext(message.GetPointer(), message.GetSize(), [](const char *txt, size_t size, void *arg) {
                    /* Get metadata. */
                    const auto &meta = *static_cast<const diag::LogMetaData *>(arg);

                    /* Put the message to uart. */
                    diag::impl::PutImpl(meta, txt, size);
                }, const_cast<diag::LogMetaData *>(std::addressof(log_meta)));
            }
            #else
            {
                AMS_UNUSED(message);
            }
            #endif
        }

        void PutLogToCustomSink(const sf::InAutoSelectBuffer &message) {
            LogPacketParser::ParseTextLogWithContext(message.GetPointer(), message.GetSize(), [](const char *txt, size_t size, void *) {
                /* Try to push the message. */
                if (!LogGetterImpl::GetBuffer().TryPush(txt, size)) {
                    LogGetterImpl::IncreaseLogPacketDropCount();
                }
            }, nullptr);
        }

    }

    LoggerImpl::LoggerImpl(LogServiceImpl *parent, os::ProcessId process_id) : m_parent(parent), m_process_id(process_id.value) {
        /* Log start of session for process. */
        EventLogTransmitter::GetDefaultInstance().PushLogSessionBegin(m_process_id);
    }

    LoggerImpl::~LoggerImpl() {
        /* Log end of session for process. */
        EventLogTransmitter::GetDefaultInstance().PushLogSessionEnd(m_process_id);
    }

    Result LoggerImpl::Log(const sf::InAutoSelectBuffer &message) {
        /* Try to set the log process id. */
        /* NOTE: Nintendo succeeds here, for whatever purpose, so we will as well. */
        R_UNLESS(SetProcessId(message, m_process_id), ResultSuccess());

        /* If we should, log to target manager. */
        if (g_log_destination & lm::LogDestination_TargetManager) {
            PutLogToTargetManager(message);
        }

        /* If we should, log to uart. */
        if ((g_log_destination & lm::LogDestination_Uart) || (IsFlushAvailable() && (g_log_destination & lm::LogDestination_UartIfSleep))) {
            PutLogToUart(message);
        }

        /* If we should, log to custom sink. */
        if (g_is_logging_to_custom_sink) {
            PutLogToCustomSink(message);
        }

        return ResultSuccess();
    }

    Result LoggerImpl::SetDestination(u32 destination) {
        /* Set the log destination. */
        g_log_destination = destination;
        return ResultSuccess();
    }

}
