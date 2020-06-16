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
#include <stratosphere.hpp>

namespace ams::fssrv {

    MemoryResourceFromStandardAllocator::MemoryResourceFromStandardAllocator(mem::StandardAllocator *allocator) : allocator(allocator), mutex() {
        this->current_free_size = this->allocator->GetTotalFreeSize();
        this->ClearPeak();
    }

    void MemoryResourceFromStandardAllocator::ClearPeak() {
        std::scoped_lock lk(this->mutex);
        this->peak_free_size      = this->current_free_size;
        this->peak_allocated_size = 0;
    }

    void *MemoryResourceFromStandardAllocator::AllocateImpl(size_t size, size_t align) {
        std::scoped_lock lk(this->mutex);

        void *p = this->allocator->Allocate(size, align);

        if (p != nullptr) {
            this->current_free_size -= this->allocator->GetSizeOf(p);
            this->peak_free_size = std::min(this->peak_free_size, this->current_free_size);
        }

        this->peak_allocated_size = std::max(this->peak_allocated_size, size);

        return p;
    }

    void MemoryResourceFromStandardAllocator::DeallocateImpl(void *p, size_t size, size_t align) {
        std::scoped_lock lk(this->mutex);

        this->current_free_size += this->allocator->GetSizeOf(p);
        this->allocator->Free(p);
    }

}
