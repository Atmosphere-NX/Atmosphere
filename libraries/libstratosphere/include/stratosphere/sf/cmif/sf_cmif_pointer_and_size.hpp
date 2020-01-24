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
#include "../sf_common.hpp"

namespace ams::sf::cmif {

    class PointerAndSize {
        private:
            uintptr_t pointer;
            size_t size;
        public:
            constexpr PointerAndSize() : pointer(0), size(0) { /* ... */ }
            constexpr PointerAndSize(uintptr_t ptr, size_t sz) : pointer(ptr), size(sz) { /* ... */ }
            constexpr PointerAndSize(void *ptr, size_t sz) : PointerAndSize(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }

            constexpr void *GetPointer() const {
                return reinterpret_cast<void *>(this->pointer);
            }

            constexpr uintptr_t GetAddress() const {
                return this->pointer;
            }

            constexpr size_t GetSize() const {
                return this->size;
            }
    };

}
