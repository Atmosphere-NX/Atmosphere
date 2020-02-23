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
#include <vapours.hpp>

namespace ams::crypto::impl {

    namespace {

        constexpr ALWAYS_INLINE BigNum::Word GetTop2Bits(BigNum::Word w) {
            return (w >> (BigNum::BitsPerWord - 2)) & 0x3u;
        }

        constexpr ALWAYS_INLINE void MultWord(BigNum::Word *dst, BigNum::Word lhs, BigNum::Word rhs) {
            static_assert(sizeof(BigNum::DoubleWord) == sizeof(BigNum::Word) * 2);
            BigNum::DoubleWord result = static_cast<BigNum::DoubleWord>(lhs) * static_cast<BigNum::DoubleWord>(rhs);
            dst[0] = static_cast<BigNum::Word>(result & ~BigNum::Word());
            dst[1] = static_cast<BigNum::Word>(result >> BITSIZEOF(BigNum::Word));
        }

        constexpr ALWAYS_INLINE BigNum::HalfWord GetUpperHalf(BigNum::Word word) {
            static_assert(sizeof(BigNum::Word) == sizeof(BigNum::HalfWord) * 2);
            return static_cast<BigNum::HalfWord>((word >> BITSIZEOF(BigNum::HalfWord)) & ~BigNum::HalfWord());
        }

        constexpr ALWAYS_INLINE BigNum::HalfWord GetLowerHalf(BigNum::Word word) {
            static_assert(sizeof(BigNum::Word) == sizeof(BigNum::HalfWord) * 2);
            return static_cast<BigNum::HalfWord>(word & ~BigNum::HalfWord());
        }

        constexpr ALWAYS_INLINE BigNum::Word ToUpperHalf(BigNum::HalfWord half) {
            static_assert(sizeof(BigNum::Word) == sizeof(BigNum::HalfWord) * 2);
            return static_cast<BigNum::Word>(half) << BITSIZEOF(BigNum::HalfWord);
        }

        constexpr ALWAYS_INLINE BigNum::Word ToLowerHalf(BigNum::HalfWord half) {
            static_assert(sizeof(BigNum::Word) == sizeof(BigNum::HalfWord) * 2);
            return static_cast<BigNum::Word>(half);
        }

        constexpr ALWAYS_INLINE BigNum::Word DivWord(const BigNum::Word *w, BigNum::Word div) {
            using Word = BigNum::Word;
            using HalfWord = BigNum::HalfWord;

            Word work[2] = { w[0], w[1] };
            HalfWord r_hi = 0, r_lo = 0;

            HalfWord d_hi = GetUpperHalf(div);
            HalfWord d_lo = GetLowerHalf(div);

            if (d_hi == BigNum::MaxHalfWord) {
                r_hi = GetUpperHalf(work[1]);
            } else {
                r_hi = GetLowerHalf(work[1] / (d_hi + 1));
            }

            {
                const Word hh = static_cast<Word>(r_hi) * static_cast<Word>(d_hi);
                const Word hl = static_cast<Word>(r_hi) * static_cast<Word>(d_lo);

                const Word uhl = ToUpperHalf(static_cast<HalfWord>(hl));
                if ((work[0] -= uhl) > (BigNum::MaxWord - uhl)) {
                    work[1]--;
                }
                work[1] -= GetUpperHalf(hl);
                work[1] -= hh;

                const Word udl = ToUpperHalf(d_lo);
                while (work[1] > d_hi || (work[1] == d_hi && work[0] >= udl)) {
                    if ((work[0] -= udl) > (BigNum::MaxWord - udl)) {
                        work[1]--;
                    }
                    work[1] -= d_hi;
                    r_hi++;
                }
            }

            if (d_hi == BigNum::MaxHalfWord) {
                r_lo = GetLowerHalf(work[1]);
            } else {
                r_lo = GetLowerHalf((ToUpperHalf(static_cast<HalfWord>(work[1])) + GetUpperHalf(work[0])) / (d_hi + 1));
            }

            {
                const Word ll = static_cast<Word>(r_lo) * static_cast<Word>(d_lo);
                const Word lh = static_cast<Word>(r_lo) * static_cast<Word>(d_hi);

                if ((work[0] -= ll) > (BigNum::MaxWord - ll)) {
                    work[1]--;
                }

                const Word ulh = ToUpperHalf(static_cast<HalfWord>(lh));
                if ((work[0] -= ulh) > (BigNum::MaxWord - ulh)) {
                    work[1]--;
                }
                work[1] -= GetUpperHalf(lh);

                while ((work[1] > 0) || (work[1] == 0 && work[0] >= div)) {
                    if ((work[0] -= div) > (BigNum::MaxWord - div)) {
                        work[1]--;
                    }
                    r_lo++;
                }
            }

            return ToUpperHalf(r_hi) + r_lo;
        }

    }

    bool BigNum::IsZero(const Word *w, size_t num_words) {
        for (size_t i = 0; i < num_words; i++) {
            if (w[i]) {
                return false;
            }
        }
        return true;
    }

