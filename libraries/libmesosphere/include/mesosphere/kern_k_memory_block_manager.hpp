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
            static constexpr size_t NumBlocks = 2;
        private:
            KMemoryBlock *blocks[NumBlocks];
            size_t index;
            KMemoryBlockSlabManager *slab_manager;
            Result result;
        public:
            explicit KMemoryBlockManagerUpdateAllocator(KMemoryBlockSlabManager *sm) : blocks(), index(), slab_manager(sm), result(svc::ResultOutOfResource()) {
                for (size_t i = 0; i < NumBlocks; i++) {
                    this->blocks[i] = this->slab_manager->Allocate();
                    if (this->blocks[i] == nullptr) {
                        this->result = svc::ResultOutOfResource();
                        return;
                    }
                }

                this->result = ResultSuccess();
            }

            ~KMemoryBlockManagerUpdateAllocator() {
                for (size_t i = 0; i < NumBlocks; i++) {
                    if (this->blocks[i] != nullptr) {
                        this->slab_manager->Free(this->blocks[i]);
                    }
                }
            }

            Result GetResult() const {
                return this->result;
            }

            KMemoryBlock *Allocate() {
                MESOSPHERE_ABORT_UNLESS(this->index < NumBlocks);
                MESOSPHERE_ABORT_UNLESS(this->blocks[this->index] != nullptr);
                KMemoryBlock *block = nullptr;
                std::swap(block, this->blocks[this->index++]);
                return block;
            }

            void Free(KMemoryBlock *block) {
                MESOSPHERE_ABORT_UNLESS(this->index <= NumBlocks);
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
            using iterator = MemoryBlockTree::iterator;
            using const_iterator = MemoryBlockTree::const_iterator;
        private:
            MemoryBlockTree memory_block_tree;
            KProcessAddress start_address;
            KProcessAddress end_address;
        public:
            constexpr KMemoryBlockManager() : memory_block_tree(), start_address(), end_address() { /* ... */ }

            iterator end() { return this->memory_block_tree.end(); }
            const_iterator end() const { return this->memory_block_tree.end(); }
            const_iterator cend() const { return this->memory_block_tree.cend(); }

            Result Initialize(KProcessAddress st, KProcessAddress nd, KMemoryBlockSlabManager *slab_manager);
            void   Finalize(KMemoryBlockSlabManager *slab_manager);

            KProcessAddress FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const;

            void Update(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr);
            void UpdateLock(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, void (KMemoryBlock::*lock_func)(KMemoryPermission new_perm), KMemoryPermission perm);

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
