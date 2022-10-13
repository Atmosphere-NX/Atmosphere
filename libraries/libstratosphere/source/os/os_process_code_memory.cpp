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
#include "impl/os_process_code_memory_impl.hpp"

namespace ams::os {

    Result MapProcessCodeMemory(u64 *out, NativeHandle handle, const ProcessMemoryRegion *regions, size_t num_regions, AddressSpaceGenerateRandomFunction generate_random) {
        R_RETURN(::ams::os::impl::ProcessCodeMemoryImpl::Map(out, handle, regions, num_regions, generate_random));
    }

    Result UnmapProcessCodeMemory(NativeHandle handle, u64 process_code_address, const ProcessMemoryRegion *regions, size_t num_regions) {
        R_RETURN(::ams::os::impl::ProcessCodeMemoryImpl::Unmap(handle, process_code_address, regions, num_regions));
    }

}
