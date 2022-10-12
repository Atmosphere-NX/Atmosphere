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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_aslr_space_manager_impl.os.horizon.hpp"
#elif defined(ATMOSPHERE_OS_WINDOWS)
    #include "os_aslr_space_manager_impl.os.windows.hpp"
#elif defined(ATMOSPHERE_OS_LINUX)
    #include "os_aslr_space_manager_impl.os.linux.hpp"
#elif defined(ATMOSPHERE_OS_MACOS)
    #include "os_aslr_space_manager_impl.os.macos.hpp"
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
            using AddressType = typename Allocator::AddressType;
            using SizeType    = typename Allocator::SizeType;
        private:
            Impl m_impl;
            Allocator m_allocator;
        public:
            AslrSpaceManagerTemplate() : m_impl(), m_allocator(m_impl.GetAslrSpaceBeginAddress(), m_impl.GetAslrSpaceEndAddress(), AslrSpaceGuardSize, m_impl.GetForbiddenRegions(), m_impl.GetForbiddenRegionCount()) {
                /* ... */
            }

            #if defined(ATMOSPHERE_OS_HORIZON)
            AslrSpaceManagerTemplate(os::NativeHandle process_handle) : m_impl(process_handle), m_allocator(m_impl.GetAslrSpaceBeginAddress(process_handle), m_impl.GetAslrSpaceEndAddress(process_handle), AslrSpaceGuardSize, m_impl.GetForbiddenRegions(), m_impl.GetForbiddenRegionCount(), process_handle) {
                /* ... */
            }
            #endif

            AddressType AllocateSpace(SizeType size, SizeType align_offset, AddressSpaceGenerateRandomFunction generate_random) {
                /* Try to allocate a large-aligned space, if we can. */
                if (align_offset || (size / AslrSpaceLargeAlign) != 0) {
                    if (auto large_align = m_allocator.AllocateSpace(size, AslrSpaceLargeAlign, align_offset & (AslrSpaceLargeAlign - 1), generate_random); large_align != 0) {
                        return large_align;
                    }
                }

                /* Allocate a page-aligned space. */
                return m_allocator.AllocateSpace(size, MemoryPageSize, 0, generate_random);
            }

            bool CheckGuardSpace(AddressType address, SizeType size) {
                return m_allocator.CheckGuardSpace(address, size, AslrSpaceGuardSize);
            }

            template<typename MapFunction, typename UnmapFunction>
            Result MapAtRandomAddressWithCustomRandomGenerator(AddressType *out, MapFunction map_function, UnmapFunction unmap_function, SizeType size, SizeType align_offset, AddressSpaceGenerateRandomFunction generate_random) {
                /* Try to map up to 64 times. */
                for (auto i = 0; i < 64; ++i) {
                    /* Reserve space to map the memory. */
                    const uintptr_t map_address = this->AllocateSpace(size, align_offset, generate_random);
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

                        /* NOTE: Nintendo is missing this continue; this is almost certainly a bug. */
                        /* This will cause them to incorrectly return success after unmapping if guard space is not present. */
                        continue;
                    }

                    /* We mapped successfully. */
                    *out = map_address;
                    R_SUCCEED();
                }

                /* We failed to map. */
                R_THROW(os::ResultOutOfAddressSpace());
            }

            template<typename MapFunction, typename UnmapFunction>
            Result MapAtRandomAddress(AddressType *out, MapFunction map_function, UnmapFunction unmap_function, SizeType size, SizeType align_offset) {
                R_RETURN(this->MapAtRandomAddressWithCustomRandomGenerator(out, map_function, unmap_function, size, align_offset, os::impl::AddressSpaceAllocatorDefaultGenerateRandom));
            }
    };

    using AslrSpaceManager = AslrSpaceManagerTemplate<AddressSpaceAllocator, AslrSpaceManagerImpl>;

}
