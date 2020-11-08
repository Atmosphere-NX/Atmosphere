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
#pragma once
#include <stratosphere.hpp>

namespace ams::dd::impl {

    class DeviceAddressSpaceImplByHorizon {
        public:
            static Result Create(DeviceAddressSpaceHandle *out, u64 address, u64 size);
            static void   Close(DeviceAddressSpaceHandle handle);

            static Result MapAligned(DeviceAddressSpaceHandle handle, ProcessHandle process_handle, u64 process_address, size_t process_size, DeviceVirtualAddress device_address, dd::MemoryPermission device_perm);
            static Result MapNotAligned(DeviceAddressSpaceHandle handle, ProcessHandle process_handle, u64 process_address, size_t process_size, DeviceVirtualAddress device_address, dd::MemoryPermission device_perm);
            static Result MapPartially(size_t *out_mapped_size, DeviceAddressSpaceHandle handle, ProcessHandle process_handle, u64 process_address, size_t process_size, DeviceVirtualAddress device_address, dd::MemoryPermission device_perm);
            static void   Unmap(DeviceAddressSpaceHandle handle, ProcessHandle process_handle, u64 process_address, size_t process_size, DeviceVirtualAddress device_address);

            static Result Attach(DeviceAddressSpaceType *das, DeviceName device_name);
            static void   Detach(DeviceAddressSpaceType *das, DeviceName device_name);
    };

    using DeviceAddressSpaceImpl = DeviceAddressSpaceImplByHorizon;

}