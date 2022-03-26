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

        constinit bool g_used_default_allocator = false;

        void *DefaultAllocate(size_t size) {
            g_used_default_allocator = true;
            return ams::Malloc(size);
        }

        void DefaultDeallocate(void *ptr, size_t size) {
            AMS_UNUSED(size);
            ams::Free(ptr);
        }

        constinit os::SdkMutex g_mutex;
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
            R_SUCCEED();
        }

    }

    void SetAllocator(AllocateFunction allocator, DeallocateFunction deallocator) {
        R_ABORT_UNLESS(SetAllocatorImpl(allocator, deallocator));
    }

    namespace impl {

        void LockAllocatorMutex() {
            g_mutex.Lock();
        }

        void UnlockAllocatorMutex() {
            g_mutex.Unlock();
        }

        void *AllocateUnsafe(size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(g_mutex.IsLockedByCurrentThread());

            /* Allocate. */
            void * const ptr = g_allocate_func(size);

            /* Check alignment. */
            if (AMS_UNLIKELY(!util::IsAligned(reinterpret_cast<uintptr_t>(ptr), RequiredAlignment))) {
                R_ABORT_UNLESS(fs::ResultAllocatorAlignmentViolation());
            }

            /* Return allocated pointer. */
            return ptr;
        }

        void DeallocateUnsafe(void *ptr, size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(g_mutex.IsLockedByCurrentThread());

            /* Deallocate the pointer. */
            g_deallocate_func(ptr, size);
        }

        void *Allocate(size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(g_allocate_func != nullptr);

            /* Lock the allocator. */
            std::scoped_lock lk(g_mutex);

            return AllocateUnsafe(size);
        }

        void Deallocate(void *ptr, size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(g_deallocate_func != nullptr);

            /* If the pointer is non-null, deallocate it. */
            if (ptr != nullptr) {
                /* Lock the allocator. */
                std::scoped_lock lk(g_mutex);

                DeallocateUnsafe(ptr, size);
            }
        }

    }

}