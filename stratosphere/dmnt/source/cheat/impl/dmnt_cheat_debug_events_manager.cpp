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
#include "dmnt_cheat_debug_events_manager.hpp"

/* WORKAROUND: This design prevents a kernel deadlock from occurring on 6.0.0+ */

namespace ams::dmnt::cheat::impl {

    namespace {

        class DebugEventsManager {
            public:
                static constexpr size_t NumCores = 4;
                static constexpr size_t ThreadStackSize = os::MemoryPageSize;
            private:
                std::array<uintptr_t, NumCores> handle_message_queue_buffers;
                std::array<uintptr_t, NumCores> result_message_queue_buffers;
                std::array<os::MessageQueue, NumCores> handle_message_queues;
                std::array<os::MessageQueue, NumCores> result_message_queues;
                std::array<os::ThreadType, NumCores> threads;

                alignas(os::MemoryPageSize) u8 thread_stacks[NumCores][ThreadStackSize];
            private:
                static void PerCoreThreadFunction(void *_this) {
                    /* This thread will wait on the appropriate message queue. */
                    DebugEventsManager *this_ptr = reinterpret_cast<DebugEventsManager *>(_this);
                    const size_t current_core = svcGetCurrentProcessorNumber();
                    while (true) {
                        /* Receive handle. */
                        Handle debug_handle = this_ptr->WaitReceiveHandle(current_core);

                        /* Continue events on the correct core. */
                        Result result = this_ptr->ContinueDebugEvent(debug_handle);

                        /* Return our result. */
                        this_ptr->SendContinueResult(current_core, result);
                    }
                }

                Result GetTargetCore(size_t *out, const svc::DebugEventInfo &dbg_event, Handle debug_handle) {
                    /* If we don't need to continue on a specific core, use the system core. */
                    size_t target_core = NumCores - 1;

                    /* Retrieve correct core for new thread event. */
                    if (dbg_event.type == svc::DebugEvent_CreateThread) {
                        u64 out64 = 0;
                        u32 out32 = 0;

                        R_TRY_CATCH(svcGetDebugThreadParam(&out64, &out32, debug_handle, dbg_event.info.create_thread.thread_id, DebugThreadParam_CurrentCore)) {
                            R_CATCH_RETHROW(svc::ResultProcessTerminated)
                        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                        target_core = out32;
                    }

                    /* Set the target core. */
                    *out = target_core;
                    return ResultSuccess();
                }

                void SendHandle(size_t target_core, Handle debug_handle) {
                    this->handle_message_queues[target_core].Send(static_cast<uintptr_t>(debug_handle));
                }

                Handle WaitReceiveHandle(size_t core_id) {
                    uintptr_t x = 0;
                    this->handle_message_queues[core_id].Receive(&x);
                    return static_cast<Handle>(x);
                }

                Result ContinueDebugEvent(Handle debug_handle) {
                    if (hos::GetVersion() >= hos::Version_3_0_0) {
                        return svcContinueDebugEvent(debug_handle, 5, nullptr, 0);
                    } else {
                        return svcLegacyContinueDebugEvent(debug_handle, 5, 0);
                    }
                }

                void SendContinueResult(size_t target_core, Result result) {
                    this->result_message_queues[target_core].Send(static_cast<uintptr_t>(result.GetValue()));
                }

                Result GetContinueResult(size_t core_id) {
                    uintptr_t x = 0;
                    this->result_message_queues[core_id].Receive(&x);
                    return static_cast<Result>(x);
                }
            public:
                DebugEventsManager()
                    : handle_message_queues{
                        os::MessageQueue(std::addressof(handle_message_queue_buffers[0]), 1),
                        os::MessageQueue(std::addressof(handle_message_queue_buffers[1]), 1),
                        os::MessageQueue(std::addressof(handle_message_queue_buffers[2]), 1),
                        os::MessageQueue(std::addressof(handle_message_queue_buffers[3]), 1)},
                    result_message_queues{
                        os::MessageQueue(std::addressof(result_message_queue_buffers[0]), 1),
                        os::MessageQueue(std::addressof(result_message_queue_buffers[1]), 1),
                        os::MessageQueue(std::addressof(result_message_queue_buffers[2]), 1),
                        os::MessageQueue(std::addressof(result_message_queue_buffers[3]), 1)},
                     thread_stacks{}
                {
                    for (size_t i = 0; i < NumCores; i++) {
                        /* Create thread. */
                        R_ABORT_UNLESS(os::CreateThread(std::addressof(this->threads[i]), PerCoreThreadFunction, this, this->thread_stacks[i], ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(dmnt, MultiCoreEventManager), i));
                        os::SetThreadNamePointer(std::addressof(this->threads[i]), AMS_GET_SYSTEM_THREAD_NAME(dmnt, MultiCoreEventManager));

                        /* Set core mask. */
                        os::SetThreadCoreMask(std::addressof(this->threads[i]), i, (1u << i));

                        /* Start thread. */
                        os::StartThread(std::addressof(this->threads[i]));
                    }
                }

                Result ContinueCheatProcess(Handle cheat_dbg_hnd) {
                    /* Loop getting all debug events. */
                    svc::DebugEventInfo d;
                    size_t target_core = NumCores - 1;
                    while (R_SUCCEEDED(svc::GetDebugEvent(std::addressof(d), cheat_dbg_hnd))) {
                        if (d.type == svc::DebugEvent_CreateThread) {
                            R_TRY(GetTargetCore(std::addressof(target_core), d, cheat_dbg_hnd));
                        }
                    }

                    /* Send handle to correct core, wait for continue to finish. */
                    this->SendHandle(target_core, cheat_dbg_hnd);
                    return this->GetContinueResult(target_core);
                }
        };

        /* Manager global. */
        TYPED_STORAGE(DebugEventsManager) g_events_manager;

    }

    void InitializeDebugEventsManager() {
        new (GetPointer(g_events_manager)) DebugEventsManager;
    }

    Result ContinueCheatProcess(Handle cheat_dbg_hnd) {
        return GetReference(g_events_manager).ContinueCheatProcess(cheat_dbg_hnd);
    }

}
