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
#include "os_shared_memory_impl.hpp"
#include "os_aslr_space_manager.hpp"

namespace ams::os::impl {

    namespace {

        svc::MemoryPermission ConvertToSvcMemoryPermission(os::MemoryPermission perm) {
            switch (perm) {
                case os::MemoryPermission_None:      return svc::MemoryPermission_None;
                case os::MemoryPermission_ReadOnly:  return svc::MemoryPermission_Read;
                case os::MemoryPermission_WriteOnly: return svc::MemoryPermission_Write;
                case os::MemoryPermission_ReadWrite: return svc::MemoryPermission_ReadWrite;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    Result SharedMemoryImpl::Create(NativeHandle *out, size_t size, MemoryPermission my_perm, MemoryPermission other_perm) {
        /* Convert memory permissions. */
        const auto svc_my_perm    = ConvertToSvcMemoryPermission(my_perm);
        const auto svc_other_perm = ConvertToSvcMemoryPermission(other_perm);

        /* Create the memory. */
        svc::Handle handle;
        R_TRY_CATCH(svc::CreateSharedMemory(std::addressof(handle), size, svc_my_perm, svc_other_perm)) {
            R_CONVERT(svc::ResultOutOfHandles,  os::ResultOutOfHandles())
            R_CONVERT(svc::ResultOutOfResource, os::ResultOutOfResource())
            R_CONVERT(svc::ResultLimitReached,  os::ResultOutOfMemory())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        *out = handle;
        R_SUCCEED();
    }

    void SharedMemoryImpl::Close(NativeHandle handle) {
        R_ABORT_UNLESS(svc::CloseHandle(handle));
    }

    Result SharedMemoryImpl::Map(void **out, NativeHandle handle, size_t size, MemoryPermission perm) {
        /* Convert memory permission. */
        const auto svc_perm = ConvertToSvcMemoryPermission(perm);

        /* Map at a random address. */
        uintptr_t mapped_address;
        R_TRY(impl::GetAslrSpaceManager().MapAtRandomAddress(std::addressof(mapped_address),
            [handle, svc_perm](uintptr_t map_address, size_t map_size) -> Result {
                R_TRY_CATCH(svc::MapSharedMemory(handle, map_address, map_size, svc_perm)) {
                    R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultInvalidCurrentMemoryState())
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                R_SUCCEED();
            },
            [handle](uintptr_t map_address, size_t map_size) -> void {
                return SharedMemoryImpl::Unmap(handle, reinterpret_cast<void *>(map_address), map_size);
            },
            size,
            0
        ));

        /* Return the address we mapped at. */
        *out = reinterpret_cast<void *>(mapped_address);
        R_SUCCEED();
    }

    void SharedMemoryImpl::Unmap(NativeHandle handle, void *address, size_t size) {
        R_ABORT_UNLESS(svc::UnmapSharedMemory(handle, reinterpret_cast<uintptr_t>(address), size));
    }

}
