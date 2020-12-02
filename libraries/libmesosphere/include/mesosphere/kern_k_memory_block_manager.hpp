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
#include <mesosphere/kern_k_memory_block.hpp>
#include <mesosphere/kern_k_page_table_manager.hpp>

namespace ams::kern {

    class KMemoryBlockManagerUpdateAllocator {
        public:
            static constexpr size_t MaxBlocks = 2;
        private:
            KMemoryBlock *blocks[MaxBlocks];
            size_t index;
            KMemoryBlockSlabManager *slab_manager;
        public:
            constexpr explicit KMemoryBlockManagerUpdateAllocator(KMemoryBlockSlabManager *sm) : blocks(), index(MaxBlocks), slab_manager(sm) { /* ... */ }

            ~KMemoryBlockManagerUpdateAllocator() {
                for (const auto &block : this->blocks) {
                    if (block != nullptr) {
                        this->slab_manager->Free(block);
                    }
                }
            }

            Result Initialize(size_t num_blocks) {
                /* Check num blocks. */
                MESOSPHERE_ASSERT(num_blocks <= MaxBlocks);

                /* Set index. */
                this->index = MaxBlocks - num_blocks;

                /* Allocate the blocks. */
                for (size_t i = 0; i < num_blocks && i < MaxBlocks; ++i) {
                    this->blocks[this->index + i] = this->slab_manager->Allocate();
                    R_UNLESS(this->blocks[this->index + i] != nullptr, svc::ResultOutOfResource());
                }

                return ResultSuccess();
            }

            KMemoryBlock *Allocate() {
                MESOSPHERE_ABORT_UNLESS(this->index < MaxBlocks);
                MESOSPHERE_ABORT_UNLESS(this->blocks[this->index] != nullptr);
                KMemoryBlock *block = nullptr;
                std::swap(block, this->blocks[this->index++]);
                return block;
            }

            void Free(KMemoryBlock *block) {
                MESOSPHERE_ABORT_UNLESS(this->index <= MaxBlocks);
                MESOSPHERE_ABORT_UNLESS(block != nullptr);
                if (this->index == 0) {
                    this->slab_manager->Free(block);
                } else {
                    this->blocks[--this->index] = block;
                }
            }
    };

    class KMemoryBlockManager {
        public:
            using MemoryBlockTree = util::IntrusiveRedBlackTreeBaseTraits<KMemoryBlock>::TreeType<KMemoryBlock>;
            using MemoryBlockLockFunction = void (KMemoryBlock::*)(KMemoryPermission new_perm, bool left, bool right);
            using iterator = MemoryBlockTree::iterator;
            using const_iterator = MemoryBlockTree::const_iterator;
        private:
            MemoryBlockTree memory_block_tree;
            KProcessAddress start_address;
            KProcessAddress end_address;
        private:
            void CoalesceForUpdate(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages);
        public:
            constexpr KMemoryBlockManager() : memory_block_tree(), start_address(), end_address() { /* ... */ }

            iterator end() { return this->memory_block_tree.end(); }
            const_iterator end() const { return this->memory_block_tree.end(); }
            const_iterator cend() const { return this->memory_block_tree.cend(); }

            Result Initialize(KProcessAddress st, KProcessAddress nd, KMemoryBlockSlabManager *slab_manager);
            void   Finalize(KMemoryBlockSlabManager *slab_manager);

            KProcessAddress FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const;

            void Update(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr, KMemoryBlockDisableMergeAttribute set_disable_attr, KMemoryBlockDisableMergeAttribute clear_disable_attr);
            void UpdateLock(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, MemoryBlockLockFunction lock_func, KMemoryPermission perm);

            void UpdateIfMatch(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState test_state, KMemoryPermission test_perm, KMemoryAttribute test_attr, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr);

            iterator FindIterator(KProcessAddress address) const {
                return this->memory_block_tree.find(KMemoryBlock(address, 1, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None));
            }

            const KMemoryBlock *FindBlock(KProcessAddress address) const {
                if (const_iterator it = this->FindIterator(address); it != this->memory_block_tree.end()) {
                    return std::addressof(*it);
                }

                return nullptr;
            }

            /* Debug. */
            bool CheckState() const;
            void DumpBlocks() const;
    };

    class KScopedMemoryBlockManagerAuditor {
        private:
            KMemoryBlockManager *manager;
        public:
            explicit ALWAYS_INLINE KScopedMemoryBlockManagerAuditor(KMemoryBlockManager *m) : manager(m) { MESOSPHERE_AUDIT(this->manager->CheckState()); }
            explicit ALWAYS_INLINE KScopedMemoryBlockManagerAuditor(KMemoryBlockManager &m) : KScopedMemoryBlockManagerAuditor(std::addressof(m)) { /* ... */ }
            ALWAYS_INLINE ~KScopedMemoryBlockManagerAuditor() { MESOSPHERE_AUDIT(this->manager->CheckState()); }
    };

}
