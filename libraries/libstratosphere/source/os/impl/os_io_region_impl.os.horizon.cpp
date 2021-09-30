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

    Result IoRegionImpl::CreateIoRegion(Handle *out, Handle io_pool_handle, uintptr_t address, size_t size, MemoryMapping mapping, MemoryPermission permission) {
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

    Result IoRegionImpl::MapIoRegion(void **out, Handle handle, size_t size, MemoryPermission perm) {
        /* Convert permission. */
        const auto svc_perm = ConvertToSvcMemoryPermission(perm);

        /* NOTE: It is unknown what algorithm Nintendo uses for mapping, so we're using */
        /*       the transfer memory algorithm for now. */

        /* Try to map up to 64 times. */
        for (int i = 0; i < 64; ++i) {
            /* Reserve space to map the memory. */
            void *map_address = impl::GetAslrSpaceManager().AllocateSpace(size);
            R_UNLESS(map_address != nullptr, os::ResultOutOfAddressSpace());

            /* Try to map. */
            /* TODO: Result conversion/abort on unexpected result? */
            R_TRY_CATCH(svc::MapIoRegion(handle, reinterpret_cast<uintptr_t>(map_address), size, svc_perm)) {
                /* If we failed to map at the address, retry. */
                R_CATCH(svc::ResultInvalidCurrentMemory) { continue; }
                R_CATCH(svc::ResultInvalidMemoryRegion)  { continue; }
            } R_END_TRY_CATCH;

            /* Check guard space via aslr manager. */
            if (!impl::GetAslrSpaceManager().CheckGuardSpace(reinterpret_cast<uintptr_t>(map_address), size)) {
                /* We don't have guard space, so unmap. */
                UnmapIoRegion(handle, map_address, size);
                continue;
            }

            /* We mapped successfully. */
            *out = map_address;
            return ResultSuccess();
        }

        /* We failed to map. */
        return os::ResultOutOfAddressSpace();
    }

    void IoRegionImpl::UnmapIoRegion(Handle handle, void *address, size_t size) {
        R_ABORT_UNLESS(svc::UnmapIoRegion(handle, reinterpret_cast<uintptr_t>(address), size));
    }

}
