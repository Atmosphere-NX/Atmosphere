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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <arm_neon.h>
#endif

namespace ams::crypto::impl {

#if defined(ATMOSPHERE_IS_STRATOSPHERE)


    namespace {

        /* Helper macros to setup for inline AES asm */
        #define AES_ENC_DEC_SETUP_VARS()                                                       \
            const auto *ctx = reinterpret_cast<const RoundKeyHelper<KeySize> *>(m_round_keys); \
            static_assert(sizeof(*ctx) == sizeof(m_round_keys));                               \
                                                                                               \
            uint8x16_t tmp = vld1q_u8((const uint8_t *)src);                                   \
            uint8x16_t tmp2

        #define AES_ENC_DEC_OUTPUT_VARS() \
            [tmp]"+w"(tmp), [tmp2]"=w"(tmp2)

        #define AES_ENC_DEC_STORE_RESULT() \
            vst1q_u8((uint8_t *)dst, tmp)

        /* Helper macros to do AES encryption, via inline asm. */
        #define AES_ENC_ROUND(n)                  \
            "ldr %q[tmp2], %[round_key_" #n "]\n" \
            "aese %[tmp].16b, %[tmp2].16b\n"      \
            "aesmc %[tmp].16b, %[tmp].16b\n"

        #define AES_ENC_FINAL_ROUND()                  \
            "ldr %q[tmp2], %[round_key_second_last]\n" \
            "aese %[tmp].16b, %[tmp2].16b\n"           \
            "ldr %q[tmp2], %[round_key_last]\n"        \
            "eor %[tmp].16b, %[tmp].16b, %[tmp2].16b"

