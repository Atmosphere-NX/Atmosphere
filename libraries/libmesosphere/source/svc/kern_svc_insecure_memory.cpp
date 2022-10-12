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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        Result MapInsecureMemory(uintptr_t address, size_t size) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Verify that the mapping is in range. */
            auto &pt = GetCurrentProcess().GetPageTable();
            R_UNLESS(GetCurrentProcess().GetPageTable().CanContain(address, size, KMemoryState_Insecure), svc::ResultInvalidMemoryRegion());

            /* Map the insecure memory. */
            R_RETURN(pt.MapInsecureMemory(address, size));
        }

        Result UnmapInsecureMemory(uintptr_t address, size_t size) {
            /* Validate the address/size. */
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Verify that the mapping is in range. */
            auto &pt = GetCurrentProcess().GetPageTable();
            R_UNLESS(GetCurrentProcess().GetPageTable().CanContain(address, size, KMemoryState_Insecure), svc::ResultInvalidMemoryRegion());

            /* Map the insecure memory. */
            R_RETURN(pt.UnmapInsecureMemory(address, size));
        }

    }

    /* =============================    64 ABI    ============================= */

    Result MapInsecureMemory64(ams::svc::Address address, ams::svc::Size size) {
        R_RETURN(MapInsecureMemory(address, size));
    }

    Result UnmapInsecureMemory64(ams::svc::Address address, ams::svc::Size size) {
        R_RETURN(UnmapInsecureMemory(address, size));
    }

    /* ============================= 64From32 ABI ============================= */

    Result MapInsecureMemory64From32(ams::svc::Address address, ams::svc::Size size) {
        R_RETURN(MapInsecureMemory(address, size));
    }

    Result UnmapInsecureMemory64From32(ams::svc::Address address, ams::svc::Size size) {
        R_RETURN(UnmapInsecureMemory(address, size));
    }

}
