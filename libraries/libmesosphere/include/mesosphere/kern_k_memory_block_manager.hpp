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

    class KMemoryBlockManager {
        public:
            using MemoryBlockTree = util::IntrusiveRedBlackTreeBaseTraits<KMemoryBlock>::TreeType<KMemoryBlock>;
        private:
            MemoryBlockTree memory_block_tree;
            KProcessAddress start;
            KProcessAddress end;
        public:
            constexpr KMemoryBlockManager() : memory_block_tree(), start(), end() { /* ... */ }

            Result Initialize(KProcessAddress st, KProcessAddress nd, KMemoryBlockSlabManager *slab_manager);
            void   Finalize(KMemoryBlockSlabManager *slab_manager);

            /* Debug. */
            bool CheckState() const;
            void DumpBlocks() const;
    };

    class KScopedMemoryBlockManagerVerifier {
        private:
            KMemoryBlockManager *manager;
        public:
            explicit ALWAYS_INLINE KScopedMemoryBlockManagerVerifier(KMemoryBlockManager *m) : manager(m) { MESOSPHERE_AUDIT(this->manager->CheckState()); }
            explicit ALWAYS_INLINE KScopedMemoryBlockManagerVerifier(KMemoryBlockManager &m) : KScopedMemoryBlockManagerVerifier(std::addressof(m)) { /* ... */ }
            ALWAYS_INLINE ~KScopedMemoryBlockManagerVerifier() { MESOSPHERE_AUDIT(this->manager->CheckState()); }
    };

}
