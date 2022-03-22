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

namespace ams::lm::srv {

    class LogServerProxy {
        AMS_SINGLETON_TRAITS(LogServerProxy);
        public:
            using ConnectionObserver = void (*)(bool connected);
        private:
            alignas(os::ThreadStackAlignment) u8 m_thread_stack[4_KB];
            os::ThreadType m_thread;
            os::SdkConditionVariable m_cv_connected;
            os::Event m_stop_event;
            os::SdkMutex m_connection_mutex;
            os::SdkMutex m_observer_mutex;
            std::atomic<int> m_server_socket;
            std::atomic<int> m_client_socket;
            ConnectionObserver m_connection_observer;
        public:
            void Start();
            void Stop();

            bool IsConnected();

            void SetConnectionObserver(ConnectionObserver observer);

            bool Send(const u8 *data, size_t size);
        private:
            void LoopAuto();
            void SignalConnection();
            void InvokeConnectionObserver(bool connected);
    };

    void StartLogServerProxy();
    void StopLogServerProxy();

}
