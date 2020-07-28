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
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>
#include <mesosphere/kern_k_memory_manager.hpp>
#include <mesosphere/kern_k_core_local_region.hpp>
#include <mesosphere/kern_k_worker_task_manager.hpp>

namespace ams::kern {

    class KThread;
    class KHardwareTimer;
    class KResourceLimit;
    class KInterruptManager;
    class KInterruptTaskManager;
    class KScheduler;
    class KMemoryManager;
    class KPageTableManager;
    class KMemoryBlockSlabManager;
    class KBlockInfoManager;
    class KSynchronization;
    class KUnsafeMemory;



#if defined(ATMOSPHERE_ARCH_ARM64)

    namespace arch::arm64 {
        class KSupervisorPageTable;
    }
    using ams::kern::arch::arm64::KSupervisorPageTable;

#else

    #error "Unknown architecture for KSupervisorPageTable forward declare"

#endif

    class Kernel {
        public:
            enum class State : u8 {
                Invalid = 0,
                Initializing = 1,
                Initialized  = 2,
            };

            static constexpr size_t ApplicationMemoryBlockSlabHeapSize = 20000;
            static constexpr size_t SystemMemoryBlockSlabHeapSize      = 10000;
            static constexpr size_t BlockInfoSlabHeapSize              = 4000;
        private:
            static State s_state;
            static KResourceLimit s_system_resource_limit;
            static KMemoryManager s_memory_manager;
            static KPageTableManager s_page_table_manager;
            static KMemoryBlockSlabManager s_app_memory_block_manager;
            static KMemoryBlockSlabManager s_sys_memory_block_manager;
            static KBlockInfoManager s_block_info_manager;
            static KSupervisorPageTable s_supervisor_page_table;
            static KSynchronization s_synchronization;
            static KUnsafeMemory s_unsafe_memory;
            static KWorkerTaskManager s_worker_task_managers[KWorkerTaskManager::WorkerType_Count];
        private:
            static ALWAYS_INLINE KCoreLocalContext &GetCoreLocalContext() {
                return reinterpret_cast<KCoreLocalRegion *>(cpu::GetCoreLocalRegionAddress())->current.context;
            }
            static ALWAYS_INLINE KCoreLocalContext &GetCoreLocalContext(s32 core_id) {
                return reinterpret_cast<KCoreLocalRegion *>(cpu::GetCoreLocalRegionAddress())->absolute[core_id].context;
            }
        public:
            static NOINLINE void InitializeCoreLocalRegion(s32 core_id);
            static NOINLINE void InitializeMainAndIdleThreads(s32 core_id);
            static NOINLINE void InitializeResourceManagers(KVirtualAddress address, size_t size);
            static NOINLINE void PrintLayout();

            static ALWAYS_INLINE State GetState() { return s_state; }
            static ALWAYS_INLINE void SetState(State state) { s_state = state; }

            static KThread &GetMainThread(s32 core_id);
            static KThread &GetIdleThread(s32 core_id);

            static ALWAYS_INLINE KCurrentContext &GetCurrentContext(s32 core_id) {
                return GetCoreLocalContext(core_id).current;
            }

            static ALWAYS_INLINE KScheduler &GetScheduler() {
                return GetCoreLocalContext().scheduler;
            }

            static ALWAYS_INLINE KScheduler &GetScheduler(s32 core_id) {
                return GetCoreLocalContext(core_id).scheduler;
            }

            static ALWAYS_INLINE KInterruptTaskManager &GetInterruptTaskManager() {
                return GetCoreLocalContext().interrupt_task_manager;
            }

            static ALWAYS_INLINE KInterruptManager &GetInterruptManager() {
                return GetCoreLocalContext().interrupt_manager;
            }

            static ALWAYS_INLINE KHardwareTimer &GetHardwareTimer() {
                return GetCoreLocalContext(GetCurrentCoreId()).hardware_timer;
            }

            static ALWAYS_INLINE KResourceLimit &GetSystemResourceLimit() {
                return s_system_resource_limit;
            }

            static ALWAYS_INLINE KMemoryManager &GetMemoryManager() {
                return s_memory_manager;
            }

            static ALWAYS_INLINE KMemoryBlockSlabManager &GetApplicationMemoryBlockManager() {
                return s_app_memory_block_manager;
            }

            static ALWAYS_INLINE KMemoryBlockSlabManager &GetSystemMemoryBlockManager() {
                return s_sys_memory_block_manager;
            }

            static ALWAYS_INLINE KBlockInfoManager &GetBlockInfoManager() {
                return s_block_info_manager;
            }

            static ALWAYS_INLINE KPageTableManager &GetPageTableManager() {
                return s_page_table_manager;
            }

            static ALWAYS_INLINE KSupervisorPageTable &GetKernelPageTable() {
                return s_supervisor_page_table;
            }

            static ALWAYS_INLINE KSynchronization &GetSynchronization() {
                return s_synchronization;
            }

            static ALWAYS_INLINE KUnsafeMemory &GetUnsafeMemory() {
                return s_unsafe_memory;
            }

            static ALWAYS_INLINE KWorkerTaskManager &GetWorkerTaskManager(KWorkerTaskManager::WorkerType type) {
                MESOSPHERE_ASSERT(type <= KWorkerTaskManager::WorkerType_Count);
                return s_worker_task_managers[type];
            }
    };

}
