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

#ifdef ATMOSPHERE_IS_STRATOSPHERE
#include <arm_neon.h>

namespace ams::crypto::impl {

    /* Variable management macros. */
    #define DECLARE_ROUND_KEY_VAR(n) \
    const uint8x16_t round_key_##n = vld1q_u8(keys + (BlockSize * n))

    #define AES_ENC_DEC_OUTPUT_THREE_BLOCKS() \
    [tmp0]"+w"(tmp0), [tmp1]"+w"(tmp1), [tmp2]"+w"(tmp2)

    #define AES_ENC_DEC_OUTPUT_THREE_CTRS() \
    [ctr0]"+w"(ctr0), [ctr1]"+w"(ctr1), [ctr2]"+w"(ctr2)

    #define AES_ENC_DEC_OUTPUT_ONE_BLOCK() \
    [tmp0]"+w"(tmp0)

    #define AES_ENC_DEC_OUTPUT_ONE_CTR() \
    [ctr0]"+w"(ctr0)

    #define CTR_INCREMENT_OUTPUT_HIGH_LOW() \
    [high]"=&r"(high), [low]"=&r"(low)

    #define CTR_INCREMENT_OUTPUT_HIGH_LOW_TMP() \
    [high_tmp]"=&r"(high_tmp), [low_tmp]"=&r"(low_tmp)

    #define CTR_INCREMENT_OUTPUT_HL_SINGLE_TMP() \
    [hl_tmp]"=&r"(hl_tmp)

