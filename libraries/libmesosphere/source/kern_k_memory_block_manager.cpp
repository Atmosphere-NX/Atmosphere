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

namespace ams::kern {

    namespace {

        constexpr std::tuple<KMemoryState, const char *> MemoryStateNames[] = {
            {KMemoryState_Free               , "----- Free -----"},
            {KMemoryState_Io                 , "Io              "},
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
        };

        constexpr const char *GetMemoryStateName(KMemoryState state) {
            for (size_t i = 0; i < util::size(MemoryStateNames); i++) {
                if (std::get<0>(MemoryStateNames[i]) == state) {
                    return std::get<1>(MemoryStateNames[i]);
                }
            }
            return "Unknown         ";
        }

        constexpr const char *GetMemoryPermissionString(const KMemoryInfo &info) {
            if (info.state == KMemoryState_Free) {
                return "   ";
            } else {
                switch (info.perm) {
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

        void DumpMemoryInfo(const KMemoryInfo &info) {
            const char *state     = GetMemoryStateName(info.state);
            const char *perm      = GetMemoryPermissionString(info);
            const uintptr_t start = info.GetAddress();
            const uintptr_t end   = info.GetLastAddress();
            const size_t kb   = info.GetSize() / 1_KB;

            const char l = (info.attribute & KMemoryAttribute_Locked)       ? 'L' : '-';
            const char i = (info.attribute & KMemoryAttribute_IpcLocked)    ? 'I' : '-';
            const char d = (info.attribute & KMemoryAttribute_DeviceShared) ? 'D' : '-';
            const char u = (info.attribute & KMemoryAttribute_Uncached)     ? 'U' : '-';

            MESOSPHERE_LOG("0x%10lx - 0x%10lx (%9zu KB) %s %s %c%c%c%c [%d, %d]\n", start, end, kb, perm, state, l, i, d, u, info.ipc_lock_count, info.device_use_count);
        }

    }

    Result KMemoryBlockManager::Initialize(KProcessAddress st, KProcessAddress nd, KMemoryBlockSlabManager *slab_manager) {
        /* Allocate a block to encapsulate the address space, insert it into the tree. */
        KMemoryBlock *start_block = slab_manager->Allocate();
        R_UNLESS(start_block != nullptr, svc::ResultOutOfResource());

        /* Set our start and end. */
        this->start_address = st;
        this->end_address   = nd;
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(this->start_address), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(this->end_address), PageSize));

        /* Initialize and insert the block. */
        start_block->Initialize(this->start_address, (this->end_address - this->start_address) / PageSize, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None);
        this->memory_block_tree.insert(*start_block);

        return ResultSuccess();
    }

    void KMemoryBlockManager::Finalize(KMemoryBlockSlabManager *slab_manager) {
        /* Erase every block until we have none left. */
        auto it = this->memory_block_tree.begin();
        while (it != this->memory_block_tree.end()) {
            KMemoryBlock *block = std::addressof(*it);
            it = this->memory_block_tree.erase(it);
            slab_manager->Free(block);
        }

        MESOSPHERE_ASSERT(this->memory_block_tree.empty());
    }

    KProcessAddress KMemoryBlockManager::FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const {
        if (num_pages > 0) {
            const KProcessAddress region_end  = region_start + region_num_pages * PageSize;
            const KProcessAddress region_last = region_end - 1;
            for (const_iterator it = this->FindIterator(region_start); it != this->memory_block_tree.cend(); it++) {
                const KMemoryInfo info = it->GetMemoryInfo();
                if (region_last < info.GetAddress()) {
                    break;
                }
                if (info.state != KMemoryState_Free) {
                    continue;
                }

                KProcessAddress area = (info.GetAddress() <= GetInteger(region_start)) ? region_start : info.GetAddress();
                area += guard_pages * PageSize;

                const KProcessAddress offset_area = util::AlignDown(GetInteger(area), alignment) + offset;
                area = (area <= offset_area) ? offset_area : offset_area + alignment;

                const KProcessAddress area_end = area + num_pages * PageSize + guard_pages * PageSize;
                const KProcessAddress area_last = area_end - 1;

                if (info.GetAddress() <= GetInteger(area) && area < area_last && area_last <= region_last && GetInteger(area_last) <= info.GetLastAddress()) {
                    return area;
                }
            }
        }

        return Null<KProcessAddress>;
    }

