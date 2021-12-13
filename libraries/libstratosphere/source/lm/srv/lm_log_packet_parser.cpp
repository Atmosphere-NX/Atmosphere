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
#include <stratosphere.hpp>
#include "lm_log_packet_parser.hpp"

namespace ams::lm::srv {

    namespace {

        const u8 *ParseUleb128(u64 *out, const u8 *cur, const u8 *end) {
            u64 value = 0;
            size_t shift = 0;
            while (cur < end && shift + 7 <= BITSIZEOF(u64)) {
                value |= static_cast<u64>(*cur & 0x7F) << shift;
                if ((*cur & 0x80) == 0) {
                    *out = value;
                    return cur;
                }

                ++cur;
                shift += 7;
            }

            return end;
        }

    }

    bool LogPacketParser::ParsePacket(const void *buffer, size_t buffer_size, ParsePacketCallback callback, void *arg) {
        const u8 *cur  = static_cast<const u8 *>(buffer);
        const u8 *end  = cur + buffer_size;

        while (cur < end) {
            /* Check that we can parse a header. */
            size_t remaining_size = end - cur;
            if (remaining_size < sizeof(impl::LogPacketHeader)) {
                AMS_ASSERT(remaining_size >= sizeof(impl::LogPacketHeader));
                return false;
            }

            /* Get the header. */
            impl::LogPacketHeader header;
            std::memcpy(std::addressof(header), cur, sizeof(header));

            /* Advance past the header. */
            cur += sizeof(header);

            /* Check that we can parse the payload. */
            const auto payload_size = header.GetPayloadSize();
            remaining_size = end - cur;
            if (remaining_size < payload_size) {
                AMS_ASSERT(remaining_size >= payload_size);
                return false;
            }

            /* Invoke the callback. */
            if (!callback(header, cur, payload_size, arg)) {
                return false;
            }

            /* Advance. */
            cur += payload_size;
        }

        /* Check that we parsed all the data. */
        AMS_ASSERT(cur == end);

        return true;
    }

    bool LogPacketParser::ParseDataChunk(const void *payload, size_t payload_size, ParseDataChunkCallback callback, void *arg) {
        const u8 *cur  = static_cast<const u8 *>(payload);
        const u8 *end  = cur + payload_size;

        while (cur < end) {
            /* Get the key. */
            u64 key;
            const auto key_last = ParseUleb128(std::addressof(key), cur, end);
            if (key_last >= end) {
                return false;
            }
            cur = key_last + 1;

            /* Get the size. */
            u64 size;
            const auto size_last = ParseUleb128(std::addressof(size), cur, end);
            if (size_last >= end) {
                return false;
            }
            cur = size_last + 1;

            /* If we're in bounds, invoke the callback. */
            if (cur + size <= end && !callback(static_cast<impl::LogDataChunkKey>(key), cur, size, arg)) {
                return false;
            }

            cur += size;
        }

        return true;
    }

    bool LogPacketParser::FindDataChunk(const void **out, size_t *out_size, impl::LogDataChunkKey key, const void *buffer, size_t buffer_size) {
        /* Create context for iteration. */
        struct FindDataChunkContext {
            const void *chunk;
            size_t chunk_size;
            bool found;
            impl::LogDataChunkKey key;
        } context = { nullptr, 0, false, key };

        /* Find the chunk. */
        LogPacketParser::ParsePacket(buffer, buffer_size, [](const impl::LogPacketHeader &header, const void *payload, size_t payload_size, void *arg) -> bool {
            /* If the header isn't a header packet, continue. */
            if (!header.IsHead()) {
                return true;
            }

            return LogPacketParser::ParseDataChunk(payload, payload_size, [](impl::LogDataChunkKey cur_key, const void *chunk, size_t chunk_size, void *arg) -> bool {
                /* Get the context. */
                auto *context = static_cast<FindDataChunkContext *>(arg);

                /* Check if we found the desired key. */
                if (context->key == cur_key) {
                    context->chunk      = chunk;
                    context->chunk_size = chunk_size;
                    context->found      = true;
                    return false;
                }

                /* Otherwise, continue. */
                return true;
            }, arg);
        }, std::addressof(context));

        /* Write the chunk we found. */
        if (context.found) {
            *out = context.chunk;
            *out_size = context.chunk_size;
            return true;
        } else {
            return false;
        }
    }

