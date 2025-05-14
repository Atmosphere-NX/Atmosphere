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
#include <cstdint>
#include <cstddef>

/* NOTE: This file serves as a substitute for libnx <switch/types.h>. */

typedef uint8_t u8;       ///<   8-bit unsigned integer.
typedef uint16_t u16;     ///<  16-bit unsigned integer.
typedef uint32_t u32;     ///<  32-bit unsigned integer.
typedef uint64_t u64;     ///<  64-bit unsigned integer.

typedef int8_t s8;       ///<   8-bit signed integer.
typedef int16_t s16;     ///<  16-bit signed integer.
typedef int32_t s32;     ///<  32-bit signed integer.
typedef int64_t s64;     ///<  64-bit signed integer.

typedef volatile u8 vu8;     ///<   8-bit volatile unsigned integer.
typedef volatile u16 vu16;   ///<  16-bit volatile unsigned integer.
typedef volatile u32 vu32;   ///<  32-bit volatile unsigned integer.
typedef volatile u64 vu64;   ///<  64-bit volatile unsigned integer.

typedef volatile s8 vs8;     ///<   8-bit volatile signed integer.
typedef volatile s16 vs16;   ///<  16-bit volatile signed integer.
typedef volatile s32 vs32;   ///<  32-bit volatile signed integer.
typedef volatile s64 vs64;   ///<  64-bit volatile signed integer.

#ifdef ATMOSPHERE_ARCH_ARM64
typedef __uint128_t u128; ///< 128-bit unsigned integer.
typedef __int128_t s128; ///< 128-bit unsigned integer.
typedef volatile u128 vu128; ///< 128-bit volatile unsigned integer.
typedef volatile s128 vs128; ///< 128-bit volatile signed integer.
#endif

typedef u32 Result;          ///< Function error code result type.

/// Creates a bitmask from a bit number.
#ifndef BIT
#define BIT(n) (1U<<(n))
#endif

/// Creates a bitmask from a bit number (long).
#ifndef BITL
#define BITL(n) (1UL<<(n))
#endif

/// Creates a bitmask representing the n least significant bits.
#ifndef MASK
#define MASK(n) (BIT(n) - 1U)
#endif

/// Creates a bitmask representing the n least significant bits (long).
#ifndef MASKL
#define MASKL(n) (BITL(n) - 1UL)
#endif

/// Creates a bitmask for bit range extraction.
#ifndef MASK2
#define MASK2(a,b) (MASK((a) + 1) & ~MASK(b))
#endif

/// Creates a bitmask for bit range extraction (long).
#ifndef MASK2L
#define MASK2L(a,b) (MASKL((a) + 1) & ~MASKL(b))
#endif

/// Marks a function as not returning, for the purposes of compiler optimization.
#ifndef NORETURN
#define NORETURN   __attribute__((noreturn))
#endif

/// This will get un-defined by <vapours/results/results_common.hpp>
#define R_SUCCEEDED(res) (res == 0)
#define R_FAILED(res)    (res != 0)


/// Flags a function as (always) inline.
#define NX_INLINE __attribute__((always_inline)) static inline

/// Flags a function as constexpr in C++14 and above; or as (always) inline otherwise.
#if __cplusplus >= 201402L
#define NX_CONSTEXPR NX_INLINE constexpr
#else
#define NX_CONSTEXPR NX_INLINE
#endif
