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

namespace ams::util {

    /* TODO: C++20 std::endian */

    constexpr bool IsLittleEndian() {
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return true;
        #else
            return false;
        #endif
    }

    constexpr bool IsBigEndian() {
        #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            return true;
        #else
            return false;
        #endif
    }

    static_assert(IsLittleEndian() ^ IsBigEndian());

    template<typename U> /* requires unsigned_integral<U> */
    constexpr ALWAYS_INLINE U SwapBytes(const U u) {
        static_assert(BITSIZEOF(u8) == 8);
        constexpr U ByteMask = 0xFFu;
        if constexpr (std::is_same<U, u64>::value) {
            return  ((u & (ByteMask << 56)) >> 56) |
                    ((u & (ByteMask << 48)) >> 40) |
                    ((u & (ByteMask << 40)) >> 24) |
                    ((u & (ByteMask << 32)) >>  8) |
                    ((u & (ByteMask << 24)) <<  8) |
                    ((u & (ByteMask << 16)) << 24) |
                    ((u & (ByteMask <<  8)) << 40) |
                    ((u & (ByteMask <<  0)) << 56);

        } else if constexpr (std::is_same<U, u32>::value) {
            return  ((u & (ByteMask << 24)) >> 24) |
                    ((u & (ByteMask << 16)) >>  8) |
                    ((u & (ByteMask <<  8)) <<  8) |
                    ((u & (ByteMask <<  0)) << 24);

        } else if constexpr (std::is_same<U, u16>::value) {
            return  ((u & (ByteMask << 8)) >> 8) |
                    ((u & (ByteMask << 0)) << 8);

        } else if constexpr (std::is_same<U, u8>::value) {
            return u;
        } else {
            static_assert(!std::is_same<U, U>::value);
        }
    }

    template<typename T> /* requires integral<T> */
    constexpr ALWAYS_INLINE void SwapBytes(T *ptr) {
        using U = typename std::make_unsigned<T>::type;

        *ptr = static_cast<T>(SwapBytes(static_cast<U>(*ptr)));
    }

    template<typename T>
    constexpr ALWAYS_INLINE T ConvertToBigEndian(const T val) {
        if constexpr (IsBigEndian()) {
            return val;
        } else {
            static_assert(IsLittleEndian());
            return SwapBytes(val);
        }
    }

    template<typename T>
    constexpr ALWAYS_INLINE T ConvertToLittleEndian(const T val) {
        if constexpr (IsBigEndian()) {
            return SwapBytes(val);
        } else {
            static_assert(IsLittleEndian());
            return val;
        }
    }

    template<typename T> /* requires integral<T> */
    constexpr ALWAYS_INLINE T LoadBigEndian(T *ptr) {
        return ConvertToBigEndian(*ptr);
    }

    template<typename T> /* requires integral<T> */
    constexpr ALWAYS_INLINE T LoadLittleEndian(T *ptr) {
        return ConvertToLittleEndian(*ptr);
    }

    template<typename T>
    constexpr ALWAYS_INLINE void StoreBigEndian(T *ptr, T val) {
        *ptr = ConvertToBigEndian(val);
    }

    template<typename T>
    constexpr ALWAYS_INLINE void StoreLittleEndian(T *ptr, T val) {
        *ptr = ConvertToLittleEndian(val);
    }

}
