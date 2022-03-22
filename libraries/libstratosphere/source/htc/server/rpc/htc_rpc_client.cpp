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
        : m_allocator(nullptr),
          m_driver(driver),
          m_channel_id(channel),
          m_receive_thread_stack(g_receive_thread_stack),
          m_send_thread_stack(g_send_thread_stack),
          m_mutex(g_rpc_mutex),
          m_task_id_free_list(g_task_id_free_list),
          m_task_table(g_task_table),
          m_task_active(),
          m_is_htcs_task(),
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

    RpcClient::RpcClient(mem::StandardAllocator *allocator, driver::IDriver *driver, htclow::ChannelId channel)
        : m_allocator(allocator),
          m_driver(driver),
          m_channel_id(channel),
          m_receive_thread_stack(m_allocator->Allocate(ThreadStackSize, os::ThreadStackAlignment)),
          m_send_thread_stack(m_allocator->Allocate(ThreadStackSize, os::ThreadStackAlignment)),
          m_mutex(g_rpc_mutex),
          m_task_id_free_list(g_task_id_free_list),
          m_task_table(g_task_table),
          m_task_active(),
          m_is_htcs_task(),
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

    RpcClient::~RpcClient() {
        /* Finalize all events. */
        for (size_t i = 0; i < MaxRpcCount; ++i) {
            os::FinalizeEvent(std::addressof(m_receive_buffer_available_events[i]));
            os::FinalizeEvent(std::addressof(m_send_buffer_available_events[i]));
        }

        /* Free the thread stacks. */
        if (m_allocator != nullptr) {
            m_allocator->Free(m_receive_thread_stack);
            m_allocator->Free(m_send_thread_stack);
        }
        m_receive_thread_stack = nullptr;
        m_send_thread_stack = nullptr;

        /* Free all tasks. */
        for (u32 i = 0; i < MaxRpcCount; ++i) {
            if (m_task_active[i]) {
                std::scoped_lock lk(m_mutex);

                m_task_table.Delete(i);
                m_task_id_free_list.Free(i);
            }
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
                m_task_table.Get<Task>(i)->Cancel(RpcTaskCancelReason::ClientFinalized);
            }
        }
    }

    int RpcClient::WaitAny(htclow::ChannelState state, os::EventType *event) {
        /* Check if we're already signaled. */
        if (os::TryWaitEvent(event)) {
            return 1;
        }

        /* Wait. */
        while (m_driver->GetChannelState(m_channel_id) != state) {
            const auto idx = os::WaitAny(m_driver->GetChannelStateEvent(m_channel_id), event);
            if (idx != 0) {
                return idx;
            }

            /* Clear the channel state event. */
            os::ClearEvent(m_driver->GetChannelStateEvent(m_channel_id));
        }

        return 0;
    }

    Result RpcClient::ReceiveThread() {
        /* Loop forever. */
        auto *header = reinterpret_cast<RpcPacket *>(m_receive_buffer);
        while (true) {
            /* Try to receive a packet header. */
            R_TRY(this->ReceiveHeader(header));

            /* Track how much we've received. */
            size_t received = sizeof(*header);

            /* If the packet has one, receive its body. */
            if (header->body_size > 0) {
                /* Sanity check the task id. */
                AMS_ABORT_UNLESS(header->task_id < static_cast<int>(MaxRpcCount));

                /* Sanity check the body size. */
                AMS_ABORT_UNLESS(util::IsIntValueRepresentable<size_t>(header->body_size));
                AMS_ABORT_UNLESS(static_cast<size_t>(header->body_size) <= sizeof(m_receive_buffer) - received);

                /* Receive the body. */
                R_TRY(this->ReceiveBody(header->data, header->body_size));

                /* Note that we received the body. */
                received += header->body_size;
            }

            /* Acquire exclusive access to the task tables. */
            std::scoped_lock lk(m_mutex);

            /* Get the specified task. */
            Task *task = m_task_table.Get<Task>(header->task_id);
            R_UNLESS(task != nullptr, htc::ResultInvalidTaskId());

            /* If the task is canceled, free it. */
            if (task->GetTaskState() == RpcTaskState::Cancelled) {
                m_task_active[header->task_id] = false;
                m_is_htcs_task[header->task_id] = false;
                m_task_table.Delete(header->task_id);
                m_task_id_free_list.Free(header->task_id);
                continue;
            }

            /* Handle the packet. */
            switch (header->category) {
                case PacketCategory::Response:
                    R_TRY(task->ProcessResponse(m_receive_buffer, received));
                    break;
                case PacketCategory::Notification:
                    R_TRY(task->ProcessNotification(m_receive_buffer, received));
                    break;
                default:
                    return htc::ResultInvalidCategory();
            }

            /* If we used the receive buffer, signal that we're done with it. */
            if (task->IsReceiveBufferRequired()) {
                os::SignalEvent(std::addressof(m_receive_buffer_available_events[header->task_id]));
            }
        }
    }

    Result RpcClient::ReceiveHeader(RpcPacket *header) {
        /* Receive. */
        s64 received;
        R_TRY(m_driver->Receive(std::addressof(received), reinterpret_cast<char *>(header), sizeof(*header), m_channel_id, htclow::ReceiveOption_ReceiveAllData));

        /* Check size. */
        R_UNLESS(static_cast<size_t>(received) == sizeof(*header), htc::ResultInvalidSize());

        return ResultSuccess();
    }

    Result RpcClient::ReceiveBody(char *dst, size_t size) {
        /* Receive. */
        s64 received;
        R_TRY(m_driver->Receive(std::addressof(received), dst, size, m_channel_id, htclow::ReceiveOption_ReceiveAllData));

        /* Check size. */
        R_UNLESS(static_cast<size_t>(received) == size, htc::ResultInvalidSize());

        return ResultSuccess();
    }

    Result RpcClient::SendThread() {
        while (true) {
            /* Get a task. */
            Task *task;
            u32 task_id;
            PacketCategory category;
            do {
                /* Dequeue a task. */
                R_TRY(m_task_queue.Take(std::addressof(task_id), std::addressof(category)));

                /* Get the task from the table. */
                std::scoped_lock lk(m_mutex);

                task = m_task_table.Get<Task>(task_id);
            } while (task == nullptr);

            /* If required, wait for the send buffer to become available. */
            if (task->IsSendBufferRequired()) {
                os::WaitEvent(std::addressof(m_send_buffer_available_events[task_id]));

                /* Check if we've been cancelled. */
                if (m_cancelled) {
                    break;
                }
            }

            /* Handle the task. */
            size_t packet_size;
            switch (category) {
                case PacketCategory::Request:
                    R_TRY(task->CreateRequest(std::addressof(packet_size), m_send_buffer, sizeof(m_send_buffer), task_id));
                    break;
                case PacketCategory::Notification:
                    R_TRY(task->CreateNotification(std::addressof(packet_size), m_send_buffer, sizeof(m_send_buffer), task_id));
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Send the request. */
            R_TRY(this->SendRequest(m_send_buffer, packet_size));
        }

        return htc::ResultCancelled();
    }

    Result RpcClient::SendRequest(const char *src, size_t size) {
        /* Sanity check our size. */
        AMS_ASSERT(util::IsIntValueRepresentable<s64>(size));

        /* Send the data. */
        s64 sent;
        R_TRY(m_driver->Send(std::addressof(sent), src, static_cast<s64>(size), m_channel_id));

        /* Check that we sent the right amount. */
        R_UNLESS(sent == static_cast<s64>(size), htc::ResultInvalidSize());

        return ResultSuccess();
    }

    void RpcClient::CancelBySocket(s32 handle) {
        /* Check if we need to cancel each task. */
        for (size_t i = 0; i < MaxRpcCount; ++i) {
            /* Lock ourselves. */
            std::scoped_lock lk(m_mutex);

            /* Check that the task is active and is an htcs task. */
            if (!m_task_active[i] || !m_is_htcs_task[i]) {
                continue;
            }

            /* Get the htcs task. */
            auto *htcs_task = m_task_table.Get<htcs::impl::rpc::HtcsTask>(i);

            /* Handle the case where the task handle is the one we're cancelling. */
            if (this->GetTaskHandle(i) == handle) {
                /* If the task is complete, free it. */
                if (htcs_task->GetTaskState() == RpcTaskState::Completed) {
                    m_task_active[i] = false;
                    m_is_htcs_task[i] = false;
                    m_task_table.Delete(i);
                    m_task_id_free_list.Free(i);
                } else {
                    /* If the task is a send task, notify. */
                    if (htcs_task->GetTaskType() == htcs::impl::rpc::HtcsTaskType::Send) {
                        m_task_queue.Add(i, PacketCategory::Notification);
                    }

                    /* Cancel the task. */
                    htcs_task->Cancel(RpcTaskCancelReason::BySocket);
                }

                /* The task has been cancelled, so we can move on. */
                continue;
            }

            /* Handle the case where the task is a select task. */
            if (htcs_task->GetTaskType() == htcs::impl::rpc::HtcsTaskType::Select) {
                /* Get the select task. */
                auto *select_task = m_task_table.Get<htcs::impl::rpc::SelectTask>(i);

                /* Get the handle counts. */
                const auto num_read      = select_task->GetReadHandleCount();
                const auto num_write     = select_task->GetWriteHandleCount();
                const auto num_exception = select_task->GetExceptionHandleCount();
                const auto total = num_read + num_write + num_exception;

                /* Get the handle array. */
                const auto *handles = select_task->GetHandles();

                /* Check each handle. */
                for (auto handle_idx = 0; handle_idx < total; ++handle_idx) {
                    if (handles[handle_idx] == handle) {
                        /* If the select is complete, free it. */
                        if (select_task->GetTaskState() == RpcTaskState::Completed) {
                            m_task_active[i] = false;
                            m_is_htcs_task[i] = false;
                            m_task_table.Delete(i);
                            m_task_id_free_list.Free(i);
                        } else {
                            /* Cancel the task. */
                            select_task->Cancel(RpcTaskCancelReason::BySocket);
                        }
                    }
                }
            }
        }
    }

    s32 RpcClient::GetTaskHandle(u32 task_id) {
        /* TODO: Why is this necessary to avoid a bogus array-bounds warning? */
        AMS_ASSUME(task_id < MaxRpcCount);

        /* Check pre-conditions. */
        AMS_ASSERT(m_task_active[task_id]);
        AMS_ASSERT(m_is_htcs_task[task_id]);

        /* Get the htcs task. */
        auto *task = m_task_table.Get<htcs::impl::rpc::HtcsTask>(task_id);

        /* Check that the task has a handle. */
        if (!m_task_active[task_id] || !m_is_htcs_task[task_id] || task == nullptr) {
            return -1;
        }

        /* Get the task's type. */
        const auto type = task->GetTaskType();

        /* Check that the task is new enough. */
        if (task->GetVersion() == 3) {
            if (type == htcs::impl::rpc::HtcsTaskType::Receive || type == htcs::impl::rpc::HtcsTaskType::Send) {
                return -1;
            }
        }

        /* Get the handle from the task. */
        switch (type) {
            case htcs::impl::rpc::HtcsTaskType::Receive:
                return static_cast<htcs::impl::rpc::ReceiveTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::Send:
                return static_cast<htcs::impl::rpc::SendTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::Shutdown:
                return static_cast<htcs::impl::rpc::ShutdownTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::Close:
                return -1;
            case htcs::impl::rpc::HtcsTaskType::Connect:
                return static_cast<htcs::impl::rpc::ConnectTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::Listen:
                return static_cast<htcs::impl::rpc::ListenTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::Accept:
                return static_cast<htcs::impl::rpc::AcceptTask *>(task)->GetServerHandle();
            case htcs::impl::rpc::HtcsTaskType::Socket:
                return -1;
            case htcs::impl::rpc::HtcsTaskType::Bind:
                return static_cast<htcs::impl::rpc::BindTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::Fcntl:
                return static_cast<htcs::impl::rpc::FcntlTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::ReceiveSmall:
                return static_cast<htcs::impl::rpc::ReceiveSmallTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::SendSmall:
                return static_cast<htcs::impl::rpc::SendSmallTask *>(task)->GetHandle();
            case htcs::impl::rpc::HtcsTaskType::Select:
                return -1;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
