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

namespace ams::tio {

    using SocketAcceptedCallback = void(*)(s32 desc);

    class FileServerHtcsServer {
        private:
            SocketAcceptedCallback m_on_socket_accepted;
            htcs::HtcsPortName m_port_name;
            os::ThreadType m_thread;
            os::SdkMutex m_mutex;
        public:
            constexpr FileServerHtcsServer() : m_on_socket_accepted(nullptr), m_port_name{}, m_thread{}, m_mutex{} { /* ... */ }
        private:
            static void ThreadEntry(void *arg) {
                static_cast<FileServerHtcsServer *>(arg)->ThreadFunc();
            }

            void ThreadFunc();
        public:
            os::SdkMutex &GetMutex() { return m_mutex; }
        public:
            void Initialize(const char *port_name, void *thread_stack, size_t thread_stack_size, SocketAcceptedCallback on_socket_accepted);
            void Start();
            void Wait();

            ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, s32 flags);
    };

}