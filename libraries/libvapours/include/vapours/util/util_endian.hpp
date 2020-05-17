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

    constexpr bool IsLittleEndian() {
        return std::endian::native == std::endian::little;
    }

    constexpr bool IsBigEndian() {
        return std::endian::native == std::endian::big;
    }

    static_assert(IsLittleEndian() ^ IsBigEndian());

    template<typename U> requires std::unsigned_integral<U>
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
            AMS_UNUSED(ByteMask);
            return u;
        } else {
            static_assert(!std::is_same<U, U>::value);
        }
    }

    constexpr ALWAYS_INLINE u64 SwapBytes48(const u64 u) {
        using U = u64;
        static_assert(BITSIZEOF(u8) == 8);
        constexpr U ByteMask = 0xFFu;
        AMS_ASSERT((u & UINT64_C(0xFFFF000000000000)) == 0);
        return  ((u & (ByteMask << 40)) >> 40) |
                ((u & (ByteMask << 32)) >> 24) |
                ((u & (ByteMask << 24)) >>  8) |
                ((u & (ByteMask << 16)) <<  8) |
                ((u & (ByteMask <<  8)) << 24) |
                ((u & (ByteMask <<  0)) << 40);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE void SwapBytes(T *ptr) {
        using U = typename std::make_unsigned<T>::type;

        *ptr = static_cast<T>(SwapBytes<U>(static_cast<U>(*ptr)));
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T ConvertToBigEndian(const T val) {
        using U = typename std::make_unsigned<T>::type;

        if constexpr (IsBigEndian()) {
            return static_cast<T>(static_cast<U>(val));
        } else {
            static_assert(IsLittleEndian());
            return static_cast<T>(SwapBytes<U>(static_cast<U>(val)));
        }
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T ConvertToLittleEndian(const T val) {
        using U = typename std::make_unsigned<T>::type;

        if constexpr (IsBigEndian()) {
            return static_cast<T>(SwapBytes<U>(static_cast<U>(val)));
        } else {
            static_assert(IsLittleEndian());
            return static_cast<T>(static_cast<U>(val));
        }
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T ConvertToBigEndian48(const T val) {
        using U = typename std::make_unsigned<T>::type;
        static_assert(sizeof(T) == sizeof(u64));

        if constexpr (IsBigEndian()) {
            AMS_ASSERT((static_cast<U>(val) & UINT64_C(0xFFFF000000000000)) == 0);
            return static_cast<T>(static_cast<U>(val));
        } else {
            static_assert(IsLittleEndian());
            return static_cast<T>(SwapBytes48(static_cast<U>(val)));
        }
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T ConvertToLittleEndian48(const T val) {
        using U = typename std::make_unsigned<T>::type;
        static_assert(sizeof(T) == sizeof(u64));

        if constexpr (IsBigEndian()) {
            return static_cast<T>(SwapBytes48(static_cast<U>(val)));
        } else {
            static_assert(IsLittleEndian());
            AMS_ASSERT((static_cast<U>(val) & UINT64_C(0xFFFF000000000000)) == 0);
            return static_cast<T>(static_cast<U>(val));
        }
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T LoadBigEndian(const T *ptr) {
        return ConvertToBigEndian<T>(*ptr);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T LoadLittleEndian(const T *ptr) {
        return ConvertToLittleEndian<T>(*ptr);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE void StoreBigEndian(T *ptr, T val) {
        *ptr = ConvertToBigEndian<T>(val);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE void StoreLittleEndian(T *ptr, T val) {
        *ptr = ConvertToLittleEndian<T>(val);
    }

}
