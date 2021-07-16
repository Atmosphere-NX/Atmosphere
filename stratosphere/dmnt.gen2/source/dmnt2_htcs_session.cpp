/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "dmnt2_htcs_session.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    HtcsSession::HtcsSession(int fd) : m_socket(fd), m_valid(true) {
        /* Create our thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_receive_thread), ReceiveThreadEntry, this, m_receive_thread_stack, sizeof(m_receive_thread_stack), os::HighestThreadPriority - 1));

        /* Start our thread. */
        os::StartThread(std::addressof(m_receive_thread));

        /* Note that we connected. */
        AMS_DMNT2_GDB_LOG_INFO("Created Session %d\n", m_socket);
    }

    HtcsSession::~HtcsSession() {
        /* Note that we connected. */
        AMS_DMNT2_GDB_LOG_INFO("Closing Session %d\n", m_socket);

        /* Invalidate our receive buffer. */
        m_receive_buffer.Invalidate();

        /* Shutdown our socket. */
        htcs::Shutdown(m_socket, htcs::HTCS_SHUT_RDWR);

        /* Wait for our thread. */
        os::WaitThread(std::addressof(m_receive_thread));
        os::DestroyThread(std::addressof(m_receive_thread));

        /* Close our socket. */
        htcs::Close(m_socket);
    }

    bool HtcsSession::WaitToBeReadable() {
        return m_receive_buffer.WaitToBeReadable();
    }

    bool HtcsSession::WaitToBeReadable(TimeSpan timeout) {
        return m_receive_buffer.WaitToBeReadable(timeout);
    }

    util::optional<char> HtcsSession::GetChar() {
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

    ssize_t HtcsSession::PutChar(char c) {
        /* Send the character. */
        const auto sent = htcs::Send(m_socket, std::addressof(c), sizeof(c), 0);
        if (sent < 0) {
            m_valid = false;
        }

        return sent;
    }

    ssize_t HtcsSession::PutString(const char *str) {
        /* Repeatedly send until all is sent. */
        const size_t len = std::strlen(str);

        size_t remaining = len;
        while (remaining > 0) {
            const auto sent = htcs::Send(m_socket, str, remaining, 0);
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

    void HtcsSession::ReceiveThreadFunction() {
        /* Create temporary buffer. */
        u8 buffer[HtcsReceiveBuffer::ReceiveBufferSize];

        /* Loop receiving data. */
        while (true) {
            /* Receive data. */
            const auto res = htcs::Recv(m_socket, buffer, sizeof(buffer), 0);
            if (res > 0) {
                /* Write the data to our buffer. */
                m_receive_buffer.WaitToBeWritable();
                m_receive_buffer.Write(buffer, res);
            } else {
                /* Otherwise, if we got an error other than "try again", we're done. */
                if (htcs::GetLastError() != htcs::HTCS_EAGAIN) {
                    AMS_DMNT2_GDB_LOG_INFO("Session %d invalid, res=%ld, err=%d\n", m_socket, res, static_cast<int>(htcs::GetLastError()));
                    m_valid = false;
                    break;
                }
            }
        }

        /* Invalidate our receive buffer. */
        m_receive_buffer.Invalidate();
    }

}
