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

    void HtcmiscImpl::WaitTask(u32 task_id) {
        return m_rpc_client.Wait(task_id);
    }

    Result HtcmiscImpl::GetEnvironmentVariable(size_t *out_size, char *dst, size_t dst_size, const char *name, size_t name_size) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client.Begin<rpc::GetEnvironmentVariableTask>(std::addressof(task_id), name, name_size));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        R_TRY(m_rpc_client.End<rpc::GetEnvironmentVariableTask>(task_id, out_size, dst, dst_size));

        return ResultSuccess();
    }

    Result HtcmiscImpl::GetEnvironmentVariableLength(size_t *out_size, const char *name, size_t name_size) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client.Begin<rpc::GetEnvironmentVariableLengthTask>(std::addressof(task_id), name, name_size));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        R_TRY(m_rpc_client.End<rpc::GetEnvironmentVariableLengthTask>(task_id, out_size));

        return ResultSuccess();
    }

    Result HtcmiscImpl::RunOnHostBegin(u32 *out_task_id, os::NativeHandle *out_event, const char *args, size_t args_size) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client.Begin<rpc::RunOnHostTask>(std::addressof(task_id), args, args_size));

        /* Detach the task. */
        *out_task_id = task_id;
        *out_event   = m_rpc_client.DetachReadableHandle(task_id);

        return ResultSuccess();
    }

    Result HtcmiscImpl::RunOnHostEnd(s32 *out_result, u32 task_id) {
        /* Finish the task. */
        s32 res;
        R_TRY(m_rpc_client.End<rpc::RunOnHostTask>(task_id, std::addressof(res)));

        /* Set output. */
        *out_result = res;

        return ResultSuccess();
    }

    void HtcmiscImpl::ClientThread() {
        /* Loop so long as we're not cancelled. */
        while (!m_cancelled) {
            /* Open the rpc client. */
            m_rpc_client.Open();

            /* Ensure we close, if something goes wrong. */
            auto client_guard = SCOPE_GUARD { m_rpc_client.Close(); };

            /* Wait for the rpc client. */
            if (m_rpc_client.WaitAny(htclow::ChannelState_Connectable, m_cancel_event.GetBase()) != 0) {
                break;
            }

            /* Start the rpc client. */
            if (R_FAILED(m_rpc_client.Start())) {
                break;
            }

            /* We're connected! */
            this->SetClientConnectionEvent(true);
            client_guard.Cancel();

            /* We're connected, so we want to cleanup when we're done. */
            ON_SCOPE_EXIT {
                m_rpc_client.Close();
                m_rpc_client.Cancel();
                m_rpc_client.Wait();
                this->SetClientConnectionEvent(false);
            };

            /* Wait to become disconnected. */
            if (m_rpc_client.WaitAny(htclow::ChannelState_Disconnected, m_cancel_event.GetBase()) != 0) {
                break;
            }
        }
    }

    void HtcmiscImpl::ServerThread() {
        /* Loop so long as we're not cancelled. */
        while (!m_cancelled) {
            /* Open the rpc server. */
            m_rpc_server.Open();

            /* Ensure we close, if something goes wrong. */
            auto server_guard = SCOPE_GUARD { m_rpc_server.Close(); };

            /* Wait for the rpc server. */
            if (m_rpc_server.WaitAny(htclow::ChannelState_Connectable, m_cancel_event.GetBase()) != 0) {
                break;
            }

            /* Start the rpc server. */
            if (R_FAILED(m_rpc_server.Start())) {
                break;
            }

            /* We're connected! */
            this->SetServerConnectionEvent(true);
            server_guard.Cancel();

            /* We're connected, so we want to cleanup when we're done. */
            ON_SCOPE_EXIT {
                m_rpc_server.Close();
                m_rpc_server.Cancel();
                m_rpc_server.Wait();
                this->SetServerConnectionEvent(false);
            };

            /* Wait to become disconnected. */
            if (m_rpc_server.WaitAny(htclow::ChannelState_Disconnected, m_cancel_event.GetBase()) != 0) {
                break;
            }
        }
    }

    void HtcmiscImpl::SetClientConnectionEvent(bool en) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_connection_mutex);

        /* Update our state. */
        if (m_client_connected != en) {
            m_client_connected = en;
            this->UpdateConnectionEvent();
        }
    }

    void HtcmiscImpl::SetServerConnectionEvent(bool en) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_connection_mutex);

        /* Update our state. */
        if (m_server_connected != en) {
            m_server_connected = en;
            this->UpdateConnectionEvent();
        }
    }

    void HtcmiscImpl::UpdateConnectionEvent() {
        /* Determine if we're connected. */
        const bool connected = m_client_connected && m_server_connected;

        /* Update our state. */
        if (m_connected != connected) {
            m_connected = connected;
            m_connection_event.Signal();
        }
    }

    os::EventType *HtcmiscImpl::GetConnectionEvent() const {
        return m_connection_event.GetBase();
    }

    bool HtcmiscImpl::IsConnected() const {
        /* Lock ourselves. */
        std::scoped_lock lk(m_connection_mutex);

        return m_connected;
    }

}
