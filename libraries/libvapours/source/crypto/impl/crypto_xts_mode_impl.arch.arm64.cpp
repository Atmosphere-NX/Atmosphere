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
#include "crypto_update_impl.hpp"

#ifdef ATMOSPHERE_IS_STRATOSPHERE
#include <arm_neon.h>

namespace ams::crypto::impl {


    /* Variable management macros. */
    #define DECLARE_ROUND_KEY_VAR(n) \
    const uint8x16_t round_key_##n = vld1q_u8(keys + (BlockSize * n))

    #define AES_ENC_DEC_OUTPUT_THREE_BLOCKS() \
    [tmp0]"+w"(tmp0), [tmp1]"+w"(tmp1), [tmp2]"+w"(tmp2)

    #define AES_ENC_DEC_OUTPUT_THREE_TWEAKS() \
    [tweak0]"+w"(tweak0), [tweak1]"+w"(tweak1), [tweak2]"+w"(tweak2)

    #define AES_ENC_DEC_OUTPUT_ONE_BLOCK() \
    [tmp0]"+w"(tmp0)

    #define AES_ENC_DEC_OUTPUT_ONE_TWEAK() \
    [tweak0]"+w"(tweak0)

    #define XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK() \
    [high]"=&r"(high), [low]"=&r"(low), [mask]"=&r"(mask)

    #define XTS_INCREMENT_INPUT_XOR() \
    [xorv]"r"(xorv)

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

    /* AES Decryption macros. */
    #define AES_DEC_ROUND(n, i) \
    "aesd %[tmp" #i "].16b, %[round_key_" #n "].16b\n" \
    "aesimc %[tmp" #i "].16b, %[tmp" #i "].16b\n"

    #define AES_DEC_SECOND_LAST_ROUND(n, i) \
    "aesd %[tmp" #i "].16b, %[round_key_" #n "].16b\n"

    #define AES_DEC_LAST_ROUND(n, i) \
    "eor %[tmp" #i "].16b, %[tmp" #i "].16b, %[round_key_" #n "].16b\n"

    namespace {

        /* TODO: Support non-Nintendo Endianness */

        ALWAYS_INLINE uint8x16_t MultiplyTweak(const uint8x16_t tweak) {
            /* TODO: Is the inline asm better than using intrinsics? */
            #if 1
                uint8x16_t mult;
                uint64_t high, low, mask;
                constexpr uint64_t xorv = 0x87ul;
                /* Use ASM. TODO: Better than using intrinsics? */
                __asm__ __volatile__ (
                    "mov %[high], %[tweak].d[1]\n"
                    "mov %[low], %[tweak].d[0]\n"
                    "and %[mask], %[xorv], %[high], asr 63\n"
                    "extr %[high], %[high], %[low], 63\n"
                    "eor %[low], %[mask], %[low], lsl 1\n"
                    "mov %[mult].d[1], %[high]\n"
                    "mov %[mult].d[0], %[low]\n"
                    : [mult]"=w"(mult),
                      XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                    : [tweak]"w"(tweak),
                      XTS_INCREMENT_INPUT_XOR()
                    : "cc"
                );
                return mult;
            #else
                constexpr uint64_t XorMask = 0x87ul;

                const uint64x2_t tweak64 = vreinterpretq_u64_u8(tweak);
                const uint64_t   high    = vgetq_lane_u64(tweak64, 1);
                const uint64_t   low     = vgetq_lane_u64(tweak64, 0);
                const uint64_t   mask    = static_cast<int64_t>(high) >> (BITSIZEOF(uint64_t) - 1);

                return vreinterpretq_u8_u64(vcombine_u64(vmov_n_u64((low << 1) ^ (mask & XorMask)), vmov_n_u64((high << 1) | (low >> (BITSIZEOF(uint64_t) - 1)))));
            #endif
        }

    }

    size_t XtsModeImpl::UpdateGeneric(void *dst, size_t dst_size, const void *src, size_t src_size) {
        AMS_ASSERT(this->state == State_Initialized || this->state == State_Processing);

        return UpdateImpl<void>(this, dst, dst_size, src, src_size);
    }

