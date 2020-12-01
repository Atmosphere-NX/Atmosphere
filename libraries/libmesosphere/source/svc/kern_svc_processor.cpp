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

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        int32_t GetCurrentProcessorNumber() {
            /* Setup variables to track affinity information. */
            s32 current_phys_core;
            u64 v_affinity_mask   = 0;

            /* Forever try to get the affinity. */
            while (true) {
                /* Update affinity information if we've run out. */
                while (v_affinity_mask == 0) {
                    current_phys_core = GetCurrentCoreId();
                    v_affinity_mask   = GetCurrentThread().GetVirtualAffinityMask();
                    if ((v_affinity_mask & (1ul << current_phys_core)) != 0) {
                        return current_phys_core;
                    }
                }

                /* Check the next virtual bit. */
                do {
                    const s32 next_virt_core = static_cast<s32>(__builtin_ctzll(v_affinity_mask));
                    if (current_phys_core == cpu::VirtualToPhysicalCoreMap[next_virt_core]) {
                        return next_virt_core;
                    }

                    v_affinity_mask &= ~(1ul << next_virt_core);
                } while (v_affinity_mask != 0);
            }
        }

    }

    /* =============================    64 ABI    ============================= */

    int32_t GetCurrentProcessorNumber64() {
        return GetCurrentProcessorNumber();
    }

    /* ============================= 64From32 ABI ============================= */

    int32_t GetCurrentProcessorNumber64From32() {
        return GetCurrentProcessorNumber();
    }

}
