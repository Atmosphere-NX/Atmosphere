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

namespace ams::scs {

    namespace {

        s32 CreateSocket() {
            while (true) {
                /* Try to create a socket. */
                if (const auto desc = htcs::Socket(); desc >= 0) {
                    return desc;
                }

                /* Wait 100ms before trying again. */
                os::SleepThread(TimeSpan::FromMilliSeconds(100));
            }
        }

        s32 AcceptSocket(s32 listen_socket) {
            htcs::SockAddrHtcs temp;
            return htcs::Accept(listen_socket, std::addressof(temp));
        }

        s32 Bind(s32 socket, const htcs::HtcsPortName &port_name) {
            /* Set up the bind address. */
            htcs::SockAddrHtcs addr;
            addr.family    = htcs::HTCS_AF_HTCS;
            addr.peer_name = htcs::GetPeerNameAny();
            std::strcpy(addr.port_name.name, port_name.name);

            /* Bind. */
            return htcs::Bind(socket, std::addressof(addr));
        }

        htcs::ssize_t Receive(s32 socket, void *buffer, size_t size) {
            u8 *dst = static_cast<u8 *>(buffer);
            size_t received = 0;

            while (received < size) {
                const auto ret = htcs::Recv(socket, dst + received, size - received, 0);
                if (ret <= 0) {
                    return ret;
                }

                received += ret;
            }

            return static_cast<htcs::ssize_t>(received);
        }

        bool ReceiveCommand(CommandHeader *header, void *buffer, size_t buffer_size, s32 socket) {
            /* Receive the header. */
            if (Receive(socket, header, sizeof(*header)) != sizeof(*header)) {
                return false;
            }

            /* Check that the body will fit in the buffer. */
            if (header->body_size >= buffer_size) {
                return false;
            }

            /* Receive the body. */
            if(Receive(socket, buffer, header->body_size) != static_cast<htcs::ssize_t>(header->body_size)) {
                return false;
            }

            return true;
        }

    }

    void ShellServer::Initialize(const char *port_name, void *stack, size_t stack_size, CommandProcessor *command_processor) {
        /* Set our variables. */
        m_command_processor = command_processor;
        std::strcpy(m_port_name.name, port_name);

        /* Create our thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_thread), ThreadEntry, this, stack, stack_size, AMS_GET_SYSTEM_THREAD_PRIORITY(scs, ShellServer)));

        /* Set our thread's name. */
        os::SetThreadNamePointer(std::addressof(m_thread), AMS_GET_SYSTEM_THREAD_NAME(scs, ShellServer));
    }

    void ShellServer::Start() {
        os::StartThread(std::addressof(m_thread));
    }

    void ShellServer::DoShellServer() {
        /* Loop servicing the shell server. */
        while (true) {
            /* Create a socket to listen on. */
            const auto listen_socket = CreateSocket();
            ON_SCOPE_EXIT { htcs::Close(listen_socket); };

            /* Bind to the listen socket. */
            if (Bind(listen_socket, m_port_name) != 0) {
                continue;
            }

            /* Loop processing on our bound socket. */
            while (true) {
                /* Listen on the socket. */
                if (const s32 listen_result = htcs::Listen(listen_socket, 0); listen_result != 0) {
                    /* TODO: logging. */
                    break;
                }

                /* Accept a socket. */
                const s32 socket = AcceptSocket(listen_socket);
                if (socket <= 0) {
                    /* TODO: logging. */
                    continue;
                }

                /* Ensure that the socket is cleaned up when we're done with it. */
                ON_SCOPE_EXIT {
                    UnregisterSocket(socket);
                    htcs::Close(socket);
                };

                /* Loop servicing the socket. */
                while (true) {
                    /* Receive a command header. */
                    CommandHeader header;
                    if (!ReceiveCommand(std::addressof(header), m_buffer, sizeof(m_buffer), socket)) {
                        break;
                    }

                    /* Process the command. */
                    if (!m_command_processor->ProcessCommand(header, m_buffer, socket)) {
                        break;
                    }
                }
            }
        }
    }

}
