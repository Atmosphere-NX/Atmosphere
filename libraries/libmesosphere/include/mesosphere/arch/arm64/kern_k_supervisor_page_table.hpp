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
#include <mesosphere/arch/arm64/kern_k_page_table.hpp>

namespace ams::kern::arch::arm64 {

    class KSupervisorPageTable {
        private:
            KPageTable page_table;
            u64 ttbr0[cpu::NumCores];
        public:
            constexpr KSupervisorPageTable() : page_table(), ttbr0() { /* ... */ }

            NOINLINE void Initialize(s32 core_id);
            NOINLINE void Activate();
            void Finalize(s32 core_id);

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
                return this->page_table.MapPages(out_addr, num_pages, alignment, phys_addr, region_start, region_num_pages, state, perm);
            }

            bool GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress address) const {
                return this->page_table.GetPhysicalAddress(out, address);
            }
    };

}
