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
            Storage storage;
            size_t index;
        public:
            constexpr explicit Location() : storage(Storage::Invalid), index(InvalidIndex) { /* ... */ }
            constexpr explicit Location(Storage s, size_t i) : storage(s), index(i) { /* ... */ }

            constexpr size_t GetIndex() const { return this->index; }
            constexpr Storage GetStorage() const { return this->storage; }

            constexpr bool IsValid() const {
                return this->index != InvalidIndex && this->storage != Storage::Invalid;
            }

            constexpr bool operator==(const Location &rhs) const {
                return this->index == rhs.index && this->storage == rhs.storage;
            }

            constexpr bool operator<(const Location &rhs) const {
                if (this->storage < rhs.storage) {
                    return true;
                } else if (this->storage > rhs.storage) {
                    return false;
                } else {
                    return this->index < rhs.index;
                }
            }

            constexpr bool operator>(const Location &rhs) const {
                if (this->storage > rhs.storage) {
                    return true;
                } else if (this->storage < rhs.storage) {
                    return false;
                } else {
                    return this->index > rhs.index;
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
                    char name[IdentifierLengthMax];
                    size_t index;
                public:
                    constexpr explicit Identifier() : name(), index() { /* ... */ }
                    constexpr explicit Identifier(const char *nm, size_t idx = 0) : name(), index(idx) {
                        for (size_t i = 0; i < IdentifierLengthMax && nm[i]; i++) {
                            this->name[i] = nm[i];
                        }
                    }

                    constexpr bool operator==(const Identifier &rhs) const {
                        for (size_t i = 0; i < IdentifierLengthMax; i++) {
                            if (this->name[i] != rhs.name[i]) {
                                return false;
                            }
                        }
                        return this->index == rhs.index;
                    }

                    constexpr bool operator!=(const Identifier &rhs) const {
                        return !(*this == rhs);
                    }
            };
        public:
            Identifier identifier;
            ArgumentType type;
            size_t type_size;
            size_t passed_size;
            bool passed_by_pointer;
            size_t num_locations;
            Location locations[MaxLocations];
        public:
            constexpr explicit Parameter()
                : identifier(), type(ArgumentType::Invalid), type_size(0), passed_size(0), passed_by_pointer(0), num_locations(0), locations()
            { /* ... */ }

            constexpr explicit Parameter(Identifier id, ArgumentType t, size_t ts, size_t ps, bool p, Location l)
                : identifier(id), type(t), type_size(ts), passed_size(ps), passed_by_pointer(p), num_locations(1), locations()
            {
                this->locations[0] = l;
            }

            constexpr Identifier GetIdentifier() const {
                return this->identifier;
            }

            constexpr bool Is(Identifier rhs) const {
                return this->identifier == rhs;
            }

            constexpr ArgumentType GetArgumentType() const {
                return this->type;
            }

            constexpr size_t GetTypeSize() const {
                return this->type_size;
            }

            constexpr size_t GetPassedSize() const {
                return this->passed_size;
            }

            constexpr bool IsPassedByPointer() const {
                return this->passed_by_pointer;
            }

            constexpr size_t GetNumLocations() const {
                return this->num_locations;
            }

            constexpr Location GetLocation(size_t i) const {
                return this->locations[i];
            }

            constexpr void AddLocation(Location l) {
                this->locations[this->num_locations++] = l;
            }

            constexpr bool UsesLocation(Location l) const {
                for (size_t i = 0; i < this->num_locations; i++) {
                    if (this->locations[i] == l) {
                        return true;
                    }
                }
                return false;
            }

            constexpr bool operator==(const Parameter &rhs) const {
                if (!(this->identifier == rhs.identifier &&
                       this->type == rhs.type &&
                       this->type_size == rhs.type_size &&
                       this->passed_size == rhs.passed_size &&
                       this->passed_by_pointer == rhs.passed_by_pointer &&
                       this->num_locations == rhs.num_locations))
                {
                    return false;
                }

                for (size_t i = 0; i < this->num_locations; i++) {
                    if (!(this->locations[i] == rhs.locations[i])) {
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