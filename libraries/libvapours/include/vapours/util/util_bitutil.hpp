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

    namespace impl {

        template<size_t N>
        constexpr inline size_t Log2 = Log2<N / 2> + 1;

        template<>
        constexpr inline size_t Log2<1> = 0;

    }

    template <typename T> requires std::integral<T>
    class BitsOf {
        private:
            static constexpr ALWAYS_INLINE int GetLsbPos(T v) {
                return __builtin_ctzll(static_cast<u64>(v));
            }

            T value;
        public:
            /* Note: GCC has a bug in constant-folding here. Workaround: wrap entire caller with constexpr. */
            constexpr ALWAYS_INLINE BitsOf(T value = T(0u)) : value(value) {
                /* ... */
            }

            constexpr ALWAYS_INLINE bool operator==(const BitsOf &other) const {
                return this->value == other.value;
            }

            constexpr ALWAYS_INLINE bool operator!=(const BitsOf &other) const {
                return this->value != other.value;
            }

            constexpr ALWAYS_INLINE int operator*() const {
                return GetLsbPos(this->value);
            }

            constexpr ALWAYS_INLINE BitsOf &operator++() {
                this->value &= ~(T(1u) << GetLsbPos(this->value));
                return *this;
            }

            constexpr ALWAYS_INLINE BitsOf &operator++(int) {
                BitsOf ret(this->value);
                ++(*this);
                return ret;
            }

            constexpr ALWAYS_INLINE BitsOf begin() const {
                return *this;
            }

            constexpr ALWAYS_INLINE BitsOf end() const {
                return BitsOf(T(0u));
            }
    };

    template<typename T = u64, typename ...Args> requires std::integral<T>
    constexpr ALWAYS_INLINE T CombineBits(Args... args) {
        return (... | (T(1u) << args));
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T ResetLeastSignificantOneBit(T x) {
        return x & (x - 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T SetLeastSignificantZeroBit(T x) {
        return x | (x + 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T LeastSignificantOneBit(T x) {
        return x & ~(x - 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T LeastSignificantZeroBit(T x) {
        return ~x & (x + 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T ResetTrailingOnes(T x) {
        return x & (x + 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T SetTrailingZeros(T x) {
        return x | (x - 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T MaskTrailingZeros(T x) {
        return (~x) & (x - 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T MaskTrailingOnes(T x) {
        return ~((~x) | (x + 1));
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T MaskTrailingZerosAndLeastSignificantOneBit(T x) {
        return x ^ (x - 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T MaskTrailingOnesAndLeastSignificantZeroBit(T x) {
        return x ^ (x + 1);
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE int PopCount(T x) {
        /* TODO: C++20 std::bit_cast */
        using U = typename std::make_unsigned<T>::type;
        U u = static_cast<U>(x);

        if (std::is_constant_evaluated()) {
            /* https://en.wikipedia.org/wiki/Hamming_weight */
            constexpr U m1 = U(-1) / 0x03;
            constexpr U m2 = U(-1) / 0x05;
            constexpr U m4 = U(-1) / 0x11;

            u = static_cast<U>(u - ((u >> 1) & m1));
            u = static_cast<U>((u & m2) + ((u >> 2) & m2));
            u = static_cast<U>((u + (u >> 4)) & m4);

            for (size_t i = 0; i < impl::Log2<sizeof(T)>; ++i) {
                const size_t shift = (0x1 << i) * BITSIZEOF(u8);
                u += u >> shift;
            }

            return static_cast<int>(u & 0x7Fu);
        } else {
            if constexpr (std::is_same<U, unsigned long long>::value) {
                return __builtin_popcountll(u);
            } else if constexpr (std::is_same<U, unsigned long>::value) {
                return __builtin_popcountl(u);
            } else {
                static_assert(sizeof(U) <= sizeof(unsigned int));
                return __builtin_popcount(static_cast<unsigned int>(u));
            }
        }
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE int CountLeadingZeros(T x) {
        if (std::is_constant_evaluated()) {
            for (size_t i = 0; i < impl::Log2<BITSIZEOF(T)>; ++i) {
                const size_t shift = (0x1 << i);
                x |= x >> shift;
            }
            return PopCount(static_cast<T>(~x));
        } else {
            /* TODO: C++20 std::bit_cast */
            using U = typename std::make_unsigned<T>::type;
            const U u = static_cast<U>(x);
            if constexpr (std::is_same<U, unsigned long long>::value) {
                return __builtin_clzll(u);
            } else if constexpr (std::is_same<U, unsigned long>::value) {
                return __builtin_clzl(u);
            }  else if constexpr(std::is_same<U, unsigned int>::value) {
                return __builtin_clz(u);
            } else {
                static_assert(sizeof(U) < sizeof(unsigned int));
                constexpr size_t BitDiff = BITSIZEOF(unsigned int) - BITSIZEOF(U);
                return __builtin_clz(static_cast<unsigned int>(u)) - BitDiff;
            }
        }
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE bool IsPowerOfTwo(T x) {
        return x > 0 && ResetLeastSignificantOneBit(x) == 0;
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T CeilingPowerOfTwo(T x) {
        AMS_ASSERT(x > 0);
        return T(1) << (BITSIZEOF(T) - CountLeadingZeros(T(x - 1)));
    }

    template<typename T> requires std::integral<T>
    constexpr ALWAYS_INLINE T FloorPowerOfTwo(T x) {
        AMS_ASSERT(x > 0);
        return T(1) << (BITSIZEOF(T) - CountLeadingZeros(x) - 1);
    }

    template<typename T, typename U>
    constexpr ALWAYS_INLINE T DivideUp(T v, U d) {
        using Unsigned = typename std::make_unsigned<U>::type;
        const Unsigned add = static_cast<Unsigned>(d) - 1;
        return static_cast<T>((v + add) / d);
    }

}
