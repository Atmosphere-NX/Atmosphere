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
#include <vapours/svc/codegen/impl/svc_codegen_impl_common.hpp>

namespace ams::svc::codegen::impl {

    enum class Storage {
        Register,
        Stack,
        Invalid,
    };

    class Location {
        private:
            static constexpr size_t InvalidIndex = std::numeric_limits<size_t>::max();
        public:
            Storage m_storage;
            size_t m_index;
        public:
            constexpr explicit Location() : m_storage(Storage::Invalid), m_index(InvalidIndex) { /* ... */ }
            constexpr explicit Location(Storage s, size_t i) : m_storage(s), m_index(i) { /* ... */ }

            constexpr size_t GetIndex() const { return m_index; }
            constexpr Storage GetStorage() const { return m_storage; }

            constexpr bool IsValid() const {
                return m_index != InvalidIndex && m_storage != Storage::Invalid;
            }

            constexpr bool operator==(const Location &rhs) const {
                return m_index == rhs.m_index && m_storage == rhs.m_storage;
            }

            constexpr bool operator<(const Location &rhs) const {
                if (m_storage < rhs.m_storage) {
                    return true;
                } else if (m_storage > rhs.m_storage) {
                    return false;
                } else {
                    return m_index < rhs.m_index;
                }
            }

            constexpr bool operator>(const Location &rhs) const {
                if (m_storage > rhs.m_storage) {
                    return true;
                } else if (m_storage < rhs.m_storage) {
                    return false;
                } else {
                    return m_index > rhs.m_index;
                }
            }

            constexpr bool operator!=(const Location &rhs) const {
                return !(*this == rhs);
            }
    };

    class Parameter {
        public:
            static constexpr size_t MaxLocations = 8;
            static constexpr size_t IdentifierLengthMax = 0x40;
            class Identifier {
                public:
                    char m_name[IdentifierLengthMax];
                    size_t m_index;
                public:
                    constexpr explicit Identifier() : m_name(), m_index() { /* ... */ }
                    constexpr explicit Identifier(const char *nm, size_t idx = 0) : m_name(), m_index(idx) {
                        for (size_t i = 0; i < IdentifierLengthMax && nm[i]; i++) {
                            m_name[i] = nm[i];
                        }
                    }

                    constexpr bool operator==(const Identifier &rhs) const {
                        for (size_t i = 0; i < IdentifierLengthMax; i++) {
                            if (m_name[i] != rhs.m_name[i]) {
                                return false;
                            }
                        }
                        return m_index == rhs.m_index;
                    }

                    constexpr bool operator!=(const Identifier &rhs) const {
                        return !(*this == rhs);
                    }
            };
        public:
            Identifier m_identifier;
            ArgumentType m_type;
            size_t m_type_size;
            size_t m_passed_size;
            bool m_passed_by_pointer;
            bool m_is_boolean;
            size_t m_num_locations;
            Location m_locations[MaxLocations];
        public:
            constexpr explicit Parameter()
                : m_identifier(), m_type(ArgumentType::Invalid), m_type_size(0), m_passed_size(0), m_passed_by_pointer(0), m_is_boolean(0), m_num_locations(0), m_locations()
            { /* ... */ }

            constexpr explicit Parameter(Identifier id, ArgumentType t, size_t ts, size_t ps, bool p, bool b, Location l)
                : m_identifier(id), m_type(t), m_type_size(ts), m_passed_size(ps), m_passed_by_pointer(p), m_is_boolean(b), m_num_locations(1), m_locations()
            {
                m_locations[0] = l;
            }

            constexpr Identifier GetIdentifier() const {
                return m_identifier;
            }

            constexpr bool Is(Identifier rhs) const {
                return m_identifier == rhs;
            }

            constexpr ArgumentType GetArgumentType() const {
                return m_type;
            }

            constexpr size_t GetTypeSize() const {
                return m_type_size;
            }

            constexpr size_t GetPassedSize() const {
                return m_passed_size;
            }

            constexpr bool IsPassedByPointer() const {
                return m_passed_by_pointer;
            }

            constexpr bool IsBoolean() const {
                return m_is_boolean;
            }

            constexpr size_t GetNumLocations() const {
                return m_num_locations;
            }

            constexpr Location GetLocation(size_t i) const {
                return m_locations[i];
            }

            constexpr void AddLocation(Location l) {
                m_locations[m_num_locations++] = l;
            }

            constexpr bool UsesLocation(Location l) const {
                for (size_t i = 0; i < m_num_locations; i++) {
                    if (m_locations[i] == l) {
                        return true;
                    }
                }
                return false;
            }

            constexpr bool operator==(const Parameter &rhs) const {
                if (!(m_identifier == rhs.m_identifier &&
                       m_type == rhs.m_type &&
                       m_type_size == rhs.m_type_size &&
                       m_passed_size == rhs.m_passed_size &&
                       m_passed_by_pointer == rhs.m_passed_by_pointer &&
                       m_is_boolean == rhs.m_is_boolean  &&
                       m_num_locations == rhs.m_num_locations))
                {
                    return false;
                }

                for (size_t i = 0; i < m_num_locations; i++) {
                    if (!(m_locations[i] == rhs.m_locations[i])) {
                        return false;
                    }
                }

                return true;
            }

            constexpr bool operator!=(const Parameter &rhs) const {
                return !(*this == rhs);
            }
    };


}