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
#include "impl/os_io_region_impl.hpp"

namespace ams::os {

    namespace {

        void InitializeIoRegion(IoRegionType *io_region, NativeHandle handle, size_t size, bool managed) {
            /* Set state. */
            io_region->state = IoRegionType::State_Initialized;

            /* Set member variables. */
            io_region->handle         = handle;
            io_region->size           = size;
            io_region->handle_managed = managed;
            io_region->mapped_address = nullptr;

            /* Create critical section. */
            util::ConstructAt(io_region->cs_io_region);
        }

    }

    Result CreateIoRegion(IoRegionType *io_region, NativeHandle io_pool_handle, uintptr_t address, size_t size, MemoryMapping mapping, MemoryPermission permission) {
        /* Check pre-conditions. */
        AMS_ASSERT(io_region != nullptr);
        AMS_ASSERT(io_pool_handle != svc::InvalidHandle);
        AMS_ASSERT(util::IsAligned(address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(mapping == MemoryMapping_IoRegister || mapping == MemoryMapping_Uncached || mapping == MemoryMapping_Memory);
        AMS_ASSERT(permission == MemoryPermission_ReadOnly || permission == MemoryPermission_ReadWrite);

        /* If we fail to create, ensure we reset the state. */
        auto state_guard = SCOPE_GUARD { io_region->state = IoRegionType::State_NotInitialized; };

        /* Create the io region. */
        NativeHandle handle;
        R_TRY(impl::IoRegionImpl::CreateIoRegion(std::addressof(handle), io_pool_handle, address, size, mapping, permission));

        /* Setup the object. */
        InitializeIoRegion(io_region, handle, size, true);

        state_guard.Cancel();
        return ResultSuccess();
    }

    void AttachIoRegionHandle(IoRegionType *io_region, size_t size, NativeHandle handle, bool managed) {
        /* Check pre-conditions. */
        AMS_ASSERT(io_region != nullptr);
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(handle != svc::InvalidHandle);

        /* Setup the object. */
        InitializeIoRegion(io_region, handle, size, managed);
    }

    void DestroyIoRegion(IoRegionType *io_region) {
        /* Check pre-conditions. */
        AMS_ASSERT(io_region->state != IoRegionType::State_NotInitialized);

        /* Acquire exclusive access to the io region. */
        std::scoped_lock lk(util::GetReference(io_region->cs_io_region));

        /* Check that we can destroy. */
        AMS_ASSERT(io_region->state == IoRegionType::State_Initialized);

        /* If managed, close the handle. */
        if (io_region->handle_managed) {
            os::CloseNativeHandle(io_region->handle);
        }

        /* Clear members. */
        io_region->handle         = svc::InvalidHandle;
        io_region->handle_managed = false;
        io_region->mapped_address = nullptr;
        io_region->size           = 0;

        /* Mark not initialized. */
        io_region->state          = IoRegionType::State_NotInitialized;
    }

    NativeHandle GetIoRegionHandle(const IoRegionType *io_region) {
        /* Check pre-conditions. */
        AMS_ASSERT(io_region->state != IoRegionType::State_NotInitialized);

        /* Acquire exclusive access to the io region. */
        std::scoped_lock lk(util::GetReference(io_region->cs_io_region));

        /* Get the handle. */
        return io_region->handle;
    }

    Result MapIoRegion(void **out, IoRegionType *io_region, MemoryPermission perm) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(io_region != nullptr);
        AMS_ASSERT(io_region->state != IoRegionType::State_NotInitialized);
        AMS_ASSERT(perm == MemoryPermission_ReadOnly || perm == MemoryPermission_ReadWrite);

        /* Acquire exclusive access to the io region. */
        std::scoped_lock lk(util::GetReference(io_region->cs_io_region));

        /* Check that we can map. */
        AMS_ASSERT(io_region->state == IoRegionType::State_Initialized);

        /* Get the size. */
        const size_t size = io_region->size;
        AMS_ASSERT(size == io_region->size);

        /* Map. */
        void *mapped_address;
        R_TRY(impl::IoRegionImpl::MapIoRegion(std::addressof(mapped_address), io_region->handle, size, perm));

        /* Set mapped address. */
        io_region->mapped_address = mapped_address;

        /* Set mapped. */
        io_region->state = IoRegionType::State_Mapped;

        /* Set the output address. */
        *out = mapped_address;

        return ResultSuccess();
    }

    void UnmapIoRegion(IoRegionType *io_region) {
        /* Check pre-conditions. */
        AMS_ASSERT(io_region->state != IoRegionType::State_NotInitialized);

        /* Acquire exclusive access to the io region. */
        std::scoped_lock lk(util::GetReference(io_region->cs_io_region));

        /* Check that we can unmap. */
        AMS_ASSERT(io_region->state == IoRegionType::State_Mapped);

        /* Get the size. */
        const size_t size = io_region->size;
        AMS_ASSERT(size == io_region->size);

        /* Unmap the io region. */
        impl::IoRegionImpl::UnmapIoRegion(io_region->handle, io_region->mapped_address, size);

        /* Clear mapped address. */
        io_region->mapped_address = nullptr;

        /* Set not-mapped. */
        io_region->state = IoRegionType::State_NotInitialized;
    }

}
