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

    template<template<typename> class StdAllocator>
    class StdAllocationPolicy {
        public:
            static constexpr bool HasStatefulAllocator = false;
            using Allocator = impl::StatelessDummyAllocator;

            template<typename T>
            struct StatelessAllocator {
                static void *Allocate(size_t size) {
                    return StdAllocator<T>().allocate(size / sizeof(T));
                }
                static void Deallocate(void *ptr, size_t size) {
                    StdAllocator<T>().deallocate(static_cast<T *>(ptr), size / sizeof(T));
                }
            };

            template<typename T>
            static void *AllocateAligned(size_t size, size_t align) {
                AMS_UNUSED(align);
                return StdAllocator<T>().allocate(size / sizeof(T));
            }

            template<typename T>
            static void DeallocateAligned(void *ptr, size_t size, size_t align) {
                AMS_UNUSED(align);
                StdAllocator<T>().deallocate(static_cast<T *>(ptr), size / sizeof(T));
            }
    };

}