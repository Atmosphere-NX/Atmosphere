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

        this->cached_physical_linear_region = nullptr;
        this->cached_physical_heap_region   = nullptr;
        this->cached_virtual_heap_region    = nullptr;

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

    bool KPageTableBase::Contains(KProcessAddress addr, size_t size, KMemoryState state) const {
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
        R_UNLESS((info.perm      & perm_mask)  == perm, svc::ResultInvalidCurrentMemory());
        R_UNLESS((info.attribute & attr_mask)  == attr, svc::ResultInvalidCurrentMemory());

        return ResultSuccess();
    }

    Result KPageTableBase::CheckMemoryState(KMemoryState *out_state, KMemoryPermission *out_perm, KMemoryAttribute *out_attr, KProcessAddress addr, size_t size, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr, u32 ignore_attr) const {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Get information about the first block. */
        const KProcessAddress last_addr = addr + size - 1;
        KMemoryBlockManager::const_iterator it = this->memory_block_manager.FindIterator(addr);
        KMemoryInfo info = it->GetMemoryInfo();

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

        /* Write output state. */
        if (out_state) {
            *out_state = first_state;
        }
        if (out_perm) {
            *out_perm = first_perm;
        }
        if (out_attr) {
            *out_attr = static_cast<KMemoryAttribute>(first_attr & ~ignore_attr);
        }
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
                    /* TODO: Should we account for guard pages? */
                    const size_t offset_pages = KSystemControl::GenerateRandomRange(0, region_num_pages - num_pages);
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

    Result KPageTableBase::AllocateAndMapPagesImpl(PageLinkedList *page_list, KProcessAddress address, size_t num_pages, const KPageProperties properties) {
        MESOSPHERE_ASSERT(this->IsLockedByCurrentThread());

        /* Create a page group to hold the pages we allocate. */
        KPageGroup pg(this->block_info_manager);

        /* Allocate the pages. */
        R_TRY(Kernel::GetMemoryManager().Allocate(std::addressof(pg), num_pages, this->allocate_option));

        /* Ensure that the page group is open while we work with it. */
        KScopedPageGroup spg(pg);

        /* Clear all pages. */
        for (const auto &it : pg) {
            std::memset(GetVoidPointer(it.GetAddress()), this->heap_fill_value, it.GetSize());
        }

        /* Map the pages. */
        return this->Operate(page_list, address, num_pages, std::addressof(pg), properties, OperationType_MapGroup, false);
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
                const KPageProperties unmap_properties = {};
                MESOSPHERE_R_ABORT_UNLESS(this->Operate(page_list, start_address, (cur_address - start_address) / PageSize, Null<KPhysicalAddress>, false, unmap_properties, OperationType_Unmap, true));
            }
        };

        /* Iterate, mapping all pages in the group. */
        for (const auto &block : pg) {
            /* We only allow mapping pages in the heap, and we require we're mapping non-empty blocks. */
            MESOSPHERE_ABORT_UNLESS(block.GetAddress() < block.GetLastAddress());
            MESOSPHERE_ABORT_UNLESS(IsHeapVirtualAddress(block.GetAddress(), block.GetSize()));

            /* Map and advance. */
            R_TRY(this->Operate(page_list, cur_address, block.GetNumPages(), GetHeapPhysicalAddress(block.GetAddress()), true, properties, OperationType_Map, reuse_ll));
            cur_address += block.GetSize();
        }

        /* We succeeded! */
        mapping_guard.Cancel();
        return ResultSuccess();
    }

    bool KPageTableBase::IsValidPageGroup(const KPageGroup &pg, KProcessAddress addr, size_t num_pages) {
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
        while (tot_size < num_pages * PageSize) {
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
        if (tot_size > num_pages * PageSize) {
            cur_size -= (tot_size - num_pages * PageSize);
        }

        if (!IsHeapPhysicalAddress(cur_addr)) {
            return false;
        }

        if (!UpdateCurrentIterator()) {
            return false;
        }

        return cur_block_address == GetHeapVirtualAddress(cur_addr) && cur_block_pages == (cur_size / PageSize);
    }

    Result KPageTableBase::MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, bool is_pa_valid, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(util::IsAligned(alignment, PageSize) && alignment >= PageSize);

        /* Ensure this is a valid map request. */
        R_UNLESS(this->Contains(region_start, region_num_pages * PageSize, state), svc::ResultInvalidCurrentMemory());
        R_UNLESS(num_pages < region_num_pages,                                     svc::ResultOutOfMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Find a random address to map at. */
        KProcessAddress addr = this->FindFreeArea(region_start, region_num_pages, num_pages, alignment, 0, this->GetNumGuardPages());
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), alignment));
        MESOSPHERE_ASSERT(this->Contains(addr, num_pages * PageSize, state));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.GetResult());

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, false, false, false };
        if (is_pa_valid) {
            R_TRY(this->Operate(updater.GetPageList(), addr, num_pages, phys_addr, true, properties, OperationType_Map, false));
        } else {
            R_TRY(this->AllocateAndMapPagesImpl(updater.GetPageList(), addr, num_pages, properties));
        }

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, state, perm, KMemoryAttribute_None);

        /* We successfully mapped the pages. */
        *out_addr = addr;
        return ResultSuccess();
    }

    Result KPageTableBase::UnmapPages(KProcessAddress address, size_t num_pages, KMemoryState state) {
        MESOSPHERE_TODO_IMPLEMENT();
    }

    Result KPageTableBase::MapPageGroup(KProcessAddress *out_addr, const KPageGroup &pg, KProcessAddress region_start, size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

        /* Ensure this is a valid map request. */
        const size_t num_pages = pg.GetNumPages();
        R_UNLESS(this->Contains(region_start, region_num_pages * PageSize, state), svc::ResultInvalidCurrentMemory());
        R_UNLESS(num_pages < region_num_pages,                                     svc::ResultOutOfMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Find a random address to map at. */
        KProcessAddress addr = this->FindFreeArea(region_start, region_num_pages, num_pages, PageSize, 0, this->GetNumGuardPages());
        R_UNLESS(addr != Null<KProcessAddress>, svc::ResultOutOfMemory());
        MESOSPHERE_ASSERT(this->Contains(addr, num_pages * PageSize, state));
        MESOSPHERE_R_ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState_All, KMemoryState_Free, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_None, KMemoryAttribute_None));

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.GetResult());

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform mapping operation. */
        const KPageProperties properties = { perm, state == KMemoryState_Io, false, false };
        R_TRY(this->MapPageGroupImpl(updater.GetPageList(), addr, pg, properties, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, addr, num_pages, state, perm, KMemoryAttribute_None);

        /* We successfully mapped the pages. */
        *out_addr = addr;
        return ResultSuccess();
    }

    Result KPageTableBase::UnmapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state) {
        MESOSPHERE_ASSERT(!this->IsLockedByCurrentThread());

        /* Ensure this is a valid unmap request. */
        const size_t num_pages = pg.GetNumPages();
        const size_t size = num_pages * PageSize;
        R_UNLESS(this->Contains(address, size, state), svc::ResultInvalidCurrentMemory());

        /* Lock the table. */
        KScopedLightLock lk(this->general_lock);

        /* Check if state allows us to unmap. */
        R_TRY(this->CheckMemoryState(address, size, KMemoryState_All, state, KMemoryPermission_None, KMemoryPermission_None, KMemoryAttribute_All, KMemoryAttribute_None));

        /* Check that the page group is valid. */
        R_UNLESS(this->IsValidPageGroup(pg, address, num_pages), svc::ResultInvalidCurrentMemory());

        /* Create an update allocator. */
        KMemoryBlockManagerUpdateAllocator allocator(this->memory_block_slab_manager);
        R_TRY(allocator.GetResult());

        /* We're going to perform an update, so create a helper. */
        KScopedPageTableUpdater updater(this);

        /* Perform unmapping operation. */
        const KPageProperties properties = { KMemoryPermission_None, false, false, false };
        R_TRY(this->Operate(updater.GetPageList(), address, num_pages, Null<KPhysicalAddress>, false, properties, OperationType_Unmap, false));

        /* Update the blocks. */
        this->memory_block_manager.Update(&allocator, address, num_pages, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None);

        return ResultSuccess();
    }

}
