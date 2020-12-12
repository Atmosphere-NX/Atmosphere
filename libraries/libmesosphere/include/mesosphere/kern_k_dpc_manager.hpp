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

namespace ams::kern {

    class KDpcManager {
        private:
            static constexpr s32 DpcManagerNormalThreadPriority     = 59;
            static constexpr s32 DpcManagerPreemptionThreadPriority = 63;

            static_assert(ams::svc::HighestThreadPriority <= DpcManagerNormalThreadPriority && DpcManagerNormalThreadPriority <= ams::svc::LowestThreadPriority);
            static_assert(ams::svc::HighestThreadPriority <= DpcManagerPreemptionThreadPriority && DpcManagerPreemptionThreadPriority <= ams::svc::LowestThreadPriority);
        private:
            static NOINLINE void Initialize(s32 core_id, s32 priority);
        public:
            static void Initialize() {
                const s32 core_id = GetCurrentCoreId();
                if (core_id == static_cast<s32>(cpu::NumCores) - 1) {
                    Initialize(core_id, DpcManagerPreemptionThreadPriority);
                } else {
                    Initialize(core_id, DpcManagerNormalThreadPriority);
                }
            }

            static NOINLINE void HandleDpc();
            static void Sync();
    };

}
