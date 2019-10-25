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
#include "../defines.hpp"

namespace ams::util {

    /* Utilities for alignment to power of two. */

    template<typename T>
    constexpr inline T AlignUp(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return static_cast<T>((value + invmask) & ~invmask);
    }

    template<typename T>
    constexpr inline T AlignDown(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return static_cast<T>(value & ~invmask);
    }

    template<typename T>
    constexpr inline bool IsAligned(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return (value & invmask) == 0;
    }

    template<>
    constexpr inline void *AlignUp<void *>(void *value, size_t alignment) {
        return reinterpret_cast<void *>(AlignUp(reinterpret_cast<uintptr_t>(value), alignment));
    }

    template<>
    constexpr inline const void *AlignUp<const void *>(const void *value, size_t alignment) {
        return reinterpret_cast<const void *>(AlignUp(reinterpret_cast<uintptr_t>(value), alignment));
    }

    template<>
    constexpr inline void *AlignDown<void *>(void *value, size_t alignment) {
        return reinterpret_cast<void *>(AlignDown(reinterpret_cast<uintptr_t>(value), alignment));
    }

    template<>
    constexpr inline const void *AlignDown<const void *>(const void *value, size_t alignment) {
        return reinterpret_cast<void *>(AlignDown(reinterpret_cast<uintptr_t>(value), alignment));
    }

    template<>
    constexpr inline bool IsAligned<void *>(void *value, size_t alignment) {
        return IsAligned(reinterpret_cast<uintptr_t>(value), alignment);
    }

    template<>
    constexpr inline bool IsAligned<const void *>(const void *value, size_t alignment) {
        return IsAligned(reinterpret_cast<uintptr_t>(value), alignment);
    }

}