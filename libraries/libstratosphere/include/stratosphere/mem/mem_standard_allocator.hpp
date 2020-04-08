/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include <stratosphere/os.hpp>
#include <stratosphere/mem/impl/mem_impl_declarations.hpp>

namespace ams::mem {

    class StandardAllocator {
        NON_COPYABLE(StandardAllocator);
        NON_MOVEABLE(StandardAllocator);
        public:
            using WalkCallback = int (*)(void *ptr, size_t size, void *user_data);

            struct AllocatorHash {
                size_t allocated_count;
                size_t allocated_size;
                size_t hash;
            };
        private:
            bool initialized;
            bool enable_thread_cache;
            uintptr_t unused;
            os::TlsSlot tls_slot;
            impl::InternalCentralHeapStorage central_heap_storage;
        public:
            StandardAllocator();
            StandardAllocator(void *mem, size_t size);
            StandardAllocator(void *mem, size_t size, bool enable_cache);

            ~StandardAllocator() {
                if (this->initialized) {
                    this->Finalize();
                }
            }

            void Initialize(void *mem, size_t size);
            void Initialize(void *mem, size_t size, bool enable_cache);
            void Finalize();

            void *Allocate(size_t size);
            void *Allocate(size_t size, size_t alignment);
            void Free(void *ptr);
            void *Reallocate(void *ptr, size_t new_size);
            size_t Shrink(void *ptr, size_t new_size);

            void ClearThreadCache() const;
            void CleanUpManagementArea() const;

            size_t GetSizeOf(const void *ptr) const;
            size_t GetTotalFreeSize() const;
            size_t GetAllocatableSize() const;

            void WalkAllocatedBlocks(WalkCallback callback, void *user_data) const;

            void Dump() const;
            AllocatorHash Hash() const;
    };

}
