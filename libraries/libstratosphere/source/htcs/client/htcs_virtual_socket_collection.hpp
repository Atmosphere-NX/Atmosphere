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

namespace ams::htcs::client {

    struct VirtualSocket;

    class VirtualSocketCollection {
        private:
            void *m_buffer;
            size_t m_buffer_size;
            VirtualSocket *m_socket_list;
            s32 m_list_count;
            s32 m_list_size;
            s32 m_next_id;
            os::SdkMutex m_mutex;
        public:
            static size_t GetWorkingMemorySize(int num_sockets);
        public:
            explicit VirtualSocketCollection();
            ~VirtualSocketCollection();
        public:
            void Init(void *buffer, size_t buffer_size);
            void Clear();

            s32 Socket(s32 &error_code);
            s32 Close(s32 id, s32 &error_code);
            s32 Bind(s32 id, const htcs::SockAddrHtcs *address, s32 &error_code);
            s32 Listen(s32 id, s32 backlog_count, s32 &error_code);
            s32 Accept(s32 id, htcs::SockAddrHtcs *address, s32 &error_code);
            s32 Fcntl(s32 id, s32 command, s32 value, s32 &error_code);

            s32 Shutdown(s32 id, s32 how, s32 &error_code);

            ssize_t Recv(s32 id, void *buffer, size_t buffer_size, s32 flags, s32 &error_code);
            ssize_t Send(s32 id, const void *buffer, size_t buffer_size, s32 flags, s32 &error_code);

            s32 Connect(s32 id, const htcs::SockAddrHtcs *address, s32 &error_code);

            s32 Select(htcs::FdSet *read, htcs::FdSet *write, htcs::FdSet *except, htcs::TimeVal *timeout, s32 &error_code);
        private:
            s32 CreateId();

            s32 Add(sf::SharedPointer<tma::ISocket> socket);

            void Insert(s32 id, sf::SharedPointer<tma::ISocket> socket);
            void SetSize(s32 size);

            s32 Find(s32 id, s32 *error_code = nullptr);
            s32 FindByPrimitive(s32 primitive);

            bool HasAddr(const htcs::SockAddrHtcs *address);

            sf::SharedPointer<tma::ISocket> GetSocket(s32 id, s32 *error_code = nullptr);
            sf::SharedPointer<tma::ISocket> FetchSocket(s32 id, s32 &error_code);
            sf::SharedPointer<tma::ISocket> RealizeSocket(s32 id);

            sf::SharedPointer<tma::ISocket> DoAccept(sf::SharedPointer<tma::ISocket> socket, s32 id, htcs::SockAddrHtcs *address, s32 &error_code);

            s32 GetSockets(s32 * const out_primitives, htcs::FdSet *set, s32 &error_code);
            void SetSockets(htcs::FdSet *set, s32 * const primitives, s32 count);

            s32 CreateSocket(sf::SharedPointer<tma::ISocket> socket, s32 &error_code);
    };

}
