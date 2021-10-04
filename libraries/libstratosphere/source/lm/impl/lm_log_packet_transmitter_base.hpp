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
#include "lm_log_packet_header.hpp"
#include "lm_log_data_chunk.hpp"

namespace ams::lm::impl {

    class LogPacketTransmitterBase {
        public:
            using FlushFunction = bool (*)(const u8 *data, size_t size);
        private:
            LogPacketHeader *m_header;
            u8 *m_start;
            u8 *m_end;
            u8 *m_payload;
            u8 *m_current;
            bool m_is_tail;
            FlushFunction m_flush_function;
        protected:
            LogPacketTransmitterBase(void *buffer, size_t buffer_size, FlushFunction flush_func, u8 severity, u8 verbosity, u64 process_id, bool head, bool tail);

            ~LogPacketTransmitterBase() {
                this->Flush(m_is_tail);
            }

            void PushDataChunk(LogDataChunkKey key, const void *data, size_t size) {
                this->PushDataChunkImpl(key, data, size, false);
            }

            void PushDataChunk(LogDataChunkKey key, const char *str, size_t size) {
                this->PushDataChunkImpl(key, str, size, true);
            }
        public:
            bool Flush(bool is_tail);
        private:
            size_t GetRemainSize();
            size_t GetPushableDataSize(size_t uleb_size);
            size_t GetRequiredSizeToPushUleb128(u64 v);

            void PushUleb128(u64 v);
            void PushDataChunkImpl(LogDataChunkKey key, const void *data, size_t size, bool is_text);
            void PushData(const void *data, size_t size);
    };

}
