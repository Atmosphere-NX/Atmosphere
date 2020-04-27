
#pragma once
#include "impl/lm_log_packet.hpp"

namespace ams::lm {

    static constexpr const char LogsDirectory[] = "sdmc:/atmosphere/debug_logs";

    class Logger : public sf::IServiceObject {

        private:
            enum class CommandId {
                Log = 0,
                SetDestination = 1,
            };

        private:
            u64 program_id;
            LogDestination destination;
            std::vector<impl::LogPacket> queued_packets;
            char log_path[FS_MAX_PATH];

            void WriteAndClearQueuedPackets();
        public:
            Logger(u64 program_id) : program_id(program_id), destination(LogDestination::TMA), queued_packets(), log_path() {
                /* Ensure that the logs directory exists. */
                fs::CreateDirectory(LogsDirectory);
                sprintf(this->log_path, "%s/%016lX.log", LogsDirectory, program_id);
            }

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
                OpenLogger = 0,
            };

        private:
            void OpenLogger(const sf::ClientProcessId &client_pid, sf::Out<std::shared_ptr<Logger>> out_logger);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(OpenLogger),
            };

    };

}