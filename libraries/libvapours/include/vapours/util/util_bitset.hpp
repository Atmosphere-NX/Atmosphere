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

        template<typename Storage, size_t N>
        class BitSet {
            private:
                static_assert(std::is_integral<Storage>::value);
                static_assert(std::is_unsigned<Storage>::value);
                static_assert(sizeof(Storage) <= sizeof(u64));

                static constexpr size_t FlagsPerWord = BITSIZEOF(Storage);
                static constexpr size_t NumWords     = util::AlignUp(N, FlagsPerWord) / FlagsPerWord;

                static constexpr ALWAYS_INLINE auto CountLeadingZeroImpl(Storage word) {
                    return __builtin_clzll(static_cast<unsigned long long>(word)) - (BITSIZEOF(unsigned long long) - FlagsPerWord);
                }

                static constexpr ALWAYS_INLINE Storage GetBitMask(size_t bit) {
                    return Storage(1) << (FlagsPerWord - 1 - bit);
                }
            private:
                Storage words[NumWords];
            public:
                constexpr ALWAYS_INLINE BitSet() : words() { /* ... */ }

                constexpr ALWAYS_INLINE void SetBit(size_t i) {
                    this->words[i / FlagsPerWord] |= GetBitMask(i % FlagsPerWord);
                }

                constexpr ALWAYS_INLINE void ClearBit(size_t i) {
                    this->words[i / FlagsPerWord] &= ~GetBitMask(i % FlagsPerWord);
                }

                constexpr ALWAYS_INLINE size_t CountLeadingZero() const {
                    for (size_t i = 0; i < NumWords; i++) {
                        if (this->words[i]) {
                            return FlagsPerWord * i + CountLeadingZeroImpl(this->words[i]);
                        }
                    }
                    return FlagsPerWord * NumWords;
                }

                constexpr ALWAYS_INLINE size_t GetNextSet(size_t n) const {
                    for (size_t i = (n + 1) / FlagsPerWord; i < NumWords; i++) {
                        Storage word = this->words[i];
                        if (!util::IsAligned(n + 1, FlagsPerWord)) {
                            word &= GetBitMask(n % FlagsPerWord) - 1;
                        }
                        if (word) {
                            return FlagsPerWord * i + CountLeadingZeroImpl(word);
                        }
                    }
                    return FlagsPerWord * NumWords;
                }
        };

    }

    template<size_t N>
    using BitSet8  = impl::BitSet<u8, N>;

    template<size_t N>
    using BitSet16 = impl::BitSet<u16, N>;

    template<size_t N>
    using BitSet32 = impl::BitSet<u32, N>;

    template<size_t N>
    using BitSet64 = impl::BitSet<u64, N>;

}
