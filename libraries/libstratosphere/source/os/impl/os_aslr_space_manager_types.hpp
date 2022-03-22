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
#pragma once
#include <stratosphere.hpp>
#include "os_address_space_allocator.hpp"

#ifdef ATMOSPHERE_OS_HORIZON
    #include "os_aslr_space_manager_impl.os.horizon.hpp"
#else
    #error "Unknown OS for AslrSpaceManagerImpl"
#endif

namespace ams::os::impl {

    constexpr inline size_t AslrSpaceLargeAlign = 2_MB;
    constexpr inline size_t AslrSpaceGuardSize  = 4 * MemoryPageSize;

    template<typename Allocator, typename Impl>
    class AslrSpaceManagerTemplate {
        NON_COPYABLE(AslrSpaceManagerTemplate);
        NON_MOVEABLE(AslrSpaceManagerTemplate);
        private:
            Impl m_impl;
            Allocator m_allocator;
        public:
            AslrSpaceManagerTemplate() : m_impl(), m_allocator(m_impl.GetAslrSpaceBeginAddress(), m_impl.GetAslrSpaceEndAddress(), AslrSpaceGuardSize, m_impl.GetForbiddenRegions(), m_impl.GetForbiddenRegionCount()) {
                /* ... */
            }

            uintptr_t AllocateSpace(size_t size) {
                /* Try to allocate a large-aligned space, if we can. */
                if (size >= AslrSpaceLargeAlign) {
                    if (auto large_align = m_allocator.AllocateSpace(size, AslrSpaceLargeAlign, 0); large_align != 0) {
                        return large_align;
                    }
                }

                /* Allocate a page-aligned space. */
                return m_allocator.AllocateSpace(size, MemoryPageSize, 0);
            }

            bool CheckGuardSpace(uintptr_t address, size_t size) {
                return m_allocator.CheckGuardSpace(address, size, AslrSpaceGuardSize);
            }

            template<typename MapFunction, typename UnmapFunction>
            Result MapAtRandomAddress(uintptr_t *out, size_t size, MapFunction map_function, UnmapFunction unmap_function) {
                /* Try to map up to 64 times. */
                for (int i = 0; i < 64; ++i) {
                    /* Reserve space to map the memory. */
                    const uintptr_t map_address = this->AllocateSpace(size);
                    if (map_address == 0) {
                        break;
                    }

                    /* Try to map. */
                    R_TRY_CATCH(map_function(map_address, size)) {
                        /* If we failed to map at the address, retry. */
                        R_CATCH(os::ResultInvalidCurrentMemoryState) { continue; }
                    } R_END_TRY_CATCH;

                    /* Check guard space. */
                    if (!this->CheckGuardSpace(map_address, size)) {
                        /* We don't have guard space, so unmap. */
                        unmap_function(map_address, size);
                        continue;
                    }

                    /* We mapped successfully. */
                    *out = map_address;
                    return ResultSuccess();
                }

                /* We failed to map. */
                return os::ResultOutOfAddressSpace();
            }
    };

    using AslrSpaceManager = AslrSpaceManagerTemplate<AddressSpaceAllocator, AslrSpaceManagerImpl>;

}
