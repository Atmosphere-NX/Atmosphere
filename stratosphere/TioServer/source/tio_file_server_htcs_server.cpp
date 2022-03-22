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
#include "tio_file_server_htcs_server.hpp"

namespace ams::tio {

    void FileServerHtcsServer::Initialize(const char *port_name, void *thread_stack, size_t thread_stack_size, SocketAcceptedCallback on_socket_accepted) {
        /* Set our port name. */
        std::strcpy(m_port_name.name, port_name);

        /* Set our callback. */
        m_on_socket_accepted = on_socket_accepted;

        /* Setup our thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_thread), ThreadEntry, this, thread_stack, thread_stack_size, AMS_GET_SYSTEM_THREAD_PRIORITY(TioServer, FileServerHtcsServer)));

        /* Set our thread name pointer. */
        os::SetThreadNamePointer(std::addressof(m_thread), AMS_GET_SYSTEM_THREAD_NAME(TioServer, FileServerHtcsServer));
    }

    void FileServerHtcsServer::Start() {
        os::StartThread(std::addressof(m_thread));
    }
    void FileServerHtcsServer::Wait() {
        os::WaitThread(std::addressof(m_thread));
    }

    void FileServerHtcsServer::ThreadFunc() {
        /* Loop forever, servicing sockets. */
        while (true) {
            /* Get a socket. */
            int fd;
            while ((fd = htcs::Socket()) == -1) {
                os::SleepThread(TimeSpan::FromSeconds(1));
            }

            /* Ensure we cleanup the socket when we're done with it. */
            ON_SCOPE_EXIT {
                htcs::Close(fd);
                os::SleepThread(TimeSpan::FromSeconds(1));
            };

            /* Create a sock addr for our server. */
            htcs::SockAddrHtcs addr;
            addr.family    = htcs::HTCS_AF_HTCS;
            addr.peer_name = htcs::GetPeerNameAny();
            addr.port_name = m_port_name;

            /* Bind. */
            if (htcs::Bind(fd, std::addressof(addr)) == -1) {
                continue;
            }

            /* Listen on our port. */
            while (htcs::Listen(fd, 0) == 0) {
                /* Continue accepting clients, so long as we can. */
                int client_fd;
                while (true) {
                    /* Try to accept a client. */
                    if (client_fd = htcs::Accept(fd, std::addressof(addr)); client_fd < 0) {
                        break;
                    }

                    /* Handle the client. */
                    m_on_socket_accepted(client_fd);
                }

                /* NOTE: This seems unnecessary (client_fd guaranteed < 0 here), but Nintendo does it. */
                htcs::Close(client_fd);
            }
        }
    }

    ssize_t FileServerHtcsServer::Send(s32 desc, const void *buffer, size_t buffer_size, s32 flags) {
        AMS_ASSERT(m_mutex.IsLockedByCurrentThread());

        return htcs::Send(desc, buffer, buffer_size, flags);
    }

}