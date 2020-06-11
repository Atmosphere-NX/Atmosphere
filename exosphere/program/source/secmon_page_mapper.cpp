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
#include <exosphere.hpp>
#include "secmon_map.hpp"
#include "secmon_page_mapper.hpp"

namespace ams::secmon {

    namespace impl {

        void *PageMapperImpl::GetPointerTo(uintptr_t phys, size_t size) const {
            /* Ensure we stay within the page. */
            if (util::AlignDown(phys, 4_KB) != this->physical_address) {
                return nullptr;
            }
            if (size != 0) {
                if (util::AlignDown(phys + size - 1, 4_KB) != this->physical_address) {
                    return nullptr;
                }
            }

            return reinterpret_cast<void *>(phys + (this->virtual_address - this->physical_address));
        }

        bool PageMapperImpl::CopyToMapping(uintptr_t dst_phys, const void *src, size_t size) const {
            void * const dst = this->GetPointerTo(dst_phys, size);
            if (dst == nullptr) {
                return false;
            }

            std::memcpy(dst, src, size);
            return true;
        }

        bool PageMapperImpl::CopyFromMapping(void *dst, uintptr_t src_phys, size_t size) const {
            const void * const src = this->GetPointerTo(src_phys, size);
            if (src == nullptr) {
                return false;
            }

            std::memcpy(dst, src, size);
            return true;
        }

    }

    bool UserPageMapper::Map() {
        return this->MapImpl<MapSmcUserPage>();
    }

    bool AtmosphereIramPageMapper::Map() {
        return this->MapImpl<MapAtmosphereIramPage>();
    }

    bool AtmosphereUserPageMapper::Map() {
        return this->MapImpl<MapAtmosphereUserPage>();
    }

    AtmosphereIramPageMapper::~AtmosphereIramPageMapper() {
        this->UnmapImpl<UnmapAtmosphereIramPage>();
    }

    AtmosphereUserPageMapper::~AtmosphereUserPageMapper() {
        this->UnmapImpl<UnmapAtmosphereUserPage>();
    }

}
