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

namespace ams::ncm {

    void HeapState::Initialize(lmem::HeapHandle heap_handle) {
        std::scoped_lock lk(m_mutex);
        m_heap_handle = heap_handle;
    }

    void HeapState::Allocate(size_t size) {
        std::scoped_lock lk(m_mutex);
        m_total_alloc_size += size;
        m_peak_total_alloc_size = std::max(m_total_alloc_size, m_peak_total_alloc_size);
        m_peak_alloc_size = std::max(size, m_peak_alloc_size);
    }

    void HeapState::Free(size_t size) {
        std::scoped_lock lk(m_mutex);
        m_total_alloc_size -= size;
    }

    void HeapState::GetMemoryResourceState(MemoryResourceState *out) {
        *out = {};
        std::scoped_lock lk(m_mutex);
        out->peak_total_alloc_size = m_peak_total_alloc_size;
        out->peak_alloc_size = m_peak_alloc_size;
        out->total_free_size = lmem::GetExpHeapTotalFreeSize(m_heap_handle);
        out->allocatable_size = lmem::GetExpHeapAllocatableSize(m_heap_handle, alignof(s32));
    }

    HeapState &GetHeapState() {
        AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(HeapState, s_heap_state);

        return s_heap_state;
    }

}