    int BigNum::Compare(const Word *lhs, const Word *rhs, size_t num_words) {
        for (s32 i = static_cast<s32>(num_words) - 1; i >= 0; i--) {
            if (lhs[i] > rhs[i]) {
                return 1;
            } else if (lhs[i] < rhs[i]) {
                return -1;
            }
        }
        return 0;
    }

    size_t BigNum::CountWords(const Word *w, size_t num_words) {
        s32 i = static_cast<s32>(num_words) - 1;
        while (i >= 0 && !w[i]) {
            i--;
        }
        return i + 1;
    }

    size_t BigNum::CountSignificantBits(Word w) {
        size_t i;
        for (i = 0; i < BitsPerWord && w != 0; i++) {
            w >>= 1;
        }
        return i;
    }

    void BigNum::ClearToZero(Word *w, size_t num_words) {
        for (size_t i = 0; i < num_words; i++) {
            w[i] = 0;
        }
    }

    void BigNum::SetToWord(Word *w, size_t num_words, Word v) {
        ClearToZero(w, num_words);
        w[0] = v;
    }

    void BigNum::Copy(Word *dst, const Word *src, size_t num_words) {
        for (size_t i = 0; i < num_words; i++) {
            dst[i] = src[i];
        }
    }

    BigNum::Word BigNum::LeftShift(Word *dst, const Word *w, size_t num_words, const size_t shift) {
        if (shift >= BitsPerWord) {
            return 0;
        }

        const size_t invshift = BitsPerWord - shift;
        Word carry = 0;
        for (size_t i = 0; i < num_words; i++) {
            const Word cur = w[i];
            dst[i] = (cur << shift) | carry;
            carry = shift ? (cur >> invshift) : 0;
        }

        return carry;
    }

    BigNum::Word BigNum::RightShift(Word *dst, const Word *w, size_t num_words, const size_t shift) {
        if (shift >= BitsPerWord) {
            return 0;
        }

        const size_t invshift = BitsPerWord - shift;
        Word carry = 0;
        for (s32 i = static_cast<s32>(num_words) - 1; i >= 0; i--) {
            const Word cur = w[i];
            dst[i] = (cur >> shift) | carry;
            carry = shift ? (cur << invshift) : 0;
        }

        return carry;
    }

    BigNum::Word BigNum::MultSub(Word *dst, const Word *w, const Word *v, size_t num_words, Word mult) {
        /* If multiplying by zero, nothing to do. */
        if (mult == 0) {
            return 0;
        }

        Word borrow = 0, work[2];
        for (size_t i = 0; i < num_words; i++) {
            /* Multiply, calculate borrow for next. */
            MultWord(work, mult, v[i]);
            if ((dst[i] = (w[i] - borrow)) > (MaxWord - borrow)) {
                borrow = 1;
            } else {
                borrow = 0;
            }

            if ((dst[i] -= work[0]) > (MaxWord - work[0])) {
                borrow++;
            }
            borrow += work[1];
        }

        return borrow;
    }

    bool BigNum::ExpMod(Word *dst, const Word *src, const Word *exp, size_t exp_words, const Word *mod, size_t mod_words, WordAllocator *allocator) {
        /* Nintendo uses an algorithm that relies on powers of exp. */
        bool needs_exp[4] = {};
        if (exp_words > 1) {
            needs_exp[2] = true;
            needs_exp[3] = true;
        } else {
            Word exp_w = exp[0];

            for (size_t i = 0; i < BitsPerWord / 2; i++) {
                /* Nintendo at each step determines needed exponent from a pair of two bits. */
                needs_exp[exp_w & 0x3u] = true;
                exp_w >>= 2;
            }

            if (needs_exp[3]) {
                needs_exp[2] = true;
            }
        }

        /* Allocate space for powers 1, 2, 3. */
        auto power_1 = allocator->Allocate(mod_words);
        auto power_2 = allocator->Allocate(mod_words);
        auto power_3 = allocator->Allocate(mod_words);
        if (!(power_1.IsValid() && power_2.IsValid() && power_3.IsValid())) {
            return false;
        }
        decltype(power_1)* powers[3] = { &power_1, &power_2, &power_3 };

        /* Set the powers of src. */
        Copy(power_1.GetBuffer(), src, mod_words);
        if (needs_exp[2]) {
            if (!MultMod(power_2.GetBuffer(), power_1.GetBuffer(), src, mod, mod_words, allocator)) {
                return false;
            }
        }
        if (needs_exp[3]) {
            if (!MultMod(power_3.GetBuffer(), power_2.GetBuffer(), src, mod, mod_words, allocator)) {
                return false;
            }
        }

        /* Allocate space to work. */
        auto work = allocator->Allocate(mod_words);
        if (!work.IsValid()) {
            return false;
        }
        SetToWord(work.GetBuffer(), work.GetCount(), 1);

        /* Ensure we're working with the correct exponent word count. */
        exp_words = CountWords(exp, exp_words);

        for (s32 i = static_cast<s32>(exp_words - 1); i >= 0; i--) {
            Word cur_word = exp[i];
            size_t cur_bits = BitsPerWord;

            /* Remove leading zeroes in first word. */
            if (i == static_cast<s32>(exp_words - 1)) {
                while (!GetTop2Bits(cur_word)) {
                    cur_word <<= 2;
                    cur_bits -= 2;
                }
            }

            /* Compute current modular multiplicative step. */
            for (size_t j = 0; j < cur_bits; j += 2, cur_word <<= 2) {
                /* Exponentiate current work to the 4th power. */
                if (!MultMod(work.GetBuffer(), work.GetBuffer(), work.GetBuffer(), mod, mod_words, allocator)) {
                    return false;
                }

                if (!MultMod(work.GetBuffer(), work.GetBuffer(), work.GetBuffer(), mod, mod_words, allocator)) {
                    return false;
                }

                if (const Word top = GetTop2Bits(cur_word)) {
                    if (!MultMod(work.GetBuffer(), work.GetBuffer(), powers[top - 1]->GetBuffer(), mod, mod_words, allocator)) {
                        return false;
                    }
                }
            }
        }

        /* Copy work to output. */
        Copy(dst, work.GetBuffer(), mod_words);

        return true;
    }

