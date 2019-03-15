/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <map>
#include <switch.h>
#include "dmnt_config.hpp"
#include "dmnt_cheat_debug_events_manager.hpp"


/* WORKAROUND: This design prevents a kernel deadlock from occurring on 6.0.0+ */

static HosThread g_per_core_threads[DmntCheatDebugEventsManager::NumCores];
static HosMessageQueue *g_per_core_queues[DmntCheatDebugEventsManager::NumCores];
static HosSignal g_continued_signal;

void DmntCheatDebugEventsManager::PerCoreThreadFunc(void *arg) {
    /* This thread will simply wait on the appropriate message queue. */
    size_t current_core = reinterpret_cast<size_t>(arg);
    while (true) {
        Handle debug_handle = 0;
        /* Get the debug handle. */
        {
            uintptr_t x = 0;
            g_per_core_queues[current_core]->Receive(&x);
            debug_handle = static_cast<Handle>(x);
        }
        
        /* Continue the process, if needed. */
        if (kernelAbove300()) {
            svcContinueDebugEvent(debug_handle, 5, nullptr, 0);
        } else {
            svcLegacyContinueDebugEvent(debug_handle, 5, 0);
        }
        
        g_continued_signal.Signal();
    }
}

void DmntCheatDebugEventsManager::ContinueCheatProcess(Handle cheat_dbg_hnd) {
    /* Loop getting debug events. */
    DebugEventInfo dbg_event;
    while (R_SUCCEEDED(svcGetDebugEvent((u8 *)&dbg_event, cheat_dbg_hnd))) {
        /* ... */
    }
    
    size_t target_core = DmntCheatDebugEventsManager::NumCores - 1;
    /* Retrieve correct core for new thread event. */
    if (dbg_event.type == DebugEventType::AttachThread) {
        u64 out64;
        u32 out32;
        Result rc = svcGetDebugThreadParam(&out64, &out32, cheat_dbg_hnd, dbg_event.info.attach_thread.thread_id, DebugThreadParam_CurrentCore);
        if (R_FAILED(rc)) {
            fatalSimple(rc);
        }
        
        
        target_core = out32;
    }
    
    /* Make appropriate thread continue. */
    g_per_core_queues[target_core]->Send(static_cast<uintptr_t>(cheat_dbg_hnd));
    
    /* Wait. */
    g_continued_signal.Wait();
    g_continued_signal.Reset();
}

void DmntCheatDebugEventsManager::Initialize() {
    /* Spawn per core resources. */
    for (size_t i = 0; i < DmntCheatDebugEventsManager::NumCores; i++) {
        /* Create queue. */
        g_per_core_queues[i] = new HosMessageQueue(1);
        
        /* Create thread. */
        if (R_FAILED(g_per_core_threads[i].Initialize(&DmntCheatDebugEventsManager::PerCoreThreadFunc, reinterpret_cast<void *>(i), 0x1000, 24, i))) {
            std::abort();
        }
        
        /* Set core mask. */
        if (R_FAILED(svcSetThreadCoreMask(g_per_core_threads[i].GetHandle(), i, (1u << i)))) {
            std::abort();
        }
        
        /* Start thread. */
        if (R_FAILED(g_per_core_threads[i].Start())) {
            std::abort();
        }
    }
}