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
#pragma once
#include <stratosphere.hpp>
#include "../driver/htc_i_driver.hpp"
#include "htc_rpc_task_table.hpp"
#include "htc_rpc_task_queue.hpp"
#include "htc_rpc_task_id_free_list.hpp"

namespace ams::htc::server::rpc {

    template<typename T>
    concept IsRpcTask = std::derived_from<T, Task>;

    struct RpcTaskFunctionTraits {
        public:
            template<typename R, typename C, typename... A>
            static std::tuple<A...> GetArgsImpl(R(C::*)(A...));
    };

    template<typename T> requires IsRpcTask<T>
    using RpcTaskArgumentsType = decltype(RpcTaskFunctionTraits::GetArgsImpl(&T::SetArguments));

    template<typename T> requires IsRpcTask<T>
    using RpcTaskResultType = decltype(RpcTaskFunctionTraits::GetArgsImpl(&T::GetResult));

    template<typename T, typename... Args>
    concept IsRpcTaskArgumentsType = IsRpcTask<T> && std::same_as<std::tuple<Args...>, RpcTaskArgumentsType<T>>;

    template<typename T, typename... Args>
    concept IsRpcTaskResultType = IsRpcTask<T> && std::same_as<std::tuple<Args...>, RpcTaskResultType<T>>;

    class RpcClient {
        private:
            /* TODO: where is this value coming from, again? */
            static constexpr size_t BufferSize = 0xE400;
        private:
            mem::StandardAllocator *m_allocator;
            driver::IDriver *m_driver;
            htclow::ChannelId m_channel_id;
            void *m_receive_thread_stack;
            void *m_send_thread_stack;
            os::ThreadType m_receive_thread;
            os::ThreadType m_send_thread;
            os::SdkMutex &m_mutex;
            RpcTaskIdFreeList &m_task_id_free_list;
            RpcTaskTable &m_task_table;
            bool m_task_active[MaxRpcCount];
            RpcTaskQueue m_task_queue;
            bool m_cancelled;
            bool m_thread_running;
            os::EventType m_receive_buffer_available_events[MaxRpcCount];
            os::EventType m_send_buffer_available_events[MaxRpcCount];
            char m_receive_buffer[BufferSize];
            char m_send_buffer[BufferSize];
        private:
            static void ReceiveThreadEntry(void *arg) { static_cast<RpcClient *>(arg)->ReceiveThread(); }
            static void SendThreadEntry(void *arg) { static_cast<RpcClient *>(arg)->SendThread(); }

            Result ReceiveThread();
            Result SendThread();
        public:
            RpcClient(driver::IDriver *driver, htclow::ChannelId channel);
            RpcClient(mem::StandardAllocator *allocator, driver::IDriver *driver, htclow::ChannelId channel);
            ~RpcClient();
        public:
            void Open();
            void Close();

            Result Start();
            void Cancel();
            void Wait();

            int WaitAny(htclow::ChannelState state, os::EventType *event);
        private:
            Result ReceiveHeader(RpcPacket *header);
            Result ReceiveBody(char *dst, size_t size);
            Result SendRequest(const char *src, size_t size);
        public:
            void Wait(u32 task_id) {
                os::WaitEvent(m_task_table.Get<Task>(task_id)->GetEvent());
            }

            template<typename T, typename... Args> requires IsRpcTaskArgumentsType<T, Args...>
            Result Begin(u32 *out_task_id, Args... args) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Allocate a free task id. */
                u32 task_id;
                R_TRY(m_task_id_free_list.Allocate(std::addressof(task_id)));

                /* Create the new task. */
                T *task = m_task_table.New<T>(task_id);
                m_task_active[task_id] = true;

                /* Ensure we clean up the task, if we fail after this. */
                auto task_guard = SCOPE_GUARD {
                    m_task_active[task_id] = false;
                    m_task_table.Delete<T>(task_id);
                    m_task_id_free_list.Free(task_id);
                };

                /* Set the task arguments. */
                R_TRY(task->SetArguments(args...));

                /* Clear the task's events. */
                os::ClearEvent(std::addressof(m_receive_buffer_available_events[task_id]));
                os::ClearEvent(std::addressof(m_send_buffer_available_events[task_id]));

                /* Add the task to our queue if we can, or cancel it. */
                if (m_thread_running) {
                    m_task_queue.Add(task_id, PacketCategory::Request);
                } else {
                    task->Cancel(RpcTaskCancelReason::QueueNotAvailable);
                }

                /* Set the output task id. */
                *out_task_id = task_id;

                /* We succeeded. */
                task_guard.Cancel();
                return ResultSuccess();
            }

            template<typename T, typename... Args> requires IsRpcTaskResultType<T, Args...>
            Result End(u32 task_id, Args... args) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get the task. */
                T *task = m_task_table.Get<T>(task_id);
                R_UNLESS(task != nullptr, htc::ResultInvalidTaskId());

                /* Ensure the task is freed if it needs to be, when we're done. */
                auto task_guard = SCOPE_GUARD {
                    m_task_active[task_id] = false;
                    m_task_table.Delete<T>(task_id);
                    m_task_id_free_list.Free(task_id);
                };

                /* If the task was cancelled, handle that. */
                if (task->GetTaskState() == RpcTaskState::Cancelled) {
                    switch (task->GetTaskCancelReason()) {
                        case RpcTaskCancelReason::One:
                            task_guard.Cancel();
                            return htc::ResultUnknown2021();
                        case RpcTaskCancelReason::Two:
                            return htc::ResultCancelled();
                        case RpcTaskCancelReason::QueueNotAvailable:
                            return htc::ResultTaskQueueNotAvailable();
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }

                /* Get the task's result. */
                R_TRY(task->GetResult(args...));

                return ResultSuccess();
            }
    };

}
