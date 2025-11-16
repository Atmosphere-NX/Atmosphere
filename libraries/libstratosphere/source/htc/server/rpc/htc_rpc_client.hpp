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
#include "../driver/htc_i_driver.hpp"
#include "htc_rpc_task_table.hpp"
#include "htc_rpc_task_queue.hpp"
#include "htc_rpc_task_id_free_list.hpp"
#include "../../../htcs/impl/rpc/htcs_rpc_tasks.hpp"

namespace ams::htc::server::rpc {

    template<typename T>
    concept IsRpcTask = std::derived_from<T, Task>;

    struct RpcTaskFunctionTraits {
        public:
            template<typename R, typename C, typename... A>
            static std::tuple<A...> GetSetArgumentsImpl(R(C::*)(A...));
            template<typename R, typename C, typename... A>
            static std::tuple<A...> GetGetResultImpl(R(C::*)(A...) const);
    };

    template<typename T> requires IsRpcTask<T>
    using RpcTaskArgumentsType = decltype(RpcTaskFunctionTraits::GetSetArgumentsImpl(&T::SetArguments));

    template<typename T> requires IsRpcTask<T>
    using RpcTaskResultsType = decltype(RpcTaskFunctionTraits::GetGetResultImpl(&T::GetResult));

    template<typename T, size_t Ix> requires IsRpcTask<T>
    using RpcTaskArgumentType = typename std::tuple_element<Ix, RpcTaskArgumentsType<T>>::type;

    template<typename T, size_t Ix> requires IsRpcTask<T>
    using RpcTaskResultType = typename std::tuple_element<Ix, RpcTaskResultsType<T>>::type;

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
            bool m_is_htcs_task[MaxRpcCount];
            RpcTaskQueue m_task_queue;
            bool m_cancelled;
            bool m_thread_running;
            os::EventType m_receive_buffer_available_events[MaxRpcCount];
            os::EventType m_send_buffer_available_events[MaxRpcCount];
            char m_receive_buffer[BufferSize];
            char m_send_buffer[BufferSize];
        private:
            static void ReceiveThreadEntry(void *arg) { static_cast<void>(static_cast<RpcClient *>(arg)->ReceiveThread()); }
            static void SendThreadEntry(void *arg) { static_cast<void>(static_cast<RpcClient *>(arg)->SendThread()); }

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
        private:
            s32 GetTaskHandle(u32 task_id);
        public:
            void Wait(u32 task_id) {
                os::WaitEvent(m_task_table.Get<Task>(task_id)->GetEvent());
            }

            os::NativeHandle DetachReadableHandle(u32 task_id) {
                return os::DetachReadableHandleOfSystemEvent(m_task_table.Get<Task>(task_id)->GetSystemEvent());
            }

            void CancelBySocket(s32 handle);

            template<typename T, typename... Args> requires (IsRpcTask<T> && sizeof...(Args) == std::tuple_size<RpcTaskArgumentsType<T>>::value)
            Result Begin(u32 *out_task_id, Args &&... args) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Allocate a free task id. */
                u32 task_id{};
                R_TRY(m_task_id_free_list.Allocate(std::addressof(task_id)));

                /* Create the new task. */
                T *task = m_task_table.New<T>(task_id);
                m_task_active[task_id] = true;
                m_is_htcs_task[task_id] = htcs::impl::rpc::IsHtcsTask<T>;

                /* Ensure we clean up the task, if we fail after this. */
                auto task_guard = SCOPE_GUARD {
                    m_task_active[task_id] = false;
                    m_is_htcs_task[task_id] = false;
                    m_task_table.Delete<T>(task_id);
                    m_task_id_free_list.Free(task_id);
                };