    size_t XtsModeImpl::ProcessBlocksGeneric(u8 *dst, const u8 *src, size_t num_blocks) {
        size_t processed = BlockSize * (num_blocks - 1);

        if (this->state == State_Processing) {
            this->ProcessBlock(dst, this->last_block);
            dst       += BlockSize;
            processed += BlockSize;
        }

        uint8x16_t tweak = vld1q_u8(this->tweak);

        while ((--num_blocks) > 0) {
            /* Xor */
            uint8x16_t block = vld1q_u8(src);
            src += BlockSize;
            block = veorq_u8(block, tweak);

            /* Encrypt */
            vst1q_u8(dst, block);
            this->cipher_func(dst, dst, this->cipher_ctx);
            block = vld1q_u8(dst);

            /* Xor */
            veorq_u8(block, tweak);
            vst1q_u8(dst, block);
            dst += BlockSize;

            /* Increment tweak. */
            tweak = MultiplyTweak(tweak);
        }

        vst1q_u8(this->tweak, tweak);

        std::memcpy(this->last_block, src, BlockSize);

        this->state = State_Processing;

        return processed;
    }

    template<> size_t XtsModeImpl::Update<AesEncryptor128>(void *dst, size_t dst_size, const void *src, size_t src_size) { return UpdateImpl<AesEncryptor128>(this, dst, dst_size, src, src_size); }
    template<> size_t XtsModeImpl::Update<AesEncryptor192>(void *dst, size_t dst_size, const void *src, size_t src_size) { return UpdateImpl<AesEncryptor192>(this, dst, dst_size, src, src_size); }
    template<> size_t XtsModeImpl::Update<AesEncryptor256>(void *dst, size_t dst_size, const void *src, size_t src_size) { return UpdateImpl<AesEncryptor256>(this, dst, dst_size, src, src_size); }

    template<> size_t XtsModeImpl::Update<AesDecryptor128>(void *dst, size_t dst_size, const void *src, size_t src_size) { return UpdateImpl<AesDecryptor128>(this, dst, dst_size, src, src_size); }
    template<> size_t XtsModeImpl::Update<AesDecryptor192>(void *dst, size_t dst_size, const void *src, size_t src_size) { return UpdateImpl<AesDecryptor192>(this, dst, dst_size, src, src_size); }
    template<> size_t XtsModeImpl::Update<AesDecryptor256>(void *dst, size_t dst_size, const void *src, size_t src_size) { return UpdateImpl<AesDecryptor256>(this, dst, dst_size, src, src_size); }

