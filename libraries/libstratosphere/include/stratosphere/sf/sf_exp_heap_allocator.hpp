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
#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/sf/sf_allocation_policies.hpp>

namespace ams::sf {

    struct ExpHeapAllocator {
        using Policy = StatefulAllocationPolicy<ExpHeapAllocator>;

        lmem::HeapHandle _handle;
        os::SdkMutexType _mutex;

        void Attach(lmem::HeapHandle h) {
            this->_handle = h;
            os::InitializeSdkMutex(std::addressof(this->_mutex));
        }

        void Detach() {
            this->_handle = {};
        }

        void *Allocate(size_t size) {
            os::LockSdkMutex(std::addressof(this->_mutex));
            auto ptr = lmem::AllocateFromExpHeap(this->_handle, size);
            os::UnlockSdkMutex(std::addressof(this->_mutex));
            return ptr;
        }

        void Deallocate(void *ptr, size_t size) {
            AMS_UNUSED(size);

            os::LockSdkMutex(std::addressof(this->_mutex));
            lmem::FreeToExpHeap(this->_handle, ptr);
            os::UnlockSdkMutex(std::addressof(this->_mutex));
        }
    };
    static_assert(util::is_pod<ExpHeapAllocator>::value);

    template<size_t Size, typename Tag = void>
    struct ExpHeapStaticAllocator {
        using Policy = StatelessAllocationPolicy<ExpHeapStaticAllocator<Size, Tag>>;

        struct Globals {
            ExpHeapAllocator allocator;
            typename std::aligned_storage<Size == 0 ? 1 : Size>::type buffer;
        };

        static constinit inline Globals _globals;


        static void Initialize(int option) {
            _globals.allocator.Attach(lmem::CreateExpHeap(std::addressof(_globals.buffer), Size, option));
        }

        static void Initialize(lmem::HeapHandle handle) {
            _globals.allocator.Attach(handle);
        }

        static void *Allocate(size_t size) {
            return _globals.allocator.Allocate(size);
        }

        static void Deallocate(void *ptr, size_t size) {
            return _globals.allocator.Deallocate(ptr, size);
        }
    };

}
