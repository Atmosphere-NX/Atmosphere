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

    template <typename T>
    class BitsOf {
        private:
            static_assert(std::is_integral<T>::value);

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

    template<typename T = u64, typename ...Args>
    constexpr ALWAYS_INLINE T CombineBits(Args... args) {
        return (... | (T(1u) << args));
    }

}
