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
#include "lm_common.hpp"

namespace ams::lm::impl {

    class EventLogTransmitter {
        NON_COPYABLE(EventLogTransmitter);
        NON_MOVEABLE(EventLogTransmitter);
        private:
            detail::LogPacketTransmitter::FlushFunction flush_func;
            u64 log_packet_drop_count;
            os::SdkMutex log_packet_drop_count_lock;
        public:
            EventLogTransmitter(detail::LogPacketTransmitter::FlushFunction flush_func) : flush_func(flush_func), log_packet_drop_count(0), log_packet_drop_count_lock() {}

            bool SendLogSessionBeginPacket(u64 process_id);
            bool SendLogSessionEndPacket(u64 process_id);
            bool SendLogPacketDropCountPacket();

            void IncrementLogPacketDropCount() {
                std::scoped_lock lk(this->log_packet_drop_count_lock);
                this->log_packet_drop_count++;
            }
    };

    EventLogTransmitter *GetEventLogTransmitter();

}