    #define AES_ENC_DEC_INPUT_ROUND_KEY(n) \
    [round_key_##n]"w"(round_key_##n)

    /* AES Encryption macros. */
    #define AES_ENC_ROUND(n, i) \
    "aese %[tmp" #i "].16b, %[round_key_" #n "].16b\n" \
    "aesmc %[tmp" #i "].16b, %[tmp" #i "].16b\n"

    #define AES_ENC_SECOND_LAST_ROUND(n, i) \
    "aese %[tmp" #i "].16b, %[round_key_" #n "].16b\n"

    #define AES_ENC_LAST_ROUND(n, i) \
    "eor %[tmp" #i "].16b, %[tmp" #i "].16b, %[round_key_" #n "].16b\n"

    namespace {

        ALWAYS_INLINE uint8x16_t IncrementCounterOptimized(const uint8x16_t ctr) {
            uint8x16_t inc;
            uint64_t high, low;
            /* Use ASM. TODO: Better than using intrinsics? */
            __asm__ __volatile__ (
                "mov %[high], %[ctr].d[0]\n"
                "mov %[low], %[ctr].d[1]\n"
                "rev %[high], %[high]\n"
                "rev %[low], %[low]\n"
                "adds %[low], %[low], 1\n"
                "cinc %[high], %[high], cs\n"
                "rev %[high], %[high]\n"
                "rev %[low], %[low]\n"
                "mov %[inc].d[0], %[high]\n"
                "mov %[inc].d[1], %[low]\n"
                : [inc]"=w"(inc),
                  CTR_INCREMENT_OUTPUT_HIGH_LOW()
                : [ctr]"w"(ctr)
                : "cc"
            );
            return inc;
        }

    }

    template<>
    void CtrModeImpl<AesEncryptor128>::ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = m_block_cipher->GetRoundKey();
        DECLARE_ROUND_KEY_VAR(0);
        DECLARE_ROUND_KEY_VAR(1);
        DECLARE_ROUND_KEY_VAR(2);
        DECLARE_ROUND_KEY_VAR(3);
        DECLARE_ROUND_KEY_VAR(4);
        DECLARE_ROUND_KEY_VAR(5);
        DECLARE_ROUND_KEY_VAR(6);
        DECLARE_ROUND_KEY_VAR(7);
        DECLARE_ROUND_KEY_VAR(8);
        DECLARE_ROUND_KEY_VAR(9);
        DECLARE_ROUND_KEY_VAR(10);
        uint8x16_t ctr0 = vld1q_u8(m_counter);
        uint64_t high, low;

        /* Process three blocks at a time, when possible. */
        if (num_blocks >= 3) {
            /* Increment CTR twice. */
            uint8x16_t ctr1 = IncrementCounterOptimized(ctr0);
            uint8x16_t ctr2 = IncrementCounterOptimized(ctr1);
            uint64_t high_tmp, low_tmp;

            while (num_blocks >= 3) {
                /* Read blocks in. Keep them in registers for XOR later. */
                const uint8x16_t block0 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;
                const uint8x16_t block1 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;
                const uint8x16_t block2 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;

                /* We'll be encrypting the three CTRs. */
                uint8x16_t tmp0 = ctr0, tmp1 = ctr1, tmp2 = ctr2;

                /* Actually do encryption, use optimized asm. */
                /* Interleave CTR calculations with AES ones, to mask latencies. */
                __asm__ __volatile__ (
                    AES_ENC_ROUND(0, 0) "mov %[high], %[ctr2].d[0]\n"
                    AES_ENC_ROUND(0, 1) "mov %[low], %[ctr2].d[1]\n"
                    AES_ENC_ROUND(0, 2) "rev %[high], %[high]\n"
                    AES_ENC_ROUND(1, 0) "rev %[low], %[low]\n"
                    AES_ENC_ROUND(1, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(1, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(2, 0) "rev %[high_tmp], %[high]\n"
                    AES_ENC_ROUND(2, 1) "rev %[low_tmp], %[low]\n"
                    AES_ENC_ROUND(2, 2) "mov %[ctr0].d[0], %[high_tmp]\n"
                    AES_ENC_ROUND(3, 0) "mov %[ctr0].d[1], %[low_tmp]\n"
                    AES_ENC_ROUND(3, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(3, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(4, 0) "rev %[high_tmp], %[high]\n"
                    AES_ENC_ROUND(4, 1) "rev %[low_tmp], %[low]\n"
                    AES_ENC_ROUND(4, 2) "mov %[ctr1].d[0], %[high_tmp]\n"
                    AES_ENC_ROUND(5, 0) "mov %[ctr1].d[1], %[low_tmp]\n"
                    AES_ENC_ROUND(5, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(5, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(6, 0) "rev %[high_tmp], %[high]\n"
                    AES_ENC_ROUND(6, 1) "rev %[low_tmp], %[low]\n"
                    AES_ENC_ROUND(6, 2) "mov %[ctr2].d[0], %[high_tmp]\n"
                    AES_ENC_ROUND(7, 0) "mov %[ctr2].d[1], %[low_tmp]\n"
                    AES_ENC_ROUND(7, 1)
                    AES_ENC_ROUND(7, 2)
                    AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                    AES_ENC_SECOND_LAST_ROUND(9, 0) AES_ENC_SECOND_LAST_ROUND(9, 1) AES_ENC_SECOND_LAST_ROUND(9, 2)
                    AES_ENC_LAST_ROUND(10, 0) AES_ENC_LAST_ROUND(10, 1) AES_ENC_LAST_ROUND(10, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_CTRS(),
                      CTR_INCREMENT_OUTPUT_HIGH_LOW(),
                      CTR_INCREMENT_OUTPUT_HIGH_LOW_TMP()
                    : AES_ENC_DEC_INPUT_ROUND_KEY(0),
                      AES_ENC_DEC_INPUT_ROUND_KEY(1),
                      AES_ENC_DEC_INPUT_ROUND_KEY(2),
                      AES_ENC_DEC_INPUT_ROUND_KEY(3),
                      AES_ENC_DEC_INPUT_ROUND_KEY(4),
                      AES_ENC_DEC_INPUT_ROUND_KEY(5),
                      AES_ENC_DEC_INPUT_ROUND_KEY(6),
                      AES_ENC_DEC_INPUT_ROUND_KEY(7),
                      AES_ENC_DEC_INPUT_ROUND_KEY(8),
                      AES_ENC_DEC_INPUT_ROUND_KEY(9),
                      AES_ENC_DEC_INPUT_ROUND_KEY(10)
                    : "cc"
                );

                /* XOR blocks. */
                tmp0 = veorq_u8(block0, tmp0);
                tmp1 = veorq_u8(block1, tmp1);
                tmp2 = veorq_u8(block2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0);
                dst += AES_BLOCK_SIZE;
                vst1q_u8(dst, tmp1);
                dst += AES_BLOCK_SIZE;
                vst1q_u8(dst, tmp2);
                dst += AES_BLOCK_SIZE;

                num_blocks -= 3;
            }
        }

        while (num_blocks >= 1) {
            /* Read block in, keep in register for XOR. */
            const uint8x16_t block0 = vld1q_u8(src);
            src += AES_BLOCK_SIZE;

            /* We'll be encrypting the CTR. */
            uint8x16_t tmp0 = ctr0;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0)             "mov %[high], %[ctr0].d[0]\n"
                AES_ENC_ROUND(1, 0)             "mov %[low], %[ctr0].d[1]\n"
                AES_ENC_ROUND(2, 0)             "rev %[high], %[high]\n"
                AES_ENC_ROUND(3, 0)             "rev %[low], %[low]\n"
                AES_ENC_ROUND(4, 0)             "adds %[low], %[low], 1\n"
                AES_ENC_ROUND(5, 0)             "cinc %[high], %[high], cs\n"
                AES_ENC_ROUND(6, 0)             "rev %[high], %[high]\n"
                AES_ENC_ROUND(7, 0)             "rev %[low], %[low]\n"
                AES_ENC_ROUND(8, 0)             "mov %[ctr0].d[0], %[high]\n"
                AES_ENC_SECOND_LAST_ROUND(9, 0) "mov %[ctr0].d[1], %[low]\n"
                AES_ENC_LAST_ROUND(10, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_CTR(),
                  CTR_INCREMENT_OUTPUT_HIGH_LOW()
                : AES_ENC_DEC_INPUT_ROUND_KEY(0),
                  AES_ENC_DEC_INPUT_ROUND_KEY(1),
                  AES_ENC_DEC_INPUT_ROUND_KEY(2),
                  AES_ENC_DEC_INPUT_ROUND_KEY(3),
                  AES_ENC_DEC_INPUT_ROUND_KEY(4),
                  AES_ENC_DEC_INPUT_ROUND_KEY(5),
                  AES_ENC_DEC_INPUT_ROUND_KEY(6),
                  AES_ENC_DEC_INPUT_ROUND_KEY(7),
                  AES_ENC_DEC_INPUT_ROUND_KEY(8),
                  AES_ENC_DEC_INPUT_ROUND_KEY(9),
                  AES_ENC_DEC_INPUT_ROUND_KEY(10)
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(block0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += AES_BLOCK_SIZE;

            num_blocks--;
        }

        vst1q_u8(m_counter, ctr0);
    }

    template<>
    void CtrModeImpl<AesEncryptor192>::ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = m_block_cipher->GetRoundKey();
        DECLARE_ROUND_KEY_VAR(0);
        DECLARE_ROUND_KEY_VAR(1);
        DECLARE_ROUND_KEY_VAR(2);
        DECLARE_ROUND_KEY_VAR(3);
        DECLARE_ROUND_KEY_VAR(4);
        DECLARE_ROUND_KEY_VAR(5);
        DECLARE_ROUND_KEY_VAR(6);
        DECLARE_ROUND_KEY_VAR(7);
        DECLARE_ROUND_KEY_VAR(8);
        DECLARE_ROUND_KEY_VAR(9);
        DECLARE_ROUND_KEY_VAR(10);
        DECLARE_ROUND_KEY_VAR(11);
        DECLARE_ROUND_KEY_VAR(12);
        uint8x16_t ctr0 = vld1q_u8(m_counter);
        uint64_t high, low;

        /* Process three blocks at a time, when possible. */
        if (num_blocks >= 3) {
            /* Increment CTR twice. */
            uint8x16_t ctr1 = IncrementCounterOptimized(ctr0);
            uint8x16_t ctr2 = IncrementCounterOptimized(ctr1);
            uint64_t high_tmp, low_tmp;

            while (num_blocks >= 3) {
                /* Read blocks in. Keep them in registers for XOR later. */
                const uint8x16_t block0 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;
                const uint8x16_t block1 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;
                const uint8x16_t block2 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;

                /* We'll be encrypting the three CTRs. */
                uint8x16_t tmp0 = ctr0, tmp1 = ctr1, tmp2 = ctr2;

                /* Actually do encryption, use optimized asm. */
                /* Interleave CTR calculations with AES ones, to mask latencies. */
                __asm__ __volatile__ (
                    AES_ENC_ROUND(0, 0) "mov %[high], %[ctr2].d[0]\n"
                    AES_ENC_ROUND(0, 1) "mov %[low], %[ctr2].d[1]\n"
                    AES_ENC_ROUND(0, 2) "rev %[high], %[high]\n"
                    AES_ENC_ROUND(1, 0) "rev %[low], %[low]\n"
                    AES_ENC_ROUND(1, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(1, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(2, 0) "rev %[high_tmp], %[high]\n"
                    AES_ENC_ROUND(2, 1) "rev %[low_tmp], %[low]\n"
                    AES_ENC_ROUND(2, 2) "mov %[ctr0].d[0], %[high_tmp]\n"
                    AES_ENC_ROUND(3, 0) "mov %[ctr0].d[1], %[low_tmp]\n"
                    AES_ENC_ROUND(3, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(3, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(4, 0) "rev %[high_tmp], %[high]\n"
                    AES_ENC_ROUND(4, 1) "rev %[low_tmp], %[low]\n"
                    AES_ENC_ROUND(4, 2) "mov %[ctr1].d[0], %[high_tmp]\n"
                    AES_ENC_ROUND(5, 0) "mov %[ctr1].d[1], %[low_tmp]\n"
                    AES_ENC_ROUND(5, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(5, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(6, 0) "rev %[high_tmp], %[high]\n"
                    AES_ENC_ROUND(6, 1) "rev %[low_tmp], %[low]\n"
                    AES_ENC_ROUND(6, 2) "mov %[ctr2].d[0], %[high_tmp]\n"
                    AES_ENC_ROUND(7, 0) "mov %[ctr2].d[1], %[low_tmp]\n"
                    AES_ENC_ROUND(7, 1)
                    AES_ENC_ROUND(7, 2)
                    AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                    AES_ENC_ROUND(9, 0) AES_ENC_ROUND(9, 1) AES_ENC_ROUND(9, 2)
                    AES_ENC_ROUND(10, 0) AES_ENC_ROUND(10, 1) AES_ENC_ROUND(10, 2)
                    AES_ENC_SECOND_LAST_ROUND(11, 0) AES_ENC_SECOND_LAST_ROUND(11, 1) AES_ENC_SECOND_LAST_ROUND(11, 2)
                    AES_ENC_LAST_ROUND(12, 0) AES_ENC_LAST_ROUND(12, 1) AES_ENC_LAST_ROUND(12, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_CTRS(),
                      CTR_INCREMENT_OUTPUT_HIGH_LOW(),
                      CTR_INCREMENT_OUTPUT_HIGH_LOW_TMP()
                    : AES_ENC_DEC_INPUT_ROUND_KEY(0),
                      AES_ENC_DEC_INPUT_ROUND_KEY(1),
                      AES_ENC_DEC_INPUT_ROUND_KEY(2),
                      AES_ENC_DEC_INPUT_ROUND_KEY(3),
                      AES_ENC_DEC_INPUT_ROUND_KEY(4),
                      AES_ENC_DEC_INPUT_ROUND_KEY(5),
                      AES_ENC_DEC_INPUT_ROUND_KEY(6),
                      AES_ENC_DEC_INPUT_ROUND_KEY(7),
                      AES_ENC_DEC_INPUT_ROUND_KEY(8),
                      AES_ENC_DEC_INPUT_ROUND_KEY(9),
                      AES_ENC_DEC_INPUT_ROUND_KEY(10),
                      AES_ENC_DEC_INPUT_ROUND_KEY(11),
                      AES_ENC_DEC_INPUT_ROUND_KEY(12)
                    : "cc"
                );

                /* XOR blocks. */
                tmp0 = veorq_u8(block0, tmp0);
                tmp1 = veorq_u8(block1, tmp1);
                tmp2 = veorq_u8(block2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0);
                dst += AES_BLOCK_SIZE;
                vst1q_u8(dst, tmp1);
                dst += AES_BLOCK_SIZE;
                vst1q_u8(dst, tmp2);
                dst += AES_BLOCK_SIZE;

                num_blocks -= 3;
            }
        }

        while (num_blocks >= 1) {
            /* Read block in, keep in register for XOR. */
            const uint8x16_t block0 = vld1q_u8(src);
            src += AES_BLOCK_SIZE;

            /* We'll be encrypting the CTR. */
            uint8x16_t tmp0 = ctr0;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0) "mov %[high], %[ctr0].d[0]\n"
                AES_ENC_ROUND(1, 0) "mov %[low], %[ctr0].d[1]\n"
                AES_ENC_ROUND(2, 0) "rev %[high], %[high]\n"
                AES_ENC_ROUND(3, 0) "rev %[low], %[low]\n"
                AES_ENC_ROUND(4, 0) "adds %[low], %[low], 1\n"
                AES_ENC_ROUND(5, 0) "cinc %[high], %[high], cs\n"
                AES_ENC_ROUND(6, 0) "rev %[high], %[high]\n"
                AES_ENC_ROUND(7, 0) "rev %[low], %[low]\n"
                AES_ENC_ROUND(8, 0) "mov %[ctr0].d[0], %[high]\n"
                AES_ENC_ROUND(9, 0) "mov %[ctr0].d[1], %[low]\n"
                AES_ENC_ROUND(10, 0)
                AES_ENC_SECOND_LAST_ROUND(11, 0)
                AES_ENC_LAST_ROUND(12, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_CTR(),
                  CTR_INCREMENT_OUTPUT_HIGH_LOW()
                : AES_ENC_DEC_INPUT_ROUND_KEY(0),
                  AES_ENC_DEC_INPUT_ROUND_KEY(1),
                  AES_ENC_DEC_INPUT_ROUND_KEY(2),
                  AES_ENC_DEC_INPUT_ROUND_KEY(3),
                  AES_ENC_DEC_INPUT_ROUND_KEY(4),
                  AES_ENC_DEC_INPUT_ROUND_KEY(5),
                  AES_ENC_DEC_INPUT_ROUND_KEY(6),
                  AES_ENC_DEC_INPUT_ROUND_KEY(7),
                  AES_ENC_DEC_INPUT_ROUND_KEY(8),
                  AES_ENC_DEC_INPUT_ROUND_KEY(9),
                  AES_ENC_DEC_INPUT_ROUND_KEY(10),
                  AES_ENC_DEC_INPUT_ROUND_KEY(11),
                  AES_ENC_DEC_INPUT_ROUND_KEY(12)
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(block0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += AES_BLOCK_SIZE;

            num_blocks--;
        }

        vst1q_u8(m_counter, ctr0);
    }

    template<>
    void CtrModeImpl<AesEncryptor256>::ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = m_block_cipher->GetRoundKey();
        DECLARE_ROUND_KEY_VAR(0);
        DECLARE_ROUND_KEY_VAR(1);
        DECLARE_ROUND_KEY_VAR(2);
        DECLARE_ROUND_KEY_VAR(3);
        DECLARE_ROUND_KEY_VAR(4);
        DECLARE_ROUND_KEY_VAR(5);
        DECLARE_ROUND_KEY_VAR(6);
        DECLARE_ROUND_KEY_VAR(7);
        DECLARE_ROUND_KEY_VAR(8);
        DECLARE_ROUND_KEY_VAR(9);
        DECLARE_ROUND_KEY_VAR(10);
        DECLARE_ROUND_KEY_VAR(11);
        DECLARE_ROUND_KEY_VAR(12);
        DECLARE_ROUND_KEY_VAR(13);
        DECLARE_ROUND_KEY_VAR(14);
        uint8x16_t ctr0 = vld1q_u8(m_counter);
        uint64_t high, low;

        /* Process three blocks at a time, when possible. */
        if (num_blocks >= 3) {
            /* Increment CTR twice. */
            uint8x16_t ctr1 = IncrementCounterOptimized(ctr0);
            uint8x16_t ctr2 = IncrementCounterOptimized(ctr1);
            uint64_t hl_tmp;

            while (num_blocks >= 3) {
                /* Read blocks in. Keep them in registers for XOR later. */
                const uint8x16_t block0 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;
                const uint8x16_t block1 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;
                const uint8x16_t block2 = vld1q_u8(src);
                src += AES_BLOCK_SIZE;

                /* We'll be encrypting the three CTRs. */
                uint8x16_t tmp0 = ctr0, tmp1 = ctr1, tmp2 = ctr2;

                /* Actually do encryption, use optimized asm. */
                /* Interleave CTR calculations with AES ones, to mask latencies. */
                /* Note: ASM here only uses one temporary u64 instead of two, due to 30 operand limit. */
                __asm__ __volatile__ (
                    AES_ENC_ROUND(0, 0) "mov %[high], %[ctr2].d[0]\n"
                    AES_ENC_ROUND(0, 1) "mov %[low], %[ctr2].d[1]\n"
                    AES_ENC_ROUND(0, 2) "rev %[high], %[high]\n"
                    AES_ENC_ROUND(1, 0) "rev %[low], %[low]\n"
                    AES_ENC_ROUND(1, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(1, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(2, 0) "rev %[hl_tmp], %[high]\n"
                    AES_ENC_ROUND(2, 1) "mov %[ctr0].d[0], %[hl_tmp]\n"
                    AES_ENC_ROUND(2, 2) "rev %[hl_tmp], %[low]\n"
                    AES_ENC_ROUND(3, 0) "mov %[ctr0].d[1], %[hl_tmp]\n"
                    AES_ENC_ROUND(3, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(3, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(4, 0) "rev %[hl_tmp], %[high]\n"
                    AES_ENC_ROUND(4, 1) "mov %[ctr1].d[0], %[hl_tmp]\n"
                    AES_ENC_ROUND(4, 2) "rev %[hl_tmp], %[low]\n"
                    AES_ENC_ROUND(5, 0) "mov %[ctr1].d[1], %[hl_tmp]\n"
                    AES_ENC_ROUND(5, 1) "adds %[low], %[low], 1\n"
                    AES_ENC_ROUND(5, 2) "cinc %[high], %[high], cs\n"
                    AES_ENC_ROUND(6, 0) "rev %[hl_tmp], %[high]\n"
                    AES_ENC_ROUND(6, 1) "mov %[ctr2].d[0], %[hl_tmp]\n"
                    AES_ENC_ROUND(6, 2) "rev %[hl_tmp], %[low]\n"
                    AES_ENC_ROUND(7, 0) "mov %[ctr2].d[1], %[hl_tmp]\n"
                    AES_ENC_ROUND(7, 1)
                    AES_ENC_ROUND(7, 2)
                    AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                    AES_ENC_ROUND(9, 0) AES_ENC_ROUND(9, 1) AES_ENC_ROUND(9, 2)
                    AES_ENC_ROUND(10, 0) AES_ENC_ROUND(10, 1) AES_ENC_ROUND(10, 2)
                    AES_ENC_ROUND(11, 0) AES_ENC_ROUND(11, 1) AES_ENC_ROUND(11, 2)
                    AES_ENC_ROUND(12, 0) AES_ENC_ROUND(12, 1) AES_ENC_ROUND(12, 2)
                    AES_ENC_SECOND_LAST_ROUND(13, 0) AES_ENC_SECOND_LAST_ROUND(13, 1) AES_ENC_SECOND_LAST_ROUND(13, 2)
                    AES_ENC_LAST_ROUND(14, 0) AES_ENC_LAST_ROUND(14, 1) AES_ENC_LAST_ROUND(14, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_CTRS(),
                      CTR_INCREMENT_OUTPUT_HIGH_LOW(),
                      CTR_INCREMENT_OUTPUT_HL_SINGLE_TMP()
                    : AES_ENC_DEC_INPUT_ROUND_KEY(0),
                      AES_ENC_DEC_INPUT_ROUND_KEY(1),
                      AES_ENC_DEC_INPUT_ROUND_KEY(2),
                      AES_ENC_DEC_INPUT_ROUND_KEY(3),
                      AES_ENC_DEC_INPUT_ROUND_KEY(4),
                      AES_ENC_DEC_INPUT_ROUND_KEY(5),
                      AES_ENC_DEC_INPUT_ROUND_KEY(6),
                      AES_ENC_DEC_INPUT_ROUND_KEY(7),
                      AES_ENC_DEC_INPUT_ROUND_KEY(8),
                      AES_ENC_DEC_INPUT_ROUND_KEY(9),
                      AES_ENC_DEC_INPUT_ROUND_KEY(10),
                      AES_ENC_DEC_INPUT_ROUND_KEY(11),
                      AES_ENC_DEC_INPUT_ROUND_KEY(12),
                      AES_ENC_DEC_INPUT_ROUND_KEY(13),
                      AES_ENC_DEC_INPUT_ROUND_KEY(14)
                    : "cc"
                );

                /* XOR blocks. */
                tmp0 = veorq_u8(block0, tmp0);
                tmp1 = veorq_u8(block1, tmp1);
                tmp2 = veorq_u8(block2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0);
                dst += AES_BLOCK_SIZE;
                vst1q_u8(dst, tmp1);
                dst += AES_BLOCK_SIZE;
                vst1q_u8(dst, tmp2);
                dst += AES_BLOCK_SIZE;

                num_blocks -= 3;
            }
        }

        while (num_blocks >= 1) {
            /* Read block in, keep in register for XOR. */
            const uint8x16_t block0 = vld1q_u8(src);
            src += AES_BLOCK_SIZE;

            /* We'll be encrypting the CTR. */
            uint8x16_t tmp0 = ctr0;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0) "mov %[high], %[ctr0].d[0]\n"
                AES_ENC_ROUND(1, 0) "mov %[low], %[ctr0].d[1]\n"
                AES_ENC_ROUND(2, 0) "rev %[high], %[high]\n"
                AES_ENC_ROUND(3, 0) "rev %[low], %[low]\n"
                AES_ENC_ROUND(4, 0) "adds %[low], %[low], 1\n"
                AES_ENC_ROUND(5, 0) "cinc %[high], %[high], cs\n"
                AES_ENC_ROUND(6, 0) "rev %[high], %[high]\n"
                AES_ENC_ROUND(7, 0) "rev %[low], %[low]\n"
                AES_ENC_ROUND(8, 0) "mov %[ctr0].d[0], %[high]\n"
                AES_ENC_ROUND(9, 0) "mov %[ctr0].d[1], %[low]\n"
                AES_ENC_ROUND(10, 0)
                AES_ENC_ROUND(11, 0)
                AES_ENC_ROUND(12, 0)
                AES_ENC_SECOND_LAST_ROUND(13, 0)
                AES_ENC_LAST_ROUND(14, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_CTR(),
                  CTR_INCREMENT_OUTPUT_HIGH_LOW()
                : AES_ENC_DEC_INPUT_ROUND_KEY(0),
                  AES_ENC_DEC_INPUT_ROUND_KEY(1),
                  AES_ENC_DEC_INPUT_ROUND_KEY(2),
                  AES_ENC_DEC_INPUT_ROUND_KEY(3),
                  AES_ENC_DEC_INPUT_ROUND_KEY(4),
                  AES_ENC_DEC_INPUT_ROUND_KEY(5),
                  AES_ENC_DEC_INPUT_ROUND_KEY(6),
                  AES_ENC_DEC_INPUT_ROUND_KEY(7),
                  AES_ENC_DEC_INPUT_ROUND_KEY(8),
                  AES_ENC_DEC_INPUT_ROUND_KEY(9),
                  AES_ENC_DEC_INPUT_ROUND_KEY(10),
                  AES_ENC_DEC_INPUT_ROUND_KEY(11),
                  AES_ENC_DEC_INPUT_ROUND_KEY(12),
                  AES_ENC_DEC_INPUT_ROUND_KEY(13),
                  AES_ENC_DEC_INPUT_ROUND_KEY(14)
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(block0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += AES_BLOCK_SIZE;

            num_blocks--;
        }

        vst1q_u8(m_counter, ctr0);
    }

}

#else

/* TODO: Non-EL0 implementation. */
namespace ams::crypto::impl {

}

#endif
