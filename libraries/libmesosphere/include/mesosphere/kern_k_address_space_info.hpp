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

namespace ams::kern {



    struct KAddressSpaceInfo {
        public:
            enum Type {
                Type_MapSmall   = 0,
                Type_MapLarge   = 1,
                Type_Map39Bit   = 2,
                Type_Heap       = 3,
                Type_Stack      = 4,
                Type_Alias      = 5,

                Type_Count,
            };
        private:
            size_t bit_width;
            size_t address;
            size_t size;
            Type type;
        public:
            static uintptr_t GetAddressSpaceStart(size_t width, Type type);
            static size_t GetAddressSpaceSize(size_t width, Type type);

            constexpr KAddressSpaceInfo(size_t bw, size_t a, size_t s, Type t) : bit_width(bw), address(a), size(s), type(t) { /* ... */ }

            constexpr size_t GetWidth() const { return this->bit_width; }
            constexpr size_t GetAddress() const { return this->address; }
            constexpr size_t GetSize() const { return this->size; }
            constexpr Type GetType() const { return this->type; }
    };

}
