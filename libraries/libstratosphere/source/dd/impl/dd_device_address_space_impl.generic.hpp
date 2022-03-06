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
#include <stratosphere.hpp>

namespace ams::dd::impl {

    class DeviceAddressSpaceImplByWindows {
        public:
            static Result Create(DeviceAddressSpaceHandle *, u64, u64) {
                R_THROW(dd::ResultNotSupported());
            }

            static void Close(DeviceAddressSpaceHandle) {
                /* ... */
            }

            static Result MapAligned(DeviceAddressSpaceHandle, ProcessHandle, u64, size_t, DeviceVirtualAddress, dd::MemoryPermission) {
                R_THROW(dd::ResultNotSupported());
            }

            static Result MapNotAligned(DeviceAddressSpaceHandle, ProcessHandle, u64, size_t, DeviceVirtualAddress, dd::MemoryPermission) {
                R_THROW(dd::ResultNotSupported());
            }

            static void Unmap(DeviceAddressSpaceHandle, ProcessHandle, u64, size_t, DeviceVirtualAddress) {
                /* ... */
            }

            static Result Attach(DeviceAddressSpaceType *, DeviceName) {
                R_THROW(dd::ResultNotSupported());
            }

            static void Detach(DeviceAddressSpaceType *, DeviceName) {
                /* ... */
            }
    };

    using DeviceAddressSpaceImpl = DeviceAddressSpaceImplByWindows;

}