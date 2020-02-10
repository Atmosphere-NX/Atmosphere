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
#include <mesosphere/kern_select_page_table.hpp>

namespace ams::kern {

    Result KPageTableBase::InitializeForKernel(bool is_64_bit, void *table, KVirtualAddress start, KVirtualAddress end) {
        /* Initialize our members. */
        this->address_space_size        = (is_64_bit) ? BITSIZEOF(u64) : BITSIZEOF(u32);
        this->address_space_start       = KProcessAddress(GetInteger(start));
        this->address_space_end         = KProcessAddress(GetInteger(end));
        this->is_kernel                 = true;
        this->enable_aslr               = true;

        this->heap_region_start         = 0;
        this->heap_region_end           = 0;
        this->current_heap_end          = 0;
        this->alias_region_start        = 0;
        this->alias_region_end          = 0;
        this->stack_region_start        = 0;
        this->stack_region_end          = 0;
        this->kernel_map_region_start   = 0;
        this->kernel_map_region_end     = 0;
        this->alias_code_region_start   = 0;
        this->alias_code_region_end     = 0;
        this->code_region_start         = 0;
        this->code_region_end           = 0;
        this->max_heap_size             = 0;
        this->max_physical_memory_size  = 0;

        this->memory_block_slab_manager = std::addressof(Kernel::GetSystemMemoryBlockManager());
        this->block_info_manager        = std::addressof(Kernel::GetBlockInfoManager());

        this->allocate_option           = KMemoryManager::EncodeOption(KMemoryManager::Pool_System, KMemoryManager::Direction_FromFront);
        this->heap_fill_value           = MemoryFillValue_Zero;
        this->ipc_fill_value            = MemoryFillValue_Zero;
        this->stack_fill_value          = MemoryFillValue_Zero;

        this->cached_physical_linear_region           = nullptr;
        this->cached_physical_non_kernel_dram_region  = nullptr;
        this->cached_virtual_managed_pool_dram_region = nullptr;

        /* Initialize our implementation. */
        this->impl.InitializeForKernel(table, start, end);

        /* Initialize our memory block manager. */
        return this->memory_block_manager.Initialize(this->address_space_start, this->address_space_end, this->memory_block_slab_manager);

        return ResultSuccess();
    }

    void KPageTableBase::Finalize() {
        this->memory_block_manager.Finalize(this->memory_block_slab_manager);
        MESOSPHERE_TODO("cpu::InvalidateEntireInstructionCache();");
    }
}
