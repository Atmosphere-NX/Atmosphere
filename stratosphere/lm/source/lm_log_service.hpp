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
#pragma once
#include "detail/lm_log_packet.hpp"

namespace ams::lm {

    enum LogDestination : u32 {
        LogDestination_TMA = (1 << 0),
        LogDestination_UART = (1 << 1),
        LogDestination_UARTSleeping = (1 << 2)
    };

    namespace impl {

        #define AMS_LM_I_LOGGER_INFO(C, H)                                                              \
        AMS_SF_METHOD_INFO(C, H,  0, void, Log,            (const sf::InAutoSelectBuffer &log_buffer))  \
        AMS_SF_METHOD_INFO(C, H,  1, void, SetDestination, (LogDestination log_destination))

        AMS_SF_DEFINE_INTERFACE(ILogger, AMS_LM_I_LOGGER_INFO)

        #define AMS_LM_I_LOG_SERVICE_INFO(C, H)                                                                                                       \
        AMS_SF_METHOD_INFO(C, H,  0, void, OpenLogger,     (const sf::ClientProcessId &client_pid, sf::Out<std::shared_ptr<ILogger>> out_logger))

        AMS_SF_DEFINE_INTERFACE(ILogService, AMS_LM_I_LOG_SERVICE_INFO)

    }

    class Logger {
        private:
            os::ProcessId process_id;
        public:
            Logger(os::ProcessId process_id);
            ~Logger();

            void *operator new(size_t size);
            void operator delete(void *p);

            void Log(const sf::InAutoSelectBuffer &log_buffer);
            void SetDestination(LogDestination log_destination);
    };
    static_assert(impl::IsILogger<Logger>);

    class LogService {
        public:
            void OpenLogger(const sf::ClientProcessId &client_pid, sf::Out<std::shared_ptr<impl::ILogger>> out_logger);
    };
    static_assert(impl::IsILogService<LogService>);

}