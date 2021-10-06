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
#include <stratosphere.hpp>

namespace ams::fs {

    namespace {

        bool g_used_default_allocator;

        void *DefaultAllocate(size_t size) {
            g_used_default_allocator = true;
            return ams::Malloc(size);
        }

        void DefaultDeallocate(void *ptr, size_t size) {
            AMS_UNUSED(size);
            ams::Free(ptr);
        }

        constinit os::SdkMutex g_lock;
        constinit AllocateFunction g_allocate_func     = DefaultAllocate;
        constinit DeallocateFunction g_deallocate_func = DefaultDeallocate;

        constexpr size_t RequiredAlignment = alignof(u64);

        Result SetAllocatorImpl(AllocateFunction allocator, DeallocateFunction deallocator) {
            /* Ensure SetAllocator is used correctly. */
            R_UNLESS(g_allocate_func   == DefaultAllocate,   fs::ResultAllocatorAlreadyRegistered());
            R_UNLESS(g_deallocate_func == DefaultDeallocate, fs::ResultAllocatorAlreadyRegistered());
            R_UNLESS(allocator != nullptr,                   fs::ResultNullptrArgument());
            R_UNLESS(deallocator != nullptr,                 fs::ResultNullptrArgument());
            R_UNLESS(!g_used_default_allocator,              fs::ResultDefaultAllocatorUsed());

            /* Set allocators. */
            g_allocate_func   = allocator;
            g_deallocate_func = deallocator;
            return ResultSuccess();
        }

    }

    void SetAllocator(AllocateFunction allocator, DeallocateFunction deallocator) {
        R_ABORT_UNLESS(SetAllocatorImpl(allocator, deallocator));
    }

    namespace impl {

        void *Allocate(size_t size) {
            void *ptr;
            {
                std::scoped_lock lk(g_lock);
                ptr = g_allocate_func(size);
                if (!util::IsAligned(reinterpret_cast<uintptr_t>(ptr), RequiredAlignment)) {
                    R_ABORT_UNLESS(fs::ResultAllocatorAlignmentViolation());
                }
            }
            return ptr;
        }

        void Deallocate(void *ptr, size_t size) {
            if (ptr == nullptr) {
                return;
            }
            std::scoped_lock lk(g_lock);
            g_deallocate_func(ptr, size);
        }

    }

}