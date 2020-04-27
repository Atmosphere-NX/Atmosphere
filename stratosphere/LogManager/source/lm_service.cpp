#include "lm_service.hpp"

namespace ams::lm {

    void Logger::WriteAndClearQueuedPackets() {
        impl::WriteLogPackets(this->log_path, this->queued_packets, this->program_id, this->destination);
        this->queued_packets.clear();
    }

    void Logger::Log(const sf::InAutoSelectBuffer &buf) {
        auto packet = impl::ParseLogPacket(buf.GetPointer(), buf.GetSize());
        /* Check if there's a queue already started. */
        const bool has_queued_packets = !this->queued_packets.empty();
        /* Initially always push the packet. */
        this->queued_packets.push_back(packet);
        if(has_queued_packets && packet.header.IsTail()) {
            /* This is the final packet of the queue - write and clear it. */
            this->WriteAndClearQueuedPackets();
        }
        else if(!has_queued_packets && !packet.header.IsHead()) {
            /* No head flag and queue not started, so just log the packet and don't start a queue. */
            this->WriteAndClearQueuedPackets();
        }
        /* Otherwise, the packet is a regular packet of a list, so push it and continue. */
    }

    void Logger::SetDestination(LogDestination destination) {
        this->destination = destination;
    }

    void LogService::OpenLogger(const sf::ClientProcessId &client_pid, sf::Out<std::shared_ptr<Logger>> out_logger) {
        u64 program_id = 0;
        /* Apparently lm succeeds on many/all commands, so we will succeed on them too. */
        pminfoGetProgramId(&program_id, static_cast<u64>(client_pid.GetValue()));
        auto logger = std::make_shared<Logger>(program_id);
        out_logger.SetValue(std::move(logger));
    }

}