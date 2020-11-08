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
#include "impl/dd_device_address_space_impl.hpp"

namespace ams::dd {

    Result CreateDeviceAddressSpace(DeviceAddressSpaceType *das, u64 address, u64 size) {
        /* Check pre-conditions. */
        AMS_ASSERT(util::IsAligned(address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(size > 0);

        /* Ensure we leave in a consistent state. */
        auto state_guard = SCOPE_GUARD { das->state = DeviceAddressSpaceType::State_NotInitialized; };

        /* Create the address space. */
        DeviceAddressSpaceHandle handle;
        R_TRY(impl::DeviceAddressSpaceImpl::Create(std::addressof(handle), address, size));

        /* Set the values in the das. */
        das->device_handle     = handle;
        das->is_handle_managed = true;
        das->state             = DeviceAddressSpaceType::State_Initialized;

        /* We succeeded. */
        state_guard.Cancel();
        return ResultSuccess();
    }

    Result CreateDeviceAddressSpace(DeviceAddressSpaceType *das, u64 size) {
        return CreateDeviceAddressSpace(das, 0, size);
    }

    void DestroyDeviceAddressSpace(DeviceAddressSpaceType *das) {
        /* Check pre-conditions. */
        AMS_ASSERT(das->state == DeviceAddressSpaceType::State_Initialized);

        /* Destroy the handle. */
        if (das->is_handle_managed) {
            impl::DeviceAddressSpaceImpl::Close(das->device_handle);
        }

        das->device_handle     = 0;
        das->is_handle_managed = false;
        das->state             = DeviceAddressSpaceType::State_NotInitialized;
    }

    void AttachDeviceAddressSpaceHandle(DeviceAddressSpaceType *das, Handle handle, bool managed) {
        /* Check pre-conditions. */
        AMS_ASSERT(handle != svc::InvalidHandle);

        das->device_handle     = handle;
        das->is_handle_managed = managed;
        das->state             = DeviceAddressSpaceType::State_Initialized;
    }

    Handle GetDeviceAddressSpaceHandle(DeviceAddressSpaceType *das) {
        /* Check pre-conditions. */
        AMS_ASSERT(das->state == DeviceAddressSpaceType::State_Initialized);

        return das->device_handle;
    }

    Result MapDeviceAddressSpaceAligned(DeviceAddressSpaceType *das, ProcessHandle process_handle, u64 process_address, size_t size, DeviceVirtualAddress device_address, MemoryPermission device_perm) {
        /* Check pre-conditions. */
        AMS_ASSERT(das->state == DeviceAddressSpaceType::State_Initialized);
        AMS_ASSERT(util::IsAligned(process_address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(device_address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(process_address + size > process_address);
        AMS_ASSERT(device_address + size > device_address);
        AMS_ASSERT(size > 0);
        AMS_ASSERT((process_address & (4_MB - 1)) == (device_address & (4_MB - 1)));

        return impl::DeviceAddressSpaceImpl::MapAligned(das->device_handle, process_handle, process_address, size, device_address, device_perm);
    }

    Result MapDeviceAddressSpaceNotAligned(DeviceAddressSpaceType *das, ProcessHandle process_handle, u64 process_address, size_t size, DeviceVirtualAddress device_address, MemoryPermission device_perm) {
        /* Check pre-conditions. */
        AMS_ASSERT(das->state == DeviceAddressSpaceType::State_Initialized);
        AMS_ASSERT(util::IsAligned(process_address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(device_address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(process_address + size > process_address);
        AMS_ASSERT(device_address + size > device_address);
        AMS_ASSERT(size > 0);

        return impl::DeviceAddressSpaceImpl::MapNotAligned(das->device_handle, process_handle, process_address, size, device_address, device_perm);
    }

    void UnmapDeviceAddressSpace(DeviceAddressSpaceType *das, ProcessHandle process_handle, u64 process_address, size_t size, DeviceVirtualAddress device_address) {
        /* Check pre-conditions. */
        AMS_ASSERT(das->state == DeviceAddressSpaceType::State_Initialized);
        AMS_ASSERT(util::IsAligned(process_address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(device_address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(process_address + size > process_address);
        AMS_ASSERT(device_address + size > device_address);
        AMS_ASSERT(size > 0);

        return impl::DeviceAddressSpaceImpl::Unmap(das->device_handle, process_handle, process_address, size, device_address);
    }

    void InitializeDeviceAddressSpaceMapInfo(DeviceAddressSpaceMapInfo *info, DeviceAddressSpaceType *das, ProcessHandle process_handle, u64 process_address, size_t size, DeviceVirtualAddress device_address, MemoryPermission device_perm) {
        /* Check pre-conditions. */
        AMS_ASSERT(das->state == DeviceAddressSpaceType::State_Initialized);
        AMS_ASSERT(util::IsAligned(process_address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(device_address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));
        AMS_ASSERT(process_address + size > process_address);
        AMS_ASSERT(device_address + size > device_address);
        AMS_ASSERT(size > 0);

        info->last_mapped_size     = 0;
        info->process_address      = process_address;
        info->size                 = size;
        info->device_start_address = device_address;
        info->device_end_address   = device_address + size;
        info->process_handle       = process_handle;
        info->device_permission    = device_perm;
        info->device_address_space = das;
    }

    Result MapNextDeviceAddressSpaceRegion(size_t *out_mapped_size, DeviceAddressSpaceMapInfo *info) {
        /* Check pre-conditions. */
        AMS_ASSERT(info->last_mapped_size == 0);

        size_t mapped_size = 0;
        if (info->device_start_address < info->device_end_address) {
            R_TRY(impl::DeviceAddressSpaceImpl::MapPartially(std::addressof(mapped_size), info->device_address_space->device_handle, info->process_handle, info->process_address, info->size, info->device_start_address, info->device_permission));
        }

        info->last_mapped_size = mapped_size;
        *out_mapped_size       = mapped_size;
        return ResultSuccess();
    }

    void UnmapDeviceAddressSpaceRegion(DeviceAddressSpaceMapInfo *info) {
        /* Check pre-conditions. */
        const size_t last_mapped_size = info->last_mapped_size;
        AMS_ASSERT(last_mapped_size > 0);

        impl::DeviceAddressSpaceImpl::Unmap(info->device_address_space->device_handle, info->process_handle, info->process_address, last_mapped_size, info->device_start_address);

        info->last_mapped_size = 0;
        info->process_address      += last_mapped_size;
        info->device_start_address += last_mapped_size;
    }

    u64 GetMappedProcessAddress(DeviceAddressSpaceMapInfo *info) {
        return info->process_address;
    }

    DeviceVirtualAddress GetMappedDeviceVirtualAddress(DeviceAddressSpaceMapInfo *info) {
        return info->device_start_address;
    }

    size_t GetMappedSize(DeviceAddressSpaceMapInfo *info) {
        return info->last_mapped_size;
    }

    Result AttachDeviceAddressSpace(DeviceAddressSpaceType *das, DeviceName device_name) {
        /* Check pre-conditions. */
        AMS_ASSERT(das->state == DeviceAddressSpaceType::State_Initialized);

        return impl::DeviceAddressSpaceImpl::Attach(das, device_name);
    }

    void DetachDeviceAddressSpace(DeviceAddressSpaceType *das, DeviceName device_name) {
        /* Check pre-conditions. */
        AMS_ASSERT(das->state == DeviceAddressSpaceType::State_Initialized);

        return impl::DeviceAddressSpaceImpl::Detach(das, device_name);
    }

}
