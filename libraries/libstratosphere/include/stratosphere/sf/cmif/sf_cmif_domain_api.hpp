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
#include "sf_cmif_service_object_holder.hpp"

namespace ams::sf::cmif {

    struct DomainObjectId {
        u32 value;

        constexpr void SetValue(u32 new_value) { this->value = new_value; }
    };

    static_assert(std::is_trivial<DomainObjectId>::value && sizeof(DomainObjectId) == sizeof(u32), "DomainObjectId");

    inline constexpr bool operator==(const DomainObjectId &lhs, const DomainObjectId &rhs) {
        return lhs.value == rhs.value;
    }

    inline constexpr bool operator!=(const DomainObjectId &lhs, const DomainObjectId &rhs) {
        return lhs.value != rhs.value;
    }

    inline constexpr bool operator<(const DomainObjectId &lhs, const DomainObjectId &rhs) {
        return lhs.value < rhs.value;
    }

    inline constexpr bool operator<=(const DomainObjectId &lhs, const DomainObjectId &rhs) {
        return lhs.value <= rhs.value;
    }

    inline constexpr bool operator>(const DomainObjectId &lhs, const DomainObjectId &rhs) {
        return lhs.value > rhs.value;
    }

    inline constexpr bool operator>=(const DomainObjectId &lhs, const DomainObjectId &rhs) {
        return lhs.value >= rhs.value;
    }

    constexpr inline const DomainObjectId InvalidDomainObjectId = { .value = 0 };

    class ServerDomainBase {
        public:
            virtual Result ReserveIds(DomainObjectId *out_ids, size_t count) = 0;
            virtual void   ReserveSpecificIds(const DomainObjectId *ids, size_t count) = 0;
            virtual void   UnreserveIds(const DomainObjectId *ids, size_t count) = 0;
            virtual void   RegisterObject(DomainObjectId id, ServiceObjectHolder &&obj) = 0;

            virtual ServiceObjectHolder UnregisterObject(DomainObjectId id) = 0;
            virtual ServiceObjectHolder GetObject(DomainObjectId id) = 0;
    };

}
