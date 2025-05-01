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
            size_t m_bit_width;
            size_t m_address;
            size_t m_size;
            Type m_type;
        public:
            static uintptr_t GetAddressSpaceStart(ams::svc::CreateProcessFlag flags, Type type, size_t code_size);
            static size_t GetAddressSpaceSize(ams::svc::CreateProcessFlag flags, Type type);

            static void SetAddressSpaceSize(size_t width, Type type, size_t size);

            constexpr KAddressSpaceInfo(size_t bw, size_t a, size_t s, Type t) : m_bit_width(bw), m_address(a), m_size(s), m_type(t) { /* ... */ }

            constexpr size_t GetWidth() const { return m_bit_width; }
            constexpr size_t GetAddress() const { return m_address; }
            constexpr size_t GetSize() const { return m_size; }
            constexpr Type GetType() const { return m_type; }

            constexpr void SetSize(size_t size) { m_size = size; }
    };

}
