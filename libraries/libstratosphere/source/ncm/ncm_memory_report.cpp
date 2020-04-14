/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
        std::scoped_lock lk(this->mutex);
        this->heap_handle = heap_handle;
    }

    void HeapState::Allocate(size_t size) {
        std::scoped_lock lk(this->mutex);
        this->total_alloc_size += size;
        this->peak_total_alloc_size = std::max(this->total_alloc_size, this->peak_total_alloc_size);
        this->peak_alloc_size = std::max(size, this->peak_alloc_size);
    }

    void HeapState::Free(size_t size) {
        std::scoped_lock lk(this->mutex);
        this->total_alloc_size -= size;
    }

    void HeapState::GetMemoryResourceState(MemoryResourceState *out) {
        *out = {};
        std::scoped_lock lk(this->mutex);
        out->peak_total_alloc_size = this->peak_total_alloc_size;
        out->peak_alloc_size = this->peak_alloc_size;
        out->total_free_size = lmem::GetExpHeapTotalFreeSize(this->heap_handle);
        out->allocatable_size = lmem::GetExpHeapAllocatableSize(this->heap_handle, alignof(s32));
    }

    HeapState &GetHeapState() {
        static HeapState s_heap_state = {};
        return s_heap_state;
    }

}
