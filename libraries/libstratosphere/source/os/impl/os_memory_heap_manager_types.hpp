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
#include <stratosphere.hpp>

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_memory_heap_manager_impl.os.horizon.hpp"
#elif defined(ATMOSPHERE_OS_WINDOWS)
    #include "os_memory_heap_manager_impl.os.windows.hpp"
#elif defined(ATMOSPHERE_OS_LINUX)
    #include "os_memory_heap_manager_impl.os.linux.hpp"
#elif defined(ATMOSPHERE_OS_MACOS)
    #include "os_memory_heap_manager_impl.os.macos.hpp"
#else
    #error "Unknown OS for MemoryHeapManagerImpl"
#endif

namespace ams::os::impl {

    class MemoryHeapManager;

    class FreeMemoryNode {
        private:
            friend class MemoryHeapManager;
        private:
            util::IntrusiveListNode m_node;
            size_t m_size;
        public:
            ALWAYS_INLINE uintptr_t GetAddress() const { return reinterpret_cast<uintptr_t>(this); }
            ALWAYS_INLINE size_t GetSize() const { return m_size; }
            ALWAYS_INLINE void SetSize(size_t size) { m_size = size; }
            ALWAYS_INLINE void Clean() { std::memset(reinterpret_cast<void *>(this), 0, sizeof(FreeMemoryNode)); }
    };
    static_assert(sizeof(FreeMemoryNode) == sizeof(util::IntrusiveListNode) + sizeof(size_t));

    class MemoryHeapManager {
        NON_COPYABLE(MemoryHeapManager);
        NON_MOVEABLE(MemoryHeapManager);
        private:
            using FreeMemoryList = typename util::IntrusiveListMemberTraits<&FreeMemoryNode::m_node>::ListType;
        private:
            uintptr_t m_heap_address;
            size_t m_heap_size;
            size_t m_used_heap_size;
            FreeMemoryList m_free_memory_list;
            InternalCriticalSection m_cs;
            MemoryHeapManagerImpl m_impl;
        public:
            MemoryHeapManager() : m_heap_address(0), m_heap_size(0), m_used_heap_size(0) { /* ... */ }

            Result SetHeapSize(size_t size);

            Result AllocateFromHeap(uintptr_t *out_address, size_t size);
            void ReleaseToHeap(uintptr_t address, size_t size);

            bool IsRegionInMemoryHeap(uintptr_t address, size_t size) const {
                return m_heap_address <= address && (address + size) <= (m_heap_address + m_heap_size);
            }

            uintptr_t GetHeapAddress() const { return m_heap_address; }
            size_t GetHeapSize() const { return m_heap_size; }
            size_t GetUsedHeapSize() const { return m_used_heap_size; }
        private:
            FreeMemoryList::iterator FindFreeSpaceUnsafe(size_t size);
            FreeMemoryList::iterator ConcatenatePreviousFreeMemoryNodeUnsafe(FreeMemoryList::iterator node);
            void SplitFreeMemoryNodeUnsafe(FreeMemoryList::iterator it, size_t size);
            void AddToFreeSpaceUnsafe(uintptr_t address, size_t size);
            bool IsRegionAllocatedMemoryUnsafe(uintptr_t address, size_t size);
    };

}
