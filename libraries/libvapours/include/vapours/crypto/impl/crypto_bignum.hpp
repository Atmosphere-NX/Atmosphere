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
#include <vapours/util.hpp>
#include <vapours/crypto/crypto_memory_compare.hpp>
#include <vapours/crypto/crypto_memory_clear.hpp>

namespace ams::crypto::impl {

    class BigNum {
        NON_COPYABLE(BigNum);
        NON_MOVEABLE(BigNum);
        public:
            using HalfWord   = u16;
            using Word       = u32;
            using DoubleWord = u64;

            static constexpr size_t MaxBits = 4096;
            static constexpr size_t BitsPerWord = sizeof(Word) * CHAR_BIT;
            static constexpr Word MaxWord     = std::numeric_limits<Word>::max();
            static constexpr Word MaxHalfWord = std::numeric_limits<HalfWord>::max();

            class WordAllocator {
                    NON_COPYABLE(WordAllocator);
                    NON_MOVEABLE(WordAllocator);
                public:
                    class Allocation {
                        NON_COPYABLE(Allocation);
                        NON_MOVEABLE(Allocation);
                        private:
                            friend class WordAllocator;
                        private:
                            WordAllocator *allocator;
                            Word *buffer;
                            size_t count;
                        private:
                            constexpr ALWAYS_INLINE Allocation(WordAllocator *a, Word *w, size_t c) : allocator(a), buffer(w), count(c) { /* ... */ }
                        public:
                            ALWAYS_INLINE ~Allocation() { if (allocator) { allocator->Free(this->buffer, this->count); } }

                            constexpr ALWAYS_INLINE Word *GetBuffer() const { return this->buffer; }
                            constexpr ALWAYS_INLINE size_t GetCount() const { return this->count; }
                            constexpr ALWAYS_INLINE bool IsValid() const { return this->buffer != nullptr; }
                    };

                    friend class Allocation;
                private:
                    Word *buffer;
                    size_t count;
                    size_t max_count;
                    size_t min_count;
                private:
                    ALWAYS_INLINE void Free(void *words, size_t num) {
                        this->buffer -= num;
                        this->count  += num;

                        AMS_ASSERT(words == this->buffer);
                    }
                public:
                    constexpr ALWAYS_INLINE WordAllocator(Word *buf, size_t c) : buffer(buf), count(c), max_count(c), min_count(c) { /* ... */ }

                    ALWAYS_INLINE Allocation Allocate(size_t num) {
                        if (num <= this->count) {
                            Word *allocated = this->buffer;

                            this->buffer += num;
                            this->count -= num;
                            this->min_count = std::min(this->count, this->min_count);

                            return Allocation(this, allocated, num);
                        } else {
                            return Allocation(nullptr, nullptr, 0);
                        }
                    }

                    constexpr ALWAYS_INLINE size_t GetMaxUsedSize() const {
                        return (this->max_count - this->min_count) * sizeof(Word);
                    }
            };
        private:
            Word *words;
            size_t num_words;
            size_t max_words;
        private:
            static void ImportImpl(Word *out, size_t out_size, const u8 *src, size_t src_size);
            static void ExportImpl(u8 *out, size_t out_size, const Word *src, size_t src_size);
        public:
            constexpr BigNum() : words(), num_words(), max_words() { /* ... */ }
            ~BigNum() { /* ... */ }

            constexpr void ReserveStatic(Word *buf, size_t capacity) {
                this->words = buf;
                this->max_words = capacity;
            }

            bool Import(const void *src, size_t src_size);
            void Export(void *dst, size_t dst_size);

            size_t GetSize() const;

            bool IsZero() const {
                return this->num_words == 0;
            }

            bool ExpMod(void *dst, const void *src, size_t size, const BigNum &exp, u32 *work_buf, size_t work_buf_size) const;
            void ClearToZero();
            void UpdateCount();
        public:
            /* Utility. */
            static bool   IsZero(const Word *w, size_t num_words);
            static int    Compare(const Word *lhs, const Word *rhs, size_t num_words);
            static size_t CountWords(const Word *w, size_t num_words);
            static size_t CountSignificantBits(Word w);
            static void   ClearToZero(Word *w, size_t num_words);
            static void   SetToWord(Word *w, size_t num_words, Word v);
            static void   Copy(Word *dst, const Word *src, size_t num_words);

            /* Arithmetic. */
            static bool   ExpMod(Word *dst, const Word *src, const Word *exp, size_t exp_num_words, const Word *mod, size_t mod_num_words, WordAllocator *allocator);
            static bool   MultMod(Word *dst, const Word *src, const Word *mult, const Word *mod, size_t num_words, WordAllocator *allocator);
            static bool   Mod(Word *dst, const Word *src, size_t src_words, const Word *mod, size_t mod_words, WordAllocator *allocator);
            static bool   DivMod(Word *quot, Word *rem, const Word *top, size_t top_words, const Word *bot, size_t bot_words, WordAllocator *allocator);
            static bool   Mult(Word *dst, const Word *lhs, const Word *rhs, size_t num_words, WordAllocator *allocator);

            static Word   LeftShift(Word *dst, const Word *w, size_t num_words, const size_t shift);
            static Word   RightShift(Word *dst, const Word *w, size_t num_words, const size_t shift);
            static Word   Add(Word *dst, const Word *lhs, const Word *rhs, size_t num_words);
            static Word   Sub(Word *dst, const Word *lhs, const Word *rhs, size_t num_words);
            static Word   MultAdd(Word *dst, const Word *w, size_t num_words, Word mult);
            static Word   MultSub(Word *dst, const Word *w, const Word *v, size_t num_words, Word mult);
    };

    template<size_t Bits>
    class StaticBigNum : public BigNum {
        public:
            static constexpr size_t NumBits  = Bits;
            static constexpr size_t NumWords = util::AlignUp(NumBits, BitsPerWord) / BitsPerWord;
            static constexpr size_t NumBytes = NumWords * sizeof(Word);
        private:
            Word word_buf[NumWords];
        public:
            constexpr StaticBigNum() : word_buf() {
                this->ReserveStatic(word_buf, NumWords);
            }
    };

}
