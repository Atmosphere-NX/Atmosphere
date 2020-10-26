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
#include <vapours.hpp>

namespace ams::reg {

    template<typename T>
    concept UnsignedNonConstIntegral = std::unsigned_integral<T> && !std::is_const<T>::value;

    using BitsValue = std::tuple<u16, u16, u32>;
    using BitsMask  = std::tuple<u16, u16>;

    constexpr ALWAYS_INLINE u32 GetOffset(const BitsMask v)  { return static_cast<u32>(std::get<0>(v)); }
    constexpr ALWAYS_INLINE u32 GetOffset(const BitsValue v) { return static_cast<u32>(std::get<0>(v)); }
    constexpr ALWAYS_INLINE u32 GetWidth(const BitsMask v)   { return static_cast<u32>(std::get<1>(v)); }
    constexpr ALWAYS_INLINE u32 GetWidth(const BitsValue v)  { return static_cast<u32>(std::get<1>(v)); }
    constexpr ALWAYS_INLINE u32 GetValue(const BitsValue v)  { return static_cast<u32>(std::get<2>(v)); }

    constexpr ALWAYS_INLINE ::ams::reg::BitsValue GetValue(const BitsMask m, const u32 v) {
        return ::ams::reg::BitsValue{GetOffset(m), GetWidth(m), v};
    }

    constexpr ALWAYS_INLINE u32 EncodeMask(const BitsMask v) {
        return (~0u >> (BITSIZEOF(u32) - GetWidth(v))) << GetOffset(v);
    }

    constexpr ALWAYS_INLINE u32 EncodeMask(const BitsValue v) {
        return (~0u >> (BITSIZEOF(u32) - GetWidth(v))) << GetOffset(v);
    }

    constexpr ALWAYS_INLINE u32 EncodeValue(const BitsValue v) {
        return ((GetValue(v) << GetOffset(v)) & EncodeMask(v));
    }

    template<typename... Values> requires ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    constexpr ALWAYS_INLINE u32 Encode(const Values... values) {
        return (EncodeValue(values) | ...);
    }

    template<typename... Masks> requires ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    constexpr ALWAYS_INLINE u32 EncodeMask(const Masks... masks) {
        return (EncodeMask(masks) | ...);
    }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void Write(volatile IntType *reg, std::type_identity_t<IntType> val) { *reg = val; }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void Write(volatile IntType &reg, std::type_identity_t<IntType> val) {  reg = val; }

    ALWAYS_INLINE void Write(uintptr_t reg, u32 val) { Write(reinterpret_cast<volatile u32 *>(reg), val); }

    template<typename IntType, typename... Values> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE void Write(volatile IntType *reg, const Values... values) { return Write(reg, static_cast<IntType>((EncodeValue(values) | ...))); }

    template<typename IntType, typename... Values> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE void Write(volatile IntType &reg, const Values... values) { return Write(reg, static_cast<IntType>((EncodeValue(values) | ...))); }

