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
        this->address_space_width               = (is_64_bit) ? BITSIZEOF(u64) : BITSIZEOF(u32);
        this->address_space_start               = KProcessAddress(GetInteger(start));
        this->address_space_end                 = KProcessAddress(GetInteger(end));
        this->is_kernel                         = true;
        this->enable_aslr                       = true;
        this->enable_device_address_space_merge = false;

        this->heap_region_start                 = 0;
        this->heap_region_end                   = 0;
        this->current_heap_end                  = 0;
        this->alias_region_start                = 0;
        this->alias_region_end                  = 0;
        this->stack_region_start                = 0;
        this->stack_region_end                  = 0;
        this->kernel_map_region_start           = 0;
        this->kernel_map_region_end             = 0;
        this->alias_code_region_start           = 0;
        this->alias_code_region_end             = 0;
        this->code_region_start                 = 0;
        this->code_region_end                   = 0;
        this->max_heap_size                     = 0;
        this->mapped_physical_memory_size       = 0;
        this->mapped_unsafe_physical_memory     = 0;

        this->memory_block_slab_manager         = std::addressof(Kernel::GetSystemMemoryBlockManager());
        this->block_info_manager                = std::addressof(Kernel::GetBlockInfoManager());

        this->allocate_option                   = KMemoryManager::EncodeOption(KMemoryManager::Pool_System, KMemoryManager::Direction_FromFront);
        this->heap_fill_value                   = MemoryFillValue_Zero;
        this->ipc_fill_value                    = MemoryFillValue_Zero;
        this->stack_fill_value                  = MemoryFillValue_Zero;

        this->cached_physical_linear_region     = nullptr;
        this->cached_physical_heap_region       = nullptr;
        this->cached_virtual_heap_region        = nullptr;

        /* Initialize our implementation. */
        this->impl.InitializeForKernel(table, start, end);

        /* Initialize our memory block manager. */
        return this->memory_block_manager.Initialize(this->address_space_start, this->address_space_end, this->memory_block_slab_manager);

        return ResultSuccess();
    }

    Result KPageTableBase::InitializeForProcess(ams::svc::CreateProcessFlag as_type, bool enable_aslr, bool enable_das_merge, bool from_back, KMemoryManager::Pool pool, void *table, KProcessAddress start, KProcessAddress end, KProcessAddress code_address, size_t code_size, KMemoryBlockSlabManager *mem_block_slab_manager, KBlockInfoManager *block_info_manager) {
        /* Validate the region. */
        MESOSPHERE_ABORT_UNLESS(start <= code_address);
        MESOSPHERE_ABORT_UNLESS(code_address < code_address + code_size);
        MESOSPHERE_ABORT_UNLESS(code_address + code_size - 1 <= end - 1);

        /* Declare variables to hold our region sizes. */

        /* Define helpers. */
        auto GetSpaceStart = [&](KAddressSpaceInfo::Type type) ALWAYS_INLINE_LAMBDA {
            return KAddressSpaceInfo::GetAddressSpaceStart(this->address_space_width, type);
        };
        auto GetSpaceSize = [&](KAddressSpaceInfo::Type type) ALWAYS_INLINE_LAMBDA {
            return KAddressSpaceInfo::GetAddressSpaceSize(this->address_space_width, type);
        };

        /* Set our width and heap/alias sizes. */
        this->address_space_width = GetAddressSpaceWidth(as_type);
        size_t alias_region_size  = GetSpaceSize(KAddressSpaceInfo::Type_Alias);
        size_t heap_region_size   = GetSpaceSize(KAddressSpaceInfo::Type_Heap);

        /* Adjust heap/alias size if we don't have an alias region. */
        if ((as_type & ams::svc::CreateProcessFlag_AddressSpaceMask) == ams::svc::CreateProcessFlag_AddressSpace32BitWithoutAlias) {
            heap_region_size += alias_region_size;
            alias_region_size = 0;
        }

        /* Set code regions and determine remaining sizes. */
        KProcessAddress process_code_start;
        KProcessAddress process_code_end;
        size_t stack_region_size;
        size_t kernel_map_region_size;
        if (this->address_space_width == 39) {
            alias_region_size               = GetSpaceSize(KAddressSpaceInfo::Type_Alias);
            heap_region_size                = GetSpaceSize(KAddressSpaceInfo::Type_Heap);
            stack_region_size               = GetSpaceSize(KAddressSpaceInfo::Type_Stack);
            kernel_map_region_size          = GetSpaceSize(KAddressSpaceInfo::Type_MapSmall);
            this->code_region_start         = GetSpaceStart(KAddressSpaceInfo::Type_Map39Bit);
            this->code_region_end           = this->code_region_start + GetSpaceSize(KAddressSpaceInfo::Type_Map39Bit);
            this->alias_code_region_start   = this->code_region_start;
            this->alias_code_region_end     = this->code_region_end;
            process_code_start              = util::AlignDown(GetInteger(code_address), RegionAlignment);
            process_code_end                = util::AlignUp(GetInteger(code_address) + code_size, RegionAlignment);
        } else {
            stack_region_size               = 0;
            kernel_map_region_size          = 0;
            this->code_region_start         = GetSpaceStart(KAddressSpaceInfo::Type_MapSmall);
            this->code_region_end           = this->code_region_start + GetSpaceSize(KAddressSpaceInfo::Type_MapSmall);
            this->stack_region_start        = this->code_region_start;
            this->alias_code_region_start   = this->code_region_start;
            this->alias_code_region_end     = GetSpaceStart(KAddressSpaceInfo::Type_MapLarge) + GetSpaceSize(KAddressSpaceInfo::Type_MapLarge);
            this->stack_region_end          = this->code_region_end;
            this->kernel_map_region_start   = this->code_region_start;
            this->kernel_map_region_end     = this->code_region_end;
            process_code_start              = this->code_region_start;
            process_code_end                = this->code_region_end;
        }

        /* Set other basic fields. */
        this->enable_aslr                       = enable_aslr;
        this->enable_device_address_space_merge = enable_das_merge;
        this->address_space_start               = start;
        this->address_space_end                 = end;
        this->is_kernel                         = false;
        this->memory_block_slab_manager         = mem_block_slab_manager;
        this->block_info_manager                = block_info_manager;

        /* Determine the region we can place our undetermineds in. */
        KProcessAddress alloc_start;
        size_t alloc_size;
        if ((GetInteger(process_code_start) - GetInteger(this->code_region_start)) >= (GetInteger(end) - GetInteger(process_code_end))) {
            alloc_start = this->code_region_start;
            alloc_size  = GetInteger(process_code_start) - GetInteger(this->code_region_start);
        } else {
            alloc_start = process_code_end;
            alloc_size  = GetInteger(end) - GetInteger(process_code_end);
        }
        const size_t needed_size = (alias_region_size + heap_region_size + stack_region_size + kernel_map_region_size);
        R_UNLESS(alloc_size >= needed_size, svc::ResultOutOfMemory());

        const size_t remaining_size = alloc_size - needed_size;

        /* Determine random placements for each region. */
        size_t alias_rnd = 0, heap_rnd = 0, stack_rnd = 0, kmap_rnd = 0;
        if (enable_aslr) {
            alias_rnd = KSystemControl::GenerateRandomRange(0, remaining_size / RegionAlignment) * RegionAlignment;
            heap_rnd  = KSystemControl::GenerateRandomRange(0, remaining_size / RegionAlignment) * RegionAlignment;
            stack_rnd = KSystemControl::GenerateRandomRange(0, remaining_size / RegionAlignment) * RegionAlignment;
            kmap_rnd  = KSystemControl::GenerateRandomRange(0, remaining_size / RegionAlignment) * RegionAlignment;
        }

        /* Setup heap and alias regions. */
        this->alias_region_start = alloc_start + alias_rnd;
        this->alias_region_end   = this->alias_region_start + alias_region_size;
        this->heap_region_start  = alloc_start + heap_rnd;
        this->heap_region_end    = this->heap_region_start + heap_region_size;

        if (alias_rnd <= heap_rnd) {
            this->heap_region_start  += alias_region_size;
            this->heap_region_end    += alias_region_size;
        } else {
            this->alias_region_start += heap_region_size;
            this->alias_region_end   += heap_region_size;
        }

        /* Setup stack region. */
        if (stack_region_size) {
            this->stack_region_start = alloc_start + stack_rnd;
            this->stack_region_end   = this->stack_region_start + stack_region_size;

            if (alias_rnd < stack_rnd) {
                this->stack_region_start += alias_region_size;
                this->stack_region_end   += alias_region_size;
            } else {
                this->alias_region_start += stack_region_size;
                this->alias_region_end   += stack_region_size;
            }

            if (heap_rnd < stack_rnd) {
                this->stack_region_start += heap_region_size;
                this->stack_region_end   += heap_region_size;
            } else {
                this->heap_region_start  += stack_region_size;
                this->heap_region_end    += stack_region_size;
            }
        }

        /* Setup kernel map region. */
        if (kernel_map_region_size) {
            this->kernel_map_region_start = alloc_start + kmap_rnd;
            this->kernel_map_region_end   = this->kernel_map_region_start + kernel_map_region_size;

            if (alias_rnd < kmap_rnd) {
                this->kernel_map_region_start += alias_region_size;
                this->kernel_map_region_end   += alias_region_size;
            } else {
                this->alias_region_start      += kernel_map_region_size;
                this->alias_region_end        += kernel_map_region_size;
            }

            if (heap_rnd < kmap_rnd) {
                this->kernel_map_region_start += heap_region_size;
                this->kernel_map_region_end   += heap_region_size;
            } else {
                this->heap_region_start       += kernel_map_region_size;
                this->heap_region_end         += kernel_map_region_size;
            }

            if (stack_region_size) {
                if (stack_rnd < kmap_rnd) {
                    this->kernel_map_region_start += stack_region_size;
                    this->kernel_map_region_end   += stack_region_size;
                } else {
                    this->stack_region_start      += kernel_map_region_size;
                    this->stack_region_end        += kernel_map_region_size;
                }
            }
        }

        /* Set heap and fill members. */
        this->current_heap_end              = this->heap_region_start;
        this->max_heap_size                 = 0;
        this->mapped_physical_memory_size    = 0;
        this->mapped_unsafe_physical_memory = 0;

        const bool fill_memory = KTargetSystem::IsDebugMemoryFillEnabled();
        this->heap_fill_value  = fill_memory ? MemoryFillValue_Heap  : MemoryFillValue_Zero;
        this->ipc_fill_value   = fill_memory ? MemoryFillValue_Ipc   : MemoryFillValue_Zero;
        this->stack_fill_value = fill_memory ? MemoryFillValue_Stack : MemoryFillValue_Zero;

        /* Set allocation option. */
        this->allocate_option = KMemoryManager::EncodeOption(pool, from_back ? KMemoryManager::Direction_FromBack : KMemoryManager::Direction_FromFront);

        /* Ensure that we regions inside our address space. */
        auto IsInAddressSpace = [&](KProcessAddress addr) ALWAYS_INLINE_LAMBDA { return this->address_space_start <= addr && addr <= this->address_space_end; };
        MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(this->alias_region_start));
        MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(this->alias_region_end));
        MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(this->heap_region_start));
        MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(this->heap_region_end));
        MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(this->stack_region_start));
        MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(this->stack_region_end));
        MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(this->kernel_map_region_start));
        MESOSPHERE_ABORT_UNLESS(IsInAddressSpace(this->kernel_map_region_end));

        /* Ensure that we selected regions that don't overlap. */
        const KProcessAddress alias_start = this->alias_region_start;
        const KProcessAddress alias_last  = this->alias_region_end - 1;
        const KProcessAddress heap_start  = this->heap_region_start;
        const KProcessAddress heap_last   = this->heap_region_end - 1;
        const KProcessAddress stack_start = this->stack_region_start;
        const KProcessAddress stack_last  = this->stack_region_end - 1;
        const KProcessAddress kmap_start  = this->kernel_map_region_start;
        const KProcessAddress kmap_last   = this->kernel_map_region_end - 1;
        MESOSPHERE_ABORT_UNLESS(alias_last < heap_start  || heap_last  < alias_start);
        MESOSPHERE_ABORT_UNLESS(alias_last < stack_start || stack_last < alias_start);
        MESOSPHERE_ABORT_UNLESS(alias_last < kmap_start  || kmap_last  < alias_start);
        MESOSPHERE_ABORT_UNLESS(heap_last  < stack_start || stack_last < heap_start);
        MESOSPHERE_ABORT_UNLESS(heap_last  < kmap_start  || kmap_last  < heap_start);

        /* Initialize our implementation. */
        this->impl.InitializeForProcess(table, GetInteger(start), GetInteger(end));

        /* Initialize our memory block manager. */
        return this->memory_block_manager.Initialize(this->address_space_start, this->address_space_end, this->memory_block_slab_manager);

        return ResultSuccess();
    }


    void KPageTableBase::Finalize() {
        /* Finalize memory blocks. */
        this->memory_block_manager.Finalize(this->memory_block_slab_manager);

        /* Free any unsafe mapped memory. */
        if (this->mapped_unsafe_physical_memory) {
            Kernel::GetUnsafeMemory().Release(this->mapped_unsafe_physical_memory);
        }

        /* Invalidate the entire instruction cache. */
        cpu::InvalidateEntireInstructionCache();
    }

    KProcessAddress KPageTableBase::GetRegionAddress(KMemoryState state) const {
        switch (state) {
            case KMemoryState_Free:
            case KMemoryState_Kernel:
                return this->address_space_start;
            case KMemoryState_Normal:
                return this->heap_region_start;
            case KMemoryState_Ipc:
            case KMemoryState_NonSecureIpc:
            case KMemoryState_NonDeviceIpc:
                return this->alias_region_start;
            case KMemoryState_Stack:
                return this->stack_region_start;
            case KMemoryState_Io:
            case KMemoryState_Static:
            case KMemoryState_ThreadLocal:
                return this->kernel_map_region_start;
            case KMemoryState_Shared:
            case KMemoryState_AliasCode:
            case KMemoryState_AliasCodeData:
            case KMemoryState_Transfered:
            case KMemoryState_SharedTransfered:
            case KMemoryState_SharedCode:
            case KMemoryState_GeneratedCode:
            case KMemoryState_CodeOut:
                return this->alias_code_region_start;
            case KMemoryState_Code:
            case KMemoryState_CodeData:
                return this->code_region_start;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }
    }

    size_t KPageTableBase::GetRegionSize(KMemoryState state) const {
        switch (state) {
            case KMemoryState_Free:
            case KMemoryState_Kernel:
                return this->address_space_end - this->address_space_start;
            case KMemoryState_Normal:
                return this->heap_region_end - this->heap_region_start;
            case KMemoryState_Ipc:
            case KMemoryState_NonSecureIpc:
            case KMemoryState_NonDeviceIpc:
                return this->alias_region_end - this->alias_region_start;
            case KMemoryState_Stack:
                return this->stack_region_end - this->stack_region_start;
            case KMemoryState_Io:
            case KMemoryState_Static:
            case KMemoryState_ThreadLocal:
                return this->kernel_map_region_end - this->kernel_map_region_start;
            case KMemoryState_Shared:
            case KMemoryState_AliasCode:
            case KMemoryState_AliasCodeData:
            case KMemoryState_Transfered:
            case KMemoryState_SharedTransfered:
            case KMemoryState_SharedCode:
            case KMemoryState_GeneratedCode:
            case KMemoryState_CodeOut:
                return this->alias_code_region_end - this->alias_code_region_start;
            case KMemoryState_Code:
            case KMemoryState_CodeData:
                return this->code_region_end - this->code_region_start;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool KPageTableBase::CanContain(KProcessAddress addr, size_t size, KMemoryState state) const {
        const KProcessAddress end = addr + size;
        const KProcessAddress last = end - 1;

        const KProcessAddress region_start = this->GetRegionAddress(state);
        const size_t region_size           = this->GetRegionSize(state);

        const bool is_in_region = region_start <= addr && addr < end && last <= region_start + region_size - 1;
        const bool is_in_heap   = !(end <= this->heap_region_start || this->heap_region_end <= addr);
        const bool is_in_alias  = !(end <= this->alias_region_start || this->alias_region_end <= addr);
        switch (state) {
            case KMemoryState_Free:
            case KMemoryState_Kernel:
                return is_in_region;
            case KMemoryState_Io:
            case KMemoryState_Static:
            case KMemoryState_Code:
            case KMemoryState_CodeData:
            case KMemoryState_Shared:
            case KMemoryState_AliasCode:
            case KMemoryState_AliasCodeData:
            case KMemoryState_Stack:
            case KMemoryState_ThreadLocal:
            case KMemoryState_Transfered:
            case KMemoryState_SharedTransfered:
            case KMemoryState_SharedCode:
            case KMemoryState_GeneratedCode:
            case KMemoryState_CodeOut:
                return is_in_region && !is_in_heap && !is_in_alias;
            case KMemoryState_Normal:
                MESOSPHERE_ASSERT(is_in_heap);
                return is_in_region && !is_in_alias;
            case KMemoryState_Ipc:
            case KMemoryState_NonSecureIpc:
            case KMemoryState_NonDeviceIpc:
                MESOSPHERE_ASSERT(is_in_alias);
                return is_in_region && !is_in_heap;
            default:
                return false;
        }
    }

    Result KPageTableBase::CheckMemoryState(const KMemoryInfo &info, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const {
        /* Validate the states match expectation. */
        R_UNLESS((info.state     & state_mask) == state, svc::ResultInvalidCurrentMemory());
        R_UNLESS((info.perm      & perm_mask)  == perm,  svc::ResultInvalidCurrentMemory());
        R_UNLESS((info.attribute & attr_mask)  == attr,  svc::ResultInvalidCurrentMemory());

        return ResultSuccess();
    }

    Result KPageTableBase::CheckMemoryStateContiguous(size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) const {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Get information about the first block. */
        const KProcessAddress last_addr = addr + size - 1;
        KMemoryBlockManager::const_iterator it = this->memory_block_manager.FindIterator(addr);
        KMemoryInfo info = it->GetMemoryInfo();

        /* If the start address isn't aligned, we need a block. */
        const size_t blocks_for_start_align = (util::AlignDown(GetInteger(addr), PageSize) != info.GetAddress()) ? 1 : 0;

        while (true) {
            /* Validate against the provided masks. */
            R_TRY(this->CheckMemoryState(info, state_mask, state, perm_mask, perm, attr_mask, attr));

            /* Break once we're done. */
            if (last_addr <= info.GetLastAddress()) {
                break;
            }

            /* Advance our iterator. */
            it++;
            MESOSPHERE_ASSERT(it != this->memory_block_manager.cend());
            info = it->GetMemoryInfo();
        }

        /* If the end address isn't aligned, we need a block. */
        const size_t blocks_for_end_align = (util::AlignUp(GetInteger(addr) + size, PageSize) != info.GetEndAddress()) ? 1 : 0;

        if (out_blocks_needed != nullptr) {
            *out_blocks_needed = blocks_for_start_align + blocks_for_end_align;
        }

        return ResultSuccess();
    }

    Result KPageTableBase::CheckMemoryState(KMemoryState *out_state, KMemoryPermission *out_perm, KMemoryAttribute *out_attr, size_t *out_blocks_needed, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr) const {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Get information about the first block. */
        const KProcessAddress last_addr = addr + size - 1;
        KMemoryBlockManager::const_iterator it = this->memory_block_manager.FindIterator(addr);
        KMemoryInfo info = it->GetMemoryInfo();

        /* If the start address isn't aligned, we need a block. */
        const size_t blocks_for_start_align = (util::AlignDown(GetInteger(addr), PageSize) != info.GetAddress()) ? 1 : 0;

        /* Validate all blocks in the range have correct state. */
        const KMemoryState      first_state = info.state;
        const KMemoryPermission first_perm  = info.perm;
        const KMemoryAttribute  first_attr  = info.attribute;
        while (true) {
            /* Validate the current block. */
            R_UNLESS(info.state == first_state,                                    svc::ResultInvalidCurrentMemory());
            R_UNLESS(info.perm  == first_perm,                                     svc::ResultInvalidCurrentMemory());
            R_UNLESS((info.attribute | ignore_attr) == (first_attr | ignore_attr), svc::ResultInvalidCurrentMemory());

            /* Validate against the provided masks. */
            R_TRY(this->CheckMemoryState(info, state_mask, state, perm_mask, perm, attr_mask, attr));

            /* Break once we're done. */
            if (last_addr <= info.GetLastAddress()) {
                break;
            }

            /* Advance our iterator. */
            it++;
            MESOSPHERE_ASSERT(it != this->memory_block_manager.cend());
            info = it->GetMemoryInfo();
        }

        /* If the end address isn't aligned, we need a block. */
        const size_t blocks_for_end_align = (util::AlignUp(GetInteger(addr) + size, PageSize) != info.GetEndAddress()) ? 1 : 0;

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
        if (out_blocks_needed != nullptr) {
            *out_blocks_needed = blocks_for_start_align + blocks_for_end_align;
        }
        return ResultSuccess();
    }

    Result KPageTableBase::LockMemoryAndOpen(KPageGroup *out_pg, KPhysicalAddress *out_paddr, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, KMemoryPermission new_perm, u32 lock_attr) {
        /* Validate basic preconditions. */
        MESOSPHERE_ASSERT((lock_attr & attr) == 0);
        MESOSPHERE_ASSERT((lock_attr & (KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared)) == 0);

        /* Validate the lock request. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(addr, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

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
            MESOSPHERE_ABORT_UNLESS(this->GetPhysicalAddress(out_paddr, addr));
        }

        /* Make the page group, if we're supposed to. */
        if (out_pg != nullptr) {
            R_TRY(this->MakePageGroup(*out_pg, addr, num_pages));
        }

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

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
        this->memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, new_perm, new_attr, KMemoryBlockDisableMergeAttribute_Locked, KMemoryBlockDisableMergeAttribute_None);

        /* If we have an output group, open. */
        if (out_pg) {
            out_pg->Open();
        }

        return ResultSuccess();
    }

    Result KPageTableBase::UnlockMemory(KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, KMemoryPermission new_perm, u32 lock_attr, const KPageGroup *pg) {
        /* Validate basic preconditions. */
        MESOSPHERE_ASSERT((attr_mask & lock_attr) == lock_attr);
        MESOSPHERE_ASSERT((attr      & lock_attr) == lock_attr);

        /* Validate the unlock request. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(addr, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

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
        /* NOTE: Nintendo does not initialize the allocator with any blocks. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(0));
        MESOSPHERE_UNUSED(num_allocator_blocks);

        /* Update permission, if we need to. */
        if (new_perm != old_perm) {
            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            const KPageProperties properties = { new_perm, false, (old_attr & KMemoryAttribute_Uncached) != 0, DisableMergeAttribute_EnableAndMergeHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
        }

        /* Apply the memory block updates. */
        this->memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, new_perm, new_attr, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Locked);

        return ResultSuccess();
    }

    Result KPageTableBase::QueryInfoImpl(KMemoryInfo *out_info, ams::svc::PageInfo *out_page, KProcessAddress address) const {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(out_info != nullptr);
        MESOSPHERE_ASSERT(out_page != nullptr);

        const KMemoryBlock *block = this->memory_block_manager.FindBlock(address);
        R_UNLESS(block != nullptr, svc::ResultInvalidCurrentMemory());

        *out_info = block->GetMemoryInfo();
        out_page->flags = 0;
        return ResultSuccess();
    }

    Result KPageTableBase::QueryMappingImpl(KProcessAddress *out, KPhysicalAddress address, size_t size, KMemoryState state) const {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(out != nullptr);

        const KProcessAddress region_start = this->GetRegionAddress(state);
        const size_t region_size           = this->GetRegionSize(state);

        /* Check that the address/size are potentially valid. */
        R_UNLESS((address < address + size), svc::ResultNotFound());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        auto &impl = this->GetImpl();

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   cur_entry  = {};
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
                    if (R_SUCCEEDED(this->CheckMemoryState(mapped_address, size, KMemoryState_All, state, KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None))) {
                        /* It is! */
                        *out = mapped_address;
                        return ResultSuccess();
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
        return ResultSuccess();
    }

    Result KPageTableBase::MapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Validate that the source address's state is valid. */
        KMemoryState src_state;
        size_t num_src_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(src_state), nullptr, nullptr, std::addressof(num_src_allocator_blocks), src_address, size, KMemoryState_FlagCanAlias, KMemoryState_FlagCanAlias, KMemoryPermission_All, KMemoryPermission_UserReadWrite, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Validate that the dst address's state is valid. */
        size_t num_dst_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator for the source. */
        KMemoryBlockManagerUpdateAllocator src_allocator(this->memory_block_slab_manager);
        R_TRY(src_allocator.Initialize(num_src_allocator_blocks));

        /* Create an update allocator for the destination. */
        KMemoryBlockManagerUpdateAllocator dst_allocator(this->memory_block_slab_manager);
        R_TRY(dst_allocator.Initialize(num_dst_allocator_blocks));

        /* Map the memory. */
        {
            /* Determine the number of pages being operated on. */
            const size_t num_pages = size / PageSize;

            /* Create page groups for the memory being unmapped. */
            KPageGroup pg(this->block_info_manager);

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
            auto unprot_guard = SCOPE_GUARD {
                const KPageProperties unprotect_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_EnableHeadBodyTail };
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, unprotect_properties, OperationType_ChangePermissions, true));
            };

            /* Map the alias pages. */
            const KPageProperties dst_map_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->MapPageGroupImpl(updater.GetPageList(), dst_address, pg, dst_map_properties, false));

            /* We successfully mapped the alias pages, so we don't need to unprotect the src pages on failure. */
            unprot_guard.Cancel();

            /* Apply the memory block updates. */
            this->memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages, src_state, new_src_perm, new_src_attr, KMemoryBlockDisableMergeAttribute_Locked, KMemoryBlockDisableMergeAttribute_None);
            this->memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages, KMemoryState_Stack, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);
        }

        return ResultSuccess();
    }

    Result KPageTableBase::UnmapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Validate that the source address's state is valid. */
        KMemoryState src_state;
        size_t num_src_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(src_state), nullptr, nullptr, std::addressof(num_src_allocator_blocks), src_address, size, KMemoryState_FlagCanAlias, KMemoryState_FlagCanAlias, KMemoryPermission_All, KMemoryPermission_NotMapped | KMemoryPermission_KernelRead, KMemoryAttribute_All, KMemoryAttribute_Locked));

        /* Validate that the dst address's state is valid. */
        KMemoryPermission dst_perm;
        size_t num_dst_allocator_blocks;
        R_TRY(this->CheckMemoryState(nullptr, std::addressof(dst_perm), nullptr, std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState_All, KMemoryState_Stack, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator for the source. */
        KMemoryBlockManagerUpdateAllocator src_allocator(this->memory_block_slab_manager);
        R_TRY(src_allocator.Initialize(num_src_allocator_blocks));

        /* Create an update allocator for the destination. */
        KMemoryBlockManagerUpdateAllocator dst_allocator(this->memory_block_slab_manager);
        R_TRY(dst_allocator.Initialize(num_dst_allocator_blocks));

        /* Unmap the memory. */
        {
            /* Determine the number of pages being operated on. */
            const size_t num_pages = size / PageSize;

            /* Create page groups for the memory being unmapped. */
            KPageGroup pg(this->block_info_manager);

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
            auto remap_guard = SCOPE_GUARD {
                this->RemapPageGroup(updater.GetPageList(), dst_address, size, pg);
            };

            /* Try to set the permissions for the source pages back to what they should be. */
            const KPageProperties src_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_EnableAndMergeHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, src_properties, OperationType_ChangePermissions, false));

            /* We successfully changed the permissions for the source pages, so we don't need to re-map the dst pages on failure. */
            remap_guard.Cancel();

            /* Apply the memory block updates. */
            this->memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages, src_state,         KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Locked);
            this->memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages, KMemoryState_None, KMemoryPermission_None,          KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);
        }

        return ResultSuccess();
    }

    Result KPageTableBase::MapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
        /* Validate the mapping request. */
        R_UNLESS(this->CanContain(dst_address, size, KMemoryState_AliasCode), svc::ResultInvalidMemoryRegion());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Verify that the source memory is normal heap. */
        KMemoryState src_state;
        KMemoryPermission src_perm;
        size_t num_src_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(src_state), std::addressof(src_perm), nullptr, std::addressof(num_src_allocator_blocks), src_address, size, KMemoryState_All, KMemoryState_Normal, KMemoryPermission_All, KMemoryPermission_UserReadWrite, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Verify that the destination memory is unmapped. */
        size_t num_dst_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator for the source. */
        KMemoryBlockManagerUpdateAllocator src_allocator(this->memory_block_slab_manager);
        R_TRY(src_allocator.Initialize(num_src_allocator_blocks));

        /* Create an update allocator for the destination. */
        KMemoryBlockManagerUpdateAllocator dst_allocator(this->memory_block_slab_manager);
        R_TRY(dst_allocator.Initialize(num_dst_allocator_blocks));

        /* Map the code memory. */
        {
            /* Determine the number of pages being operated on. */
            const size_t num_pages = size / PageSize;

            /* Create page groups for the memory being unmapped. */
            KPageGroup pg(this->block_info_manager);

            /* Create the page group representing the source. */
            R_TRY(this->MakePageGroup(pg, src_address, num_pages));

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Reprotect the source as kernel-read/not mapped. */
            const KMemoryPermission new_perm = static_cast<KMemoryPermission>(KMemoryPermission_KernelRead | KMemoryPermission_NotMapped);
            const KPageProperties src_properties = { new_perm, false, false, DisableMergeAttribute_DisableHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, src_properties, OperationType_ChangePermissions, false));

            /* Ensure that we unprotect the source pages on failure. */
            auto unprot_guard = SCOPE_GUARD {
                const KPageProperties unprotect_properties = { src_perm, false, false, DisableMergeAttribute_EnableHeadBodyTail };
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, unprotect_properties, OperationType_ChangePermissions, true));
            };

            /* Map the alias pages. */
            const KPageProperties dst_properties = { new_perm, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->MapPageGroupImpl(updater.GetPageList(), dst_address, pg, dst_properties, false));

            /* We successfully mapped the alias pages, so we don't need to unprotect the src pages on failure. */
            unprot_guard.Cancel();

            /* Apply the memory block updates. */
            this->memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages, src_state,              new_perm, KMemoryAttribute_Locked, KMemoryBlockDisableMergeAttribute_Locked, KMemoryBlockDisableMergeAttribute_None);
            this->memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages, KMemoryState_AliasCode, new_perm, KMemoryAttribute_None,   KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);
        }

        return ResultSuccess();
    }

    Result KPageTableBase::UnmapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
        /* Validate the mapping request. */
        R_UNLESS(this->CanContain(dst_address, size, KMemoryState_AliasCode), svc::ResultInvalidMemoryRegion());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Verify that the source memory is locked normal heap. */
        size_t num_src_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_src_allocator_blocks), src_address, size, KMemoryState_All, KMemoryState_Normal, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_Locked));

        /* Verify that the destination memory is aliasable code. */
        size_t num_dst_allocator_blocks;
        R_TRY(this->CheckMemoryStateContiguous(std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState_FlagCanCodeAlias, KMemoryState_FlagCanCodeAlias, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Determine whether any pages being unmapped are code. */
        bool any_code_pages = false;
        {
            KMemoryBlockManager::const_iterator it = this->memory_block_manager.FindIterator(dst_address);
            while (true) {
                /* Get the memory info. */
                const KMemoryInfo info = it->GetMemoryInfo();

                /* Check if the memory has code flag. */
                if ((info.GetState() & KMemoryState_FlagCode) != 0) {
                    any_code_pages = true;
                    break;
                }

                /* Check if we're done. */
                if (dst_address + size - 1 <= info.GetLastAddress()) {
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
            KPageGroup pg(this->block_info_manager);

            /* Create the page group representing the destination. */
            R_TRY(this->MakePageGroup(pg, dst_address, num_pages));

            /* Verify that the page group contains the same pages as the source. */
            R_UNLESS(this->IsValidPageGroup(pg, src_address, num_pages), svc::ResultInvalidMemoryRegion());

            /* Create an update allocator for the source. */
            KMemoryBlockManagerUpdateAllocator src_allocator(this->memory_block_slab_manager);
            R_TRY(src_allocator.Initialize(num_src_allocator_blocks));

            /* Create an update allocator for the destination. */
            KMemoryBlockManagerUpdateAllocator dst_allocator(this->memory_block_slab_manager);
            R_TRY(dst_allocator.Initialize(num_dst_allocator_blocks));

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Unmap the aliased copy of the pages. */
            const KPageProperties dst_unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), dst_address, num_pages, Null<KPhysicalAddress>, false, dst_unmap_properties, OperationType_Unmap, false));

            /* Ensure that we re-map the aliased pages on failure. */
            auto remap_guard = SCOPE_GUARD {
                this->RemapPageGroup(updater.GetPageList(), dst_address, size, pg);
            };

            /* Try to set the permissions for the source pages back to what they should be. */
            const KPageProperties src_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_EnableAndMergeHeadBodyTail };
            R_TRY(this->Operate(updater.GetPageList(), src_address, num_pages, Null<KPhysicalAddress>, false, src_properties, OperationType_ChangePermissions, false));

            /* We successfully changed the permissions for the source pages, so we don't need to re-map the dst pages on failure. */
            remap_guard.Cancel();

            /* Apply the memory block updates. */
            this->memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages, KMemoryState_None,   KMemoryPermission_None,          KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);
            this->memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages, KMemoryState_Normal, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Locked);

            /* Note that we reprotected pages. */
            reprotected_pages = true;
        }

        return ResultSuccess();
    }

    KProcessAddress KPageTableBase::FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const {
        KProcessAddress address = Null<KProcessAddress>;

        if (num_pages <= region_num_pages) {
            if (this->IsAslrEnabled()) {
                /* Try to directly find a free area up to 8 times. */
                for (size_t i = 0; i < 8; i++) {
                    const size_t random_offset = KSystemControl::GenerateRandomRange(0, (region_num_pages - num_pages - guard_pages) * PageSize / alignment) * alignment;
                    const KProcessAddress candidate = util::AlignDown(GetInteger(region_start + random_offset), alignment) + offset;

                    KMemoryInfo info;
                    ams::svc::PageInfo page_info;
                    MESOSPHERE_R_ABORT_UNLESS(this->QueryInfoImpl(&info, &page_info, candidate));

                    if (info.state != KMemoryState_Free) { continue; }
                    if (!(region_start <= candidate)) { continue; }
                    if (!(info.GetAddress() + guard_pages * PageSize <= GetInteger(candidate))) { continue; }
                    if (!(candidate + (num_pages + guard_pages) * PageSize - 1 <= info.GetLastAddress())) { continue; }
                    if (!(candidate + (num_pages + guard_pages) * PageSize - 1 <= region_start + region_num_pages * PageSize - 1)) { continue; }

                    address = candidate;
                    break;
                }
                /* Fall back to finding the first free area with a random offset. */
                if (address == Null<KProcessAddress>) {
                    /* NOTE: Nintendo does not account for guard pages here. */
                    /* This may theoretically cause an offset to be chosen that cannot be mapped. */
                    /* We will account for guard pages. */
                    const size_t offset_pages = KSystemControl::GenerateRandomRange(0, region_num_pages - num_pages - guard_pages);
                    address = this->memory_block_manager.FindFreeArea(region_start + offset_pages * PageSize, region_num_pages - offset_pages, num_pages, alignment, offset, guard_pages);
                }
            }
            /* Find the first free area. */
            if (address == Null<KProcessAddress>) {
                address = this->memory_block_manager.FindFreeArea(region_start, region_num_pages, num_pages, alignment, offset, guard_pages);
            }
        }

        return address;
    }

    size_t KPageTableBase::GetSize(KMemoryState state) const {
        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Iterate, counting blocks with the desired state. */
        size_t total_size = 0;
        for (KMemoryBlockManager::const_iterator it = this->memory_block_manager.FindIterator(this->address_space_start); it != this->memory_block_manager.end(); ++it) {
            /* Get the memory info. */
            const KMemoryInfo info = it->GetMemoryInfo();
            if (info.GetState() == state) {
                total_size += info.GetSize();
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

    Result KPageTableBase::AllocateAndMapPagesImpl(PageLinkedList *page_list, KProcessAddress address, size_t num_pages, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Create a page group to hold the pages we allocate. */
        KPageGroup pg(this->block_info_manager);

        /* Allocate the pages. */
        R_TRY(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(pg), num_pages, this->allocate_option));

        /* Ensure that the page group is closed when we're done working with it. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Clear all pages. */
        for (const auto &it : pg) {
            std::memset(GetVoidPointer(it.GetAddress()), this->heap_fill_value, it.GetSize());
        }

        /* Map the pages. */
        const KPageProperties properties = { perm, false, false, DisableMergeAttribute_None };
        return this->Operate(page_list, address, num_pages, pg, properties, OperationType_MapGroup, false);
    }

    Result KPageTableBase::MapPageGroupImpl(PageLinkedList *page_list, KProcessAddress address, const KPageGroup &pg, const KPageProperties properties, bool reuse_ll) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Note the current address, so that we can iterate. */
        const KProcessAddress start_address = address;
        KProcessAddress cur_address = address;

        /* Ensure that we clean up on failure. */
        auto mapping_guard = SCOPE_GUARD {
            MESOSPHERE_ABORT_UNLESS(!reuse_ll);
            if (cur_address != start_address) {
                const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(page_list, start_address, (cur_address - start_address) / PageSize, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
            }
        };

        /* Iterate, mapping all pages in the group. */
        for (const auto &block : pg) {
            /* We only allow mapping pages in the heap, and we require we're mapping non-empty blocks. */
            MESOSPHERE_ABORT_UNLESS(block.GetAddress() < block.GetLastAddress());
            MESOSPHERE_ABORT_UNLESS(IsHeapVirtualAddress(block.GetAddress(), block.GetSize()));

            /* Map and advance. */
            const KPageProperties cur_properties = (cur_address == start_address) ? properties : KPageProperties{ properties.perm, properties.io, properties.uncached, DisableMergeAttribute_None };
            R_TRY(this->Operate(page_list, cur_address, block.GetNumPages(), GetHeapPhysicalAddress(block.GetAddress()), true, cur_properties, OperationType_Map, reuse_ll));
            cur_address += block.GetSize();
        }

        /* We succeeded! */
        mapping_guard.Cancel();
        return ResultSuccess();
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

        KPhysicalAddress pg_phys_addr = GetHeapPhysicalAddress(pg_it->GetAddress());
        size_t pg_pages = pg_it->GetNumPages();

        auto it = this->memory_block_manager.FindIterator(start_address);
        while (true) {
            /* Check that the iterator is valid. */
            MESOSPHERE_ASSERT(it != this->memory_block_manager.end());

            /* Get the memory info. */
            const KMemoryInfo info = it->GetMemoryInfo();

            /* Determine the range to map. */
                  KProcessAddress map_address     = std::max(info.GetAddress(), GetInteger(start_address));
            const KProcessAddress map_end_address = std::min(info.GetEndAddress(), GetInteger(end_address));
            MESOSPHERE_ABORT_UNLESS(map_end_address != map_address);

            /* Determine if we should disable head merge. */
            const bool disable_head_merge = info.GetAddress() >= GetInteger(start_address) && (info.GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute_Normal) != 0;
            const KPageProperties map_properties = { info.GetPermission(), false, false, disable_head_merge ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };

            /* While we have pages to map, map them. */
            size_t map_pages = (map_end_address - map_address) / PageSize;
            while (map_pages > 0) {
                /* Check if we're at the end of the physical block. */
                if (pg_pages == 0) {
                    /* Ensure there are more pages to map. */
                    MESOSPHERE_ABORT_UNLESS(pg_it != pg.end());

                    /* Advance our physical block. */
                    ++pg_it;
                    pg_phys_addr = GetHeapPhysicalAddress(pg_it->GetAddress());
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
            if (last_address <= info.GetLastAddress()) {
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
                R_TRY(pg.AddBlock(GetHeapVirtualAddress(cur_addr), cur_pages));

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
        R_TRY(pg.AddBlock(GetHeapVirtualAddress(cur_addr), cur_pages));

        return ResultSuccess();
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
        KVirtualAddress cur_block_address = cur_it->GetAddress();
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

                if (cur_block_address != GetHeapVirtualAddress(cur_addr) || cur_block_pages < cur_pages) {
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

        return cur_block_address == GetHeapVirtualAddress(cur_addr) && cur_block_pages == (cur_size / PageSize);
    }

    Result KPageTableBase::SetMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission svc_perm) {
        const size_t num_pages = size / PageSize;

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Verify we can change the memory permission. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), nullptr, std::addressof(num_allocator_blocks), addr, size, KMemoryState_FlagCanReprotect, KMemoryState_FlagCanReprotect, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Determine new perm. */
        const KMemoryPermission new_perm = ConvertToKMemoryPermission(svc_perm);
        R_SUCCEED_IF(old_perm == new_perm);

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { new_perm, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, old_state, new_perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);

        return ResultSuccess();
    }

    Result KPageTableBase::SetProcessMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission svc_perm) {
        const size_t num_pages = size / PageSize;

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Verify we can change the memory permission. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), nullptr, std::addressof(num_allocator_blocks), addr, size, KMemoryState_FlagCode, KMemoryState_FlagCode, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Make a new page group for the region. */
        KPageGroup pg(this->block_info_manager);

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
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { new_perm, false, false, DisableMergeAttribute_None };
        const auto operation = was_x ? OperationType_ChangePermissionsAndRefresh : OperationType_ChangePermissions;
        R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, operation, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, new_state, new_perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);

        /* Ensure cache coherency, if we're setting pages as executable. */
        if (is_x) {
            for (const auto &block : pg) {
                cpu::StoreDataCache(GetVoidPointer(block.GetAddress()), block.GetSize());
            }
            cpu::InvalidateEntireInstructionCache();
        }

        return ResultSuccess();
    }

    Result KPageTableBase::SetMemoryAttribute(KProcessAddress addr, size_t size, u32 mask, u32 attr) {
        const size_t num_pages = size / PageSize;
        MESOSPHERE_ASSERT((mask | KMemoryAttribute_SetMask) == KMemoryAttribute_SetMask);

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Verify we can change the memory attribute. */
        KMemoryState old_state;
        KMemoryPermission old_perm;
        KMemoryAttribute old_attr;
        size_t num_allocator_blocks;
        constexpr u32 AttributeTestMask = ~(KMemoryAttribute_SetMask | KMemoryAttribute_DeviceShared);
        R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), std::addressof(old_attr), std::addressof(num_allocator_blocks),
                                     addr, size,
                                     KMemoryState_FlagCanChangeAttribute, KMemoryState_FlagCanChangeAttribute,
                                     KMemoryPermission_None, KMemoryPermission_None,
                                     AttributeTestMask, KMemoryAttribute_None, ~AttributeTestMask));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Determine the new attribute. */
        const KMemoryAttribute new_attr = static_cast<KMemoryAttribute>(((old_attr & ~mask) | (attr & mask)));

        /* Perform operation. */
        const KPageProperties properties = { old_perm, false, (new_attr & KMemoryAttribute_Uncached) != 0, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissionsAndRefresh, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, old_state, old_perm, new_attr, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);

        return ResultSuccess();
    }

    Result KPageTableBase::SetHeapSize(KProcessAddress *out, size_t size) {
        /* Lock the physical memory mutex. */
        KScopedLightLock map_phys_mem_lk(this->map_physical_memory_lock);

        /* Try to perform a reduction in heap, instead of an extension. */
        KProcessAddress cur_address;
        size_t allocation_size;
        {
            /* Lock the table. */
            KScopedLightLock lk(this->general_lock);

            /* Validate that setting heap size is possible at all. */
            R_UNLESS(!this->is_kernel,                                                             svc::ResultOutOfMemory());
            R_UNLESS(size <= static_cast<size_t>(this->heap_region_end - this->heap_region_start), svc::ResultOutOfMemory());
            R_UNLESS(size <= this->max_heap_size,                                                  svc::ResultOutOfMemory());

            if (size < static_cast<size_t>(this->current_heap_end - this->heap_region_start)) {
                /* The size being requested is less than the current size, so we need to free the end of the heap. */

                /* Validate memory state. */
                size_t num_allocator_blocks;
                R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks),
                                             this->heap_region_start + size, (this->current_heap_end - this->heap_region_start) - size,
                                             KMemoryState_All, KMemoryState_Normal,
                                             KMemoryPermission_All, KMemoryPermission_UserReadWrite,
                                             KMemoryAttribute_All,  KMemoryAttribute_None));

                /* Create an update allocator. */
                KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
                R_TRY(allocator.Initialize(num_allocator_blocks));

                /* We're going to perform an update, so create a helper. */
                KScopedPageTableUpdater updater(this);

                /* Unmap the end of the heap. */
                const size_t num_pages = ((this->current_heap_end - this->heap_region_start) - size) / PageSize;
                const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                R_TRY(this->Operate(updater.GetPageList(), this->heap_region_start + size, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

                /* Release the memory from the resource limit. */
                GetCurrentProcess().ReleaseResource(ams::svc::LimitableResource_PhysicalMemoryMax, num_pages * PageSize);

                /* Apply the memory block update. */
                this->memory_block_manager.Update(std::addressof(allocator), this->heap_region_start + size, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, size == 0 ? KMemoryBlockDisableMergeAttribute_Normal : KMemoryBlockDisableMergeAttribute_None);

                /* Update the current heap end. */
                this->current_heap_end = this->heap_region_start + size;

                /* Set the output. */
                *out = this->heap_region_start;
                return ResultSuccess();
            } else if (size == static_cast<size_t>(this->current_heap_end - this->heap_region_start)) {
                /* The size requested is exactly the current size. */
                *out = this->heap_region_start;
                return ResultSuccess();
            } else {
                /* We have to allocate memory. Determine how much to allocate and where while the table is locked. */
                cur_address     = this->current_heap_end;
                allocation_size = size - (this->current_heap_end - this->heap_region_start);
            }
        }

        /* Reserve memory for the heap extension. */
        KScopedResourceReservation memory_reservation(GetCurrentProcess().GetResourceLimit(), ams::svc::LimitableResource_PhysicalMemoryMax, allocation_size);
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

        /* Allocate pages for the heap extension. */
        KPageGroup pg(this->block_info_manager);
        R_TRY(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(pg), allocation_size / PageSize, this->allocate_option));

        /* Close the opened pages when we're done with them. */
        /* If the mapping succeeds, each page will gain an extra reference, otherwise they will be freed automatically. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Clear all the newly allocated pages. */
        for (const auto &it : pg) {
            std::memset(GetVoidPointer(it.GetAddress()), this->heap_fill_value, it.GetSize());
        }

        /* Map the pages. */
        {
            /* Lock the table. */
            KScopedLightLock lk(this->general_lock);

            /* Ensure that the heap hasn't changed since we began executing. */
            MESOSPHERE_ABORT_UNLESS(cur_address == this->current_heap_end);

            /* Check the memory state. */
            size_t num_allocator_blocks;
            R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), this->current_heap_end, allocation_size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

            /* Create an update allocator. */
            KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
            R_TRY(allocator.Initialize(num_allocator_blocks));

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Map the pages. */
            const size_t num_pages = allocation_size / PageSize;
            const KPageProperties map_properties = { KMemoryPermission_UserReadWrite, false, false, (this->current_heap_end == this->heap_region_start) ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), this->current_heap_end, num_pages, pg, map_properties, OperationType_MapGroup, false));

            /* We succeeded, so commit our memory reservation. */
            memory_reservation.Commit();

            /* Apply the memory block update. */
            this->memory_block_manager.Update(std::addressof(allocator), this->current_heap_end, num_pages, KMemoryState_Normal, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, this->heap_region_start == this->current_heap_end ? KMemoryBlockDisableMergeAttribute_Normal : KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);

            /* Update the current heap end. */
            this->current_heap_end = this->heap_region_start + size;

            /* Set the output. */
            *out = this->heap_region_start;
            return ResultSuccess();
        }
    }

    Result KPageTableBase::SetMaxHeapSize(size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Only process page tables are allowed to set heap size. */
        MESOSPHERE_ASSERT(!this->IsKernel());

        this->max_heap_size = size;

        return ResultSuccess();
    }

    Result KPageTableBase::QueryInfo(KMemoryInfo *out_info, ams::svc::PageInfo *out_page_info, KProcessAddress addr) const {
        /* If the address is invalid, create a fake block. */
        if (!this->Contains(addr, 1)) {
            *out_info = {
                .address                          = GetInteger(this->address_space_end),
                .size                             = 0 - GetInteger(this->address_space_end),
                .state                            = static_cast<KMemoryState>(ams::svc::MemoryState_Inaccessible),
                .device_disable_merge_left_count  = 0,
                .device_disable_merge_right_count = 0,
                .ipc_lock_count                   = 0,
                .device_use_count                 = 0,
                .ipc_disable_merge_count          = 0,
                .perm                             = KMemoryPermission_None,
                .attribute                        = KMemoryAttribute_None,
                .original_perm                    = KMemoryPermission_None,
                .disable_merge_attribute          = KMemoryBlockDisableMergeAttribute_None,
            };
            out_page_info->flags = 0;

            return ResultSuccess();
        }

        /* Otherwise, lock the table and query. */
        KScopedLightLock lk(this->general_lock);
        return this->QueryInfoImpl(out_info, out_page_info, addr);
    }

    Result KPageTableBase::QueryPhysicalAddress(ams::svc::PhysicalMemoryInfo *out, KProcessAddress address) const {
        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Align the address down to page size. */
        address = util::AlignDown(GetInteger(address), PageSize);

        /* Verify that we can query the address. */
        KMemoryInfo info;
        ams::svc::PageInfo page_info;
        R_TRY(this->QueryInfoImpl(std::addressof(info), std::addressof(page_info), address));

        /* Check the memory state. */
        R_TRY(this->CheckMemoryState(info, KMemoryState_FlagCanQueryPhysical, KMemoryState_FlagCanQueryPhysical, KMemoryPermission_UserReadExecute, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Prepare to traverse. */
        KPhysicalAddress phys_addr;
        size_t phys_size;

        KProcessAddress virt_addr = info.GetAddress();
        KProcessAddress end_addr  = info.GetEndAddress();

        /* Perform traversal. */
        {
            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   next_entry;
            bool traverse_valid = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), virt_addr);
            R_UNLESS(traverse_valid, svc::ResultInvalidCurrentMemory());

            /* Set tracking variables. */
            phys_addr = next_entry.phys_addr;
            phys_size = next_entry.block_size - (GetInteger(phys_addr) & (next_entry.block_size - 1));

            /* Iterate. */
            while (true) {
                /* Continue the traversal. */
                traverse_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
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
        return ResultSuccess();
    }

    Result KPageTableBase::MapIo(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(size,                  PageSize));
        MESOSPHERE_ASSERT(size > 0);
        R_UNLESS(phys_addr < phys_addr + size, svc::ResultInvalidAddress());
        const size_t num_pages = size / PageSize;
        const KPhysicalAddress last = phys_addr + size - 1;

        /* Get region extents. */
        const KProcessAddress region_start     = this->GetRegionAddress(KMemoryState_Io);
        const size_t          region_size      = this->GetRegionSize(KMemoryState_Io);
        const size_t          region_num_pages = region_size / PageSize;

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

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Select an address to map at. */
        KProcessAddress addr = Null<KProcessAddress>;
        const size_t phys_alignment = std::min(std::min(GetInteger(phys_addr) & -GetInteger(phys_addr), size & -size), MaxPhysicalMapAlignment);
        for (s32 block_type = KPageTable::GetMaxBlockType(); block_type >= 0; block_type--) {
            const size_t alignment = KPageTable::GetBlockSize(static_cast<KPageTable::BlockType>(block_type));
            if (alignment > phys_alignment) {
                continue;
            }

            addr = this->FindFreeArea(region_start, region_num_pages, num_pages, alignment, 0, this->GetNumGuardPages());
            if (addr != Null<KProcessAddress>) {
                break;
            }
        }
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());

        /* Check that we can map IO here. */
        MESOSPHERE_ASSERT(this->CanContain(addr, size, KMemoryState_Io));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(KMemoryBlockManagerUpdateAllocator::MaxBlocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, true, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, phys_addr, true, properties, OperationType_Map, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, KMemoryState_Io, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        return ResultSuccess();
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
        KScopedLightLock lk(this->general_lock);

        /* Select an address to map at. */
        KProcessAddress addr = Null<KProcessAddress>;
        const size_t phys_alignment = std::min(std::min(GetInteger(phys_addr) & -GetInteger(phys_addr), size & -size), MaxPhysicalMapAlignment);
        for (s32 block_type = KPageTable::GetMaxBlockType(); block_type >= 0; block_type--) {
            const size_t alignment = KPageTable::GetBlockSize(static_cast<KPageTable::BlockType>(block_type));
            if (alignment > phys_alignment) {
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
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(KMemoryBlockManagerUpdateAllocator::MaxBlocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, false, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, phys_addr, true, properties, OperationType_Map, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, KMemoryState_Static, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        return ResultSuccess();
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

        return ResultSuccess();
    }

    Result KPageTableBase::MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, bool is_pa_valid, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(util::IsAligned(alignment, PageSize) && alignment >= PageSize);

        /* Ensure this is a valid map request. */
        R_UNLESS(this->CanContain(region_start, region_num_pages * PageSize, state), svc::ResultInvalidCurrentMemory());
        R_UNLESS(num_pages < region_num_pages,                                     svc::ResultOutOfMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Find a random address to map at. */
        KProcessAddress addr = this->FindFreeArea(region_start, region_num_pages, num_pages, alignment, 0, this->GetNumGuardPages());
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), alignment));
        MESOSPHERE_ASSERT(this->CanContain(addr, num_pages * PageSize, state));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(KMemoryBlockManagerUpdateAllocator::MaxBlocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        if (is_pa_valid) {
            const KPageProperties properties = { perm, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, phys_addr, true, properties, OperationType_Map, false));
        } else {
            R_TRY(this->AllocateAndMapPagesImpl(updater.GetPageList(), addr, num_pages, perm));
        }

        /* Update the blocks. */
        this->memory_block_manager.Update(std::addressof(allocator), addr, num_pages, state, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        *out_addr = addr;
        return ResultSuccess();
    }

    Result KPageTableBase::MapPages(KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm) {
        /* Check that the map is in range. */
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->CanContain(address, size, state), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Map the pages. */
        R_TRY(this->AllocateAndMapPagesImpl(updater.GetPageList(), address, num_pages, perm));

        /* Update the blocks. */
        this->memory_block_manager.Update(std::addressof(allocator), address, num_pages, state, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        return ResultSuccess();
    }

    Result KPageTableBase::UnmapPages(KProcessAddress address, size_t num_pages, KMemoryState state) {
        /* Check that the unmap is in range. */
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, state, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform the unmap. */
        const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        return ResultSuccess();
    }

    Result KPageTableBase::MapPageGroup(KProcessAddress *out_addr, const KPageGroup &pg, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

        /* Ensure this is a valid map request. */
        const size_t num_pages = pg.GetNumPages();
        R_UNLESS(this->CanContain(region_start, region_num_pages * PageSize, state), svc::ResultInvalidCurrentMemory());
        R_UNLESS(num_pages < region_num_pages,                                       svc::ResultOutOfMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Find a random address to map at. */
        KProcessAddress addr = this->FindFreeArea(region_start, region_num_pages, num_pages, PageSize, 0, this->GetNumGuardPages());
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());
        MESOSPHERE_ASSERT(this->CanContain(addr, num_pages * PageSize, state));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(KMemoryBlockManagerUpdateAllocator::MaxBlocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, state == KMemoryState_Io, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->MapPageGroupImpl(updater.GetPageList(), addr, pg, properties, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, state, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        *out_addr = addr;
        return ResultSuccess();
    }

    Result KPageTableBase::MapPageGroup(KProcessAddress addr, const KPageGroup &pg, KMemoryState state, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

        /* Ensure this is a valid map request. */
        const size_t num_pages = pg.GetNumPages();
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->CanContain(addr, size, state), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check if state allows us to map. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), addr, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, state == KMemoryState_Io, false, DisableMergeAttribute_DisableHead };
        R_TRY(this->MapPageGroupImpl(updater.GetPageList(), addr, pg, properties, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, state, perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* We successfully mapped the pages. */
        return ResultSuccess();
    }

    Result KPageTableBase::UnmapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state) {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

        /* Ensure this is a valid unmap request. */
        const size_t num_pages = pg.GetNumPages();
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->CanContain(address, size, state), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check if state allows us to unmap. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, state, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Check that the page group is valid. */
        R_UNLESS(this->IsValidPageGroup(pg, address, num_pages), svc::ResultInvalidCurrentMemory());

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform unmapping operation. */
        const KPageProperties properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_Unmap, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        return ResultSuccess();
    }

    Result KPageTableBase::MakeAndOpenPageGroup(KPageGroup *out, KProcessAddress address, size_t num_pages, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) {
        /* Ensure that the page group isn't null. */
        MESOSPHERE_ASSERT(out != nullptr);

        /* Make sure that the region we're mapping is valid for the table. */
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check if state allows us to create the group. */
        R_TRY(this->CheckMemoryState(address, size, state_mask | KMemoryState_FlagReferenceCounted, state | KMemoryState_FlagReferenceCounted, perm_mask, perm, attr_mask, attr));

        /* Create a new page group for the region. */
        R_TRY(this->MakePageGroup(*out, address, num_pages));

        /* Open a new reference to the pages in the group. */
        out->Open();

        return ResultSuccess();
    }

    Result KPageTableBase::InvalidateProcessDataCache(KProcessAddress address, size_t size) {
        /* Check that the region is in range. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

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

        return ResultSuccess();
    }

    Result KPageTableBase::ReadDebugMemory(void *buffer, KProcessAddress address, size_t size) {
        /* Lightly validate the region is in range. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Require that the memory either be user readable or debuggable. */
        const bool can_read = R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, KMemoryState_None, KMemoryState_None, KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None));
        if (!can_read) {
            const bool can_debug = R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, KMemoryState_FlagCanDebug, KMemoryState_FlagCanDebug, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));
            R_UNLESS(can_debug, svc::ResultInvalidCurrentMemory());
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

        auto PerformCopy = [&] ALWAYS_INLINE_LAMBDA () -> Result {
            /* Ensure the address is linear mapped. */
            R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

            /* Copy as much aligned data as we can. */
            if (cur_size >= sizeof(u32)) {
                const size_t copy_size = util::AlignDown(cur_size, sizeof(u32));
                R_UNLESS(UserspaceAccess::CopyMemoryToUserAligned32Bit(buffer, GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), copy_size), svc::ResultInvalidPointer());
                buffer    = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + copy_size);
                cur_addr += copy_size;
                cur_size -= copy_size;
            }

            /* Copy remaining data. */
            if (cur_size > 0) {
                R_UNLESS(UserspaceAccess::CopyMemoryToUser(buffer, GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), cur_size), svc::ResultInvalidPointer());
            }

            return ResultSuccess();
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

        return ResultSuccess();
    }

    Result KPageTableBase::WriteDebugMemory(KProcessAddress address, const void *buffer, size_t size) {
        /* Lightly validate the region is in range. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Require that the memory either be user writable or debuggable. */
        const bool can_read = R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, KMemoryState_None, KMemoryState_None, KMemoryPermission_UserReadWrite, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryAttribute_None));
        if (!can_read) {
            const bool can_debug = R_SUCCEEDED(this->CheckMemoryStateContiguous(address, size, KMemoryState_FlagCanDebug, KMemoryState_FlagCanDebug, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));
            R_UNLESS(can_debug, svc::ResultInvalidCurrentMemory());
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

        auto PerformCopy = [&] ALWAYS_INLINE_LAMBDA () -> Result {
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

            return ResultSuccess();
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

        return ResultSuccess();
    }

    Result KPageTableBase::LockForDeviceAddressSpace(KPageGroup *out, KProcessAddress address, size_t size, KMemoryPermission perm, bool is_aligned) {
        /* Lightly validate the range before doing anything else. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check the memory state. */
        const u32 test_state = (is_aligned ? KMemoryState_FlagCanAlignedDeviceMap : KMemoryState_FlagCanDeviceMap);
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, test_state, test_state, perm, perm, KMemoryAttribute_IpcLocked | KMemoryAttribute_Locked, KMemoryAttribute_None, KMemoryAttribute_DeviceShared));

        /* Make the page group, if we should. */
        if (out != nullptr) {
            R_TRY(this->MakePageGroup(*out, address, num_pages));
        }

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* Update the memory blocks. */
        this->memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, &KMemoryBlock::ShareToDevice, KMemoryPermission_None);

        /* Open the page group. */
        if (out != nullptr) {
            out->Open();
        }

        return ResultSuccess();
    }

    Result KPageTableBase::UnlockForDeviceAddressSpace(KProcessAddress address, size_t size) {
        /* Lightly validate the range before doing anything else. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryStateContiguous(std::addressof(num_allocator_blocks),
                                               address, size,
                                               KMemoryState_FlagCanDeviceMap, KMemoryState_FlagCanDeviceMap,
                                               KMemoryPermission_None, KMemoryPermission_None,
                                               KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* Update the memory blocks. */
        this->memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, &KMemoryBlock::UnshareToDevice, KMemoryPermission_None);

        return ResultSuccess();
    }

    Result KPageTableBase::MakePageGroupForUnmapDeviceAddressSpace(KPageGroup *out, KProcessAddress address, size_t size) {
        /* Lightly validate the range before doing anything else. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryStateContiguous(std::addressof(num_allocator_blocks),
                                               address, size,
                                               KMemoryState_FlagReferenceCounted | KMemoryState_FlagCanDeviceMap, KMemoryState_FlagReferenceCounted | KMemoryState_FlagCanDeviceMap,
                                               KMemoryPermission_None, KMemoryPermission_None,
                                               KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* Make the page group. */
        R_TRY(this->MakePageGroup(*out, address, num_pages));

        /* Update the memory blocks. */
        const KMemoryBlockManager::MemoryBlockLockFunction lock_func = this->enable_device_address_space_merge ? &KMemoryBlock::UpdateDeviceDisableMergeStateForShare : &KMemoryBlock::UpdateDeviceDisableMergeStateForShareRight;
        this->memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, lock_func, KMemoryPermission_None);

        /* Open a reference to the pages in the page group. */
        out->Open();

        return ResultSuccess();
    }

    Result KPageTableBase::UnlockForDeviceAddressSpacePartialMap(KProcessAddress address, size_t size, size_t mapped_size) {
        /* Lightly validate the range before doing anything else. */
        const size_t num_pages = size / PageSize;
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Determine useful extents. */
        const KProcessAddress mapped_end_address = address + mapped_size;
        const size_t unmapped_size = size - mapped_size;

        /* Check memory state. */
        size_t allocator_num_blocks = 0, unmapped_allocator_num_blocks = 0;
        if (unmapped_size) {
            if (this->enable_device_address_space_merge) {
                R_TRY(this->CheckMemoryState(std::addressof(allocator_num_blocks),
                                             address, size,
                                             KMemoryState_FlagCanDeviceMap, KMemoryState_FlagCanDeviceMap,
                                             KMemoryPermission_None, KMemoryPermission_None,
                                             KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));
            }
            R_TRY(this->CheckMemoryState(std::addressof(unmapped_allocator_num_blocks),
                                         mapped_end_address, unmapped_size,
                                         KMemoryState_FlagCanDeviceMap, KMemoryState_FlagCanDeviceMap,
                                         KMemoryPermission_None, KMemoryPermission_None,
                                         KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));
        } else {
            R_TRY(this->CheckMemoryState(std::addressof(allocator_num_blocks),
                                         address, size,
                                         KMemoryState_FlagCanDeviceMap, KMemoryState_FlagCanDeviceMap,
                                         KMemoryPermission_None, KMemoryPermission_None,
                                         KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked, KMemoryAttribute_DeviceShared));
        }

        /* Create an update allocator for the region. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(allocator_num_blocks));

        /* Create an update allocator for the unmapped region. */
        KMemoryBlockManagerUpdateAllocator unmapped_allocator(this->memory_block_slab_manager);
        R_TRY(unmapped_allocator.Initialize(unmapped_allocator_num_blocks));

        /* Determine parameters for the update lock call. */
        KMemoryBlockManagerUpdateAllocator *lock_allocator;
        KProcessAddress lock_address;
        size_t lock_num_pages;
        KMemoryBlockManager::MemoryBlockLockFunction lock_func;
        if (unmapped_size) {
            /* If device address space merge is enabled, update tracking appropriately. */
            if (this->enable_device_address_space_merge) {
                this->memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, &KMemoryBlock::UpdateDeviceDisableMergeStateForUnshareLeft, KMemoryPermission_None);
            }

            lock_allocator = std::addressof(unmapped_allocator);
            lock_address   = mapped_end_address;
            lock_num_pages = unmapped_size / PageSize;
            lock_func      = &KMemoryBlock::UnshareToDeviceRight;
        } else {
            lock_allocator = std::addressof(allocator);
            lock_address   = address;
            lock_num_pages = num_pages;
            if (this->enable_device_address_space_merge) {
                lock_func = &KMemoryBlock::UpdateDeviceDisableMergeStateForUnshare;
            } else {
                lock_func = &KMemoryBlock::UpdateDeviceDisableMergeStateForUnshareRight;
            }
        }

        /* Update the memory blocks. */
        this->memory_block_manager.UpdateLock(lock_allocator, lock_address, lock_num_pages, lock_func, KMemoryPermission_None);

        return ResultSuccess();
    }

    Result KPageTableBase::LockForIpcUserBuffer(KPhysicalAddress *out, KProcessAddress address, size_t size) {
        return this->LockMemoryAndOpen(nullptr, out, address, size,
                                       KMemoryState_FlagCanIpcUserBuffer, KMemoryState_FlagCanIpcUserBuffer,
                                       KMemoryPermission_All, KMemoryPermission_UserReadWrite,
                                       KMemoryAttribute_All, KMemoryAttribute_None,
                                       static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite),
                                       KMemoryAttribute_Locked);
    }

    Result KPageTableBase::UnlockForIpcUserBuffer(KProcessAddress address, size_t size) {
        return this->UnlockMemory(address, size,
                                  KMemoryState_FlagCanIpcUserBuffer, KMemoryState_FlagCanIpcUserBuffer,
                                  KMemoryPermission_None, KMemoryPermission_None,
                                  KMemoryAttribute_All, KMemoryAttribute_Locked,
                                  KMemoryPermission_UserReadWrite,
                                  KMemoryAttribute_Locked, nullptr);
    }

    Result KPageTableBase::LockForTransferMemory(KPageGroup *out, KProcessAddress address, size_t size, KMemoryPermission perm) {
        return this->LockMemoryAndOpen(out, nullptr, address, size,
                                       KMemoryState_FlagCanTransfer, KMemoryState_FlagCanTransfer,
                                       KMemoryPermission_All, KMemoryPermission_UserReadWrite,
                                       KMemoryAttribute_All, KMemoryAttribute_None,
                                       perm,
                                       KMemoryAttribute_Locked);
    }

    Result KPageTableBase::UnlockForTransferMemory(KProcessAddress address, size_t size, const KPageGroup &pg) {
        return this->UnlockMemory(address, size,
                                  KMemoryState_FlagCanTransfer, KMemoryState_FlagCanTransfer,
                                  KMemoryPermission_None, KMemoryPermission_None,
                                  KMemoryAttribute_All, KMemoryAttribute_Locked,
                                  KMemoryPermission_UserReadWrite,
                                  KMemoryAttribute_Locked, std::addressof(pg));
    }

    Result KPageTableBase::LockForCodeMemory(KPageGroup *out, KProcessAddress address, size_t size) {
        return this->LockMemoryAndOpen(out, nullptr, address, size,
                                       KMemoryState_FlagCanCodeMemory, KMemoryState_FlagCanCodeMemory,
                                       KMemoryPermission_All, KMemoryPermission_UserReadWrite,
                                       KMemoryAttribute_All, KMemoryAttribute_None,
                                       static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite),
                                       KMemoryAttribute_Locked);
    }

    Result KPageTableBase::UnlockForCodeMemory(KProcessAddress address, size_t size, const KPageGroup &pg) {
        return this->UnlockMemory(address, size,
                                  KMemoryState_FlagCanCodeMemory, KMemoryState_FlagCanCodeMemory,
                                  KMemoryPermission_None, KMemoryPermission_None,
                                  KMemoryAttribute_All, KMemoryAttribute_Locked,
                                  KMemoryPermission_UserReadWrite,
                                  KMemoryAttribute_Locked, std::addressof(pg));
    }

    Result KPageTableBase::CopyMemoryFromLinearToUser(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(src_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(this->general_lock);

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

            auto PerformCopy = [&] ALWAYS_INLINE_LAMBDA () -> Result {
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

                return ResultSuccess();
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

        return ResultSuccess();
    }

    Result KPageTableBase::CopyMemoryFromLinearToKernel(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(src_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(this->general_lock);

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

            auto PerformCopy = [&] ALWAYS_INLINE_LAMBDA () -> Result {
                /* Ensure the address is linear mapped. */
                R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

                /* Copy the data. */
                std::memcpy(GetVoidPointer(dst_addr), GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), cur_size);

                return ResultSuccess();
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

        return ResultSuccess();
    }

    Result KPageTableBase::CopyMemoryFromUserToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(dst_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(this->general_lock);

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

            auto PerformCopy = [&] ALWAYS_INLINE_LAMBDA () -> Result {
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

                return ResultSuccess();
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

        return ResultSuccess();
    }

    Result KPageTableBase::CopyMemoryFromKernelToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr) {
        /* Lightly validate the range before doing anything else. */
        R_UNLESS(this->Contains(dst_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(this->general_lock);

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

            auto PerformCopy = [&] ALWAYS_INLINE_LAMBDA () -> Result {
                /* Ensure the address is linear mapped. */
                R_UNLESS(IsLinearMappedPhysicalAddress(cur_addr), svc::ResultInvalidCurrentMemory());

                /* Copy the data. */
                std::memcpy(GetVoidPointer(GetLinearMappedVirtualAddress(cur_addr)), GetVoidPointer(src_addr), cur_size);

                return ResultSuccess();
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

        return ResultSuccess();
    }

    Result KPageTableBase::CopyMemoryFromHeapToHeap(KPageTableBase &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
        /* For convenience, alias this. */
        KPageTableBase &src_page_table = *this;

        /* Lightly validate the ranges before doing anything else. */
        R_UNLESS(src_page_table.Contains(src_addr, size), svc::ResultInvalidCurrentMemory());
        R_UNLESS(dst_page_table.Contains(dst_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Get the table locks. */
            KLightLock &lock_0 = (reinterpret_cast<uintptr_t>(std::addressof(src_page_table)) <= reinterpret_cast<uintptr_t>(std::addressof(dst_page_table))) ? src_page_table.general_lock : dst_page_table.general_lock;
            KLightLock &lock_1 = (reinterpret_cast<uintptr_t>(std::addressof(src_page_table)) <= reinterpret_cast<uintptr_t>(std::addressof(dst_page_table))) ? dst_page_table.general_lock : src_page_table.general_lock;

            /* Lock the first lock. */
            KScopedLightLock lk0(lock_0);

            /* If necessary, lock the second lock. */
            std::optional<KScopedLightLock> lk1;
            if (std::addressof(lock_0) != std::addressof(lock_1)) {
                lk1.emplace(lock_1);
            }

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

        return ResultSuccess();
    }

    Result KPageTableBase::CopyMemoryFromHeapToHeapWithoutCheckDestination(KPageTableBase &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
        /* For convenience, alias this. */
        KPageTableBase &src_page_table = *this;

        /* Lightly validate the ranges before doing anything else. */
        R_UNLESS(src_page_table.Contains(src_addr, size), svc::ResultInvalidCurrentMemory());
        R_UNLESS(dst_page_table.Contains(dst_addr, size), svc::ResultInvalidCurrentMemory());

        /* Copy the memory. */
        {
            /* Get the table locks. */
            KLightLock &lock_0 = (reinterpret_cast<uintptr_t>(std::addressof(src_page_table)) <= reinterpret_cast<uintptr_t>(std::addressof(dst_page_table))) ? src_page_table.general_lock : dst_page_table.general_lock;
            KLightLock &lock_1 = (reinterpret_cast<uintptr_t>(std::addressof(src_page_table)) <= reinterpret_cast<uintptr_t>(std::addressof(dst_page_table))) ? dst_page_table.general_lock : src_page_table.general_lock;

            /* Lock the first lock. */
            KScopedLightLock lk0(lock_0);

            /* If necessary, lock the second lock. */
            std::optional<KScopedLightLock> lk1;
            if (std::addressof(lock_0) != std::addressof(lock_1)) {
                lk1.emplace(lock_1);
            }

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

        return ResultSuccess();
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
        const auto src_perm = static_cast<KMemoryPermission>((test_perm == KMemoryPermission_UserReadWrite) ? KMemoryPermission_KernelReadWrite | KMemoryPermission_NotMapped : KMemoryPermission_UserRead);

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
                return svc::ResultInvalidCombination();
        }

        /* Ensure that on failure, we roll back appropriately. */
        size_t mapped_size = 0;
        auto cleanup_guard = SCOPE_GUARD {
            if (mapped_size > 0) {
                this->CleanupForIpcClientOnServerSetupFailure(page_list, mapping_src_start, mapped_size, src_perm);
            }
        };

        size_t blocks_needed = 0;

        /* Iterate, mapping as needed. */
        KMemoryBlockManager::const_iterator it = this->memory_block_manager.FindIterator(aligned_src_start);
        while (true) {
            const KMemoryInfo info = it->GetMemoryInfo();

            /* Validate the current block. */
            R_TRY(this->CheckMemoryState(info, test_state, test_state, test_perm, test_perm, test_attr_mask, KMemoryAttribute_None));

            if (mapping_src_start < mapping_src_end && GetInteger(mapping_src_start) < info.GetEndAddress() && info.GetAddress() < GetInteger(mapping_src_end)) {
                const auto cur_start  = info.GetAddress() >= GetInteger(mapping_src_start) ? info.GetAddress() : GetInteger(mapping_src_start);
                const auto cur_end    = mapping_src_last >= info.GetLastAddress() ? info.GetEndAddress() : GetInteger(mapping_src_end);
                const size_t cur_size = cur_end - cur_start;

                if (info.GetAddress() < GetInteger(mapping_src_start)) {
                    ++blocks_needed;
                }
                if (mapping_src_last < info.GetLastAddress()) {
                    ++blocks_needed;
                }

                /* Set the permissions on the block, if we need to. */
                if ((info.GetPermission() & KMemoryPermission_IpcLockChangeMask) != src_perm) {
                    const DisableMergeAttribute head_body_attr = (GetInteger(mapping_src_start) >= info.GetAddress()) ? DisableMergeAttribute_DisableHeadAndBody : DisableMergeAttribute_None;
                    const DisableMergeAttribute tail_attr      = (cur_end == GetInteger(mapping_src_end))             ? DisableMergeAttribute_DisableTail        : DisableMergeAttribute_None;
                    const KPageProperties properties = { src_perm, false, false, static_cast<DisableMergeAttribute>(head_body_attr | tail_attr) };
                    R_TRY(this->Operate(page_list, cur_start, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
                }

                /* Note that we mapped this part. */
                mapped_size += cur_size;
            }

            /* If the block is at the end, we're done. */
            if (aligned_src_last <= info.GetLastAddress()) {
                break;
            }

            /* Advance. */
            ++it;
            MESOSPHERE_ABORT_UNLESS(it != this->memory_block_manager.end());
        }

        /* We succeeded, so no need to cleanup. */
        cleanup_guard.Cancel();

        if (out_blocks_needed != nullptr) {
            MESOSPHERE_ASSERT(blocks_needed <= KMemoryBlockManagerUpdateAllocator::MaxBlocks);
            *out_blocks_needed = blocks_needed;
        }

        return ResultSuccess();
    }

    Result KPageTableBase::SetupForIpcServer(KProcessAddress *out_addr, size_t size, KProcessAddress src_addr, KMemoryPermission test_perm, KMemoryState dst_state, KPageTableBase &src_page_table, bool send) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(src_page_table.IsLockedByCurrentThread());

        /* Check that we can theoretically map. */
        const KProcessAddress region_start = this->alias_region_start;
        const size_t          region_size  = this->alias_region_end - this->alias_region_start;
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
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(KMemoryBlockManagerUpdateAllocator::MaxBlocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Reserve space for any partial pages we allocate. */
        const size_t unmapped_size    = aligned_src_size - mapping_src_size;
        KScopedResourceReservation memory_reservation(GetCurrentProcess().GetResourceLimit(), ams::svc::LimitableResource_PhysicalMemoryMax, unmapped_size);
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

        /* Ensure that we manage page references correctly. */
        KVirtualAddress start_partial_page = Null<KVirtualAddress>;
        KVirtualAddress end_partial_page   = Null<KVirtualAddress>;
        KProcessAddress cur_mapped_addr    = dst_addr;

        /* If the partial pages are mapped, an extra reference will have been opened. Otherwise, they'll free on scope exit. */
        ON_SCOPE_EXIT {
            if (start_partial_page != Null<KVirtualAddress>) {
                Kernel::GetMemoryManager().Close(start_partial_page, 1);
            }
            if (end_partial_page != Null<KVirtualAddress>) {
                Kernel::GetMemoryManager().Close(end_partial_page, 1);
            }
        };

        auto cleanup_guard = SCOPE_GUARD {
            if (cur_mapped_addr != dst_addr) {
                const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), dst_addr, (cur_mapped_addr - dst_addr) / PageSize, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
            }
        };

        /* Allocate the start page as needed. */
        if (aligned_src_start < mapping_src_start) {
            start_partial_page = Kernel::GetMemoryManager().AllocateAndOpenContinuous(1, 0, this->allocate_option);
            R_UNLESS(start_partial_page != Null<KVirtualAddress>, svc::ResultOutOfMemory());
        }

        /* Allocate the end page as needed. */
        if (mapping_src_end < aligned_src_end && (aligned_src_start < mapping_src_end || aligned_src_start == mapping_src_start)) {
            end_partial_page = Kernel::GetMemoryManager().AllocateAndOpenContinuous(1, 0, this->allocate_option);
            R_UNLESS(end_partial_page != Null<KVirtualAddress>, svc::ResultOutOfMemory());
        }

        /* Get the implementation. */
        auto &src_impl = src_page_table.GetImpl();

        /* Get the fill value for partial pages. */
        const auto fill_val = this->ipc_fill_value;

        /* Begin traversal. */
        TraversalContext context;
        TraversalEntry   next_entry;
        bool traverse_valid = src_impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), aligned_src_start);
        MESOSPHERE_ASSERT(traverse_valid);

        /* Prepare tracking variables. */
        KPhysicalAddress cur_block_addr = next_entry.phys_addr;
        size_t cur_block_size           = next_entry.block_size - (GetInteger(cur_block_addr) & (next_entry.block_size - 1));
        size_t tot_block_size           = cur_block_size;

        /* Map the start page, if we have one. */
        if (start_partial_page != Null<KVirtualAddress>) {
            /* Ensure the page holds correct data. */
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

                std::memset(GetVoidPointer(start_partial_page), fill_val, partial_offset);
                std::memcpy(GetVoidPointer(start_partial_page + partial_offset), GetVoidPointer(GetHeapVirtualAddress(cur_block_addr) + partial_offset), copy_size);
                if (clear_size > 0) {
                    std::memset(GetVoidPointer(start_partial_page + partial_offset + copy_size), fill_val, clear_size);
                }
            } else {
                std::memset(GetVoidPointer(start_partial_page), fill_val, PageSize);
            }

            /* Map the page. */
            const KPageProperties start_map_properties = { test_perm, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->Operate(updater.GetPageList(), cur_mapped_addr, 1, GetHeapPhysicalAddress(start_partial_page), true, start_map_properties, OperationType_Map, false));

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
        if (end_partial_page != Null<KVirtualAddress>) {
            /* Ensure the page holds correct data. */
            if (send) {
                const size_t copy_size = src_end - mapping_src_end;
                std::memcpy(GetVoidPointer(end_partial_page), GetVoidPointer(GetHeapVirtualAddress(cur_block_addr)), copy_size);
                std::memset(GetVoidPointer(end_partial_page + copy_size), fill_val, PageSize - copy_size);
            } else {
                std::memset(GetVoidPointer(end_partial_page), fill_val, PageSize);
            }

            /* Map the page. */
            const KPageProperties map_properties = { test_perm, false, false, (cur_mapped_addr == dst_addr) ? DisableMergeAttribute_DisableHead : DisableMergeAttribute_None };
            R_TRY(this->Operate(updater.GetPageList(), cur_mapped_addr, 1, GetHeapPhysicalAddress(end_partial_page), true, map_properties, OperationType_Map, false));
        }

        /* Update memory blocks to reflect our changes */
        this->memory_block_manager.Update(std::addressof(allocator), dst_addr, aligned_src_size / PageSize, dst_state, test_perm, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

        /* Set the output address. */
        *out_addr = dst_addr + (src_start - aligned_src_start);

        /* We succeeded. */
        cleanup_guard.Cancel();
        memory_reservation.Commit();
        return ResultSuccess();
    }

    Result KPageTableBase::SetupForIpc(KProcessAddress *out_dst_addr, size_t size, KProcessAddress src_addr, KPageTableBase &src_page_table, KMemoryPermission test_perm, KMemoryState dst_state, bool send) {
        /* For convenience, alias this. */
        KPageTableBase &dst_page_table = *this;

        /* Get the table locks. */
        KLightLock &lock_0 = (reinterpret_cast<uintptr_t>(std::addressof(src_page_table)) <= reinterpret_cast<uintptr_t>(std::addressof(dst_page_table))) ? src_page_table.general_lock : dst_page_table.general_lock;
        KLightLock &lock_1 = (reinterpret_cast<uintptr_t>(std::addressof(src_page_table)) <= reinterpret_cast<uintptr_t>(std::addressof(dst_page_table))) ? dst_page_table.general_lock : src_page_table.general_lock;

        /* Lock the first lock. */
        KScopedLightLock lk0(lock_0);

        /* If necessary, lock the second lock. */
        std::optional<KScopedLightLock> lk1;
        if (std::addressof(lock_0) != std::addressof(lock_1)) {
            lk1.emplace(lock_1);
        }

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(std::addressof(src_page_table));

        /* Perform client setup. */
        size_t num_allocator_blocks;
        R_TRY(src_page_table.SetupForIpcClient(updater.GetPageList(), std::addressof(num_allocator_blocks), src_addr, size, test_perm, dst_state));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(src_page_table.memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* Get the mapped extents. */
        const KProcessAddress src_map_start = util::AlignUp(GetInteger(src_addr), PageSize);
        const KProcessAddress src_map_end   = util::AlignDown(GetInteger(src_addr) + size, PageSize);
        const size_t src_map_size           = src_map_end - src_map_start;

        /* Ensure that we clean up appropriately if we fail after this. */
        const auto src_perm = static_cast<KMemoryPermission>((test_perm == KMemoryPermission_UserReadWrite) ? KMemoryPermission_KernelReadWrite | KMemoryPermission_NotMapped : KMemoryPermission_UserRead);
        auto cleanup_guard = SCOPE_GUARD {
            if (src_map_end > src_map_start) {
                src_page_table.CleanupForIpcClientOnServerSetupFailure(updater.GetPageList(), src_map_start, src_map_size, src_perm);
            }
        };

        /* Perform server setup. */
        R_TRY(dst_page_table.SetupForIpcServer(out_dst_addr, size, src_addr, test_perm, dst_state, src_page_table, send));

        /* If anything was mapped, ipc-lock the pages. */
        if (src_map_start < src_map_end) {
            /* Get the source permission. */
            src_page_table.memory_block_manager.UpdateLock(std::addressof(allocator), src_map_start, (src_map_end - src_map_start) / PageSize, &KMemoryBlock::LockForIpc, src_perm);
        }

        /* We succeeded, so cancel our cleanup guard. */
        cleanup_guard.Cancel();

        return ResultSuccess();
    }

    Result KPageTableBase::CleanupForIpcServer(KProcessAddress address, size_t size, KMemoryState dst_state, KProcess *server_process) {
        /* Validate the address. */
        R_UNLESS(this->Contains(address, size), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Validate the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, dst_state, KMemoryPermission_UserRead, KMemoryPermission_UserRead, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

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
        this->memory_block_manager.Update(std::addressof(allocator), aligned_start, aligned_num_pages, KMemoryState_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        /* Release from the resource limit as relevant. */
        if (auto *resource_limit = server_process->GetResourceLimit(); resource_limit != nullptr) {
            const KProcessAddress mapping_start = util::AlignUp(GetInteger(address), PageSize);
            const KProcessAddress mapping_end   = util::AlignDown(GetInteger(address) + size, PageSize);
            const size_t mapping_size           = (mapping_start < mapping_end) ? mapping_end - mapping_start : 0;
            resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, aligned_size - mapping_size);
        }

        return ResultSuccess();
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
                return svc::ResultInvalidCombination();
        }

        /* Lock the table. */
        /* NOTE: Nintendo does this *after* creating the updater below, but this does not follow convention elsewhere in KPageTableBase. */
        KScopedLightLock lk(this->general_lock);

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Ensure that on failure, we roll back appropriately. */
        size_t mapped_size = 0;
        auto unmap_guard = SCOPE_GUARD {
            if (mapped_size > 0) {
                /* Determine where the mapping ends. */
                const auto mapped_end  = GetInteger(mapping_start) + mapped_size;
                const auto mapped_last = mapped_end - 1;

                /* Get current and next iterators. */
                KMemoryBlockManager::const_iterator start_it = this->memory_block_manager.FindIterator(mapping_start);
                KMemoryBlockManager::const_iterator next_it  = start_it;
                ++next_it;

                /* Get the current block info. */
                KMemoryInfo cur_info = start_it->GetMemoryInfo();

                /* Create tracking variables. */
                KProcessAddress cur_address = cur_info.GetAddress();
                size_t cur_size             = cur_info.GetSize();
                bool cur_perm_eq            = cur_info.GetPermission() == cur_info.GetOriginalPermission();
                bool cur_needs_set_perm     = !cur_perm_eq && cur_info.GetIpcLockCount() == 1;
                bool first                  = cur_info.GetIpcDisableMergeCount() == 1 && (cur_info.GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute_Locked) == 0;

                while ((GetInteger(cur_address) + cur_size - 1) < mapped_last) {
                    /* Check that we have a next block. */
                    MESOSPHERE_ABORT_UNLESS(next_it != this->memory_block_manager.end());

                    /* Get the next info. */
                    const KMemoryInfo next_info = next_it->GetMemoryInfo();

                    /* Check if we can consolidate the next block's permission set with the current one. */
                    const bool next_perm_eq        = next_info.GetPermission() == next_info.GetOriginalPermission();
                    const bool next_needs_set_perm = !next_perm_eq && next_info.GetIpcLockCount() == 1;
                    if (cur_perm_eq == next_perm_eq && cur_needs_set_perm == next_needs_set_perm && cur_info.GetOriginalPermission() == next_info.GetOriginalPermission()) {
                        /* We can consolidate the reprotection for the current and next block into a single call. */
                        cur_size += next_info.GetSize();
                    } else {
                        /* We have to operate on the current block. */
                        if ((cur_needs_set_perm || first) && !cur_perm_eq) {
                            const KPageProperties properties = { cur_info.GetPermission(), false, false, first ? DisableMergeAttribute_EnableAndMergeHeadBodyTail : DisableMergeAttribute_None };
                            MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), cur_address, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, true));
                        }

                        /* Advance. */
                        cur_address = next_info.GetAddress();
                        cur_size    = next_info.GetSize();
                        first       = false;
                    }

                    /* Advance. */
                    cur_info           = next_info;
                    cur_perm_eq        = next_perm_eq;
                    cur_needs_set_perm = next_needs_set_perm;
                    ++next_it;
                }

                /* Process the last block. */
                if ((first || cur_needs_set_perm) && !cur_perm_eq) {
                    const KPageProperties properties = { cur_info.GetPermission(), false, false, first ? DisableMergeAttribute_EnableAndMergeHeadBodyTail : DisableMergeAttribute_None };
                    MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), cur_address, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, true));
                }
            }
        };

        /* Iterate, reprotecting as needed. */
        {
            /* Get current and next iterators. */
            KMemoryBlockManager::const_iterator start_it = this->memory_block_manager.FindIterator(mapping_start);
            KMemoryBlockManager::const_iterator next_it  = start_it;
            ++next_it;

            /* Validate the current block. */
            KMemoryInfo cur_info = start_it->GetMemoryInfo();
            MESOSPHERE_R_ABORT_UNLESS(this->CheckMemoryState(cur_info, test_state, test_state, KMemoryPermission_None, KMemoryPermission_None, test_attr_mask | KMemoryAttribute_IpcLocked, KMemoryAttribute_IpcLocked));

            /* Create tracking variables. */
            KProcessAddress cur_address = cur_info.GetAddress();
            size_t cur_size             = cur_info.GetSize();
            bool cur_perm_eq            = cur_info.GetPermission() == cur_info.GetOriginalPermission();
            bool cur_needs_set_perm     = !cur_perm_eq && cur_info.GetIpcLockCount() == 1;
            bool first                  = cur_info.GetIpcDisableMergeCount() == 1 && (cur_info.GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute_Locked) == 0;

            while ((cur_address + cur_size - 1) < mapping_last) {
                /* Check that we have a next block. */
                MESOSPHERE_ABORT_UNLESS(next_it != this->memory_block_manager.end());

                /* Get the next info. */
                const KMemoryInfo next_info = next_it->GetMemoryInfo();

                /* Validate the next block. */
                MESOSPHERE_R_ABORT_UNLESS(this->CheckMemoryState(next_info, test_state, test_state, KMemoryPermission_None, KMemoryPermission_None, test_attr_mask | KMemoryAttribute_IpcLocked, KMemoryAttribute_IpcLocked));

                /* Check if we can consolidate the next block's permission set with the current one. */
                const bool next_perm_eq        = next_info.GetPermission() == next_info.GetOriginalPermission();
                const bool next_needs_set_perm = !next_perm_eq && next_info.GetIpcLockCount() == 1;
                if (cur_perm_eq == next_perm_eq && cur_needs_set_perm == next_needs_set_perm && cur_info.GetOriginalPermission() == next_info.GetOriginalPermission()) {
                    /* We can consolidate the reprotection for the current and next block into a single call. */
                    cur_size += next_info.GetSize();
                } else {
                    /* We have to operate on the current block. */
                    if ((cur_needs_set_perm || first) && !cur_perm_eq) {
                        const KPageProperties properties = { cur_needs_set_perm ? cur_info.GetOriginalPermission() : cur_info.GetPermission(), false, false, first ? DisableMergeAttribute_EnableHeadAndBody : DisableMergeAttribute_None };
                        R_TRY(this->Operate(updater.GetPageList(), cur_address, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
                    }

                    /* Mark that we mapped the block. */
                    mapped_size += cur_size;

                    /* Advance. */
                    cur_address = next_info.GetAddress();
                    cur_size    = next_info.GetSize();
                    first       = false;
                }

                /* Advance. */
                cur_info           = next_info;
                cur_perm_eq        = next_perm_eq;
                cur_needs_set_perm = next_needs_set_perm;
                ++next_it;
            }

            /* Process the last block. */
            const auto lock_count = cur_info.GetIpcLockCount() + (next_it != this->memory_block_manager.end() ? (next_it->GetIpcDisableMergeCount() - next_it->GetIpcLockCount()) : 0);
            if ((first || cur_needs_set_perm || (lock_count == 1)) && !cur_perm_eq) {
                const DisableMergeAttribute head_body_attr = first ? DisableMergeAttribute_EnableHeadAndBody : DisableMergeAttribute_None;
                const DisableMergeAttribute tail_attr      = lock_count == 1 ? DisableMergeAttribute_EnableTail : DisableMergeAttribute_None;
                const KPageProperties properties = { cur_needs_set_perm ? cur_info.GetOriginalPermission() : cur_info.GetPermission(), false, false, static_cast<DisableMergeAttribute>(head_body_attr | tail_attr) };
                R_TRY(this->Operate(updater.GetPageList(), cur_address, cur_size / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, false));
            }
        }

        /* Create an update allocator. */
        /* NOTE: Guaranteed zero blocks needed here. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(0));

        /* Unlock the pages. */
        this->memory_block_manager.UpdateLock(std::addressof(allocator), mapping_start, mapping_size / PageSize, &KMemoryBlock::UnlockForIpc, KMemoryPermission_None);

        /* We succeeded, so no need to unmap. */
        unmap_guard.Cancel();

        return ResultSuccess();
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
        KMemoryBlockManager::const_iterator it = this->memory_block_manager.FindIterator(address);
        while (true) {
            const KMemoryInfo info = it->GetMemoryInfo();

            const auto cur_start = info.GetAddress() >= GetInteger(src_map_start) ? info.GetAddress() : GetInteger(src_map_start);
            const auto cur_end   = src_map_last <= info.GetLastAddress() ? src_map_end : info.GetEndAddress();

            /* If we can, fix the protections on the block. */
            if ((info.GetIpcLockCount() == 0 && (info.GetPermission() & KMemoryPermission_IpcLockChangeMask) != prot_perm) ||
                (info.GetIpcLockCount() != 0 && (info.GetOriginalPermission() & KMemoryPermission_IpcLockChangeMask) != prot_perm))
            {
                /* Check if we actually need to fix the protections on the block. */
                if (cur_end == src_map_end || info.GetAddress() <= GetInteger(src_map_start) || (info.GetPermission() & KMemoryPermission_IpcLockChangeMask) != prot_perm) {
                    const bool start_nc = (info.GetAddress() == GetInteger(src_map_start)) ? ((info.GetDisableMergeAttribute() & (KMemoryBlockDisableMergeAttribute_Locked | KMemoryBlockDisableMergeAttribute_IpcLeft)) == 0) : info.GetAddress() <= GetInteger(src_map_start);

                    const DisableMergeAttribute head_body_attr = start_nc ? DisableMergeAttribute_EnableHeadAndBody : DisableMergeAttribute_None;
                    DisableMergeAttribute tail_attr;
                    if (cur_end == src_map_end && info.GetEndAddress() == src_map_end) {
                        auto next_it = it;
                        ++next_it;

                        const auto lock_count = info.GetIpcLockCount() + (next_it != this->memory_block_manager.end() ? (next_it->GetIpcDisableMergeCount() - next_it->GetIpcLockCount()) : 0);
                        tail_attr = lock_count == 0 ? DisableMergeAttribute_EnableTail : DisableMergeAttribute_None;
                    } else {
                        tail_attr = DisableMergeAttribute_None;
                    }

                    const KPageProperties properties = { info.GetPermission(), false, false, static_cast<DisableMergeAttribute>(head_body_attr | tail_attr) };
                    MESOSPHERE_R_ABORT_UNLESS(this->Operate(page_list, cur_start, (cur_end - cur_start) / PageSize, Null<KPhysicalAddress>, false, properties, OperationType_ChangePermissions, true));
                }
            }

            /* If we're past the end of the region, we're done. */
            if (src_map_last <= info.GetLastAddress()) {
                break;
            }

            /* Advance. */
            ++it;
            MESOSPHERE_ABORT_UNLESS(it != this->memory_block_manager.end());
        }
    }

    #pragma GCC pop_options

    Result KPageTableBase::MapPhysicalMemory(KProcessAddress address, size_t size) {
        /* Lock the physical memory lock. */
        KScopedLightLock phys_lk(this->map_physical_memory_lock);

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
                KScopedLightLock lk(this->general_lock);

                /* Iterate over the memory. */
                cur_address = address;
                mapped_size = 0;

                auto it = this->memory_block_manager.FindIterator(cur_address);
                while (true) {
                    /* Check that the iterator is valid. */
                    MESOSPHERE_ASSERT(it != this->memory_block_manager.end());

                    /* Get the memory info. */
                    const KMemoryInfo info = it->GetMemoryInfo();

                    /* Check if we're done. */
                    if (last_address <= info.GetLastAddress()) {
                        if (info.GetState() != KMemoryState_Free) {
                            mapped_size += (last_address + 1 - cur_address);
                        }
                        break;
                    }

                    /* Track the memory if it's mapped. */
                    if (info.GetState() != KMemoryState_Free) {
                        mapped_size += KProcessAddress(info.GetEndAddress()) - cur_address;
                    }

                    /* Advance. */
                    cur_address = info.GetEndAddress();
                    ++it;
                }

                /* If the size mapped is the size requested, we've nothing to do. */
                R_SUCCEED_IF(size == mapped_size);
            }

            /* Allocate and map the memory. */
            {
                /* Reserve the memory from the process resource limit. */
                KScopedResourceReservation memory_reservation(GetCurrentProcess().GetResourceLimit(), ams::svc::LimitableResource_PhysicalMemoryMax, size - mapped_size);
                R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

                /* Allocate pages for the new memory. */
                KPageGroup pg(this->block_info_manager);
                R_TRY(Kernel::GetMemoryManager().AllocateAndOpenForProcess(std::addressof(pg), (size - mapped_size) / PageSize, this->allocate_option, GetCurrentProcess().GetId(), this->heap_fill_value));

                /* Close our reference when we're done. */
                ON_SCOPE_EXIT { pg.Close(); };

                /* Map the memory. */
                {
                    /* Lock the table. */
                    KScopedLightLock lk(this->general_lock);

                    size_t num_allocator_blocks = 0;

                    /* Verify that nobody has mapped memory since we first checked. */
                    {
                        /* Iterate over the memory. */
                        size_t checked_mapped_size = 0;
                        cur_address = address;

                        auto it = this->memory_block_manager.FindIterator(cur_address);
                        while (true) {
                            /* Check that the iterator is valid. */
                            MESOSPHERE_ASSERT(it != this->memory_block_manager.end());

                            /* Get the memory info. */
                            const KMemoryInfo info = it->GetMemoryInfo();

                            const bool is_free = info.GetState() == KMemoryState_Free;
                            if (is_free) {
                                if (info.GetAddress() < GetInteger(address)) {
                                    ++num_allocator_blocks;
                                }
                                if (last_address < info.GetLastAddress()) {
                                    ++num_allocator_blocks;
                                }
                            }

                            /* Check if we're done. */
                            if (last_address <= info.GetLastAddress()) {
                                if (!is_free) {
                                    checked_mapped_size += (last_address + 1 - cur_address);
                                }
                                break;
                            }

                            /* Track the memory if it's mapped. */
                            if (!is_free) {
                                checked_mapped_size += KProcessAddress(info.GetEndAddress()) - cur_address;
                            }

                            /* Advance. */
                            cur_address = info.GetEndAddress();
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
                    KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
                    R_TRY(allocator.Initialize(num_allocator_blocks));

                    /* We're going to perform an update, so create a helper. */
                    KScopedPageTableUpdater updater(this);

                    /* Reset the current tracking address, and make sure we clean up on failure. */
                    cur_address = address;
                    auto unmap_guard = SCOPE_GUARD {
                        if (cur_address > address) {
                            const KProcessAddress last_unmap_address = cur_address - 1;

                            /* Iterate, unmapping the pages. */
                            cur_address = address;

                            auto it = this->memory_block_manager.FindIterator(cur_address);
                            while (true) {
                                /* Check that the iterator is valid. */
                                MESOSPHERE_ASSERT(it != this->memory_block_manager.end());

                                /* Get the memory info. */
                                const KMemoryInfo info = it->GetMemoryInfo();

                                /* If the memory state is free, we mapped it and need to unmap it. */
                                if (info.GetState() == KMemoryState_Free) {
                                    /* Determine the range to unmap. */
                                    const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                                    const size_t cur_pages = std::min(KProcessAddress(info.GetEndAddress()) - cur_address, last_unmap_address + 1 - cur_address) / PageSize;

                                    /* Unmap. */
                                    MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), cur_address, cur_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
                                }

                                /* Check if we're done. */
                                if (last_unmap_address <= info.GetLastAddress()) {
                                    break;
                                }

                                /* Advance. */
                                cur_address = info.GetEndAddress();
                                ++it;
                            }
                        }
                    };

                    /* Iterate over the memory. */
                    auto pg_it = pg.begin();
                    KPhysicalAddress pg_phys_addr = GetHeapPhysicalAddress(pg_it->GetAddress());
                    size_t pg_pages = pg_it->GetNumPages();

                    auto it = this->memory_block_manager.FindIterator(cur_address);
                    while (true) {
                        /* Check that the iterator is valid. */
                        MESOSPHERE_ASSERT(it != this->memory_block_manager.end());

                        /* Get the memory info. */
                        const KMemoryInfo info = it->GetMemoryInfo();

                        /* If it's unmapped, we need to map it. */
                        if (info.GetState() == KMemoryState_Free) {
                            /* Determine the range to map. */
                            const KPageProperties map_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_None };
                            size_t map_pages                     = std::min(KProcessAddress(info.GetEndAddress()) - cur_address, last_address + 1 - cur_address) / PageSize;

                            /* While we have pages to map, map them. */
                            while (map_pages > 0) {
                                /* Check if we're at the end of the physical block. */
                                if (pg_pages == 0) {
                                    /* Ensure there are more pages to map. */
                                    MESOSPHERE_ASSERT(pg_it != pg.end());

                                    /* Advance our physical block. */
                                    ++pg_it;
                                    pg_phys_addr = GetHeapPhysicalAddress(pg_it->GetAddress());
                                    pg_pages     = pg_it->GetNumPages();
                                }

                                /* Map whatever we can. */
                                const size_t cur_pages = std::min(pg_pages, map_pages);
                                R_TRY(this->Operate(updater.GetPageList(), cur_address, cur_pages, pg_phys_addr, true, map_properties, OperationType_Map, false));

                                /* Advance. */
                                cur_address += cur_pages * PageSize;
                                map_pages   -= cur_pages;

                                pg_phys_addr += cur_pages * PageSize;
                                pg_pages     -= cur_pages;
                            }
                        }

                        /* Check if we're done. */
                        if (last_address <= info.GetLastAddress()) {
                            break;
                        }

                        /* Advance. */
                        cur_address = info.GetEndAddress();
                        ++it;
                    }

                    /* We succeeded, so commit the memory reservation. */
                    memory_reservation.Commit();

                    /* Increase our tracked mapped size. */
                    this->mapped_physical_memory_size += (size - mapped_size);

                    /* Update the relevant memory blocks. */
                    this->memory_block_manager.UpdateIfMatch(std::addressof(allocator), address, size / PageSize,
                                                             KMemoryState_Free,   KMemoryPermission_None,          KMemoryAttribute_None,
                                                             KMemoryState_Normal, KMemoryPermission_UserReadWrite, KMemoryAttribute_None);

                    /* Cancel our guard. */
                    unmap_guard.Cancel();

                    return ResultSuccess();
                }
            }
        }
    }

    Result KPageTableBase::UnmapPhysicalMemory(KProcessAddress address, size_t size) {
        /* Lock the physical memory lock. */
        KScopedLightLock phys_lk(this->map_physical_memory_lock);

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Calculate the last address for convenience. */
        const KProcessAddress last_address = address + size - 1;

        /* Define iteration variables. */
        KProcessAddress cur_address;
        size_t mapped_size;
        size_t num_allocator_blocks = 0;

        /* Check if the memory is mapped. */
        {
            /* Iterate over the memory. */
            cur_address = address;
            mapped_size = 0;

            auto it = this->memory_block_manager.FindIterator(cur_address);
            while (true) {
                /* Check that the iterator is valid. */
                MESOSPHERE_ASSERT(it != this->memory_block_manager.end());

                /* Get the memory info. */
                const KMemoryInfo info = it->GetMemoryInfo();

                /* Verify the memory's state. */
                const bool is_normal = info.GetState() == KMemoryState_Normal && info.GetAttribute() == 0;
                const bool is_free   = info.GetState() == KMemoryState_Free;
                R_UNLESS(is_normal || is_free, svc::ResultInvalidCurrentMemory());

                if (is_normal) {
                    R_UNLESS(info.GetAttribute() == KMemoryAttribute_None, svc::ResultInvalidCurrentMemory());

                    if (info.GetAddress() < GetInteger(address)) {
                        ++num_allocator_blocks;
                    }
                    if (last_address < info.GetLastAddress()) {
                        ++num_allocator_blocks;
                    }
                }

                /* Check if we're done. */
                if (last_address <= info.GetLastAddress()) {
                    if (is_normal) {
                        mapped_size += (last_address + 1 - cur_address);
                    }
                    break;
                }

                /* Track the memory if it's mapped. */
                if (is_normal) {
                    mapped_size += KProcessAddress(info.GetEndAddress()) - cur_address;
                }

                /* Advance. */
                cur_address = info.GetEndAddress();
                ++it;
            }

            /* If there's nothing mapped, we've nothing to do. */
            R_SUCCEED_IF(mapped_size == 0);
        }

        /* Make a page group for the unmap region. */
        KPageGroup pg(this->block_info_manager);
        {
            auto &impl = this->GetImpl();

            /* Begin traversal. */
            TraversalContext context;
            TraversalEntry   cur_entry  = {};
            bool             cur_valid  = false;
            TraversalEntry   next_entry;
            bool             next_valid;
            size_t           tot_size   = 0;

            cur_address = address;
            next_valid  = impl.BeginTraversal(std::addressof(next_entry), std::addressof(context), cur_address);
            next_entry.block_size = (next_entry.block_size - (GetInteger(next_entry.phys_addr) & (next_entry.block_size - 1)));

            /* Iterate, building the group. */
            while (true) {
                if ((!next_valid && !cur_valid) || (next_valid && cur_valid && next_entry.phys_addr == cur_entry.phys_addr + cur_entry.block_size)) {
                    cur_entry.block_size += next_entry.block_size;
                } else {
                    if (cur_valid) {
                        MESOSPHERE_ABORT_UNLESS(IsHeapPhysicalAddress(cur_entry.phys_addr));
                        R_TRY(pg.AddBlock(GetHeapVirtualAddress(cur_entry.phys_addr), cur_entry.block_size / PageSize));
                    }

                    /* Update tracking variables. */
                    tot_size += cur_entry.block_size;
                    cur_entry = next_entry;
                    cur_valid = next_valid;
                }

                if (cur_entry.block_size + tot_size >= size) {
                    break;
                }

                next_valid = impl.ContinueTraversal(std::addressof(next_entry), std::addressof(context));
            }

            /* Add the last block. */
            if (cur_valid) {
                MESOSPHERE_ABORT_UNLESS(IsHeapPhysicalAddress(cur_entry.phys_addr));
                R_TRY(pg.AddBlock(GetHeapVirtualAddress(cur_entry.phys_addr), (size - tot_size) / PageSize));
            }
        }
        MESOSPHERE_ASSERT(pg.GetNumPages() == mapped_size / PageSize);

        /* Create an update allocator. */
        MESOSPHERE_ASSERT(num_allocator_blocks <= KMemoryBlockManagerUpdateAllocator::MaxBlocks);
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Open a reference to the pages, we're unmapping, and close the reference when we're done. */
        pg.Open();
        ON_SCOPE_EXIT { pg.Close(); };

        /* Reset the current tracking address, and make sure we clean up on failure. */
        cur_address = address;
        auto remap_guard = SCOPE_GUARD {
            if (cur_address > address) {
                const KProcessAddress last_map_address = cur_address - 1;
                cur_address = address;

                /* Iterate over the memory we unmapped. */
                auto it = this->memory_block_manager.FindIterator(cur_address);
                auto pg_it = pg.begin();
                KPhysicalAddress pg_phys_addr = GetHeapPhysicalAddress(pg_it->GetAddress());
                size_t pg_pages = pg_it->GetNumPages();

                while (true) {
                    /* Get the memory info for the pages we unmapped, convert to property. */
                    const KMemoryInfo info = it->GetMemoryInfo();
                    const KPageProperties prev_properties = { info.GetPermission(), false, false, DisableMergeAttribute_None };

                    /* If the memory is normal, we unmapped it and need to re-map it. */
                    if (info.GetState() == KMemoryState_Normal) {
                        /* Determine the range to map. */
                        size_t map_pages = std::min(KProcessAddress(info.GetEndAddress()) - cur_address, last_map_address + 1 - cur_address) / PageSize;

                        /* While we have pages to map, map them. */
                        while (map_pages > 0) {
                            /* Check if we're at the end of the physical block. */
                            if (pg_pages == 0) {
                                /* Ensure there are more pages to map. */
                                MESOSPHERE_ABORT_UNLESS(pg_it != pg.end());

                                /* Advance our physical block. */
                                ++pg_it;
                                pg_phys_addr = GetHeapPhysicalAddress(pg_it->GetAddress());
                                pg_pages     = pg_it->GetNumPages();
                            }

                            /* Map whatever we can. */
                            const size_t cur_pages = std::min(pg_pages, map_pages);
                            MESOSPHERE_R_ABORT_UNLESS(this->Operate(updater.GetPageList(), cur_address, cur_pages, pg_phys_addr, true, prev_properties, OperationType_Map, true));

                            /* Advance. */
                            cur_address += cur_pages * PageSize;
                            map_pages   -= cur_pages;

                            pg_phys_addr += cur_pages * PageSize;
                            pg_pages     -= cur_pages;
                        }
                    }

                    /* Check if we're done. */
                    if (last_map_address <= info.GetLastAddress()) {
                        break;
                    }

                    /* Advance. */
                    ++it;
                }
            }
        };

        /* Iterate over the memory, unmapping as we go. */
        auto it = this->memory_block_manager.FindIterator(cur_address);
        while (true) {
            /* Check that the iterator is valid. */
            MESOSPHERE_ASSERT(it != this->memory_block_manager.end());

            /* Get the memory info. */
            const KMemoryInfo info = it->GetMemoryInfo();

            /* If the memory state is normal, we need to unmap it. */
            if (info.GetState() == KMemoryState_Normal) {
                /* Determine the range to unmap. */
                const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
                const size_t cur_pages = std::min(KProcessAddress(info.GetEndAddress()) - cur_address, last_address + 1 - cur_address) / PageSize;

                /* Unmap. */
                R_TRY(this->Operate(updater.GetPageList(), cur_address, cur_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));
            }

            /* Check if we're done. */
            if (last_address <= info.GetLastAddress()) {
                break;
            }

            /* Advance. */
            cur_address = info.GetEndAddress();
            ++it;
        }

        /* Release the memory resource. */
        this->mapped_physical_memory_size -= mapped_size;
        GetCurrentProcess().ReleaseResource(ams::svc::LimitableResource_PhysicalMemoryMax, mapped_size);

        /* Update memory blocks. */
        this->memory_block_manager.Update(std::addressof(allocator), address, size / PageSize, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);

        /* We succeeded. */
        remap_guard.Cancel();
        return ResultSuccess();
    }

    Result KPageTableBase::MapPhysicalMemoryUnsafe(KProcessAddress address, size_t size) {
        /* Try to reserve the unsafe memory. */
        R_UNLESS(Kernel::GetUnsafeMemory().TryReserve(size), svc::ResultLimitReached());

        /* Ensure we release our reservation on failure. */
        auto reserve_guard = SCOPE_GUARD { Kernel::GetUnsafeMemory().Release(size); };

        /* Create a page group for the new memory. */
        KPageGroup pg(this->block_info_manager);

        /* Allocate the new memory. */
        const size_t num_pages = size / PageSize;
        R_TRY(Kernel::GetMemoryManager().AllocateAndOpen(std::addressof(pg), num_pages, KMemoryManager::EncodeOption(KMemoryManager::Pool_Unsafe, KMemoryManager::Direction_FromFront)));

        /* Close the page group when we're done with it. */
        ON_SCOPE_EXIT { pg.Close(); };

        /* Clear the new memory. */
        for (const auto &block : pg) {
            std::memset(GetVoidPointer(block.GetAddress()), this->heap_fill_value, block.GetSize());
        }

        /* Map the new memory. */
        {
            /* Lock the table. */
            KScopedLightLock lk(this->general_lock);

            /* Check the memory state. */
            size_t num_allocator_blocks;
            R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

            /* Create an update allocator. */
            KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
            R_TRY(allocator.Initialize(num_allocator_blocks));

            /* We're going to perform an update, so create a helper. */
            KScopedPageTableUpdater updater(this);

            /* Map the pages. */
            const KPageProperties map_properties = { KMemoryPermission_UserReadWrite, false, false, DisableMergeAttribute_DisableHead };
            R_TRY(this->Operate(updater.GetPageList(), address, num_pages, pg, map_properties, OperationType_MapGroup, false));

            /* Apply the memory block update. */
            this->memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState_Normal, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_Normal, KMemoryBlockDisableMergeAttribute_None);

            /* Update our mapped unsafe size. */
            this->mapped_unsafe_physical_memory += size;

            /* We succeeded. */
            reserve_guard.Cancel();
            return ResultSuccess();
        }
    }

    Result KPageTableBase::UnmapPhysicalMemoryUnsafe(KProcessAddress address, size_t size) {
        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check whether we can unmap this much unsafe physical memory. */
        R_UNLESS(size <= this->mapped_unsafe_physical_memory, svc::ResultInvalidCurrentMemory());

        /* Check the memory state. */
        size_t num_allocator_blocks;
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size, KMemoryState_All, KMemoryState_Normal, KMemoryPermission_All, KMemoryPermission_UserReadWrite, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.Initialize(num_allocator_blocks));

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Unmap the memory. */
        const size_t num_pages = size / PageSize;
        const KPageProperties unmap_properties = { KMemoryPermission_None, false, false, DisableMergeAttribute_None };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, false));

        /* Apply the memory block update. */
        this->memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_Normal);

        /* Release the unsafe memory from the limit. */
        Kernel::GetUnsafeMemory().Release(size);

        /* Update our mapped unsafe size. */
        this->mapped_unsafe_physical_memory -= size;

        return ResultSuccess();
    }

}
