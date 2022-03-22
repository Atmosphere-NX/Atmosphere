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
#include "lm_log_packet_transmitter_base.hpp"

namespace ams::lm::impl {

    LogPacketTransmitterBase::LogPacketTransmitterBase(void *buffer, size_t buffer_size, FlushFunction flush_func, u8 severity, u8 verbosity, u64 process_id, bool head, bool tail) {
        /* Check pre-conditions. */
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(buffer), alignof(LogPacketHeader)));
        AMS_ASSERT(buffer_size >= LogPacketHeaderSize);
        AMS_ASSERT(flush_func != nullptr);

        /* Construct log packet header. */
        m_header = std::construct_at(static_cast<LogPacketHeader *>(buffer));

        /* Set fields. */
        m_start   = static_cast<u8 *>(buffer);
        m_end     = m_start + buffer_size;
        m_payload = m_start + LogPacketHeaderSize;
        m_current = m_payload;

        m_is_tail = tail;

        m_flush_function = flush_func;

        /* Set header fields. */
        m_header->SetProcessId(process_id);
        m_header->SetThreadId(os::GetThreadId(os::GetCurrentThread()));
        m_header->SetHead(head);
        m_header->SetLittleEndian(util::IsLittleEndian());
        m_header->SetSeverity(severity);
        m_header->SetVerbosity(verbosity);
    }

    bool LogPacketTransmitterBase::Flush(bool is_tail) {
        /* Check if we're already flushed. */
        if (m_current == m_payload) {
            return true;
        }

        /* Flush the data. */
        m_header->SetTail(is_tail);
        m_header->SetPayloadSize(static_cast<u32>(m_current - m_payload));
        const auto result = m_flush_function(m_start, static_cast<size_t>(m_current - m_start));
        m_header->SetHead(false);

        /* Reset. */
        m_current = m_payload;

        return result;
    }

    size_t LogPacketTransmitterBase::GetRemainSize() {
        return static_cast<size_t>(m_end - m_current);
    }

    size_t LogPacketTransmitterBase::GetPushableDataSize(size_t uleb_size) {
        const size_t remain = this->GetRemainSize();
        if (remain < uleb_size + 2) {
            return 0;
        }

        const size_t cmp = remain - uleb_size;
        u64 mask = 0x7F;
        size_t n;
        for (n = 1; mask + n < cmp; ++n) {
            mask |= mask << 7;
        }

        return cmp - n;
    }

    size_t LogPacketTransmitterBase::GetRequiredSizeToPushUleb128(u64 v) {
        /* Determine bytes needed for uleb128 value. */
        size_t required = 0;
        do {
            ++required;
            v >>= 7;
        } while (v > 0);

        return required;
    }

    void LogPacketTransmitterBase::PushUleb128(u64 v) {
        const u32        Mask = 0x7F;
        const u32 InverseMask = ~Mask;
        do {
            /* Check we're within bounds. */
            AMS_ASSERT(m_current < m_end);

            /* Write byte. */
            *(m_current++) = static_cast<u8>(v & Mask) | (((v & InverseMask) != 0) ? 0x80 : 0x00);

            /* Adjust remaining bit range. */
            v >>= 7;
        } while (v > 0);
    }

    void LogPacketTransmitterBase::PushDataChunkImpl(LogDataChunkKey key, const void *data, size_t data_size, bool is_text) {
        /* Check pre-conditions. */
        AMS_ASSERT(data != nullptr);

        /* Push as much data as we can, until the chunk is complete. */
        const u8 *cur = static_cast<const u8 *>(data);
        const u8 * const end = cur + data_size;
        const size_t required_key = this->GetRequiredSizeToPushUleb128(key);
        do {
            /* Get the pushable size. */
            size_t pushable_size = this->GetPushableDataSize(required_key);
            size_t required_size = is_text ? 4 : 1;
            if (pushable_size < required_size) {
                this->Flush(false);
                pushable_size = this->GetPushableDataSize(required_key);
            }
            AMS_ASSERT(pushable_size >= required_size);

            /* Determine the current size. */
            size_t current_size = std::min<size_t>(pushable_size, end - cur);
            if (is_text) {
                const auto valid_size = diag::impl::GetValidSizeAsUtf8String(reinterpret_cast<const char *>(cur), current_size);
                if (valid_size >= 0) {
                    current_size = static_cast<size_t>(valid_size);
                }
            }

            /* Push data. */
            this->PushUleb128(key);
            this->PushUleb128(current_size);
            this->PushData(cur, current_size);

            /* Advance. */
            cur = cur + current_size;
        } while (cur < end);

        /* Check that we pushed all the data. */
        AMS_ASSERT(cur == end);
    }

    void LogPacketTransmitterBase::PushData(const void *data, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(data != nullptr);
        AMS_ASSERT(size <= this->GetRemainSize());

        /* Push the data. */
        if (size > 0) {
            std::memcpy(m_current, data, size);
            m_current += size;
        }
    }

}
