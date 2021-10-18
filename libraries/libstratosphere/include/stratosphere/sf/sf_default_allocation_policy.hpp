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

    namespace impl {

        void *DefaultAllocateImpl(size_t size, size_t align, size_t offset);
        void DefaultDeallocateImpl(void *ptr, size_t size, size_t align, size_t offset);

        template<typename T>
        class DefaultAllocationPolicyAllocator {
            private:
                struct Holder {
                    MemoryResource *allocator;
                    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
                };
            public:
                void *Allocate(size_t size) {
                    AMS_ASSERT(size == sizeof(T));
                    AMS_UNUSED(size);
                    return DefaultAllocateImpl(sizeof(Holder), alignof(Holder), AMS_OFFSETOF(Holder, storage));
                }

                void Deallocate(void *ptr, size_t size) {
                    AMS_ASSERT(size == sizeof(T));
                    AMS_UNUSED(size);
                    return DefaultDeallocateImpl(ptr, sizeof(Holder), alignof(Holder), AMS_OFFSETOF(Holder, storage));
                }
        };

    }

    using DefaultAllocationPolicy = StatelessTypedAllocationPolicy<impl::DefaultAllocationPolicyAllocator>;

    MemoryResource *GetGlobalDefaultMemoryResource();
    MemoryResource *GetCurrentEffectiveMemoryResource();
    MemoryResource *GetCurrentMemoryResource();
    MemoryResource *GetNewDeleteMemoryResource();

    MemoryResource *SetGlobalDefaultMemoryResource(MemoryResource *mr);
    MemoryResource *SetCurrentMemoryResource(MemoryResource *mr);

    class ScopedCurrentMemoryResourceSetter {
        NON_COPYABLE(ScopedCurrentMemoryResourceSetter);
        NON_MOVEABLE(ScopedCurrentMemoryResourceSetter);
        private:
            MemoryResource *m_prev;
        public:
            explicit ScopedCurrentMemoryResourceSetter(MemoryResource *mr);
            ~ScopedCurrentMemoryResourceSetter();
    };

}