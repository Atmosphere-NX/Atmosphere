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
#include "memlet_service.hpp"

namespace ams::memlet {

    Result Service::CreateAppletSharedMemory(sf::Out<u64> out_size, sf::OutMoveHandle out_handle, u64 desired_size) {
        /* Create a handle to set the output to when done. */
        os::NativeHandle handle = os::InvalidNativeHandle;
        ON_SCOPE_EXIT { out_handle.SetValue(handle, true); };

        /* Check that the requested size has megabyte alignment and isn't too big. */
        R_UNLESS(util::IsAligned(desired_size, 1_MB), os::ResultInvalidParameter());
        R_UNLESS(desired_size <= 128_MB,              os::ResultInvalidParameter());

        /* Try to create a shared memory of the desired size, giving up 1 MB each iteration. */
        os::SharedMemoryType shmem = {};
        while (true) {
            /* If we have zero desired-size left, we've failed. */
            R_UNLESS(desired_size > 0, os::ResultOutOfMemory());

            /* Try to create a shared memory. */
            if (R_FAILED(os::CreateSharedMemory(std::addressof(shmem), desired_size, os::MemoryPermission_ReadWrite, os::MemoryPermission_ReadWrite))) {
                /* We failed, so decrease the size to see if that works. */
                desired_size -= 1_MB;
                continue;
            }

            /* We successfully created the shared memory. */
            break;
        }

        /* Get the native handle for the shared memory we created. */
        handle = os::GetSharedMemoryHandle(std::addressof(shmem));

        /* HACK: Clear the shared memory object, since we've stolen its handle, and there's no "correct" way to detach. */
        shmem = {};

        /* We successfully created a shared memory! */
        *out_size = desired_size;
        R_SUCCEED();
    }

}
