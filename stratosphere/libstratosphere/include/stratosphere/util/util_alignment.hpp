/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include "../defines.hpp"
#include <type_traits>

namespace sts::util {

    /* Utilities for alignment to power of two. */

    template<typename T>
    static constexpr inline T AlignUp(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return static_cast<T>((value + invmask) & ~invmask);
    }

    template<typename T>
    static constexpr inline T AlignDown(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return static_cast<T>(value & ~invmask);
    }

    template<typename T>
    static constexpr inline bool IsAligned(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return (value & invmask) == 0;
    }

}