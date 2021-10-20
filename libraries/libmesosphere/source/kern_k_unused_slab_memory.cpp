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

        class KUnusedSlabMemory : public util::IntrusiveRedBlackTreeBaseNode<KUnusedSlabMemory> {
            NON_COPYABLE(KUnusedSlabMemory);
            NON_MOVEABLE(KUnusedSlabMemory);
            private:
                size_t m_size;
            public:
                struct RedBlackKeyType {
                    size_t m_size;

                    constexpr ALWAYS_INLINE size_t GetSize() const {
                        return m_size;
                    }
                };

                template<typename T> requires (std::same_as<T, KUnusedSlabMemory> || std::same_as<T, RedBlackKeyType>)
                static constexpr ALWAYS_INLINE int Compare(const T &lhs, const KUnusedSlabMemory &rhs) {
                    if (lhs.GetSize() < rhs.GetSize()) {
                        return -1;
                    } else {
                        return 1;
                    }
                }
            public:
                KUnusedSlabMemory(size_t size) : m_size(size) { /* ... */ }

                constexpr ALWAYS_INLINE KVirtualAddress GetAddress() const { return reinterpret_cast<uintptr_t>(this); }
                constexpr ALWAYS_INLINE size_t GetSize() const { return m_size; }

        };
        static_assert(std::is_trivially_destructible<KUnusedSlabMemory>::value);

        using KUnusedSlabMemoryTree = util::IntrusiveRedBlackTreeBaseTraits<KUnusedSlabMemory>::TreeType<KUnusedSlabMemory>;

        constinit KLightLock g_unused_slab_memory_lock;
        constinit KUnusedSlabMemoryTree g_unused_slab_memory_tree;

    }

    KVirtualAddress AllocateUnusedSlabMemory(size_t size, size_t alignment) {
        /* Acquire exclusive access to the memory tree. */
        KScopedLightLock lk(g_unused_slab_memory_lock);

        /* Adjust size and alignment. */
        size      = std::max(size, sizeof(KUnusedSlabMemory));
        alignment = std::max(alignment, alignof(KUnusedSlabMemory));

        /* Find the smallest block which fits our allocation. */
        KUnusedSlabMemory *best_fit = std::addressof(*g_unused_slab_memory_tree.nfind_key({ size - 1 }));

        /* Ensure that the chunk is valid. */
        size_t prefix_waste;
        KVirtualAddress alloc_start;
        KVirtualAddress alloc_last;
        KVirtualAddress alloc_end;
        KVirtualAddress chunk_last;
        KVirtualAddress chunk_end;
        while (true) {
            /* Check that we still have a chunk satisfying our size requirement. */
            if (AMS_UNLIKELY(best_fit == nullptr)) {
                return Null<KVirtualAddress>;
            }

            /* Determine where the actual allocation would start. */
            alloc_start  = util::AlignUp(GetInteger(best_fit->GetAddress()), alignment);
            if (AMS_LIKELY(alloc_start >= best_fit->GetAddress())) {
                prefix_waste = alloc_start - best_fit->GetAddress();
                alloc_end    = alloc_start + size;
                alloc_last   = alloc_end - 1;

                /* Check that the allocation remains in bounds. */
                if (alloc_start <= alloc_last) {
                    chunk_end  = best_fit->GetAddress() + best_fit->GetSize();
                    chunk_last = chunk_end - 1;
                    if (AMS_LIKELY(alloc_last <= chunk_last)) {
                        break;
                    }
                }
            }

            /* Check the next smallest block. */
            best_fit = best_fit->GetNext();
        }

        /* Remove the chunk we selected from the tree. */
        g_unused_slab_memory_tree.erase(g_unused_slab_memory_tree.iterator_to(*best_fit));
        std::destroy_at(best_fit);

        /* If there's enough prefix waste due to alignment for a new chunk, insert it into the tree. */
        if (prefix_waste >= sizeof(KUnusedSlabMemory)) {
            std::construct_at(best_fit, prefix_waste);
            g_unused_slab_memory_tree.insert(*best_fit);
        }

        /* If there's enough suffix waste after the allocation for a new chunk, insert it into the tree. */
        if (alloc_last < alloc_end + sizeof(KUnusedSlabMemory) - 1 && alloc_end + sizeof(KUnusedSlabMemory) - 1 <= chunk_last) {
            KUnusedSlabMemory *suffix_chunk = GetPointer<KUnusedSlabMemory>(alloc_end);
            std::construct_at(suffix_chunk, chunk_end - alloc_end);
            g_unused_slab_memory_tree.insert(*suffix_chunk);
        }

        /* Return the allocated memory. */
        return alloc_start;
    }

    void FreeUnusedSlabMemory(KVirtualAddress address, size_t size) {
        /* NOTE: This is called only during initialization, so we don't need exclusive access. */
        /*       Nintendo doesn't acquire the lock here, either. */

        /* Check that there's anything at all for us to free. */
        if (AMS_UNLIKELY(size == 0)) {
            return;
        }

        /* Determine the start of the block. */
        const KVirtualAddress block_start = util::AlignUp(GetInteger(address), alignof(KUnusedSlabMemory));

        /* Check that there's space for a KUnusedSlabMemory to exist. */
        if (AMS_UNLIKELY(std::numeric_limits<uintptr_t>::max() - sizeof(KUnusedSlabMemory) < GetInteger(block_start))) {
            return;
        }

        /* Determine the end of the block region. */
        const KVirtualAddress block_end = util::AlignDown(GetInteger(address) + size, alignof(KUnusedSlabMemory));

        /* Check that the block remains within bounds. */
        if (AMS_UNLIKELY(block_start + sizeof(KUnusedSlabMemory) - 1 > block_end - 1)){
            return;
        }

        /* Create the block. */
        KUnusedSlabMemory *block = GetPointer<KUnusedSlabMemory>(block_start);
        std::construct_at(block, GetInteger(block_end) - GetInteger(block_start));

        /* Insert the block into the tree. */
        g_unused_slab_memory_tree.insert(*block);
    }

}
