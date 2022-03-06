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
#include "impl/os_vamm_manager.hpp"

namespace ams::os {

    void InitializeVirtualAddressMemory() {
        return impl::GetVammManager().InitializeIfEnabled();
    }

    Result AllocateAddressRegion(uintptr_t *out, size_t size) {
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, MemoryPageSize));

        R_RETURN(impl::GetVammManager().AllocateAddressRegion(out, size));
    }

    Result AllocateMemory(uintptr_t *out, size_t size) {
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, MemoryPageSize));

        R_RETURN(impl::GetVammManager().AllocateMemory(out, size));
    }

    Result AllocateMemoryPages(uintptr_t address, size_t size) {
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, MemoryPageSize));
        AMS_ASSERT(util::IsAligned(address, MemoryPageSize));

        R_RETURN(impl::GetVammManager().AllocateMemoryPages(address, size));
    }

    Result FreeAddressRegion(uintptr_t address) {
        R_RETURN(impl::GetVammManager().FreeAddressRegion(address));
    }

    Result FreeMemoryPages(uintptr_t address, size_t size) {
        AMS_ASSERT(size > 0);
        AMS_ASSERT(util::IsAligned(size, MemoryPageSize));
        AMS_ASSERT(util::IsAligned(address, MemoryPageSize));

        R_RETURN(impl::GetVammManager().FreeMemoryPages(address, size));
    }

    VirtualAddressMemoryResourceUsage GetVirtualAddressMemoryResourceUsage() {
        return impl::GetVammManager().GetVirtualAddressMemoryResourceUsage();
    }

    bool IsVirtualAddressMemoryEnabled() {
        return impl::VammManager::IsVirtualAddressMemoryEnabled();
    }

}
