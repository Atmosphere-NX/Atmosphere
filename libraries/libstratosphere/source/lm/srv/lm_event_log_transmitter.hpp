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
#pragma once
#include <stratosphere.hpp>

namespace ams::lm::srv {

    class EventLogTransmitter {
        NON_COPYABLE(EventLogTransmitter);
        NON_MOVEABLE(EventLogTransmitter);
        public:
            using FlushFunction = bool (*)(const u8 *data, size_t size);
        private:
            FlushFunction m_flush_function;
            size_t m_log_packet_drop_count;
            os::SdkMutex m_log_packet_drop_count_mutex;
        public:
            constexpr explicit EventLogTransmitter(FlushFunction f) : m_flush_function(f), m_log_packet_drop_count(0), m_log_packet_drop_count_mutex() {
                AMS_ASSERT(f != nullptr);
            }

            static EventLogTransmitter &GetDefaultInstance();

            bool PushLogSessionBegin(u64 process_id);
            bool PushLogSessionEnd(u64 process_id);

            bool PushLogPacketDropCountIfExists();

            void IncreaseLogPacketDropCount();
    };

}
