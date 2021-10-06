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
#include <vapours.hpp>

namespace ams::sf {

    namespace impl {

        struct StatelessDummyAllocator;

    }

    template<typename A>
    class StatelessAllocationPolicy {
        public:
            static constexpr bool HasStatefulAllocator = false;
            using Allocator = impl::StatelessDummyAllocator;

            template<typename>
            using StatelessAllocator = A;

            template<typename>
            static void *AllocateAligned(size_t size, size_t align) {
                AMS_UNUSED(align);
                return A().Allocate(size);
            }

            template<typename>
            static void DeallocateAligned(void *ptr, size_t size, size_t align) {
                AMS_UNUSED(align);
                A().Deallocate(ptr, size);
            }
    };

    template<template<typename> typename A>
    class StatelessTypedAllocationPolicy {
        public:
            static constexpr bool HasStatefulAllocator = false;
            using Allocator = impl::StatelessDummyAllocator;

            template<typename T>
            using StatelessAllocator = A<T>;

            template<typename T>
            static void *AllocateAligned(size_t size, size_t align) {
                AMS_UNUSED(align);
                return StatelessAllocator<T>().Allocate(size);
            }

            template<typename T>
            static void DeallocateAligned(void *ptr, size_t size, size_t align) {
                AMS_UNUSED(align);
                StatelessAllocator<T>().Deallocate(ptr, size);
            }
    };

    template<typename A>
    class StatefulAllocationPolicy {
        public:
            static constexpr bool HasStatefulAllocator = true;
            using Allocator = A;

            static void *AllocateAligned(Allocator *allocator, size_t size, size_t align) {
                AMS_UNUSED(align);
                return allocator->Allocate(size);
            }

            static void DeallocateAligned(Allocator *allocator, void *ptr, size_t size, size_t align) {
                AMS_UNUSED(align);
                allocator->Deallocate(ptr, size);
            }
    };

    template<typename T>
    concept IsStatefulPolicy = T::HasStatefulAllocator;

}
