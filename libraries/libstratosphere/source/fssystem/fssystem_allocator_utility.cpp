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

namespace ams::fssystem {

    namespace {

        constexpr bool UseDefaultAllocators = false;

        bool g_used_default_allocator;

        void *DefaultAllocate(size_t size) {
            g_used_default_allocator = true;
            return std::malloc(size);
        }

        void DefaultDeallocate(void *ptr, size_t size) {
            std::free(ptr);
        }

        AllocateFunction g_allocate_func     = UseDefaultAllocators ? DefaultAllocate   : nullptr;
        DeallocateFunction g_deallocate_func = UseDefaultAllocators ? DefaultDeallocate : nullptr;

    }

    void *Allocate(size_t size) {
        AMS_ASSERT(g_allocate_func != nullptr);
        return g_allocate_func(size);
    }

    void Deallocate(void *ptr, size_t size) {
        AMS_ASSERT(g_deallocate_func != nullptr);
        return g_deallocate_func(ptr, size);
    }

    void InitializeAllocator(AllocateFunction allocate_func, DeallocateFunction deallocate_func) {
        AMS_ASSERT(allocate_func != nullptr);
        AMS_ASSERT(deallocate_func != nullptr);

        if constexpr (UseDefaultAllocators) {
            AMS_ASSERT(g_used_default_allocator == false);
        } else {
            AMS_ASSERT(g_allocate_func == nullptr);
            AMS_ASSERT(g_deallocate_func == nullptr);
        }

        g_allocate_func   = allocate_func;
        g_deallocate_func = deallocate_func;
    }

}
