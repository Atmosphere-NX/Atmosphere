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
#include "dmnt2_transport_layer.hpp"
#include "dmnt2_transport_session.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    TransportSession::TransportSession(int fd) : m_socket(fd), m_valid(true) {
        /* Create our thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_receive_thread), ReceiveThreadEntry, this, m_receive_thread_stack, sizeof(m_receive_thread_stack), os::HighestThreadPriority - 1));

        /* Start our thread. */
        os::StartThread(std::addressof(m_receive_thread));

        /* Note that we connected. */
        AMS_DMNT2_GDB_LOG_INFO("Created Session %d\n", m_socket);
    }

    TransportSession::~TransportSession() {
        /* Note that we connected. */
        AMS_DMNT2_GDB_LOG_INFO("Closing Session %d\n", m_socket);

        /* Invalidate our receive buffer. */
        m_receive_buffer.Invalidate();

        /* Shutdown our socket. */
        transport::Shutdown(m_socket);

        /* Wait for our thread. */
        os::WaitThread(std::addressof(m_receive_thread));
        os::DestroyThread(std::addressof(m_receive_thread));

        /* Close our socket. */
        transport::Close(m_socket);
    }

    bool TransportSession::WaitToBeReadable() {
        return m_receive_buffer.WaitToBeReadable();
    }

    bool TransportSession::WaitToBeReadable(TimeSpan timeout) {
        return m_receive_buffer.WaitToBeReadable(timeout);
    }

    util::optional<char> TransportSession::GetChar() {
        /* Wait for us to have data. */
        m_receive_buffer.WaitToBeReadable();

        /* Get our data. */
        char c;
        if (m_receive_buffer.Read(std::addressof(c), sizeof(c)) > 0) {
            return c;
        } else {
            return util::nullopt;
        }
    }

    ssize_t TransportSession::PutChar(char c) {
        /* Send the character. */
        const auto sent = transport::Send(m_socket, std::addressof(c), sizeof(c), 0);
        if (sent < 0) {
            m_valid = false;
        }

        return sent;
    }

    ssize_t TransportSession::PutString(const char *str) {
        /* Repeatedly send until all is sent. */
        const size_t len = std::strlen(str);

        size_t remaining = len;
        while (remaining > 0) {
            const auto sent = transport::Send(m_socket, str, remaining, 0);
            if (sent >= 0) {
                remaining -= sent;
                str += sent;
            } else {
                m_valid = false;
                return sent;
            }
        }

        return len;
    }

    void TransportSession::ReceiveThreadFunction() {
        /* Create temporary buffer. */
        u8 buffer[TransportReceiveBuffer::ReceiveBufferSize];

        /* Loop receiving data. */
        while (true) {
            /* Receive data. */
            const auto res = transport::Recv(m_socket, buffer, sizeof(buffer), 0);
            if (res > 0) {
                /* Write the data to our buffer. */
                m_receive_buffer.WaitToBeWritable();
                m_receive_buffer.Write(buffer, res);
            } else {
                /* Otherwise, if we got an error other than "try again", we're done. */
                if (!transport::IsLastErrorEAgain()) {
                    AMS_DMNT2_GDB_LOG_INFO("Session %d invalid, res=%ld, err=%d\n", m_socket, res, static_cast<int>(transport::GetLastError()));
                    m_valid = false;
                    break;
                }
            }
        }

        /* Invalidate our receive buffer. */
        m_receive_buffer.Invalidate();
    }

}
