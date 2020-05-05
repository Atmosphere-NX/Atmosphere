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

        template<typename IntegralStorageType>
        struct BitPack {
            IntegralStorageType value;
            private:
                static_assert(std::is_integral<IntegralStorageType>::value);
                static_assert(std::is_unsigned<IntegralStorageType>::value);

                template<size_t Index, size_t Count>
                static constexpr inline IntegralStorageType Mask = [] {
                    static_assert(Index < BITSIZEOF(IntegralStorageType));
                    static_assert(0 < Count && Count <= BITSIZEOF(IntegralStorageType));
                    static_assert(Index + Count <= BITSIZEOF(IntegralStorageType));

                    if constexpr (Count == BITSIZEOF(IntegralStorageType)) {
                        return ~IntegralStorageType(0);
                    } else {
                        return ((IntegralStorageType(1) << Count) - 1) << Index;
                    }
                }();
            public:
                template<size_t _Index, size_t _Count, typename T = IntegralStorageType>
                struct Field {
                    using Type = T;
                    static constexpr size_t Index = _Index;
                    static constexpr size_t Count = _Count;
                    static constexpr size_t Next = Index + Count;

                    using BitPackType = BitPack<IntegralStorageType>;
                    static_assert(util::is_pod<BitPackType>::value);

                    static_assert(Mask<Index, Count> != 0);
                    static_assert(std::is_integral<T>::value || std::is_enum<T>::value);
                    static_assert(!std::is_same<T, bool>::value || Count == 1);
                };
            public:
                constexpr ALWAYS_INLINE void Clear() {
                    constexpr IntegralStorageType Zero = IntegralStorageType(0);
                    this->value = Zero;
                }

                template<typename FieldType>
                constexpr ALWAYS_INLINE typename FieldType::Type Get() const {
                    static_assert(std::is_same<FieldType, Field<FieldType::Index, FieldType::Count, typename FieldType::Type>>::value);
                    return static_cast<typename FieldType::Type>((this->value & Mask<FieldType::Index, FieldType::Count>) >> FieldType::Index);
                }

                template<typename FieldType>
                constexpr ALWAYS_INLINE void Set(typename FieldType::Type field_value) {
                    static_assert(std::is_same<FieldType, Field<FieldType::Index, FieldType::Count, typename FieldType::Type>>::value);
                    constexpr IntegralStorageType FieldMask = Mask<FieldType::Index, FieldType::Count>;
                    this->value &= ~FieldMask;
                    this->value |= (static_cast<IntegralStorageType>(field_value) << FieldType::Index) & FieldMask;
                }
        };

    }

    using BitPack8  = impl::BitPack<u8>;
    using BitPack16 = impl::BitPack<u16>;
    using BitPack32 = impl::BitPack<u32>;
    using BitPack64 = impl::BitPack<u64>;

    static_assert(util::is_pod<BitPack8>::value);
    static_assert(util::is_pod<BitPack16>::value);
    static_assert(util::is_pod<BitPack32>::value);
    static_assert(util::is_pod<BitPack64>::value);
    static_assert(std::is_trivially_destructible<BitPack8 >::value);
    static_assert(std::is_trivially_destructible<BitPack16>::value);
    static_assert(std::is_trivially_destructible<BitPack32>::value);
    static_assert(std::is_trivially_destructible<BitPack64>::value);

}
