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

    /* Debug. */
    bool KMemoryBlockManager::CheckState() const {
        /* If we fail, we should dump blocks. */
        auto dump_guard = SCOPE_GUARD { this->DumpBlocks(); };

        /* Loop over every block, ensuring that we are sorted and coalesced. */
        auto it   = this->memory_block_tree.cbegin();
        auto prev = it++;
        while (it != this->memory_block_tree.cend()) {
            MESOSPHERE_TODO("Validate KMemoryBlock Tree");

            /* Advance the iterator. */
            prev = it++;
        }

        /* We're valid, so no need to print. */
        dump_guard.Cancel();
        return true;
    }

    void KMemoryBlockManager::DumpBlocks() const {
        MESOSPHERE_TODO("Dump useful debugging information");
    }
}
