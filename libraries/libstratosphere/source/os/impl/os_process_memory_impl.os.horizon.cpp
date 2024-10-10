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
#include "os_process_memory_impl.hpp"
#include "os_aslr_space_manager.hpp"

namespace ams::os::impl {

    namespace {

        svc::MemoryPermission ConvertToSvcMemoryPermission(os::MemoryPermission perm) {
            switch (perm) {
                case os::MemoryPermission_None:        return svc::MemoryPermission_None;
                case os::MemoryPermission_ReadOnly:    return svc::MemoryPermission_Read;
                case os::MemoryPermission_ReadWrite:   return svc::MemoryPermission_ReadWrite;
                case os::MemoryPermission_ReadExecute: return svc::MemoryPermission_ReadExecute;
                case os::MemoryPermission_ExecuteOnly: return svc::MemoryPermission_Execute;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    Result ProcessMemoryImpl::Map(void **out, NativeHandle handle, u64 process_address, size_t size, AddressSpaceGenerateRandomFunction generate_random) {
        /* Map at a random address. */
        uintptr_t mapped_address;
        R_TRY(impl::GetAslrSpaceManager().MapAtRandomAddressWithCustomRandomGenerator(std::addressof(mapped_address),
            [handle, process_address](uintptr_t map_address, size_t map_size) -> Result {
                R_TRY_CATCH(svc::MapProcessMemory(map_address, handle, process_address, map_size)) {
                    R_CONVERT(svc::ResultOutOfResource,        os::ResultOutOfResource())
                    R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultInvalidCurrentMemoryState())
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                R_SUCCEED();
            },
            [handle, process_address](uintptr_t map_address, size_t map_size) -> void {
                return ProcessMemoryImpl::Unmap(reinterpret_cast<void *>(map_address), handle, process_address, map_size);
            },
            size,
            process_address,
            generate_random
        ));

        /* Return the address we mapped at. */
        *out = reinterpret_cast<void *>(mapped_address);
        R_SUCCEED();
    }

    void ProcessMemoryImpl::Unmap(void *mapped_memory, NativeHandle handle, u64 process_address, size_t size) {
        R_ABORT_UNLESS(svc::UnmapProcessMemory(reinterpret_cast<uintptr_t>(mapped_memory), handle, process_address, size));
    }

    Result ProcessMemoryImpl::SetMemoryPermission(NativeHandle handle, u64 process_address, u64 size, MemoryPermission perm) {
        /* Set the process memory permission. */
        R_TRY_CATCH(svc::SetProcessMemoryPermission(handle, process_address, size, ConvertToSvcMemoryPermission(perm))) {
            R_CONVERT(svc::ResultOutOfResource,        os::ResultOutOfResource())
            R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultInvalidCurrentMemoryState())
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        R_SUCCEED();
    }

}
