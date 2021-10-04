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
#include <vapours.hpp>

namespace ams::ncm {

    struct DataId {
        u64 value;

        static const DataId Invalid;
    };

    inline constexpr bool operator==(const DataId &lhs, const DataId &rhs) {
        return lhs.value == rhs.value;
    }

    inline constexpr bool operator!=(const DataId &lhs, const DataId &rhs) {
        return lhs.value != rhs.value;
    }

    inline constexpr bool operator<(const DataId &lhs, const DataId &rhs) {
        return lhs.value < rhs.value;
    }

    inline constexpr bool operator<=(const DataId &lhs, const DataId &rhs) {
        return lhs.value <= rhs.value;
    }

    inline constexpr bool operator>(const DataId &lhs, const DataId &rhs) {
        return lhs.value > rhs.value;
    }

    inline constexpr bool operator>=(const DataId &lhs, const DataId &rhs) {
        return lhs.value >= rhs.value;
    }

    inline constexpr const DataId DataId::Invalid = {};
    inline constexpr const DataId InvalidDataId = DataId::Invalid;

}
