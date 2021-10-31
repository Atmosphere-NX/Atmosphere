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

namespace ams::dmnt {

    class TransportReceiveBuffer {
        public:
            static constexpr size_t ReceiveBufferSize = 4_KB;
        private:
            u8 m_buffer[ReceiveBufferSize];
            os::Event m_readable_event;
            os::Event m_writable_event;
            os::SdkMutex m_mutex;
            size_t m_readable_size;
            size_t m_offset;
            bool m_valid;
        public:
            TransportReceiveBuffer() : m_readable_event(os::EventClearMode_ManualClear), m_writable_event(os::EventClearMode_ManualClear), m_mutex(), m_readable_size(), m_offset(), m_valid(true) { /* ... */ }

            ALWAYS_INLINE bool IsReadable() const { return m_readable_size != 0; }
            ALWAYS_INLINE bool IsWritable() const { return m_readable_size == 0; }
            ALWAYS_INLINE bool IsValid() const { return m_valid; }

            ssize_t Read(void *dst, size_t size);
            ssize_t Write(const void *src, size_t size);

            bool WaitToBeReadable();
            bool WaitToBeReadable(TimeSpan timeout);
            bool WaitToBeWritable();

            void Invalidate();
    };

}