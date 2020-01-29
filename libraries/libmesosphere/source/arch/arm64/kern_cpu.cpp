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

namespace ams::kern::arm64::cpu {

    /* Declare prototype to be implemented in asm. */
    void SynchronizeAllCoresImpl(s32 *sync_var, s32 num_cores);


    namespace {

        /* Expose this as a global, for asm to use. */
        s32 g_all_core_sync_count;

        void FlushEntireDataCacheImpl(int level) {
            /* Used in multiple locations. */
            const u64 level_sel_value = static_cast<u64>(level << 1);

            /* Set selection register. */
            cpu::SetCsselrEl1(level_sel_value);
            cpu::InstructionMemoryBarrier();

            /* Get cache size id info. */
            CacheSizeIdRegisterAccessor ccsidr_el1;
            const int num_sets  = ccsidr_el1.GetNumberOfSets();
            const int num_ways  = ccsidr_el1.GetAssociativity();
            const int line_size = ccsidr_el1.GetLineSize();

            const u64 way_shift = static_cast<u64>(__builtin_clz(num_ways));
            const u64 set_shift = static_cast<u64>(line_size + 4);

            for (int way = 0; way <= num_ways; way++) {
                for (int set = 0; set <= num_sets; set++) {
                    const u64 way_value  = static_cast<u64>(way) << way_shift;
                    const u64 set_value  = static_cast<u64>(set) << set_shift;
                    const u64 cisw_value = way_value | set_value | level_sel_value;
                    __asm__ __volatile__("dc cisw, %0" ::"r"(cisw_value) : "memory");
                }
            }
        }

        ALWAYS_INLINE void SetEventLocally() {
            __asm__ __volatile__("sevl" ::: "memory");
        }

        ALWAYS_INLINE void WaitForEvent() {
            __asm__ __volatile__("wfe" ::: "memory");
        }

    }

    void FlushEntireDataCacheShared() {
        CacheLineIdRegisterAccessor clidr_el1;
        const int levels_of_coherency   = clidr_el1.GetLevelsOfCoherency();
        const int levels_of_unification = clidr_el1.GetLevelsOfUnification();

        for (int level = levels_of_coherency; level >= levels_of_unification; level--) {
            FlushEntireDataCacheImpl(level);
        }
    }

    void FlushEntireDataCacheLocal() {
        CacheLineIdRegisterAccessor clidr_el1;
        const int levels_of_unification = clidr_el1.GetLevelsOfUnification();

        for (int level = levels_of_unification - 1; level >= 0; level--) {
            FlushEntireDataCacheImpl(level);
        }
    }

    NOINLINE void SynchronizeAllCores() {
        SynchronizeAllCoresImpl(&g_all_core_sync_count, static_cast<s32>(cpu::NumCores));
    }

}
