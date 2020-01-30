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
#include <mesosphere.hpp>

namespace ams::kern {

    void Kernel::Initialize(s32 core_id) {
        /* Construct the core local region object in place. */
        KCoreLocalContext *clc = GetPointer<KCoreLocalContext>(KMemoryLayout::GetCoreLocalRegionAddress());
        new (clc) KCoreLocalContext;

        /* Set the core local region address into the global register. */
        cpu::SetCoreLocalRegionAddress(reinterpret_cast<uintptr_t>(clc));

        /* Initialize current context. */
        clc->current.current_thread = nullptr;
        clc->current.current_process = nullptr;
        clc->current.scheduler = std::addressof(clc->scheduler);
        clc->current.interrupt_task_manager = std::addressof(clc->interrupt_task_manager);
        clc->current.core_id = core_id;
        clc->current.exception_stack_bottom = GetVoidPointer(KMemoryLayout::GetExceptionStackBottomAddress(core_id));

        /* Clear debugging counters. */
        clc->num_sw_interrupts = 0;
        clc->num_hw_interrupts = 0;
        clc->num_svc = 0;
        clc->num_process_switches = 0;
        clc->num_thread_switches = 0;
        clc->num_fpu_switches = 0;

        for (size_t i = 0; i < util::size(clc->perf_counters); i++) {
            clc->perf_counters[i] = 0;
        }
    }

    void Kernel::InitializeCoreThreads(s32 core_id) {
        /* TODO: This function wants to setup the main thread and the idle thread. */
        /* It also wants to initialize the scheduler/interrupt manager/hardware timer. */
    }

}
