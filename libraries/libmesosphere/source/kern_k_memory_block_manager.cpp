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

namespace ams::kern {

    namespace {

        constexpr const std::pair<KMemoryState, const char *> MemoryStateNames[] = {
            {KMemoryState_Free               , "----- Free -----"},
            {KMemoryState_IoMemory           , "IoMemory        "},
            {KMemoryState_IoRegister         , "IoRegister      "},
            {KMemoryState_Static             , "Static          "},
            {KMemoryState_Code               , "Code            "},
            {KMemoryState_CodeData           , "CodeData        "},
            {KMemoryState_Normal             , "Normal          "},
            {KMemoryState_Shared             , "Shared          "},
            {KMemoryState_AliasCode          , "AliasCode       "},
            {KMemoryState_AliasCodeData      , "AliasCodeData   "},
            {KMemoryState_Ipc                , "Ipc             "},
            {KMemoryState_Stack              , "Stack           "},
            {KMemoryState_ThreadLocal        , "ThreadLocal     "},
            {KMemoryState_Transfered         , "Transfered      "},
            {KMemoryState_SharedTransfered   , "SharedTransfered"},
            {KMemoryState_SharedCode         , "SharedCode      "},
            {KMemoryState_Inaccessible       , "Inaccessible    "},
            {KMemoryState_NonSecureIpc       , "NonSecureIpc    "},
            {KMemoryState_NonDeviceIpc       , "NonDeviceIpc    "},
            {KMemoryState_Kernel             , "Kernel          "},
            {KMemoryState_GeneratedCode      , "GeneratedCode   "},
            {KMemoryState_CodeOut            , "CodeOut         "},
            {KMemoryState_Coverage           , "Coverage        "},
        };

        constexpr const char *GetMemoryStateName(KMemoryState state) {
            for (size_t i = 0; i < util::size(MemoryStateNames); i++) {
                if (std::get<0>(MemoryStateNames[i]) == state) {
                    return std::get<1>(MemoryStateNames[i]);
                }
            }
            return "Unknown         ";
        }

        constexpr const char *GetMemoryPermissionString(const KMemoryBlock &block) {
            if (block.GetState() == KMemoryState_Free) {
                return "   ";
            } else {
                switch (block.GetPermission()) {
                    case KMemoryPermission_UserReadExecute:
                        return "r-x";
                    case KMemoryPermission_UserRead:
                        return "r--";
                    case KMemoryPermission_UserReadWrite:
                        return "rw-";
                    default:
                        return "---";
                }
            }
        }

        void DumpMemoryBlock(const KMemoryBlock &block) {
            const char *state     = GetMemoryStateName(block.GetState());
            const char *perm      = GetMemoryPermissionString(block);
            const uintptr_t start = GetInteger(block.GetAddress());
            const uintptr_t end   = GetInteger(block.GetLastAddress());
            const size_t kb   = block.GetSize() / 1_KB;

            const char l = (block.GetAttribute() & KMemoryAttribute_Locked)       ? 'L' : '-';
            const char i = (block.GetAttribute() & KMemoryAttribute_IpcLocked)    ? 'I' : '-';
            const char d = (block.GetAttribute() & KMemoryAttribute_DeviceShared) ? 'D' : '-';
            const char u = (block.GetAttribute() & KMemoryAttribute_Uncached)     ? 'U' : '-';

            MESOSPHERE_LOG("0x%10lx - 0x%10lx (%9zu KB) %s %s %c%c%c%c [%d, %d]\n", start, end, kb, perm, state, l, i, d, u, block.GetIpcLockCount(), block.GetDeviceUseCount());
        }

    }