    template<typename... Values> requires ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE void Write(uintptr_t reg, const Values... values)     { return Write(reg, (EncodeValue(values) | ...)); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE IntType Read(volatile IntType *reg) { return *reg; }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE IntType Read(volatile IntType &reg) { return  reg; }

    ALWAYS_INLINE u32 Read(uintptr_t reg) { return Read(reinterpret_cast<volatile u32 *>(reg)); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE IntType Read(volatile IntType *reg, std::type_identity_t<IntType> mask) { return *reg & mask; }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE IntType Read(volatile IntType &reg, std::type_identity_t<IntType> mask) { return  reg & mask; }

    ALWAYS_INLINE u32 Read(uintptr_t reg, u32 mask) { return Read(reinterpret_cast<volatile u32 *>(reg), mask); }

    template<typename IntType, typename... Masks> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE IntType Read(volatile IntType *reg, const Masks... masks) { return Read(reg, static_cast<IntType>((EncodeMask(masks) | ...))); }

    template<typename IntType, typename... Masks> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE IntType Read(volatile IntType &reg, const Masks... masks) { return Read(reg, static_cast<IntType>((EncodeMask(masks) | ...))); }

    template<typename... Masks> requires ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE u32 Read(uintptr_t reg, const Masks... masks) { return Read(reg, (EncodeMask(masks) | ...)); }

    template<typename IntType, typename... Values> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE bool HasValue(volatile IntType *reg, const Values... values) { return Read(reg, static_cast<IntType>((EncodeMask(values) | ...))) == static_cast<IntType>(Encode(values...)); }

    template<typename IntType, typename... Values> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE bool HasValue(volatile IntType &reg, const Values... values) { return Read(reg, static_cast<IntType>((EncodeMask(values) | ...))) == static_cast<IntType>(Encode(values...)); }

    template<typename... Values> requires ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE bool HasValue(uintptr_t reg, const Values... values) { return Read(reg, (EncodeMask(values) | ...)) == Encode(values...); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE IntType GetValue(volatile IntType *reg, const BitsMask mask) { return Read(reg, mask) >> GetOffset(mask); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE IntType GetValue(volatile IntType &reg, const BitsMask mask) { return Read(reg, mask) >> GetOffset(mask); }

    ALWAYS_INLINE u32 GetValue(uintptr_t reg, const BitsMask mask) { return Read(reg, mask) >> GetOffset(mask); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void ReadWrite(volatile IntType *reg, std::type_identity_t<IntType> val, std::type_identity_t<IntType> mask) { *reg = (*reg & (~mask)) | (val & mask); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void ReadWrite(volatile IntType &reg, std::type_identity_t<IntType> val, std::type_identity_t<IntType> mask) {  reg = ( reg & (~mask)) | (val & mask); }

    ALWAYS_INLINE void ReadWrite(uintptr_t reg, u32 val, u32 mask) { ReadWrite(reinterpret_cast<volatile u32 *>(reg), val, mask); }

    template<typename IntType, typename... Values> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE void ReadWrite(volatile IntType *reg, const Values... values) { return ReadWrite(reg, static_cast<IntType>((EncodeValue(values) | ...)), static_cast<IntType>((EncodeMask(values) | ...))); }

    template<typename IntType, typename... Values> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE void ReadWrite(volatile IntType &reg, const Values... values) { return ReadWrite(reg, static_cast<IntType>((EncodeValue(values) | ...)), static_cast<IntType>((EncodeMask(values) | ...))); }

    template<typename... Values> requires ((sizeof...(Values) > 0) && (std::is_same<Values, BitsValue>::value && ...))
    ALWAYS_INLINE void ReadWrite(uintptr_t reg, const Values... values)         { return ReadWrite(reg, (EncodeValue(values) | ...), (EncodeMask(values) | ...)); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void SetBits(volatile IntType *reg, std::type_identity_t<IntType> mask) { *reg = *reg | mask; }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void SetBits(volatile IntType &reg, std::type_identity_t<IntType> mask) {  reg =  reg | mask; }

    ALWAYS_INLINE void SetBits(uintptr_t reg, u32 mask) { SetBits(reinterpret_cast<volatile u32 *>(reg), mask); }

    template<typename IntType, typename... Masks> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void SetBits(volatile IntType *reg, const Masks... masks) { return SetBits(reg, static_cast<IntType>((EncodeMask(masks) | ...))); }

    template<typename IntType, typename... Masks> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void SetBits(volatile IntType &reg, const Masks... masks) { return SetBits(reg, static_cast<IntType>((EncodeMask(masks) | ...))); }

    template<typename... Masks> requires ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void SetBits(uintptr_t reg, const Masks... masks)         { return SetBits(reg, (EncodeMask(masks) | ...)); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void ClearBits(volatile IntType *reg, std::type_identity_t<IntType> mask) { *reg = *reg & ~mask; }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void ClearBits(volatile IntType &reg, std::type_identity_t<IntType> mask) {  reg =  reg & ~mask; }

    ALWAYS_INLINE void ClearBits(uintptr_t reg, u32 mask) { ClearBits(reinterpret_cast<volatile u32 *>(reg), mask); }

    template<typename IntType, typename... Masks> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void ClearBits(volatile IntType *reg, const Masks... masks) { return ClearBits(reg, static_cast<IntType>((EncodeMask(masks) | ...))); }

    template<typename IntType, typename... Masks> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void ClearBits(volatile IntType &reg, const Masks... masks) { return ClearBits(reg, static_cast<IntType>((EncodeMask(masks) | ...))); }

    template<typename... Masks> requires ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void ClearBits(uintptr_t reg, const Masks... masks)         { return ClearBits(reg, (EncodeMask(masks) | ...)); }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void MaskBits(volatile IntType *reg, std::type_identity_t<IntType> mask) { *reg = *reg & mask; }

    template<typename IntType> requires UnsignedNonConstIntegral<IntType>
    ALWAYS_INLINE void MaskBits(volatile IntType &reg, std::type_identity_t<IntType> mask) {  reg =  reg & mask; }

    ALWAYS_INLINE void MaskBits(uintptr_t reg, u32 mask) { MaskBits(reinterpret_cast<volatile u32 *>(reg), mask); }

    template<typename IntType, typename... Masks> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void MaskBits(volatile IntType *reg, const Masks... masks) { return MaskBits(reg, static_cast<IntType>((EncodeMask(masks) | ...))); }

    template<typename IntType, typename... Masks> requires UnsignedNonConstIntegral<IntType> && ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void MaskBits(volatile IntType &reg, const Masks... masks) { return MaskBits(reg, static_cast<IntType>((EncodeMask(masks) | ...))); }

    template<typename... Masks> requires ((sizeof...(Masks) > 0) && (std::is_same<Masks, BitsMask>::value && ...))
    ALWAYS_INLINE void MaskBits(uintptr_t reg, const Masks... masks)         { return MaskBits(reg, (EncodeMask(masks) | ...)); }

    #define REG_BITS_MASK(OFFSET, WIDTH)         ::ams::reg::BitsMask{OFFSET, WIDTH}
    #define REG_BITS_VALUE(OFFSET, WIDTH, VALUE) ::ams::reg::BitsValue{OFFSET, WIDTH, VALUE}

    #define REG_BITS_VALUE_FROM_MASK(MASK, VALUE) ::ams::reg::GetValue(MASK, VALUE)

    #define REG_NAMED_BITS_MASK(PREFIX, NAME)         REG_BITS_MASK(PREFIX##_##NAME##_OFFSET, PREFIX##_##NAME##_WIDTH)
    #define REG_NAMED_BITS_VALUE(PREFIX, NAME, VALUE) REG_BITS_VALUE(PREFIX##_##NAME##_OFFSET, PREFIX##_##NAME##_WIDTH, VALUE)
    #define REG_NAMED_BITS_ENUM(PREFIX, NAME, ENUM)   REG_BITS_VALUE(PREFIX##_##NAME##_OFFSET, PREFIX##_##NAME##_WIDTH, PREFIX##_##NAME##_##ENUM)

    #define REG_NAMED_BITS_ENUM_SEL(PREFIX, NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_BITS_VALUE(PREFIX##_##NAME##_OFFSET, PREFIX##_##NAME##_WIDTH, (__COND__) ? PREFIX##_##NAME##_##TRUE_ENUM : PREFIX##_##NAME##_##FALSE_ENUM)

    #define REG_DEFINE_NAMED_REG(PREFIX, NAME, __OFFSET__, __WIDTH__) \
        constexpr inline u32 PREFIX##_##NAME##_OFFSET = __OFFSET__;   \
        constexpr inline u32 PREFIX##_##NAME##_WIDTH  = __WIDTH__

    #define REG_DEFINE_NAMED_BIT_ENUM(PREFIX, NAME, __OFFSET__, ZERO, ONE) \
        REG_DEFINE_NAMED_REG(PREFIX, NAME, __OFFSET__, 1);                 \
                                                                           \
        enum PREFIX##_##NAME {                                             \
            PREFIX##_##NAME##_##ZERO = 0,                                  \
            PREFIX##_##NAME##_##ONE  = 1,                                  \
        };

    #define REG_DEFINE_NAMED_TWO_BIT_ENUM(PREFIX, NAME, __OFFSET__, ZERO, ONE, TWO, THREE) \
        REG_DEFINE_NAMED_REG(PREFIX, NAME, __OFFSET__, 2);                                 \
                                                                                           \
        enum PREFIX##_##NAME {                                                             \
            PREFIX##_##NAME##_##ZERO   = 0,                                                \
            PREFIX##_##NAME##_##ONE    = 1,                                                \
            PREFIX##_##NAME##_##TWO    = 2,                                                \
            PREFIX##_##NAME##_##THREE  = 3,                                                \
        };

    #define REG_DEFINE_NAMED_THREE_BIT_ENUM(PREFIX, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN) \
        REG_DEFINE_NAMED_REG(PREFIX, NAME, __OFFSET__, 3);                                                           \
                                                                                                                     \
        enum PREFIX##_##NAME {                                                                                       \
            PREFIX##_##NAME##_##ZERO   = 0,                                                                          \
            PREFIX##_##NAME##_##ONE    = 1,                                                                          \
            PREFIX##_##NAME##_##TWO    = 2,                                                                          \
            PREFIX##_##NAME##_##THREE  = 3,                                                                          \
            PREFIX##_##NAME##_##FOUR   = 4,                                                                          \
            PREFIX##_##NAME##_##FIVE   = 5,                                                                          \
            PREFIX##_##NAME##_##SIX    = 6,                                                                          \
            PREFIX##_##NAME##_##SEVEN  = 7,                                                                          \
        };

    #define REG_DEFINE_NAMED_FOUR_BIT_ENUM(PREFIX, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) \
        REG_DEFINE_NAMED_REG(PREFIX, NAME, __OFFSET__, 4);                                                                                                                         \
                                                                                                                                                                                   \
        enum PREFIX##_##NAME {                                                                                                                                                     \
            PREFIX##_##NAME##_##ZERO     =  0,                                                                                                                                     \
            PREFIX##_##NAME##_##ONE      =  1,                                                                                                                                     \
            PREFIX##_##NAME##_##TWO      =  2,                                                                                                                                     \
            PREFIX##_##NAME##_##THREE    =  3,                                                                                                                                     \
            PREFIX##_##NAME##_##FOUR     =  4,                                                                                                                                     \
            PREFIX##_##NAME##_##FIVE     =  5,                                                                                                                                     \
            PREFIX##_##NAME##_##SIX      =  6,                                                                                                                                     \
            PREFIX##_##NAME##_##SEVEN    =  7,                                                                                                                                     \
            PREFIX##_##NAME##_##EIGHT    =  8,                                                                                                                                     \
            PREFIX##_##NAME##_##NINE     =  9,                                                                                                                                     \
            PREFIX##_##NAME##_##TEN      = 10,                                                                                                                                     \
            PREFIX##_##NAME##_##ELEVEN   = 11,                                                                                                                                     \
            PREFIX##_##NAME##_##TWELVE   = 12,                                                                                                                                     \
            PREFIX##_##NAME##_##THIRTEEN = 13,                                                                                                                                     \
            PREFIX##_##NAME##_##FOURTEEN = 14,                                                                                                                                     \
            PREFIX##_##NAME##_##FIFTEEN  = 15,                                                                                                                                     \
        };

}
