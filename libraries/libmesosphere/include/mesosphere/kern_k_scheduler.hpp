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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_priority_queue.hpp>

namespace ams::kern {

    using KSchedulerPriorityQueue = KPriorityQueue<KThread, cpu::NumCores, ams::svc::LowestThreadPriority, ams::svc::HighestThreadPriority>;
    static_assert(std::is_same<KSchedulerPriorityQueue::AffinityMaskType, KAffinityMask>::value);
    static_assert(KSchedulerPriorityQueue::NumCores    == cpu::NumCores);
    static_assert(KSchedulerPriorityQueue::NumPriority == BITSIZEOF(u64));

    class KScheduler {
        NON_COPYABLE(KScheduler);
        NON_MOVEABLE(KScheduler);
        public:
            struct SchedulingState {
                std::atomic<bool> needs_scheduling;
                bool interrupt_task_thread_runnable;
                bool should_count_idle;
                u64  idle_count;
                KThread *highest_priority_thread;
                void *idle_thread_stack;
            };
        private:
            SchedulingState state;
            bool is_active;
            s32 core_id;
            KThread *prev_thread;
            u64 last_context_switch_time;
            KThread *idle_thread;
        public:
            KScheduler();
            /* TODO: Actually implement KScheduler. This is a placeholder. */
    };

}
