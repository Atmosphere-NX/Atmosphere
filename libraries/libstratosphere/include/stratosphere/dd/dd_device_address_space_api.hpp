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
#pragma once
#include <vapours.hpp>
#include <stratosphere/dd/dd_types.hpp>
#include <stratosphere/dd/dd_device_address_space_common.hpp>
#include <stratosphere/dd/dd_device_address_space_types.hpp>

namespace ams::dd {

    Result CreateDeviceAddressSpace(DeviceAddressSpaceType *das, u64 address, u64 size);
    Result CreateDeviceAddressSpace(DeviceAddressSpaceType *das, u64 size);
    void DestroyDeviceAddressSpace(DeviceAddressSpaceType *das);

    void AttachDeviceAddressSpaceHandle(DeviceAddressSpaceType *das, DeviceAddressSpaceHandle handle, bool managed);

    DeviceAddressSpaceHandle GetDeviceAddressSpaceHandle(DeviceAddressSpaceType *das);

    Result MapDeviceAddressSpaceAligned(DeviceAddressSpaceType *das, ProcessHandle process_handle, u64 process_address, size_t size, DeviceVirtualAddress device_address, MemoryPermission device_perm);
    Result MapDeviceAddressSpaceNotAligned(DeviceAddressSpaceType *das, ProcessHandle process_handle, u64 process_address, size_t size, DeviceVirtualAddress device_address, MemoryPermission device_perm);
    void UnmapDeviceAddressSpace(DeviceAddressSpaceType *das, ProcessHandle process_handle, u64 process_address, size_t size, DeviceVirtualAddress device_address);

    Result AttachDeviceAddressSpace(DeviceAddressSpaceType *das, DeviceName device_name);
    void DetachDeviceAddressSpace(DeviceAddressSpaceType *das, DeviceName device_name);

}
