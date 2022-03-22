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
#include "os_transfer_memory_impl.hpp"
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

    Result TransferMemoryImpl::Create(NativeHandle *out, void *address, size_t size, MemoryPermission perm) {
        /* Convert memory permission. */
        const auto svc_perm = ConvertToSvcMemoryPermission(perm);

        /* Create the memory. */
        svc::Handle handle;
        R_TRY_CATCH(svc::CreateTransferMemory(std::addressof(handle), reinterpret_cast<uintptr_t>(address), size, svc_perm)) {
            R_CONVERT(svc::ResultOutOfHandles,  os::ResultOutOfHandles())
            R_CONVERT(svc::ResultOutOfResource, os::ResultOutOfTransferMemory())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        *out = handle;
        return ResultSuccess();
    }

    void TransferMemoryImpl::Close(NativeHandle handle) {
        R_ABORT_UNLESS(svc::CloseHandle(handle));
    }

    Result TransferMemoryImpl::Map(void **out, NativeHandle handle, size_t size, MemoryPermission owner_perm) {
        /* Convert memory permission. */
        const auto svc_owner_perm = ConvertToSvcMemoryPermission(owner_perm);

        /* Map at a random address. */
        uintptr_t mapped_address;
        R_TRY(impl::GetAslrSpaceManager().MapAtRandomAddress(std::addressof(mapped_address), size,
            [handle, svc_owner_perm](uintptr_t map_address, size_t map_size) -> Result {
                R_TRY_CATCH(svc::MapTransferMemory(handle, map_address, map_size, svc_owner_perm)) {
                    R_CONVERT(svc::ResultInvalidHandle,        os::ResultInvalidHandle())
                    R_CONVERT(svc::ResultInvalidSize,          os::ResultInvalidTransferMemorySize())
                    R_CONVERT(svc::ResultInvalidState,         os::ResultInvalidTransferMemoryState())
                    R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultInvalidCurrentMemoryState())
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                return ResultSuccess();
            },
            [handle](uintptr_t map_address, size_t map_size) -> void {
                return TransferMemoryImpl::Unmap(handle, reinterpret_cast<void *>(map_address), map_size);
            }
        ));

        /* Return the address we mapped at. */
        *out = reinterpret_cast<void *>(mapped_address);
        return ResultSuccess();
    }

    void TransferMemoryImpl::Unmap(NativeHandle handle, void *address, size_t size) {
        R_ABORT_UNLESS(svc::UnmapTransferMemory(handle, reinterpret_cast<uintptr_t>(address), size));
    }

}
