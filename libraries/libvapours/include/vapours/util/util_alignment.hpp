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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_bitutil.hpp>

namespace ams::util {

    /* Utilities for alignment to power of two. */
    template<typename T>
    constexpr ALWAYS_INLINE T AlignUp(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return static_cast<T>((value + invmask) & ~invmask);
    }

    template<typename T>
    constexpr ALWAYS_INLINE T AlignDown(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return static_cast<T>(value & ~invmask);
    }

    template<typename T>
    constexpr ALWAYS_INLINE bool IsAligned(T value, size_t alignment) {
        using U = typename std::make_unsigned<T>::type;
        const U invmask = static_cast<U>(alignment - 1);
        return (value & invmask) == 0;
    }

    template<>
    constexpr ALWAYS_INLINE void *AlignUp<void *>(void *value, size_t alignment) {
        return reinterpret_cast<void *>(AlignUp(reinterpret_cast<uintptr_t>(value), alignment));
    }

    template<>
    constexpr ALWAYS_INLINE const void *AlignUp<const void *>(const void *value, size_t alignment) {
        return reinterpret_cast<const void *>(AlignUp(reinterpret_cast<uintptr_t>(value), alignment));
    }

    template<>
    constexpr ALWAYS_INLINE void *AlignDown<void *>(void *value, size_t alignment) {
        return reinterpret_cast<void *>(AlignDown(reinterpret_cast<uintptr_t>(value), alignment));
    }

    template<>
    constexpr ALWAYS_INLINE const void *AlignDown<const void *>(const void *value, size_t alignment) {
        return reinterpret_cast<void *>(AlignDown(reinterpret_cast<uintptr_t>(value), alignment));
    }

    template<>
    constexpr ALWAYS_INLINE bool IsAligned<void *>(void *value, size_t alignment) {
        return IsAligned(reinterpret_cast<uintptr_t>(value), alignment);
    }

    template<>
    constexpr ALWAYS_INLINE bool IsAligned<const void *>(const void *value, size_t alignment) {
        return IsAligned(reinterpret_cast<uintptr_t>(value), alignment);
    }

    template<typename T, typename U> requires std::integral<T> && std::integral<U>
    constexpr ALWAYS_INLINE T DivideUp(T x, U y) {
        return (x + (y - 1)) / y;
    }

}
