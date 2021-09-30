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
#include <stratosphere/os/os_memory_permission.hpp>

namespace ams::os {

    struct IoRegionType;

    Result CreateIoRegion(IoRegionType *io_region, Handle io_pool_handle, uintptr_t address, size_t size, MemoryMapping mapping, MemoryPermission permission);

    void AttachIoRegion(IoRegionType *io_region, size_t size, Handle handle, bool managed);

    void DestroyIoRegion(IoRegionType *io_region);

    Handle GetIoRegionHandle(const IoRegionType *io_region);

    Result MapIoRegion(void **out, IoRegionType *io_region, MemoryPermission perm);
    void UnmapIoRegion(IoRegionType *io_region);

}
