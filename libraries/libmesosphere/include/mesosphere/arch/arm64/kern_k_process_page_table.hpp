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
#include <mesosphere/arch/arm64/kern_k_page_table.hpp>

namespace ams::kern::arch::arm64 {

    class KProcessPageTable {
        private:
            KPageTable page_table;
        public:
            constexpr KProcessPageTable() : page_table() { /* ... */ }

            Result Initialize(u32 id, ams::svc::CreateProcessFlag as_type, bool enable_aslr, bool from_back, KMemoryManager::Pool pool, KProcessAddress code_address, size_t code_size, KMemoryBlockSlabManager *mem_block_slab_manager, KBlockInfoManager *block_info_manager, KPageTableManager *pt_manager) {
                return this->page_table.InitializeForProcess(id, as_type, enable_aslr, from_back, pool, code_address, code_size, mem_block_slab_manager, block_info_manager, pt_manager);
            }

            void Finalize() { this->page_table.Finalize(); }

            Result MapPageGroup(KProcessAddress addr, const KPageGroup &pg, KMemoryState state, KMemoryPermission perm) {
                return this->page_table.MapPageGroup(addr, pg, state, perm);
            }

            bool CanContain(KProcessAddress addr, size_t size) const { return this->page_table.CanContain(addr, size); }
            bool CanContain(KProcessAddress addr, size_t size, KMemoryState state) const { return this->page_table.CanContain(addr, size, state); }
    };

}
