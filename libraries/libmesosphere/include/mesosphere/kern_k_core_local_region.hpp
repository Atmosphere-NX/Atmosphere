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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_current_context.hpp>
#include <mesosphere/kern_k_scheduler.hpp>
#include <mesosphere/kern_k_interrupt_task_manager.hpp>
#include <mesosphere/kern_select_interrupt_manager.hpp>
#include <mesosphere/kern_select_hardware_timer.hpp>
#include <mesosphere/kern_k_memory_manager.hpp>

namespace ams::kern {

    struct KCoreLocalContext {
        KCurrentContext current;
        KScheduler scheduler;
        KInterruptTaskManager interrupt_task_manager;
        KInterruptManager interrupt_manager;
        KHardwareTimer hardware_timer;
        /* Everything after this point is for debugging. */
        /* Retail kernel doesn't even consistently update these fields. */
        u64 num_sw_interrupts;
        u64 num_hw_interrupts;
        std::atomic<u64> num_svc;
        u64 num_process_switches;
        u64 num_thread_switches;
        u64 num_fpu_switches;
        u64 num_scheduler_updates;
        u64 num_invoked_scheduler_updates;
        std::atomic<u64> num_specific_svc[0x80];
        u32 perf_counters[6];
    };
    static_assert(sizeof(KCoreLocalContext) < PageSize);

    struct KCoreLocalPage {
        KCoreLocalContext context;
        u8 padding[PageSize - sizeof(KCoreLocalContext)];
    };
    static_assert(sizeof(KCoreLocalPage) == PageSize);

    struct KCoreLocalRegion {
        KCoreLocalPage current;
        KCoreLocalPage absolute[cpu::NumCores];
    };
    static_assert(sizeof(KCoreLocalRegion) == PageSize * (1 + cpu::NumCores));

}
