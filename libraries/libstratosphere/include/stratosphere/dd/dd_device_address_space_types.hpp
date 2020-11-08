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
#include <vapours.hpp>
#include <stratosphere/dd/dd_types.hpp>

namespace ams::dd {

    using DeviceVirtualAddress = u64;

    using DeviceAddressSpaceHandle = ::Handle;

    struct DeviceAddressSpaceType {
        enum State {
            State_NotInitialized = 0,
            State_Initialized    = 1,
        };
        DeviceAddressSpaceHandle device_handle;
        u8       state;
        bool     is_handle_managed;
    };
    static_assert(std::is_trivial<DeviceAddressSpaceType>::value);

    struct DeviceAddressSpaceMapInfo {
        size_t last_mapped_size;
        size_t size;
        u64 process_address;
        DeviceVirtualAddress device_start_address;
        DeviceVirtualAddress device_end_address;
        ProcessHandle process_handle;
        MemoryPermission device_permission;
        DeviceAddressSpaceType *device_address_space;
    };
    static_assert(std::is_trivial<DeviceAddressSpaceMapInfo>::value);

}
