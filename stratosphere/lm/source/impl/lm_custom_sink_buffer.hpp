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

    class CustomSinkBuffer {
        NON_COPYABLE(CustomSinkBuffer);
        NON_MOVEABLE(CustomSinkBuffer);
        public:
            using FlushFunction = bool (*)(u8 *log_data, size_t log_data_size);
        private:
            u8 *log_data_buffer;
            size_t size;
            size_t cur_offset;
            FlushFunction flush_fn;
            bool expects_more_packets;
        public:
            CustomSinkBuffer(u8 *log_data_buf, size_t size, FlushFunction flush_fn) : log_data_buffer(log_data_buf), size(size), cur_offset(0), flush_fn(flush_fn), expects_more_packets(false) {}

            bool HasLogData();
            bool Log(const void *log_data, size_t size);

            bool ExpectsMorePackets() {
                return this->expects_more_packets;
            }

            void SetExpectsMorePackets(bool expects_more_packets) {
                this->expects_more_packets = expects_more_packets;
            }
    };

    CustomSinkBuffer *GetCustomSinkBuffer();
    
    void WriteLogToCustomSink(const detail::LogPacketHeader *log_packet_header, size_t log_packet_size, u64 unk_param);
    size_t ReadLogFromCustomSink(void *out_log_data, size_t data_size, u64 *out_packet_drop_count);

}