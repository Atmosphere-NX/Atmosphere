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
#include "htcs_manager.hpp"
#include "htcs_manager_impl.hpp"

namespace ams::htcs::impl {

    HtcsMonitor::HtcsMonitor(mem::StandardAllocator *allocator, htc::server::driver::IDriver *drv, htc::server::rpc::RpcClient *rc, HtcsService *srv)
        : m_allocator(allocator),
          m_driver(drv),
          m_rpc_client(rc),
          m_service(srv),
          m_monitor_thread_stack(m_allocator->Allocate(os::MemoryPageSize, os::ThreadStackAlignment)),
          m_mutex(),
          m_cancel_event(os::EventClearMode_ManualClear),
          m_service_availability_event(os::EventClearMode_ManualClear),
          m_cancelled(false),
          m_is_service_available(false)
    {
        /* ... */
    }

    HtcsMonitor::~HtcsMonitor() {
        /* Free thread stack. */
        m_allocator->Free(m_monitor_thread_stack);
    }

    void HtcsMonitor::Start() {
        /* Create the monitor thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_monitor_thread), ThreadEntry, this, m_monitor_thread_stack, os::MemoryPageSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcsMonitor)));

        /* Set thread name. */
        os::SetThreadNamePointer(std::addressof(m_monitor_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtcsMonitor));

        /* Start the monitor thread. */
        os::StartThread(std::addressof(m_monitor_thread));
    }

    void HtcsMonitor::Cancel() {
        /* Cancel, and signal. */
        m_cancelled = true;
        m_cancel_event.Signal();
    }

    void HtcsMonitor::Wait() {
        /* Wait for the thread. */
        os::WaitThread(std::addressof(m_monitor_thread));
        os::DestroyThread(std::addressof(m_monitor_thread));
    }

    void HtcsMonitor::ThreadBody() {
        /* Loop so long as we're not cancelled. */
        while (!m_cancelled) {
            /* Open the rpc client. */
            m_rpc_client->Open();

            /* Ensure we close, if something goes wrong. */
            auto client_guard = SCOPE_GUARD { m_rpc_client->Close(); };

            /* Wait for the rpc server. */
            if (m_rpc_client->WaitAny(htclow::ChannelState_Connectable, m_cancel_event.GetBase()) != 0) {
                break;
            }

            /* Start the rpc client. */
            const Result start_result = m_rpc_client->Start();
            if (R_FAILED(start_result)) {
                /* DEBUG */
                R_ABORT_UNLESS(start_result);
                continue;
            }

            /* We're available! */
            this->SetServiceAvailability(true);
            client_guard.Cancel();

            /* We're available, so we want to cleanup when we're done. */
            ON_SCOPE_EXIT {
                m_rpc_client->Close();
                m_rpc_client->Cancel();
                m_rpc_client->Wait();
                this->SetServiceAvailability(false);
            };

            /* Wait to become disconnected. */
            if (m_rpc_client->WaitAny(htclow::ChannelState_Disconnected, m_cancel_event.GetBase()) != 0) {
                break;
            }
        }
    }

}
