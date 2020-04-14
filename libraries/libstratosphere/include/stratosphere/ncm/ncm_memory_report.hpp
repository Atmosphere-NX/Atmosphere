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
#pragma once
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/lmem/lmem_common.hpp>

namespace ams::ncm {

    struct MemoryResourceState {
        size_t peak_total_alloc_size;
        size_t peak_alloc_size;
        size_t allocatable_size;
        size_t total_free_size;
    };

    static_assert(sizeof(MemoryResourceState) == 0x20);

    struct MemoryReport {
        MemoryResourceState system_content_meta_resource_state;
        MemoryResourceState sd_and_user_content_meta_resource_state;
        MemoryResourceState gamecard_content_meta_resource_state;
        MemoryResourceState heap_resource_state;
    };

    static_assert(sizeof(MemoryReport) == 0x80);

    class HeapState {
        private:
            os::Mutex mutex;
            lmem::HeapHandle heap_handle;
            size_t total_alloc_size;
            size_t peak_total_alloc_size;
            size_t peak_alloc_size;
        public:
            constexpr HeapState() : mutex(false), heap_handle(nullptr), total_alloc_size(0), peak_total_alloc_size(0), peak_alloc_size(0) { /* ... */ }

            void Initialize(lmem::HeapHandle heap_handle);
            void Allocate(size_t size);
            void Free(size_t size);
            void GetMemoryResourceState(MemoryResourceState *out);
    };

    HeapState &GetHeapState();

}
