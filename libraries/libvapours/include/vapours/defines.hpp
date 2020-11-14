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
#include <vapours/includes.hpp>

/* Any broadly useful language defines should go here. */

#define NON_COPYABLE(cls) \
    cls(const cls&) = delete; \
    cls& operator=(const cls&) = delete

#define NON_MOVEABLE(cls) \
    cls(cls&&) = delete; \
    cls& operator=(cls&&) = delete

#define ALIGNED(algn) __attribute__((aligned(algn)))
#define NORETURN      __attribute__((noreturn))
#define WEAK_SYMBOL   __attribute__((weak))
#define ALWAYS_INLINE_LAMBDA __attribute__((always_inline))
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NOINLINE      __attribute__((noinline))

#define CONST_FOLD(x) (__builtin_constant_p(x) ? (x) : (x))

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#define BITSIZEOF(x) (sizeof(x) * CHAR_BIT)

#define STRINGIZE(x) STRINGIZE_IMPL(x)
#define STRINGIZE_IMPL(x) #x

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __COUNTER__)
#else
#define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __LINE__)
#endif

#define AMS_PREDICT(expr, value, _probability) __builtin_expect_with_probability(expr, value, ({ \
                                                    constexpr double probability = _probability; \
                                                    static_assert(0.0 <= probability);           \
                                                    static_assert(probability <= 1.0);           \
                                                    probability;                                 \
                                               }))

#define AMS_PREDICT_TRUE(expr, probability)  AMS_PREDICT(!!(expr), 1, probability)
#define AMS_PREDICT_FALSE(expr, probability) AMS_PREDICT(!!(expr), 0, probability)

#define AMS_LIKELY(expr)   AMS_PREDICT_TRUE(expr, 1.0)
#define AMS_UNLIKELY(expr) AMS_PREDICT_FALSE(expr, 1.0)

#define AMS_ASSUME(expr) do { if (!static_cast<bool>((expr))) { __builtin_unreachable(); } } while (0)

#define AMS_CURRENT_FUNCTION_NAME __FUNCTION__

#if defined(__cplusplus)

namespace ams::impl {

    template<typename... ArgTypes>
    constexpr ALWAYS_INLINE void UnusedImpl(ArgTypes &&... args) {
        (static_cast<void>(args), ...);
    }

}

#endif

#define AMS_UNUSED(...) ::ams::impl::UnusedImpl(__VA_ARGS__)

#define AMS_INFINITE_LOOP() do { __asm__ __volatile__("" ::: "memory"); } while (1)
