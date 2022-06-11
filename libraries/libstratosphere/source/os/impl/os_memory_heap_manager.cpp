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
#include <stratosphere.hpp>
#include "os_memory_heap_manager.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    Result MemoryHeapManager::SetHeapSize(size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(util::IsAligned(size, MemoryHeapUnitSize));

        /* Acquire locks. */
        std::scoped_lock lk1(util::GetReference(::ams::os::impl::GetCurrentThread()->cs_thread));
        std::scoped_lock lk2(m_cs);

        /* If we need to, expand the heap. */
        if (size > m_heap_size) {
            /* Set the new heap size. */
            uintptr_t address = 0;
            R_TRY(m_impl.SetHeapSize(std::addressof(address), size));
            R_UNLESS(address != 0, os::ResultOutOfMemory());

            /* Check that the new heap address is consistent. */
            if (m_heap_size == 0) {
                AMS_ASSERT(util::IsAligned(address, MemoryHeapUnitSize));
            } else {
                AMS_ASSERT(address == m_heap_address);
            }

            /* Set up the new heap address. */
            this->AddToFreeSpaceUnsafe(address + m_heap_size, size - m_heap_size);

            /* Set our heap address. */
            m_heap_address = address;
            m_heap_size    = size;
        } else if (size < m_heap_size) {
            /* We're shrinking the heap, so we need to remove memory blocks. */
            const uintptr_t end_address = m_heap_address + size;
            const size_t remove_size    = m_heap_size - size;

            /* Get the end of the heap. */
            auto it = m_free_memory_list.end();
            --it;
            R_UNLESS(it != m_free_memory_list.end(), os::ResultBusy());

            /* Check that the block can be decommitted. */
            R_UNLESS(it->GetAddress() <= end_address, os::ResultBusy());
            R_UNLESS(it->GetSize() >= remove_size,    os::ResultBusy());

            /* Adjust the last node. */
            if (const size_t node_size = it->GetSize() - remove_size; node_size == 0) {
                m_free_memory_list.erase(it);
                it->Clean();
            } else {
                it->SetSize(node_size);
            }

            /* Set the reduced heap size. */
            uintptr_t address = 0;
            R_ABORT_UNLESS(m_impl.SetHeapSize(std::addressof(address), size));

            /* Set our heap address. */
            m_heap_size = size;
            if (size == 0) {
                m_heap_address = 0;
            }
        }

        R_SUCCEED();
    }

    Result MemoryHeapManager::AllocateFromHeap(uintptr_t *out_address, size_t size) {
        /* Acquire locks. */
        std::scoped_lock lk1(util::GetReference(::ams::os::impl::GetCurrentThread()->cs_thread));
        std::scoped_lock lk2(m_cs);

        /* Find free space. */
        auto it = this->FindFreeSpaceUnsafe(size);
        R_UNLESS(it != m_free_memory_list.end(), os::ResultOutOfMemory());

        /* If necessary, split the memory block. */
        if (it->GetSize() > size) {
            this->SplitFreeMemoryNodeUnsafe(it, size);
        }

        /* Remove the block. */
        m_free_memory_list.erase(it);
        it->Clean();

        /* Increment the used heap size. */
        m_used_heap_size += size;

        /* Set the output address. */
        *out_address = it->GetAddress();

        R_SUCCEED();
    }

    void MemoryHeapManager::ReleaseToHeap(uintptr_t address, size_t size) {
        /* Acquire locks. */
        std::scoped_lock lk1(util::GetReference(::ams::os::impl::GetCurrentThread()->cs_thread));
        std::scoped_lock lk2(m_cs);

        /* Check pre-condition. */
        AMS_ABORT_UNLESS(this->IsRegionAllocatedMemoryUnsafe(address, size));

        /* Restore the permissions on the memory. */
        os::SetMemoryPermission(address, size, MemoryPermission_ReadWrite);
        os::SetMemoryAttribute(address, size, MemoryAttribute_Normal);

        /* Add the memory back to our free list. */
        this->AddToFreeSpaceUnsafe(address, size);

        /* Decrement the used heap size. */
        m_used_heap_size -= size;
    }

    MemoryHeapManager::FreeMemoryList::iterator MemoryHeapManager::FindFreeSpaceUnsafe(size_t size) {
        /* Find the best fit candidate. */
        auto best = m_free_memory_list.end();

        for (auto it = m_free_memory_list.begin(); it != m_free_memory_list.end(); ++it) {
            if (const size_t node_size = it->GetSize(); node_size >= size) {
                if (best == m_free_memory_list.end() || node_size < best->GetSize()) {
                    best = it;
                }
            }
        }

        return best;
    }

    MemoryHeapManager::FreeMemoryList::iterator MemoryHeapManager::ConcatenatePreviousFreeMemoryNodeUnsafe(FreeMemoryList::iterator node) {
        /* Get the previous node. */
        auto prev = node;
        --prev;

        /* If there's no previous, we're done. */
        if (prev == m_free_memory_list.end() || node == m_free_memory_list.end()) {
            return node;
        }

        /* Otherwise, if the previous isn't contiguous, we can't merge. */
        if (prev->GetAddress() + prev->GetSize() != node->GetAddress()) {
            return node;
        }

        /* Otherwise, increase the size of the previous node, and remove the current node. */
        prev->SetSize(prev->GetSize() + node->GetSize());
        m_free_memory_list.erase(node);
        node->Clean();

        return prev;
    }

    void MemoryHeapManager::SplitFreeMemoryNodeUnsafe(FreeMemoryList::iterator it, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(it->GetSize() > size);
        AMS_ASSERT(util::IsAligned(size, MemoryBlockUnitSize));

        /* Create new node. */
        auto *new_node = std::construct_at(reinterpret_cast<FreeMemoryNode *>(it->GetAddress() + size));
        new_node->SetSize(it->GetSize() - size);

        /* Set the old node's size. */
        it->SetSize(size);

        /* Insert the new node. */
        m_free_memory_list.insert(++it, *new_node);
    }

    void MemoryHeapManager::AddToFreeSpaceUnsafe(uintptr_t address, size_t size) {
        /* Create new node. */
        auto *new_node = std::construct_at(reinterpret_cast<FreeMemoryNode *>(address));
        new_node->SetSize(size);

        /* Find the appropriate place to insert the node. */
        auto it = m_free_memory_list.begin();
        for (/* ... */; it != m_free_memory_list.end(); ++it) {
            if (address < it->GetAddress()) {
                break;
            }
        }

        /* Insert the new node. */
        it = m_free_memory_list.insert(it, *new_node);

        /* Perform coalescing as relevant. */
        it = this->ConcatenatePreviousFreeMemoryNodeUnsafe(it);
        this->ConcatenatePreviousFreeMemoryNodeUnsafe(++it);
    }

    bool MemoryHeapManager::IsRegionAllocatedMemoryUnsafe(uintptr_t address, size_t size) {
        /* Look for a node containing the region. */
        for (auto it = m_free_memory_list.begin(); it != m_free_memory_list.end(); ++it) {
            const uintptr_t node_address = it->GetAddress();
            const size_t    node_size    = it->GetSize();

            if (node_address < address + size && address < node_address + node_size) {
                return false;
            }
        }

        return true;
    }

}
