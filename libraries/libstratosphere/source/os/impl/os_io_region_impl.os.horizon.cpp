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
#include "os_aslr_space_manager_types.hpp"
#include "os_aslr_space_manager.hpp"

namespace ams::os::impl {

    namespace {

        constexpr svc::MemoryPermission ConvertToSvcMemoryPermission(os::MemoryPermission perm) {
            switch (perm) {
                case os::MemoryPermission_None:      return svc::MemoryPermission_None;
                case os::MemoryPermission_ReadOnly:  return svc::MemoryPermission_Read;
                case os::MemoryPermission_WriteOnly: return svc::MemoryPermission_Write;
                case os::MemoryPermission_ReadWrite: return svc::MemoryPermission_ReadWrite;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        constexpr svc::MemoryMapping ConvertToSvcMemoryMapping(os::MemoryMapping mapping) {
            static_assert(std::same_as<svc::MemoryMapping, os::MemoryMapping>);
            return mapping;
        }

    }

    Result IoRegionImpl::CreateIoRegion(NativeHandle *out, NativeHandle io_pool_handle, uintptr_t address, size_t size, MemoryMapping mapping, MemoryPermission permission) {
        /* Convert mapping/permission. */
        const auto svc_mapping = ConvertToSvcMemoryMapping(mapping);
        const auto svc_perm    = ConvertToSvcMemoryPermission(permission);

        /* Create the io region. */
        /* TODO: Result conversion/abort on unexpected result? */
        svc::Handle handle;
        R_TRY(svc::CreateIoRegion(std::addressof(handle), io_pool_handle, address, size, svc_mapping, svc_perm));

        *out = handle;
        return ResultSuccess();
    }

    Result IoRegionImpl::MapIoRegion(void **out, NativeHandle handle, size_t size, MemoryPermission perm) {
        /* Convert permission. */
        const auto svc_perm = ConvertToSvcMemoryPermission(perm);

        /* Map at a random address. */
        uintptr_t mapped_address;
        R_TRY(impl::GetAslrSpaceManager().MapAtRandomAddress(std::addressof(mapped_address), size,
            [handle, svc_perm](uintptr_t map_address, size_t map_size) -> Result {
                R_TRY_CATCH(svc::MapIoRegion(handle, map_address, map_size, svc_perm)) {
                    /* TODO: What's the correct result for these? */
                    // R_CONVERT(svc::ResultInvalidHandle,        os::ResultInvalidHandle())
                    // R_CONVERT(svc::ResultInvalidSize,          os::Result???())
                    // R_CONVERT(svc::ResultInvalidState,         os::Result???())
                    R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultInvalidCurrentMemoryState())
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                return ResultSuccess();
            },
            [handle](uintptr_t map_address, size_t map_size) -> void {
                return IoRegionImpl::UnmapIoRegion(handle, reinterpret_cast<void *>(map_address), map_size);
            }
        ));

        /* Return the address we mapped at. */
        *out = reinterpret_cast<void *>(mapped_address);
        return ResultSuccess();
    }

    void IoRegionImpl::UnmapIoRegion(NativeHandle handle, void *address, size_t size) {
        R_ABORT_UNLESS(svc::UnmapIoRegion(handle, reinterpret_cast<uintptr_t>(address), size));
    }

}
