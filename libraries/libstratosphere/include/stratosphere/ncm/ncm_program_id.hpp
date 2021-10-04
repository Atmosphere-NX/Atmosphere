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

    struct ProgramId {
        u64 value;

        inline explicit operator svc::ProgramId() const {
            static_assert(sizeof(value) == sizeof(svc::ProgramId));
            return { this->value };
        }
    };

    inline constexpr bool operator==(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value == rhs.value;
    }

    inline constexpr bool operator!=(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value != rhs.value;
    }

    inline constexpr bool operator<(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value < rhs.value;
    }

    inline constexpr bool operator<=(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value <= rhs.value;
    }

    inline constexpr bool operator>(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value > rhs.value;
    }

    inline constexpr bool operator>=(const ProgramId &lhs, const ProgramId &rhs) {
        return lhs.value >= rhs.value;
    }

    inline constexpr const ProgramId InvalidProgramId = {};

}
