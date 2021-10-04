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
#include "lm_log_server_proxy.hpp"

namespace ams::lm::srv {

    namespace {

        constexpr inline const char PortName[]          = "iywys@$LogManager";
        constexpr inline const int HtcsSessionCountMax  = 2;
        constexpr inline const TimeSpan PollingInterval = TimeSpan::FromSeconds(1);

        constinit u8 g_htcs_heap_buffer[2_KB];

        constexpr inline const int InvalidHtcsSocket = -1;

        constexpr ALWAYS_INLINE bool IsValidHtcsSocket(int socket) {
            return socket >= 0;
        }

        static_assert(!IsValidHtcsSocket(InvalidHtcsSocket));

        bool IsHtcEnabled() {
            u8 enable_htc = 0;
            settings::fwdbg::GetSettingsItemValue(&enable_htc, sizeof(enable_htc), "atmosphere", "enable_htc");
            return enable_htc != 0;
        }

    }

    LogServerProxy::LogServerProxy() : m_cv_connected(), m_stop_event(os::EventClearMode_ManualClear), m_connection_mutex(), m_observer_mutex(), m_server_socket(InvalidHtcsSocket), m_client_socket(InvalidHtcsSocket), m_connection_observer(nullptr) {
        /* ... */
    }

    void LogServerProxy::Start() {
        /* Create thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_thread), [](void *_this) { static_cast<LogServerProxy *>(_this)->LoopAuto(); }, this, m_thread_stack, sizeof(m_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(lm, HtcsConnection)));

        /* Set thread name pointer. */
        os::SetThreadNamePointer(std::addressof(m_thread), AMS_GET_SYSTEM_THREAD_NAME(lm, HtcsConnection));

        /* Clear stop event. */
        m_stop_event.Clear();

        /* Start thread. */
        os::StartThread(std::addressof(m_thread));
    }

    void LogServerProxy::Stop() {
        /* Signal to connection thread to stop. */
        m_stop_event.Signal();

        /* Close client socket. */
        if (const int client_socket = m_client_socket; client_socket >= 0) {
            htcs::Close(client_socket);
        }

        /* Close server socket. */
        if (const int server_socket = m_server_socket; server_socket >= 0) {
            htcs::Close(server_socket);
        }

        /* Wait for the connection thread to exit. */
        os::WaitThread(std::addressof(m_thread));
        os::DestroyThread(std::addressof(m_thread));
    }

    bool LogServerProxy::IsConnected() {
        /* Return whether there's a valid client socket. */
        return IsValidHtcsSocket(m_client_socket);
    }

    void LogServerProxy::SetConnectionObserver(ConnectionObserver observer) {
        /* Acquire exclusive access to observer data. */
        std::scoped_lock lk(m_observer_mutex);

        /* Set the observer. */
        m_connection_observer = observer;
    }

    bool LogServerProxy::Send(const u8 *data, size_t size) {
        /* Send as much data as we can, until it's all send. */
        size_t offset = 0;
        while (this->IsConnected() && offset < size) {
            /* Try to send the remaining data. */
            if (const auto result = htcs::Send(m_client_socket, data + offset, size - offset, 0); result >= 0) {
                /* Advance. */
                offset += static_cast<size_t>(result);
            } else {
                /* We failed to send data, shutdown the socket. */
                htcs::Shutdown(m_client_socket, htcs::HTCS_SHUT_RDWR);

                /* Wait a second, before returning to the caller. */
                os::SleepThread(TimeSpan::FromSeconds(1));

                return false;
            }
        }

        /* Return whether we sent all the data. */
        AMS_ASSERT(offset <= size);
        return offset == size;
    }

    void LogServerProxy::LoopAuto() {
        /* If we're not using htcs, there's nothing to do. */
        if (!IsHtcEnabled()) {
            return;
        }

        /* Check that we have enough working memory. */
        const auto working_memory_size = htcs::GetWorkingMemorySize(HtcsSessionCountMax);
        AMS_ABORT_UNLESS(working_memory_size <= sizeof(g_htcs_heap_buffer));

        /* Initialize htcs for the duration that we loop. */
        htcs::InitializeForDisableDisconnectionEmulation(g_htcs_heap_buffer, working_memory_size);
        ON_SCOPE_EXIT { htcs::Finalize(); };

        /* Setup socket address. */
        htcs::SockAddrHtcs server_address;
        server_address.family = htcs::HTCS_AF_HTCS;
        server_address.peer_name = htcs::GetPeerNameAny();
        std::strcpy(server_address.port_name.name, PortName);

        /* Manage htcs connections until a stop is requested. */
        do {
            /* Create a server socket. */
            const auto server_socket = htcs::Socket();
            if (!IsValidHtcsSocket(server_socket)) {
                continue;
            }
            m_server_socket = server_socket;

            /* Ensure we cleanup the server socket when done. */
            ON_SCOPE_EXIT {
                htcs::Close(server_socket);
                m_server_socket = InvalidHtcsSocket;
            };

            /* Bind to the server socket. */
            if (htcs::Bind(server_socket, std::addressof(server_address)) != 0) {
                continue;
            }

            /* Listen on the server socket. */
            if (htcs::Listen(server_socket, 0) != 0) {
                continue;
            }

            /* Loop on clients, until we're asked to stop. */
            while (!m_stop_event.TryWait()) {
                /* Accept a client socket. */
                const auto client_socket = htcs::Accept(server_socket, nullptr);
                if (!IsValidHtcsSocket(client_socket)) {
                    break;
                }
                m_client_socket = client_socket;

                /* Note that we're connected. */
                this->InvokeConnectionObserver(true);
                this->SignalConnection();

                /* Ensure we cleanup the client socket when done. */
                ON_SCOPE_EXIT {
                    htcs::Close(client_socket);
                    m_client_socket = InvalidHtcsSocket;

                    this->InvokeConnectionObserver(false);
                };

                /* Receive data (and do nothing with it), so long as we're connected. */
                u8 v;
                while (htcs::Recv(client_socket, std::addressof(v), sizeof(v), 0) == sizeof(v)) { /* ... */ }
            }
        } while (!m_stop_event.TimedWait(PollingInterval));
    }

    void LogServerProxy::SignalConnection() {
        /* Acquire exclusive access to observer data. */
        std::scoped_lock lk(m_connection_mutex);

        /* Broadcast to our connected cv. */
        m_cv_connected.Broadcast();
    }

    void LogServerProxy::InvokeConnectionObserver(bool connected) {
        /* Acquire exclusive access to observer data. */
        std::scoped_lock lk(m_observer_mutex);

        /* If we have an observer, observe the connection state. */
        if (m_connection_observer) {
            m_connection_observer(connected);
        }
    }

    void StartLogServerProxy() {
        LogServerProxy::GetInstance().Start();
    }

    void StopLogServerProxy() {
        LogServerProxy::GetInstance().Stop();
    }

}
