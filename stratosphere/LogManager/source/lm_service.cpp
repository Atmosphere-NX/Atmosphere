#include "lm_service.hpp"

namespace ams::lm {

    void Logger::WriteQueuedPackets() {
        impl::WriteLogPackets(this->queued_packets);
    }

    void Logger::Log(const sf::InAutoSelectBuffer &buf) {
        impl::LogPacketBuffer log_packet_buf(this->program_id, buf.GetPointer(), buf.GetSize());

        /* Check if there's a queue already started. */
        const bool has_queued_packets = !this->queued_packets.empty();

        if(log_packet_buf.IsHead() && log_packet_buf.IsTail()) {
            /* Single packet to be logged - ensure the queue is empty, push it alone on the queue and log it. */
            this->queued_packets.clear();
            this->queued_packets.push_back(std::move(log_packet_buf));
            impl::WriteLogPackets(this->queued_packets);
        }
        else if(log_packet_buf.IsHead()) {
            /* This is the initial packet of a queue - ensure the queue is empty and push it. */
            this->queued_packets.clear();
            this->queued_packets.push_back(std::move(log_packet_buf));
        }
        else if(log_packet_buf.IsTail()) {
            /* This is the last packet of the queue - push it and log the queue. */
            this->queued_packets.push_back(std::move(log_packet_buf));
            impl::WriteLogPackets(this->queued_packets);
        }
        else if(has_queued_packets) {
            /* Another packet of the queue - push it. */
            this->queued_packets.push_back(std::move(log_packet_buf));
        }
        else {
            /* Invalid packet - but lm always must succeed on this call. */
        }
    }

    void Logger::SetDestination(LogDestination destination) {
        /* TODO: shall we make use of this value? */
        this->destination = destination;
    }

    void LogService::OpenLogger(const sf::ClientProcessId &client_pid, sf::Out<std::shared_ptr<Logger>> out_logger) {
        u64 program_id = 0;
        /* Apparently lm succeeds on many/all commands, so we will succeed on them too. */
        pminfoGetProgramId(&program_id, static_cast<u64>(client_pid.GetValue()));
        auto logger = std::make_shared<Logger>(program_id);
        out_logger.SetValue(std::move(logger));
    }

    void LogService::AtmosphereGetLogEvent(sf::OutCopyHandle out_event) {
        out_event.SetValue(impl::GetLogEventHandle());
    }

    void LogService::AtmosphereGetLastLogInfo(sf::Out<s64> out_log_id, sf::Out<u64> out_program_id) {
        const auto info = impl::GetLastLogInfo();
        out_log_id.SetValue(info.log_id);
        out_program_id.SetValue(info.program_id);
    }

}