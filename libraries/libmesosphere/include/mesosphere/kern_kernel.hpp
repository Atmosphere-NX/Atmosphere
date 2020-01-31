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
#include <mesosphere/kern_k_thread.hpp>

namespace ams::kern {

    class Kernel {
        public:
            enum class State : u8 {
                Invalid = 0,
                Initializing = 1,
                Initialized  = 2,
            };
        private:
            static inline State s_state = State::Invalid;
            static inline KThread s_main_threads[cpu::NumCores];
            static inline KThread s_idle_threads[cpu::NumCores];
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
    };

}