    void KMemoryBlockManager::CoalesceForUpdate(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages) {
        /* Find the iterator now that we've updated. */
        iterator it = this->FindIterator(address);
        if (address != this->start_address) {
            it--;
        }

        /* Coalesce blocks that we can. */
        while (true) {
            iterator prev = it++;
            if (it == this->memory_block_tree.end()) {
                break;
            }

            if (prev->CanMergeWith(*it)) {
                KMemoryBlock *block = std::addressof(*it);
                this->memory_block_tree.erase(it);
                prev->Add(*block);
                allocator->Free(block);
                it = prev;
            }

            if (address + num_pages * PageSize < it->GetMemoryInfo().GetEndAddress()) {
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
            KMemoryInfo cur_info = it->GetMemoryInfo();
            if (it->HasProperties(state, perm, attr)) {
                /* If we already have the right properties, just advance. */
                if (cur_address + remaining_size < cur_info.GetEndAddress()) {
                    remaining_pages = 0;
                    cur_address += remaining_size;
                } else {
                    remaining_pages = (cur_address + remaining_size - cur_info.GetEndAddress()) / PageSize;
                    cur_address = cur_info.GetEndAddress();
                }
            } else {
                /* If we need to, create a new block before and insert it. */
                if (cur_info.GetAddress() != GetInteger(cur_address)) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address);
                    it = this->memory_block_tree.insert(*new_block);
                    it++;

                    cur_info = it->GetMemoryInfo();
                    cur_address = cur_info.GetAddress();
                }

                /* If we need to, create a new block after and insert it. */
                if (cur_info.GetSize() > remaining_size) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address + remaining_size);
                    it = this->memory_block_tree.insert(*new_block);

                    cur_info = it->GetMemoryInfo();
                }

