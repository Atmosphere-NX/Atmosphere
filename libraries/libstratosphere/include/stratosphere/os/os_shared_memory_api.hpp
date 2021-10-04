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
#include <stratosphere/os/os_memory_permission.hpp>
#include <stratosphere/os/os_native_handle.hpp>

namespace ams::os {

    struct SharedMemoryType;

    Result CreateSharedMemory(SharedMemoryType *shared_memory, size_t size, MemoryPermission my_perm, MemoryPermission other_perm);

    void AttachSharedMemory(SharedMemoryType *shared_memory, size_t size, NativeHandle handle, bool managed);

    void DestroySharedMemory(SharedMemoryType *shared_memory);

    void *MapSharedMemory(SharedMemoryType *shared_memory, MemoryPermission perm);
    void UnmapSharedMemory(SharedMemoryType *shared_memory);

    void *GetSharedMemoryAddress(const SharedMemoryType *shared_memory);
    size_t GetSharedMemorySize(const SharedMemoryType *shared_memory);
    NativeHandle GetSharedMemoryHandle(const SharedMemoryType *shared_memory);

}
