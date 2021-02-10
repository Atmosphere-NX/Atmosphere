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
#include "htc_rpc_client.hpp"

namespace ams::htc::server::rpc {

    namespace {

        constexpr inline size_t ThreadStackSize = os::MemoryPageSize;

        alignas(os::ThreadStackAlignment) constinit u8 g_receive_thread_stack[ThreadStackSize];
        alignas(os::ThreadStackAlignment) constinit u8 g_send_thread_stack[ThreadStackSize];

        constinit os::SdkMutex g_rpc_mutex;

        constinit RpcTaskIdFreeList g_task_id_free_list;
        constinit RpcTaskTable g_task_table;

    }

    RpcClient::RpcClient(driver::IDriver *driver, htclow::ChannelId channel)
        : m_00(0),
          m_driver(driver),
          m_channel_id(channel),
          m_receive_thread_stack(g_receive_thread_stack),
          m_send_thread_stack(g_send_thread_stack),
          m_mutex(g_rpc_mutex),
          m_task_id_free_list(g_task_id_free_list),
          m_task_table(g_task_table),
          m_task_active(),
          m_task_queue(),
          m_cancelled(false),
          m_thread_running(false)
    {
        /* Initialize all events. */
        for (size_t i = 0; i < MaxRpcCount; ++i) {
            os::InitializeEvent(std::addressof(m_receive_buffer_available_events[i]), false, os::EventClearMode_AutoClear);
            os::InitializeEvent(std::addressof(m_send_buffer_available_events[i]), false, os::EventClearMode_AutoClear);
        }
    }

    void RpcClient::Open() {
        R_ABORT_UNLESS(m_driver->Open(m_channel_id));
    }

    void RpcClient::Close() {
        m_driver->Close(m_channel_id);
    }

    Result RpcClient::Start() {
        /* Connect. */
        R_TRY(m_driver->Connect(m_channel_id));

        /* Initialize our task queue. */
        m_task_queue.Initialize();

        /* Create our threads. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_receive_thread), ReceiveThreadEntry, this, m_receive_thread_stack, ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcmiscReceive)));
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_send_thread),    SendThreadEntry,    this, m_send_thread_stack,    ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcmiscSend)));

        /* Set thread name pointers. */
        os::SetThreadNamePointer(std::addressof(m_receive_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtcmiscReceive));
        os::SetThreadNamePointer(std::addressof(m_send_thread),    AMS_GET_SYSTEM_THREAD_NAME(htc, HtcmiscSend));

        /* Start threads. */
        os::StartThread(std::addressof(m_receive_thread));
        os::StartThread(std::addressof(m_send_thread));

        /* Set initial state. */
        m_cancelled      = false;
        m_thread_running = true;

        /* Clear events. */
        for (size_t i = 0; i < MaxRpcCount; ++i) {
            os::ClearEvent(std::addressof(m_receive_buffer_available_events[i]));
            os::ClearEvent(std::addressof(m_send_buffer_available_events[i]));
        }

        return ResultSuccess();
    }

    void RpcClient::Cancel() {
        /* Set cancelled. */
        m_cancelled = true;

        /* Signal all events. */
        for (size_t i = 0; i < MaxRpcCount; ++i) {
            os::SignalEvent(std::addressof(m_receive_buffer_available_events[i]));
            os::SignalEvent(std::addressof(m_send_buffer_available_events[i]));
        }

        /* Cancel our queue. */
        m_task_queue.Cancel();
    }

    void RpcClient::Wait() {
        /* Wait for thread to not be running. */
        if (m_thread_running) {
            os::WaitThread(std::addressof(m_receive_thread));
            os::WaitThread(std::addressof(m_send_thread));
            os::DestroyThread(std::addressof(m_receive_thread));
            os::DestroyThread(std::addressof(m_send_thread));
        }
        m_thread_running = false;

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Finalize the task queue. */
        m_task_queue.Finalize();

        /* Cancel all tasks. */
        for (size_t i = 0; i < MaxRpcCount; ++i) {
            if (m_task_active[i]) {
                /* TODO: enum member */
                m_task_table.Get<Task>(i)->Cancel(static_cast<RpcTaskCancelReason>(2));
            }
        }
    }

    int RpcClient::WaitAny(htclow::ChannelState state, os::EventType *event) {
        /* Check if we're already signaled. */
        if (os::TryWaitEvent(event)) {
            return 1;
        }
        if (m_driver->GetChannelState(m_channel_id) == state) {
            return 0;
        }

        /* Wait. */
        while (true) {
            const auto idx = os::WaitAny(m_driver->GetChannelStateEvent(m_channel_id), event);
            if (idx == 0) {
                if (m_driver->GetChannelState(m_channel_id) == state) {
                    return 0;
                }
            } else {
                return idx;
            }
        }
    }

    void RpcClient::ReceiveThread() {
        AMS_ABORT("RpcClient::ReceiveThread");
    }

    void RpcClient::SendThread() {
        AMS_ABORT("RpcClient::SendThread");
    }

}
