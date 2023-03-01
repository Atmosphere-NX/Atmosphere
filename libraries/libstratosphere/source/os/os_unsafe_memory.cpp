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
#include "impl/os_unsafe_memory_impl.hpp"

namespace ams::os {

    Result AllocateUnsafeMemory(uintptr_t *out_address, size_t size) {
        /* Check arguments. */
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));

        /* Allocate memory. */
        R_RETURN(impl::UnsafeMemoryImpl::AllocateUnsafeMemoryImpl(out_address, size));
    }

    Result FreeUnsafeMemory(uintptr_t address, size_t size) {
        /* Check arguments. */
        AMS_ASSERT(util::IsAligned(address, os::MemoryPageSize));
        AMS_ASSERT(util::IsAligned(size, os::MemoryPageSize));

        /* Free memory. */
        R_RETURN(impl::UnsafeMemoryImpl::FreeUnsafeMemoryImpl(address, size));
    }

}
