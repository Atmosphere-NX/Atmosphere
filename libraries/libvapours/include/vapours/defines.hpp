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

#define AMS__NARG__(...)  AMS__NARG_I_(__VA_ARGS__,AMS__RSEQ_N())
#define AMS__NARG_I_(...) AMS__ARG_N(__VA_ARGS__)
#define AMS__ARG_N( \
      _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#define AMS__RSEQ_N() \
     63,62,61,60,                   \
     59,58,57,56,55,54,53,52,51,50, \
     49,48,47,46,45,44,43,42,41,40, \
     39,38,37,36,35,34,33,32,31,30, \
     29,28,27,26,25,24,23,22,21,20, \
     19,18,17,16,15,14,13,12,11,10, \
     9,8,7,6,5,4,3,2,1,0

#define AMS__VMACRO_(name, n) name##_##n
#define AMS__VMACRO(name, n) AMS__VMACRO_(name, n)
#define AMS_VMACRO(func, ...) AMS__VMACRO(func, AMS__NARG__(__VA_ARGS__)) (__VA_ARGS__)
