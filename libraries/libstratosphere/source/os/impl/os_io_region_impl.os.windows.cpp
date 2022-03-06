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
#include "os_io_region_impl.hpp"

namespace ams::os::impl {

    Result IoRegionImpl::CreateIoRegion(NativeHandle *out, NativeHandle io_pool_handle, uintptr_t address, size_t size, MemoryMapping mapping, MemoryPermission permission) {
        AMS_UNUSED(out, io_pool_handle, address, size, mapping, permission);
        R_THROW(os::ResultNotSupported());
    }

    Result IoRegionImpl::MapIoRegion(void **out, NativeHandle handle, size_t size, MemoryPermission perm) {
        AMS_UNUSED(out, handle, size, perm);
        R_THROW(os::ResultNotSupported());
    }

    void IoRegionImpl::UnmapIoRegion(NativeHandle handle, void *address, size_t size) {
        AMS_UNUSED(handle, address, size);
    }

}