    Result KMemoryBlockManager::Initialize(KProcessAddress st, KProcessAddress nd, KMemoryBlockSlabManager *slab_manager) {
        /* Allocate a block to encapsulate the address space, insert it into the tree. */
        KMemoryBlock *start_block = slab_manager->Allocate();
        R_UNLESS(start_block != nullptr, svc::ResultOutOfResource());

        /* Set our start and end. */
        m_start_address = st;
        m_end_address   = nd;
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(m_start_address), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(m_end_address), PageSize));

        /* Initialize and insert the block. */
        start_block->Initialize(m_start_address, (m_end_address - m_start_address) / PageSize, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None);
        m_memory_block_tree.insert(*start_block);

        R_SUCCEED();
    }

    void KMemoryBlockManager::Finalize(KMemoryBlockSlabManager *slab_manager) {
        /* Erase every block until we have none left. */
        auto it = m_memory_block_tree.begin();
        while (it != m_memory_block_tree.end()) {
            KMemoryBlock *block = std::addressof(*it);
            it = m_memory_block_tree.erase(it);
            slab_manager->Free(block);
        }

        MESOSPHERE_ASSERT(m_memory_block_tree.empty());
    }

    bool KMemoryBlockManager::GetRegionForFindFreeArea(KProcessAddress *out_start, KProcessAddress *out_end, KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) {
        /* Check that there's room for the pages in the specified region. */
        if (num_pages + 2 * guard_pages > region_num_pages) {
            return false;
        }

        /* Determine the aligned start of the guarded region. */
        const KProcessAddress guarded_start                     = region_start + guard_pages * PageSize;
        const KProcessAddress aligned_guarded_start             = util::AlignDown(GetInteger(guarded_start), alignment);
              KProcessAddress aligned_guarded_start_with_offset = aligned_guarded_start + offset;
        if (guarded_start > aligned_guarded_start_with_offset) {
            if (!util::CanAddWithoutOverflow<uintptr_t>(GetInteger(aligned_guarded_start), alignment)) {
                return false;
            }
            aligned_guarded_start_with_offset += alignment;
        }

        /* Determine the aligned end of the guarded region. */
        const KProcessAddress guarded_end                     = region_start + ((region_num_pages - (num_pages + guard_pages)) * PageSize);
        const KProcessAddress aligned_guarded_end             = util::AlignDown(GetInteger(guarded_end), alignment);
              KProcessAddress aligned_guarded_end_with_offset = aligned_guarded_end + offset;
        if (aligned_guarded_end_with_offset > guarded_end) {
            if (aligned_guarded_end < alignment) {
                return false;
            }
            aligned_guarded_end_with_offset -= alignment;
        }

        /* Check that the extents are valid. */
        if (aligned_guarded_end_with_offset < aligned_guarded_start_with_offset) {
            return false;
        }

        /* Set the output extents. */
        *out_start = aligned_guarded_start_with_offset;
        *out_end   = aligned_guarded_end_with_offset;
        return true;
    }

    KProcessAddress KMemoryBlockManager::FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const {
        /* Determine the range to search in. */
        KProcessAddress search_start = Null<KProcessAddress>;
        KProcessAddress search_end   = Null<KProcessAddress>;
        if (this->GetRegionForFindFreeArea(std::addressof(search_start), std::addressof(search_end), region_start, region_num_pages, num_pages, alignment, offset, guard_pages)) {
            /* Iterate over blocks in the search space, looking for a suitable one. */
            for (const_iterator it = this->FindIterator(search_start); it != m_memory_block_tree.cend(); it++) {
                /* If our block is past the end of our search space, we're done. */
                if (search_end < it->GetAddress()) {
                    break;
                }

                /* We only want to consider free blocks. */
                if (it->GetState() != KMemoryState_Free) {
                    continue;
                }

                /* Determine the candidate range. */
                KProcessAddress candidate_start = Null<KProcessAddress>;
                KProcessAddress candidate_end   = Null<KProcessAddress>;
                if (!this->GetRegionForFindFreeArea(std::addressof(candidate_start), std::addressof(candidate_end), it->GetAddress(), it->GetNumPages(), num_pages, alignment, offset, guard_pages)) {
                    continue;
                }

                /* Try the suggested candidate (coercing into the search region if needed). */
                KProcessAddress candidate = candidate_start;
                if (candidate < search_start) {
                    candidate = search_start;
                }

                /* Check if the candidate is valid. */
                if (candidate <= search_end && candidate <= candidate_end) {
                    return candidate;
                }
            }
        }

        return Null<KProcessAddress>;
    }

    void KMemoryBlockManager::CoalesceForUpdate(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages) {
        /* Find the iterator now that we've updated. */
        iterator it = this->FindIterator(address);
        if (address != m_start_address) {
            it--;
        }

        /* Coalesce blocks that we can. */
        while (true) {
            iterator prev = it++;
            if (it == m_memory_block_tree.end()) {
                break;
            }

            if (prev->CanMergeWith(*it)) {
                KMemoryBlock *block = std::addressof(*it);
                m_memory_block_tree.erase(it);
                prev->Add(*block);
                allocator->Free(block);
                it = prev;
            }

            if (address + num_pages * PageSize < it->GetEndAddress()) {
                break;
            }
        }
    }

    void KMemoryBlockManager::Update(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr, KMemoryBlockDisableMergeAttribute set_disable_attr, KMemoryBlockDisableMergeAttribute clear_disable_attr) {
        /* Ensure for auditing that we never end up with an invalid tree. */
        KScopedMemoryBlockManagerAuditor auditor(this);
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ASSERT((attr & (KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared)) == 0);

        KProcessAddress cur_address = address;
        size_t remaining_pages = num_pages;
        iterator it = this->FindIterator(address);

        while (remaining_pages > 0) {
            const size_t remaining_size = remaining_pages * PageSize;
            if (it->HasProperties(state, perm, attr)) {
                /* If we already have the right properties, just advance. */
                if (cur_address + remaining_size < it->GetEndAddress()) {
                    remaining_pages = 0;
                    cur_address += remaining_size;
                } else {
                    remaining_pages = (cur_address + remaining_size - it->GetEndAddress()) / PageSize;
                    cur_address = it->GetEndAddress();
                }
            } else {
                /* If we need to, create a new block before and insert it. */
                if (it->GetAddress() != GetInteger(cur_address)) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address);
                    it = m_memory_block_tree.insert(*new_block);
                    it++;

                    cur_address = it->GetAddress();
                }

                /* If we need to, create a new block after and insert it. */
                if (it->GetSize() > remaining_size) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address + remaining_size);
                    it = m_memory_block_tree.insert(*new_block);
                }

                /* Update block state. */
                it->Update(state, perm, attr, it->GetAddress() == address, set_disable_attr, clear_disable_attr);
                cur_address += it->GetSize();
                remaining_pages -= it->GetNumPages();
            }
            it++;
        }

        this->CoalesceForUpdate(allocator, address, num_pages);
    }

    void KMemoryBlockManager::UpdateIfMatch(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState test_state, KMemoryPermission test_perm, KMemoryAttribute test_attr, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr, KMemoryBlockDisableMergeAttribute set_disable_attr, KMemoryBlockDisableMergeAttribute clear_disable_attr) {
        /* Ensure for auditing that we never end up with an invalid tree. */
        KScopedMemoryBlockManagerAuditor auditor(this);
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ASSERT((attr & (KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared)) == 0);

        KProcessAddress cur_address = address;
        size_t remaining_pages = num_pages;
        iterator it = this->FindIterator(address);

        while (remaining_pages > 0) {
            const size_t remaining_size = remaining_pages * PageSize;
            if (it->HasProperties(test_state, test_perm, test_attr) && !it->HasProperties(state, perm, attr)) {
                /* If we need to, create a new block before and insert it. */
                if (it->GetAddress() != GetInteger(cur_address)) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address);
                    it = m_memory_block_tree.insert(*new_block);
                    it++;

                    cur_address = it->GetAddress();
                }

                /* If we need to, create a new block after and insert it. */
                if (it->GetSize() > remaining_size) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address + remaining_size);
                    it = m_memory_block_tree.insert(*new_block);
                }

                /* Update block state. */
                it->Update(state, perm, attr, it->GetAddress() == address, set_disable_attr, clear_disable_attr);
                cur_address     += it->GetSize();
                remaining_pages -= it->GetNumPages();
            } else {
                /* If we already have the right properties, just advance. */
                if (cur_address + remaining_size < it->GetEndAddress()) {
                    remaining_pages = 0;
                    cur_address += remaining_size;
                } else {
                    remaining_pages = (cur_address + remaining_size - it->GetEndAddress()) / PageSize;
                    cur_address     = it->GetEndAddress();
                }
            }
            it++;
        }

        this->CoalesceForUpdate(allocator, address, num_pages);
    }

    void KMemoryBlockManager::UpdateLock(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, MemoryBlockLockFunction lock_func, KMemoryPermission perm) {
        /* Ensure for auditing that we never end up with an invalid tree. */
        KScopedMemoryBlockManagerAuditor auditor(this);
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));

        KProcessAddress cur_address = address;
        size_t remaining_pages = num_pages;
        iterator it   = this->FindIterator(address);

        const KProcessAddress end_address = address + (num_pages * PageSize);

        while (remaining_pages > 0) {
            const size_t remaining_size = remaining_pages * PageSize;

            /* If we need to, create a new block before and insert it. */
            if (it->GetAddress() != cur_address) {
                KMemoryBlock *new_block = allocator->Allocate();

                it->Split(new_block, cur_address);
                it = m_memory_block_tree.insert(*new_block);
                it++;

                cur_address = it->GetAddress();
            }

            if (it->GetSize() > remaining_size) {
                /* If we need to, create a new block after and insert it. */
                KMemoryBlock *new_block = allocator->Allocate();

                it->Split(new_block, cur_address + remaining_size);
                it = m_memory_block_tree.insert(*new_block);
            }

            /* Call the locked update function. */
            (std::addressof(*it)->*lock_func)(perm, it->GetAddress() == address, it->GetEndAddress() == end_address);
            cur_address += it->GetSize();
            remaining_pages -= it->GetNumPages();
            it++;
        }

        this->CoalesceForUpdate(allocator, address, num_pages);
    }

    void KMemoryBlockManager::UpdateAttribute(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, u32 mask, u32 attr) {
        /* Ensure for auditing that we never end up with an invalid tree. */
        KScopedMemoryBlockManagerAuditor auditor(this);
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));

        KProcessAddress cur_address = address;
        size_t remaining_pages = num_pages;
        iterator it   = this->FindIterator(address);

        while (remaining_pages > 0) {
            const size_t remaining_size = remaining_pages * PageSize;

            if ((it->GetAttribute() & mask) != attr) {
                /* If we need to, create a new block before and insert it. */
                if (it->GetAddress() != GetInteger(cur_address)) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address);
                    it = m_memory_block_tree.insert(*new_block);
                    it++;

                    cur_address = it->GetAddress();
                }

                /* If we need to, create a new block after and insert it. */
                if (it->GetSize() > remaining_size) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address + remaining_size);
                    it = m_memory_block_tree.insert(*new_block);
                }

                /* Update block state. */
                it->UpdateAttribute(mask, attr);
                cur_address     += it->GetSize();
                remaining_pages -= it->GetNumPages();
            } else {
                /* If we already have the right attributes, just advance. */
                if (cur_address + remaining_size < it->GetEndAddress()) {
                    remaining_pages = 0;
                    cur_address += remaining_size;
                } else {
                    remaining_pages = (cur_address + remaining_size - it->GetEndAddress()) / PageSize;
                    cur_address     = it->GetEndAddress();
                }
            }
            it++;
        }

        this->CoalesceForUpdate(allocator, address, num_pages);
    }

    /* Debug. */
    bool KMemoryBlockManager::CheckState() const {
        /* If we fail, we should dump blocks. */
        auto dump_guard = SCOPE_GUARD { this->DumpBlocks(); };

        /* Loop over every block, ensuring that we are sorted and coalesced. */
        auto it   = m_memory_block_tree.cbegin();
        auto prev = it++;
        while (it != m_memory_block_tree.cend()) {

            /* Sequential blocks which can be merged should be merged. */
            if (prev->CanMergeWith(*it)) {
                return false;
            }

            /* Sequential blocks should be sequential. */
            if (prev->GetEndAddress() != it->GetAddress()) {
                return false;
            }

            /* If the block is ipc locked, it must have a count. */
            if ((it->GetAttribute() & KMemoryAttribute_IpcLocked) != 0 && it->GetIpcLockCount() == 0) {
                return false;
            }

            /* If the block is device shared, it must have a count. */
            if ((it->GetAttribute() & KMemoryAttribute_DeviceShared) != 0 && it->GetDeviceUseCount() == 0) {
                return false;
            }

            /* Advance the iterator. */
            prev = it++;
        }

        /* Our loop will miss checking the last block, potentially, so check it. */
        if (prev != m_memory_block_tree.cend()) {
            /* If the block is ipc locked, it must have a count. */
            if ((prev->GetAttribute() & KMemoryAttribute_IpcLocked) != 0 && prev->GetIpcLockCount() == 0) {
                return false;
            }

            /* If the block is device shared, it must have a count. */
            if ((prev->GetAttribute() & KMemoryAttribute_DeviceShared) != 0 && prev->GetDeviceUseCount() == 0) {
                return false;
            }
        }

        /* We're valid, so no need to print. */
        dump_guard.Cancel();
        return true;
    }

    void KMemoryBlockManager::DumpBlocks() const {
        /* Dump each block. */
        for (const auto &block : m_memory_block_tree) {
            DumpMemoryBlock(block);
        }
    }
}