    bool BigNum::MultMod(Word *dst, const Word *src, const Word *mult, const Word *mod, size_t num_words, WordAllocator *allocator) {
        /* Allocate work. */
        auto work = allocator->Allocate(2 * num_words);
        if (!work.IsValid()) {
            return false;
        }

        /* Multiply. */
        if (!Mult(work.GetBuffer(), src, mult, num_words, allocator)) {
            return false;
        }

        /* Mod. */
        if (!Mod(dst, work.GetBuffer(), 2 * num_words, mod, num_words, allocator)) {
            return false;
        }

        return true;
    }

    bool BigNum::Mod(Word *dst, const Word *src, size_t src_words, const Word *mod, size_t mod_words, WordAllocator *allocator) {
        /* Allocate work. */
        auto work = allocator->Allocate(src_words);
        if (!work.IsValid()) {
            return false;
        }

        if (!DivMod(work.GetBuffer(), dst, src, src_words, mod, mod_words, allocator)) {
            return false;
        }

        return true;
    }

    bool BigNum::DivMod(Word *quot, Word *rem, const Word *top, size_t top_words, const Word *bot, size_t bot_words, WordAllocator *allocator) {
        /* Allocate work. */
        auto top_work = allocator->Allocate(top_words + 1);
        auto bot_work = allocator->Allocate(bot_words);
        if (!(top_work.IsValid() && bot_work.IsValid())) {
            return false;
        }

        /* Prevent division by zero. */
        size_t bot_work_words = CountWords(bot, bot_words);
        if (bot_work_words == 0) {
            return false;
        }

        ClearToZero(quot, top_words);
        ClearToZero(top_work.GetBuffer(), bot_work_words);

        /* Align to edges. */
        const size_t shift = BitsPerWord - CountSignificantBits(bot[bot_work_words - 1]);
        top_work.GetBuffer()[top_words] = LeftShift(top_work.GetBuffer(), top, top_words, shift);
        LeftShift(bot_work.GetBuffer(), bot, bot_work_words, shift);
        const Word tb = bot_work.GetBuffer()[bot_work_words - 1];

        /* Repeatedly div + sub. */
        for (s32 i = (top_words - bot_work_words); i >= 0; i--) {
            Word cur_word;
            if (tb == MaxWord) {
                cur_word = top_work.GetBuffer()[i + bot_work_words];
            } else {
                cur_word = DivWord(top_work.GetBuffer() + i + bot_work_words - 1, tb + 1);
            }
            top_work.GetBuffer()[i + bot_work_words] -= MultSub(top_work.GetBuffer() + i, top_work.GetBuffer() + i, bot_work.GetBuffer(), bot_work_words, cur_word);

            while (top_work.GetBuffer()[i + bot_work_words] || Compare(top_work.GetBuffer() + i, bot_work.GetBuffer(), bot_work_words) >= 0) {
                cur_word++;
                top_work.GetBuffer()[i + bot_work_words] -= Sub(top_work.GetBuffer() + i, top_work.GetBuffer() + i, bot_work.GetBuffer(), bot_work_words);
            }
            quot[i] = cur_word;
        }

        /* Calculate remainder. */
        ClearToZero(rem, bot_words);
        RightShift(rem, top_work.GetBuffer(), bot_work_words, shift);

        return true;
    }

    bool BigNum::Mult(Word *dst, const Word *lhs, const Word *rhs, size_t num_words, WordAllocator *allocator) {
        /* Allocate work. */
        auto work = allocator->Allocate(2 * num_words);
        if (!work.IsValid()) {
            return false;
        }
        ClearToZero(work.GetBuffer(), work.GetCount());

        /* Repeatedly add and multiply. */
        const size_t lhs_words = CountWords(lhs, num_words);
        const size_t rhs_words = CountWords(rhs, num_words);

        for (size_t i = 0; i < lhs_words; i++) {
            work.GetBuffer()[i + rhs_words] += MultAdd(work.GetBuffer() + i, rhs, rhs_words, lhs[i]);
        }

        /* Copy to output. */
        Copy(dst, work.GetBuffer(), work.GetCount());

        return true;
    }

}