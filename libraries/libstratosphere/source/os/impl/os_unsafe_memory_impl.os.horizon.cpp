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
#include "os_unsafe_memory_impl.hpp"
#include "os_aslr_space_manager.hpp"

namespace ams::os::impl {

    Result UnsafeMemoryImpl::AllocateUnsafeMemoryImpl(uintptr_t *out_address, size_t size) {
        /* Map at a random address. */
        R_RETURN(impl::GetAslrSpaceManager().MapAtRandomAddress(out_address,
            [](uintptr_t map_address, size_t map_size) -> Result {
                R_TRY_CATCH(svc::MapPhysicalMemoryUnsafe(map_address, map_size)) {
                    R_CONVERT(svc::ResultOutOfResource,        os::ResultOutOfResource())
                    R_CONVERT(svc::ResultOutOfMemory,          os::ResultOutOfMemory())
                    R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultInvalidCurrentMemoryState())
                    R_CONVERT(svc::ResultLimitReached,         os::ResultOutOfMemory())
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                R_SUCCEED();
            },
            [](uintptr_t map_address, size_t map_size) -> void {
                R_ABORT_UNLESS(UnsafeMemoryImpl::FreeUnsafeMemoryImpl(map_address, map_size));
            },
            size,
            0
        ));
    }

    Result UnsafeMemoryImpl::FreeUnsafeMemoryImpl(uintptr_t address, size_t size) {
        R_TRY_CATCH(svc::UnmapPhysicalMemoryUnsafe(address, size)) {
            R_CONVERT(svc::ResultOutOfResource,        os::ResultOutOfResource())
            R_CONVERT(svc::ResultOutOfMemory,          os::ResultOutOfResource())
            R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultBusy())
            R_CONVERT(svc::ResultInvalidMemoryRegion,  os::ResultInvalidParameter())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        R_SUCCEED();
    }

}
