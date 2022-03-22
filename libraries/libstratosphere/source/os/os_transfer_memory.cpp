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
#include "impl/os_transfer_memory_impl.hpp"

namespace ams::os {

    namespace {

        inline void SetupTransferMemoryType(TransferMemoryType *tmem, size_t size, NativeHandle handle, bool managed) {
            /* Set members. */
            tmem->handle    = handle;
            tmem->size      = size;
            tmem->address   = nullptr;
            tmem->allocated = false;

            /* Set managed. */
            tmem->handle_managed = managed;

            /* Create the critical section. */
            util::ConstructAt(tmem->cs_transfer_memory);
        }

    }

    Result CreateTransferMemory(TransferMemoryType *tmem, void *address, size_t size, MemoryPermission perm) {
        /* Validate pre-conditions. */
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(address != nullptr);
        AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(address), os::MemoryPageSize));

        /* Create the memory. */
        NativeHandle handle;
        R_TRY(impl::TransferMemoryImpl::Create(std::addressof(handle), address, size, perm));

        /* Setup the object. */
        SetupTransferMemoryType(tmem, size, handle, true);

        return ResultSuccess();
    }

    void AttachTransferMemory(TransferMemoryType *tmem, size_t size, NativeHandle handle, bool managed) {
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(handle != svc::InvalidHandle);

        /* Setup the object. */
        SetupTransferMemoryType(tmem, size, handle, managed);
    }

    NativeHandle DetachTransferMemory(TransferMemoryType *tmem) {
        AMS_ASSERT(tmem->state == TransferMemoryType::State_Created);

        /* Set state to detached. */
        tmem->state = TransferMemoryType::State_Detached;

        /* Clear handle. */
        const NativeHandle handle = tmem->handle;

        tmem->handle         = os::InvalidNativeHandle;
        tmem->handle_managed = false;

        return handle;
    }

    void DestroyTransferMemory(TransferMemoryType *tmem) {
        /* Unmap the transfer memory, if required. */
        if (tmem->state == TransferMemoryType::State_Mapped) {
            UnmapTransferMemory(tmem);
        }

        /* Check the state is valid. */
        AMS_ASSERT(tmem->state == TransferMemoryType::State_Created || tmem->state == TransferMemoryType::State_Detached);

        /* Set state to not initialized. */
        tmem->state = TransferMemoryType::State_NotInitialized;

        /* Close the handle, if it's managed. */
        if (tmem->handle_managed) {
            impl::TransferMemoryImpl::Close(tmem->handle);
        }
        tmem->handle_managed = false;

        /* Clear members. */
        tmem->address = nullptr;
        tmem->size    = 0;
        tmem->handle  = os::InvalidNativeHandle;

        /* Destroy the critical section. */
        util::DestroyAt(tmem->cs_transfer_memory);
    }

    Result MapTransferMemory(void **out, TransferMemoryType *tmem, MemoryPermission owner_perm) {
        /* Lock the current thread, and then the transfer memory. */
        std::scoped_lock thread_lk(GetReference(impl::GetCurrentThread()->cs_thread));
        std::scoped_lock lk(GetReference(tmem->cs_transfer_memory));

        /* Ensure we're in a mappable state. */
        AMS_ASSERT(tmem->state == TransferMemoryType::State_Created);

        /* Map. */
        void *mapped_address;
        R_TRY(impl::TransferMemoryImpl::Map(std::addressof(mapped_address), tmem->handle, tmem->size, owner_perm));

        /* Set fields now that we've mapped. */
        tmem->allocated = true;
        tmem->address   = mapped_address;
        tmem->state     = TransferMemoryType::State_Mapped;

        /* Set output address. */
        *out = mapped_address;

        return ResultSuccess();
    }

    void UnmapTransferMemory(TransferMemoryType *tmem) {
        /* Lock the memory. */
        std::scoped_lock lk(GetReference(tmem->cs_transfer_memory));

        /* If the memory isn't mapped, we can't unmap it. */
        if (tmem->state != TransferMemoryType::State_Mapped) {
            return;
        }

        /* Unmap the memory. */
        impl::TransferMemoryImpl::Unmap(tmem->handle, tmem->address, tmem->size);

        /* Unmapped memory is necessarily not allocated. */
        if (tmem->allocated) {
            tmem->allocated = false;
        }

        /* Clear the address. */
        tmem->address = nullptr;
        tmem->state   = TransferMemoryType::State_Created;
    }

}
