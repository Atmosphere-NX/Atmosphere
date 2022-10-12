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
#include "impl/os_process_memory_impl.hpp"

namespace ams::os {

    Result MapProcessMemory(void **out, NativeHandle handle, u64 process_address, size_t process_size, AddressSpaceGenerateRandomFunction generate_random) {
        R_RETURN(::ams::os::impl::ProcessMemoryImpl::Map(out, handle, process_address, process_size, generate_random));
    }

    void UnmapProcessMemory(void *mapped_memory, NativeHandle handle, u64 process_address, size_t process_size) {
        return ::ams::os::impl::ProcessMemoryImpl::Unmap(mapped_memory, handle, process_address, process_size);
    }

    Result SetProcessMemoryPermission(NativeHandle handle, u64 process_address, u64 process_size, MemoryPermission perm) {
        R_RETURN(::ams::os::impl::ProcessMemoryImpl::SetMemoryPermission(handle, process_address, process_size, perm));
    }

}
