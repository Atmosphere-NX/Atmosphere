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
#include "lm_event_log_transmitter.hpp"
#include "lm_log_buffer.hpp"
#include "../impl/lm_log_packet_header.hpp"
#include "../impl/lm_log_packet_transmitter.hpp"

namespace ams::lm::srv {

    namespace {

        constexpr inline size_t TransmitterBufferAlign              = alignof(impl::LogPacketHeader);
        constexpr inline size_t TransmitterBufferSizeForSessionInfo = impl::LogPacketHeaderSize + 3;
        constexpr inline size_t TransmitterBufferSizeForDropCount   = impl::LogPacketHeaderSize + 10;

        bool DefaultFlushFunction(const u8 *data, size_t size) {
            return LogBuffer::GetDefaultInstance().TryPush(data, size);
        }

    }

    EventLogTransmitter &EventLogTransmitter::GetDefaultInstance() {
        AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(EventLogTransmitter, s_default_event_log_transmitter, DefaultFlushFunction);

        return s_default_event_log_transmitter;
    }

    bool EventLogTransmitter::PushLogSessionBegin(u64 process_id) {
        /* Create a transmitter. */
        alignas(TransmitterBufferAlign) u8 buffer[TransmitterBufferSizeForSessionInfo];
        impl::LogPacketTransmitter transmitter(buffer, sizeof(buffer), m_flush_function, static_cast<u8>(diag::LogSeverity_Info), 0, process_id, true, true);

        /* Push session begin. */
        transmitter.PushLogSessionBegin();

        /* Flush the data. */
        const bool success = transmitter.Flush(true);

        /* Update drop count. */
        if (!success) {
            ++m_log_packet_drop_count;
        }

        return success;
    }

    bool EventLogTransmitter::PushLogSessionEnd(u64 process_id) {
        /* Create a transmitter. */
        alignas(TransmitterBufferAlign) u8 buffer[TransmitterBufferSizeForSessionInfo];
        impl::LogPacketTransmitter transmitter(buffer, sizeof(buffer), m_flush_function, static_cast<u8>(diag::LogSeverity_Info), 0, process_id, true, true);

        /* Push session end. */
        transmitter.PushLogSessionEnd();

        /* Flush the data. */
        const bool success = transmitter.Flush(true);

        /* Update drop count. */
        if (!success) {
            ++m_log_packet_drop_count;
        }

        return success;
    }

    bool EventLogTransmitter::PushLogPacketDropCountIfExists() {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_log_packet_drop_count_mutex);

        /* If we have no dropped packets, nothing to push. */
        if (m_log_packet_drop_count == 0) {
            return true;
        }

        /* Create a transmitter. */
        alignas(TransmitterBufferAlign) u8 buffer[TransmitterBufferSizeForDropCount];
        impl::LogPacketTransmitter transmitter(buffer, sizeof(buffer), m_flush_function, static_cast<u8>(diag::LogSeverity_Info), 0, 0, true, true);

        /* Push log packet drop count. */
        transmitter.PushLogPacketDropCount(m_log_packet_drop_count);

        /* Flush the data. */
        const bool success = transmitter.Flush(true);

        /* Update drop count. */
        if (success) {
            m_log_packet_drop_count = 0;
        } else {
            ++m_log_packet_drop_count;
        }

        return success;
    }

    void EventLogTransmitter::IncreaseLogPacketDropCount() {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_log_packet_drop_count_mutex);

        /* Increase the dropped packet count. */
        ++m_log_packet_drop_count;
    }

}
