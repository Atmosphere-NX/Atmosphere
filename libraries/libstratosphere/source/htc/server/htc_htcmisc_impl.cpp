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
#include "htc_htcmisc_impl.hpp"

namespace ams::htc::server {

    namespace {

        alignas(os::ThreadStackAlignment) u8 g_client_thread_stack[os::MemoryPageSize];
        alignas(os::ThreadStackAlignment) u8 g_server_thread_stack[os::MemoryPageSize];

    }

    HtcmiscImpl::HtcmiscImpl(htclow::HtclowManager *htclow_manager)
        : m_htclow_driver(htclow_manager, htclow::ModuleId::Htcmisc),
          m_driver_manager(std::addressof(m_htclow_driver)),
          m_rpc_client(std::addressof(m_htclow_driver), HtcmiscClientChannelId),
          m_rpc_server(std::addressof(m_htclow_driver), HtcmiscServerChannelId),
          m_cancel_event(os::EventClearMode_ManualClear),
          m_cancelled(false),
          m_connection_event(os::EventClearMode_ManualClear),
          m_client_connected(false),
          m_server_connected(false),
          m_connected(false),
          m_connection_mutex()
    {
        /* Create the client thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_client_thread), ClientThreadEntry, this, g_client_thread_stack, sizeof(g_client_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(htc, Htcmisc)));

        /* Set the client thread name. */
        os::SetThreadNamePointer(std::addressof(m_client_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, Htcmisc));

        /* Start the client thread. */
        os::StartThread(std::addressof(m_client_thread));

        /* Create the server thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_server_thread), ServerThreadEntry, this, g_server_thread_stack, sizeof(g_server_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(htc, Htcmisc)));

        /* Set the server thread name. */
        os::SetThreadNamePointer(std::addressof(m_server_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, Htcmisc));

        /* Start the server thread. */
        os::StartThread(std::addressof(m_server_thread));
    }

    HtcmiscImpl::~HtcmiscImpl() {
        /* Cancel ourselves. */
        this->Cancel();

        /* Wait for our threads to be done, and destroy them. */
        os::WaitThread(std::addressof(m_client_thread));
        os::DestroyThread(std::addressof(m_client_thread));
        os::WaitThread(std::addressof(m_server_thread));
        os::DestroyThread(std::addressof(m_server_thread));
    }

    void HtcmiscImpl::Cancel() {
        /* Set ourselves as cancelled. */
        m_cancelled = true;

        /* Signal our cancel event. */
        m_cancel_event.Signal();
    }

    void HtcmiscImpl::ClientThread() {
        AMS_ABORT("HtcmiscImpl::ClientThread");
    }

    void HtcmiscImpl::ServerThread() {
        AMS_ABORT("HtcmiscImpl::ServerThread");
    }

}
