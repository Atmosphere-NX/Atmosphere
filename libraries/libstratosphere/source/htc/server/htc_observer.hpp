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
#include "htc_htcmisc_impl.hpp"

namespace ams::htc::server {

    class Observer {
        private:
            os::SystemEvent m_connect_event;
            os::SystemEvent m_disconnect_event;
            os::Event m_stop_event;
            os::ThreadType m_observer_thread;
            const HtcmiscImpl &m_misc_impl;
            bool m_thread_running;
            bool m_stopped;
            bool m_connected;
            bool m_is_service_available;
            alignas(os::ThreadStackAlignment) u8 m_observer_thread_stack[os::MemoryPageSize];
        public:
            Observer(const HtcmiscImpl &misc_impl);
        private:
            static void ObserverThreadEntry(void *arg) { static_cast<Observer *>(arg)->ObserverThreadBody(); }

            void ObserverThreadBody();
        private:
            Result Start();

            void UpdateEvent();
        public:
            os::SystemEvent *GetConnectEvent() { return std::addressof(m_connect_event); }
            os::SystemEvent *GetDisconnectEvent() { return std::addressof(m_disconnect_event); }
    };

}