    size_t LogPacketParser::ParseModuleName(char *dst, size_t dst_size, const void *buffer, size_t buffer_size) {
        /* Check pre-conditions. */
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size > 0);
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(buffer_size > 0);

        /* Find the relevant data chunk. */
        const void *chunk;
        size_t chunk_size;
        const bool found = LogPacketParser::FindDataChunk(std::addressof(chunk), std::addressof(chunk_size), impl::LogDataChunkKey_ModuleName, buffer, buffer_size);
        if (!found || chunk_size == 0) {
            dst[0] = '\x00';
            return 0;
        }

        /* Copy as much of the module name as we can. */
        const size_t copy_size = std::min(chunk_size, dst_size - 1);
        std::memcpy(dst, chunk, copy_size);
        dst[copy_size] = '\x00';

        return chunk_size;
    }

    void LogPacketParser::ParseTextLogWithContext(const void *buffer, size_t buffer_size, ParseTextLogCallback callback, void *arg) {
        /* Declare context for inter-call storage. */
        struct PreviousPacketContext {
            u64 process_id;
            u64 thread_id;
            size_t carry_size;
            bool ends_with_text_log;
        };
        AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(PreviousPacketContext, s_previous_packet_context);

        /* Get the packet header. */
        auto *header  = static_cast<const impl::LogPacketHeader *>(buffer);
        auto *payload = static_cast<const char *>(buffer) + impl::LogPacketHeaderSize;
        auto payload_size = buffer_size - impl::LogPacketHeaderSize;

        /* Determine if the packet is a continuation. */
        const bool is_continuation = !header->IsHead() && header->GetProcessId() == s_previous_packet_context.process_id && header->GetThreadId() == s_previous_packet_context.thread_id;

        /* Require that the packet be a header or a continuation. */
        if (!header->IsHead() && !is_continuation) {
            return;
        }

        /* If the packet is a continuation, handle the leftover data. */
        if (is_continuation && s_previous_packet_context.carry_size > 0) {
            /* Invoke the callback on what we can. */
            const size_t sendable = std::min(s_previous_packet_context.carry_size, payload_size);
            if (s_previous_packet_context.ends_with_text_log) {
                callback(payload, sendable, arg);
            }

            /* Advance the leftover data. */
            s_previous_packet_context.carry_size -= sendable;
            payload += sendable;
            payload_size -= sendable;
        }

        /* If we've sent the whole payload, we're done. */
        if (payload_size == 0) {
            return;
        }

        /* Parse the payload. */
        size_t carry_size = 0;
        bool ends_with_text_log = false;
        {
            const u8 *cur  = reinterpret_cast<const u8 *>(payload);
            const u8 *end  = cur + payload_size;

            while (cur < end) {
                /* Get the key. */
                u64 key;
                const auto key_last = ParseUleb128(std::addressof(key), cur, end);
                if (key_last >= end) {
                    break;
                }
                cur = key_last + 1;

                /* Get the size. */
                u64 size;
                const auto size_last = ParseUleb128(std::addressof(size), cur, end);
                if (size_last >= end) {
                    break;
                }
                cur = size_last + 1;

                /* Process the data. */
                const bool is_text_log = static_cast<impl::LogDataChunkKey>(key) == impl::LogDataChunkKey_TextLog;
                const size_t remaining = end - cur;

                if (size >= remaining) {
                    carry_size         = size - remaining;
                    ends_with_text_log = is_text_log;
                }

                if (is_text_log) {
                    const size_t sendable_size = std::min(size, remaining);
                    callback(reinterpret_cast<const char *>(cur), sendable_size, arg);
                }

                cur += size;
            }
        }

        /* If the packet isn't a tail packet, update the context. */
        if (!header->IsTail()) {
            s_previous_packet_context.process_id         = header->GetProcessId();
            s_previous_packet_context.thread_id          = header->GetThreadId();
            s_previous_packet_context.carry_size         = carry_size;
            s_previous_packet_context.ends_with_text_log = ends_with_text_log;
        }
    }

}
