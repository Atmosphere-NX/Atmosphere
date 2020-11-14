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
#include "../detail/lm_log_packet.hpp"

namespace ams::lm::impl {

    using BLoggerSomeFunction = bool(*)(void*, size_t);

    class BLogger {
        private:
            detail::LogPacketFlushFunction flush_fn;
            u64 packet_drop_count;
            os::SdkMutex packet_drop_count_lock;
        public:
            BLogger(detail::LogPacketFlushFunction flush_fn) : flush_fn(flush_fn), packet_drop_count(0), packet_drop_count_lock() {}

            void IncrementPacketDropCount() {
                std::scoped_lock lk(this->packet_drop_count_lock);
                this->packet_drop_count++;
            }

            bool SendLogSessionBeginPacket(u64 process_id);
            bool SendLogSessionEndPacket(u64 process_id);
            bool SendLogPacketDropCountPacket();
    };

    BLogger *GetBLogger();

}