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

    /* Declare kernel data members in kernel TU. */
    constinit Kernel::State           Kernel::s_state = Kernel::State::Invalid;
    constinit KResourceLimit          Kernel::s_system_resource_limit;
              KMemoryManager          Kernel::s_memory_manager;
    constinit KPageTableManager       Kernel::s_page_table_manager;
    constinit KMemoryBlockSlabManager Kernel::s_app_memory_block_manager;
    constinit KMemoryBlockSlabManager Kernel::s_sys_memory_block_manager;
    constinit KBlockInfoManager       Kernel::s_block_info_manager;
    constinit KSupervisorPageTable    Kernel::s_supervisor_page_table;
    constinit KUnsafeMemory           Kernel::s_unsafe_memory;
    constinit KWorkerTaskManager      Kernel::s_worker_task_managers[KWorkerTaskManager::WorkerType_Count];
    constinit KInterruptManager       Kernel::s_interrupt_manager;
    constinit KScheduler              Kernel::s_schedulers[cpu::NumCores];
    constinit KInterruptTaskManager   Kernel::s_interrupt_task_managers[cpu::NumCores];
    constinit KHardwareTimer          Kernel::s_hardware_timers[cpu::NumCores];

    namespace {

        constinit std::array<KThread, cpu::NumCores> g_main_threads;
        constinit std::array<KThread, cpu::NumCores> g_idle_threads;
    }

    KThread &Kernel::GetMainThread(s32 core_id) { return g_main_threads[core_id]; }
    KThread &Kernel::GetIdleThread(s32 core_id) { return g_idle_threads[core_id]; }

}
