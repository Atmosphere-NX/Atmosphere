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
#include "dmnt2_transport_receive_buffer.hpp"

namespace ams::dmnt {

    ssize_t TransportReceiveBuffer::Read(void *dst, size_t size) {
        /* Acquire exclusive access to ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're readable and valid. */
        if (!(this->IsValid() && this->IsReadable())) {
            return -1;
        }

        /* Check that we have data to read. */
        const size_t readable = std::min(size, m_readable_size);
        if (readable <= 0) {
            return  -1;
        }

        /* Copy the data. */
        std::memcpy(dst, m_buffer + m_offset, readable);

        /* Advance our pointers. */
        m_readable_size -= readable;
        m_offset += readable;

        /* Handle the case where we're done consuming. */
        if (m_readable_size == 0) {
            m_offset = 0;
            m_readable_event.Clear();
            m_writable_event.Signal();
        }

        return readable;
    }

    ssize_t TransportReceiveBuffer::Write(const void *src, size_t size) {
        /* Acquire exclusive access to ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're readable and valid. */
        if (!(this->IsValid() && this->IsWritable())) {
            return -1;
        }

        /* Copy the data to our buffer. */
        std::memcpy(m_buffer, src, size);

        /* Set our fields. */
        m_readable_size = size;
        m_offset        = 0;
        m_writable_event.Clear();
        m_readable_event.Signal();

        return size;
    }

    bool TransportReceiveBuffer::WaitToBeReadable() {
        /* Check if we're already readable. */
        {
            std::scoped_lock lk(m_mutex);

            if (this->IsReadable()) {
                return true;
            } else if (!this->IsValid()) {
                return false;
            } else {
                m_readable_event.Clear();
            }
        }

        /* Wait for us to be readable. */
        m_readable_event.Wait();

        return this->IsValid();
    }

    bool TransportReceiveBuffer::WaitToBeReadable(TimeSpan timeout) {
        /* Check if we're already readable. */
        {
            std::scoped_lock lk(m_mutex);

            if (this->IsReadable()) {
                return true;
            } else if (!this->IsValid()) {
                return false;
            } else {
                m_readable_event.Clear();
            }
        }

        /* Wait for us to be readable. */
        const bool res = m_readable_event.TimedWait(timeout);

        return res && this->IsValid();
    }

    bool TransportReceiveBuffer::WaitToBeWritable() {
        /* Check if we're already writable. */
        {
            std::scoped_lock lk(m_mutex);

            if (this->IsWritable()) {
                return true;
            } else if (!this->IsValid()) {
                return false;
            } else {
                m_writable_event.Clear();
            }
        }

        /* Wait for us to be writable. */
        m_writable_event.Wait();

        return this->IsValid();
    }

    void TransportReceiveBuffer::Invalidate() {
        /* Acquire exclusive access to ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Set ourselves as invalid. */
        m_valid = false;

        /* Signal our events. */
        m_readable_event.Signal();
        m_writable_event.Signal();
    }

}
