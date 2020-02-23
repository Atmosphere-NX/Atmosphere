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
#include "dmnt_cheat_debug_events_manager.hpp"

/* WORKAROUND: This design prevents a kernel deadlock from occurring on 6.0.0+ */

namespace ams::dmnt::cheat::impl {

    namespace {

        class DebugEventsManager {
            public:
                static constexpr size_t NumCores = 4;
                static constexpr size_t ThreadStackSize = os::MemoryPageSize;
                static constexpr size_t ThreadPriority = 24;
            private:
                std::array<os::MessageQueue, NumCores> message_queues;
                std::array<os::Thread, NumCores> threads;
                os::Event continued_event;

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
                        R_ABORT_UNLESS(this_ptr->ContinueDebugEvent(debug_handle));

                        /* Signal that we've continued. */
                        this_ptr->SignalContinued();
                    }
                }

                size_t GetTargetCore(const svc::DebugEventInfo &dbg_event, Handle debug_handle) {
                    /* If we don't need to continue on a specific core, use the system core. */
                    size_t target_core = NumCores - 1;

                    /* Retrieve correct core for new thread event. */
                    if (dbg_event.type == svc::DebugEvent_AttachThread) {
                        u64 out64 = 0;
                        u32 out32 = 0;
                        R_ABORT_UNLESS(svcGetDebugThreadParam(&out64, &out32, debug_handle, dbg_event.info.attach_thread.thread_id, DebugThreadParam_CurrentCore));
                        target_core = out32;
                    }

                    return target_core;
                }

                void SendHandle(size_t target_core, Handle debug_handle) {
                    this->message_queues[target_core].Send(static_cast<uintptr_t>(debug_handle));
                }

                Handle WaitReceiveHandle(size_t core_id) {
                    uintptr_t x = 0;
                    this->message_queues[core_id].Receive(&x);
                    return static_cast<Handle>(x);
                }

                Result ContinueDebugEvent(Handle debug_handle) {
                    if (hos::GetVersion() >= hos::Version_300) {
                        return svcContinueDebugEvent(debug_handle, 5, nullptr, 0);
                    } else {
                        return svcLegacyContinueDebugEvent(debug_handle, 5, 0);
                    }
                }

                void WaitContinued() {
                    this->continued_event.Wait();
                }

                void SignalContinued() {
                    this->continued_event.Signal();
                }

            public:
                DebugEventsManager() : message_queues{os::MessageQueue(1), os::MessageQueue(1), os::MessageQueue(1), os::MessageQueue(1)}, thread_stacks{} {
                    for (size_t i = 0; i < NumCores; i++) {
                        /* Create thread. */
                        R_ABORT_UNLESS(this->threads[i].Initialize(&DebugEventsManager::PerCoreThreadFunction, reinterpret_cast<void *>(this), this->thread_stacks[i], ThreadStackSize, ThreadPriority, i));

                        /* Set core mask. */
                        R_ABORT_UNLESS(svcSetThreadCoreMask(this->threads[i].GetHandle(), i, (1u << i)));

                        /* Start thread. */
                        R_ABORT_UNLESS(this->threads[i].Start());
                    }
                }

                void ContinueCheatProcess(Handle cheat_dbg_hnd) {
                    /* Loop getting all debug events. */
                    svc::DebugEventInfo d;
                    size_t target_core = NumCores - 1;
                    while (R_SUCCEEDED(svcGetDebugEvent(reinterpret_cast<u8 *>(&d), cheat_dbg_hnd))) {
                        if (d.type == svc::DebugEvent_AttachThread) {
                            target_core = GetTargetCore(d, cheat_dbg_hnd);
                        }
                    }

                    /* Send handle to correct core, wait for continue to finish. */
                    this->SendHandle(target_core, cheat_dbg_hnd);
                    this->WaitContinued();
                }
        };

        /* Manager global. */
        DebugEventsManager g_events_manager;

    }

    void ContinueCheatProcess(Handle cheat_dbg_hnd) {
        g_events_manager.ContinueCheatProcess(cheat_dbg_hnd);
    }

}
