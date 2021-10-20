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
#include <mesosphere.hpp>

namespace ams::kern {

    /* Declare kernel data members in kernel TU. */
    constinit Kernel::State           Kernel::s_state = Kernel::State::Invalid;
    constinit KResourceLimit          Kernel::s_system_resource_limit{util::ConstantInitialize};
              KMemoryManager          Kernel::s_memory_manager;
    constinit KSupervisorPageTable    Kernel::s_supervisor_page_table;
    constinit KUnsafeMemory           Kernel::s_unsafe_memory;
    constinit KWorkerTaskManager      Kernel::s_worker_task_managers[KWorkerTaskManager::WorkerType_Count];
    constinit KInterruptManager       Kernel::s_interrupt_manager;
    constinit KScheduler              Kernel::s_schedulers[cpu::NumCores];
    constinit KInterruptTaskManager   Kernel::s_interrupt_task_managers[cpu::NumCores];
    constinit KHardwareTimer          Kernel::s_hardware_timers[cpu::NumCores];

    constinit KPageTableSlabHeap      Kernel::s_page_table_heap;
    constinit KMemoryBlockSlabHeap    Kernel::s_app_memory_block_heap;
    constinit KMemoryBlockSlabHeap    Kernel::s_sys_memory_block_heap;
    constinit KBlockInfoSlabHeap      Kernel::s_block_info_heap;
    constinit KPageTableManager       Kernel::s_app_page_table_manager{util::ConstantInitialize};
    constinit KPageTableManager       Kernel::s_sys_page_table_manager{util::ConstantInitialize};
    constinit KMemoryBlockSlabManager Kernel::s_app_memory_block_manager;
    constinit KMemoryBlockSlabManager Kernel::s_sys_memory_block_manager;
    constinit KBlockInfoManager       Kernel::s_app_block_info_manager;
    constinit KBlockInfoManager       Kernel::s_sys_block_info_manager;

    namespace {

        template<size_t N> requires (N > 0)
        union KThreadArray {
            struct RecursiveHolder {
                KThread m_thread;
                KThreadArray<N - 1> m_next;

                consteval RecursiveHolder() : m_thread{util::ConstantInitialize}, m_next() { /* ... */ }
            } m_holder;
            KThread m_arr[N];

            consteval KThreadArray() : m_holder() { /* ... */ }
        };

        template<>
        union KThreadArray<1>{
            struct RecursiveHolder {
                KThread m_thread;

                consteval RecursiveHolder() : m_thread{util::ConstantInitialize} { /* ... */ }
            } m_holder;
            KThread m_arr[1];

            consteval KThreadArray() : m_holder() { /* ... */ }
        };

        template<size_t Ix>
        consteval bool IsKThreadArrayValid(const KThreadArray<Ix> &v, const KThread *thread) {
            if (std::addressof(v.m_holder.m_thread) != thread) {
                return false;
            }

            if constexpr (Ix == 1) {
                return true;
            } else {
                return IsKThreadArrayValid(v.m_holder.m_next, thread + 1);
            }
        }

        template<size_t N>
        consteval bool IsKThreadArrayValid() {
            const KThreadArray<N> v{};

            if (!IsKThreadArrayValid(v, v.m_arr)) {
                return false;
            }

            if constexpr (N == 1) {
                return true;
            } else {
                return IsKThreadArrayValid<N - 1>();
            }
        }

        static_assert(IsKThreadArrayValid<cpu::NumCores>());

        constinit KThreadArray<cpu::NumCores> g_main_threads;
        constinit KThreadArray<cpu::NumCores> g_idle_threads;

        static_assert(sizeof(g_main_threads) == cpu::NumCores * sizeof(KThread));
        static_assert(sizeof(g_main_threads.m_holder) == sizeof(g_main_threads.m_arr));

    }

    KThread &Kernel::GetMainThread(s32 core_id) { return g_main_threads.m_arr[core_id]; }
    KThread &Kernel::GetIdleThread(s32 core_id) { return g_idle_threads.m_arr[core_id]; }

}
