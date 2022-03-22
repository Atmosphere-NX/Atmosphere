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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams::util {

    template<typename Enum> requires std::is_enum<Enum>::value
    constexpr ALWAYS_INLINE typename std::underlying_type<Enum>::type ToUnderlying(Enum e) {
        return static_cast<typename std::underlying_type<Enum>::type>(e);
    }

    template<typename Enum> requires std::is_enum<Enum>::value
    constexpr ALWAYS_INLINE Enum FromUnderlying(typename std::underlying_type<Enum>::type v) {
        return static_cast<Enum>(v);
    }

}
