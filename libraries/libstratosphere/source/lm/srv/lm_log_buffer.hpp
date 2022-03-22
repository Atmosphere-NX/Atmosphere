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

    class LogBuffer {
        NON_COPYABLE(LogBuffer);
        NON_MOVEABLE(LogBuffer);
        private:
            struct BufferInfo {
                u8 *m_head;
                size_t m_stored_size;
                size_t m_reference_count;
            };
        public:
            using FlushFunction = bool (*)(const u8 *data, size_t size);
        private:
            BufferInfo m_buffers[2];
            BufferInfo *m_push_buffer;
            BufferInfo *m_flush_buffer;
            size_t m_buffer_size;
            FlushFunction m_flush_function;
            os::SdkMutex m_push_buffer_mutex;
            os::SdkMutex m_flush_buffer_mutex;
            os::SdkConditionVariable m_cv_push_ready;
            os::SdkConditionVariable m_cv_flush_ready;
            bool m_push_canceled;
            size_t m_push_ready_wait_count;
        public:
            constexpr explicit LogBuffer(void *buffer, size_t buffer_size, FlushFunction f)
                : m_buffers{}, m_push_buffer(m_buffers + 0), m_flush_buffer(m_buffers + 1),
                  m_buffer_size(buffer_size / 2), m_flush_function(f), m_push_buffer_mutex{},
                  m_flush_buffer_mutex{}, m_cv_push_ready{}, m_cv_flush_ready{},
                  m_push_canceled(false), m_push_ready_wait_count(0)
            {
                AMS_ASSERT(buffer != nullptr);
                AMS_ASSERT(buffer_size > 0);
                AMS_ASSERT(f != nullptr);

                m_buffers[0].m_head = static_cast<u8 *>(buffer);
                m_buffers[1].m_head = static_cast<u8 *>(buffer) + (buffer_size / 2);
            }

            static LogBuffer &GetDefaultInstance();

            bool Push(const void *data, size_t size) { return this->PushImpl(data, size, true); }
            bool TryPush(const void *data, size_t size) { return this->PushImpl(data, size, false); }

            void CancelPush();

            bool Flush() { return this->FlushImpl(true); }
            bool TryFlush() { return this->FlushImpl(false); }
        private:
            bool PushImpl(const void *data, size_t size, bool blocking);
            bool FlushImpl(bool blocking);
    };

}
