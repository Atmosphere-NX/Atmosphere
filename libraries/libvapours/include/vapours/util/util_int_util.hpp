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

    namespace impl {

        template<std::signed_integral To, std::signed_integral From>
        constexpr ALWAYS_INLINE bool IsIntValueRepresentableImpl(From v) {
            using ToLimit   = std::numeric_limits<To>;
            using FromLimit = std::numeric_limits<From>;
            if constexpr (ToLimit::min() <= FromLimit::min() && FromLimit::max() <= ToLimit::max()) {
                return true;
            } else {
                return ToLimit::min() <= v && v <= ToLimit::max();
            }
        }

        template<std::unsigned_integral To, std::unsigned_integral From>
        constexpr ALWAYS_INLINE bool IsIntValueRepresentableImpl(From v) {
            using ToLimit   = std::numeric_limits<To>;
            using FromLimit = std::numeric_limits<From>;
            if constexpr (ToLimit::min() <= FromLimit::min() && FromLimit::max() <= ToLimit::max()) {
                return true;
            } else {
                return ToLimit::min() <= v && v <= ToLimit::max();
            }
        }

        template<std::unsigned_integral To, std::signed_integral From>
        constexpr ALWAYS_INLINE bool IsIntValueRepresentableImpl(From v) {
            using UnsignedFrom = typename std::make_unsigned<From>::type;

            if (v < 0) {
                return false;
            } else {
                return IsIntValueRepresentableImpl<To, UnsignedFrom>(static_cast<UnsignedFrom>(v));
            }
        }

        template<std::signed_integral To, std::unsigned_integral From>
        constexpr ALWAYS_INLINE bool IsIntValueRepresentableImpl(From v) {
            using UnsignedTo = typename std::make_unsigned<To>::type;

            return v <= static_cast<UnsignedTo>(std::numeric_limits<To>::max());
        }

    }

    template<std::integral To, std::integral From>
    constexpr ALWAYS_INLINE bool IsIntValueRepresentable(From v) {
        return ::ams::util::impl::IsIntValueRepresentableImpl<To, From>(v);
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE bool CanAddWithoutOverflow(T x, T y) {
        if constexpr (std::unsigned_integral<T>) {
            return x <= std::numeric_limits<T>::max() - y;
        } else {
            if (y >= 0) {
                return x <= std::numeric_limits<T>::max() - y;
            } else {
                return x >= std::numeric_limits<T>::min() - y;
            }
        }
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE bool CanSubtractWithoutOverflow(T x, T y) {
        if constexpr (std::unsigned_integral<T>) {
            return x >= std::numeric_limits<T>::min() + y;
        } else {
            if (y >= 0) {
                return x >= std::numeric_limits<T>::min() + y;
            } else {
                return x <= std::numeric_limits<T>::max() + y;
            }
        }
    }

    template<std::integral T>
    constexpr ALWAYS_INLINE bool CanMultiplyWithoutOverflow(T x, T y) {
        if (x == 0 || y == 0) {
            return true;
        }

        if constexpr (std::unsigned_integral<T>) {
            return y <= std::numeric_limits<T>::max() / x;
        } else {
            if (x > 0) {
                if (y > 0) {
                    return y <= std::numeric_limits<T>::max() / x;
                } else /*if (y < 0) */ {
                    return y >= std::numeric_limits<T>::min() / x;
                }
            } else /* if (x < 0) */ {
                if (y > 0) {
                    return x >= std::numeric_limits<T>::min() / y;
                } else /*if (y < 0) */ {
                    return y >= std::numeric_limits<T>::max() / x;
                }
            }
        }
    }

    template<std::integral T>
    constexpr inline bool TryAddWithoutOverflow(T *out, T x, T y) {
        AMS_ASSERT(out != nullptr);

        if (CanAddWithoutOverflow(x, y)) {
            *out = x + y;
            return true;
        } else {
            return false;
        }
    }

    template<std::integral T>
    constexpr inline bool TrySubtractWithoutOverflow(T *out, T x, T y) {
        AMS_ASSERT(out != nullptr);

        if (CanSubtractWithoutOverflow(x, y)) {
            *out = x - y;
            return true;
        } else {
            return false;
        }
    }

    template<std::integral T>
    constexpr inline bool TryMultiplyWithoutOverflow(T *out, T x, T y) {
        AMS_ASSERT(out != nullptr);

        if (CanMultiplyWithoutOverflow(x, y)) {
            *out = x * y;
            return true;
        } else {
            return false;
        }
    }

}
