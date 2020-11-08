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
#include "dd_device_address_space_impl.os.horizon.hpp"

namespace ams::dd::impl {

    static_assert(static_cast<int>(dd::MemoryPermission_None) == static_cast<int>(svc::MemoryPermission_None));
    static_assert(static_cast<int>(dd::MemoryPermission_ReadOnly) == static_cast<int>(svc::MemoryPermission_Read));
    static_assert(static_cast<int>(dd::MemoryPermission_WriteOnly) == static_cast<int>(svc::MemoryPermission_Write));
    static_assert(static_cast<int>(dd::MemoryPermission_ReadWrite) == static_cast<int>(svc::MemoryPermission_ReadWrite));

    Result DeviceAddressSpaceImplByHorizon::Create(DeviceAddressSpaceHandle *out, u64 address, u64 size) {
        /* Create the space. */
        svc::Handle handle;
        R_TRY_CATCH(svc::CreateDeviceAddressSpace(std::addressof(handle), address, size)) {
            R_CONVERT(svc::ResultOutOfMemory,   dd::ResultOutOfMemory())
            R_CONVERT(svc::ResultOutOfResource, dd::ResultOutOfResource())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        *out = static_cast<DeviceAddressSpaceHandle>(handle);
        return ResultSuccess();
    }

    void DeviceAddressSpaceImplByHorizon::Close(DeviceAddressSpaceHandle handle) {
        const auto svc_handle = svc::Handle(handle);
        if (svc_handle == svc::PseudoHandle::CurrentThread || svc_handle == svc::PseudoHandle::CurrentProcess) {
            return;
        }

        R_ABORT_UNLESS(svc::CloseHandle(svc_handle));
    }

    Result DeviceAddressSpaceImplByHorizon::MapAligned(DeviceAddressSpaceHandle handle, ProcessHandle process_handle, u64 process_address, size_t process_size, DeviceVirtualAddress device_address, dd::MemoryPermission device_perm) {
        /* Check alignment. */
        AMS_ABORT_UNLESS((process_address & (4_MB - 1)) == (device_address & (4_MB - 1)));

        R_TRY_CATCH(svc::MapDeviceAddressSpaceAligned(svc::Handle(handle), svc::Handle(process_handle), process_address, process_size, device_address, static_cast<svc::MemoryPermission>(device_perm))) {
            R_CONVERT(svc::ResultInvalidHandle,        dd::ResultInvalidHandle())
            R_CONVERT(svc::ResultOutOfMemory,          dd::ResultOutOfMemory())
            R_CONVERT(svc::ResultOutOfResource,        dd::ResultOutOfResource())
            R_CONVERT(svc::ResultInvalidCurrentMemory, dd::ResultInvalidMemoryState())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        return ResultSuccess();
    }

    Result DeviceAddressSpaceImplByHorizon::MapNotAligned(DeviceAddressSpaceHandle handle, ProcessHandle process_handle, u64 process_address, size_t process_size, DeviceVirtualAddress device_address, dd::MemoryPermission device_perm) {
        R_TRY_CATCH(svc::MapDeviceAddressSpaceByForce(svc::Handle(handle), svc::Handle(process_handle), process_address, process_size, device_address, static_cast<svc::MemoryPermission>(device_perm))) {
            R_CONVERT(svc::ResultInvalidHandle,        dd::ResultInvalidHandle())
            R_CONVERT(svc::ResultOutOfMemory,          dd::ResultOutOfMemory())
            R_CONVERT(svc::ResultOutOfResource,        dd::ResultOutOfResource())
            R_CONVERT(svc::ResultInvalidCurrentMemory, dd::ResultInvalidMemoryState())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        return ResultSuccess();
    }

    Result DeviceAddressSpaceImplByHorizon::MapPartially(size_t *out_mapped_size, DeviceAddressSpaceHandle handle, ProcessHandle process_handle, u64 process_address, size_t process_size, DeviceVirtualAddress device_address, dd::MemoryPermission device_perm) {
        ams::svc::Size mapped_size = 0;
        R_TRY_CATCH(svc::MapDeviceAddressSpace(std::addressof(mapped_size), svc::Handle(handle), svc::Handle(process_handle), process_address, process_size, device_address, static_cast<svc::MemoryPermission>(device_perm))) {
            R_CONVERT(svc::ResultInvalidHandle,        dd::ResultInvalidHandle())
            R_CONVERT(svc::ResultOutOfMemory,          dd::ResultOutOfMemory())
            R_CONVERT(svc::ResultOutOfResource,        dd::ResultOutOfResource())
            R_CONVERT(svc::ResultInvalidCurrentMemory, dd::ResultInvalidMemoryState())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        *out_mapped_size = mapped_size;
        return ResultSuccess();
    }

    void DeviceAddressSpaceImplByHorizon::Unmap(DeviceAddressSpaceHandle handle, ProcessHandle process_handle, u64 process_address, size_t process_size, DeviceVirtualAddress device_address) {
        R_ABORT_UNLESS(svc::UnmapDeviceAddressSpace(svc::Handle(handle), svc::Handle(process_handle), process_address, process_size, device_address));
    }

    Result DeviceAddressSpaceImplByHorizon::Attach(DeviceAddressSpaceType *das, DeviceName device_name) {
        R_TRY_CATCH(svc::AttachDeviceAddressSpace(static_cast<svc::DeviceName>(device_name), svc::Handle(das->device_handle))) {
            R_CONVERT(svc::ResultOutOfMemory,          dd::ResultOutOfMemory())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        return ResultSuccess();
    }

    void DeviceAddressSpaceImplByHorizon::Detach(DeviceAddressSpaceType *das, DeviceName device_name) {
        R_ABORT_UNLESS(svc::DetachDeviceAddressSpace(static_cast<svc::DeviceName>(device_name), svc::Handle(das->device_handle)));
    }

}