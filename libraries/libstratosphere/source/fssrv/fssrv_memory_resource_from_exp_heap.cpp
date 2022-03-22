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

namespace ams::fssrv {

    namespace {

        size_t GetUsedSize(void *p) {
            const auto block_head = reinterpret_cast<const lmem::impl::ExpHeapMemoryBlockHead *>(reinterpret_cast<uintptr_t>(p) - sizeof(lmem::impl::ExpHeapMemoryBlockHead));
            return block_head->block_size + ((block_head->attributes >> 8) & 0x7F) + sizeof(lmem::impl::ExpHeapMemoryBlockHead);
        }

    }

    void PeakCheckableMemoryResourceFromExpHeap::OnAllocate(void *p, size_t size) {
        AMS_UNUSED(size);

        if (p != nullptr) {
            m_current_free_size = GetUsedSize(p);
            m_peak_free_size = std::min(m_peak_free_size, m_current_free_size);
        }
    }

    void PeakCheckableMemoryResourceFromExpHeap::OnDeallocate(void *p, size_t size) {
        AMS_UNUSED(size);

        if (p != nullptr) {
            m_current_free_size += GetUsedSize(p);
        }
    }

    void *PeakCheckableMemoryResourceFromExpHeap::AllocateImpl(size_t size, size_t align) {
        std::scoped_lock lk(m_mutex);

        void *p = lmem::AllocateFromExpHeap(m_heap_handle, size, static_cast<s32>(align));
        this->OnAllocate(p, size);
        return p;
    }

    void PeakCheckableMemoryResourceFromExpHeap::DeallocateImpl(void *p, size_t size, size_t align) {
        AMS_UNUSED(align);

        std::scoped_lock lk(m_mutex);

        this->OnDeallocate(p, size);
        lmem::FreeToExpHeap(m_heap_handle, p);
    }

}