                /* Update block state. */
                it->Update(state, perm, attr, cur_address == address, set_disable_attr, clear_disable_attr);
                cur_address += cur_info.GetSize();
                remaining_pages -= cur_info.GetNumPages();
            }
            it++;
        }

        this->CoalesceForUpdate(allocator, address, num_pages);
    }

    void KMemoryBlockManager::UpdateIfMatch(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState test_state, KMemoryPermission test_perm, KMemoryAttribute test_attr, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr) {
        /* Ensure for auditing that we never end up with an invalid tree. */
        KScopedMemoryBlockManagerAuditor auditor(this);
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ASSERT((attr & (KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared)) == 0);

        KProcessAddress cur_address = address;
        size_t remaining_pages = num_pages;
        iterator it = this->FindIterator(address);

        while (remaining_pages > 0) {
            const size_t remaining_size = remaining_pages * PageSize;
            KMemoryInfo cur_info = it->GetMemoryInfo();
            if (it->HasProperties(test_state, test_perm, test_attr) && !it->HasProperties(state, perm, attr)) {
                /* If we need to, create a new block before and insert it. */
                if (cur_info.GetAddress() != GetInteger(cur_address)) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address);
                    it = this->memory_block_tree.insert(*new_block);
                    it++;

                    cur_info    = it->GetMemoryInfo();
                    cur_address = cur_info.GetAddress();
                }

                /* If we need to, create a new block after and insert it. */
                if (cur_info.GetSize() > remaining_size) {
                    KMemoryBlock *new_block = allocator->Allocate();

                    it->Split(new_block, cur_address + remaining_size);
                    it = this->memory_block_tree.insert(*new_block);

                    cur_info = it->GetMemoryInfo();
                }

                /* Update block state. */
                it->Update(state, perm, attr, false, KMemoryBlockDisableMergeAttribute_None, KMemoryBlockDisableMergeAttribute_None);
                cur_address     += cur_info.GetSize();
                remaining_pages -= cur_info.GetNumPages();
            } else {
                /* If we already have the right properties, just advance. */
                if (cur_address + remaining_size < cur_info.GetEndAddress()) {
                    remaining_pages = 0;
                    cur_address += remaining_size;
                } else {
                    remaining_pages = (cur_address + remaining_size - cur_info.GetEndAddress()) / PageSize;
                    cur_address     = cur_info.GetEndAddress();
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
            KMemoryInfo cur_info = it->GetMemoryInfo();

            /* If we need to, create a new block before and insert it. */
            if (cur_info.address != GetInteger(cur_address)) {
                KMemoryBlock *new_block = allocator->Allocate();

                it->Split(new_block, cur_address);
                it = this->memory_block_tree.insert(*new_block);
                it++;

                cur_info = it->GetMemoryInfo();
                cur_address = cur_info.GetAddress();
            }

            if (cur_info.GetSize() > remaining_size) {
                /* If we need to, create a new block after and insert it. */
                KMemoryBlock *new_block = allocator->Allocate();

                it->Split(new_block, cur_address + remaining_size);
                it = this->memory_block_tree.insert(*new_block);

                cur_info = it->GetMemoryInfo();
            }

            /* Call the locked update function. */
            (std::addressof(*it)->*lock_func)(perm, cur_info.GetAddress() == address, cur_info.GetEndAddress() == end_address);
            cur_address += cur_info.GetSize();
            remaining_pages -= cur_info.GetNumPages();
            it++;
        }

        this->CoalesceForUpdate(allocator, address, num_pages);
    }

    /* Debug. */
    bool KMemoryBlockManager::CheckState() const {
        /* If we fail, we should dump blocks. */
        auto dump_guard = SCOPE_GUARD { this->DumpBlocks(); };

        /* Loop over every block, ensuring that we are sorted and coalesced. */
        auto it   = this->memory_block_tree.cbegin();
        auto prev = it++;
        while (it != this->memory_block_tree.cend()) {
            const KMemoryInfo prev_info = prev->GetMemoryInfo();
            const KMemoryInfo cur_info  = it->GetMemoryInfo();

            /* Sequential blocks which can be merged should be merged. */
            if (prev->CanMergeWith(*it)) {
                return false;
            }

            /* Sequential blocks should be sequential. */
            if (prev_info.GetEndAddress() != cur_info.GetAddress()) {
                return false;
            }

            /* If the block is ipc locked, it must have a count. */
            if ((cur_info.attribute & KMemoryAttribute_IpcLocked) != 0 && cur_info.ipc_lock_count == 0) {
                return false;
            }

            /* If the block is device shared, it must have a count. */
            if ((cur_info.attribute & KMemoryAttribute_DeviceShared) != 0 && cur_info.device_use_count == 0) {
                return false;
            }

            /* Advance the iterator. */
            prev = it++;
        }

        /* Our loop will miss checking the last block, potentially, so check it. */
        if (prev != this->memory_block_tree.cend()) {
            const KMemoryInfo prev_info = prev->GetMemoryInfo();
            /* If the block is ipc locked, it must have a count. */
            if ((prev_info.attribute & KMemoryAttribute_IpcLocked) != 0 && prev_info.ipc_lock_count == 0) {
                return false;
            }

            /* If the block is device shared, it must have a count. */
            if ((prev_info.attribute & KMemoryAttribute_DeviceShared) != 0 && prev_info.device_use_count == 0) {
                return false;
            }
        }

        /* We're valid, so no need to print. */
        dump_guard.Cancel();
        return true;
    }

    void KMemoryBlockManager::DumpBlocks() const {
        /* Dump each block. */
        for (const auto &block : this->memory_block_tree) {
            DumpMemoryInfo(block.GetMemoryInfo());
        }
    }
}
