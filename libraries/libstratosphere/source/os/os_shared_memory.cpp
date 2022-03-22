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
#include "impl/os_thread_manager.hpp"
#include "impl/os_shared_memory_impl.hpp"

namespace ams::os {

    namespace {

        void SetupSharedMemoryType(SharedMemoryType *shared_memory, size_t size, NativeHandle handle, bool managed) {
            /* Set members. */
            shared_memory->handle    = handle;
            shared_memory->size      = size;
            shared_memory->address   = nullptr;
            shared_memory->allocated = false;

            /* Set managed. */
            shared_memory->handle_managed = managed;

            /* Create the critical section. */
            util::ConstructAt(shared_memory->cs_shared_memory);
        }

    }

    Result CreateSharedMemory(SharedMemoryType *shared_memory, size_t size, MemoryPermission my_perm, MemoryPermission other_perm) {
        /* Check pre-conditions. */
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, MemoryPageSize));

        /* Create the memory. */
        NativeHandle handle;
        R_TRY(impl::SharedMemoryImpl::Create(std::addressof(handle), size, my_perm, other_perm));

        /* Setup the object. */
        SetupSharedMemoryType(shared_memory, size, handle, true);

        return ResultSuccess();
    }

    void AttachSharedMemory(SharedMemoryType *shared_memory, size_t size, NativeHandle handle, bool managed) {
        /* Check pre-conditions. */
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, MemoryPageSize));
        AMS_ASSERT(handle != os::InvalidNativeHandle);

        /* Setup the object. */
        SetupSharedMemoryType(shared_memory, size, handle, managed);
    }

    void DestroySharedMemory(SharedMemoryType *shared_memory) {
        /* Unmap the shared memory, if required. */
        if (shared_memory->state == SharedMemoryType::State_Mapped) {
            UnmapSharedMemory(shared_memory);
        }

        /* Check the state. */
        AMS_ASSERT(shared_memory->state == SharedMemoryType::State_Initialized);

        /* Set state to not initialized. */
        shared_memory->state = SharedMemoryType::State_NotInitialized;

        /* Close the handle, if it's managed. */
        if (shared_memory->handle_managed) {
            impl::SharedMemoryImpl::Close(shared_memory->handle);
        }
        shared_memory->handle_managed = false;

        /* Clear members. */
        shared_memory->address = nullptr;
        shared_memory->size    = 0;
        shared_memory->handle  = os::InvalidNativeHandle;

        /* Destroy the critical section. */
        util::DestroyAt(shared_memory->cs_shared_memory);
    }

    void *MapSharedMemory(SharedMemoryType *shared_memory, MemoryPermission perm) {
        /* Lock the current thread, and then the shared memory. */
        std::scoped_lock thread_lk(util::GetReference(impl::GetCurrentThread()->cs_thread));
        std::scoped_lock lk(util::GetReference(shared_memory->cs_shared_memory));

        /* Ensure we're in a mappable state. */
        AMS_ASSERT(shared_memory->state == SharedMemoryType::State_Initialized);

        /* Try to map. */
        void *mapped_address;
        if (R_FAILED(impl::SharedMemoryImpl::Map(std::addressof(mapped_address), shared_memory->handle, shared_memory->size, perm))) {
            return nullptr;
        }

        /* Set fields now that we've mapped successfully. */
        shared_memory->allocated = true;
        shared_memory->address   = mapped_address;
        shared_memory->state     = SharedMemoryType::State_Mapped;

        return mapped_address;
    }

    void UnmapSharedMemory(SharedMemoryType *shared_memory) {
        /* Lock the memory. */
        std::scoped_lock lk(util::GetReference(shared_memory->cs_shared_memory));

        /* If the memory isn't mapped, we can't unmap it. */
        if (shared_memory->state != SharedMemoryType::State_Mapped) {
            return;
        }

        /* Unmap the memory. */
        impl::SharedMemoryImpl::Unmap(shared_memory->handle, shared_memory->address, shared_memory->size);

        /* Unmapped memory is necessarily not allocated. */
        if (shared_memory->allocated) {
            shared_memory->allocated = false;
        }

        /* Clear the address. */
        shared_memory->address = nullptr;
        shared_memory->state   = SharedMemoryType::State_Initialized;
    }

    void *GetSharedMemoryAddress(const SharedMemoryType *shared_memory) {
        /* Check pre-conditions. */
        AMS_ASSERT(shared_memory->state == SharedMemoryType::State_Initialized || shared_memory->state == SharedMemoryType::State_Mapped);

        return shared_memory->address;
    }

    size_t GetSharedMemorySize(const SharedMemoryType *shared_memory) {
        /* Check pre-conditions. */
        AMS_ASSERT(shared_memory->state == SharedMemoryType::State_Initialized || shared_memory->state == SharedMemoryType::State_Mapped);

        return shared_memory->size;
    }

    NativeHandle GetSharedMemoryHandle(const SharedMemoryType *shared_memory) {
        /* Check pre-conditions. */
        AMS_ASSERT(shared_memory->state == SharedMemoryType::State_Initialized || shared_memory->state == SharedMemoryType::State_Mapped);

        return shared_memory->handle;
    }

}
