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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_memory_block.hpp>
#include <mesosphere/kern_k_page_table_manager.hpp>

namespace ams::kern {

    class KMemoryBlockManagerUpdateAllocator {
        public:
            static constexpr size_t MaxBlocks = 2;
        private:
            KMemoryBlock *m_blocks[MaxBlocks];
            size_t m_index;
            KMemoryBlockSlabManager *m_slab_manager;
        private:
            ALWAYS_INLINE Result Initialize(size_t num_blocks) {
                /* Check num blocks. */
                MESOSPHERE_ASSERT(num_blocks <= MaxBlocks);

                /* Set index. */
                m_index = MaxBlocks - num_blocks;

                /* Allocate the blocks. */
                for (size_t i = 0; i < num_blocks && i < MaxBlocks; ++i) {
                    m_blocks[m_index + i] = m_slab_manager->Allocate();
                    R_UNLESS(m_blocks[m_index + i] != nullptr, svc::ResultOutOfResource());
                }

                R_SUCCEED();
            }
        public:
            KMemoryBlockManagerUpdateAllocator(Result *out_result, KMemoryBlockSlabManager *sm, size_t num_blocks = MaxBlocks) : m_blocks(), m_index(MaxBlocks), m_slab_manager(sm) {
                *out_result = this->Initialize(num_blocks);
            }

            ~KMemoryBlockManagerUpdateAllocator() {
                for (const auto &block : m_blocks) {
                    if (block != nullptr) {
                        m_slab_manager->Free(block);
                    }
                }
            }

            KMemoryBlock *Allocate() {
                MESOSPHERE_ABORT_UNLESS(m_index < MaxBlocks);
                MESOSPHERE_ABORT_UNLESS(m_blocks[m_index] != nullptr);
                KMemoryBlock *block = nullptr;
                std::swap(block, m_blocks[m_index++]);
                return block;
            }

            void Free(KMemoryBlock *block) {
                MESOSPHERE_ABORT_UNLESS(m_index <= MaxBlocks);
                MESOSPHERE_ABORT_UNLESS(block != nullptr);
                if (m_index == 0) {
                    m_slab_manager->Free(block);
                } else {
                    m_blocks[--m_index] = block;
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
            MemoryBlockTree m_memory_block_tree;
            KProcessAddress m_start_address;
            KProcessAddress m_end_address;
        private:
            void CoalesceForUpdate(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages);
        public:
            constexpr explicit KMemoryBlockManager(util::ConstantInitializeTag) : m_memory_block_tree(), m_start_address(Null<KProcessAddress>), m_end_address(Null<KProcessAddress>) { /* ... */ }

            explicit KMemoryBlockManager() { /* ... */ }

            iterator end() { return m_memory_block_tree.end(); }
            const_iterator end() const { return m_memory_block_tree.end(); }
            const_iterator cend() const { return m_memory_block_tree.cend(); }

            Result Initialize(KProcessAddress st, KProcessAddress nd, KMemoryBlockSlabManager *slab_manager);
            void   Finalize(KMemoryBlockSlabManager *slab_manager);

            static bool GetRegionForFindFreeArea(KProcessAddress *out_start, KProcessAddress *out_end, KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages);
            KProcessAddress FindFreeArea(KProcessAddress region_start, size_t region_num_pages, size_t num_pages, size_t alignment, size_t offset, size_t guard_pages) const;

            void Update(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr, KMemoryBlockDisableMergeAttribute set_disable_attr, KMemoryBlockDisableMergeAttribute clear_disable_attr);
            void UpdateLock(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, MemoryBlockLockFunction lock_func, KMemoryPermission perm);

            void UpdateIfMatch(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, KMemoryState test_state, KMemoryPermission test_perm, KMemoryAttribute test_attr, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr, KMemoryBlockDisableMergeAttribute set_disable_attr, KMemoryBlockDisableMergeAttribute clear_disable_attr);

            void UpdateAttribute(KMemoryBlockManagerUpdateAllocator *allocator, KProcessAddress address, size_t num_pages, u32 mask, u32 attr);

            iterator FindIterator(KProcessAddress address) const {
                return m_memory_block_tree.find(KMemoryBlock(util::ConstantInitialize, address, 1, KMemoryState_Free, KMemoryPermission_None, KMemoryAttribute_None));
            }

            const KMemoryBlock *FindBlock(KProcessAddress address) const {
                if (const_iterator it = this->FindIterator(address); it != m_memory_block_tree.end()) {
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
            KMemoryBlockManager *m_manager;
        public:
            explicit ALWAYS_INLINE KScopedMemoryBlockManagerAuditor(KMemoryBlockManager *m) : m_manager(m) { MESOSPHERE_AUDIT(m_manager->CheckState()); }
            explicit ALWAYS_INLINE KScopedMemoryBlockManagerAuditor(KMemoryBlockManager &m) : KScopedMemoryBlockManagerAuditor(std::addressof(m)) { /* ... */ }
            ALWAYS_INLINE ~KScopedMemoryBlockManagerAuditor() { MESOSPHERE_AUDIT(m_manager->CheckState()); }
    };

}
