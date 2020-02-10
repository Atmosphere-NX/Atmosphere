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

    Result KMemoryBlockManager::Initialize(KProcessAddress st, KProcessAddress nd, KMemoryBlockSlabManager *slab_manager) {
        /* Allocate a block to encapsulate the address space, insert it into the tree. */
        KMemoryBlock *start_block = slab_manager->Allocate();
        R_UNLESS(start_block != nullptr, svc::ResultOutOfResource());

        /* Set our start and end. */
        this->start = st;
        this->end   = nd;
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(this->start), PageSize));
        MESOSPHERE_ASSERT(util::IsAligned(GetInteger(this->end), PageSize));

        /* Initialize and insert the block. */
        start_block->Initialize(this->start, (this->end - this->start) / PageSize, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None);
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

    void KMemoryBlockManager::Update(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr) {
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
                it->Update(state, perm, attr);
                cur_address += cur_info.GetSize();
                remaining_pages -= cur_info.GetNumPages();
            }
        }

        /* Find the iterator now that we've updated. */
        it = this->FindIterator(address);
        if (address != this->start) {
            it--;
        }

        /* Coalesce blocks that we can. */
        while (true) {
            iterator prev = it++;
            if (it == this->memory_block_tree.end()) {
                break;
            }

            if (prev->HasSameProperties(*it)) {
                KMemoryBlock *block = std::addressof(*it);
                const size_t pages = it->GetNumPages();
                this->memory_block_tree.erase(it);
                allocator->Free(block);
                prev->Add(pages);
                it = prev;
            }

            if (address + num_pages * PageSize < it->GetMemoryInfo().GetEndAddress()) {
                break;
            }
        }
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

            /* Sequential blocks with same properties should be coalesced. */
            if (prev->HasSameProperties(*it)) {
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
        MESOSPHERE_TODO("Dump useful debugging information");
    }
}
