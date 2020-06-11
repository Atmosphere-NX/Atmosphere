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
#pragma once
#include <exosphere.hpp>

namespace ams::secmon {

    namespace impl {

        class PageMapperImpl {
            private:
                uintptr_t physical_address;
                uintptr_t virtual_address;
            public:
                constexpr PageMapperImpl(uintptr_t phys) : physical_address(util::AlignDown(phys, 4_KB)), virtual_address() { /* ... */ }

                void *GetPointerTo(uintptr_t phys, size_t size) const;

                bool CopyToMapping(uintptr_t dst_phys, const void *src, size_t size) const;
                bool CopyFromMapping(void *dst, uintptr_t src_phys, size_t size) const;

                ALWAYS_INLINE bool CopyToUser(uintptr_t dst_phys, const void *src, size_t size) const { return CopyToMapping(dst_phys, src, size); }
                ALWAYS_INLINE bool CopyFromUser(void *dst, uintptr_t src_phys, size_t size) const { return CopyFromMapping(dst, src_phys, size); }

                template<auto F>
                bool MapImpl() {
                    this->virtual_address = F(this->physical_address);
                    return this->virtual_address != 0;
                }

                template<auto F>
                void UnmapImpl() {
                    F();
                    this->virtual_address = 0;
                }
        };

    }

    class UserPageMapper : public impl::PageMapperImpl {
        public:
            constexpr UserPageMapper(uintptr_t phys) : PageMapperImpl(phys) { /* ... */ }

            bool Map();
    };

    class AtmosphereIramPageMapper : public impl::PageMapperImpl {
        public:
            constexpr AtmosphereIramPageMapper(uintptr_t phys) : PageMapperImpl(phys) { /* ... */ }
            ~AtmosphereIramPageMapper();

            bool Map();
    };

    class AtmosphereUserPageMapper : public impl::PageMapperImpl {
        public:
            constexpr AtmosphereUserPageMapper(uintptr_t phys) : PageMapperImpl(phys) { /* ... */ }
            ~AtmosphereUserPageMapper();

            bool Map();
    };

}
