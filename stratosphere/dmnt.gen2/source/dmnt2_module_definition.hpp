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

namespace ams::dmnt {

    class ModuleDefinition {
        NON_MOVEABLE(ModuleDefinition);
        public:
            static constexpr size_t PathLengthMax = 0x200;
        private:
            char m_name[PathLengthMax];
            u64 m_address;
            u64 m_size;
            size_t m_name_start;
        public:
            constexpr ModuleDefinition() : m_name(), m_address(), m_size() { /* ... */ }
            constexpr ~ModuleDefinition() { /* ... */ }

            constexpr ModuleDefinition(const ModuleDefinition &rhs) = default;
            constexpr ModuleDefinition &operator=(const ModuleDefinition &rhs) = default;

            constexpr void Reset() {
                m_name[0]    = 0;
                m_address    = 0;
                m_size       = 0;
                m_name_start = 0;
            }

            constexpr bool operator==(const ModuleDefinition &rhs) const {
                return m_address == rhs.m_address && m_size == rhs.m_size && std::strcmp(m_name, rhs.m_name) == 0;
            }

            constexpr bool operator!=(const ModuleDefinition &rhs) const {
                return !(*this == rhs);
            }

            constexpr char *GetNameBuffer() { return m_name; }
            constexpr const char *GetName() const { return m_name + m_name_start; }

            constexpr u64 GetAddress() const { return m_address; }
            constexpr u64 GetSize() const { return m_size; }

            constexpr void SetAddressSize(u64 address, u64 size) {
                m_address = address;
                m_size    = size;
            }

            constexpr void SetNameStart(size_t offset) {
                m_name_start = offset;
            }
    };

}