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
#include "lm_service.hpp"

namespace ams::lm {

    namespace {

        bool SaveDebugLogs() {
            /* Get whether we should actually save logs. */
            u8 save_debug_logs = 0;
            if (settings::fwdbg::GetSettingsItemValue(&save_debug_logs, sizeof(save_debug_logs), "atmosphere", "logmanager_save_debug_logs") != sizeof(save_debug_logs)) {
                return false;
            }

            return save_debug_logs != 0;
        }

    }

    void Logger::Log(const sf::InAutoSelectBuffer &buf) {
        /* Don't log unless we should do it. */
        if(SaveDebugLogs()) {
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
                /* Invalid packet - but lm must succeed on this call. */
                /* This shouldn't happen at all... */
            }
        }
    }

    void Logger::SetDestination(LogDestination destination) {
        /* TODO: shall we make use of this value? */
        this->destination = destination;
    }

    void LogService::OpenLogger(const sf::ClientProcessId &client_pid, sf::Out<std::shared_ptr<Logger>> out_logger) {
        /* Apparently lm succeeds on many/all commands, so we will succeed on them too. */
        ncm::ProgramId program_id;
        pm::info::GetProgramId(&program_id, client_pid.GetValue());
        
        auto logger = std::make_shared<Logger>(program_id);
        out_logger.SetValue(std::move(logger));
    }

    void LogService::AtmosphereGetLogEvent(sf::OutCopyHandle out_event) {
        out_event.SetValue(impl::GetLogEventHandle());
    }

    void LogService::AtmosphereGetLastLogInfo(sf::Out<s64> out_log_id, sf::Out<ncm::ProgramId> out_program_id) {
        const auto info = impl::GetLastLogInfo();
        out_log_id.SetValue(info.log_id);
        out_program_id.SetValue(info.program_id);
    }

}