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
                            WordAllocator *m_allocator;
                            Word *m_buffer;
                            size_t m_count;
                        private:
                            constexpr ALWAYS_INLINE Allocation(WordAllocator *a, Word *w, size_t c) : m_allocator(a), m_buffer(w), m_count(c) { /* ... */ }
                        public:
                            ALWAYS_INLINE ~Allocation() { if (m_allocator) { m_allocator->Free(m_buffer, m_count); } }

                            constexpr ALWAYS_INLINE Word *GetBuffer() const { return m_buffer; }
                            constexpr ALWAYS_INLINE size_t GetCount() const { return m_count; }
                            constexpr ALWAYS_INLINE bool IsValid() const { return m_buffer != nullptr; }
                    };

                    friend class Allocation;
                private:
                    Word *m_buffer;
                    size_t m_count;
                    size_t m_max_count;
                    size_t m_min_count;
                private:
                    ALWAYS_INLINE void Free(void *words, size_t num) {
                        m_buffer -= num;
                        m_count  += num;

                        AMS_ASSERT(words == m_buffer);
                        AMS_UNUSED(words);
                    }
                public:
                    constexpr ALWAYS_INLINE WordAllocator(Word *buf, size_t c) : m_buffer(buf), m_count(c), m_max_count(c), m_min_count(c) { /* ... */ }

                    ALWAYS_INLINE Allocation Allocate(size_t num) {
                        if (num <= m_count) {
                            Word *allocated = m_buffer;

                            m_buffer += num;
                            m_count -= num;
                            m_min_count = std::min(m_count, m_min_count);

                            return Allocation(this, allocated, num);
                        } else {
                            return Allocation(nullptr, nullptr, 0);
                        }
                    }

                    constexpr ALWAYS_INLINE size_t GetMaxUsedSize() const {
                        return (m_max_count - m_min_count) * sizeof(Word);
                    }
            };
        private:
            Word *m_words;
            size_t m_num_words;
            size_t m_max_words;
        private:
            static void ImportImpl(Word *out, size_t out_size, const u8 *src, size_t src_size);
            static void ExportImpl(u8 *out, size_t out_size, const Word *src, size_t src_size);
        public:
            constexpr BigNum() : m_words(), m_num_words(), m_max_words() { /* ... */ }
            ~BigNum() { /* ... */ }

            constexpr void ReserveStatic(Word *buf, size_t capacity) {
                m_words = buf;
                m_max_words = capacity;
            }

            bool Import(const void *src, size_t src_size);
            void Export(void *dst, size_t dst_size);

            size_t GetSize() const;

            bool IsZero() const {
                return m_num_words == 0;
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
            Word m_word_buf[NumWords];
        public:
            constexpr StaticBigNum() : m_word_buf() {
                this->ReserveStatic(m_word_buf, NumWords);
            }
    };

}
