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

namespace ams::nsd {

    struct EnvironmentIdentifier {
        static constexpr size_t Size = 8;
        char value[Size];

        constexpr friend bool operator==(const EnvironmentIdentifier &lhs, const EnvironmentIdentifier &rhs) {
            return util::Strncmp(lhs.value, rhs.value, Size) == 0;
        }

        constexpr friend bool operator!=(const EnvironmentIdentifier &lhs, const EnvironmentIdentifier &rhs) {
            return !(lhs == rhs);
        }
    };

    constexpr inline const EnvironmentIdentifier EnvironmentIdentifierOfProductDevice    = {"lp1"};
    constexpr inline const EnvironmentIdentifier EnvironmentIdentifierOfNotProductDevice = {"dd1"};

}