        #define AES_ENC_INPUT_ROUND_KEY(num_rounds, n) \
            [round_key_##n]"m"(ctx->round_keys[(n-1)])

        #define AES_ENC_INPUT_LAST_ROUND_KEYS(num_rounds)                 \
            [round_key_second_last]"m"(ctx->round_keys[(num_rounds - 1)]), \
            [round_key_last]"m"(ctx->round_keys[(num_rounds)])

        /* Helper macros to do AES decryption, via inline asm. */
        #define AES_DEC_ROUND(n)                  \
            "ldr %q[tmp2], %[round_key_" #n "]\n" \
            "aesd %[tmp].16b, %[tmp2].16b\n"      \
            "aesimc %[tmp].16b, %[tmp].16b\n"

        #define AES_DEC_FINAL_ROUND()                  \
            "ldr %q[tmp2], %[round_key_second_last]\n" \
            "aesd %[tmp].16b, %[tmp2].16b\n"           \
            "ldr %q[tmp2], %[round_key_last]\n"        \
            "eor %[tmp].16b, %[tmp].16b, %[tmp2].16b"

        #define AES_DEC_INPUT_ROUND_KEY(num_rounds, n) \
            [round_key_##n]"m"(ctx->round_keys[(num_rounds + 1 - n)])

        #define AES_DEC_INPUT_LAST_ROUND_KEYS(num_rounds)   \
            [round_key_second_last]"m"(ctx->round_keys[1]), \
            [round_key_last]"m"(ctx->round_keys[0])


        constexpr const u8 RoundKeyRcon0[] = {
            0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36, 0x6C, 0xD8, 0xAB, 0x4D, 0x9A, 0x2F,
            0x5E, 0xBC, 0x63, 0xC6, 0x97, 0x35, 0x6A, 0xD4, 0xB3, 0x7D, 0xFA, 0xEF, 0xC5, 0x91,
        };

        constexpr const u8 SubBytesTable[0x100] = {
            0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
            0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
            0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
            0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
            0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
            0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
            0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
            0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
            0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
            0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
            0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
            0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
            0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
            0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
            0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
            0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
        };

        constexpr auto AesWordByte0Shift = 0 * BITSIZEOF(u8);
        constexpr auto AesWordByte1Shift = 1 * BITSIZEOF(u8);
        constexpr auto AesWordByte2Shift = 2 * BITSIZEOF(u8);
        constexpr auto AesWordByte3Shift = 3 * BITSIZEOF(u8);

        constexpr u32 SubBytesAndRotate(u32 v) {
            return (static_cast<u32>(SubBytesTable[(v >> AesWordByte0Shift) & 0xFFu]) << AesWordByte3Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte1Shift) & 0xFFu]) << AesWordByte0Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte2Shift) & 0xFFu]) << AesWordByte1Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte3Shift) & 0xFFu]) << AesWordByte2Shift);
        }

        constexpr u32 SubBytes(u32 v) {
            return (static_cast<u32>(SubBytesTable[(v >> AesWordByte0Shift) & 0xFFu]) << AesWordByte0Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte1Shift) & 0xFFu]) << AesWordByte1Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte2Shift) & 0xFFu]) << AesWordByte2Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte3Shift) & 0xFFu]) << AesWordByte3Shift);
        }

    }

    template<size_t KeySize>
    AesImpl<KeySize>::~AesImpl() {
        ClearMemory(this, sizeof(*this));
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::Initialize(const void *key, size_t key_size, bool is_encrypt) {
        /* Check pre-conditions. */
        AMS_ASSERT(key_size == KeySize);
        AMS_UNUSED(key_size);

        /* Set up key. */
        u32 *dst = m_round_keys;
        std::memcpy(dst, key, KeySize);

        /* Perform key scheduling. */
        constexpr auto InitialKeyWords = KeySize / sizeof(u32);
        u32 tmp = dst[InitialKeyWords - 1];

        for (auto i = InitialKeyWords; i < (RoundCount  + 1) * 4; ++i) {
            const auto idx_in_key = i % InitialKeyWords;
            if (idx_in_key == 0) {
                /* At start of key word, we need to handle sub/rotate/rcon. */
                tmp = SubBytesAndRotate(tmp);
                tmp ^= (RoundKeyRcon0[i / InitialKeyWords - 1] << AesWordByte0Shift);
            } else if ((InitialKeyWords > 6) && idx_in_key == 4) {
                /* Halfway into a 256-bit key word, we need to do an additional subbytes. */
                tmp = SubBytes(tmp);
            }

            /* Set the key word. */
            tmp ^= dst[i - InitialKeyWords];
            dst[i] = tmp;
        }

        /* If decrypting, perform inverse mix columns on all round keys. */
        if (!is_encrypt) {
            auto *key8 = reinterpret_cast<u8 *>(m_round_keys) + BlockSize;

            for (auto i = 1; i < RoundCount; ++i) {
                vst1q_u8(key8, vaesimcq_u8(vld1q_u8(key8)));
                key8 += BlockSize;
            }
        }
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::EncryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        AMS_UNUSED(src_size, dst_size);

        /* Get the key. */
        const u8 *key8 = reinterpret_cast<const u8 *>(m_round_keys);

        /* Read the block. */
        uint8x16_t block = vld1q_u8(static_cast<const u8 *>(src));

        /* Encrypt block. */
        for (auto round = 1; round < RoundCount; ++round) {
            /* Do aes round. */
            block = vaeseq_u8(block, vld1q_u8(key8));
            key8 += BlockSize;

            /* Do mix columns. */
            block = vaesmcq_u8(block);
        }

        /* Do last aes round. */
        block = vaeseq_u8(block, vld1q_u8(key8));
        key8 += BlockSize;

        /* Add the final round key. */
        block = veorq_u8(block, vld1q_u8(key8));

        /* Store the block. */
        vst1q_u8(static_cast<u8 *>(dst), block);
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::DecryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        AMS_UNUSED(src_size, dst_size);

        /* Get the key. */
        const u8 *key8 = reinterpret_cast<const u8 *>(m_round_keys) + (RoundCount * BlockSize);

        /* Read the block. */
        uint8x16_t block = vld1q_u8(static_cast<const u8 *>(src));

        /* Encrypt block. */
        for (auto round = RoundCount; round > 1; --round) {
            /* Do aes round. */
            block = vaesdq_u8(block, vld1q_u8(key8));
            key8 -= BlockSize;

            /* Do mix columns. */
            block = vaesimcq_u8(block);
        }

        /* Do last aes round. */
        block = vaesdq_u8(block, vld1q_u8(key8));
        key8 -= BlockSize;

        /* Add the first round key. */
        block = veorq_u8(block, vld1q_u8(key8));

        /* Store the block. */
        vst1q_u8(static_cast<u8 *>(dst), block);
    }

    /* Specializations when building specifically for cortex-a57 (or for apple M* processors). */
    #if defined(ATMOSPHERE_CPU_ARM_CORTEX_A57) || defined(ATMOSPHERE_OS_MACOS)

    namespace {

        template<size_t KeySize>
        struct RoundKeyHelper {
            u8 round_keys[AesImpl<KeySize>::RoundCount + 1][AesImpl<KeySize>::BlockSize];
        };

    }

    template<>
    void AesImpl<16>::EncryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        AMS_UNUSED(src_size, dst_size);

        /* Setup for asm */
        AES_ENC_DEC_SETUP_VARS();

        /* Use optimized assembly to do all rounds. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(1)
            AES_ENC_ROUND(2)
            AES_ENC_ROUND(3)
            AES_ENC_ROUND(4)
            AES_ENC_ROUND(5)
            AES_ENC_ROUND(6)
            AES_ENC_ROUND(7)
            AES_ENC_ROUND(8)
            AES_ENC_ROUND(9)
            AES_ENC_FINAL_ROUND()
            : AES_ENC_DEC_OUTPUT_VARS()
            : AES_ENC_INPUT_ROUND_KEY(RoundCount, 1),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 2),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 3),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 4),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 5),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 6),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 7),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 8),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 9),
              AES_ENC_INPUT_LAST_ROUND_KEYS(RoundCount)
        );

        /* Store result. */
        AES_ENC_DEC_STORE_RESULT();
    }

    template<>
    void AesImpl<24>::EncryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        AMS_UNUSED(src_size, dst_size);

        /* Setup for asm */
        AES_ENC_DEC_SETUP_VARS();

        /* Use optimized assembly to do all rounds. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(1)
            AES_ENC_ROUND(2)
            AES_ENC_ROUND(3)
            AES_ENC_ROUND(4)
            AES_ENC_ROUND(5)
            AES_ENC_ROUND(6)
            AES_ENC_ROUND(7)
            AES_ENC_ROUND(8)
            AES_ENC_ROUND(9)
            AES_ENC_ROUND(10)
            AES_ENC_ROUND(11)
            AES_ENC_FINAL_ROUND()
            : AES_ENC_DEC_OUTPUT_VARS()
            : AES_ENC_INPUT_ROUND_KEY(RoundCount, 1),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 2),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 3),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 4),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 5),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 6),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 7),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 8),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 9),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 10),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 11),
              AES_ENC_INPUT_LAST_ROUND_KEYS(RoundCount)
        );

        /* Store result. */
        AES_ENC_DEC_STORE_RESULT();
    }

    template<>
    void AesImpl<32>::EncryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        AMS_UNUSED(src_size, dst_size);

        /* Setup for asm */
        AES_ENC_DEC_SETUP_VARS();

        /* Use optimized assembly to do all rounds. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(1)
            AES_ENC_ROUND(2)
            AES_ENC_ROUND(3)
            AES_ENC_ROUND(4)
            AES_ENC_ROUND(5)
            AES_ENC_ROUND(6)
            AES_ENC_ROUND(7)
            AES_ENC_ROUND(8)
            AES_ENC_ROUND(9)
            AES_ENC_ROUND(10)
            AES_ENC_ROUND(11)
            AES_ENC_ROUND(12)
            AES_ENC_ROUND(13)
            AES_ENC_FINAL_ROUND()
            : AES_ENC_DEC_OUTPUT_VARS()
            : AES_ENC_INPUT_ROUND_KEY(RoundCount, 1),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 2),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 3),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 4),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 5),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 6),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 7),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 8),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 9),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 10),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 11),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 12),
              AES_ENC_INPUT_ROUND_KEY(RoundCount, 13),
              AES_ENC_INPUT_LAST_ROUND_KEYS(RoundCount)
        );

        /* Store result. */
        AES_ENC_DEC_STORE_RESULT();
    }

    template<>
    void AesImpl<16>::DecryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        AMS_UNUSED(src_size, dst_size);

        /* Setup for asm */
        AES_ENC_DEC_SETUP_VARS();

        /* Use optimized assembly to do all rounds. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(1)
            AES_DEC_ROUND(2)
            AES_DEC_ROUND(3)
            AES_DEC_ROUND(4)
            AES_DEC_ROUND(5)
            AES_DEC_ROUND(6)
            AES_DEC_ROUND(7)
            AES_DEC_ROUND(8)
            AES_DEC_ROUND(9)
            AES_DEC_FINAL_ROUND()
            : AES_ENC_DEC_OUTPUT_VARS()
            : AES_DEC_INPUT_ROUND_KEY(RoundCount, 1),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 2),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 3),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 4),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 5),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 6),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 7),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 8),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 9),
              AES_DEC_INPUT_LAST_ROUND_KEYS(RoundCount)
        );

        /* Store result. */
        AES_ENC_DEC_STORE_RESULT();
    }

    template<>
    void AesImpl<24>::DecryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        AMS_UNUSED(src_size, dst_size);

        /* Setup for asm */
        AES_ENC_DEC_SETUP_VARS();

        /* Use optimized assembly to do all rounds. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(1)
            AES_DEC_ROUND(2)
            AES_DEC_ROUND(3)
            AES_DEC_ROUND(4)
            AES_DEC_ROUND(5)
            AES_DEC_ROUND(6)
            AES_DEC_ROUND(7)
            AES_DEC_ROUND(8)
            AES_DEC_ROUND(9)
            AES_DEC_ROUND(10)
            AES_DEC_ROUND(11)
            AES_DEC_FINAL_ROUND()
            : AES_ENC_DEC_OUTPUT_VARS()
            : AES_DEC_INPUT_ROUND_KEY(RoundCount, 1),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 2),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 3),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 4),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 5),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 6),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 7),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 8),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 9),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 10),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 11),
              AES_DEC_INPUT_LAST_ROUND_KEYS(RoundCount)
        );

        /* Store result. */
        AES_ENC_DEC_STORE_RESULT();
    }

    template<>
    void AesImpl<32>::DecryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        AMS_UNUSED(src_size, dst_size);

        /* Setup for asm */
        AES_ENC_DEC_SETUP_VARS();

        /* Use optimized assembly to do all rounds. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(1)
            AES_DEC_ROUND(2)
            AES_DEC_ROUND(3)
            AES_DEC_ROUND(4)
            AES_DEC_ROUND(5)
            AES_DEC_ROUND(6)
            AES_DEC_ROUND(7)
            AES_DEC_ROUND(8)
            AES_DEC_ROUND(9)
            AES_DEC_ROUND(10)
            AES_DEC_ROUND(11)
            AES_DEC_ROUND(12)
            AES_DEC_ROUND(13)
            AES_DEC_FINAL_ROUND()
            : AES_ENC_DEC_OUTPUT_VARS()
            : AES_DEC_INPUT_ROUND_KEY(RoundCount, 1),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 2),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 3),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 4),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 5),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 6),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 7),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 8),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 9),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 10),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 11),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 12),
              AES_DEC_INPUT_ROUND_KEY(RoundCount, 13),
              AES_DEC_INPUT_LAST_ROUND_KEYS(RoundCount)
        );

        /* Store result. */
        AES_ENC_DEC_STORE_RESULT();
    }

    #endif

    /* Explicitly instantiate the three supported key sizes. */
    template class AesImpl<16>;
    template class AesImpl<24>;
    template class AesImpl<32>;

#else

    /* NOTE: Exosphere defines this in libexosphere. */

    /* TODO: Non-EL0 implementation. */

#endif

}