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
    Kernel::State           Kernel::s_state = Kernel::State::Invalid;
    KResourceLimit          Kernel::s_system_resource_limit;
    KMemoryManager          Kernel::s_memory_manager;
    KPageTableManager       Kernel::s_page_table_manager;
    KMemoryBlockSlabManager Kernel::s_app_memory_block_manager;
    KMemoryBlockSlabManager Kernel::s_sys_memory_block_manager;
    KBlockInfoManager       Kernel::s_block_info_manager;
    KSupervisorPageTable    Kernel::s_supervisor_page_table;
    KSynchronization        Kernel::s_synchronization;
    KWorkerTaskManager      Kernel::s_worker_task_managers[KWorkerTaskManager::WorkerType_Count];

    namespace {

        /* TODO: C++20 constinit */ std::array<KThread, cpu::NumCores> g_main_threads;
        /* TODO: C++20 constinit */ std::array<KThread, cpu::NumCores> g_idle_threads;
    }

    KThread &Kernel::GetMainThread(s32 core_id) { return g_main_threads[core_id]; }
    KThread &Kernel::GetIdleThread(s32 core_id) { return g_idle_threads[core_id]; }

}
