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

namespace ams::htclow::driver {

    class SocketDiscoveryManager {
        private:
            bool m_driver_closed;
            mem::StandardAllocator *m_allocator;
            void *m_thread_stack;
            os::ThreadType m_discovery_thread;
            s32 m_socket;
        public:
            SocketDiscoveryManager(mem::StandardAllocator *allocator)
                : m_driver_closed(false), m_allocator(allocator), m_thread_stack(allocator->Allocate(os::MemoryPageSize, os::ThreadStackAlignment))
            {
                /* ... */
            }
        private:
            static void ThreadEntry(void *arg) {
                static_cast<SocketDiscoveryManager *>(arg)->ThreadFunc();
            }

            void ThreadFunc();

            Result DoDiscovery();
        public:
            void OnDriverOpen();
            void OnDriverClose();
            void OnSocketAcceptBegin(u16 port);
            void OnSocketAcceptEnd();
    };

}
