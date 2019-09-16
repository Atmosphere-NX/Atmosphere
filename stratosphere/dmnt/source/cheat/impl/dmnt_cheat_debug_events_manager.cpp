/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace sts::dmnt::cheat::impl {

    namespace {

        class DebugEventsManager {
            public:
                static constexpr size_t NumCores = 4;
            private:
                HosMessageQueue message_queues[NumCores];
                HosThread threads[NumCores];
                HosSignal continued_signal;
            private:
                static void PerCoreThreadFunction(void *_this) {
                    /* This thread will wait on the appropriate message queue. */
                    DebugEventsManager *this_ptr = reinterpret_cast<DebugEventsManager *>(_this);
                    const u32 current_core = svcGetCurrentProcessorNumber();
                    while (true) {
                        /* Receive handle. */
                        Handle debug_handle = this_ptr->WaitReceiveHandle(current_core);

                        /* Continue events on the correct core. */
                        R_ASSERT(this_ptr->ContinueDebugEvent(debug_handle));

                        /* Signal that we've continued. */
                        this_ptr->SignalContinued();
                    }
                }

                u32 GetTargetCore(const svc::DebugEventInfo &dbg_event, Handle debug_handle) {
                    /* If we don't need to continue on a specific core, use the system core. */
                    u32 target_core = NumCores - 1;

                    /* Retrieve correct core for new thread event. */
                    if (dbg_event.type == svc::DebugEventType::AttachThread) {
                        u64 out64 = 0;
                        u32 out32 = 0;
                        R_ASSERT(svcGetDebugThreadParam(&out64, &out32, debug_handle, dbg_event.info.attach_thread.thread_id, DebugThreadParam_CurrentCore));
                        target_core = out32;
                    }

                    return target_core;
                }

                void SendHandle(const svc::DebugEventInfo &dbg_event, Handle debug_handle) {
                    this->message_queues[GetTargetCore(dbg_event, debug_handle)].Send(static_cast<uintptr_t>(debug_handle));
                }

                Handle WaitReceiveHandle(u32 core_id) {
                    uintptr_t x = 0;
                    this->message_queues[core_id].Receive(&x);
                    return static_cast<Handle>(x);
                }

                Result ContinueDebugEvent(Handle debug_handle) {
                    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_300) {
                        return svcContinueDebugEvent(debug_handle, 5, nullptr, 0);
                    } else {
                        return svcLegacyContinueDebugEvent(debug_handle, 5, 0);
                    }
                }

                void WaitContinued() {
                    this->continued_signal.Wait();
                    this->continued_signal.Reset();
                }

                void SignalContinued() {
                    this->continued_signal.Signal();
                }

            public:
                DebugEventsManager() : message_queues{HosMessageQueue(1), HosMessageQueue(1), HosMessageQueue(1), HosMessageQueue(1)} {
                    for (size_t i = 0; i < NumCores; i++) {
                        /* Create thread. */
                        R_ASSERT(this->threads[i].Initialize(&DebugEventsManager::PerCoreThreadFunction, reinterpret_cast<void *>(this), 0x1000, 24, i));

                        /* Set core mask. */
                        R_ASSERT(svcSetThreadCoreMask(this->threads[i].GetHandle(), i, (1u << i)));

                        /* Start thread. */
                        R_ASSERT(this->threads[i].Start());
                    }
                }

                void ContinueCheatProcess(Handle cheat_dbg_hnd) {
                    /* Loop getting all debug events. */
                    svc::DebugEventInfo d;
                    while (R_SUCCEEDED(svcGetDebugEvent(reinterpret_cast<u8 *>(&d), cheat_dbg_hnd))) {
                        /* ... */
                    }

                    /* Send handle to correct core, wait for continue to finish. */
                    this->SendHandle(d, cheat_dbg_hnd);
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
