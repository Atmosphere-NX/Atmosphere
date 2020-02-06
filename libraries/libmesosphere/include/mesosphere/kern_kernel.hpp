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

namespace ams::kern {

    class KThread;
    class KHardwareTimer;
    class KResourceLimit;
    class KInterruptManager;
    class KInterruptTaskManager;
    class KScheduler;

    class Kernel {
        public:
            enum class State : u8 {
                Invalid = 0,
                Initializing = 1,
                Initialized  = 2,
            };
        private:
            static State s_state;
            static KThread s_main_threads[cpu::NumCores];
            static KThread s_idle_threads[cpu::NumCores];
            static KResourceLimit s_system_resource_limit;
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

            static ALWAYS_INLINE State GetState() { return s_state; }
            static ALWAYS_INLINE void SetState(State state) { s_state = state; }

            static ALWAYS_INLINE KThread &GetMainThread(s32 core_id) {
                return s_main_threads[core_id];
            }

            static ALWAYS_INLINE KThread &GetIdleThread(s32 core_id) {
                return s_idle_threads[core_id];
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
                return GetCoreLocalContext().hardware_timer;
            }

            static ALWAYS_INLINE KResourceLimit &GetSystemResourceLimit() {
                return s_system_resource_limit;
            }
    };

}
