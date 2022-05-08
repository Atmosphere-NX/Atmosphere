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

    consteval bool IsLittleEndian() {
        return std::endian::native == std::endian::little;
    }

    consteval bool IsBigEndian() {
        return std::endian::native == std::endian::big;
    }

    static_assert(IsLittleEndian() ^ IsBigEndian());

    template<std::unsigned_integral U>
    constexpr ALWAYS_INLINE U SwapEndian(const U u) {
        static_assert(BITSIZEOF(u8) == 8);

        if constexpr (sizeof(U) * BITSIZEOF(u8) == 64) {
            static_assert(__builtin_bswap64(UINT64_C(0x0123456789ABCDEF)) == UINT64_C(0xEFCDAB8967452301));
            return __builtin_bswap64(u);
        } else if constexpr (sizeof(U) * BITSIZEOF(u8) == 32) {
            static_assert(__builtin_bswap32(0x01234567u) == 0x67452301u);
            return __builtin_bswap32(u);
        } else if constexpr (sizeof(U) * BITSIZEOF(u8) == 16) {
            static_assert(__builtin_bswap16(0x0123u) == 0x2301u);
            return __builtin_bswap16(u);
        } else if constexpr (sizeof(U) * BITSIZEOF(u8) == 8) {
            return u;
        } else {
            static_assert(!std::is_same<U, U>::value);
        }
    }

    constexpr ALWAYS_INLINE u64 SwapEndian48(const u64 u) {
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

    template<std::integral T>
    constexpr ALWAYS_INLINE void SwapEndian(T *ptr) {
        using U = typename std::make_unsigned<T>::type;

        *ptr = static_cast<T>(SwapEndian<U>(static_cast<U>(*ptr)));
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE T ConvertToBigEndian(const T val) {
        using U = typename std::make_unsigned<T>::type;

        if constexpr (IsBigEndian()) {
            return static_cast<T>(static_cast<U>(val));
        } else {
            static_assert(IsLittleEndian());
            return static_cast<T>(SwapEndian<U>(static_cast<U>(val)));
        }
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE T ConvertToLittleEndian(const T val) {
        using U = typename std::make_unsigned<T>::type;

        if constexpr (IsBigEndian()) {
            return static_cast<T>(SwapEndian<U>(static_cast<U>(val)));
        } else {
            static_assert(IsLittleEndian());
            return static_cast<T>(static_cast<U>(val));
        }
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE T ConvertFromBigEndian(const T val) {
        return ConvertToBigEndian(val);
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE T ConvertFromLittleEndian(const T val) {
        return ConvertToLittleEndian(val);
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE T ConvertToBigEndian48(const T val) {
        using U = typename std::make_unsigned<T>::type;
        static_assert(sizeof(T) == sizeof(u64));

        if constexpr (IsBigEndian()) {
            AMS_ASSERT((static_cast<U>(val) & UINT64_C(0xFFFF000000000000)) == 0);
            return static_cast<T>(static_cast<U>(val));
        } else {
            static_assert(IsLittleEndian());
            return static_cast<T>(SwapEndian48(static_cast<U>(val)));
        }
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE T ConvertToLittleEndian48(const T val) {
        using U = typename std::make_unsigned<T>::type;
        static_assert(sizeof(T) == sizeof(u64));

        if constexpr (IsBigEndian()) {
            return static_cast<T>(SwapEndian48(static_cast<U>(val)));
        } else {
            static_assert(IsLittleEndian());
            AMS_ASSERT((static_cast<U>(val) & UINT64_C(0xFFFF000000000000)) == 0);
            return static_cast<T>(static_cast<U>(val));
        }
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE T LoadBigEndian(const T *ptr) {
        return ConvertToBigEndian<T>(*ptr);
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE T LoadLittleEndian(const T *ptr) {
        return ConvertToLittleEndian<T>(*ptr);
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE void StoreBigEndian(T *ptr, T val) {
        *static_cast<volatile T *>(ptr) = ConvertToBigEndian<T>(val);
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE void StoreLittleEndian(T *ptr, T val) {
        *static_cast<volatile T *>(ptr) = ConvertToLittleEndian<T>(val);
    }

}