    template<>
    size_t XtsModeImpl::ProcessBlocks<AesEncryptor128>(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Handle last buffered block. */
        size_t processed = (num_blocks - 1) * BlockSize;

        if (this->state == State_Processing) {
            this->ProcessBlock(dst, this->last_block);
            dst += BlockSize;
            processed += BlockSize;
        }

        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = static_cast<const AesEncryptor128 *>(this->cipher_ctx)->GetRoundKey();
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
        uint8x16_t tweak0 = vld1q_u8(this->tweak);
        constexpr uint64_t xorv = 0x87ul;
        uint64_t high, low, mask;

        /* Process three blocks at a time, when possible. */
        if (num_blocks > 3) {
            /* Multiply tweak twice. */
            uint8x16_t tweak1 = MultiplyTweak(tweak0);
            uint8x16_t tweak2 = MultiplyTweak(tweak1);

            do {
                /* Save tweaks for xor usage. */
                const uint8x16_t mask0 = tweak0;
                const uint8x16_t mask1 = tweak1;
                const uint8x16_t mask2 = tweak2;

                /* Read blocks in, XOR with tweaks. */
                uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src)); src += BlockSize;

                /* Actually do encryption, use optimized asm. */
                /* Interleave GF mult calculations with AES ones, to mask latencies. */
                __asm__ __volatile__ (
                    AES_ENC_ROUND(0, 0) "mov %[high], %[tweak2].d[1]\n"
                    AES_ENC_ROUND(0, 1) "mov %[low], %[tweak2].d[0]\n"
                    AES_ENC_ROUND(0, 2) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_ENC_ROUND(1, 0) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(1, 1) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(1, 2) "mov %[tweak0].d[1], %[high]\n"
                    AES_ENC_ROUND(2, 0) "mov %[tweak0].d[0], %[low]\n"
                    AES_ENC_ROUND(2, 1) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_ENC_ROUND(2, 2) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(3, 0) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(3, 1) "mov %[tweak1].d[1], %[high]\n"
                    AES_ENC_ROUND(3, 2) "mov %[tweak1].d[0], %[low]\n"
                    AES_ENC_ROUND(4, 0) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_ENC_ROUND(4, 1) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(4, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(5, 0) "mov %[tweak2].d[1], %[high]\n"
                    AES_ENC_ROUND(5, 1) "mov %[tweak2].d[0], %[low]\n"
                    AES_ENC_ROUND(5, 2)
                    AES_ENC_ROUND(6, 0) AES_ENC_ROUND(6, 1) AES_ENC_ROUND(6, 2)
                    AES_ENC_ROUND(7, 0) AES_ENC_ROUND(7, 1) AES_ENC_ROUND(7, 2)
                    AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                    AES_ENC_SECOND_LAST_ROUND(9, 0) AES_ENC_SECOND_LAST_ROUND(9, 1) AES_ENC_SECOND_LAST_ROUND(9, 2)
                    AES_ENC_LAST_ROUND(10, 0) AES_ENC_LAST_ROUND(10, 1) AES_ENC_LAST_ROUND(10, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                      XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                    : XTS_INCREMENT_INPUT_XOR(),
                      AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
                tmp0 = veorq_u8(mask0, tmp0);
                tmp1 = veorq_u8(mask1, tmp1);
                tmp2 = veorq_u8(mask2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0); dst += BlockSize;
                vst1q_u8(dst, tmp1); dst += BlockSize;
                vst1q_u8(dst, tmp2); dst += BlockSize;

                num_blocks -= 3;
            } while (num_blocks > 3);
        }

        while ((--num_blocks) > 0) {
            /* Save tweak for xor usage. */
            const uint8x16_t mask0 = tweak0;

            /* Read block in, XOR with tweak. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src));
            src += BlockSize;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0) "mov %[high], %[tweak0].d[1]\n"
                AES_ENC_ROUND(1, 0) "mov %[low], %[tweak0].d[0]\n"
                AES_ENC_ROUND(2, 0) "and %[mask], %[xorv], %[high], asr 63\n"
                AES_ENC_ROUND(3, 0) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(4, 0) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(5, 0) "mov %[tweak0].d[1], %[high]\n"
                AES_ENC_ROUND(6, 0) "mov %[tweak0].d[0], %[low]\n"
                AES_ENC_ROUND(7, 0)
                AES_ENC_ROUND(8, 0)
                AES_ENC_SECOND_LAST_ROUND(9, 0)
                AES_ENC_LAST_ROUND(10, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            tmp0 = veorq_u8(mask0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += BlockSize;
        }

        vst1q_u8(this->tweak, tweak0);

        std::memcpy(this->last_block, src, BlockSize);
        this->state = State_Processing;

        return processed;
    }

    template<>
    size_t XtsModeImpl::ProcessBlocks<AesEncryptor192>(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Handle last buffered block. */
        size_t processed = (num_blocks - 1) * BlockSize;

        if (this->state == State_Processing) {
            this->ProcessBlock(dst, this->last_block);
            dst += BlockSize;
            processed += BlockSize;
        }

        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = static_cast<const AesEncryptor192 *>(this->cipher_ctx)->GetRoundKey();
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
        uint8x16_t tweak0 = vld1q_u8(this->tweak);
        constexpr uint64_t xorv = 0x87ul;
        uint64_t high, low, mask;

        /* Process three blocks at a time, when possible. */
        if (num_blocks > 3) {
            /* Multiply tweak twice. */
            uint8x16_t tweak1 = MultiplyTweak(tweak0);
            uint8x16_t tweak2 = MultiplyTweak(tweak1);

            do {
                /* Save tweaks for xor usage. */
                const uint8x16_t mask0 = tweak0;
                const uint8x16_t mask1 = tweak1;
                const uint8x16_t mask2 = tweak2;

                /* Read blocks in, XOR with tweaks. */
                uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src)); src += BlockSize;

                /* Actually do encryption, use optimized asm. */
                /* Interleave GF mult calculations with AES ones, to mask latencies. */
                __asm__ __volatile__ (
                    AES_ENC_ROUND(0, 0) "mov %[high], %[tweak2].d[1]\n"
                    AES_ENC_ROUND(0, 1) "mov %[low], %[tweak2].d[0]\n"
                    AES_ENC_ROUND(0, 2) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_ENC_ROUND(1, 0) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(1, 1) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(1, 2) "mov %[tweak0].d[1], %[high]\n"
                    AES_ENC_ROUND(2, 0) "mov %[tweak0].d[0], %[low]\n"
                    AES_ENC_ROUND(2, 1) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_ENC_ROUND(2, 2) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(3, 0) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(3, 1) "mov %[tweak1].d[1], %[high]\n"
                    AES_ENC_ROUND(3, 2) "mov %[tweak1].d[0], %[low]\n"
                    AES_ENC_ROUND(4, 0) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_ENC_ROUND(4, 1) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(4, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(5, 0) "mov %[tweak2].d[1], %[high]\n"
                    AES_ENC_ROUND(5, 1) "mov %[tweak2].d[0], %[low]\n"
                    AES_ENC_ROUND(5, 2)
                    AES_ENC_ROUND(6, 0) AES_ENC_ROUND(6, 1) AES_ENC_ROUND(6, 2)
                    AES_ENC_ROUND(7, 0) AES_ENC_ROUND(7, 1) AES_ENC_ROUND(7, 2)
                    AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                    AES_ENC_ROUND(9, 0) AES_ENC_ROUND(9, 1) AES_ENC_ROUND(9, 2)
                    AES_ENC_ROUND(10, 0) AES_ENC_ROUND(10, 1) AES_ENC_ROUND(10, 2)
                    AES_ENC_SECOND_LAST_ROUND(11, 0) AES_ENC_SECOND_LAST_ROUND(11, 1) AES_ENC_SECOND_LAST_ROUND(11, 2)
                    AES_ENC_LAST_ROUND(12, 0) AES_ENC_LAST_ROUND(12, 1) AES_ENC_LAST_ROUND(12, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                      XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                    : XTS_INCREMENT_INPUT_XOR(),
                      AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
                tmp0 = veorq_u8(mask0, tmp0);
                tmp1 = veorq_u8(mask1, tmp1);
                tmp2 = veorq_u8(mask2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0); dst += BlockSize;
                vst1q_u8(dst, tmp1); dst += BlockSize;
                vst1q_u8(dst, tmp2); dst += BlockSize;

                num_blocks -= 3;
            } while (num_blocks > 3);
        }

        while ((--num_blocks) > 0) {
            /* Save tweak for xor usage. */
            const uint8x16_t mask0 = tweak0;

            /* Read block in, XOR with tweak. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src));
            src += BlockSize;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0) "mov %[high], %[tweak0].d[1]\n"
                AES_ENC_ROUND(1, 0) "mov %[low], %[tweak0].d[0]\n"
                AES_ENC_ROUND(2, 0) "and %[mask], %[xorv], %[high], asr 63\n"
                AES_ENC_ROUND(3, 0) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(4, 0) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(5, 0) "mov %[tweak0].d[1], %[high]\n"
                AES_ENC_ROUND(6, 0) "mov %[tweak0].d[0], %[low]\n"
                AES_ENC_ROUND(7, 0)
                AES_ENC_ROUND(8, 0)
                AES_ENC_ROUND(9, 0)
                AES_ENC_ROUND(10, 0)
                AES_ENC_SECOND_LAST_ROUND(11, 0)
                AES_ENC_LAST_ROUND(12, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            tmp0 = veorq_u8(mask0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += BlockSize;
        }

        vst1q_u8(this->tweak, tweak0);

        std::memcpy(this->last_block, src, BlockSize);
        this->state = State_Processing;

        return processed;
    }

    template<>
    size_t XtsModeImpl::ProcessBlocks<AesEncryptor256>(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Handle last buffered block. */
        size_t processed = (num_blocks - 1) * BlockSize;

        if (this->state == State_Processing) {
            this->ProcessBlock(dst, this->last_block);
            dst += BlockSize;
            processed += BlockSize;
        }

        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = static_cast<const AesEncryptor256 *>(this->cipher_ctx)->GetRoundKey();
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
        uint8x16_t tweak0 = vld1q_u8(this->tweak);
        constexpr uint64_t xorv = 0x87ul;
        uint64_t high, low, mask;

        /* Process three blocks at a time, when possible. */
        if (num_blocks > 3) {
            /* Multiply tweak twice. */
            uint8x16_t tweak1 = MultiplyTweak(tweak0);
            uint8x16_t tweak2 = MultiplyTweak(tweak1);

            do {
                /* Save tweaks for xor usage. */
                const uint8x16_t mask0 = tweak0;
                const uint8x16_t mask1 = tweak1;
                const uint8x16_t mask2 = tweak2;

                /* Read blocks in, XOR with tweaks. */
                uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src)); src += BlockSize;

                /* Actually do encryption, use optimized asm. */
                /* Interleave GF mult calculations with AES ones, to mask latencies. */
                __asm__ __volatile__ (
                    AES_ENC_ROUND(0, 0) "mov %[high], %[tweak2].d[1]\n"
                    AES_ENC_ROUND(0, 1) "mov %[low], %[tweak2].d[0]\n"
                    AES_ENC_ROUND(0, 2) "mov %[mask], #0x87\n"
                    AES_ENC_ROUND(1, 0) "and %[mask], %[mask], %[high], asr 63\n"
                    AES_ENC_ROUND(1, 1) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(1, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(2, 0) "mov %[tweak0].d[1], %[high]\n"
                    AES_ENC_ROUND(2, 1) "mov %[tweak0].d[0], %[low]\n"
                    AES_ENC_ROUND(2, 2) "mov %[mask], #0x87\n"
                    AES_ENC_ROUND(3, 0) "and %[mask], %[mask], %[high], asr 63\n"
                    AES_ENC_ROUND(3, 1) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(3, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(4, 0) "mov %[tweak1].d[1], %[high]\n"
                    AES_ENC_ROUND(4, 1) "mov %[tweak1].d[0], %[low]\n"
                    AES_ENC_ROUND(4, 2) "mov %[mask], #0x87\n"
                    AES_ENC_ROUND(5, 0) "and %[mask], %[mask], %[high], asr 63\n"
                    AES_ENC_ROUND(5, 1) "extr %[high], %[high], %[low], 63\n"
                    AES_ENC_ROUND(5, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_ENC_ROUND(6, 0) "mov %[tweak2].d[1], %[high]\n"
                    AES_ENC_ROUND(6, 1) "mov %[tweak2].d[0], %[low]\n"
                    AES_ENC_ROUND(6, 2)
                    AES_ENC_ROUND(7, 0) AES_ENC_ROUND(7, 1) AES_ENC_ROUND(7, 2)
                    AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                    AES_ENC_ROUND(9, 0) AES_ENC_ROUND(9, 1) AES_ENC_ROUND(9, 2)
                    AES_ENC_ROUND(10, 0) AES_ENC_ROUND(10, 1) AES_ENC_ROUND(10, 2)
                    AES_ENC_ROUND(11, 0) AES_ENC_ROUND(11, 1) AES_ENC_ROUND(11, 2)
                    AES_ENC_ROUND(12, 0) AES_ENC_ROUND(12, 1) AES_ENC_ROUND(12, 2)
                    AES_ENC_SECOND_LAST_ROUND(13, 0) AES_ENC_SECOND_LAST_ROUND(13, 1) AES_ENC_SECOND_LAST_ROUND(13, 2)
                    AES_ENC_LAST_ROUND(14, 0) AES_ENC_LAST_ROUND(14, 1) AES_ENC_LAST_ROUND(14, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                      XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
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
                tmp0 = veorq_u8(mask0, tmp0);
                tmp1 = veorq_u8(mask1, tmp1);
                tmp2 = veorq_u8(mask2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0); dst += BlockSize;
                vst1q_u8(dst, tmp1); dst += BlockSize;
                vst1q_u8(dst, tmp2); dst += BlockSize;

                num_blocks -= 3;
            } while (num_blocks > 3);
        }

        while ((--num_blocks) > 0) {
            /* Save tweak for xor usage. */
            const uint8x16_t mask0 = tweak0;

            /* Read block in, XOR with tweak. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src));
            src += BlockSize;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0) "mov %[high], %[tweak0].d[1]\n"
                AES_ENC_ROUND(1, 0) "mov %[low], %[tweak0].d[0]\n"
                AES_ENC_ROUND(2, 0) "and %[mask], %[xorv], %[high], asr 63\n"
                AES_ENC_ROUND(3, 0) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(4, 0) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(5, 0) "mov %[tweak0].d[1], %[high]\n"
                AES_ENC_ROUND(6, 0) "mov %[tweak0].d[0], %[low]\n"
                AES_ENC_ROUND(7, 0)
                AES_ENC_ROUND(8, 0)
                AES_ENC_ROUND(9, 0)
                AES_ENC_ROUND(10, 0)
                AES_ENC_ROUND(11, 0)
                AES_ENC_ROUND(12, 0)
                AES_ENC_SECOND_LAST_ROUND(13, 0)
                AES_ENC_LAST_ROUND(14, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            tmp0 = veorq_u8(mask0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += BlockSize;
        }

        vst1q_u8(this->tweak, tweak0);

        std::memcpy(this->last_block, src, BlockSize);
        this->state = State_Processing;

        return processed;
    }

    template<>
    size_t XtsModeImpl::ProcessBlocks<AesDecryptor128>(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Handle last buffered block. */
        size_t processed = (num_blocks - 1) * BlockSize;

        if (this->state == State_Processing) {
            this->ProcessBlock(dst, this->last_block);
            dst += BlockSize;
            processed += BlockSize;
        }

        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = static_cast<const AesDecryptor128 *>(this->cipher_ctx)->GetRoundKey();
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
        uint8x16_t tweak0 = vld1q_u8(this->tweak);
        constexpr uint64_t xorv = 0x87ul;
        uint64_t high, low, mask;

        /* Process three blocks at a time, when possible. */
        if (num_blocks > 3) {
            /* Multiply tweak twice. */
            uint8x16_t tweak1 = MultiplyTweak(tweak0);
            uint8x16_t tweak2 = MultiplyTweak(tweak1);

            do {
                /* Save tweaks for xor usage. */
                const uint8x16_t mask0 = tweak0;
                const uint8x16_t mask1 = tweak1;
                const uint8x16_t mask2 = tweak2;

                /* Read blocks in, XOR with tweaks. */
                uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src)); src += BlockSize;

                /* Actually do encryption, use optimized asm. */
                /* Interleave GF mult calculations with AES ones, to mask latencies. */
                __asm__ __volatile__ (
                    AES_DEC_ROUND(10, 0) "mov %[high], %[tweak2].d[1]\n"
                    AES_DEC_ROUND(10, 1) "mov %[low], %[tweak2].d[0]\n"
                    AES_DEC_ROUND(10, 2) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_DEC_ROUND(9, 0)  "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(9, 1)  "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(9, 2)  "mov %[tweak0].d[1], %[high]\n"
                    AES_DEC_ROUND(8, 0)  "mov %[tweak0].d[0], %[low]\n"
                    AES_DEC_ROUND(8, 1)  "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_DEC_ROUND(8, 2)  "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(7, 0)  "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(7, 1)  "mov %[tweak1].d[1], %[high]\n"
                    AES_DEC_ROUND(7, 2)  "mov %[tweak1].d[0], %[low]\n"
                    AES_DEC_ROUND(6, 0)  "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_DEC_ROUND(6, 1)  "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(6, 2)  "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(5, 0)  "mov %[tweak2].d[1], %[high]\n"
                    AES_DEC_ROUND(5, 1)  "mov %[tweak2].d[0], %[low]\n"
                    AES_DEC_ROUND(5, 2)
                    AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
                    AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
                    AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
                    AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
                    AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                      XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                    : XTS_INCREMENT_INPUT_XOR(),
                      AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
                tmp0 = veorq_u8(mask0, tmp0);
                tmp1 = veorq_u8(mask1, tmp1);
                tmp2 = veorq_u8(mask2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0); dst += BlockSize;
                vst1q_u8(dst, tmp1); dst += BlockSize;
                vst1q_u8(dst, tmp2); dst += BlockSize;

                num_blocks -= 3;
            } while (num_blocks > 3);
        }

        while ((--num_blocks) > 0) {
            /* Save tweak for xor usage. */
            const uint8x16_t mask0 = tweak0;

            /* Read block in, XOR with tweak. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src));
            src += BlockSize;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_DEC_ROUND(10, 0) "mov %[high], %[tweak0].d[1]\n"
                AES_DEC_ROUND(9, 0)  "mov %[low], %[tweak0].d[0]\n"
                AES_DEC_ROUND(8, 0)  "and %[mask], %[xorv], %[high], asr 63\n"
                AES_DEC_ROUND(7, 0)  "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(6, 0)  "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(5, 0)  "mov %[tweak0].d[1], %[high]\n"
                AES_DEC_ROUND(4, 0)  "mov %[tweak0].d[0], %[low]\n"
                AES_DEC_ROUND(3, 0)
                AES_DEC_ROUND(2, 0)
                AES_DEC_SECOND_LAST_ROUND(1, 0)
                AES_DEC_LAST_ROUND(0, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            tmp0 = veorq_u8(mask0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += BlockSize;
        }

        vst1q_u8(this->tweak, tweak0);

        std::memcpy(this->last_block, src, BlockSize);
        this->state = State_Processing;

        return processed;
    }

    template<>
    size_t XtsModeImpl::ProcessBlocks<AesDecryptor192>(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Handle last buffered block. */
        size_t processed = (num_blocks - 1) * BlockSize;

        if (this->state == State_Processing) {
            this->ProcessBlock(dst, this->last_block);
            dst += BlockSize;
            processed += BlockSize;
        }

        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = static_cast<const AesDecryptor192 *>(this->cipher_ctx)->GetRoundKey();
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
        uint8x16_t tweak0 = vld1q_u8(this->tweak);
        constexpr uint64_t xorv = 0x87ul;
        uint64_t high, low, mask;

        /* Process three blocks at a time, when possible. */
        if (num_blocks > 3) {
            /* Multiply tweak twice. */
            uint8x16_t tweak1 = MultiplyTweak(tweak0);
            uint8x16_t tweak2 = MultiplyTweak(tweak1);

            do {
                /* Save tweaks for xor usage. */
                const uint8x16_t mask0 = tweak0;
                const uint8x16_t mask1 = tweak1;
                const uint8x16_t mask2 = tweak2;

                /* Read blocks in, XOR with tweaks. */
                uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src)); src += BlockSize;

                /* Actually do encryption, use optimized asm. */
                /* Interleave GF mult calculations with AES ones, to mask latencies. */
                __asm__ __volatile__ (
                    AES_DEC_ROUND(12, 0) "mov %[high], %[tweak2].d[1]\n"
                    AES_DEC_ROUND(12, 1) "mov %[low], %[tweak2].d[0]\n"
                    AES_DEC_ROUND(12, 2) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_DEC_ROUND(11, 0) "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(11, 1) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(11, 2) "mov %[tweak0].d[1], %[high]\n"
                    AES_DEC_ROUND(10, 0) "mov %[tweak0].d[0], %[low]\n"
                    AES_DEC_ROUND(10, 1) "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_DEC_ROUND(10, 2) "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(9, 0)  "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(9, 1)  "mov %[tweak1].d[1], %[high]\n"
                    AES_DEC_ROUND(9, 2)  "mov %[tweak1].d[0], %[low]\n"
                    AES_DEC_ROUND(8, 0)  "and %[mask], %[xorv], %[high], asr 63\n"
                    AES_DEC_ROUND(8, 1)  "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(8, 2)  "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(7, 0)  "mov %[tweak2].d[1], %[high]\n"
                    AES_DEC_ROUND(7, 1)  "mov %[tweak2].d[0], %[low]\n"
                    AES_DEC_ROUND(7, 2)
                    AES_DEC_ROUND(6, 0) AES_DEC_ROUND(6, 1) AES_DEC_ROUND(6, 2)
                    AES_DEC_ROUND(5, 0) AES_DEC_ROUND(5, 1) AES_DEC_ROUND(5, 2)
                    AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
                    AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
                    AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
                    AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
                    AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                      XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                    : XTS_INCREMENT_INPUT_XOR(),
                      AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
                tmp0 = veorq_u8(mask0, tmp0);
                tmp1 = veorq_u8(mask1, tmp1);
                tmp2 = veorq_u8(mask2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0); dst += BlockSize;
                vst1q_u8(dst, tmp1); dst += BlockSize;
                vst1q_u8(dst, tmp2); dst += BlockSize;

                num_blocks -= 3;
            } while (num_blocks > 3);
        }

        while ((--num_blocks) > 0) {
            /* Save tweak for xor usage. */
            const uint8x16_t mask0 = tweak0;

            /* Read block in, XOR with tweak. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src));
            src += BlockSize;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_DEC_ROUND(12, 0) "mov %[high], %[tweak0].d[1]\n"
                AES_DEC_ROUND(11, 0) "mov %[low], %[tweak0].d[0]\n"
                AES_DEC_ROUND(10, 0) "and %[mask], %[xorv], %[high], asr 63\n"
                AES_DEC_ROUND(9, 0)  "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(8, 0)  "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(7, 0)  "mov %[tweak0].d[1], %[high]\n"
                AES_DEC_ROUND(6, 0)  "mov %[tweak0].d[0], %[low]\n"
                AES_DEC_ROUND(5, 0)
                AES_DEC_ROUND(4, 0)
                AES_DEC_ROUND(3, 0)
                AES_DEC_ROUND(2, 0)
                AES_DEC_SECOND_LAST_ROUND(1, 0)
                AES_DEC_LAST_ROUND(0, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            tmp0 = veorq_u8(mask0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += BlockSize;
        }

        vst1q_u8(this->tweak, tweak0);

        std::memcpy(this->last_block, src, BlockSize);
        this->state = State_Processing;

        return processed;
    }

    template<>
    size_t XtsModeImpl::ProcessBlocks<AesDecryptor256>(u8 *dst, const u8 *src, size_t num_blocks) {
        /* Handle last buffered block. */
        size_t processed = (num_blocks - 1) * BlockSize;

        if (this->state == State_Processing) {
            this->ProcessBlock(dst, this->last_block);
            dst += BlockSize;
            processed += BlockSize;
        }

        /* Preload all round keys + iv into neon registers. */
        const u8 *keys = static_cast<const AesDecryptor256 *>(this->cipher_ctx)->GetRoundKey();
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
        uint8x16_t tweak0 = vld1q_u8(this->tweak);
        constexpr uint64_t xorv = 0x87ul;
        uint64_t high, low, mask;

        /* Process three blocks at a time, when possible. */
        if (num_blocks > 3) {
            /* Multiply tweak twice. */
            uint8x16_t tweak1 = MultiplyTweak(tweak0);
            uint8x16_t tweak2 = MultiplyTweak(tweak1);

            do {
                /* Save tweaks for xor usage. */
                const uint8x16_t mask0 = tweak0;
                const uint8x16_t mask1 = tweak1;
                const uint8x16_t mask2 = tweak2;

                /* Read blocks in, XOR with tweaks. */
                uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src)); src += BlockSize;
                uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src)); src += BlockSize;

                /* Actually do encryption, use optimized asm. */
                /* Interleave GF mult calculations with AES ones, to mask latencies. */
                __asm__ __volatile__ (
                    AES_DEC_ROUND(14, 0) "mov %[high], %[tweak2].d[1]\n"
                    AES_DEC_ROUND(14, 1) "mov %[low], %[tweak2].d[0]\n"
                    AES_DEC_ROUND(14, 2) "mov %[mask], 0x87\n"
                    AES_DEC_ROUND(13, 0) "and %[mask], %[mask], %[high], asr 63\n"
                    AES_DEC_ROUND(13, 1) "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(13, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(12, 0) "mov %[tweak0].d[1], %[high]\n"
                    AES_DEC_ROUND(12, 1) "mov %[tweak0].d[0], %[low]\n"
                    AES_DEC_ROUND(12, 2) "mov %[mask], 0x87\n"
                    AES_DEC_ROUND(11, 0) "and %[mask], %[mask], %[high], asr 63\n"
                    AES_DEC_ROUND(11, 1) "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(11, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(10, 0) "mov %[tweak1].d[1], %[high]\n"
                    AES_DEC_ROUND(10, 1) "mov %[tweak1].d[0], %[low]\n"
                    AES_DEC_ROUND(10, 2) "mov %[mask], 0x87\n"
                    AES_DEC_ROUND(9, 0)  "and %[mask], %[mask], %[high], asr 63\n"
                    AES_DEC_ROUND(9, 1)  "extr %[high], %[high], %[low], 63\n"
                    AES_DEC_ROUND(9, 2)  "eor %[low], %[mask], %[low], lsl 1\n"
                    AES_DEC_ROUND(8, 0)  "mov %[tweak2].d[1], %[high]\n"
                    AES_DEC_ROUND(8, 1)  "mov %[tweak2].d[0], %[low]\n"
                    AES_DEC_ROUND(8, 2)
                    AES_DEC_ROUND(7, 0) AES_DEC_ROUND(7, 1) AES_DEC_ROUND(7, 2)
                    AES_DEC_ROUND(6, 0) AES_DEC_ROUND(6, 1) AES_DEC_ROUND(6, 2)
                    AES_DEC_ROUND(5, 0) AES_DEC_ROUND(5, 1) AES_DEC_ROUND(5, 2)
                    AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
                    AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
                    AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
                    AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
                    AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
                    : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                      AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                      XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
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
                tmp0 = veorq_u8(mask0, tmp0);
                tmp1 = veorq_u8(mask1, tmp1);
                tmp2 = veorq_u8(mask2, tmp2);

                /* Store to output. */
                vst1q_u8(dst, tmp0); dst += BlockSize;
                vst1q_u8(dst, tmp1); dst += BlockSize;
                vst1q_u8(dst, tmp2); dst += BlockSize;

                num_blocks -= 3;
            } while (num_blocks > 3);
        }

        while ((--num_blocks) > 0) {
            /* Save tweak for xor usage. */
            const uint8x16_t mask0 = tweak0;

            /* Read block in, XOR with tweak. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src));
            src += BlockSize;

            /* Actually do encryption, use optimized asm. */
            /* Interleave CTR calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_DEC_ROUND(14, 0) "mov %[high], %[tweak0].d[1]\n"
                AES_DEC_ROUND(13, 0) "mov %[low], %[tweak0].d[0]\n"
                AES_DEC_ROUND(12, 0) "and %[mask], %[xorv], %[high], asr 63\n"
                AES_DEC_ROUND(11, 0) "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(10, 0) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(9, 0)  "mov %[tweak0].d[1], %[high]\n"
                AES_DEC_ROUND(8, 0)  "mov %[tweak0].d[0], %[low]\n"
                AES_DEC_ROUND(7, 0)
                AES_DEC_ROUND(6, 0)
                AES_DEC_ROUND(5, 0)
                AES_DEC_ROUND(4, 0)
                AES_DEC_ROUND(3, 0)
                AES_DEC_ROUND(2, 0)
                AES_DEC_SECOND_LAST_ROUND(1, 0)
                AES_DEC_LAST_ROUND(0, 0)
                : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
                  AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            tmp0 = veorq_u8(mask0, tmp0);

            /* Store to output. */
            vst1q_u8(dst, tmp0);
            dst += BlockSize;
        }

        vst1q_u8(this->tweak, tweak0);

        std::memcpy(this->last_block, src, BlockSize);
        this->state = State_Processing;

        return processed;
    }

}

#else

/* TODO: Non-EL0 implementation. */
namespace ams::crypto::impl {

}

#endif