                /* Set the task arguments. */
                R_TRY(task->SetArguments(std::forward<Args>(args)...));

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
                R_SUCCEED();
            }

            template<typename T, typename... Args> requires (IsRpcTask<T> && sizeof...(Args) == std::tuple_size<RpcTaskResultsType<T>>::value)
            Result GetResult(u32 task_id, Args &&... args) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get the task. */
                T *task = m_task_table.Get<T>(task_id);
                R_UNLESS(task != nullptr, htc::ResultInvalidTaskId());

                /* Check that the task is completed. */
                R_UNLESS(task->GetTaskState() == RpcTaskState::Completed, htc::ResultTaskNotCompleted());

                /* Get the task's result. */
                R_TRY(task->GetResult(std::forward<Args>(args)...));

                R_SUCCEED();
            }

            template<typename T, typename... Args> requires (IsRpcTask<T> && sizeof...(Args) == std::tuple_size<RpcTaskResultsType<T>>::value)
            Result End(u32 task_id, Args &&... args) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get the task. */
                T *task = m_task_table.Get<T>(task_id);
                R_UNLESS(task != nullptr, htc::ResultInvalidTaskId());

                /* Ensure the task is freed if it needs to be, when we're done. */
                auto task_guard = SCOPE_GUARD {
                    m_task_active[task_id] = false;
                    m_is_htcs_task[task_id] = false;
                    m_task_table.Delete<T>(task_id);
                    m_task_id_free_list.Free(task_id);
                };

                /* If the task was cancelled, handle that. */
                if (task->GetTaskState() == RpcTaskState::Cancelled) {
                    switch (task->GetTaskCancelReason()) {
                        case RpcTaskCancelReason::BySocket:
                            task_guard.Cancel();
                            R_THROW(htc::ResultTaskCancelled());
                        case RpcTaskCancelReason::ClientFinalized:
                            R_THROW(htc::ResultCancelled());
                        case RpcTaskCancelReason::QueueNotAvailable:
                            R_THROW(htc::ResultTaskQueueNotAvailable());
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }

                /* Get the task's result. */
                R_TRY(task->GetResult(std::forward<Args>(args)...));

                R_SUCCEED();
            }

            template<typename T> requires IsRpcTask<T>
            Result VerifyTaskIdWithHandle(u32 task_id, s32 handle) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get the task. */
                T *task = m_task_table.Get<T>(task_id);
                R_UNLESS(task != nullptr, htc::ResultInvalidTaskId());

                /* Check the task handle. */
                R_UNLESS(task->GetHandle() == handle, htc::ResultInvalidTaskId());

                R_SUCCEED();
            }

            template<typename T> requires IsRpcTask<T>
            Result Notify(u32 task_id) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Check that our queue is available. */
                R_UNLESS(m_thread_running, htc::ResultTaskQueueNotAvailable());

                /* Get the task. */
                T *task = m_task_table.Get<T>(task_id);
                R_UNLESS(task != nullptr, htc::ResultInvalidTaskId());

                /* Add notification to our queue. */
                m_task_queue.Add(task_id, PacketCategory::Notification);

                R_SUCCEED();
            }

            template<typename T> requires IsRpcTask<T>
            void WaitNotification(u32 task_id) {
                /* Get the task from the table, releasing our lock afterwards. */
                T *task;
                {
                    /* Lock ourselves. */
                    std::scoped_lock lk(m_mutex);

                    /* Get the task. */
                    task = m_task_table.Get<T>(task_id);
                }

                /* Wait for a notification. */
                task->WaitNotification();
            }

            template<typename T> requires IsRpcTask<T>
            bool IsCancelled(u32 task_id) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get the task. */
                T *task = m_task_table.Get<T>(task_id);

                /* Check the task state. */
                return task != nullptr && task->GetTaskState() == RpcTaskState::Cancelled;
            }

            template<typename T> requires IsRpcTask<T>
            bool IsCompleted(u32 task_id) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get the task. */
                T *task = m_task_table.Get<T>(task_id);

                /* Check the task state. */
                return task != nullptr && task->GetTaskState() == RpcTaskState::Completed;
            }

            template<typename T> requires IsRpcTask<T>
            Result SendContinue(u32 task_id, const void *buffer, s64 buffer_size) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get the task. */
                T *task = m_task_table.Get<T>(task_id);
                R_UNLESS(task != nullptr, htc::ResultInvalidTaskId());

                /* If the task was cancelled, handle that. */
                if (task->GetTaskState() == RpcTaskState::Cancelled) {
                    switch (task->GetTaskCancelReason()) {
                        case RpcTaskCancelReason::QueueNotAvailable:
                            R_THROW(htc::ResultTaskQueueNotAvailable());
                        default:
                            R_THROW(htc::ResultTaskCancelled());
                    }
                }

                /* Set the task's buffer. */
                if (buffer_size > 0) {
                    task->SetBuffer(buffer, buffer_size);
                    os::SignalEvent(std::addressof(m_send_buffer_available_events[task_id]));
                }

                R_SUCCEED();
            }

            template<typename T> requires IsRpcTask<T>
            Result ReceiveContinue(u32 task_id, void *buffer, s64 buffer_size) {
                /* Get the task's buffer, and prepare to receive. */
                const void *result_buffer;
                s64 result_size;
                {
                    /* Lock ourselves. */
                    std::scoped_lock lk(m_mutex);

                    /* Get the task. */
                    T *task = m_task_table.Get<T>(task_id);
                    R_UNLESS(task != nullptr, htc::ResultInvalidTaskId());

                    /* If the task was cancelled, handle that. */
                    if (task->GetTaskState() == RpcTaskState::Cancelled) {
                        switch (task->GetTaskCancelReason()) {
                            case RpcTaskCancelReason::QueueNotAvailable:
                                R_THROW(htc::ResultTaskQueueNotAvailable());
                            default:
                                R_THROW(htc::ResultTaskCancelled());
                        }
                    }

                    /* Get the result size. */
                    result_size = task->GetResultSize();
                    R_SUCCEED_IF(result_size == 0);

                    /* Get the result buffer. */
                    result_buffer = task->GetBuffer();
                }

                /* Wait for the receive buffer to become available. */
                os::WaitEvent(std::addressof(m_receive_buffer_available_events[task_id]));

                /* Check that we weren't cancelled. */
                R_UNLESS(!m_cancelled, htc::ResultCancelled());

                /* Copy the received data. */
                AMS_ASSERT(0 <= result_size && result_size <= buffer_size);
                AMS_UNUSED(buffer_size);

                std::memcpy(buffer, result_buffer, result_size);

                R_SUCCEED();
            }
    };

}
