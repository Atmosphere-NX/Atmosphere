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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_scheduler.hpp>
#include <mesosphere/kern_select_interrupt_manager.hpp>

namespace ams::kern {

    /* NOTE: This header is included after all main headers. */
    consteval bool KScheduler::ValidateAssemblyOffsets() {
        static_assert(AMS_OFFSETOF(KScheduler, m_state.needs_scheduling)        == KSCHEDULER_NEEDS_SCHEDULING);
        static_assert(AMS_OFFSETOF(KScheduler, m_state.interrupt_task_runnable) == KSCHEDULER_INTERRUPT_TASK_RUNNABLE);
        static_assert(AMS_OFFSETOF(KScheduler, m_state.highest_priority_thread) == KSCHEDULER_HIGHEST_PRIORITY_THREAD);
        static_assert(AMS_OFFSETOF(KScheduler, m_state.idle_thread_stack)       == KSCHEDULER_IDLE_THREAD_STACK);
        static_assert(AMS_OFFSETOF(KScheduler, m_state.prev_thread)             == KSCHEDULER_PREVIOUS_THREAD);
        static_assert(AMS_OFFSETOF(KScheduler, m_state.interrupt_task_manager)  == KSCHEDULER_INTERRUPT_TASK_MANAGER);

        return true;
    }
    static_assert(KScheduler::ValidateAssemblyOffsets());

    ALWAYS_INLINE void KScheduler::RescheduleOtherCores(u64 cores_needing_scheduling) {
        if (const u64 core_mask = cores_needing_scheduling & ~(1ul << m_core_id); core_mask != 0) {
            cpu::DataSynchronizationBarrier();
            Kernel::GetInterruptManager().SendInterProcessorInterrupt(KInterruptName_Scheduler, core_mask);
        }
    }

}
