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

namespace ams::fssystem {

    namespace {

        constexpr bool UseDefaultAllocators = false;

        constinit bool g_used_default_allocator = false;

        void *DefaultAllocate(size_t size) {
            g_used_default_allocator = true;
            return ams::Malloc(size);
        }

        void DefaultDeallocate(void *ptr, size_t size) {
            AMS_UNUSED(size);
            ams::Free(ptr);
        }

        constinit os::SdkMutex g_allocate_mutex;
        constinit os::SdkMutex g_allocate_mutex_for_system;

        constinit AllocateFunction g_allocate_func     = UseDefaultAllocators ? DefaultAllocate   : nullptr;
        constinit DeallocateFunction g_deallocate_func = UseDefaultAllocators ? DefaultDeallocate : nullptr;

        constinit AllocateFunction g_allocate_func_for_system     = nullptr;
        constinit DeallocateFunction g_deallocate_func_for_system = nullptr;

        void *AllocateUnsafe(size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(g_allocate_mutex.IsLockedByCurrentThread());
            AMS_ASSERT(g_allocate_func != nullptr);

            /* Allocate memory. */
            return g_allocate_func(size);
        }

        void DeallocateUnsafe(void *ptr, size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(g_allocate_mutex.IsLockedByCurrentThread());
            AMS_ASSERT(g_deallocate_func != nullptr);

            /* Deallocate the pointer. */
            g_deallocate_func(ptr, size);
        }


    }

    namespace impl {

        /* Normal allocator set. */
        template<> void *AllocatorFunctionSet<false>::Allocate(size_t size) { return ::ams::fssystem::Allocate(size); }
        template<> void *AllocatorFunctionSet<false>::AllocateUnsafe(size_t size)  { return ::ams::fssystem::AllocateUnsafe(size); }

        template<> void AllocatorFunctionSet<false>::Deallocate(void *ptr, size_t size) { return ::ams::fssystem::Deallocate(ptr, size); }
        template<> void AllocatorFunctionSet<false>::DeallocateUnsafe(void *ptr, size_t size) { return ::ams::fssystem::DeallocateUnsafe(ptr, size); }

        template<> void AllocatorFunctionSet<false>::LockAllocatorMutex() { ::ams::fssystem::g_allocate_mutex.Lock(); }
        template<> void AllocatorFunctionSet<false>::UnlockAllocatorMutex() { ::ams::fssystem::g_allocate_mutex.Unlock(); }

        /* System allocator set. */
        template<> void *AllocatorFunctionSet<true>::AllocateUnsafe(size_t size)  {
            /* Check pre-conditions. */
            AMS_ASSERT(::ams::fssystem::g_allocate_mutex_for_system.IsLockedByCurrentThread());
            AMS_ASSERT(::ams::fssystem::g_allocate_func_for_system != nullptr);

            /* Allocate memory. */
            return g_allocate_func_for_system(size);
        }

        template<> void AllocatorFunctionSet<true>::DeallocateUnsafe(void *ptr, size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(::ams::fssystem::g_allocate_mutex_for_system.IsLockedByCurrentThread());
            AMS_ASSERT(::ams::fssystem::g_deallocate_func_for_system != nullptr);

            /* Deallocate the pointer. */
            ::ams::fssystem::g_deallocate_func_for_system(ptr, size);
        }

        template<> void *AllocatorFunctionSet<true>::Allocate(size_t size) {
            std::scoped_lock lk(::ams::fssystem::g_allocate_mutex_for_system);
            return ::ams::fssystem::impl::AllocatorFunctionSet<true>::AllocateUnsafe(size);
        }

        template<> void AllocatorFunctionSet<true>::Deallocate(void *ptr, size_t size) {
            std::scoped_lock lk(::ams::fssystem::g_allocate_mutex_for_system);
            return ::ams::fssystem::impl::AllocatorFunctionSet<true>::DeallocateUnsafe(ptr, size);
        }

        template<> void AllocatorFunctionSet<true>::LockAllocatorMutex() { ::ams::fssystem::g_allocate_mutex_for_system.Lock(); }
        template<> void AllocatorFunctionSet<true>::UnlockAllocatorMutex() { ::ams::fssystem::g_allocate_mutex_for_system.Unlock(); }

    }

    void *Allocate(size_t size) {
        std::scoped_lock lk(g_allocate_mutex);
        return AllocateUnsafe(size);
    }

    void Deallocate(void *ptr, size_t size) {
        std::scoped_lock lk(g_allocate_mutex);
        return DeallocateUnsafe(ptr, size);
    }

    void InitializeAllocator(AllocateFunction allocate_func, DeallocateFunction deallocate_func) {
        /* Check pre-conditions. */
        AMS_ASSERT(allocate_func != nullptr);
        AMS_ASSERT(deallocate_func != nullptr);

        /* Check that we can initialize. */
        if constexpr (UseDefaultAllocators) {
            AMS_ASSERT(g_used_default_allocator == false);
        } else {
            AMS_ASSERT(g_allocate_func == nullptr);
            AMS_ASSERT(g_deallocate_func == nullptr);
        }

        /* Set the allocator functions. */
        g_allocate_func   = allocate_func;
        g_deallocate_func = deallocate_func;
    }

    void InitializeAllocatorForSystem(AllocateFunction allocate_func, DeallocateFunction deallocate_func) {
        /* Check pre-conditions. */
        AMS_ASSERT(allocate_func != nullptr);
        AMS_ASSERT(deallocate_func != nullptr);

        /* Check that we can initialize. */
        AMS_ASSERT(g_allocate_func_for_system == nullptr);
        AMS_ASSERT(g_deallocate_func_for_system == nullptr);

        /* Set the system allocator functions. */
        g_allocate_func_for_system   = allocate_func;
        g_deallocate_func_for_system = deallocate_func;
    }

}
