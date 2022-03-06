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
#include <vapours.hpp>
#include "crypto_aes_impl.arch.x64.hpp"

namespace ams::crypto::impl {

    template<> void CtrModeImpl<AesEncryptor128>::ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Check pre-conditions. */
        AMS_ASSERT(src != nullptr);
        AMS_ASSERT(dst != nullptr);

        /* If we have aes-ni, use an optimized impl. */
        if (IsAesNiAvailable()) {
            /* Load all keys into sse2 registers. */
            const u8 *raw_round_keys = m_block_cipher->GetRoundKey();
            const __m128i round_keys[AesEncryptor128::RoundKeySize / BlockSize] = {
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  0)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  1)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  2)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  3)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  4)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  5)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  6)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  7)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  8)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize *  9)),
                _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw_round_keys + BlockSize * 10)),
            };
            static_assert(AesEncryptor128::RoundKeySize / BlockSize == 11);

            /* Declare constant for counter math. */
            const __m128i One = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);

            /* Process eight blocks at a time, while we can. */
            constexpr const auto UnrolledBlockCount = 8;
            constexpr const auto CounterThreshold   = static_cast<u8>(0x100 - UnrolledBlockCount);

            /* Load the counter. */
            auto counter = _mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter));

            size_t cur_blocks;
            for (cur_blocks = 0; cur_blocks + UnrolledBlockCount <= num_blocks; cur_blocks += UnrolledBlockCount) {
                __m128i b0;
                __m128i b1;
                __m128i b2;
                __m128i b3;
                __m128i b4;
                __m128i b5;
                __m128i b6;
                __m128i b7;

                __m128i key = round_keys[0];

                /* Get the last byte of the block. */
                static_assert(util::IsLittleEndian());
                const u8 counter_val = _mm_extract_epi16(counter, 7) >> BITSIZEOF(u8);

                /* Do initial encryption of each block. */
                if (CounterThreshold <= counter_val) {
                    /* We'll overwrap, so take slow path for counter. */
                    _mm_storeu_si128(reinterpret_cast<__m128i *>(m_counter), counter);

                    b0 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter)), key);
                    this->IncrementCounter();
                    b1 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter)), key);
                    this->IncrementCounter();
                    b2 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter)), key);
                    this->IncrementCounter();
                    b3 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter)), key);
                    this->IncrementCounter();
                    b4 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter)), key);
                    this->IncrementCounter();
                    b5 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter)), key);
                    this->IncrementCounter();
                    b6 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter)), key);
                    this->IncrementCounter();
                    b7 = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter)), key);
                    this->IncrementCounter();

                    counter = _mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter));
                } else {
                    /* We can take the fast path for the counter. */
                    b0 = _mm_xor_si128(counter, key);
                    counter = _mm_add_epi64(counter, One);
                    b1 = _mm_xor_si128(counter, key);
                    counter = _mm_add_epi64(counter, One);
                    b2 = _mm_xor_si128(counter, key);
                    counter = _mm_add_epi64(counter, One);
                    b3 = _mm_xor_si128(counter, key);
                    counter = _mm_add_epi64(counter, One);
                    b4 = _mm_xor_si128(counter, key);
                    counter = _mm_add_epi64(counter, One);
                    b5 = _mm_xor_si128(counter, key);
                    counter = _mm_add_epi64(counter, One);
                    b6 = _mm_xor_si128(counter, key);
                    counter = _mm_add_epi64(counter, One);
                    b7 = _mm_xor_si128(counter, key);
                    counter = _mm_add_epi64(counter, One);
                }

                /* Do encryption for all rounds. */
                key = round_keys[1];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[2];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[3];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[4];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[5];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[6];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[7];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[8];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[9];
                b0 = _mm_aesenc_si128(b0, key);
                b1 = _mm_aesenc_si128(b1, key);
                b2 = _mm_aesenc_si128(b2, key);
                b3 = _mm_aesenc_si128(b3, key);
                b4 = _mm_aesenc_si128(b4, key);
                b5 = _mm_aesenc_si128(b5, key);
                b6 = _mm_aesenc_si128(b6, key);
                b7 = _mm_aesenc_si128(b7, key);

                key = round_keys[10];
                b0 = _mm_aesenclast_si128(b0, key);
                b1 = _mm_aesenclast_si128(b1, key);
                b2 = _mm_aesenclast_si128(b2, key);
                b3 = _mm_aesenclast_si128(b3, key);
                b4 = _mm_aesenclast_si128(b4, key);
                b5 = _mm_aesenclast_si128(b5, key);
                b6 = _mm_aesenclast_si128(b6, key);
                b7 = _mm_aesenclast_si128(b7, key);

                /* Write the blocks. */
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + BlockSize * 0), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src + BlockSize * 0)), b0));
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + BlockSize * 1), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src + BlockSize * 1)), b1));
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + BlockSize * 2), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src + BlockSize * 2)), b2));
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + BlockSize * 3), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src + BlockSize * 3)), b3));
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + BlockSize * 4), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src + BlockSize * 4)), b4));
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + BlockSize * 5), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src + BlockSize * 5)), b5));
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + BlockSize * 6), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src + BlockSize * 6)), b6));
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + BlockSize * 7), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src + BlockSize * 7)), b7));

                src += BlockSize * UnrolledBlockCount;
                dst += BlockSize * UnrolledBlockCount;
            }

            /* Store the updated counter. */
            _mm_storeu_si128(reinterpret_cast<__m128i *>(m_counter), counter);

            /* Process blocks one at a time. */
            for (/* ... */; cur_blocks < num_blocks; ++cur_blocks) {
                /* Load current counter. */
                __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(m_counter));

                /* Do aes rounds. */
                b = _mm_xor_si128(b, round_keys[0]);
                b = _mm_aesenc_si128(b, round_keys[1]);
                b = _mm_aesenc_si128(b, round_keys[2]);
                b = _mm_aesenc_si128(b, round_keys[3]);
                b = _mm_aesenc_si128(b, round_keys[4]);
                b = _mm_aesenc_si128(b, round_keys[5]);
                b = _mm_aesenc_si128(b, round_keys[6]);
                b = _mm_aesenc_si128(b, round_keys[7]);
                b = _mm_aesenc_si128(b, round_keys[8]);
                b = _mm_aesenc_si128(b, round_keys[9]);
                b = _mm_aesenclast_si128(b, round_keys[10]);

                /* Write the block. */
                _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src)), b));

                /* Advance. */
                src += BlockSize;
                dst += BlockSize;
                this->IncrementCounter();
            }
        } else {
            /* Fall back to the default implementation. */
            while (num_blocks--) {
                this->ProcessBlock(dst, src, BlockSize);
                dst += BlockSize;
                src += BlockSize;
            }
        }
    }

}
