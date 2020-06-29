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
#include <stratosphere.hpp>
#include "impl/lm_logging.hpp"

namespace ams::lm {

    class Logger : public sf::IServiceObject {

        private:
            enum class CommandId {
                Log            = 0,
                SetDestination = 1,
            };

        private:
            ncm::ProgramId program_id;
            LogDestination destination;
            std::vector<impl::LogPacketBuffer> queued_packets;
        public:
            Logger(ncm::ProgramId program_id) : program_id(program_id), destination(LogDestination::TMA), queued_packets() {}

        private:
            void Log(const sf::InAutoSelectBuffer &buf);
            void SetDestination(LogDestination destination);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(Log),
                MAKE_SERVICE_COMMAND_META(SetDestination, hos::Version_3_0_0),
            };

    };

    class LogService : public sf::IServiceObject {
        private:
            enum class CommandId {
                OpenLogger               = 0,
                AtmosphereGetLogEvent    = 65000,
                AtmosphereGetLastLogInfo = 65001,
            };

        private:
            /* Official command. */
            void OpenLogger(const sf::ClientProcessId &client_pid, sf::Out<std::shared_ptr<Logger>> out_logger);

            /* Atmosphere commands. */
            void AtmosphereGetLogEvent(sf::OutCopyHandle out_event);
            void AtmosphereGetLastLogInfo(sf::Out<s64> out_log_id, sf::Out<ncm::ProgramId> out_program_id);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(OpenLogger),
                MAKE_SERVICE_COMMAND_META(AtmosphereGetLogEvent),
                MAKE_SERVICE_COMMAND_META(AtmosphereGetLastLogInfo),
            };

    };

}