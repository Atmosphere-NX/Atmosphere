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
#include "../impl/lm_log_data_chunk.hpp"
#include "../impl/lm_log_packet_header.hpp"

namespace ams::lm::srv {

    class LogPacketParser {
        public:
            using ParsePacketCallback    = bool (*)(const impl::LogPacketHeader &header, const void *payload, size_t payload_size, void *arg);
            using ParseDataChunkCallback = bool (*)(impl::LogDataChunkKey key, const void *chunk, size_t chunk_size, void *arg);
            using ParseTextLogCallback   = void (*)(const char *txt, size_t size, void *arg);
        public:
            static bool ParsePacket(const void *buffer, size_t buffer_size, ParsePacketCallback callback, void *arg);
            static bool ParseDataChunk(const void *payload, size_t payload_size, ParseDataChunkCallback callback, void *arg);

            static bool FindDataChunk(const void **out, size_t *out_size, impl::LogDataChunkKey key, const void *buffer, size_t buffer_size);

            static size_t ParseModuleName(char *dst, size_t dst_size, const void *buffer, size_t buffer_size);

            static void ParseTextLogWithContext(const void *buffer, size_t buffer_size, ParseTextLogCallback callback, void *arg);
    };

}
