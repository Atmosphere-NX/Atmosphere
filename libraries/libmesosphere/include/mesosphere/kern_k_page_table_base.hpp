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
#include <mesosphere/kern_select_page_table_impl.hpp>
#include <mesosphere/kern_k_light_lock.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_memory_manager.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>

namespace ams::kern {

    class KPageTableBase {
        NON_COPYABLE(KPageTableBase);
        NON_MOVEABLE(KPageTableBase);
        protected:
            enum MemoryFillValue {
                MemoryFillValue_Zero  = 0,
                MemoryFillValue_Stack = 'X',
                MemoryFillValue_Ipc   = 'Y',
                MemoryFillValue_Heap  = 'Z',
            };
        private:
            KProcessAddress address_space_start;
            KProcessAddress address_space_end;
            KProcessAddress heap_region_start;
            KProcessAddress heap_region_end;
            KProcessAddress current_heap_end;
            KProcessAddress alias_region_start;
            KProcessAddress alias_region_end;
            KProcessAddress stack_region_start;
            KProcessAddress stack_region_end;
            KProcessAddress kernel_map_region_start;
            KProcessAddress kernel_map_region_end;
            KProcessAddress alias_code_region_start;
            KProcessAddress alias_code_region_end;
            KProcessAddress code_region_start;
            KProcessAddress code_region_end;
            size_t max_heap_size;
            size_t max_physical_memory_size;
            mutable KLightLock general_lock;
            mutable KLightLock map_physical_memory_lock;
            KPageTableImpl impl;
            /* TODO KMemoryBlockManager memory_block_manager; */
            u32 allocate_option;
            u32 address_space_size;
            bool is_kernel;
            bool enable_aslr;
            KMemoryBlockSlabManager *memory_block_slab_manager;
            KBlockInfoManager *block_info_manager;
            KMemoryRegion *cached_physical_linear_region;
            KMemoryRegion *cached_physical_non_kernel_dram_region;
            KMemoryRegion *cached_virtual_managed_pool_dram_region;
            MemoryFillValue heap_fill_value;
            MemoryFillValue ipc_fill_value;
            MemoryFillValue stack_fill_value;
        public:
            constexpr KPageTableBase() :
                address_space_start(), address_space_end(), heap_region_start(), heap_region_end(), current_heap_end(),
                alias_region_start(), alias_region_end(), stack_region_start(), stack_region_end(), kernel_map_region_start(),
                kernel_map_region_end(), alias_code_region_start(), alias_code_region_end(), code_region_start(), code_region_end(),
                max_heap_size(), max_physical_memory_size(), general_lock(), map_physical_memory_lock(), impl(), /* TODO: memory_block_manager(), */
                allocate_option(), address_space_size(), is_kernel(), enable_aslr(), memory_block_slab_manager(), block_info_manager(),
                cached_physical_linear_region(), cached_physical_non_kernel_dram_region(), cached_virtual_managed_pool_dram_region(),
                heap_fill_value(), ipc_fill_value(), stack_fill_value()
            {
                /* ... */
            }

            NOINLINE Result InitializeForKernel(bool is_64_bit, void *table, KVirtualAddress start, KVirtualAddress end);

            void Finalize();
        public:
            static ALWAYS_INLINE KVirtualAddress GetLinearVirtualAddress(KPhysicalAddress addr) {
                return KMemoryLayout::GetLinearVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetLinearPhysicalAddress(KVirtualAddress addr) {
                return KMemoryLayout::GetLinearPhysicalAddress(addr);
            }
    };

}
