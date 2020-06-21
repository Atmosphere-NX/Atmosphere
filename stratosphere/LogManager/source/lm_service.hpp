
#pragma once
#include "impl/lm_logging.hpp"

namespace ams::lm {

    class Logger : public sf::IServiceObject {

        private:
            enum class CommandId {
                Log            = 0,
                SetDestination = 1,
            };

        private:
            u64 program_id;
            LogDestination destination;
            std::vector<impl::LogPacketBuffer> queued_packets;
        public:
            Logger(u64 program_id) : program_id(program_id), destination(LogDestination::TMA), queued_packets() {}

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
            void AtmosphereGetLastLogInfo(sf::Out<s64> out_log_id, sf::Out<u64> out_program_id);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(OpenLogger),
                MAKE_SERVICE_COMMAND_META(AtmosphereGetLogEvent),
                MAKE_SERVICE_COMMAND_META(AtmosphereGetLastLogInfo),
            };

    };

}