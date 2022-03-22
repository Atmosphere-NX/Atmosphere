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
#include "dmnt2_transport_receive_buffer.hpp"

namespace ams::dmnt {

    class TransportSession {
        private:
            alignas(os::ThreadStackAlignment) u8 m_receive_thread_stack[util::AlignUp(os::MemoryPageSize + TransportReceiveBuffer::ReceiveBufferSize, os::ThreadStackAlignment)];
            TransportReceiveBuffer m_receive_buffer;
            os::ThreadType m_receive_thread;
            int m_socket;
            bool m_valid;
        public:
            TransportSession(int fd);
            ~TransportSession();

            ALWAYS_INLINE bool IsValid() const { return m_valid; }

            bool WaitToBeReadable();
            bool WaitToBeReadable(TimeSpan timeout);

            util::optional<char> GetChar();
            ssize_t PutChar(char c);
            ssize_t PutString(const char *str);

        private:
            static void ReceiveThreadEntry(void *arg) {
                static_cast<TransportSession *>(arg)->ReceiveThreadFunction();
            }

            void ReceiveThreadFunction();
    };

}