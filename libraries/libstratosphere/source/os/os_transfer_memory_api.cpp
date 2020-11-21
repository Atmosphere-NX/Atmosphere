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
#include "impl/os_thread_manager.hpp"
#include "impl/os_transfer_memory_impl.hpp"
#include "impl/os_aslr_space_manager_types.hpp"
#include "impl/os_aslr_space_manager.hpp"

namespace ams::os {

    namespace {

        Result MapTransferMemoryWithAddressUnsafe(TransferMemoryType *tmem, void *address, os::MemoryPermission owner_perm) {
            /* Map the transfer memory. */
            void *mapped_address = nullptr;
            R_TRY(impl::TransferMemoryImpl::Map(std::addressof(mapped_address), tmem->handle, address, tmem->size, owner_perm));

            /* Set fields now that we've mapped. */
            tmem->address = mapped_address;
            tmem->state   = TransferMemoryType::State_Mapped;

            return ResultSuccess();
        }

        inline void SetupTransferMemoryType(TransferMemoryType *tmem, size_t size, Handle handle, bool managed) {
            /* Set members. */
            tmem->handle    = handle;
            tmem->size      = size;
            tmem->address   = nullptr;
            tmem->allocated = false;

            /* Set managed. */
            tmem->handle_managed = managed;

            /* Create the critical section. */
            new (GetPointer(tmem->cs_transfer_memory)) impl::InternalCriticalSection;
        }

    }

    Result CreateTransferMemory(TransferMemoryType *tmem, void *address, size_t size, MemoryPermission perm) {
        /* Validate pre-conditions. */
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(address != nullptr);
        AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(address), os::MemoryPageSize));

        /* Create the memory. */
        Handle handle;
        R_TRY(impl::TransferMemoryImpl::Create(std::addressof(handle), address, size, perm));

        /* Setup the object. */
        SetupTransferMemoryType(tmem, size, handle, true);

        return ResultSuccess();
    }

    Result AttachTransferMemory(TransferMemoryType *tmem, size_t size, Handle handle, bool managed) {
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(handle != svc::InvalidHandle);

        /* Setup the object. */
        SetupTransferMemoryType(tmem, size, handle, managed);

        return ResultSuccess();
    }

    Handle DetachTransferMemory(TransferMemoryType *tmem) {
        AMS_ASSERT(tmem->state == TransferMemoryType::State_Created);

        /* Set state to detached. */
        tmem->state = TransferMemoryType::State_Detached;

        /* Clear handle. */
        Handle handle = tmem->handle;

        tmem->handle         = svc::InvalidHandle;
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
        tmem->handle  = svc::InvalidHandle;

        /* Destroy the critical section. */
        GetReference(tmem->cs_transfer_memory).~InternalCriticalSection();
    }

    Result MapTransferMemory(void **out, TransferMemoryType *tmem, MemoryPermission owner_perm) {
        /* Lock the current thread, and then the transfer memory. */
        std::scoped_lock thread_lk(GetReference(impl::GetCurrentThread()->cs_thread));
        std::scoped_lock lk(GetReference(tmem->cs_transfer_memory));

        /* Ensure we're in a mappable state. */
        AMS_ASSERT(tmem->state == TransferMemoryType::State_Created);

        /* Try to map up to 64 times. */
        for (int i = 0; i < 64; ++i) {
            /* Reserve space to map the memory. */
            void *map_address = impl::GetAslrSpaceManager().AllocateSpace(tmem->size);
            R_UNLESS(map_address != nullptr, os::ResultOutOfAddressSpace());

            /* Mark allocated. */
            tmem->allocated = true;
            auto alloc_guard = SCOPE_GUARD { tmem->allocated = false; };

            /* Try to map. */
            R_TRY_CATCH(MapTransferMemoryWithAddressUnsafe(tmem, map_address, owner_perm)) {
                /* If we failed to map at the address, retry. */
                R_CATCH(os::ResultInvalidCurrentMemoryState) { continue; }
            } R_END_TRY_CATCH;

            /* Check guard space via aslr manager. */
            if (!impl::GetAslrSpaceManager().CheckGuardSpace(reinterpret_cast<uintptr_t>(tmem->address), tmem->size)) {
                /* NOTE: Nintendo bug here. If this case occurs, they will return ResultSuccess() without actually mapping the transfer memory. */
                /* This is because they basically do if (!os::ResultInvalidCurrentMemoryState::Includes(result)) { return result; }, and */
                /* ResultSuccess() is not included by ResultInvalidCurrentMemoryState. */

                /* We will do better than them, and will not falsely return ResultSuccess(). */
                impl::TransferMemoryImpl::Unmap(tmem->handle, tmem->address, tmem->size);
                continue;
            }

            /* We mapped successfully. */
            alloc_guard.Cancel();
            *out = tmem->address;
            return ResultSuccess();
        }

        /* We failed to map. */
        return os::ResultOutOfAddressSpace();
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
