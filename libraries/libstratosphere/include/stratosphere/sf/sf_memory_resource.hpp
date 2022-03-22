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

    struct MemoryResourceAllocationPolicy {
        static constexpr bool HasStatefulAllocator = true;
        using Allocator = MemoryResource;

        static void *AllocateAligned(MemoryResource *mr, size_t size, size_t align) {
            return mr->allocate(size, align);
        }

        static void DeallocateAligned(MemoryResource *mr, void *ptr, size_t size, size_t align) {
            return mr->deallocate(ptr, size, align);
        }
    };

    template<typename T>
    struct MemoryResourceStaticAllocator {
        static constinit inline MemoryResource *g_memory_resource = nullptr;

        static constexpr void Initialize(MemoryResource *mr) {
            g_memory_resource = mr;
        }

        struct Policy {
            static constexpr bool HasStatefulAllocator = false;
            using Allocator = MemoryResource;

            template<typename>
            using StatelessAllocator = Allocator;

            template<typename>
            static void *AllocateAligned(size_t size, size_t align) {
                return g_memory_resource->allocate(size, align);
            }

            template<typename>
            static void DeallocateAligned(void *ptr, size_t size, size_t align) {
                g_memory_resource->deallocate(ptr, size, align);
            }
        };
    };

}