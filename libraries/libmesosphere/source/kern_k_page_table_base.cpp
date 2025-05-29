/*
 * Copyright (c) Atmosphère-NX
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

    namespace {

        class KScopedLightLockPair {
            NON_COPYABLE(KScopedLightLockPair);
            NON_MOVEABLE(KScopedLightLockPair);
            private:
                KLightLock *m_lower;
                KLightLock *m_upper;
            public:
                ALWAYS_INLINE KScopedLightLockPair(KLightLock &lhs, KLightLock &rhs) {
                    /* Ensure our locks are in a consistent order. */
                    if (std::addressof(lhs) <= std::addressof(rhs)) {
                        m_lower = std::addressof(lhs);
                        m_upper = std::addressof(rhs);
                    } else {
                        m_lower = std::addressof(rhs);
                        m_upper = std::addressof(lhs);
                    }

                    /* Acquire both locks. */
                    m_lower->Lock();
                    if (m_lower != m_upper) {
                        m_upper->Lock();
                    }
                }

                ~KScopedLightLockPair() {
                    /* Unlock the upper lock. */
                    if (m_upper != nullptr && m_upper != m_lower) {
                        m_upper->Unlock();
                    }

                    /* Unlock the lower lock. */
                    if (m_lower != nullptr) {
                        m_lower->Unlock();
                    }
                }
            public:
                /* Utility. */
                ALWAYS_INLINE void TryUnlockHalf(KLightLock &lock) {
                    /* Only allow unlocking if the lock is half the pair. */
                    if (m_lower != m_upper) {
                        /* We want to be sure the lock is one we own. */
                        if (m_lower == std::addressof(lock)) {
                            lock.Unlock();
                            m_lower = nullptr;
                        } else if (m_upper == std::addressof(lock)) {
                            lock.Unlock();
                            m_upper = nullptr;
                        }
                    }
                }
        };

    }

    void KPageTableBase::MemoryRange::Open() {
        /* If the range contains heap pages, open them. */
        if (this->IsHeap()) {
            Kernel::GetMemoryManager().Open(this->GetAddress(), this->GetSize() / PageSize);
        }
    }

    void KPageTableBase::MemoryRange::Close() {
        /* If the range contains heap pages, close them. */
        if (this->IsHeap()) {
            Kernel::GetMemoryManager().Close(this->GetAddress(), this->GetSize() / PageSize);
        }
    }

    Result KPageTableBase::InitializeForKernel(bool is_64_bit, void *table, KVirtualAddress start, KVirtualAddress end) {
        /* Initialize our members. */
        m_address_space_width               = (is_64_bit) ? BITSIZEOF(u64) : BITSIZEOF(u32);
        m_address_space_start               = KProcessAddress(GetInteger(start));
        m_address_space_end                 = KProcessAddress(GetInteger(end));
        m_is_kernel                         = true;
        m_enable_aslr                       = true;
        m_enable_device_address_space_merge = false;

        for (auto i = 0; i < RegionType_Count; ++i) {
            m_region_starts[i] = 0;
            m_region_ends[i]   = 0;
        }

        m_current_heap_end                  = 0;
        m_alias_code_region_start           = 0;
        m_alias_code_region_end             = 0;
        m_code_region_start                 = 0;
        m_code_region_end                   = 0;
        m_max_heap_size                     = 0;
        m_mapped_physical_memory_size       = 0;
        m_mapped_unsafe_physical_memory     = 0;
        m_mapped_insecure_memory            = 0;
        m_mapped_ipc_server_memory          = 0;
        m_alias_region_extra_size           = 0;

        m_memory_block_slab_manager         = Kernel::GetSystemSystemResource().GetMemoryBlockSlabManagerPointer();
        m_block_info_manager                = Kernel::GetSystemSystemResource().GetBlockInfoManagerPointer();
        m_resource_limit                    = std::addressof(Kernel::GetSystemResourceLimit());

        m_allocate_option                   = KMemoryManager::EncodeOption(KMemoryManager::Pool_System, KMemoryManager::Direction_FromFront);
        m_heap_fill_value                   = MemoryFillValue_Zero;
        m_ipc_fill_value                    = MemoryFillValue_Zero;
        m_stack_fill_value                  = MemoryFillValue_Zero;

        m_cached_physical_linear_region     = nullptr;
        m_cached_physical_heap_region       = nullptr;

        /* Initialize our implementation. */
        m_impl.InitializeForKernel(table, start, end);

        /* Initialize our memory block manager. */
        R_RETURN(m_memory_block_manager.Initialize(m_address_space_start, m_address_space_end, m_memory_block_slab_manager));
    }

    Result KPageTableBase::InitializeForProcess(ams::svc::CreateProcessFlag flags, bool from_back, KMemoryManager::Pool pool, void *table, KProcessAddress start, KProcessAddress end, KProcessAddress code_address, size_t code_size, KSystemResource *system_resource, KResourceLimit *resource_limit) {
        /* Validate the region. */
        MESOSPHERE_ABORT_UNLESS(start <= code_address);
        MESOSPHERE_ABORT_UNLESS(code_address < code_address + code_size);
        MESOSPHERE_ABORT_UNLESS(code_address + code_size - 1 <= end - 1);

        /* Define helpers. */
        auto GetSpaceStart = [&](KAddressSpaceInfo::Type type) ALWAYS_INLINE_LAMBDA {
            return KAddressSpaceInfo::GetAddressSpaceStart(flags, type, code_size);
        };
        auto GetSpaceSize = [&](KAddressSpaceInfo::Type type) ALWAYS_INLINE_LAMBDA {
            return KAddressSpaceInfo::GetAddressSpaceSize(flags, type);
        };

        /* Default to zero alias region extra size. */
        m_alias_region_extra_size = 0;

        /* Set our width and heap/alias sizes. */
        m_address_space_width = GetAddressSpaceWidth(flags);
        size_t alias_region_size  = GetSpaceSize(KAddressSpaceInfo::Type_Alias);
        size_t heap_region_size   = GetSpaceSize(KAddressSpaceInfo::Type_Heap);

        /* Set code regions and determine remaining sizes. */
        KProcessAddress process_code_start;
        KProcessAddress process_code_end;
        size_t stack_region_size;
        size_t kernel_map_region_size;
        KProcessAddress before_process_code_start, after_process_code_start;
        size_t before_process_code_size, after_process_code_size;
        if (m_address_space_width == 39) {
            stack_region_size                     = GetSpaceSize(KAddressSpaceInfo::Type_Stack);
            kernel_map_region_size                = GetSpaceSize(KAddressSpaceInfo::Type_MapSmall);

            m_code_region_start                   = GetSpaceStart(KAddressSpaceInfo::Type_Map39Bit);
            m_code_region_end                     = m_code_region_start + GetSpaceSize(KAddressSpaceInfo::Type_Map39Bit);
            m_alias_code_region_start             = m_code_region_start;
            m_alias_code_region_end               = m_code_region_end;

            process_code_start                    = util::AlignDown(GetInteger(code_address), RegionAlignment);
            process_code_end                      = util::AlignUp(GetInteger(code_address) + code_size, RegionAlignment);

            before_process_code_start             = m_code_region_start;
            before_process_code_size              = process_code_start - before_process_code_start;
            after_process_code_start              = process_code_end;
            after_process_code_size               = m_code_region_end - process_code_end;

            /* If we have a 39-bit address space and should, enable extra size to the alias region. */
            if (flags & ams::svc::CreateProcessFlag_EnableAliasRegionExtraSize) {
                /* Extra size is 1/8th of the address space. */
                m_alias_region_extra_size = (static_cast<size_t>(1) << m_address_space_width) / 8;

                alias_region_size += m_alias_region_extra_size;
            }
        } else {
            stack_region_size                     = 0;
            kernel_map_region_size                = 0;

            m_code_region_start                   = GetSpaceStart(KAddressSpaceInfo::Type_MapSmall);
            m_code_region_end                     = m_code_region_start + GetSpaceSize(KAddressSpaceInfo::Type_MapSmall);
            m_alias_code_region_start             = m_code_region_start;
            m_alias_code_region_end               = GetSpaceStart(KAddressSpaceInfo::Type_MapLarge) + GetSpaceSize(KAddressSpaceInfo::Type_MapLarge);
            m_region_starts[RegionType_Stack]     = m_code_region_start;
            m_region_ends[RegionType_Stack]       = m_code_region_end;
            m_region_starts[RegionType_KernelMap] = m_code_region_start;
            m_region_ends[RegionType_KernelMap]   = m_code_region_end;

            process_code_start                    = m_code_region_start;
            process_code_end                      = m_code_region_end;

            before_process_code_start             = m_code_region_start;
            before_process_code_size              = 0;
            after_process_code_start              = GetSpaceStart(KAddressSpaceInfo::Type_MapLarge);
            after_process_code_size               = GetSpaceSize(KAddressSpaceInfo::Type_MapLarge);
        }

        /* Set other basic fields. */
        m_enable_aslr                       = (flags & ams::svc::CreateProcessFlag_EnableAslr) != 0;
        m_enable_device_address_space_merge = (flags & ams::svc::CreateProcessFlag_DisableDeviceAddressSpaceMerge) == 0;
        m_address_space_start               = start;
        m_address_space_end                 = end;
        m_is_kernel                         = false;
        m_memory_block_slab_manager         = system_resource->GetMemoryBlockSlabManagerPointer();
        m_block_info_manager                = system_resource->GetBlockInfoManagerPointer();
        m_resource_limit                    = resource_limit;

        /* Set up our undetermined regions. */
        {
            /* Declare helper structure for layout process. */
            struct RegionLayoutInfo {
                size_t size;
                RegionType type;
                s32 alloc_index; /* 0 for before process code, 1 for after process code */
            };

            /* Create region layout info array, and add regions to it. */
            RegionLayoutInfo region_layouts[RegionType_Count] = {};
            size_t num_regions = 0;

            if (kernel_map_region_size > 0) { region_layouts[num_regions++] = { .size = kernel_map_region_size, .type = RegionType_KernelMap, .alloc_index = 0, }; }
            if (stack_region_size > 0)      { region_layouts[num_regions++] = { .size = stack_region_size,      .type = RegionType_Stack,     .alloc_index = 0, }; }

            region_layouts[num_regions++] = { .size = alias_region_size, .type = RegionType_Alias, .alloc_index = 0, };
            region_layouts[num_regions++] = { .size = heap_region_size,  .type = RegionType_Heap,  .alloc_index = 0, };

            /* Selection-sort the regions by size largest-to-smallest. */
            for (size_t i = 0; i < num_regions - 1; ++i) {
                for (size_t j = i + 1; j < num_regions; ++j) {
                    if (region_layouts[i].size < region_layouts[j].size) {
                        std::swap(region_layouts[i], region_layouts[j]);
                    }
                }
            }

            /* Layout the regions. */
            constexpr auto AllocIndexCount = 2;
            KProcessAddress alloc_starts[AllocIndexCount] = { before_process_code_start, after_process_code_start };
            size_t alloc_sizes[AllocIndexCount] = { before_process_code_size, after_process_code_size };
            size_t alloc_counts[AllocIndexCount] = {};
            for (size_t i = 0; i < num_regions; ++i) {
                /* Get reference to the current region. */
                auto &cur_region = region_layouts[i];

                /* Determine where the current region should go. */
                cur_region.alloc_index = alloc_sizes[1] >= alloc_sizes[0] ? 1 : 0;
                ++alloc_counts[cur_region.alloc_index];

                /* Check that the current region can fit. */
                R_UNLESS(alloc_sizes[cur_region.alloc_index] >= cur_region.size, svc::ResultOutOfMemory());

                /* Update our remaining size tracking. */
                alloc_sizes[cur_region.alloc_index] -= cur_region.size;
            }

            /* Selection sort the regions to coalesce them by alloc index. */
            for (size_t i = 0; i < num_regions - 1; ++i) {
                for (size_t j = i + 1; j < num_regions; ++j) {
                    if (region_layouts[i].alloc_index > region_layouts[j].alloc_index) {
                        std::swap(region_layouts[i], region_layouts[j]);
                    }
                }
            }

            /* Layout the regions for each alloc index. */
            for (auto cur_alloc_index = 0; cur_alloc_index < AllocIndexCount; ++cur_alloc_index) {
                /* If there are no regions to place, continue. */
                const size_t cur_alloc_count = alloc_counts[cur_alloc_index];
                if (cur_alloc_count == 0) {
                    continue;
                }

                /* Determine the starting region index for the current alloc index. */
                size_t cur_region_index = 0;
                for (size_t i = 0; i < num_regions; ++i) {
                    if (region_layouts[i].alloc_index == cur_alloc_index) {
                        cur_region_index = i;
                        break;
                    }
                }

                /* If aslr is enabled, randomize the current region order. Otherwise, sort by type. */
                if (m_enable_aslr) {
                    for (size_t i = 0; i < cur_alloc_count - 1; ++i) {
                        std::swap(region_layouts[cur_region_index + i], region_layouts[cur_region_index + KSystemControl::GenerateRandomRange(i, cur_alloc_count - 1)]);
                    }
                } else {
                    for (size_t i = 0; i < cur_alloc_count - 1; ++i) {
                        for (size_t j = i + 1; j < cur_alloc_count; ++j) {
                            if (region_layouts[cur_region_index + i].type > region_layouts[cur_region_index + j].type) {
                                std::swap(region_layouts[cur_region_index + i], region_layouts[cur_region_index + j]);
                            }
                        }
                    }
                }

                /* Determine aslr offsets for the current space. */
                size_t aslr_offsets[RegionType_Count] = {};
                if (m_enable_aslr) {
                    /* Generate the aslr offsets. */
                    for (size_t i = 0; i < cur_alloc_count; ++i) {
                        aslr_offsets[i] = KSystemControl::GenerateRandomRange(0, alloc_sizes[cur_alloc_index] / RegionAlignment) * RegionAlignment;
                    }

                    /* Sort the aslr offsets. */
                    for (size_t i = 0; i < cur_alloc_count - 1; ++i) {
                        for (size_t j = i + 1; j < cur_alloc_count; ++j) {
                            if (aslr_offsets[i] > aslr_offsets[j]) {
                                std::swap(aslr_offsets[i], aslr_offsets[j]);
                            }
                        }
                    }
                }

                /* Calculate final region positions. */
                KProcessAddress prev_region_end = alloc_starts[cur_alloc_index];
                size_t prev_aslr_offset = 0;
                for (size_t i = 0; i < cur_alloc_count; ++i) {
                    /* Get the current region. */
                    auto &cur_region = region_layouts[cur_region_index + i];

                    /* Set the current region start/end. */
                    m_region_starts[cur_region.type] = (aslr_offsets[i] - prev_aslr_offset) + GetInteger(prev_region_end);
                    m_region_ends[cur_region.type]   = m_region_starts[cur_region.type] + cur_region.size;

                    /* Update tracking variables. */
                    prev_region_end  = m_region_ends[cur_region.type];
                    prev_aslr_offset = aslr_offsets[i];
                }
            }

            /* Declare helpers to check that regions are inside our address space. */
            const KProcessAddress process_code_last = process_code_end - 1;
            auto IsInAddressSpace = [&](KProcessAddress addr) ALWAYS_INLINE_LAMBDA { return m_address_space_start <= addr && addr <= m_address_space_end; };

            /* Ensure that the KernelMap region is valid. */
            for (size_t k = 0; k < num_regions; ++k) {
                if (const auto &kmap_region = region_layouts[k]; kmap_region.type == RegionType_KernelMap) {
                    /* If there's no kmap region, we have nothing to check. */
                    if (kmap_region.size == 0) {
                        break;
                    }

                    /* Check that the kmap region is within our address space. */
                    MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(m_region_starts[RegionType_KernelMap]));
                    MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(m_region_ends[RegionType_KernelMap]));

                    /* Check for overlap with process code. */
                    const KProcessAddress kmap_start  = m_region_starts[RegionType_KernelMap];
                    const KProcessAddress kmap_last   = m_region_ends[RegionType_KernelMap] - 1;
                    MESOSPHERE_ABORT_UNLESS(kernel_map_region_size == 0 || kmap_last < process_code_start || process_code_last < kmap_start);

                    /* Check for overlap with stack. */
                    for (size_t s = 0; s < num_regions; ++s) {
                        if (const auto &stack_region = region_layouts[s]; stack_region.type == RegionType_Stack) {
                            if (stack_region.size != 0) {
                                const KProcessAddress stack_start = m_region_starts[RegionType_Stack];
                                const KProcessAddress stack_last  = m_region_ends[RegionType_Stack] - 1;
                                MESOSPHERE_ABORT_UNLESS((kernel_map_region_size == 0 && stack_region_size == 0) || kmap_last < stack_start || stack_last < kmap_start);
                            }
                            break;
                        }
                    }

                    /* Check for overlap with alias. */
                    for (size_t a = 0; a < num_regions; ++a) {
                        if (const auto &alias_region = region_layouts[a]; alias_region.type == RegionType_Alias) {
                            if (alias_region.size != 0) {
                                const KProcessAddress alias_start = m_region_starts[RegionType_Alias];
                                const KProcessAddress alias_last  = m_region_ends[RegionType_Alias] - 1;
                                MESOSPHERE_ABORT_UNLESS(kmap_last < alias_start || alias_last < kmap_start);
                            }
                            break;
                        }
                    }

                    /* Check for overlap with heap. */
                    for (size_t h = 0; h < num_regions; ++h) {
                        if (const auto &heap_region = region_layouts[h]; heap_region.type == RegionType_Heap) {
                            if (heap_region.size != 0) {
                                const KProcessAddress heap_start = m_region_starts[RegionType_Heap];
                                const KProcessAddress heap_last  = m_region_ends[RegionType_Heap] - 1;
                                MESOSPHERE_ABORT_UNLESS(kmap_last < heap_start || heap_last < kmap_start);
                            }
                            break;
                        }
                    }
                }
            }

            /* Check that the Stack region is valid. */
            for (size_t s = 0; s < num_regions; ++s) {
                if (const auto &stack_region = region_layouts[s]; stack_region.type == RegionType_Stack) {
                    /* If there's no stack region, we have nothing to check. */
                    if (stack_region.size == 0) {
                        break;
                    }

                    /* Check that the stack region is within our address space. */
                    MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(m_region_starts[RegionType_Stack]));
                    MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(m_region_ends[RegionType_Stack]));

                    /* Check for overlap with process code. */
                    const KProcessAddress stack_start = m_region_starts[RegionType_Stack];
                    const KProcessAddress stack_last  = m_region_ends[RegionType_Stack] - 1;
                    MESOSPHERE_ABORT_UNLESS(stack_region_size == 0 || stack_last < process_code_start || process_code_last < stack_start);

                    /* Check for overlap with alias. */
                    for (size_t a = 0; a < num_regions; ++a) {
                        if (const auto &alias_region = region_layouts[a]; alias_region.type == RegionType_Alias) {
                            if (alias_region.size != 0) {
                                const KProcessAddress alias_start = m_region_starts[RegionType_Alias];
                                const KProcessAddress alias_last  = m_region_ends[RegionType_Alias] - 1;
                                MESOSPHERE_ABORT_UNLESS(stack_last < alias_start || alias_last < stack_start);
                            }
                            break;
                        }
                    }

                    /* Check for overlap with heap. */
                    for (size_t h = 0; h < num_regions; ++h) {
                        if (const auto &heap_region = region_layouts[h]; heap_region.type == RegionType_Heap) {
                            if (heap_region.size != 0) {
                                const KProcessAddress heap_start = m_region_starts[RegionType_Heap];
                                const KProcessAddress heap_last  = m_region_ends[RegionType_Heap] - 1;
                                MESOSPHERE_ABORT_UNLESS(stack_last < heap_start || heap_last < stack_start);
                            }
                            break;
                        }
                    }
                }
            }

            /* Check that the Alias region is valid. */
            for (size_t a = 0; a < num_regions; ++a) {
                if (const auto &alias_region = region_layouts[a]; alias_region.type == RegionType_Alias) {
                    /* If there's no alias region, we have nothing to check. */
                    if (alias_region.size == 0) {
                        break;
                    }

                    /* Check that the alias region is within our address space. */
                    MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(m_region_starts[RegionType_Alias]));
                    MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(m_region_ends[RegionType_Alias]));

                    /* Check for overlap with process code. */
                    const KProcessAddress alias_start = m_region_starts[RegionType_Alias];
                    const KProcessAddress alias_last  = m_region_ends[RegionType_Alias] - 1;
                    MESOSPHERE_ABORT_UNLESS(alias_last < process_code_start || process_code_last < alias_start);

                    /* Check for overlap with heap. */
                    for (size_t h = 0; h < num_regions; ++h) {
                        if (const auto &heap_region = region_layouts[h]; heap_region.type == RegionType_Heap) {
                            if (heap_region.size != 0) {
                                const KProcessAddress heap_start = m_region_starts[RegionType_Heap];
                                const KProcessAddress heap_last  = m_region_ends[RegionType_Heap] - 1;
                                MESOSPHERE_ABORT_UNLESS(alias_last < heap_start || heap_last < alias_start);
                            }
                            break;
                        }
                    }
                }
            }

            /* Check that the Heap region is valid. */
            for (size_t h = 0; h < num_regions; ++h) {
                if (const auto &heap_region = region_layouts[h]; heap_region.type == RegionType_Heap) {
                    /* If there's no heap region, we have nothing to check. */
                    if (heap_region.size == 0) {
                        break;
                    }

                    /* Check that the heap region is within our address space. */
                    MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(m_region_starts[RegionType_Heap]));
                    MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(m_region_ends[RegionType_Heap]));

                    /* Check for overlap with process code. */
                    const KProcessAddress heap_start = m_region_starts[RegionType_Heap];
                    const KProcessAddress heap_last  = m_region_ends[RegionType_Heap] - 1;
                    MESOSPHERE_ABORT_UNLESS(heap_last < process_code_start || process_code_last < heap_start);
                }
            }
        }

        /* Set heap and fill members. */
        m_current_heap_end              = m_region_starts[RegionType_Heap];
        m_max_heap_size                 = 0;
        m_mapped_physical_memory_size   = 0;
        m_mapped_unsafe_physical_memory = 0;
        m_mapped_insecure_memory        = 0;
        m_mapped_ipc_server_memory      = 0;

        const bool fill_memory = KTargetSystem::IsDebugMemoryFillEnabled();
        m_heap_fill_value  = fill_memory ? MemoryFillValue_Heap  : MemoryFillValue_Zero;
        m_ipc_fill_value   = fill_memory ? MemoryFillValue_Ipc   : MemoryFillValue_Zero;
        m_stack_fill_value = fill_memory ? MemoryFillValue_Stack : MemoryFillValue_Zero;

        /* Set allocation option. */
        m_allocate_option = KMemoryManager::EncodeOption(pool, from_back ? KMemoryManager::Direction_FromBack : KMemoryManager::Direction_FromFront);

        /* Initialize our implementation. */
        m_impl.InitializeForProcess(table, GetInteger(start), GetInteger(end));

        /* Initialize our memory block manager. */
        R_RETURN(m_memory_block_manager.Initialize(m_address_space_start, m_address_space_end, m_memory_block_slab_manager));
    }


    void KPageTableBase::Finalize() {
        /* Finalize memory blocks. */
        m_memory_block_manager.Finalize(m_memory_block_slab_manager);

        /* Free any unsafe mapped memory. */
        if (m_mapped_unsafe_physical_memory) {
            Kernel::GetUnsafeMemory().Release(m_mapped_unsafe_physical_memory);
        }

        /* Release any insecure mapped memory. */
        if (m_mapped_insecure_memory) {
            if (auto * const insecure_resource_limit = KSystemControl::GetInsecureMemoryResourceLimit(); insecure_resource_limit != nullptr) {
                insecure_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, m_mapped_insecure_memory);
            }
        }

        /* Release any ipc server memory. */
        if (m_mapped_ipc_server_memory) {
            m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, m_mapped_ipc_server_memory);
        }

        /* Invalidate the entire instruction cache. */
        cpu::InvalidateEntireInstructionCache();
    }

    KProcessAddress KPageTableBase::GetRegionAddress(ams::svc::MemoryState state) const {
        switch (state) {
            case ams::svc::MemoryState_Free:
            case ams::svc::MemoryState_Kernel:
                return m_address_space_start;
            case ams::svc::MemoryState_Normal:
                return m_region_starts[RegionType_Heap];
            case ams::svc::MemoryState_Ipc:
            case ams::svc::MemoryState_NonSecureIpc:
            case ams::svc::MemoryState_NonDeviceIpc:
                return m_region_starts[RegionType_Alias];
            case ams::svc::MemoryState_Stack:
                return m_region_starts[RegionType_Stack];
            case ams::svc::MemoryState_Static:
            case ams::svc::MemoryState_ThreadLocal:
                return m_region_starts[RegionType_KernelMap];
            case ams::svc::MemoryState_Io:
            case ams::svc::MemoryState_Shared:
            case ams::svc::MemoryState_AliasCode:
            case ams::svc::MemoryState_AliasCodeData:
            case ams::svc::MemoryState_Transfered:
            case ams::svc::MemoryState_SharedTransfered:
            case ams::svc::MemoryState_SharedCode:
            case ams::svc::MemoryState_GeneratedCode:
            case ams::svc::MemoryState_CodeOut:
            case ams::svc::MemoryState_Coverage:
            case ams::svc::MemoryState_Insecure:
                return m_alias_code_region_start;
            case ams::svc::MemoryState_Code:
            case ams::svc::MemoryState_CodeData:
                return m_code_region_start;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }
    }

    size_t KPageTableBase::GetRegionSize(ams::svc::MemoryState state) const {
        switch (state) {
            case ams::svc::MemoryState_Free:
            case ams::svc::MemoryState_Kernel:
                return m_address_space_end - m_address_space_start;
            case ams::svc::MemoryState_Normal:
                return m_region_ends[RegionType_Heap] - m_region_starts[RegionType_Heap];
            case ams::svc::MemoryState_Ipc:
            case ams::svc::MemoryState_NonSecureIpc:
            case ams::svc::MemoryState_NonDeviceIpc:
                return m_region_ends[RegionType_Alias] - m_region_starts[RegionType_Alias];
            case ams::svc::MemoryState_Stack:
                return m_region_ends[RegionType_Stack] - m_region_starts[RegionType_Stack];
            case ams::svc::MemoryState_Static:
            case ams::svc::MemoryState_ThreadLocal:
                return m_region_ends[RegionType_KernelMap] - m_region_starts[RegionType_KernelMap];
            case ams::svc::MemoryState_Io:
            case ams::svc::MemoryState_Shared:
            case ams::svc::MemoryState_AliasCode:
            case ams::svc::MemoryState_AliasCodeData:
            case ams::svc::MemoryState_Transfered:
            case ams::svc::MemoryState_SharedTransfered:
            case ams::svc::MemoryState_SharedCode:
            case ams::svc::MemoryState_GeneratedCode:
            case ams::svc::MemoryState_CodeOut:
            case ams::svc::MemoryState_Coverage:
            case ams::svc::MemoryState_Insecure:
                return m_alias_code_region_end - m_alias_code_region_start;
            case ams::svc::MemoryState_Code:
            case ams::svc::MemoryState_CodeData:
                return m_code_region_end - m_code_region_start;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool KPageTableBase::CanContain(KProcessAddress addr, size_t size, ams::svc::MemoryState state) const {
        const KProcessAddress end = addr + size;
        const KProcessAddress last = end - 1;

        const KProcessAddress region_start = this->GetRegionAddress(state);
        const size_t region_size           = this->GetRegionSize(state);

        const bool is_in_region = region_start <= addr && addr < end && last <= region_start + region_size - 1;
        const bool is_in_heap   = !(end <= m_region_starts[RegionType_Heap] || m_region_ends[RegionType_Heap] <= addr || m_region_starts[RegionType_Heap] == m_region_ends[RegionType_Heap]);
        const bool is_in_alias  = !(end <= m_region_starts[RegionType_Alias] || m_region_ends[RegionType_Alias] <= addr || m_region_starts[RegionType_Alias] == m_region_ends[RegionType_Alias]);
        switch (state) {
            case ams::svc::MemoryState_Free:
            case ams::svc::MemoryState_Kernel:
                return is_in_region;
            case ams::svc::MemoryState_Io:
            case ams::svc::MemoryState_Static:
            case ams::svc::MemoryState_Code:
            case ams::svc::MemoryState_CodeData:
            case ams::svc::MemoryState_Shared:
            case ams::svc::MemoryState_AliasCode:
            case ams::svc::MemoryState_AliasCodeData:
            case ams::svc::MemoryState_Stack:
            case ams::svc::MemoryState_ThreadLocal:
            case ams::svc::MemoryState_Transfered:
            case ams::svc::MemoryState_SharedTransfered:
            case ams::svc::MemoryState_SharedCode:
            case ams::svc::MemoryState_GeneratedCode:
            case ams::svc::MemoryState_CodeOut:
            case ams::svc::MemoryState_Coverage:
            case ams::svc::MemoryState_Insecure:
                return is_in_region && !is_in_heap && !is_in_alias;
            case ams::svc::MemoryState_Normal:
                MESOSPHERE_ASSERT(is_in_heap);
                return is_in_region && !is_in_alias;
            case ams::svc::MemoryState_Ipc:
            case ams::svc::MemoryState_NonSecureIpc:
            case ams::svc::MemoryState_NonDeviceIpc:
                MESOSPHERE_ASSERT(is_in_alias);
                return is_in_region && !is_in_heap;
            default:
                return false;
        }
    }

    Result KPageTableBase::CheckMemoryState(KMemoryBlockManager::const_iterator it, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const {
        /* Validate the states match expectation. */
        R_UNLESS((it->GetState()      & state_mask) == state, svc::ResultInvalidCurrentMemory());
        R_UNLESS((it->GetPermission() & perm_mask)  == perm,  svc::ResultInvalidCurrentMemory());
        R_UNLESS((it->GetAttribute()  & attr_mask)  == attr,  svc::ResultInvalidCurrentMemory());

        R_SUCCEED();
    }

    Result KPageTableBase::CheckMemoryStateContiguous(size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Get information about the first block. */
        const KProcessAddress last_addr = addr + size - 1;
        KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(addr);

        /* If the start address isn't aligned, we need a block. */
        const size_t blocks_for_start_align = (util::AlignDown(GetInteger(addr), PageSize) != it->GetAddress()) ? 1 : 0;

        while (true) {
            /* Validate against the provided masks. */
            R_TRY(this->CheckMemoryState(it, state_mask, state, perm_mask, perm, attr_mask, attr));

            /* Break once we're done. */
            if (last_addr <= it->GetLastAddress()) {
                break;
            }

            /* Advance our iterator. */
            it++;
            MESOSPHERE_ASSERT(it != m_memory_block_manager.cend());
        }

        /* If the end address isn't aligned, we need a block. */
        const size_t blocks_for_end_align = (util::AlignUp(GetInteger(addr) + size, PageSize) != it->GetEndAddress()) ? 1 : 0;

        if (out_blocks_needed != nullptr) {
            *out_blocks_needed = blocks_for_start_align + blocks_for_end_align;
        }

        R_SUCCEED();
    }

    Result KPageTableBase::CheckMemoryState(KMemoryState *out_state, KMemoryPermission *out_perm, KMemoryAttribute *out_attr, size_t *out_blocks_needed, KMemoryBlockManager::const_iterator it, KProcessAddress last_addr, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr) const {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Validate all blocks in the range have correct state. */
        const KMemoryState      first_state = it->GetState();
        const KMemoryPermission first_perm  = it->GetPermission();
        const KMemoryAttribute  first_attr  = it->GetAttribute();
        while (true) {
            /* Validate the current block. */
            R_UNLESS(it->GetState() == first_state,                                    svc::ResultInvalidCurrentMemory());
            R_UNLESS(it->GetPermission() == first_perm,                                svc::ResultInvalidCurrentMemory());
            R_UNLESS((it->GetAttribute() | ignore_attr) == (first_attr | ignore_attr), svc::ResultInvalidCurrentMemory());

            /* Validate against the provided masks. */
            R_TRY(this->CheckMemoryState(it, state_mask, state, perm_mask, perm, attr_mask, attr));

            /* Break once we're done. */
            if (last_addr <= it->GetLastAddress()) {
                break;
            }

            /* Advance our iterator. */
            it++;
            MESOSPHERE_ASSERT(it != m_memory_block_manager.cend());
        }

        /* Write output state. */
        if (out_state != nullptr) {
            *out_state = first_state;
        }
        if (out_perm != nullptr) {
            *out_perm = first_perm;
        }
        if (out_attr != nullptr) {
            *out_attr = static_cast<KMemoryAttribute>(first_attr & ~ignore_attr);
        }

        /* If the end address isn't aligned, we need a block. */
        if (out_blocks_needed != nullptr) {
            const size_t blocks_for_end_align = (util::AlignDown(GetInteger(last_addr), PageSize) + PageSize != it->GetEndAddress()) ? 1 : 0;
            *out_blocks_needed = blocks_for_end_align;
        }

        R_SUCCEED();
    }

    Result KPageTableBase::CheckMemoryState(KMemoryState *out_state, KMemoryPermission *out_perm, KMemoryAttribute *out_attr, size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr) const {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Check memory state. */
        const KProcessAddress last_addr = addr + size - 1;
        KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(addr);
        R_TRY(this->CheckMemoryState(out_state, out_perm, out_attr, out_blocks_needed, it, last_addr, state_mask, state, perm_mask, perm, attr_mask, attr, ignore_attr));

        /* If the start address isn't aligned, we need a block. */
        if (out_blocks_needed != nullptr && util::AlignDown(GetInteger(addr), PageSize) != it->GetAddress()) {
            ++(*out_blocks_needed);
        }

        R_SUCCEED();
    }

    Result KPageTableBase::LockMemoryAndOpen(KPageGroup *out_pg, KPhysicalAddress *out_paddr, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, KMemoryPermission new_perm, u32 lock_attr) {
        /* Validate basic preconditions. */
        MESOSPHERE_ASSERT((lock_attr & attr) == 0);
        MESOSPHERE_ASSERT((lock_attr & (KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared)) == 0);

        /* Validate the lock request. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(addr, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check that the output page group is empty, if it exists. */
        if (out_pg) {
            MESOSPHERE_ASSERT(out_pg->GetNumPages() == 0);
        }

        /* Check the state. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        KMemoryAttribute old_attr;
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), std::addressof(old_attr), std::addressof(num_allocator_blocks), addr, size, state_mask | KMemoryState_FlagReferenceCounted, state | KMemoryState_FlagReferenceCounted, perm_mask, perm, attr_mask, attr));

        /* Get the physical address, if we're supposed to. */
        if (out_paddr != nullptr) {
            MESOSPHERE_ABORT_UNLESS(this->GetPhysicalAddressLocked(out_paddr, addr));
        }

        /* Make the page group, if we're supposed to. */
        if (out_pg != nullptr) {
            R_TRY(this->MakePageGroup(*out_pg, addr, num_pages));
        }

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* Decide on new perm and attr. */
        new_perm = (new_perm != KMemoryPermission_None) ? new_perm : old_perm;
        KMemoryAttribute new_attr = static_cast<KMemoryAttribute>(old_attr | lock_attr);

        /* Update permission, if we need to. */
        if (new_perm != old_perm) {
            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            const KPageProperties properties = { new_perm, false, (old_attr & KMemoryAttribute_Uncached) != 0, DisableMergeAttribute_DisableHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
        }

        /* Apply the memory block updates. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, new_perm, new_attr, KMemoryBlockDisableMergeAttribute_Locked, KMemoryBlockDisableMergeAttribute_None);

        /* If we have an output group, open. */
        if (out_pg) {
            out_pg->Open();
        }

        R_SUCCEED();
    }

    Result KPageTableBase::UnlockMemory(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, KMemoryPermission new_perm, u32 lock_attr, const KPageGroup *pg) {
        /* Validate basic preconditions. */
        MESOSPHERE_ASSERT((attr_mask & lock_attr) == lock_attr);
        MESOSPHERE_ASSERT((attr      & lock_attr) == lock_attr);

        /* Validate the unlock request. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(addr, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the state. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        KMemoryAttribute old_attr;
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), std::addressof(old_attr), std::addressof(num_allocator_blocks), addr, size, state_mask | KMemoryState_FlagReferenceCounted, state | KMemoryState_FlagReferenceCounted, perm_mask, perm, attr_mask, attr));

        /* Check the page group. */
        if (pg != nullptr) {
            R_UNLESS(this->IsValidPageGroup(*pg, addr, num_pages), svc::ResultInvalidMemoryRegion());
        }

        /* Decide on new perm and attr. */
        new_perm = (new_perm != KMemoryPermission_None) ? new_perm : old_perm;
        KMemoryAttribute new_attr = static_cast<KMemoryAttribute>(old_attr & ~lock_attr);

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* Update permission, if we need to. */
        if (new_perm != old_perm) {
            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            const KPageProperties properties = { new_perm, false, (old_attr & KMemoryAttribute_Uncached) != 0, DisableMergeAttribute_EnableAndMergeHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
        }

        /* Apply the memory block updates. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, new_perm, new_attr, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Locked);

        R_SUCCEED();
    }

    Result KPageTableBase::QueryInfoImpl(KMemoryInfo *out_info, ams::svc::PageInfo *out_page, KProcessAddress address) const {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(out_info != nullptr);
        MESOSPHERE_ASSERT(out_page != nullptr);

        const KMemoryBlock *block = m_memory_block_manager.FindBlock(address);
        R_UNLESS(block != nullptr, svc::ResultInvalidCurrentMemory());

        *out_info = block->GetMemoryInfo();
        out_page->flags = 0;
        R_SUCCEED();
    }

    Result KPageTableBase::QueryMappingImpl(KProcessAddress *out, KPhysicalAddress address, size_t size, ams::svc::MemoryState state) const {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(out != nullptr);

        const KProcessAddress region_start = this->GetRegionAddress(state);
        const size_t region_size           = this->GetRegionSize(state);

        /* Check that the address/size are potentially valid. */
        R_UNLESS((address < address + size), svc::ResultNotFound());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        auto &impl = this->GetImpl();

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   cur_entry  = { .phys_addr = Null<KPhysicalAddress>, .block_size = 0, .sw_reserved_bits = 0, .attr = 0 };
        bool             cur_valid  = false;
        TraversalEntry   next_entry;
        bool             next_valid;
        size_t           tot_size   = 0;

        next_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), region_start);
        next_entry.block_size = (next_entry.block_size - (GetInteger(region_start) & (next_entry.block_size - 1)));

        /* Iterate, looking for entry. */
        while (true) {
            if ((!next_valid && !cur_valid) || (next_valid && cur_valid && next_entry.phys_addr == cur_entry.phys_addr + cur_entry.block_size)) {
                cur_entry.block_size += next_entry.block_size;
            } else {
                if (cur_valid && cur_entry.phys_addr <= address && address + size <= cur_entry.phys_addr + cur_entry.block_size) {
                    /* Check if this region is valid. */
                    const KProcessAddress mapped_address = (region_start + tot_size) + (address - cur_entry.phys_addr);
                    if (R_SUCCEEDED(this->CheckMemoryState(mapped_address, size, KMemoryState_Mask, static_cast<KMemoryState>(util::ToUnderlying(state)), KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None))) {
                        /* It is! */
                        *out = mapped_address;
                        R_SUCCEED();
                    }
                }

                /* Update tracking variables. */
                tot_size += cur_entry.block_size;
                cur_entry = next_entry;
                cur_valid = next_valid;
            }

            if (cur_entry.block_size + tot_size >= region_size) {
                break;
            }

            next_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
        }

        /* Check the last entry. */
        R_UNLESS(cur_valid,                                                    svc::ResultNotFound());
        R_UNLESS(cur_entry.phys_addr <= address,                               svc::ResultNotFound());
        R_UNLESS(address + size <= cur_entry.phys_addr + cur_entry.block_size, svc::ResultNotFound());

        /* Check if the last region is valid. */
        const KProcessAddress mapped_address = (region_start + tot_size) + (address - cur_entry.phys_addr);
        R_TRY_CATCH(this->CheckMemoryState(mapped_address, size, KMemoryState_All, state, KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None)) {
            R_CONVERT_ALL(svc::ResultNotFound());
        } R_END_TRY_CATCH;

        /* We found the region. */
        *out = mapped_address;
        R_SUCCEED();
    }

    Result KPageTableBase::MapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Validate that the source address's state is valid. */
        KMemoryState src_state;
        size_t num_src_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(src_state), nullptr, nullptr, std::addressof(num_src_allocator_blocks), src_address, size, KMemoryState_FlagCanAlias, KMemoryState_FlagCanAlias, KMemoryPermission_All, KMemoryPermission_UserReadWrite, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Validate that the dst address's state is valid. */
        size_t num_dst_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator for the source. */
        Result src_allocator_result;
        KMemoryBlockManagerUpdateAllocator src_allocator(std::addressof(src_allocator_result), m_memory_block_slab_manager, num_src_allocator_blocks);
        R_TRY(src_allocator_result);

        /* Create an update allocator for the destination. */
        Result dst_allocator_result;
        KMemoryBlockManagerUpdateAllocator dst_allocator(std::addressof(dst_allocator_result), m_memory_block_slab_manager, num_dst_allocator_blocks);
        R_TRY(dst_allocator_result);

        /* Map the memory. */
        {
            /* Determine the number of pages being operated on. */
            const size_t num_pages = size / PageSize;

            /* Create page groups for the memory being unmapped. */
            KPageGroup pg(m_block_info_manager);

            /* Create the page group representing the source. */
            R_TRY(this->MakePageGroup(pg, src_address, num_pages));

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Reprotect the source as kernel-read/not mapped. */
            const KMemoryPermission new_src_perm = static_cast<KMemoryPermission>(KMemoryPermission_KernelRead | KMemoryPermission_NotMapped);
            const KMemoryAttribute  new_src_attr = KMemoryAttribute_Locked;
            const KPageProperties src_properties = { new_src_perm, false, false, DisableMergeAttribute_DisableHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, src_properties, OperationType_ChangePermissions, false));

            /* Ensure that we unprotect the source pages on failure. */
            ON_RESULT_FAILURE {
                const KPageProperties unprotect_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_EnableHeadBodyTail };
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, unprotect_properties, OperationType_ChangePermissions, true));
            };

            /* Map the alias pages. */
            const KPageProperties dst_map_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->MapPageGroupImpl(updater.GetPageList(), dst_address, pg, dst_map_properties, false));

            /* Apply the memory block updates. */
            m_memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages, src_state, new_src_perm, new_src_attr, KMemoryBlockDisableMergeAttribute_Locked, KMemoryBlockDisableMergeAttribute_None);
            m_memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages, KMemoryState_Stack, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);
        }

        R_SUCCEED();
    }

    Result KPageTableBase::UnmapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Validate that the source address's state is valid. */
        KMemoryState src_state;
        size_t num_src_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(src_state), nullptr, nullptr, std::addressof(num_src_allocator_blocks), src_address, size, KMemoryState_FlagCanAlias, KMemoryState_FlagCanAlias, KMemoryPermission_All, KMemoryPermission_NotMapped | KMemoryPermission_KernelRead, KMemoryAttribute_All, KMemoryAttribute_Locked));

        /* Validate that the dst address's state is valid. */
        KMemoryPermission dst_perm;
        size_t num_dst_allocator_blocks;
        R_TRY(this->CheckMemoryState(nullptr, std::addressof(dst_perm), nullptr, std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState_All, KMemoryState_Stack, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator for the source. */
        Result src_allocator_result;
        KMemoryBlockManagerUpdateAllocator src_allocator(std::addressof(src_allocator_result), m_memory_block_slab_manager, num_src_allocator_blocks);
        R_TRY(src_allocator_result);

        /* Create an update allocator for the destination. */
        Result dst_allocator_result;
        KMemoryBlockManagerUpdateAllocator dst_allocator(std::addressof(dst_allocator_result), m_memory_block_slab_manager, num_dst_allocator_blocks);
        R_TRY(dst_allocator_result);

        /* Unmap the memory. */
        {
            /* Determine the number of pages being operated on. */
            const size_t num_pages = size / PageSize;

            /* Create page groups for the memory being unmapped. */
            KPageGroup pg(m_block_info_manager);

            /* Create the page group representing the destination. */
            R_TRY(this->MakePageGroup(pg, dst_address, num_pages));

            /* Ensure the page group is the valid for the source. */
            R_UNLESS(this->IsValidPageGroup(pg, src_address, num_pages), svc::ResultInvalidMemoryRegion());

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Unmap the aliased copy of the pages. */
            const KPageProperties dst_unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), dst_address, num_pages, Null<KPhysicalAddress>, false, dst_unmap_properties, OperationType_Unmap, false));

            /* Ensure that we re-map the aliased pages on failure. */
            ON_RESULT_FAILURE {
                this->RemapPageGroup(updater.GetPageList(), dst_address, size, pg);
            };

            /* Try to set the permissions for the source pages back to what they should be. */
            const KPageProperties src_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_EnableAndMergeHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, src_properties, OperationType_ChangePermissions, false));

            /* Apply the memory block updates. */
            m_memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages, src_state,         KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Locked);
            m_memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages, KMemoryState_None, KMemoryPermission_None,          KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);
        }

        R_SUCCEED();
    }

    Result KPageTableBase::MapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
        /* Validate the mapping request. */
        R_UNLESS(this->CanContain(dst_address, size, KMemoryState_AliasCode), svc::ResultInvalidMemoryRegion());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Verify that the source memory is normal heap. */
        KMemoryState src_state;
        KMemoryPermission src_perm;
        size_t num_src_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(src_state), std::addressof(src_perm), nullptr, std::addressof(num_src_allocator_blocks), src_address, size, KMemoryState_All, KMemoryState_Normal, KMemoryPermission_All, KMemoryPermission_UserReadWrite, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Verify that the destination memory is unmapped. */
        size_t num_dst_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator for the source. */
        Result src_allocator_result;
        KMemoryBlockManagerUpdateAllocator src_allocator(std::addressof(src_allocator_result), m_memory_block_slab_manager, num_src_allocator_blocks);
        R_TRY(src_allocator_result);

        /* Create an update allocator for the destination. */
        Result dst_allocator_result;
        KMemoryBlockManagerUpdateAllocator dst_allocator(std::addressof(dst_allocator_result), m_memory_block_slab_manager, num_dst_allocator_blocks);
        R_TRY(dst_allocator_result);

        /* Map the code memory. */
        {
            /* Determine the number of pages being operated on. */
            const size_t num_pages = size / PageSize;

            /* Create page groups for the memory being unmapped. */
            KPageGroup pg(m_block_info_manager);

            /* Create the page group representing the source. */
            R_TRY(this->MakePageGroup(pg, src_address, num_pages));

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Reprotect the source as kernel-read/not mapped. */
            const KMemoryPermission new_perm = static_cast<KMemoryPermission>(KMemoryPermission_KernelRead | KMemoryPermission_NotMapped);
            const KPageProperties src_properties = { new_perm, false, false, DisableMergeAttribute_DisableHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, src_properties, OperationType_ChangePermissions, false));

            /* Ensure that we unprotect the source pages on failure. */
            ON_RESULT_FAILURE {
                const KPageProperties unprotect_properties = { src_perm, false, false, DisableMergeAttribute_EnableHeadBodyTail };
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, unprotect_properties, OperationType_ChangePermissions, true));
            };

            /* Map the alias pages. */
            const KPageProperties dst_properties = { new_perm, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->MapPageGroupImpl(updater.GetPageList(), dst_address, pg, dst_properties, false));

            /* Apply the memory block updates. */
            m_memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages, src_state,              new_perm, KMemoryAttribute_Locked, KMemoryBlockDisableMergeAttribute_Locked, KMemoryBlockDisableMergeAttribute_None);
            m_memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages, KMemoryState_AliasCode, new_perm, KMemoryAttribute_None,   KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);
        }

        R_SUCCEED();
    }

    Result KPageTableBase::UnmapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
        /* Validate the mapping request. */
        R_UNLESS(this->CanContain(dst_address, size, KMemoryState_AliasCode), svc::ResultInvalidMemoryRegion());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Verify that the source memory is locked normal heap. */
        size_t num_src_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_src_allocator_blocks), src_address, size, KMemoryState_All, KMemoryState_Normal, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_Locked));

        /* Verify that the destination memory is aliasable code. */
        size_t num_dst_allocator_blocks;
        R_TRY(this->CheckMemoryStateContiguous(std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState_FlagCanCodeAlias, KMemoryState_FlagCanCodeAlias, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All & ~KMemoryAttribute_PermissionLocked, KMemoryAttribute_None));

        /* Determine whether any pages being unmapped are code. */
        bool any_code_pages = false;
        {
            KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(dst_address);
            while (true) {
                /* Check if the memory has code flag. */
                if ((it->GetState() & KMemoryState_FlagCode) != 0) {
                    any_code_pages = true;
                    break;
                }

                /* Check if we're done. */
                if (dst_address + size - 1 <= it->GetLastAddress()) {
                    break;
                }

                /* Advance. */
                ++it;
            }
        }

        /* Ensure that we maintain the instruction cache. */
        bool reprotected_pages = false;
        ON_SCOPE_EXIT {
            if (reprotected_pages && any_code_pages) {
                cpu::InvalidateEntireInstructionCache();
            }
        };

        /* Unmap. */
        {
            /* Determine the number of pages being operated on. */
            const size_t num_pages = size / PageSize;

            /* Create page groups for the memory being unmapped. */
            KPageGroup pg(m_block_info_manager);

            /* Create the page group representing the destination. */
            R_TRY(this->MakePageGroup(pg, dst_address, num_pages));

            /* Verify that the page group contains the same pages as the source. */
            R_UNLESS(this->IsValidPageGroup(pg, src_address, num_pages), svc::ResultInvalidMemoryRegion());

            /* Create an update allocator for the source. */
            Result src_allocator_result;
            KMemoryBlockManagerUpdateAllocator src_allocator(std::addressof(src_allocator_result), m_memory_block_slab_manager, num_src_allocator_blocks);
            R_TRY(src_allocator_result);

            /* Create an update allocator for the destination. */
            Result dst_allocator_result;
            KMemoryBlockManagerUpdateAllocator dst_allocator(std::addressof(dst_allocator_result), m_memory_block_slab_manager, num_dst_allocator_blocks);
            R_TRY(dst_allocator_result);

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Unmap the aliased copy of the pages. */
            const KPageProperties dst_unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), dst_address, num_pages, Null<KPhysicalAddress>, false, dst_unmap_properties, OperationType_Unmap, false));

            /* Ensure that we re-map the aliased pages on failure. */
            ON_RESULT_FAILURE {
                this->RemapPageGroup(updater.GetPageList(), dst_address, size, pg);
            };

            /* Try to set the permissions for the source pages back to what they should be. */
            const KPageProperties src_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_EnableAndMergeHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, src_properties, OperationType_ChangePermissions, false));

            /* Apply the memory block updates. */
            m_memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages, KMemoryState_None,   KMemoryPermission_None,          KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);
            m_memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages, KMemoryState_Normal, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Locked);

            /* Note that we reprotected pages. */
            reprotected_pages = true;
        }

        R_SUCCEED();
    }

    Result KPageTableBase::MapInsecurePhysicalMemory(KProcessAddress address, size_t size) {
        /* Get the insecure memory resource limit and pool. */
        auto * const insecure_resource_limit = KSystemControl::GetInsecureMemoryResourceLimit();
        const auto insecure_pool             = static_cast<KMemoryManager::Pool>(KSystemControl::GetInsecureMemoryPool());

        /* Reserve the insecure memory. */
        /* NOTE: ResultOutOfMemory is returned here instead of the usual LimitReached. */
        KScopedResourceReservation memory_reservation(insecure_resource_limit, ams::svc::LimitableResource_PhysicalMemoryMax, size);
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultOutOfMemory());

        /* Allocate pages for the insecure memory. */
        KPageGroup pg(m_block_info_manager);
        R_TRY(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(pg), size / PageSize, 1, KMemoryManager::EncodeOption(insecure_pool, KMemoryManager::Direction_FromFront)));

        /* Close the opened pages when we're done with them. */
        /* If the mapping succeeds, each page will gain an extra reference, otherwise they will be freed automatically. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Clear all the newly allocated pages. */
        for (const auto &it : pg) {
            std::memset(GetVoidPointer(GetHeapVirtualAddress(it.GetAddress())), m_heap_fill_value, it.GetSize());
        }

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Validate that the address's state is valid. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Map the pages. */
        const size_t num_pages = size / PageSize;
        const KPageProperties map_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, pg, map_properties, OperationType_MapGroup, false));

        /* Apply the memory block update. */
        m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState_Insecure, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* Update our mapped insecure size. */
        m_mapped_insecure_memory += size;

        /* Commit the memory reservation. */
        memory_reservation.Commit();

        /* We succeeded. */
        R_SUCCEED();
    }

    Result KPageTableBase::UnmapInsecurePhysicalMemory(KProcessAddress address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, KMemoryState_Insecure, KMemoryPermission_All, KMemoryPermission_UserReadWrite, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Unmap the memory. */
        const size_t num_pages = size / PageSize;
        const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

        /* Apply the memory block update. */
        m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        /* Update our mapped insecure size. */
        m_mapped_insecure_memory -= size;

        /* Release the insecure memory from the insecure limit. */
        if (auto * const insecure_resource_limit = KSystemControl::GetInsecureMemoryResourceLimit(); insecure_resource_limit != nullptr) {
            insecure_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, size);
        }

        R_SUCCEED();
    }

    KProcessAddress KPageTableBase::FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const {
        KProcessAddress address = Null<KProcessAddress>;

        KProcessAddress search_start = Null<KProcessAddress>;
        KProcessAddress search_end   = Null<KProcessAddress>;
        if (m_memory_block_manager.GetRegionForFindFreeArea(std::addressof(search_start), std::addressof(search_end), region_start, region_num_pages, num_pages, alignment, offset, guard_pages)) {
            if (this->IsAslrEnabled()) {
                /* Try to directly find a free area up to 8 times. */
                for (size_t i = 0; i < 8; i++) {
                    const size_t random_offset = KSystemControl::GenerateRandomRange(0, (search_end - search_start) / alignment) * alignment;
                    const KProcessAddress candidate = search_start + random_offset;

                    KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(candidate);
                    MESOSPHERE_ABORT_UNLESS(it != m_memory_block_manager.end());

                    if (it->GetState() != KMemoryState_Free) { continue; }
                    if (!(it->GetAddress() + guard_pages * PageSize <= GetInteger(candidate))) { continue; }
                    if (!(candidate + (num_pages + guard_pages) * PageSize - 1 <= it->GetLastAddress())) { continue; }

                    address = candidate;
                    break;
                }

                /* Fall back to finding the first free area with a random offset. */
                if (address == Null<KProcessAddress>) {
                    /* NOTE: Nintendo does not account for guard pages here. */
                    /* This may theoretically cause an offset to be chosen that cannot be mapped. */
                    /* We will account for guard pages. */
                    const size_t offset_blocks = KSystemControl::GenerateRandomRange(0, (search_end - search_start) / alignment);
                    const auto   region_end    = region_start + region_num_pages * PageSize;
                    address = m_memory_block_manager.FindFreeArea(search_start + offset_blocks * alignment, (region_end - (search_start + offset_blocks * alignment)) / PageSize, num_pages, alignment, offset, guard_pages);
                }
            }

            /* Find the first free area. */
            if (address == Null<KProcessAddress>) {
                address = m_memory_block_manager.FindFreeArea(region_start, region_num_pages, num_pages, alignment, offset, guard_pages);
            }
        }

        return address;
    }

    size_t KPageTableBase::GetSize(KMemoryState state) const {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Iterate, counting blocks with the desired state. */
        size_t total_size = 0;
        for (KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(m_address_space_start); it != m_memory_block_manager.end(); ++it) {
            if (it->GetState() == state) {
                total_size += it->GetSize();
            }
        }

        return total_size;
    }

    size_t KPageTableBase::GetCodeSize() const {
        return this->GetSize(KMemoryState_Code);
    }

    size_t KPageTableBase::GetCodeDataSize() const {
        return this->GetSize(KMemoryState_CodeData);
    }

    size_t KPageTableBase::GetAliasCodeSize() const {
        return this->GetSize(KMemoryState_AliasCode);
    }

    size_t KPageTableBase::GetAliasCodeDataSize() const {
        return this->GetSize(KMemoryState_AliasCodeData);
    }

    Result KPageTableBase::AllocateAndMapPagesImpl(PageLinkedList *page_list, KProcessAddress address, size_t num_pages, const KPageProperties &properties) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Create a page group to hold the pages we allocate. */
        KPageGroup pg(m_block_info_manager);

        /* Allocate the pages. */
        R_TRY(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(pg), num_pages, 1, m_allocate_option));

        /* Ensure that the page group is closed when we're done working with it. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Clear all pages. */
        for (const auto &it : pg) {
            std::memset(GetVoidPointer(GetHeapVirtualAddress(it.GetAddress())), m_heap_fill_value, it.GetSize());
        }

        /* Map the pages. */
        R_RETURN(this->Operate(page_list, address, num_pages, pg, properties, OperationType_MapGroup, false));
    }

    Result KPageTableBase::MapPageGroupImpl(PageLinkedList *page_list, KProcessAddress address, const KPageGroup &pg, const KPageProperties properties, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Note the current address, so that we can iterate. */
        const KProcessAddress start_address = address;
        KProcessAddress cur_address = address;

        /* Ensure that we clean up on failure. */
        ON_RESULT_FAILURE {
            MESOSPHERE_ABORT_UNLESS(!reuse_ll);
            if (cur_address != start_address) {
                const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(page_list, start_address, (cur_address - start_address) / PageSize, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
            }
        };

        /* Iterate, mapping all pages in the group. */
        for (const auto &block : pg) {
            /* Map and advance. */
            const KPageProperties cur_properties = (cur_address == start_address) ? properties : KPageProperties{ properties.perm, properties.io, properties.uncached, DisableMergeAttribute_None };
            R_TRY(this->Operate(page_list, cur_address, block.GetNumPages(), block.GetAddress(), true, cur_properties, OperationType_Map, reuse_ll));
            cur_address += block.GetSize();
        }

        /* We succeeded! */
        R_SUCCEED();
    }

    void KPageTableBase::RemapPageGroup(PageLinkedList *page_list, KProcessAddress address, size_t size, const KPageGroup &pg) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Note the current address, so that we can iterate. */
        const KProcessAddress start_address = address;
        const KProcessAddress last_address  = start_address + size - 1;
        const KProcessAddress end_address   = last_address + 1;

        /* Iterate over the memory. */
        auto pg_it = pg.begin();
        MESOSPHERE_ABORT_UNLESS(pg_it != pg.end());

        KPhysicalAddress pg_phys_addr = pg_it->GetAddress();
        size_t pg_pages = pg_it->GetNumPages();

        auto it = m_memory_block_manager.FindIterator(start_address);
        while (true) {
            /* Check that the iterator is valid. */
            MESOSPHERE_ASSERT(it != m_memory_block_manager.end());

            /* Determine the range to map. */
                  KProcessAddress map_address     = std::max(GetInteger(it->GetAddress()), GetInteger(start_address));
            const KProcessAddress map_end_address = std::min(GetInteger(it->GetEndAddress()), GetInteger(end_address));
            MESOSPHERE_ABORT_UNLESS(map_end_address != map_address);

            /* Determine if we should disable head merge. */
            const bool disable_head_merge = it->GetAddress() >= GetInteger(start_address) && (it->GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute_Normal) != 0;
            const KPageProperties map_properties = { it->GetPermission(), false, false, disable_head_merge ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };

            /* While we have pages to map, map them. */
            size_t map_pages = (map_end_address - map_address) / PageSize;
            while (map_pages > 0) {
                /* Check if we're at the end of the physical block. */
                if (pg_pages == 0) {
                    /* Ensure there are more pages to map. */
                    MESOSPHERE_ABORT_UNLESS(pg_it != pg.end());

                    /* Advance our physical block. */
                    ++pg_it;
                    pg_phys_addr = pg_it->GetAddress();
                    pg_pages     = pg_it->GetNumPages();
                }

                /* Map whatever we can. */
                const size_t cur_pages = std::min(pg_pages, map_pages);
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(page_list, map_address, map_pages, pg_phys_addr, true, map_properties, OperationType_Map, true));

                /* Advance. */
                map_address += cur_pages * PageSize;
                map_pages   -= cur_pages;

                pg_phys_addr += cur_pages * PageSize;
                pg_pages     -= cur_pages;
            }

            /* Check if we're done. */
            if (last_address <= it->GetLastAddress()) {
                break;
            }

            /* Advance. */
            ++it;
        }

        /* Check that we re-mapped precisely the page group. */
        MESOSPHERE_ABORT_UNLESS((++pg_it) == pg.end());
    }

    Result KPageTableBase::MakePageGroup(KPageGroup &pg, KProcessAddress addr, size_t num_pages) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        const size_t size = num_pages * PageSize;

        /* We're making a new group, not adding to an existing one. */
        R_UNLESS(pg.empty(), svc::ResultInvalidCurrentMemory());

        auto &impl = this->GetImpl();

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   next_entry;
        R_UNLESS(impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), addr), svc::ResultInvalidCurrentMemory());

        /* Prepare tracking variables. */
        KPhysicalAddress cur_addr = next_entry.phys_addr;
        size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
        size_t tot_size = cur_size;

        /* Iterate, adding to group as we go. */
        while (tot_size < size) {
            R_UNLESS(impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context)), svc::ResultInvalidCurrentMemory());

            if (next_entry.phys_addr != (cur_addr + cur_size)) {
                const size_t cur_pages = cur_size / PageSize;

                R_UNLESS(IsHeapPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());
                R_TRY(pg.AddBlock(cur_addr, cur_pages));

                cur_addr           = next_entry.phys_addr;
                cur_size           = next_entry.block_size;
            } else {
                cur_size += next_entry.block_size;
            }

            tot_size += next_entry.block_size;
        }

        /* Ensure we add the right amount for the last block. */
        if (tot_size > size) {
            cur_size -= (tot_size - size);
        }

        /* add the last block. */
        const size_t cur_pages = cur_size / PageSize;
        R_UNLESS(IsHeapPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());
        R_TRY(pg.AddBlock(cur_addr, cur_pages));

        R_SUCCEED();
    }

    bool KPageTableBase::IsValidPageGroup(const KPageGroup &pg, KProcessAddress addr, size_t num_pages) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        const size_t size = num_pages * PageSize;

        /* Empty groups are necessarily invalid. */
        if (pg.empty()) {
            return false;
        }

        auto &impl = this->GetImpl();

        /* We're going to validate that the group we'd expect is the group we see. */
        auto cur_it = pg.begin();
        KPhysicalAddress cur_block_address = cur_it->GetAddress();
        size_t cur_block_pages = cur_it->GetNumPages();

        auto UpdateCurrentIterator = [&]() ALWAYS_INLINE_LAMBDA {
            if (cur_block_pages == 0) {
                if ((++cur_it) == pg.end()) {
                    return false;
                }

                cur_block_address = cur_it->GetAddress();
                cur_block_pages   = cur_it->GetNumPages();
            }
            return true;
        };

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   next_entry;
        if (!impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), addr)) {
            return false;
        }

        /* Prepare tracking variables. */
        KPhysicalAddress cur_addr = next_entry.phys_addr;
        size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
        size_t tot_size = cur_size;

        /* Iterate, comparing expected to actual. */
        while (tot_size < size) {
            if (!impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context))) {
                return false;
            }

            if (next_entry.phys_addr != (cur_addr + cur_size)) {
                const size_t cur_pages = cur_size / PageSize;

                if (!IsHeapPhysicalAddress(cur_addr)) {
                    return false;
                }

                if (!UpdateCurrentIterator()) {
                    return false;
                }

                if (cur_block_address != cur_addr || cur_block_pages < cur_pages) {
                    return false;
                }

                cur_block_address += cur_size;
                cur_block_pages   -= cur_pages;
                cur_addr           = next_entry.phys_addr;
                cur_size           = next_entry.block_size;
            } else {
                cur_size += next_entry.block_size;
            }

            tot_size += next_entry.block_size;
        }

        /* Ensure we compare the right amount for the last block. */
        if (tot_size > size) {
            cur_size -= (tot_size - size);
        }

        if (!IsHeapPhysicalAddress(cur_addr)) {
            return false;
        }

        if (!UpdateCurrentIterator()) {
            return false;
        }

        return cur_block_address == cur_addr && cur_block_pages == (cur_size / PageSize);
    }

    Result KPageTableBase::GetContiguousMemoryRangeWithState(MemoryRange *out, KProcessAddress address, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        auto &impl = this->GetImpl();

        /* Begin a traversal. */
        TraversalContext context;
        TraversalEntry cur_entry = { .phys_addr = Null<KPhysicalAddress>, .block_size = 0, .sw_reserved_bits = 0, .attr = 0 };
        R_UNLESS(impl.BeginTraversal(std::addressof(cur_entry), std::addressof(context), address), svc::ResultInvalidCurrentMemory());

        /* Traverse until we have enough size or we aren't contiguous any more. */
        const KPhysicalAddress phys_address = cur_entry.phys_addr;
        const u8 entry_attr = cur_entry.attr;
        size_t contig_size;
        for (contig_size = cur_entry.block_size - (GetInteger(phys_address) & (cur_entry.block_size - 1)); contig_size < size; contig_size += cur_entry.block_size) {
            if (!impl.ContinueTraversal(std::addressof(cur_entry), std::addressof(context))) {
                break;
            }
            if (cur_entry.phys_addr != phys_address + contig_size) {
                break;
            }
            if (cur_entry.attr != entry_attr) {
                break;
            }
        }

        /* Take the minimum size for our region. */
        size = std::min(size, contig_size);

        /* Check that the memory is contiguous (modulo the reference count bit). */
        const u32 test_state_mask = state_mask | KMemoryState_FlagReferenceCounted;
        const bool is_heap = R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, test_state_mask, state | KMemoryState_FlagReferenceCounted, perm_mask, perm, attr_mask, attr));
        if (!is_heap) {
            R_TRY(this->CheckMemoryStateContiguous(address, size, test_state_mask, state, perm_mask, perm, attr_mask, attr));
        }

        /* The memory is contiguous, so set the output range. */
        out->Set(phys_address, size, is_heap, attr);
        R_SUCCEED();
    }

    Result KPageTableBase::SetMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission svc_perm) {
        const size_t num_pages = size / PageSize;

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Verify we can change the memory permission. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), nullptr, std::addressof(num_allocator_blocks), addr, size, KMemoryState_FlagCanReprotect, KMemoryState_FlagCanReprotect, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Determine new perm. */
        const KMemoryPermission new_perm = ConvertToKMemoryPermission(svc_perm);
        R_SUCCEED_IF(old_perm == new_perm);

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { new_perm, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, new_perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);

        R_SUCCEED();
    }

    Result KPageTableBase::SetProcessMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission svc_perm) {
        const size_t num_pages = size / PageSize;

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Verify we can change the memory permission. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), nullptr, std::addressof(num_allocator_blocks), addr, size, KMemoryState_FlagCode, KMemoryState_FlagCode, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Make a new page group for the region. */
        KPageGroup pg(m_block_info_manager);

        /* Determine new perm/state. */
        const KMemoryPermission new_perm = ConvertToKMemoryPermission(svc_perm);
        KMemoryState new_state = old_state;
        const bool is_w  = (new_perm & KMemoryPermission_UserWrite)   == KMemoryPermission_UserWrite;
        const bool is_x  = (new_perm & KMemoryPermission_UserExecute) == KMemoryPermission_UserExecute;
        const bool was_x = (old_perm & KMemoryPermission_UserExecute) == KMemoryPermission_UserExecute;
        MESOSPHERE_ASSERT(!(is_w && is_x));

        if (is_w) {
            switch (old_state) {
                case KMemoryState_Code:      new_state = KMemoryState_CodeData;      break;
                case KMemoryState_AliasCode: new_state = KMemoryState_AliasCodeData; break;
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        }

        /* Create a page group, if we're setting execute permissions. */
        if (is_x) {
            R_TRY(this->MakePageGroup(pg, GetInteger(addr), num_pages));
        }

        /* Succeed if there's nothing to do. */
        R_SUCCEED_IF(old_perm == new_perm && old_state == new_state);

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* If we're creating an executable mapping, take and immediately release the scheduler lock. This will force a reschedule. */
        if (is_x) {
            KScopedSchedulerLock sl;
        }

        /* Ensure cache coherency, if we're setting pages as executable. */
        if (is_x) {
            for (const auto &block : pg) {
                cpu::StoreDataCache(GetVoidPointer(GetHeapVirtualAddress(block.GetAddress())), block.GetSize());
            }
            cpu::InvalidateEntireInstructionCache();
        }

        /* Perform mapping operation. */
        const KPageProperties properties = { new_perm, false, false, DisableMergeAttribute_None };
        const auto operation = was_x ? OperationType_ChangePermissionsAndRefresh : OperationType_ChangePermissions;
        R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, operation, false));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, new_state, new_perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);

        /* Ensure cache coherency, if we're setting pages as executable. */
        if (was_x) {
            cpu::InvalidateEntireInstructionCache();
        }

        R_SUCCEED();
    }

    Result KPageTableBase::SetMemoryAttribute(KProcessAddress addr, size_t size, u32 mask, u32 attr) {
        const size_t num_pages = size / PageSize;
        MESOSPHERE_ASSERT((mask | KMemoryAttribute_SetMask) == KMemoryAttribute_SetMask);

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Verify we can change the memory attribute. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        KMemoryAttribute old_attr;
        size_t num_allocator_blocks;
        constexpr u32 AttributeTestMask = ~(KMemoryAttribute_SetMask | KMemoryAttribute_DeviceShared);
        const u32 state_test_mask = ((mask & KMemoryAttribute_Uncached) ? static_cast<u32>(KMemoryState_FlagCanChangeAttribute) : 0) | ((mask & KMemoryAttribute_PermissionLocked) ? static_cast<u32>(KMemoryState_FlagCanPermissionLock) : 0);
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), std::addressof(old_attr), std::addressof(num_allocator_blocks),
                                     addr, size,
                                     state_test_mask, state_test_mask,
                                     KMemoryPermission_None, KMemoryPermission_None,
                                     AttributeTestMask, KMemoryAttribute_None, ~AttributeTestMask));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* If we need to, perform a change attribute operation. */
        if ((mask & KMemoryAttribute_Uncached) != 0) {
            /* Determine the new attribute. */
            const KMemoryAttribute new_attr = static_cast<KMemoryAttribute>(((old_attr & ~mask) | (attr & mask)));

            /* Perform operation. */
            const KPageProperties properties = { old_perm, false, (new_attr & KMemoryAttribute_Uncached) != 0, DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissionsAndRefreshAndFlush, false));
        }

        /* Update the blocks. */
        m_memory_block_manager.UpdateAttribute(std::addressof(allocator), addr, num_pages, mask, attr);

        R_SUCCEED();
    }

    Result KPageTableBase::SetHeapSize(KProcessAddress *out, size_t size) {
        /* Lock the physical memory mutex. */
        KScopedLightLock map_phys_mem_lk(m_map_physical_memory_lock);

        /* Try to perform a reduction in heap, instead of an extension. */
        KProcessAddress cur_address;
        size_t allocation_size;
        {
            /* Lock the table. */
            KScopedLightLock lk(m_general_lock);

            /* Validate that setting heap size is possible at all. */
            R_UNLESS(!m_is_kernel,                                                                                   svc::ResultOutOfMemory());
            R_UNLESS(size <= static_cast<size_t>(m_region_ends[RegionType_Heap] - m_region_starts[RegionType_Heap]), svc::ResultOutOfMemory());
            R_UNLESS(size <= m_max_heap_size,                                                                        svc::ResultOutOfMemory());

            if (size < static_cast<size_t>(m_current_heap_end - m_region_starts[RegionType_Heap])) {
                /* The size being requested is less than the current size, so we need to free the end of the heap. */

                /* Validate memory state. */
                size_t num_allocator_blocks;
                R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks),
                                             m_region_starts[RegionType_Heap] + size, (m_current_heap_end - m_region_starts[RegionType_Heap]) - size,
                                             KMemoryState_All, KMemoryState_Normal,
                                             KMemoryPermission_All, KMemoryPermission_UserReadWrite,
                                             KMemoryAttribute_All,  KMemoryAttribute_None));

                /* Create an update allocator. */
                Result allocator_result;
                KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
                R_TRY(allocator_result);

                /* We're going to perform an update, so create a helper. */
                KScopedPageTableUpdater updater(this);

                /* Unmap the end of the heap. */
                const size_t num_pages = ((m_current_heap_end - m_region_starts[RegionType_Heap]) - size) / PageSize;
                const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                R_TRY(this->Operate(updater.GetPageList(), m_region_starts[RegionType_Heap] + size, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

                /* Release the memory from the resource limit. */
                m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, num_pages * PageSize);

                /* Apply the memory block update. */
                m_memory_block_manager.Update(std::addressof(allocator), m_region_starts[RegionType_Heap] + size, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, size == 0 ? KMemoryBlockDisableMergeAttribute_Normal : KMemoryBlockDisableMergeAttribute_None);

                /* Update the current heap end. */
                m_current_heap_end = m_region_starts[RegionType_Heap] + size;

                /* Set the output. */
                *out = m_region_starts[RegionType_Heap];
                R_SUCCEED();
            } else if (size == static_cast<size_t>(m_current_heap_end - m_region_starts[RegionType_Heap])) {
                /* The size requested is exactly the current size. */
                *out = m_region_starts[RegionType_Heap];
                R_SUCCEED();
            } else {
                /* We have to allocate memory. Determine how much to allocate and where while the table is locked. */
                cur_address     = m_current_heap_end;
                allocation_size = size - (m_current_heap_end - m_region_starts[RegionType_Heap]);
            }
        }

        /* Reserve memory for the heap extension. */
        KScopedResourceReservation memory_reservation(m_resource_limit, ams::svc::LimitableResource_PhysicalMemoryMax, allocation_size);
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

        /* Allocate pages for the heap extension. */
        KPageGroup pg(m_block_info_manager);
        R_TRY(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(pg), allocation_size / PageSize, 1, m_allocate_option));

        /* Close the opened pages when we're done with them. */
        /* If the mapping succeeds, each page will gain an extra reference, otherwise they will be freed automatically. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Clear all the newly allocated pages. */
        for (const auto &it : pg) {
            std::memset(GetVoidPointer(GetHeapVirtualAddress(it.GetAddress())), m_heap_fill_value, it.GetSize());
        }

        /* Map the pages. */
        {
            /* Lock the table. */
            KScopedLightLock lk(m_general_lock);

            /* Ensure that the heap hasn't changed since we began executing. */
            MESOSPHERE_ABORT_UNLESS(cur_address == m_current_heap_end);

            /* Check the memory state. */
            size_t num_allocator_blocks;
            R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), m_current_heap_end, allocation_size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

            /* Create an update allocator. */
            Result allocator_result;
            KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
            R_TRY(allocator_result);

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Map the pages. */
            const size_t num_pages = allocation_size / PageSize;
            const KPageProperties map_properties = { KMemoryPermission_UserReadWrite, false, false, (m_current_heap_end == m_region_starts[RegionType_Heap]) ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), m_current_heap_end, num_pages, pg, map_properties, OperationType_MapGroup, false));

            /* We succeeded, so commit our memory reservation. */
            memory_reservation.Commit();

            /* Apply the memory block update. */
            m_memory_block_manager.Update(std::addressof(allocator), m_current_heap_end, num_pages, KMemoryState_Normal, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, m_region_starts[RegionType_Heap] == m_current_heap_end ? KMemoryBlockDisableMergeAttribute_Normal : KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);

            /* Update the current heap end. */
            m_current_heap_end = m_region_starts[RegionType_Heap] + size;

            /* Set the output. */
            *out = m_region_starts[RegionType_Heap];
            R_SUCCEED();
        }
    }

    Result KPageTableBase::SetMaxHeapSize(size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Only process page tables are allowed to set heap size. */
        MESOSPHERE_ASSERT(!this->IsKernel());

        m_max_heap_size = size;

        R_SUCCEED();
    }

    Result KPageTableBase::QueryInfo(KMemoryInfo *out_info, ams::svc::PageInfo *out_page_info, KProcessAddress addr) const {
        /* If the address is invalid, create a fake block. */
        if (!this->Contains(addr, 1)) {
            *out_info = {
                .m_address                          = GetInteger(m_address_space_end),
                .m_size                             = 0 - GetInteger(m_address_space_end),
                .m_state                            = static_cast<KMemoryState>(ams::svc::MemoryState_Inaccessible),
                .m_device_disable_merge_left_count  = 0,
                .m_device_disable_merge_right_count = 0,
                .m_ipc_lock_count                   = 0,
                .m_device_use_count                 = 0,
                .m_ipc_disable_merge_count          = 0,
                .m_permission                       = KMemoryPermission_None,
                .m_attribute                        = KMemoryAttribute_None,
                .m_original_permission              = KMemoryPermission_None,
                .m_disable_merge_attribute          = KMemoryBlockDisableMergeAttribute_None,
            };
            out_page_info->flags = 0;

            R_SUCCEED();
        }

        /* Otherwise, lock the table and query. */
        KScopedLightLock lk(m_general_lock);
        R_RETURN(this->QueryInfoImpl(out_info, out_page_info, addr));
    }

    Result KPageTableBase::QueryPhysicalAddress(ams::svc::PhysicalMemoryInfo *out, KProcessAddress address) const {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Align the address down to page size. */
        address = util::AlignDown(GetInteger(address), PageSize);

        /* Verify that we can query the address. */
        KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(address);
        R_UNLESS(it != m_memory_block_manager.end(), svc::ResultInvalidCurrentMemory());

        /* Check the memory state. */
        R_TRY(this->CheckMemoryState(it, KMemoryState_FlagCanQueryPhysical, KMemoryState_FlagCanQueryPhysical, KMemoryPermission_UserReadExecute, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Prepare to traverse. */
        KPhysicalAddress phys_addr;
        size_t phys_size;

        KProcessAddress virt_addr = it->GetAddress();
        KProcessAddress end_addr  = it->GetEndAddress();

        /* Perform traversal. */
        {
            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   next_entry;
            bool traverse_valid = m_impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), virt_addr);
            R_UNLESS(traverse_valid, svc::ResultInvalidCurrentMemory());

            /* Set tracking variables. */
            phys_addr = next_entry.phys_addr;
            phys_size = next_entry.block_size - (GetInteger(phys_addr) & (next_entry.block_size - 1));

            /* Iterate. */
            while (true) {
                /* Continue the traversal. */
                traverse_valid = m_impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                if (!traverse_valid) {
                    break;
                }

                if (next_entry.phys_addr != (phys_addr + phys_size)) {
                    /* Check if we're done. */
                    if (virt_addr <= address && address <= virt_addr + phys_size - 1) {
                        break;
                    }

                    /* Advance. */
                    phys_addr  = next_entry.phys_addr;
                    virt_addr += next_entry.block_size;
                    phys_size  = next_entry.block_size - (GetInteger(phys_addr) & (next_entry.block_size - 1));
                } else {
                    phys_size += next_entry.block_size;
                }

                /* Check if we're done. */
                if (end_addr < virt_addr + phys_size) {
                    break;
                }
            }
            MESOSPHERE_ASSERT(virt_addr <= address && address <= virt_addr + phys_size - 1);

            /* Ensure we use the right size. */
            if (end_addr < virt_addr + phys_size) {
                phys_size = end_addr - virt_addr;
            }
        }

        /* Set the output. */
        out->physical_address = GetInteger(phys_addr);
        out->virtual_address  = GetInteger(virt_addr);
        out->size             = phys_size;
        R_SUCCEED();
    }

    Result KPageTableBase::MapIoImpl(KProcessAddress *out, PageLinkedList *page_list, KPhysicalAddress phys_addr, size_t size, KMemoryState state, KMemoryPermission perm) {
        /* Check pre-conditions. */
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(size,                  PageSize));
        MESOSPHERE_ASSERT(size > 0);

        R_UNLESS(phys_addr < phys_addr + size, svc::ResultInvalidAddress());
        const size_t num_pages = size / PageSize;
        const KPhysicalAddress last = phys_addr + size - 1;

        /* Get region extents. */
        const KProcessAddress region_start     = m_region_starts[RegionType_KernelMap];
        const size_t          region_size      = m_region_ends[RegionType_KernelMap] - m_region_starts[RegionType_KernelMap];
        const size_t          region_num_pages = region_size / PageSize;

        MESOSPHERE_ASSERT(this->CanContain(region_start, region_size, state));

        /* Locate the memory region. */
        const KMemoryRegion *region = KMemoryLayout::Find(phys_addr);
        R_UNLESS(region != nullptr, svc::ResultInvalidAddress());

        MESOSPHERE_ASSERT(region->Contains(GetInteger(phys_addr)));

        /* Ensure that the region is mappable. */
        const bool is_rw = perm == KMemoryPermission_UserReadWrite;
        while (true) {
            /* Check that the region exists. */
            R_UNLESS(region != nullptr, svc::ResultInvalidAddress());

            /* Check the region attributes. */
            R_UNLESS(!region->IsDerivedFrom(KMemoryRegionType_Dram),                      svc::ResultInvalidAddress());
            R_UNLESS(!region->HasTypeAttribute(KMemoryRegionAttr_UserReadOnly) || !is_rw, svc::ResultInvalidAddress());
            R_UNLESS(!region->HasTypeAttribute(KMemoryRegionAttr_NoUserMap),              svc::ResultInvalidAddress());

            /* Check if we're done. */
            if (GetInteger(last) <= region->GetLastAddress()) {
                break;
            }

            /* Advance. */
            region = region->GetNext();
        };

        /* Select an address to map at. */
        KProcessAddress addr = Null<KProcessAddress>;
        for (s32 block_type = KPageTable::GetMaxBlockType(); block_type >= 0; block_type--) {
            const size_t alignment = KPageTable::GetBlockSize(static_cast<KPageTable::BlockType>(block_type));

            const KPhysicalAddress aligned_phys = util::AlignUp(GetInteger(phys_addr), alignment) + alignment - 1;
            if (aligned_phys <= phys_addr) {
                continue;
            }

            const KPhysicalAddress last_aligned_paddr = util::AlignDown(GetInteger(last) + 1, alignment) - 1;
            if (!(last_aligned_paddr <= last && aligned_phys <= last_aligned_paddr)) {
                continue;
            }

            addr = this->FindFreeArea(region_start, region_num_pages, num_pages, alignment, 0, this->GetNumGuardPages());
            if (addr != Null<KProcessAddress>) {
                break;
            }
        }
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());

        /* Check that we can map IO here. */
        MESOSPHERE_ASSERT(this->CanContain(addr, size, state));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, state == KMemoryState_IoRegister, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->Operate(page_list, addr, num_pages, phys_addr, true, properties, OperationType_Map, false));

        /* Set the output address. */
        *out = addr;

        R_SUCCEED();
    }

    Result KPageTableBase::MapIo(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Map the io memory. */
        KProcessAddress addr;
        R_TRY(this->MapIoImpl(std::addressof(addr), updater.GetPageList(), phys_addr, size, KMemoryState_IoRegister, perm));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, size / PageSize, KMemoryState_IoRegister, perm, KMemoryAttribute_Locked, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        R_SUCCEED();
    }

    Result KPageTableBase::MapIoRegion(KProcessAddress dst_address, KPhysicalAddress phys_addr, size_t size, ams::svc::MemoryMapping mapping, ams::svc::MemoryPermission svc_perm) {
        const size_t num_pages = size / PageSize;

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Validate the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), dst_address, size, KMemoryState_All, KMemoryState_None, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KMemoryPermission perm = ConvertToKMemoryPermission(svc_perm);
        const KPageProperties properties = { perm, mapping == ams::svc::MemoryMapping_IoRegister, mapping == ams::svc::MemoryMapping_Uncached, DisableMergeAttribute_DisableHead };
        R_TRY(this->Operate(updater.GetPageList(), dst_address, num_pages, phys_addr, true, properties, OperationType_Map, false));

        /* Update the blocks. */
        const auto state = mapping == ams::svc::MemoryMapping_Memory ? KMemoryState_IoMemory : KMemoryState_IoRegister;
        m_memory_block_manager.Update(std::addressof(allocator), dst_address, num_pages, state, perm, KMemoryAttribute_Locked, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        R_SUCCEED();
    }

    Result KPageTableBase::UnmapIoRegion(KProcessAddress dst_address, KPhysicalAddress phys_addr, size_t size, ams::svc::MemoryMapping mapping) {
        const size_t num_pages = size / PageSize;

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Validate the memory state. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        KMemoryAttribute old_attr;
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), std::addressof(old_attr), std::addressof(num_allocator_blocks),
                                     dst_address, size,
                                     KMemoryState_All, mapping == ams::svc::MemoryMapping_Memory ? KMemoryState_IoMemory : KMemoryState_IoRegister,
                                     KMemoryPermission_None, KMemoryPermission_None,
                                     KMemoryAttribute_All, KMemoryAttribute_Locked));

        /* Validate that the region being unmapped corresponds to the physical range described. */
        {
            /* Get the impl. */
            auto &impl = this->GetImpl();

            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   next_entry;
            MESOSPHERE_ABORT_UNLESS(impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), dst_address));

            /* Check that the physical region matches. */
            R_UNLESS(next_entry.phys_addr == phys_addr, svc::ResultInvalidMemoryRegion());

            /* Iterate. */
            for (size_t checked_size = next_entry.block_size - (GetInteger(phys_addr) & (next_entry.block_size - 1)); checked_size < size; checked_size += next_entry.block_size) {
                /* Continue the traversal. */
                MESOSPHERE_ABORT_UNLESS(impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context)));

                /* Check that the physical region matches. */
                R_UNLESS(next_entry.phys_addr == phys_addr + checked_size, svc::ResultInvalidMemoryRegion());
            }
        }

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* If the region being unmapped is Memory, synchronize. */
        if (mapping == ams::svc::MemoryMapping_Memory) {
            /* Change the region to be uncached. */
            const KPageProperties properties = { old_perm, false, true, DisableMergeAttribute_None };
            MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), dst_address, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissionsAndRefresh, false));

            /* Temporarily unlock ourselves, so that other operations can occur while we flush the region. */
            m_general_lock.Unlock();
            ON_SCOPE_EXIT { m_general_lock.Lock(); };

            /* Flush the region. */
            MESOSPHERE_R_ABORT_UNLESS(cpu::FlushDataCache(GetVoidPointer(dst_address), size));
        }

        /* Perform the unmap. */
        const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), dst_address, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), dst_address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        R_SUCCEED();
    }

    Result KPageTableBase::MapStatic(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(size,                  PageSize));
        MESOSPHERE_ASSERT(size > 0);
        R_UNLESS(phys_addr < phys_addr + size, svc::ResultInvalidAddress());
        const size_t num_pages = size / PageSize;
        const KPhysicalAddress last = phys_addr + size - 1;

        /* Get region extents. */
        const KProcessAddress region_start     = this->GetRegionAddress(KMemoryState_Static);
        const size_t          region_size      = this->GetRegionSize(KMemoryState_Static);
        const size_t          region_num_pages = region_size / PageSize;

        /* Locate the memory region. */
        const KMemoryRegion *region = KMemoryLayout::Find(phys_addr);
        R_UNLESS(region != nullptr, svc::ResultInvalidAddress());

        MESOSPHERE_ASSERT(region->Contains(GetInteger(phys_addr)));
        R_UNLESS(GetInteger(last) <= region->GetLastAddress(), svc::ResultInvalidAddress());

        /* Check the region attributes. */
        const bool is_rw = perm == KMemoryPermission_UserReadWrite;
        R_UNLESS( region->IsDerivedFrom(KMemoryRegionType_Dram),                      svc::ResultInvalidAddress());
        R_UNLESS(!region->HasTypeAttribute(KMemoryRegionAttr_NoUserMap),              svc::ResultInvalidAddress());
        R_UNLESS(!region->HasTypeAttribute(KMemoryRegionAttr_UserReadOnly) || !is_rw, svc::ResultInvalidAddress());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Select an address to map at. */
        KProcessAddress addr = Null<KProcessAddress>;
        for (s32 block_type = KPageTable::GetMaxBlockType(); block_type >= 0; block_type--) {
            const size_t alignment = KPageTable::GetBlockSize(static_cast<KPageTable::BlockType>(block_type));

            const KPhysicalAddress aligned_phys = util::AlignUp(GetInteger(phys_addr), alignment) + alignment - 1;
            if (aligned_phys <= phys_addr) {
                continue;
            }

            const KPhysicalAddress last_aligned_paddr = util::AlignDown(GetInteger(last) + 1, alignment) - 1;
            if (!(last_aligned_paddr <= last && aligned_phys <= last_aligned_paddr)) {
                continue;
            }

            addr = this->FindFreeArea(region_start, region_num_pages, num_pages, alignment, 0, this->GetNumGuardPages());
            if (addr != Null<KProcessAddress>) {
                break;
            }
        }
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());

        /* Check that we can map static here. */
        MESOSPHERE_ASSERT(this->CanContain(addr, size, KMemoryState_Static));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, false, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, phys_addr, true, properties, OperationType_Map, false));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, KMemoryState_Static, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        R_SUCCEED();
    }

    Result KPageTableBase::MapRegion(KMemoryRegionType region_type, KMemoryPermission perm) {
        /* Get the memory region. */
        const KMemoryRegion *region = KMemoryLayout::GetPhysicalMemoryRegionTree().FindFirstDerived(region_type);
        R_UNLESS(region != nullptr, svc::ResultOutOfRange());

        /* Check that the region is valid. */
        MESOSPHERE_ABORT_UNLESS(region->GetEndAddress() != 0);

        /* Map the region. */
        R_TRY_CATCH(this->MapStatic(region->GetAddress(), region->GetSize(), perm)) {
            R_CONVERT(svc::ResultInvalidAddress, svc::ResultOutOfRange())
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result KPageTableBase::MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, bool is_pa_valid, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(util::IsAligned(alignment, PageSize) && alignment >= PageSize);

        /* Ensure this is a valid map request. */
        R_UNLESS(this->CanContain(region_start, region_num_pages * PageSize, state), svc::ResultInvalidCurrentMemory());
        R_UNLESS(num_pages < region_num_pages,                                     svc::ResultOutOfMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Find a random address to map at. */
        KProcessAddress addr = this->FindFreeArea(region_start, region_num_pages, num_pages, alignment, 0, this->GetNumGuardPages());
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), alignment));
        MESOSPHERE_ASSERT(this->CanContain(addr, num_pages * PageSize, state));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, false, false, DisableMergeAttribute_DisableHead };
        if (is_pa_valid) {
            R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, phys_addr, true, properties, OperationType_Map, false));
        } else {
            R_TRY(this->AllocateAndMapPagesImpl(updater.GetPageList(), addr, num_pages, properties));
        }

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, state, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        *out_addr = addr;
        R_SUCCEED();
    }

    Result KPageTableBase::MapPages(KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm) {
        /* Check that the map is in range. */
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->CanContain(address, size, state), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Map the pages. */
        const KPageProperties properties = { perm, false, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->AllocateAndMapPagesImpl(updater.GetPageList(), address, num_pages, properties));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, state, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        R_SUCCEED();
    }

    Result KPageTableBase::UnmapPages(KProcessAddress address, size_t num_pages, KMemoryState state) {
        /* Check that the unmap is in range. */
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, state, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform the unmap. */
        const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        R_SUCCEED();
    }

    Result KPageTableBase::MapPageGroup(KProcessAddress *out_addr, const KPageGroup &pg, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

        /* Ensure this is a valid map request. */
        const size_t num_pages = pg.GetNumPages();
        R_UNLESS(this->CanContain(region_start, region_num_pages * PageSize, state), svc::ResultInvalidCurrentMemory());
        R_UNLESS(num_pages < region_num_pages,                                       svc::ResultOutOfMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Find a random address to map at. */
        KProcessAddress addr = this->FindFreeArea(region_start, region_num_pages, num_pages, PageSize, 0, this->GetNumGuardPages());
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());
        MESOSPHERE_ASSERT(this->CanContain(addr, num_pages * PageSize, state));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, false, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->MapPageGroupImpl(updater.GetPageList(), addr, pg, properties, false));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, state, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        *out_addr = addr;
        R_SUCCEED();
    }

    Result KPageTableBase::MapPageGroup(KProcessAddress addr, const KPageGroup &pg, KMemoryState state, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

        /* Ensure this is a valid map request. */
        const size_t num_pages = pg.GetNumPages();
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->CanContain(addr, size, state), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check if state allows us to map. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), addr, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Ensure cache coherency, if we're mapping executable pages. */
        if ((perm & KMemoryPermission_UserExecute) == KMemoryPermission_UserExecute) {
            cpu::InvalidateEntireInstructionCache();
        }

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, false, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->MapPageGroupImpl(updater.GetPageList(), addr, pg, properties, false));

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, state, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        R_SUCCEED();
    }

    Result KPageTableBase::UnmapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state) {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

        /* Ensure this is a valid unmap request. */
        const size_t num_pages = pg.GetNumPages();
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->CanContain(address, size, state), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check if state allows us to unmap. */
        KMemoryPermission old_perm;
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(nullptr, std::addressof(old_perm), nullptr, std::addressof(num_allocator_blocks), address, size, KMemoryState_All, state, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Check that the page group is valid. */
        R_UNLESS(this->IsValidPageGroup(pg, address, num_pages), svc::ResultInvalidCurrentMemory());

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform unmapping operation. */
        const KPageProperties properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_Unmap, false));

        /* Ensure cache coherency, if we're mapping executable pages. */
        if ((old_perm & KMemoryPermission_UserExecute) == KMemoryPermission_UserExecute) {
            cpu::InvalidateEntireInstructionCache();
        }

        /* Update the blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        R_SUCCEED();
    }

    Result KPageTableBase::MakeAndOpenPageGroup(KPageGroup *out, KProcessAddress address, size_t num_pages, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) {
        /* Ensure that the page group isn't null. */
        MESOSPHERE_ASSERT(out != nullptr);

        /* Make sure that the region we're mapping is valid for the table. */
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check if state allows us to create the group. */
        R_TRY(this->CheckMemoryState(address, size, state_mask | KMemoryState_FlagReferenceCounted, state | KMemoryState_FlagReferenceCounted, perm_mask, perm, attr_mask, attr));

        /* Create a new page group for the region. */
        R_TRY(this->MakePageGroup(*out, address, num_pages));

        /* Open a new reference to the pages in the group. */
        out->Open();

        R_SUCCEED();
    }

    Result KPageTableBase::InvalidateProcessDataCache(KProcessAddress address, size_t size) {
        /* Check that the region is in range. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the memory state. */
        R_TRY(this->CheckMemoryStateContiguous(address, size, KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted, KMemoryPermission_UserReadWrite, KMemoryPermission_UserReadWrite, KMemoryAttribute_Uncached, KMemoryAttribute_None));

        /* Get the impl. */
        auto &impl = this->GetImpl();

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   next_entry;
        bool traverse_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), address);
        R_UNLESS(traverse_valid, svc::ResultInvalidCurrentMemory());

        /* Prepare tracking variables. */
        KPhysicalAddress cur_addr = next_entry.phys_addr;
        size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
        size_t tot_size = cur_size;

        /* Iterate. */
        while (tot_size < size) {
            /* Continue the traversal. */
            traverse_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
            R_UNLESS(traverse_valid, svc::ResultInvalidCurrentMemory());

            if (next_entry.phys_addr != (cur_addr + cur_size)) {
                /* Check that the pages are linearly mapped. */
                R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

                /* Invalidate the block. */
                if (cur_size > 0) {
                    /* NOTE: Nintendo does not check the result of invalidation. */
                    cpu::InvalidateDataCache(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), cur_size);
                }

                /* Advance. */
                cur_addr = next_entry.phys_addr;
                cur_size = next_entry.block_size;
            } else {
                cur_size += next_entry.block_size;
            }

            tot_size += next_entry.block_size;
        }

        /* Ensure we use the right size for the last block. */
        if (tot_size > size) {
            cur_size -= (tot_size - size);
        }

        /* Check that the last block is linearly mapped. */
        R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

        /* Invalidate the last block. */
        if (cur_size > 0) {
            /* NOTE: Nintendo does not check the result of invalidation. */
            cpu::InvalidateDataCache(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), cur_size);
        }

        R_SUCCEED();
    }

    Result KPageTableBase::InvalidateCurrentProcessDataCache(KProcessAddress address, size_t size) {
        /* Check pre-condition: this is being called on the current process. */
        MESOSPHERE_ASSERT(this == std::addressof(GetCurrentProcess().GetPageTable().GetBasePageTable()));

        /* Check that the region is in range. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the memory state. */
        R_TRY(this->CheckMemoryStateContiguous(address, size, KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted, KMemoryPermission_UserReadWrite, KMemoryPermission_UserReadWrite, KMemoryAttribute_Uncached, KMemoryAttribute_None));

        /* Invalidate the data cache. */
        R_RETURN(cpu::InvalidateDataCache(GetVoidPointer(address), size));
    }

    bool KPageTableBase::CanReadWriteDebugMemory(KProcessAddress address, size_t size, bool force_debug_prod) {
        /* Check pre-conditions. */
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* If the memory is debuggable and user-readable, we can perform the access. */
        if (R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, KMemoryState_FlagCanDebug, KMemoryState_FlagCanDebug, KMemoryPermission_NotMapped | KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None))) {
            return true;
        }

        /* If we're in debug mode, and the process isn't force debug prod, check if the memory is debuggable and kernel-readable and user-executable. */
        if (KTargetSystem::IsDebugMode() && !force_debug_prod) {
            if (R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, KMemoryState_FlagCanDebug, KMemoryState_FlagCanDebug, KMemoryPermission_KernelRead | KMemoryPermission_UserExecute, KMemoryPermission_KernelRead | KMemoryPermission_UserExecute, KMemoryAttribute_None, KMemoryAttribute_None))) {
                return true;
            }
        }

        /* If neither of the above checks passed, we can't access the memory. */
        return false;
    }

    Result KPageTableBase::ReadDebugMemory(void *buffer, KProcessAddress address, size_t size, bool force_debug_prod) {
        /* Lightly validate the region is in range. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Require that the memory either be user-readable-and-mapped or debug-accessible. */
        const bool can_read = R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, KMemoryState_None, KMemoryState_None, KMemoryPermission_NotMapped | KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None));
        if (!can_read) {
            R_UNLESS(this->CanReadWriteDebugMemory(address, size, force_debug_prod), svc::ResultInvalidCurrentMemory());
        }

        /* Get the impl. */
        auto &impl = this->GetImpl();

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   next_entry;
        bool traverse_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), address);
        R_UNLESS(traverse_valid, svc::ResultInvalidCurrentMemory());

        /* Prepare tracking variables. */
        KPhysicalAddress cur_addr = next_entry.phys_addr;
        size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
        size_t tot_size = cur_size;

        auto PerformCopy = [&]() ALWAYS_INLINE_LAMBDA -> Result {
            /* Ensure the address is linear mapped. */
            R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

            /* Copy as much aligned data as we can. */
            if (cur_size >= sizeof(u32)) {
                const size_t copy_size = util::AlignDown(cur_size, sizeof(u32));
                const void * copy_src  = GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr));
                cpu::FlushDataCache(copy_src, copy_size);
                R_UNLESS(UserspaceAccess::CopyMemoryToUserAligned32Bit(buffer, copy_src, copy_size), svc::ResultInvalidPointer());
                buffer    = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + copy_size);
                cur_addr += copy_size;
                cur_size -= copy_size;
            }

            /* Copy remaining data. */
            if (cur_size > 0) {
                const void * copy_src  = GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr));
                cpu::FlushDataCache(copy_src, cur_size);
                R_UNLESS(UserspaceAccess::CopyMemoryToUser(buffer, copy_src, cur_size), svc::ResultInvalidPointer());
            }

            R_SUCCEED();
        };

        /* Iterate. */
        while (tot_size < size) {
            /* Continue the traversal. */
            traverse_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
            MESOSPHERE_ASSERT(traverse_valid);

            if (next_entry.phys_addr != (cur_addr + cur_size)) {
                /* Perform copy. */
                R_TRY(PerformCopy());

                /* Advance. */
                buffer   = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + cur_size);

                cur_addr = next_entry.phys_addr;
                cur_size = next_entry.block_size;
            } else {
                cur_size += next_entry.block_size;
            }

            tot_size += next_entry.block_size;
        }

        /* Ensure we use the right size for the last block. */
        if (tot_size > size) {
            cur_size -= (tot_size - size);
        }

        /* Perform copy for the last block. */
        R_TRY(PerformCopy());

        R_SUCCEED();
    }

    Result KPageTableBase::WriteDebugMemory(KProcessAddress address, const void *buffer, size_t size) {
        /* Lightly validate the region is in range. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Require that the memory either be user-writable-and-mapped or debug-accessible. */
        const bool can_write = R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, KMemoryState_None, KMemoryState_None, KMemoryPermission_NotMapped | KMemoryPermission_UserReadWrite, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryAttribute_None));
        if (!can_write) {
            R_UNLESS(this->CanReadWriteDebugMemory(address, size, false), svc::ResultInvalidCurrentMemory());
        }

        /* Get the impl. */
        auto &impl = this->GetImpl();

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   next_entry;
        bool traverse_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), address);
        R_UNLESS(traverse_valid, svc::ResultInvalidCurrentMemory());

        /* Prepare tracking variables. */
        KPhysicalAddress cur_addr = next_entry.phys_addr;
        size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
        size_t tot_size = cur_size;

        auto PerformCopy = [&]() ALWAYS_INLINE_LAMBDA -> Result {
            /* Ensure the address is linear mapped. */
            R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

            /* Copy as much aligned data as we can. */
            if (cur_size >= sizeof(u32)) {
                const size_t copy_size = util::AlignDown(cur_size, sizeof(u32));
                R_UNLESS(UserspaceAccess::CopyMemoryFromUserAligned32Bit(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), buffer, copy_size), svc::ResultInvalidCurrentMemory());
                cpu::StoreDataCache(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), copy_size);

                buffer    = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + copy_size);
                cur_addr += copy_size;
                cur_size -= copy_size;
            }

            /* Copy remaining data. */
            if (cur_size > 0) {
                R_UNLESS(UserspaceAccess::CopyMemoryFromUser(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), buffer, cur_size), svc::ResultInvalidCurrentMemory());
                cpu::StoreDataCache(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), cur_size);
            }

            R_SUCCEED();
        };

        /* Iterate. */
        while (tot_size < size) {
            /* Continue the traversal. */
            traverse_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
            MESOSPHERE_ASSERT(traverse_valid);

            if (next_entry.phys_addr != (cur_addr + cur_size)) {
                /* Perform copy. */
                R_TRY(PerformCopy());

                /* Advance. */
                buffer   = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + cur_size);

                cur_addr = next_entry.phys_addr;
                cur_size = next_entry.block_size;
            } else {
                cur_size += next_entry.block_size;
            }

            tot_size += next_entry.block_size;
        }

        /* Ensure we use the right size for the last block. */
        if (tot_size > size) {
            cur_size -= (tot_size - size);
        }

        /* Perform copy for the last block. */
        R_TRY(PerformCopy());

        /* Invalidate the entire instruction cache, as this svc allows modifying executable pages. */
        cpu::InvalidateEntireInstructionCache();

        R_SUCCEED();
    }

    Result KPageTableBase::ReadIoMemoryImpl(void *buffer, KPhysicalAddress phys_addr, size_t size, KMemoryState state) {
        /* Check pre-conditions. */
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Determine the mapping extents. */
        const KPhysicalAddress map_start = util::AlignDown(GetInteger(phys_addr), PageSize);
        const KPhysicalAddress map_end   = util::AlignUp(GetInteger(phys_addr) + size, PageSize);
        const size_t map_size            = map_end - map_start;

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Temporarily map the io memory. */
        KProcessAddress io_addr;
        R_TRY(this->MapIoImpl(std::addressof(io_addr), updater.GetPageList(), map_start, map_size, state, KMemoryPermission_UserRead));

        /* Ensure we unmap the io memory when we're done with it. */
        ON_SCOPE_EXIT {
            const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
            MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), io_addr, map_size / PageSize, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
        };

        /* Read the memory. */
        const KProcessAddress read_addr = io_addr + (GetInteger(phys_addr) & (PageSize - 1));
        switch ((GetInteger(read_addr) | size) & 3) {
            case 0:
                {
                    R_UNLESS(UserspaceAccess::ReadIoMemory32Bit(buffer, GetVoidPointer(read_addr), size), svc::ResultInvalidPointer());
                }
                break;
            case 2:
                {
                    R_UNLESS(UserspaceAccess::ReadIoMemory16Bit(buffer, GetVoidPointer(read_addr), size), svc::ResultInvalidPointer());
                }
                break;
            default:
                {
                    R_UNLESS(UserspaceAccess::ReadIoMemory8Bit(buffer, GetVoidPointer(read_addr), size),  svc::ResultInvalidPointer());
                }
                break;
        }

        R_SUCCEED();
    }

    Result KPageTableBase::WriteIoMemoryImpl(KPhysicalAddress phys_addr, const void *buffer, size_t size, KMemoryState state) {
        /* Check pre-conditions. */
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Determine the mapping extents. */
        const KPhysicalAddress map_start = util::AlignDown(GetInteger(phys_addr), PageSize);
        const KPhysicalAddress map_end   = util::AlignUp(GetInteger(phys_addr) + size, PageSize);
        const size_t map_size            = map_end - map_start;

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Temporarily map the io memory. */
        KProcessAddress io_addr;
        R_TRY(this->MapIoImpl(std::addressof(io_addr), updater.GetPageList(), map_start, map_size, state, KMemoryPermission_UserReadWrite));

        /* Ensure we unmap the io memory when we're done with it. */
        ON_SCOPE_EXIT {
            const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
            MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), io_addr, map_size / PageSize, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
        };

        /* Read the memory. */
        const KProcessAddress write_addr = io_addr + (GetInteger(phys_addr) & (PageSize - 1));
        switch ((GetInteger(write_addr) | size) & 3) {
            case 0:
                {
                    R_UNLESS(UserspaceAccess::WriteIoMemory32Bit(GetVoidPointer(write_addr), buffer, size), svc::ResultInvalidPointer());
                }
                break;
            case 2:
                {
                    R_UNLESS(UserspaceAccess::WriteIoMemory16Bit(GetVoidPointer(write_addr), buffer, size), svc::ResultInvalidPointer());
                }
                break;
            default:
                {
                    R_UNLESS(UserspaceAccess::WriteIoMemory8Bit(GetVoidPointer(write_addr), buffer, size),  svc::ResultInvalidPointer());
                }
                break;
        }

        R_SUCCEED();
    }

    Result KPageTableBase::ReadDebugIoMemory(void *buffer, KProcessAddress address, size_t size, KMemoryState state) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* We need to lock both this table, and the current process's table, so set up some aliases. */
        KPageTableBase &src_page_table = *this;
        KPageTableBase &dst_page_table = GetCurrentProcess().GetPageTable().GetBasePageTable();

        /* Acquire the table locks. */
        KScopedLightLockPair lk(src_page_table.m_general_lock, dst_page_table.m_general_lock);

        /* Check that the desired range is readable io memory. */
        R_TRY(this->CheckMemoryStateContiguous(address, size, KMemoryState_All, state, KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Read the memory. */
        u8 *dst = static_cast<u8 *>(buffer);
        const KProcessAddress last_address = address + size - 1;
        while (address <= last_address) {
            /* Get the current physical address. */
            KPhysicalAddress phys_addr;
            MESOSPHERE_ABORT_UNLESS(src_page_table.GetPhysicalAddressLocked(std::addressof(phys_addr), address));

            /* Determine the current read size. */
            const size_t cur_size = std::min<size_t>(last_address - address + 1, PageSize - (GetInteger(address) & (PageSize - 1)));

            /* Read. */
            R_TRY(dst_page_table.ReadIoMemoryImpl(dst, phys_addr, cur_size, state));

            /* Advance. */
            address += cur_size;
            dst     += cur_size;
        }

        R_SUCCEED();
    }

    Result KPageTableBase::WriteDebugIoMemory(KProcessAddress address, const void *buffer, size_t size, KMemoryState state) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* We need to lock both this table, and the current process's table, so set up some aliases. */
        KPageTableBase &src_page_table = *this;
        KPageTableBase &dst_page_table = GetCurrentProcess().GetPageTable().GetBasePageTable();

        /* Acquire the table locks. */
        KScopedLightLockPair lk(src_page_table.m_general_lock, dst_page_table.m_general_lock);

        /* Check that the desired range is writable io memory. */
        R_TRY(this->CheckMemoryStateContiguous(address, size, KMemoryState_All, state, KMemoryPermission_UserReadWrite, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Read the memory. */
        const u8 *src = static_cast<const u8 *>(buffer);
        const KProcessAddress last_address = address + size - 1;
        while (address <= last_address) {
            /* Get the current physical address. */
            KPhysicalAddress phys_addr;
            MESOSPHERE_ABORT_UNLESS(src_page_table.GetPhysicalAddressLocked(std::addressof(phys_addr), address));

            /* Determine the current read size. */
            const size_t cur_size = std::min<size_t>(last_address - address + 1, PageSize - (GetInteger(address) & (PageSize - 1)));

            /* Read. */
            R_TRY(dst_page_table.WriteIoMemoryImpl(phys_addr, src, cur_size, state));

            /* Advance. */
            address += cur_size;
            src     += cur_size;
        }

        R_SUCCEED();
    }

    Result KPageTableBase::LockForMapDeviceAddressSpace(bool *out_is_io, KProcessAddress address, size_t size, KMemoryPermission perm, bool is_aligned, bool check_heap) {
        /* Lightly validate the range before doing anything else. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the memory state. */
        const u32 test_state = (is_aligned ? KMemoryState_FlagCanAlignedDeviceMap : KMemoryState_FlagCanDeviceMap) | (check_heap ? KMemoryState_FlagReferenceCounted : KMemoryState_None);
        size_t num_allocator_blocks;
        KMemoryState old_state;
        R_TRY(this->CheckMemoryState(std::addressof(old_state), nullptr, nullptr, std::addressof(num_allocator_blocks), address, size, test_state, test_state, perm, perm, KMemoryAttribute_IpcLocked | KMemoryAttribute_Locked, KMemoryAttribute_None, KMemoryAttribute_DeviceShared));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* Update the memory blocks. */
        m_memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, &KMemoryBlock::ShareToDevice, KMemoryPermission_None);

        /* Set whether the locked memory was io. */
        *out_is_io = static_cast<ams::svc::MemoryState>(old_state & KMemoryState_Mask) == ams::svc::MemoryState_Io;

        R_SUCCEED();
    }

    Result KPageTableBase::LockForUnmapDeviceAddressSpace(KProcessAddress address, size_t size, bool check_heap) {
        /* Lightly validate the range before doing anything else. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the memory state. */
        const u32 test_state = KMemoryState_FlagCanDeviceMap | (check_heap ? KMemoryState_FlagReferenceCounted : KMemoryState_None);
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryStateContiguous(std::addressof(num_allocator_blocks),
                                               address, size,
                                               test_state, test_state,
                                               KMemoryPermission_None, KMemoryPermission_None,
                                               KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* Update the memory blocks. */
        const KMemoryBlockManager::MemoryBlockLockFunction lock_func = m_enable_device_address_space_merge ? &KMemoryBlock::UpdateDeviceDisableMergeStateForShare : &KMemoryBlock::UpdateDeviceDisableMergeStateForShareRight;
        m_memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, lock_func, KMemoryPermission_None);

        R_SUCCEED();
    }

    Result KPageTableBase::UnlockForDeviceAddressSpace(KProcessAddress address, size_t size) {
        /* Lightly validate the range before doing anything else. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryStateContiguous(std::addressof(num_allocator_blocks),
                                               address, size,
                                               KMemoryState_FlagCanDeviceMap, KMemoryState_FlagCanDeviceMap,
                                               KMemoryPermission_None, KMemoryPermission_None,
                                               KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* Update the memory blocks. */
        m_memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, &KMemoryBlock::UnshareToDevice, KMemoryPermission_None);

        R_SUCCEED();
    }

    Result KPageTableBase::UnlockForDeviceAddressSpacePartialMap(KProcessAddress address, size_t size) {
        /* Lightly validate the range before doing anything else. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check memory state. */
        size_t allocator_num_blocks = 0;
        R_TRY(this->CheckMemoryStateContiguous(std::addressof(allocator_num_blocks),
                                               address, size,
                                               KMemoryState_FlagCanDeviceMap, KMemoryState_FlagCanDeviceMap,
                                               KMemoryPermission_None, KMemoryPermission_None,
                                               KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));

        /* Create an update allocator for the region. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, allocator_num_blocks);
        R_TRY(allocator_result);

        /* Update the memory blocks. */
        m_memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, m_enable_device_address_space_merge ? &KMemoryBlock::UpdateDeviceDisableMergeStateForUnshare : &KMemoryBlock::UpdateDeviceDisableMergeStateForUnshareRight, KMemoryPermission_None);

        R_SUCCEED();
    }

    Result KPageTableBase::OpenMemoryRangeForMapDeviceAddressSpace(KPageTableBase::MemoryRange *out, KProcessAddress address, size_t size, KMemoryPermission perm, bool is_aligned) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Get the range. */
        const u32 test_state = (is_aligned ? KMemoryState_FlagCanAlignedDeviceMap : KMemoryState_FlagCanDeviceMap);
        R_TRY(this->GetContiguousMemoryRangeWithState(out,
                                                      address, size,
                                                      test_state, test_state,
                                                      perm, perm,
                                                      KMemoryAttribute_IpcLocked | KMemoryAttribute_Locked, KMemoryAttribute_None));

        /* We got the range, so open it. */
        out->Open();

        R_SUCCEED();
    }

    Result KPageTableBase::OpenMemoryRangeForUnmapDeviceAddressSpace(MemoryRange *out, KProcessAddress address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Get the range. */
        R_TRY(this->GetContiguousMemoryRangeWithState(out,
                                                      address, size,
                                                      KMemoryState_FlagCanDeviceMap, KMemoryState_FlagCanDeviceMap,
                                                      KMemoryPermission_None, KMemoryPermission_None,
                                                      KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));

        /* We got the range, so open it. */
        out->Open();

        R_SUCCEED();
    }

    Result KPageTableBase::LockForIpcUserBuffer(KPhysicalAddress *out, KProcessAddress address, size_t size) {
        R_RETURN(this->LockMemoryAndOpen(nullptr, out, address, size,
                                       KMemoryState_FlagCanIpcUserBuffer, KMemoryState_FlagCanIpcUserBuffer,
                                       KMemoryPermission_All, KMemoryPermission_UserReadWrite,
                                       KMemoryAttribute_All, KMemoryAttribute_None,
                                       static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite),
                                       KMemoryAttribute_Locked));
    }

    Result KPageTableBase::UnlockForIpcUserBuffer(KProcessAddress address, size_t size) {
        R_RETURN(this->UnlockMemory(address, size,
                                  KMemoryState_FlagCanIpcUserBuffer, KMemoryState_FlagCanIpcUserBuffer,
                                  KMemoryPermission_None, KMemoryPermission_None,
                                  KMemoryAttribute_All, KMemoryAttribute_Locked,
                                  KMemoryPermission_UserReadWrite,
                                  KMemoryAttribute_Locked, nullptr));
    }

    Result KPageTableBase::LockForTransferMemory(KPageGroup *out, KProcessAddress address, size_t size, KMemoryPermission perm) {
        R_RETURN(this->LockMemoryAndOpen(out, nullptr, address, size,
                                       KMemoryState_FlagCanTransfer, KMemoryState_FlagCanTransfer,
                                       KMemoryPermission_All, KMemoryPermission_UserReadWrite,
                                       KMemoryAttribute_All, KMemoryAttribute_None,
                                       perm,
                                       KMemoryAttribute_Locked));
    }

    Result KPageTableBase::UnlockForTransferMemory(KProcessAddress address, size_t size, const KPageGroup &pg) {
        R_RETURN(this->UnlockMemory(address, size,
                                  KMemoryState_FlagCanTransfer, KMemoryState_FlagCanTransfer,
                                  KMemoryPermission_None, KMemoryPermission_None,
                                  KMemoryAttribute_All, KMemoryAttribute_Locked,
                                  KMemoryPermission_UserReadWrite,
                                  KMemoryAttribute_Locked, std::addressof(pg)));
    }

    Result KPageTableBase::LockForCodeMemory(KPageGroup *out, KProcessAddress address, size_t size) {
        R_RETURN(this->LockMemoryAndOpen(out, nullptr, address, size,
                                       KMemoryState_FlagCanCodeMemory, KMemoryState_FlagCanCodeMemory,
                                       KMemoryPermission_All, KMemoryPermission_UserReadWrite,
                                       KMemoryAttribute_All, KMemoryAttribute_None,
                                       static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite),
                                       KMemoryAttribute_Locked));
    }

    Result KPageTableBase::UnlockForCodeMemory(KProcessAddress address, size_t size, const KPageGroup &pg) {
        R_RETURN(this->UnlockMemory(address, size,
                                  KMemoryState_FlagCanCodeMemory, KMemoryState_FlagCanCodeMemory,
                                  KMemoryPermission_None, KMemoryPermission_None,
                                  KMemoryAttribute_All, KMemoryAttribute_Locked,
                                  KMemoryPermission_UserReadWrite,
                                  KMemoryAttribute_Locked, std::addressof(pg)));
    }

    Result KPageTableBase::OpenMemoryRangeForProcessCacheOperation(MemoryRange *out, KProcessAddress address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Get the range. */
        R_TRY(this->GetContiguousMemoryRangeWithState(out,
                                                      address, size,
                                                      KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                      KMemoryPermission_UserRead, KMemoryPermission_UserRead,
                                                      KMemoryAttribute_Uncached, KMemoryAttribute_None));

        /* We got the range, so open it. */
        out->Open();

        R_SUCCEED();
    }

    Result KPageTableBase::CopyMemoryFromLinearToUser(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(src_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(m_general_lock);

            /* Check memory state. */
            R_TRY(this->CheckMemoryStateContiguous(src_addr, size, src_state_mask, src_state, src_test_perm, src_test_perm, src_attr_mask | KMemoryAttribute_Uncached, src_attr));

            auto &impl = this->GetImpl();

            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   next_entry;
            bool traverse_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), src_addr);
            MESOSPHERE_ABORT_UNLESS(traverse_valid);

            /* Prepare tracking variables. */
            KPhysicalAddress cur_addr = next_entry.phys_addr;
            size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
            size_t tot_size = cur_size;

            auto PerformCopy = [&]() ALWAYS_INLINE_LAMBDA -> Result {
                /* Ensure the address is linear mapped. */
                R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

                /* Copy as much aligned data as we can. */
                if (cur_size >= sizeof(u32)) {
                    const size_t copy_size = util::AlignDown(cur_size, sizeof(u32));
                    R_UNLESS(UserspaceAccess::CopyMemoryToUserAligned32Bit(GetVoidPointer(dst_addr), GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), copy_size), svc::ResultInvalidCurrentMemory());
                    dst_addr += copy_size;
                    cur_addr += copy_size;
                    cur_size -= copy_size;
                }

                /* Copy remaining data. */
                if (cur_size > 0) {
                    R_UNLESS(UserspaceAccess::CopyMemoryToUser(GetVoidPointer(dst_addr), GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), cur_size), svc::ResultInvalidCurrentMemory());
                }

                R_SUCCEED();
            };

            /* Iterate. */
            while (tot_size < size) {
                /* Continue the traversal. */
                traverse_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                MESOSPHERE_ASSERT(traverse_valid);

                if (next_entry.phys_addr != (cur_addr + cur_size)) {
                    /* Perform copy. */
                    R_TRY(PerformCopy());

                    /* Advance. */
                    dst_addr += cur_size;

                    cur_addr = next_entry.phys_addr;
                    cur_size = next_entry.block_size;
                } else {
                    cur_size += next_entry.block_size;
                }

                tot_size += next_entry.block_size;
            }

            /* Ensure we use the right size for the last block. */
            if (tot_size > size) {
                cur_size -= (tot_size - size);
            }

            /* Perform copy for the last block. */
            R_TRY(PerformCopy());
        }

        R_SUCCEED();
    }

    Result KPageTableBase::CopyMemoryFromLinearToKernel(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(src_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(m_general_lock);

            /* Check memory state. */
            R_TRY(this->CheckMemoryStateContiguous(src_addr, size, src_state_mask, src_state, src_test_perm, src_test_perm, src_attr_mask | KMemoryAttribute_Uncached, src_attr));

            auto &impl = this->GetImpl();

            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   next_entry;
            bool traverse_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), src_addr);
            MESOSPHERE_ABORT_UNLESS(traverse_valid);

            /* Prepare tracking variables. */
            KPhysicalAddress cur_addr = next_entry.phys_addr;
            size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
            size_t tot_size = cur_size;

            auto PerformCopy = [&]() ALWAYS_INLINE_LAMBDA -> Result {
                /* Ensure the address is linear mapped. */
                R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

                /* Copy the data. */
                std::memcpy(GetVoidPointer(dst_addr), GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), cur_size);

                R_SUCCEED();
            };

            /* Iterate. */
            while (tot_size < size) {
                /* Continue the traversal. */
                traverse_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                MESOSPHERE_ASSERT(traverse_valid);

                if (next_entry.phys_addr != (cur_addr + cur_size)) {
                    /* Perform copy. */
                    R_TRY(PerformCopy());

                    /* Advance. */
                    dst_addr += cur_size;

                    cur_addr = next_entry.phys_addr;
                    cur_size = next_entry.block_size;
                } else {
                    cur_size += next_entry.block_size;
                }

                tot_size += next_entry.block_size;
            }

            /* Ensure we use the right size for the last block. */
            if (tot_size > size) {
                cur_size -= (tot_size - size);
            }

            /* Perform copy for the last block. */
            R_TRY(PerformCopy());
        }

        R_SUCCEED();
    }

    Result KPageTableBase::CopyMemoryFromUserToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(dst_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(m_general_lock);

            /* Check memory state. */
            R_TRY(this->CheckMemoryStateContiguous(dst_addr, size, dst_state_mask, dst_state, dst_test_perm, dst_test_perm, dst_attr_mask | KMemoryAttribute_Uncached, dst_attr));

            auto &impl = this->GetImpl();

            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   next_entry;
            bool traverse_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), dst_addr);
            MESOSPHERE_ABORT_UNLESS(traverse_valid);

            /* Prepare tracking variables. */
            KPhysicalAddress cur_addr = next_entry.phys_addr;
            size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
            size_t tot_size = cur_size;

            auto PerformCopy = [&]() ALWAYS_INLINE_LAMBDA -> Result {
                /* Ensure the address is linear mapped. */
                R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

                /* Copy as much aligned data as we can. */
                if (cur_size >= sizeof(u32)) {
                    const size_t copy_size = util::AlignDown(cur_size, sizeof(u32));
                    R_UNLESS(UserspaceAccess::CopyMemoryFromUserAligned32Bit(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), GetVoidPointer(src_addr), copy_size), svc::ResultInvalidCurrentMemory());
                    src_addr += copy_size;
                    cur_addr += copy_size;
                    cur_size -= copy_size;
                }

                /* Copy remaining data. */
                if (cur_size > 0) {
                    R_UNLESS(UserspaceAccess::CopyMemoryFromUser(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), GetVoidPointer(src_addr), cur_size), svc::ResultInvalidCurrentMemory());
                }

                R_SUCCEED();
            };

            /* Iterate. */
            while (tot_size < size) {
                /* Continue the traversal. */
                traverse_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                MESOSPHERE_ASSERT(traverse_valid);

                if (next_entry.phys_addr != (cur_addr + cur_size)) {
                    /* Perform copy. */
                    R_TRY(PerformCopy());

                    /* Advance. */
                    src_addr += cur_size;

                    cur_addr = next_entry.phys_addr;
                    cur_size = next_entry.block_size;
                } else {
                    cur_size += next_entry.block_size;
                }

                tot_size += next_entry.block_size;
            }

            /* Ensure we use the right size for the last block. */
            if (tot_size > size) {
                cur_size -= (tot_size - size);
            }

            /* Perform copy for the last block. */
            R_TRY(PerformCopy());
        }

        R_SUCCEED();
    }

    Result KPageTableBase::CopyMemoryFromKernelToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(dst_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(m_general_lock);

            /* Check memory state. */
            R_TRY(this->CheckMemoryStateContiguous(dst_addr, size, dst_state_mask, dst_state, dst_test_perm, dst_test_perm, dst_attr_mask | KMemoryAttribute_Uncached, dst_attr));

            auto &impl = this->GetImpl();

            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   next_entry;
            bool traverse_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), dst_addr);
            MESOSPHERE_ABORT_UNLESS(traverse_valid);

            /* Prepare tracking variables. */
            KPhysicalAddress cur_addr = next_entry.phys_addr;
            size_t cur_size = next_entry.block_size - (GetInteger(cur_addr) & (next_entry.block_size - 1));
            size_t tot_size = cur_size;

            auto PerformCopy = [&]() ALWAYS_INLINE_LAMBDA -> Result {
                /* Ensure the address is linear mapped. */
                R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

                /* Copy the data. */
                std::memcpy(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), GetVoidPointer(src_addr), cur_size);

                R_SUCCEED();
            };

            /* Iterate. */
            while (tot_size < size) {
                /* Continue the traversal. */
                traverse_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                MESOSPHERE_ASSERT(traverse_valid);

                if (next_entry.phys_addr != (cur_addr + cur_size)) {
                    /* Perform copy. */
                    R_TRY(PerformCopy());

                    /* Advance. */
                    src_addr += cur_size;

                    cur_addr = next_entry.phys_addr;
                    cur_size = next_entry.block_size;
                } else {
                    cur_size += next_entry.block_size;
                }

                tot_size += next_entry.block_size;
            }

            /* Ensure we use the right size for the last block. */
            if (tot_size > size) {
                cur_size -= (tot_size - size);
            }

            /* Perform copy for the last block. */
            R_TRY(PerformCopy());
        }

        R_SUCCEED();
    }

    Result KPageTableBase::CopyMemoryFromHeapToHeap(KPageTableBase &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
        /* For convenience, alias this. */
        KPageTableBase &src_page_table = *this;

        /* Lightly validate the ranges before doing anything else. */
        R_UNLESS(src_page_table.Contains(src_addr, size), svc::ResultInvalidCurrentMemory());
        R_UNLESS(dst_page_table.Contains(dst_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Acquire the table locks. */
            KScopedLightLockPair lk(src_page_table.m_general_lock, dst_page_table.m_general_lock);

            /* Check memory state. */
            R_TRY(src_page_table.CheckMemoryStateContiguous(src_addr, size, src_state_mask, src_state, src_test_perm, src_test_perm, src_attr_mask | KMemoryAttribute_Uncached, src_attr));
            R_TRY(dst_page_table.CheckMemoryStateContiguous(dst_addr, size, dst_state_mask, dst_state, dst_test_perm, dst_test_perm, dst_attr_mask | KMemoryAttribute_Uncached, dst_attr));

            /* Get implementations. */
            auto &src_impl = src_page_table.GetImpl();
            auto &dst_impl = dst_page_table.GetImpl();

            /* Prepare for traversal. */
            TraversalContext src_context;
            TraversalContext dst_context;
            TraversalEntry   src_next_entry;
            TraversalEntry   dst_next_entry;
            bool             traverse_valid;

            /* Begin traversal. */
            traverse_valid = src_impl.BeginTraversal(std::addressof(src_next_entry), std::addressof(src_context), src_addr);
            MESOSPHERE_ABORT_UNLESS(traverse_valid);
            traverse_valid = dst_impl.BeginTraversal(std::addressof(dst_next_entry), std::addressof(dst_context), dst_addr);
            MESOSPHERE_ABORT_UNLESS(traverse_valid);

            /* Prepare tracking variables. */
            KPhysicalAddress cur_src_block_addr = src_next_entry.phys_addr;
            KPhysicalAddress cur_dst_block_addr = dst_next_entry.phys_addr;
            size_t cur_src_size = src_next_entry.block_size - (GetInteger(cur_src_block_addr) & (src_next_entry.block_size - 1));
            size_t cur_dst_size = dst_next_entry.block_size - (GetInteger(cur_dst_block_addr) & (dst_next_entry.block_size - 1));

            /* Adjust the initial block sizes. */
            src_next_entry.block_size = cur_src_size;
            dst_next_entry.block_size = cur_dst_size;

            /* Before we get any crazier, succeed if there's nothing to do. */
            R_SUCCEED_IF(size == 0);

            /* We're going to manage dual traversal via an offset against the total size. */
            KPhysicalAddress cur_src_addr = cur_src_block_addr;
            KPhysicalAddress cur_dst_addr = cur_dst_block_addr;
            size_t cur_min_size = std::min<size_t>(cur_src_size, cur_dst_size);

            /* Iterate. */
            size_t ofs = 0;
            while (ofs < size) {
                /* Determine how much we can copy this iteration. */
                const size_t cur_copy_size = std::min<size_t>(cur_min_size, size - ofs);

                /* If we need to advance the traversals, do so. */
                bool updated_src = false, updated_dst = false, skip_copy = false;
                if (ofs + cur_copy_size != size) {
                    if (cur_src_addr + cur_min_size == cur_src_block_addr + cur_src_size) {
                        /* Continue the src traversal. */
                        traverse_valid = src_impl.ContinueTraversal(std::addressof(src_next_entry), std::addressof(src_context));
                        MESOSPHERE_ASSERT(traverse_valid);

                        /* Update source. */
                        updated_src = cur_src_addr + cur_min_size != GetInteger(src_next_entry.phys_addr);
                    }

                    if (cur_dst_addr + cur_min_size == dst_next_entry.phys_addr + dst_next_entry.block_size) {
                        /* Continue the dst traversal. */
                        traverse_valid = dst_impl.ContinueTraversal(std::addressof(dst_next_entry), std::addressof(dst_context));
                        MESOSPHERE_ASSERT(traverse_valid);

                        /* Update destination. */
                        updated_dst = cur_dst_addr + cur_min_size != GetInteger(dst_next_entry.phys_addr);
                    }

                    /* If we didn't update either of source/destination, skip the copy this iteration. */
                    if (!updated_src && !updated_dst) {
                        skip_copy = true;

                        /* Update the source block address. */
                        cur_src_block_addr = src_next_entry.phys_addr;
                    }
                }

                /* Do the copy, unless we're skipping it. */
                if (!skip_copy) {
                    /* We need both ends of the copy to be heap blocks. */
                    R_UNLESS(IsHeapPhysicalAddress(cur_src_addr), svc::ResultInvalidCurrentMemory());
                    R_UNLESS(IsHeapPhysicalAddress(cur_dst_addr), svc::ResultInvalidCurrentMemory());

                    /* Copy the data. */
                    std::memcpy(GetVoidPointer(GetHeapVirtualAddress(cur_dst_addr)), GetVoidPointer(GetHeapVirtualAddress(cur_src_addr)), cur_copy_size);

                    /* Update. */
                    cur_src_block_addr = src_next_entry.phys_addr;
                    cur_src_addr       = updated_src ? cur_src_block_addr : cur_src_addr + cur_copy_size;
                    cur_dst_block_addr = dst_next_entry.phys_addr;
                    cur_dst_addr       = updated_dst ? cur_dst_block_addr : cur_dst_addr + cur_copy_size;

                    /* Advance offset. */
                    ofs += cur_copy_size;
                }

                /* Update min size. */
                cur_src_size = src_next_entry.block_size;
                cur_dst_size = dst_next_entry.block_size;
                cur_min_size = std::min<size_t>(cur_src_block_addr - cur_src_addr + cur_src_size, cur_dst_block_addr - cur_dst_addr + cur_dst_size);
            }
        }

        R_SUCCEED();
    }

    Result KPageTableBase::CopyMemoryFromHeapToHeapWithoutCheckDestination(KPageTableBase &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
        /* For convenience, alias this. */
        KPageTableBase &src_page_table = *this;

        /* Lightly validate the ranges before doing anything else. */
        R_UNLESS(src_page_table.Contains(src_addr, size), svc::ResultInvalidCurrentMemory());
        R_UNLESS(dst_page_table.Contains(dst_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Acquire the table locks. */
            KScopedLightLockPair lk(src_page_table.m_general_lock, dst_page_table.m_general_lock);

            /* Check memory state for source. */
            R_TRY(src_page_table.CheckMemoryStateContiguous(src_addr, size, src_state_mask, src_state, src_test_perm, src_test_perm, src_attr_mask | KMemoryAttribute_Uncached, src_attr));

            /* Destination state is intentionally unchecked. */
            MESOSPHERE_UNUSED(dst_state_mask, dst_state, dst_test_perm, dst_attr_mask, dst_attr);

            /* Get implementations. */
            auto &src_impl = src_page_table.GetImpl();
            auto &dst_impl = dst_page_table.GetImpl();

            /* Prepare for traversal. */
            TraversalContext src_context;
            TraversalContext dst_context;
            TraversalEntry   src_next_entry;
            TraversalEntry   dst_next_entry;
            bool             traverse_valid;

            /* Begin traversal. */
            traverse_valid = src_impl.BeginTraversal(std::addressof(src_next_entry), std::addressof(src_context), src_addr);
            MESOSPHERE_ABORT_UNLESS(traverse_valid);
            traverse_valid = dst_impl.BeginTraversal(std::addressof(dst_next_entry), std::addressof(dst_context), dst_addr);
            MESOSPHERE_ABORT_UNLESS(traverse_valid);

            /* Prepare tracking variables. */
            KPhysicalAddress cur_src_block_addr = src_next_entry.phys_addr;
            KPhysicalAddress cur_dst_block_addr = dst_next_entry.phys_addr;
            size_t cur_src_size = src_next_entry.block_size - (GetInteger(cur_src_block_addr) & (src_next_entry.block_size - 1));
            size_t cur_dst_size = dst_next_entry.block_size - (GetInteger(cur_dst_block_addr) & (dst_next_entry.block_size - 1));

            /* Adjust the initial block sizes. */
            src_next_entry.block_size = cur_src_size;
            dst_next_entry.block_size = cur_dst_size;

            /* Before we get any crazier, succeed if there's nothing to do. */
            R_SUCCEED_IF(size == 0);

            /* We're going to manage dual traversal via an offset against the total size. */
            KPhysicalAddress cur_src_addr = cur_src_block_addr;
            KPhysicalAddress cur_dst_addr = cur_dst_block_addr;
            size_t cur_min_size = std::min<size_t>(cur_src_size, cur_dst_size);

            /* Iterate. */
            size_t ofs = 0;
            while (ofs < size) {
                /* Determine how much we can copy this iteration. */
                const size_t cur_copy_size = std::min<size_t>(cur_min_size, size - ofs);

                /* If we need to advance the traversals, do so. */
                bool updated_src = false, updated_dst = false, skip_copy = false;
                if (ofs + cur_copy_size != size) {
                    if (cur_src_addr + cur_min_size == cur_src_block_addr + cur_src_size) {
                        /* Continue the src traversal. */
                        traverse_valid = src_impl.ContinueTraversal(std::addressof(src_next_entry), std::addressof(src_context));
                        MESOSPHERE_ASSERT(traverse_valid);

                        /* Update source. */
                        updated_src = cur_src_addr + cur_min_size != GetInteger(src_next_entry.phys_addr);
                    }

                    if (cur_dst_addr + cur_min_size == dst_next_entry.phys_addr + dst_next_entry.block_size) {
                        /* Continue the dst traversal. */
                        traverse_valid = dst_impl.ContinueTraversal(std::addressof(dst_next_entry), std::addressof(dst_context));
                        MESOSPHERE_ASSERT(traverse_valid);

                        /* Update destination. */
                        updated_dst = cur_dst_addr + cur_min_size != GetInteger(dst_next_entry.phys_addr);
                    }

                    /* If we didn't update either of source/destination, skip the copy this iteration. */
                    if (!updated_src && !updated_dst) {
                        skip_copy = true;

                        /* Update the source block address. */
                        cur_src_block_addr = src_next_entry.phys_addr;
                    }
                }

                /* Do the copy, unless we're skipping it. */
                if (!skip_copy) {
                    /* We need both ends of the copy to be heap blocks. */
                    R_UNLESS(IsHeapPhysicalAddress(cur_src_addr), svc::ResultInvalidCurrentMemory());
                    R_UNLESS(IsHeapPhysicalAddress(cur_dst_addr), svc::ResultInvalidCurrentMemory());

                    /* Copy the data. */
                    std::memcpy(GetVoidPointer(GetHeapVirtualAddress(cur_dst_addr)), GetVoidPointer(GetHeapVirtualAddress(cur_src_addr)), cur_copy_size);

                    /* Update. */
                    cur_src_block_addr = src_next_entry.phys_addr;
                    cur_src_addr       = updated_src ? cur_src_block_addr : cur_src_addr + cur_copy_size;
                    cur_dst_block_addr = dst_next_entry.phys_addr;
                    cur_dst_addr       = updated_dst ? cur_dst_block_addr : cur_dst_addr + cur_copy_size;

                    /* Advance offset. */
                    ofs += cur_copy_size;
                }

                /* Update min size. */
                cur_src_size = src_next_entry.block_size;
                cur_dst_size = dst_next_entry.block_size;
                cur_min_size = std::min<size_t>(cur_src_block_addr - cur_src_addr + cur_src_size, cur_dst_block_addr - cur_dst_addr + cur_dst_size);
            }
        }

        R_SUCCEED();
    }

    #pragma GCC push_options
    #pragma GCC optimize ("-O3")

    Result KPageTableBase::SetupForIpcClient(PageLinkedList *page_list, size_t *out_blocks_needed, KProcessAddress address, size_t size, KMemoryPermission test_perm, KMemoryState dst_state) {
        /* Validate pre-conditions. */
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(test_perm == KMemoryPermission_UserReadWrite || test_perm == KMemoryPermission_UserRead);

        /* Check that the address is in range. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Get the source permission. */
        const auto src_perm = static_cast<KMemoryPermission>((test_perm == KMemoryPermission_UserReadWrite) ? (KMemoryPermission_KernelReadWrite | KMemoryPermission_NotMapped) : KMemoryPermission_UserRead);

        /* Get aligned extents. */
        const KProcessAddress aligned_src_start = util::AlignDown(GetInteger(address), PageSize);
        const KProcessAddress aligned_src_end   = util::AlignUp(GetInteger(address) + size, PageSize);
        const KProcessAddress mapping_src_start = util::AlignUp(GetInteger(address), PageSize);
        const KProcessAddress mapping_src_end   = util::AlignDown(GetInteger(address) + size, PageSize);

        const auto aligned_src_last = GetInteger(aligned_src_end) - 1;
        const auto mapping_src_last = GetInteger(mapping_src_end) - 1;

        /* Get the test state and attribute mask. */
        u32 test_state;
        u32 test_attr_mask;
        switch (dst_state) {
            case KMemoryState_Ipc:
                test_state     = KMemoryState_FlagCanUseIpc;
                test_attr_mask = KMemoryAttribute_All & (~(KMemoryAttribute_PermissionLocked | KMemoryAttribute_IpcLocked));
                break;
            case KMemoryState_NonSecureIpc:
                test_state     = KMemoryState_FlagCanUseNonSecureIpc;
                test_attr_mask = KMemoryAttribute_All & (~(KMemoryAttribute_PermissionLocked | KMemoryAttribute_DeviceShared | KMemoryAttribute_IpcLocked));
                break;
            case KMemoryState_NonDeviceIpc:
                test_state     = KMemoryState_FlagCanUseNonDeviceIpc;
                test_attr_mask = KMemoryAttribute_All & (~(KMemoryAttribute_PermissionLocked | KMemoryAttribute_DeviceShared | KMemoryAttribute_IpcLocked));
                break;
            default:
                R_THROW(svc::ResultInvalidCombination());
        }

        /* Ensure that on failure, we roll back appropriately. */
        size_t mapped_size = 0;
        ON_RESULT_FAILURE {
            if (mapped_size > 0) {
                this->CleanupForIpcClientOnServerSetupFailure(page_list, mapping_src_start, mapped_size, src_perm);
            }
        };

        size_t blocks_needed = 0;

        /* Iterate, mapping as needed. */
        KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(aligned_src_start);
        while (true) {
            /* Validate the current block. */
            R_TRY(this->CheckMemoryState(it, test_state, test_state, test_perm, test_perm, test_attr_mask, KMemoryAttribute_None));

            if (mapping_src_start < mapping_src_end && GetInteger(mapping_src_start) < GetInteger(it->GetEndAddress()) && GetInteger(it->GetAddress()) < GetInteger(mapping_src_end)) {
                const auto cur_start  = it->GetAddress() >= GetInteger(mapping_src_start) ? GetInteger(it->GetAddress()) : GetInteger(mapping_src_start);
                const auto cur_end    = mapping_src_last >= GetInteger(it->GetLastAddress()) ? GetInteger(it->GetEndAddress()) : GetInteger(mapping_src_end);
                const size_t cur_size = cur_end - cur_start;

                if (GetInteger(it->GetAddress()) < GetInteger(mapping_src_start)) {
                    ++blocks_needed;
                }
                if (mapping_src_last < GetInteger(it->GetLastAddress())) {
                    ++blocks_needed;
                }

                /* Set the permissions on the block, if we need to. */
                if ((it->GetPermission() & KMemoryPermission_IpcLockChangeMask) != src_perm) {
                    const DisableMergeAttribute head_body_attr = (GetInteger(mapping_src_start) >= GetInteger(it->GetAddress()))  ? DisableMergeAttribute_DisableHeadAndBody : DisableMergeAttribute_None;
                    const DisableMergeAttribute tail_attr      = (cur_end == GetInteger(mapping_src_end))                         ? DisableMergeAttribute_DisableTail        : DisableMergeAttribute_None;
                    const KPageProperties properties = { src_perm, false, false, static_cast<DisableMergeAttribute>(head_body_attr | tail_attr) };
                    R_TRY(this->Operate(page_list, cur_start, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
                }

                /* Note that we mapped this part. */
                mapped_size += cur_size;
            }

            /* If the block is at the end, we're done. */
            if (aligned_src_last <= GetInteger(it->GetLastAddress())) {
                break;
            }

            /* Advance. */
            ++it;
            MESOSPHERE_ABORT_UNLESS(it != m_memory_block_manager.end());
        }

        if (out_blocks_needed != nullptr) {
            MESOSPHERE_ASSERT(blocks_needed <= KMemoryBlockManagerUpdateAllocator::MaxBlocks);
            *out_blocks_needed = blocks_needed;
        }

        R_SUCCEED();
    }

    Result KPageTableBase::SetupForIpcServer(KProcessAddress *out_addr, size_t size, KProcessAddress src_addr, KMemoryPermission test_perm, KMemoryState dst_state, KPageTableBase &src_page_table, bool send) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(src_page_table.IsLockedByCurrentThread());

        /* Check that we can theoretically map. */
        const KProcessAddress region_start = m_region_starts[RegionType_Alias];
        const size_t          region_size  = m_region_ends[RegionType_Alias] - m_region_starts[RegionType_Alias];
        R_UNLESS(size < region_size, svc::ResultOutOfAddressSpace());

        /* Get aligned source extents. */
        const KProcessAddress         src_start = src_addr;
        const KProcessAddress         src_end   = src_addr + size;
        const KProcessAddress aligned_src_start = util::AlignDown(GetInteger(src_start), PageSize);
        const KProcessAddress aligned_src_end   = util::AlignUp(GetInteger(src_start) + size, PageSize);
        const KProcessAddress mapping_src_start = util::AlignUp(GetInteger(src_start), PageSize);
        const KProcessAddress mapping_src_end   = util::AlignDown(GetInteger(src_start) + size, PageSize);
        const size_t          aligned_src_size  = aligned_src_end - aligned_src_start;
        const size_t          mapping_src_size  = (mapping_src_start < mapping_src_end) ? (mapping_src_end - mapping_src_start) : 0;

        /* Select a random address to map at. */
        KProcessAddress dst_addr = Null<KProcessAddress>;
        for (s32 block_type = KPageTable::GetMaxBlockType(); block_type >= 0; block_type--) {
            const size_t alignment = KPageTable::GetBlockSize(static_cast<KPageTable::BlockType>(block_type));
            const size_t offset    = GetInteger(aligned_src_start) & (alignment - 1);

            dst_addr = this->FindFreeArea(region_start, region_size / PageSize, aligned_src_size / PageSize, alignment, offset, this->GetNumGuardPages());
            if (dst_addr != Null<KProcessAddress>) {
                break;
            }
        }
        R_UNLESS(dst_addr != Null<KProcessAddress>, svc::ResultOutOfAddressSpace());

        /* Check that we can perform the operation we're about to perform. */
        MESOSPHERE_ASSERT(this->CanContain(dst_addr, aligned_src_size, dst_state));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Reserve space for any partial pages we allocate. */
        const size_t unmapped_size    = aligned_src_size - mapping_src_size;
        KScopedResourceReservation memory_reservation(m_resource_limit, ams::svc::LimitableResource_PhysicalMemoryMax, unmapped_size);
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

        /* Ensure that we manage page references correctly. */
        KPhysicalAddress start_partial_page = Null<KPhysicalAddress>;
        KPhysicalAddress end_partial_page   = Null<KPhysicalAddress>;
        KProcessAddress cur_mapped_addr     = dst_addr;

        /* If the partial pages are mapped, an extra reference will have been opened. Otherwise, they'll free on scope exit. */
        ON_SCOPE_EXIT {
            if (start_partial_page != Null<KPhysicalAddress>) {
                Kernel::GetMemoryManager().Close(start_partial_page, 1);
            }
            if (end_partial_page != Null<KPhysicalAddress>) {
                Kernel::GetMemoryManager().Close(end_partial_page, 1);
            }
        };

        ON_RESULT_FAILURE {
            if (cur_mapped_addr != dst_addr) {
                const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), dst_addr, (cur_mapped_addr - dst_addr) / PageSize, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
            }
        };

        /* Allocate the start page as needed. */
        if (aligned_src_start < mapping_src_start) {
            start_partial_page = Kernel::GetMemoryManager().AllocateAndOpenContinuous(1, 1, m_allocate_option);
            R_UNLESS(start_partial_page != Null<KPhysicalAddress>, svc::ResultOutOfMemory());
        }

        /* Allocate the end page as needed. */
        if (mapping_src_end < aligned_src_end && (aligned_src_start < mapping_src_end || aligned_src_start == mapping_src_start)) {
            end_partial_page = Kernel::GetMemoryManager().AllocateAndOpenContinuous(1, 1, m_allocate_option);
            R_UNLESS(end_partial_page != Null<KPhysicalAddress>, svc::ResultOutOfMemory());
        }

        /* Get the implementation. */
        auto &src_impl = src_page_table.GetImpl();

        /* Get the fill value for partial pages. */
        const auto fill_val = m_ipc_fill_value;

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   next_entry;
        bool traverse_valid = src_impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), aligned_src_start);
        MESOSPHERE_ASSERT(traverse_valid);
        MESOSPHERE_UNUSED(traverse_valid);

        /* Prepare tracking variables. */
        KPhysicalAddress cur_block_addr = next_entry.phys_addr;
        size_t cur_block_size           = next_entry.block_size - (GetInteger(cur_block_addr) & (next_entry.block_size - 1));
        size_t tot_block_size           = cur_block_size;

        /* Map the start page, if we have one. */
        if (start_partial_page != Null<KPhysicalAddress>) {
            /* Ensure the page holds correct data. */
            const KVirtualAddress start_partial_virt = GetHeapVirtualAddress(start_partial_page);
            if (send) {
                const size_t partial_offset = src_start - aligned_src_start;
                size_t copy_size, clear_size;
                if (src_end < mapping_src_start) {
                    copy_size  = size;
                    clear_size = mapping_src_start - src_end;
                } else {
                    copy_size  = mapping_src_start - src_start;
                    clear_size = 0;
                }

                std::memset(GetVoidPointer(start_partial_virt), fill_val, partial_offset);
                std::memcpy(GetVoidPointer(start_partial_virt + partial_offset), GetVoidPointer(GetHeapVirtualAddress(cur_block_addr) + partial_offset), copy_size);
                if (clear_size > 0) {
                    std::memset(GetVoidPointer(start_partial_virt + partial_offset + copy_size), fill_val, clear_size);
                }
            } else {
                std::memset(GetVoidPointer(start_partial_virt), fill_val, PageSize);
            }

            /* Map the page. */
            const KPageProperties start_map_properties = { test_perm, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->Operate(updater.GetPageList(), cur_mapped_addr, 1, start_partial_page, true, start_map_properties, OperationType_Map, false));

            /* Update tracking extents. */
            cur_mapped_addr += PageSize;
            cur_block_addr  += PageSize;
            cur_block_size  -= PageSize;

            /* If the block's size was one page, we may need to continue traversal. */
            if (cur_block_size == 0 && aligned_src_size > PageSize) {
                traverse_valid = src_impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                MESOSPHERE_ASSERT(traverse_valid);

                cur_block_addr  = next_entry.phys_addr;
                cur_block_size  = next_entry.block_size;
                tot_block_size += next_entry.block_size;
            }
        }

        /* Map the remaining pages. */
        while (aligned_src_start + tot_block_size < mapping_src_end) {
            /* Continue the traversal. */
            traverse_valid = src_impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
            MESOSPHERE_ASSERT(traverse_valid);

            /* Process the block. */
            if (next_entry.phys_addr != cur_block_addr + cur_block_size) {
                /* Map the block we've been processing so far. */
                const KPageProperties map_properties = { test_perm, false, false, (cur_mapped_addr == dst_addr) ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };
                R_TRY(this->Operate(updater.GetPageList(), cur_mapped_addr, cur_block_size / PageSize, cur_block_addr, true, map_properties, OperationType_Map, false));

                /* Update tracking extents. */
                cur_mapped_addr += cur_block_size;
                cur_block_addr   = next_entry.phys_addr;
                cur_block_size   = next_entry.block_size;
            } else {
                cur_block_size += next_entry.block_size;
            }
            tot_block_size += next_entry.block_size;
        }

        /* Handle the last direct-mapped page. */
        if (const KProcessAddress mapped_block_end = aligned_src_start + tot_block_size - cur_block_size; mapped_block_end < mapping_src_end) {
            const size_t last_block_size = mapping_src_end - mapped_block_end;

            /* Map the last block. */
            const KPageProperties map_properties = { test_perm, false, false, (cur_mapped_addr == dst_addr) ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), cur_mapped_addr, last_block_size / PageSize, cur_block_addr, true, map_properties, OperationType_Map, false));

            /* Update tracking extents. */
            cur_mapped_addr += last_block_size;
            cur_block_addr  += last_block_size;
            if (mapped_block_end + cur_block_size < aligned_src_end && cur_block_size == last_block_size) {
                traverse_valid = src_impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
                MESOSPHERE_ASSERT(traverse_valid);

                cur_block_addr = next_entry.phys_addr;
            }
        }

        /* Map the end page, if we have one. */
        if (end_partial_page != Null<KPhysicalAddress>) {
            /* Ensure the page holds correct data. */
            const KVirtualAddress end_partial_virt = GetHeapVirtualAddress(end_partial_page);
            if (send) {
                const size_t copy_size = src_end - mapping_src_end;
                std::memcpy(GetVoidPointer(end_partial_virt), GetVoidPointer(GetHeapVirtualAddress(cur_block_addr)), copy_size);
                std::memset(GetVoidPointer(end_partial_virt + copy_size), fill_val, PageSize - copy_size);
            } else {
                std::memset(GetVoidPointer(end_partial_virt), fill_val, PageSize);
            }

            /* Map the page. */
            const KPageProperties map_properties = { test_perm, false, false, (cur_mapped_addr == dst_addr) ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), cur_mapped_addr, 1, end_partial_page, true, map_properties, OperationType_Map, false));
        }

        /* Update memory blocks to reflect our changes */
        m_memory_block_manager.Update(std::addressof(allocator), dst_addr, aligned_src_size / PageSize, dst_state, test_perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* Set the output address. */
        *out_addr = dst_addr + (src_start - aligned_src_start);

        /* We succeeded. */
        memory_reservation.Commit();
        R_SUCCEED();
    }

    Result KPageTableBase::SetupForIpc(KProcessAddress *out_dst_addr, size_t size, KProcessAddress src_addr, KPageTableBase &src_page_table, KMemoryPermission test_perm, KMemoryState dst_state, bool send) {
        /* For convenience, alias this. */
        KPageTableBase &dst_page_table = *this;

        /* Acquire the table locks. */
        KScopedLightLockPair lk(src_page_table.m_general_lock, dst_page_table.m_general_lock);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(std::addressof(src_page_table));

        /* Perform client setup. */
        size_t num_allocator_blocks;
        R_TRY(src_page_table.SetupForIpcClient(updater.GetPageList(), std::addressof(num_allocator_blocks), src_addr, size, test_perm, dst_state));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), src_page_table.m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* Get the mapped extents. */
        const KProcessAddress src_map_start = util::AlignUp(GetInteger(src_addr), PageSize);
        const KProcessAddress src_map_end   = util::AlignDown(GetInteger(src_addr) + size, PageSize);
        const size_t src_map_size           = src_map_end - src_map_start;

        /* Ensure that we clean up appropriately if we fail after this. */
        const auto src_perm = static_cast<KMemoryPermission>((test_perm == KMemoryPermission_UserReadWrite) ? (KMemoryPermission_KernelReadWrite | KMemoryPermission_NotMapped) : KMemoryPermission_UserRead);
        ON_RESULT_FAILURE {
            if (src_map_end > src_map_start) {
                src_page_table.CleanupForIpcClientOnServerSetupFailure(updater.GetPageList(), src_map_start, src_map_size, src_perm);
            }
        };

        /* Perform server setup. */
        R_TRY(dst_page_table.SetupForIpcServer(out_dst_addr, size, src_addr, test_perm, dst_state, src_page_table, send));

        /* If anything was mapped, ipc-lock the pages. */
        if (src_map_start < src_map_end) {
            /* Get the source permission. */
            src_page_table.m_memory_block_manager.UpdateLock(std::addressof(allocator), src_map_start, (src_map_end - src_map_start) / PageSize, &KMemoryBlock::LockForIpc, src_perm);
        }

        R_SUCCEED();
    }

    Result KPageTableBase::CleanupForIpcServer(KProcessAddress address, size_t size, KMemoryState dst_state) {
        /* Validate the address. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Validate the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, dst_state, KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Get aligned extents. */
        const KProcessAddress aligned_start = util::AlignDown(GetInteger(address), PageSize);
        const KProcessAddress aligned_end   = util::AlignUp(GetInteger(address) + size, PageSize);
        const size_t aligned_size           = aligned_end - aligned_start;
        const size_t aligned_num_pages      = aligned_size / PageSize;

        /* Unmap the pages. */
        const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), aligned_start, aligned_num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

        /* Update memory blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), aligned_start, aligned_num_pages, KMemoryState_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        /* Release from the resource limit as relevant. */
        const KProcessAddress mapping_start = util::AlignUp(GetInteger(address), PageSize);
        const KProcessAddress mapping_end   = util::AlignDown(GetInteger(address) + size, PageSize);
        const size_t mapping_size           = (mapping_start < mapping_end) ? mapping_end - mapping_start : 0;
        m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, aligned_size - mapping_size);

        R_SUCCEED();
    }

    Result KPageTableBase::CleanupForIpcClient(KProcessAddress address, size_t size, KMemoryState dst_state) {
        /* Validate the address. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Get aligned source extents. */
        const KProcessAddress mapping_start = util::AlignUp(GetInteger(address), PageSize);
        const KProcessAddress mapping_end   = util::AlignDown(GetInteger(address) + size, PageSize);
        const KProcessAddress mapping_last  = mapping_end - 1;
        const size_t          mapping_size  = (mapping_start < mapping_end) ? (mapping_end - mapping_start) : 0;

        /* If nothing was mapped, we're actually done immediately. */
        R_SUCCEED_IF(mapping_size == 0);

        /* Get the test state and attribute mask. */
        u32 test_state;
        u32 test_attr_mask;
        switch (dst_state) {
            case KMemoryState_Ipc:
                test_state     = KMemoryState_FlagCanUseIpc;
                test_attr_mask = KMemoryAttribute_Uncached | KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked;
                break;
            case KMemoryState_NonSecureIpc:
                test_state     = KMemoryState_FlagCanUseNonSecureIpc;
                test_attr_mask = KMemoryAttribute_Uncached | KMemoryAttribute_Locked;
                break;
            case KMemoryState_NonDeviceIpc:
                test_state     = KMemoryState_FlagCanUseNonDeviceIpc;
                test_attr_mask = KMemoryAttribute_Uncached | KMemoryAttribute_Locked;
                break;
            default:
                R_THROW(svc::ResultInvalidCombination());
        }

        /* Lock the table. */
        /* NOTE: Nintendo does this *after* creating the updater below, but this does not follow convention elsewhere in KPageTableBase. */
        KScopedLightLock lk(m_general_lock);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Ensure that on failure, we roll back appropriately. */
        size_t mapped_size = 0;
        ON_RESULT_FAILURE {
            if (mapped_size > 0) {
                /* Determine where the mapping ends. */
                const auto mapped_end  = GetInteger(mapping_start) + mapped_size;
                const auto mapped_last = mapped_end - 1;

                /* Get current and next iterators. */
                KMemoryBlockManager::const_iterator cur_it  = m_memory_block_manager.FindIterator(mapping_start);
                KMemoryBlockManager::const_iterator next_it = cur_it;
                ++next_it;

                /* Create tracking variables. */
                KProcessAddress cur_address = cur_it->GetAddress();
                size_t cur_size             = cur_it->GetSize();
                bool cur_perm_eq            = cur_it->GetPermission() == cur_it->GetOriginalPermission();
                bool cur_needs_set_perm     = !cur_perm_eq && cur_it->GetIpcLockCount() == 1;
                bool first                  = cur_it->GetIpcDisableMergeCount() == 1 && (cur_it->GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute_Locked) == 0;

                while ((GetInteger(cur_address) + cur_size - 1) < mapped_last) {
                    /* Check that we have a next block. */
                    MESOSPHERE_ABORT_UNLESS(next_it != m_memory_block_manager.end());

                    /* Check if we can consolidate the next block's permission set with the current one. */
                    const bool next_perm_eq        = next_it->GetPermission() == next_it->GetOriginalPermission();
                    const bool next_needs_set_perm = !next_perm_eq && next_it->GetIpcLockCount() == 1;
                    if (cur_perm_eq == next_perm_eq && cur_needs_set_perm == next_needs_set_perm && cur_it->GetOriginalPermission() == next_it->GetOriginalPermission()) {
                        /* We can consolidate the reprotection for the current and next block into a single call. */
                        cur_size += next_it->GetSize();
                    } else {
                        /* We have to operate on the current block. */
                        if ((cur_needs_set_perm || first) && !cur_perm_eq) {
                            const KPageProperties properties = { cur_it->GetPermission(), false, false, first ? DisableMergeAttribute_EnableAndMergeHeadBodyTail : DisableMergeAttribute_None };
                            MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), cur_address, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, true));
                        }

                        /* Advance. */
                        cur_address = next_it->GetAddress();
                        cur_size    = next_it->GetSize();
                        first       = false;
                    }

                    /* Advance. */
                    cur_perm_eq        = next_perm_eq;
                    cur_needs_set_perm = next_needs_set_perm;

                    cur_it             = next_it++;
                }

                /* Process the last block. */
                if ((first || cur_needs_set_perm) && !cur_perm_eq) {
                    const KPageProperties properties = { cur_it->GetPermission(), false, false, first ? DisableMergeAttribute_EnableAndMergeHeadBodyTail : DisableMergeAttribute_None };
                    MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), cur_address, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, true));
                }
            }
        };

        /* Iterate, reprotecting as needed. */
        {
            /* Get current and next iterators. */
            KMemoryBlockManager::const_iterator cur_it = m_memory_block_manager.FindIterator(mapping_start);
            KMemoryBlockManager::const_iterator next_it  = cur_it;
            ++next_it;

            /* Validate the current block. */
            MESOSPHERE_R_ABORT_UNLESS(this->CheckMemoryState(cur_it, test_state, test_state, KMemoryPermission_None, KMemoryPermission_None, test_attr_mask | KMemoryAttribute_IpcLocked, KMemoryAttribute_IpcLocked));

            /* Create tracking variables. */
            KProcessAddress cur_address = cur_it->GetAddress();
            size_t cur_size             = cur_it->GetSize();
            bool cur_perm_eq            = cur_it->GetPermission() == cur_it->GetOriginalPermission();
            bool cur_needs_set_perm     = !cur_perm_eq && cur_it->GetIpcLockCount() == 1;
            bool first                  = cur_it->GetIpcDisableMergeCount() == 1 && (cur_it->GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute_Locked) == 0;

            while ((cur_address + cur_size - 1) < mapping_last) {
                /* Check that we have a next block. */
                MESOSPHERE_ABORT_UNLESS(next_it != m_memory_block_manager.end());

                /* Validate the next block. */
                MESOSPHERE_R_ABORT_UNLESS(this->CheckMemoryState(next_it, test_state, test_state, KMemoryPermission_None, KMemoryPermission_None, test_attr_mask | KMemoryAttribute_IpcLocked, KMemoryAttribute_IpcLocked));

                /* Check if we can consolidate the next block's permission set with the current one. */
                const bool next_perm_eq        = next_it->GetPermission() == next_it->GetOriginalPermission();
                const bool next_needs_set_perm = !next_perm_eq && next_it->GetIpcLockCount() == 1;
                if (cur_perm_eq == next_perm_eq && cur_needs_set_perm == next_needs_set_perm && cur_it->GetOriginalPermission() == next_it->GetOriginalPermission()) {
                    /* We can consolidate the reprotection for the current and next block into a single call. */
                    cur_size += next_it->GetSize();
                } else {
                    /* We have to operate on the current block. */
                    if ((cur_needs_set_perm || first) && !cur_perm_eq) {
                        const KPageProperties properties = { cur_needs_set_perm ? cur_it->GetOriginalPermission() : cur_it->GetPermission(), false, false, first ? DisableMergeAttribute_EnableHeadAndBody : DisableMergeAttribute_None };
                        R_TRY(this->Operate(updater.GetPageList(), cur_address, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
                    }

                    /* Mark that we mapped the block. */
                    mapped_size += cur_size;

                    /* Advance. */
                    cur_address = next_it->GetAddress();
                    cur_size    = next_it->GetSize();
                    first       = false;
                }

                /* Advance. */
                cur_perm_eq        = next_perm_eq;
                cur_needs_set_perm = next_needs_set_perm;

                cur_it             = next_it++;
            }

            /* Process the last block. */
            const auto lock_count = cur_it->GetIpcLockCount() + (next_it != m_memory_block_manager.end() ? (next_it->GetIpcDisableMergeCount() - next_it->GetIpcLockCount()) : 0);
            if ((first || cur_needs_set_perm || (lock_count == 1)) && !cur_perm_eq) {
                const DisableMergeAttribute head_body_attr = first ? DisableMergeAttribute_EnableHeadAndBody : DisableMergeAttribute_None;
                const DisableMergeAttribute tail_attr      = lock_count == 1 ? DisableMergeAttribute_EnableTail : DisableMergeAttribute_None;
                const KPageProperties properties = { cur_needs_set_perm ? cur_it->GetOriginalPermission() : cur_it->GetPermission(), false, false, static_cast<DisableMergeAttribute>(head_body_attr | tail_attr) };
                R_TRY(this->Operate(updater.GetPageList(), cur_address, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
            }
        }

        /* Create an update allocator. */
        /* NOTE: Guaranteed zero blocks needed here. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, 0);
        R_TRY(allocator_result);

        /* Unlock the pages. */
        m_memory_block_manager.UpdateLock(std::addressof(allocator), mapping_start, mapping_size / PageSize, &KMemoryBlock::UnlockForIpc, KMemoryPermission_None);

        R_SUCCEED();
    }

    void KPageTableBase::CleanupForIpcClientOnServerSetupFailure(PageLinkedList *page_list, KProcessAddress address, size_t size, KMemoryPermission prot_perm) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(size, PageSize));

        /* Get the mapped extents. */
        const KProcessAddress src_map_start = address;
        const KProcessAddress src_map_end   = address + size;
        const KProcessAddress src_map_last  = src_map_end - 1;

        /* This function is only invoked when there's something to do. */
        MESOSPHERE_ASSERT(src_map_end > src_map_start);

        /* Iterate over blocks, fixing permissions. */
        KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(address);
        while (true) {
            const auto cur_start = it->GetAddress() >= GetInteger(src_map_start) ? it->GetAddress() : GetInteger(src_map_start);
            const auto cur_end   = src_map_last <= it->GetLastAddress() ? src_map_end : it->GetEndAddress();

            /* If we can, fix the protections on the block. */
            if ((it->GetIpcLockCount() == 0 && (it->GetPermission() & KMemoryPermission_IpcLockChangeMask) != prot_perm) ||
                (it->GetIpcLockCount() != 0 && (it->GetOriginalPermission() & KMemoryPermission_IpcLockChangeMask) != prot_perm))
            {
                /* Check if we actually need to fix the protections on the block. */
                if (cur_end == src_map_end || it->GetAddress() <= GetInteger(src_map_start) || (it->GetPermission() & KMemoryPermission_IpcLockChangeMask) != prot_perm) {
                    const bool start_nc = (it->GetAddress() == GetInteger(src_map_start)) ? ((it->GetDisableMergeAttribute() & (KMemoryBlockDisableMergeAttribute_Locked | KMemoryBlockDisableMergeAttribute_IpcLeft)) == 0) : it->GetAddress() <= GetInteger(src_map_start);

                    const DisableMergeAttribute head_body_attr = start_nc ? DisableMergeAttribute_EnableHeadAndBody : DisableMergeAttribute_None;
                    DisableMergeAttribute tail_attr;
                    if (cur_end == src_map_end && it->GetEndAddress() == src_map_end) {
                        auto next_it = it;
                        ++next_it;

                        const auto lock_count = it->GetIpcLockCount() + (next_it != m_memory_block_manager.end() ? (next_it->GetIpcDisableMergeCount() - next_it->GetIpcLockCount()) : 0);
                        tail_attr = lock_count == 0 ? DisableMergeAttribute_EnableTail : DisableMergeAttribute_None;
                    } else {
                        tail_attr = DisableMergeAttribute_None;
                    }

                    const KPageProperties properties = { it->GetPermission(), false, false, static_cast<DisableMergeAttribute>(head_body_attr | tail_attr) };
                    MESOSPHERE_R_ABORT_UNLESS(this->Operate(page_list, cur_start, (cur_end - cur_start) / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, true));
                }
            }

            /* If we're past the end of the region, we're done. */
            if (src_map_last <= it->GetLastAddress()) {
                break;
            }

            /* Advance. */
            ++it;
            MESOSPHERE_ABORT_UNLESS(it != m_memory_block_manager.end());
        }
    }

    #pragma GCC pop_options

    Result KPageTableBase::MapPhysicalMemory(KProcessAddress address, size_t size) {
        /* Lock the physical memory lock. */
        KScopedLightLock phys_lk(m_map_physical_memory_lock);

        /* Calculate the last address for convenience. */
        const KProcessAddress last_address = address + size - 1;

        /* Define iteration variables. */
        KProcessAddress cur_address;
        size_t mapped_size;

        /* The entire mapping process can be retried. */
        while (true) {
            /* Check if the memory is already mapped. */
            {
                /* Lock the table. */
                KScopedLightLock lk(m_general_lock);

                /* Iterate over the memory. */
                cur_address = address;
                mapped_size = 0;

                auto it = m_memory_block_manager.FindIterator(cur_address);
                while (true) {
                    /* Check that the iterator is valid. */
                    MESOSPHERE_ASSERT(it != m_memory_block_manager.end());

                    /* Check if we're done. */
                    if (last_address <= it->GetLastAddress()) {
                        if (it->GetState() != KMemoryState_Free) {
                            mapped_size += (last_address + 1 - cur_address);
                        }
                        break;
                    }

                    /* Track the memory if it's mapped. */
                    if (it->GetState() != KMemoryState_Free) {
                        mapped_size += it->GetEndAddress() - cur_address;
                    }

                    /* Advance. */
                    cur_address = it->GetEndAddress();
                    ++it;
                }

                /* If the size mapped is the size requested, we've nothing to do. */
                R_SUCCEED_IF(size == mapped_size);
            }

            /* Allocate and map the memory. */
            {
                /* Reserve the memory from the process resource limit. */
                KScopedResourceReservation memory_reservation(m_resource_limit, ams::svc::LimitableResource_PhysicalMemoryMax, size - mapped_size);
                R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

                /* Allocate pages for the new memory. */
                KPageGroup pg(m_block_info_manager);
                R_TRY(Kernel::GetMemoryManager().AllocateForProcess(std::addressof(pg), (size - mapped_size) / PageSize, m_allocate_option, GetCurrentProcess().GetId(), m_heap_fill_value));

                /* If we fail in the next bit (or retry), we need to cleanup the pages. */
                auto pg_guard = SCOPE_GUARD {
                    pg.OpenFirst();
                    pg.Close();
                };

                /* Map the memory. */
                {
                    /* Lock the table. */
                    KScopedLightLock lk(m_general_lock);

                    size_t num_allocator_blocks = 0;

                    /* Verify that nobody has mapped memory since we first checked. */
                    {
                        /* Iterate over the memory. */
                        size_t checked_mapped_size = 0;
                        cur_address = address;

                        auto it = m_memory_block_manager.FindIterator(cur_address);
                        while (true) {
                            /* Check that the iterator is valid. */
                            MESOSPHERE_ASSERT(it != m_memory_block_manager.end());

                            const bool is_free = it->GetState() == KMemoryState_Free;
                            if (is_free) {
                                if (it->GetAddress() < GetInteger(address)) {
                                    ++num_allocator_blocks;
                                }
                                if (last_address < it->GetLastAddress()) {
                                    ++num_allocator_blocks;
                                }
                            }

                            /* Check if we're done. */
                            if (last_address <= it->GetLastAddress()) {
                                if (!is_free) {
                                    checked_mapped_size += (last_address + 1 - cur_address);
                                }
                                break;
                            }

                            /* Track the memory if it's mapped. */
                            if (!is_free) {
                                checked_mapped_size += it->GetEndAddress() - cur_address;
                            }

                            /* Advance. */
                            cur_address = it->GetEndAddress();
                            ++it;
                        }

                        /* If the size now isn't what it was before, somebody mapped or unmapped concurrently. */
                        /* If this happened, retry. */
                        if (mapped_size != checked_mapped_size) {
                            continue;
                        }
                    }

                    /* Create an update allocator. */
                    MESOSPHERE_ASSERT(num_allocator_blocks <= KMemoryBlockManagerUpdateAllocator::MaxBlocks);
                    Result allocator_result;
                    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
                    R_TRY(allocator_result);

                    /* We're going to perform an update, so create a helper. */
                    KScopedPageTableUpdater updater(this);

                    /* Prepare to iterate over the memory. */
                    auto pg_it = pg.begin();
                    KPhysicalAddress pg_phys_addr = pg_it->GetAddress();
                    size_t pg_pages = pg_it->GetNumPages();

                    /* Reset the current tracking address, and make sure we clean up on failure. */
                    pg_guard.Cancel();
                    cur_address = address;
                    ON_RESULT_FAILURE {
                        if (cur_address > address) {
                            const KProcessAddress last_unmap_address = cur_address - 1;

                            /* Iterate, unmapping the pages. */
                            cur_address = address;

                            auto it = m_memory_block_manager.FindIterator(cur_address);
                            while (true) {
                                /* Check that the iterator is valid. */
                                MESOSPHERE_ASSERT(it != m_memory_block_manager.end());

                                /* If the memory state is free, we mapped it and need to unmap it. */
                                if (it->GetState() == KMemoryState_Free) {
                                    /* Determine the range to unmap. */
                                    const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                                    const size_t cur_pages = std::min(it->GetEndAddress() - cur_address, last_unmap_address + 1 - cur_address) / PageSize;

                                    /* Unmap. */
                                    MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), cur_address, cur_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
                                }

                                /* Check if we're done. */
                                if (last_unmap_address <= it->GetLastAddress()) {
                                    break;
                                }

                                /* Advance. */
                                cur_address = it->GetEndAddress();
                                ++it;
                            }
                        }

                        /* Release any remaining unmapped memory. */
                        Kernel::GetMemoryManager().OpenFirst(pg_phys_addr, pg_pages);
                        Kernel::GetMemoryManager().Close(pg_phys_addr, pg_pages);
                        for (++pg_it; pg_it != pg.end(); ++pg_it) {
                            Kernel::GetMemoryManager().OpenFirst(pg_it->GetAddress(), pg_it->GetNumPages());
                            Kernel::GetMemoryManager().Close(pg_it->GetAddress(), pg_it->GetNumPages());
                        }
                    };

                    auto it = m_memory_block_manager.FindIterator(cur_address);
                    while (true) {
                        /* Check that the iterator is valid. */
                        MESOSPHERE_ASSERT(it != m_memory_block_manager.end());

                        /* If it's unmapped, we need to map it. */
                        if (it->GetState() == KMemoryState_Free) {
                            /* Determine the range to map. */
                            const KPageProperties map_properties = { KMemoryPermission_UserReadWrite, false, false, cur_address == this->GetAliasRegionStart() ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };
                            size_t map_pages                     = std::min(it->GetEndAddress() - cur_address, last_address + 1 - cur_address) / PageSize;

                            /* While we have pages to map, map them. */
                            {
                                /* Create a page group for the current mapping range. */
                                KPageGroup cur_pg(m_block_info_manager);
                                {
                                    ON_RESULT_FAILURE {
                                        cur_pg.OpenFirst();
                                        cur_pg.Close();
                                    };

                                    size_t remain_pages = map_pages;
                                    while (remain_pages > 0) {
                                        /* Check if we're at the end of the physical block. */
                                        if (pg_pages == 0) {
                                            /* Ensure there are more pages to map. */
                                            MESOSPHERE_ASSERT(pg_it != pg.end());

                                            /* Advance our physical block. */
                                            ++pg_it;
                                            pg_phys_addr = pg_it->GetAddress();
                                            pg_pages     = pg_it->GetNumPages();
                                        }

                                        /* Add whatever we can to the current block. */
                                        const size_t cur_pages = std::min(pg_pages, remain_pages);
                                        R_TRY(cur_pg.AddBlock(pg_phys_addr + ((pg_pages - cur_pages) * PageSize), cur_pages));

                                        /* Advance. */
                                        remain_pages -= cur_pages;
                                        pg_pages     -= cur_pages;
                                    }
                                }

                                /* Map the pages. */
                                R_TRY(this->Operate(updater.GetPageList(), cur_address, map_pages, cur_pg, map_properties, OperationType_MapFirstGroup, false));
                            }
                        }

                        /* Check if we're done. */
                        if (last_address <= it->GetLastAddress()) {
                            break;
                        }

                        /* Advance. */
                        cur_address = it->GetEndAddress();
                        ++it;
                    }

                    /* We succeeded, so commit the memory reservation. */
                    memory_reservation.Commit();

                    /* Increase our tracked mapped size. */
                    m_mapped_physical_memory_size += (size - mapped_size);

                    /* Update the relevant memory blocks. */
                    m_memory_block_manager.UpdateIfMatch(std::addressof(allocator), address, size / PageSize,
                                                             KMemoryState_Free,   KMemoryPermission_None,          KMemoryAttribute_None,
                                                             KMemoryState_Normal, KMemoryPermission_UserReadWrite, KMemoryAttribute_None,
                                                             address == this->GetAliasRegionStart() ? KMemoryBlockDisableMergeAttribute_Normal : KMemoryBlockDisableMergeAttribute_None,
                                                             KMemoryBlockDisableMergeAttribute_None);

                    R_SUCCEED();
                }
            }
        }
    }

    Result KPageTableBase::UnmapPhysicalMemory(KProcessAddress address, size_t size) {
        /* Lock the physical memory lock. */
        KScopedLightLock phys_lk(m_map_physical_memory_lock);

        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Calculate the last address for convenience. */
        const KProcessAddress last_address = address + size - 1;

        /* Define iteration variables. */
        KProcessAddress map_start_address = Null<KProcessAddress>;
        KProcessAddress map_last_address  = Null<KProcessAddress>;

        KProcessAddress cur_address;
        size_t mapped_size;
        size_t num_allocator_blocks = 0;

        /* Check if the memory is mapped. */
        {
            /* Iterate over the memory. */
            cur_address = address;
            mapped_size = 0;

            auto it = m_memory_block_manager.FindIterator(cur_address);
            while (true) {
                /* Check that the iterator is valid. */
                MESOSPHERE_ASSERT(it != m_memory_block_manager.end());

                /* Verify the memory's state. */
                const bool is_normal = it->GetState() == KMemoryState_Normal && it->GetAttribute() == 0;
                const bool is_free   = it->GetState() == KMemoryState_Free;
                R_UNLESS(is_normal || is_free, svc::ResultInvalidCurrentMemory());

                if (is_normal) {
                    R_UNLESS(it->GetAttribute() == KMemoryAttribute_None, svc::ResultInvalidCurrentMemory());

                    if (map_start_address == Null<KProcessAddress>) {
                        map_start_address = cur_address;
                    }
                    map_last_address = (last_address >= it->GetLastAddress()) ? it->GetLastAddress() : last_address;

                    if (it->GetAddress() < GetInteger(address)) {
                        ++num_allocator_blocks;
                    }
                    if (last_address < it->GetLastAddress()) {
                        ++num_allocator_blocks;
                    }

                    mapped_size += (map_last_address + 1 - cur_address);
                }

                /* Check if we're done. */
                if (last_address <= it->GetLastAddress()) {
                    break;
                }

                /* Advance. */
                cur_address = it->GetEndAddress();
                ++it;
            }

            /* If there's nothing mapped, we've nothing to do. */
            R_SUCCEED_IF(mapped_size == 0);
        }

        /* Create an update allocator. */
        MESOSPHERE_ASSERT(num_allocator_blocks <= KMemoryBlockManagerUpdateAllocator::MaxBlocks);
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Separate the mapping. */
        const KPageProperties sep_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), map_start_address, (map_last_address + 1 - map_start_address) / PageSize, Null<KPhysicalAddress>, false, sep_properties, OperationType_Separate, false));

        /* Reset the current tracking address, and make sure we clean up on failure. */
        cur_address = address;

        /* Iterate over the memory, unmapping as we go. */
        auto it = m_memory_block_manager.FindIterator(cur_address);

        const auto clear_merge_attr = (it->GetState() == KMemoryState_Normal && it->GetAddress() == this->GetAliasRegionStart() && it->GetAddress() == address) ? KMemoryBlockDisableMergeAttribute_Normal : KMemoryBlockDisableMergeAttribute_None;

        while (true) {
            /* Check that the iterator is valid. */
            MESOSPHERE_ASSERT(it != m_memory_block_manager.end());

            /* If the memory state is normal, we need to unmap it. */
            if (it->GetState() == KMemoryState_Normal) {
                /* Determine the range to unmap. */
                const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                const size_t cur_pages = std::min(it->GetEndAddress() - cur_address, last_address + 1 - cur_address) / PageSize;

                /* Unmap. */
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), cur_address, cur_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));
            }

            /* Check if we're done. */
            if (last_address <= it->GetLastAddress()) {
                break;
            }

            /* Advance. */
            cur_address = it->GetEndAddress();
            ++it;
        }

        /* Release the memory resource. */
        m_mapped_physical_memory_size -= mapped_size;
        m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, mapped_size);

        /* Update memory blocks. */
        m_memory_block_manager.Update(std::addressof(allocator), address, size / PageSize, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, clear_merge_attr);

        /* We succeeded. */
        R_SUCCEED();
    }

    Result KPageTableBase::MapPhysicalMemoryUnsafe(KProcessAddress address, size_t size) {
        /* Try to reserve the unsafe memory. */
        R_UNLESS(Kernel::GetUnsafeMemory().TryReserve(size), svc::ResultLimitReached());

        /* Ensure we release our reservation on failure. */
        ON_RESULT_FAILURE { Kernel::GetUnsafeMemory().Release(size); };

        /* Create a page group for the new memory. */
        KPageGroup pg(m_block_info_manager);

        /* Allocate the new memory. */
        const size_t num_pages = size / PageSize;
        R_TRY(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(pg), num_pages, 1, KMemoryManager::EncodeOption(KMemoryManager::Pool_Unsafe, KMemoryManager::Direction_FromFront)));

        /* Close the page group when we're done with it. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Clear the new memory. */
        for (const auto &block : pg) {
            std::memset(GetVoidPointer(GetHeapVirtualAddress(block.GetAddress())), m_heap_fill_value, block.GetSize());
        }

        /* Map the new memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(m_general_lock);

            /* Check the memory state. */
            size_t num_allocator_blocks;
            R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

            /* Create an update allocator. */
            Result allocator_result;
            KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
            R_TRY(allocator_result);

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Map the pages. */
            const KPageProperties map_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->Operate(updater.GetPageList(), address, num_pages, pg, map_properties, OperationType_MapGroup, false));

            /* Apply the memory block update. */
            m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState_Normal, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

            /* Update our mapped unsafe size. */
            m_mapped_unsafe_physical_memory += size;

            /* We succeeded. */
            R_SUCCEED();
        }
    }

    Result KPageTableBase::UnmapPhysicalMemoryUnsafe(KProcessAddress address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(m_general_lock);

        /* Check whether we can unmap this much unsafe physical memory. */
        R_UNLESS(size <= m_mapped_unsafe_physical_memory, svc::ResultInvalidCurrentMemory());

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, KMemoryState_Normal, KMemoryPermission_All, KMemoryPermission_UserReadWrite, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Unmap the memory. */
        const size_t num_pages = size / PageSize;
        const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

        /* Apply the memory block update. */
        m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        /* Release the unsafe memory from the limit. */
        Kernel::GetUnsafeMemory().Release(size);

        /* Update our mapped unsafe size. */
        m_mapped_unsafe_physical_memory -= size;

        R_SUCCEED();
    }

    Result KPageTableBase::UnmapProcessMemory(KProcessAddress dst_address, size_t size, KPageTableBase &src_page_table, KProcessAddress src_address) {
        /* We need to lock both this table, and the current process's table, so set up an alias. */
        KPageTableBase &dst_page_table = *this;

        /* Acquire the table locks. */
        KScopedLightLockPair lk(src_page_table.m_general_lock, dst_page_table.m_general_lock);

        /* Check that the memory is mapped in the destination process. */
        size_t num_allocator_blocks;
        R_TRY(dst_page_table.CheckMemoryState(std::addressof(num_allocator_blocks), dst_address, size, KMemoryState_All, KMemoryState_SharedCode, KMemoryPermission_UserReadWrite, KMemoryPermission_UserReadWrite, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Check that the memory is mapped in the source process. */
        R_TRY(src_page_table.CheckMemoryState(src_address, size, KMemoryState_FlagCanMapProcess, KMemoryState_FlagCanMapProcess, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Validate that the memory ranges are compatible. */
        {
            /* Define a helper type. */
            struct ContiguousRangeInfo {
                public:
                    KPageTableBase &m_pt;
                    TraversalContext m_context;
                    TraversalEntry m_entry;
                    KPhysicalAddress m_phys_addr;
                    size_t m_cur_size;
                    size_t m_remaining_size;
                public:
                    ContiguousRangeInfo(KPageTableBase &pt, KProcessAddress address, size_t size) : m_pt(pt), m_remaining_size(size) {
                        /* Begin a traversal. */
                        MESOSPHERE_ABORT_UNLESS(m_pt.GetImpl().BeginTraversal(std::addressof(m_entry), std::addressof(m_context), address));

                        /* Setup tracking fields. */
                        m_phys_addr  = m_entry.phys_addr;
                        m_cur_size   = std::min<size_t>(m_remaining_size, m_entry.block_size - (GetInteger(m_phys_addr) & (m_entry.block_size - 1)));

                        /* Consume the whole contiguous block. */
                        this->DetermineContiguousBlockExtents();
                    }

                    void ContinueTraversal() {
                        /* Update our remaining size. */
                        m_remaining_size = m_remaining_size - m_cur_size;

                        /* Update our tracking fields. */
                        if (m_remaining_size > 0) {
                            m_phys_addr  = m_entry.phys_addr;
                            m_cur_size   = std::min<size_t>(m_remaining_size, m_entry.block_size);

                            /* Consume the whole contiguous block. */
                            this->DetermineContiguousBlockExtents();
                        }
                    }
                private:
                    void DetermineContiguousBlockExtents() {
                        /* Continue traversing until we're not contiguous, or we have enough. */
                        while (m_cur_size < m_remaining_size) {
                            MESOSPHERE_ABORT_UNLESS(m_pt.GetImpl().ContinueTraversal(std::addressof(m_entry), std::addressof(m_context)));

                            /* If we're not contiguous, we're done. */
                            if (m_entry.phys_addr != m_phys_addr + m_cur_size) {
                                break;
                            }

                            /* Update our current size. */
                            m_cur_size = std::min(m_remaining_size, m_cur_size + m_entry.block_size);
                        }
                    }
            };

            /* Create ranges for both tables. */
            ContiguousRangeInfo src_range(src_page_table, src_address, size);
            ContiguousRangeInfo dst_range(dst_page_table, dst_address, size);

            /* Validate the ranges. */
            while (src_range.m_remaining_size > 0 && dst_range.m_remaining_size > 0) {
                R_UNLESS(src_range.m_phys_addr == dst_range.m_phys_addr, svc::ResultInvalidMemoryRegion());
                R_UNLESS(src_range.m_cur_size  == dst_range.m_cur_size,  svc::ResultInvalidMemoryRegion());

                src_range.ContinueTraversal();
                dst_range.ContinueTraversal();
            }
        }

        /* We no longer need to hold our lock on the source page table. */
        lk.TryUnlockHalf(src_page_table.m_general_lock);

        /* Create an update allocator. */
        Result allocator_result;
        KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Unmap the memory. */
        const size_t num_pages = size / PageSize;
        const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), dst_address, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

        /* Apply the memory block update. */
        m_memory_block_manager.Update(std::addressof(allocator), dst_address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        R_SUCCEED();
    }

}
