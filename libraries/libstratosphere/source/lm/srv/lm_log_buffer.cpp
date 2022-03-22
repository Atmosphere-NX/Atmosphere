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
#include "lm_log_buffer.hpp"
#include "lm_log_server_proxy.hpp"
#include "lm_sd_card_logger.hpp"
#include "lm_time_util.hpp"
#include "lm_log_packet_parser.hpp"

namespace ams::lm::srv {

    namespace {

        void UpdateUserSystemClock(const u8 *data, size_t size) {
            /* Get the current time. */
            const time::PosixTime current_time = GetCurrentTime();

            /* Get the base time. */
            s64 base_time = current_time.value - os::GetSystemTick().ToTimeSpan().GetSeconds();

            /* Modify the message timestamp. */
            LogPacketParser::ParsePacket(data, size, [](const impl::LogPacketHeader &header, const void *payload, size_t payload_size, void *arg) -> bool {
                /* Check that we're a header message. */
                if (!header.IsHead()) {
                    return true;
                }

                /* Find the timestamp data chunk. */
                return LogPacketParser::ParseDataChunk(payload, payload_size, [](impl::LogDataChunkKey key, const void *chunk, size_t chunk_size, void *arg) -> bool {
                    /* Convert the argument. */
                    const s64 *p_base_time = static_cast<const s64 *>(arg);

                    /* Modify user system clock. */
                    if (key == impl::LogDataChunkKey_UserSystemClock) {
                        /* Get the time from the chunk. */
                        s64 time;
                        AMS_ASSERT(chunk_size == sizeof(time));
                        std::memcpy(std::addressof(time), chunk, chunk_size);

                        /* Add the base time. */
                        time += *p_base_time;

                        /* Update the time in the chunk. */
                        std::memcpy(const_cast<void *>(chunk), std::addressof(time), sizeof(time));
                    }

                    return true;
                }, arg);
            }, std::addressof(base_time));
        }

        bool DefaultFlushFunction(const u8 *data, size_t size) {
            /* Declare persistent clock-updated state storage. */
            AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(bool, s_is_user_system_clock_updated, false);

            /* Update clock. */
            if (!s_is_user_system_clock_updated) {
                UpdateUserSystemClock(data, size);
                s_is_user_system_clock_updated = true;
            }

            /* Send the message. */
            const bool tma_success = LogServerProxy::GetInstance().Send(data, size);
            const bool sd_success  = SdCardLogger::GetInstance().Write(data, size);
            const bool is_success  = tma_success || sd_success;

            /* If we succeeded, wipe the current time. */
            s_is_user_system_clock_updated &= !is_success;

            return is_success;
        }

    }

    LogBuffer &LogBuffer::GetDefaultInstance() {
        AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(u8, s_default_buffers[128_KB * 2]);
        AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(LogBuffer, s_default_log_buffer, s_default_buffers, sizeof(s_default_buffers), DefaultFlushFunction);

        return s_default_log_buffer;
    }

    void LogBuffer::CancelPush() {
        /* Acquire exclusive access to the push buffer. */
        std::scoped_lock lk(m_push_buffer_mutex);

        /* Cancel any pending pushes. */
        if (m_push_ready_wait_count > 0) {
            m_push_canceled = true;
            m_cv_push_ready.Broadcast();
        }
    }

    bool LogBuffer::PushImpl(const void *data, size_t size, bool blocking) {
        /* Check pre-conditions. */
        AMS_ASSERT(size <= m_buffer_size);
        AMS_ASSERT(data != nullptr || size == 0);

        /* Check that we have data to push. */
        if (size == 0) {
            return true;
        }

        /* Wait to be able to push. */
        u8 *dst;
        {
            /* Acquire exclusive access to the push buffer. */
            std::scoped_lock lk(m_push_buffer_mutex);

            /* Wait for enough space to be available. */
            while (size > m_buffer_size - m_push_buffer->m_stored_size) {
                /* Only block if we're allowed to. */
                if (!blocking) {
                    return false;
                }

                /* Wait for push to be ready. */
                {
                    ++m_push_ready_wait_count;
                    m_cv_push_ready.Wait(m_push_buffer_mutex);
                    --m_push_ready_wait_count;
                }

                /* Check if push was canceled. */
                if (m_push_canceled) {
                    if (m_push_ready_wait_count == 0) {
                        m_push_canceled = false;
                    }

                    return false;
                }
            }

            /* Set the destination. */
            dst = m_push_buffer->m_head + m_push_buffer->m_stored_size;

            /* Advance the push buffer. */
            m_push_buffer->m_stored_size += size;
            ++m_push_buffer->m_reference_count;
        }

        /* Copy the data to the push buffer. */
        std::memcpy(dst, data, size);

        /* Close our push buffer reference, and signal that we can flush. */
        {
            /* Acquire exclusive access to the push buffer. */
            std::scoped_lock lk(m_push_buffer_mutex);

            /* If there are no pending pushes, signal that we can flush. */
            if ((--m_push_buffer->m_reference_count) == 0) {
                m_cv_flush_ready.Signal();
            }
        }

        return true;
    }

    bool LogBuffer::FlushImpl(bool blocking) {
        /* Acquire exclusive access to the flush buffer. */
        std::scoped_lock lk(m_flush_buffer_mutex);

        /* If we don't have data to flush, wait for us to have data. */
        if (m_flush_buffer->m_stored_size == 0) {
            /* Acquire exclusive access to the push buffer. */
            std::scoped_lock lk(m_push_buffer_mutex);

            /* Wait for there to be pushed data. */
            while (m_push_buffer->m_stored_size == 0 || m_push_buffer->m_reference_count != 0) {
                /* Only block if we're allowed to. */
                if (!blocking) {
                    return false;
                }

                /* Wait for us to be ready to flush. */
                m_cv_flush_ready.Wait(m_push_buffer_mutex);
            }

            /* Swap the push buffer and the flush buffer pointers. */
            std::swap(m_push_buffer, m_flush_buffer);

            /* Signal that we can push. */
            m_cv_push_ready.Broadcast();
        }

        /* Flush any data. */
        if (!m_flush_function(m_flush_buffer->m_head, m_flush_buffer->m_stored_size)) {
            return false;
        }

        /* Reset the flush buffer. */
        m_flush_buffer->m_stored_size = 0;

        return true;
    }

}
