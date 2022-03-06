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

namespace ams::crypto::impl {

    namespace {

        constexpr const u32 RoundConstants[4] = {
            0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
        };

        /* Define for loading work var from message. */
        #define SHA1_LOAD_W_FROM_MESSAGE(which)                      \
        w[which] = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(data))); \
        data += 0x10

        #define SHA1_CALCULATE_W_FROM_PREVIOUS(i) \
        w[i] = vsha1su1q_u32(vsha1su0q_u32(w[i-4], w[i-3], w[i-2]), w[i-1])

        /* Define for doing four rounds of SHA1. */
        #define SHA1_DO_ROUND(r, insn, constant)                                   \
        do {                                                                       \
            const u32 a = vgetq_lane_u32(cur_abcd, 0);                             \
            cur_abcd = v##insn##q_u32(cur_abcd, cur_e, vaddq_u32(w[r], constant)); \
            cur_e = vsha1h_u32(a);                                                 \
        } while (0)


    }

    void Sha1Impl::Initialize() {
        /* Reset buffered bytes/bits. */
        m_buffered_bytes = 0;
        m_bits_consumed  = 0;

        /* Set intermediate hash. */
        m_intermediate_hash[0] = 0x67452301;
        m_intermediate_hash[1] = 0xEFCDAB89;
        m_intermediate_hash[2] = 0x98BADCFE;
        m_intermediate_hash[3] = 0x10325476;
        m_intermediate_hash[4] = 0xC3D2E1F0;

        /* Set state. */
        m_state = State_Initialized;
    }

    void Sha1Impl::Update(const void *data, size_t size) {
        /* Verify we're in a state to update. */
        AMS_ASSERT(m_state == State_Initialized);

        /* Advance our input bit count. */
        m_bits_consumed += BITSIZEOF(u8) * (((m_buffered_bytes + size) / BlockSize) * BlockSize);

        /* Process anything we have buffered. */
        const u8 *data8 = static_cast<const u8 *>(data);
        size_t remaining = size;

        if (m_buffered_bytes > 0) {
            const size_t copy_size = std::min(BlockSize - m_buffered_bytes, remaining);
            std::memcpy(m_buffer + m_buffered_bytes, data8, copy_size);

            data8            += copy_size;
            remaining        -= copy_size;
            m_buffered_bytes += copy_size;

            /* Process a block, if we filled one. */
            if (m_buffered_bytes == BlockSize) {
                this->ProcessBlock(m_buffer);
                m_buffered_bytes = 0;
            }
        }

        /* Process blocks, if we have any. */
        if (remaining >= BlockSize) {
            const size_t blocks = remaining / BlockSize;

            this->ProcessBlocks(data8, blocks);
            data8     += BlockSize * blocks;
            remaining -= BlockSize * blocks;
        }

        /* Copy any leftover data to our buffer. */
        if (remaining > 0) {
            m_buffered_bytes = remaining;
            std::memcpy(m_buffer, data8, remaining);
        }
    }

    void Sha1Impl::GetHash(void *dst, size_t size) {
        /* Verify we're in a state to get hash. */
        AMS_ASSERT(m_state == State_Initialized || m_state == State_Done);
        AMS_ASSERT(size >= HashSize);
        AMS_UNUSED(size);

        /* If we need to, process the last block. */
        if (m_state == State_Initialized) {
            this->ProcessLastBlock();
            m_state = State_Done;
        }

        /* Copy the output hash. */
        if constexpr (util::IsLittleEndian()) {
            static_assert(HashSize % sizeof(u32) == 0);

            u32 *dst_32 = static_cast<u32 *>(dst);
            for (size_t i = 0; i < HashSize / sizeof(u32); ++i) {
                dst_32[i] = util::LoadBigEndian<u32>(m_intermediate_hash + i);
            }
        } else {
            std::memcpy(dst, m_intermediate_hash, HashSize);
        }
    }

    ALWAYS_INLINE void Sha1Impl::ProcessBlock(const void *data) {
        return this->ProcessBlocks(static_cast<const u8 *>(data), 1);
    }

    void Sha1Impl::ProcessBlocks(const u8 *data, size_t block_count) {
        /* Setup round constants. */
        const uint32x4_t k0 = vdupq_n_u32(RoundConstants[0]);
        const uint32x4_t k1 = vdupq_n_u32(RoundConstants[1]);
        const uint32x4_t k2 = vdupq_n_u32(RoundConstants[2]);
        const uint32x4_t k3 = vdupq_n_u32(RoundConstants[3]);

        /* Load hash variables with intermediate state. */
        uint32x4_t cur_abcd = vld1q_u32(m_intermediate_hash + 0);
        u32 cur_e = m_intermediate_hash[4];

        /* Actually do hash processing blocks. */
        do {
            /* Save current state. */
            const uint32x4_t prev_abcd = cur_abcd;
            const u32 prev_e = cur_e;

            uint32x4_t w[20];

            /* Setup w[0-3] with message. */
            SHA1_LOAD_W_FROM_MESSAGE(0);
            SHA1_LOAD_W_FROM_MESSAGE(1);
            SHA1_LOAD_W_FROM_MESSAGE(2);
            SHA1_LOAD_W_FROM_MESSAGE(3);

            /* Calculate w[4-19], w[i] = sha1su1(sha1su0(w[i-4], w[i-3], w[i-2]), w[i-1]); */
            SHA1_CALCULATE_W_FROM_PREVIOUS(4);
            SHA1_CALCULATE_W_FROM_PREVIOUS(5);
            SHA1_CALCULATE_W_FROM_PREVIOUS(6);
            SHA1_CALCULATE_W_FROM_PREVIOUS(7);
            SHA1_CALCULATE_W_FROM_PREVIOUS(8);
            SHA1_CALCULATE_W_FROM_PREVIOUS(9);
            SHA1_CALCULATE_W_FROM_PREVIOUS(10);
            SHA1_CALCULATE_W_FROM_PREVIOUS(11);
            SHA1_CALCULATE_W_FROM_PREVIOUS(12);
            SHA1_CALCULATE_W_FROM_PREVIOUS(13);
            SHA1_CALCULATE_W_FROM_PREVIOUS(14);
            SHA1_CALCULATE_W_FROM_PREVIOUS(15);
            SHA1_CALCULATE_W_FROM_PREVIOUS(16);
            SHA1_CALCULATE_W_FROM_PREVIOUS(17);
            SHA1_CALCULATE_W_FROM_PREVIOUS(18);
            SHA1_CALCULATE_W_FROM_PREVIOUS(19);

            /* Do round calculations 0-20. Uses sha1c, k0. */
            SHA1_DO_ROUND(0, sha1c, k0);
            SHA1_DO_ROUND(1, sha1c, k0);
            SHA1_DO_ROUND(2, sha1c, k0);
            SHA1_DO_ROUND(3, sha1c, k0);
            SHA1_DO_ROUND(4, sha1c, k0);

            /* Do round calculations 20-40. Uses sha1p, k1. */
            SHA1_DO_ROUND(5, sha1p, k1);
            SHA1_DO_ROUND(6, sha1p, k1);
            SHA1_DO_ROUND(7, sha1p, k1);
            SHA1_DO_ROUND(8, sha1p, k1);
            SHA1_DO_ROUND(9, sha1p, k1);

            /* Do round calculations 40-60. Uses sha1m, k2. */
            SHA1_DO_ROUND(10, sha1m, k2);
            SHA1_DO_ROUND(11, sha1m, k2);
            SHA1_DO_ROUND(12, sha1m, k2);
            SHA1_DO_ROUND(13, sha1m, k2);
            SHA1_DO_ROUND(14, sha1m, k2);

            /* Do round calculations 60-80. Uses sha1p, k3. */
            SHA1_DO_ROUND(15, sha1p, k3);
            SHA1_DO_ROUND(16, sha1p, k3);
            SHA1_DO_ROUND(17, sha1p, k3);
            SHA1_DO_ROUND(18, sha1p, k3);
            SHA1_DO_ROUND(19, sha1p, k3);

            /* Add to previous. */
            cur_abcd = vaddq_u32(cur_abcd, prev_abcd);
            cur_e = cur_e + prev_e;
        } while (--block_count != 0);

        /* Save result to intermediate hash. */
        vst1q_u32(m_intermediate_hash, cur_abcd);
        m_intermediate_hash[4] = cur_e;
    }

    void Sha1Impl::ProcessLastBlock() {
        /* Setup the final block. */
        constexpr const auto BlockSizeWithoutSizeField = BlockSize - sizeof(u64);

        /* Increment our bits consumed. */
        m_bits_consumed += BITSIZEOF(u8) * m_buffered_bytes;

        /* Add 0x80 terminator. */
        m_buffer[m_buffered_bytes++] = 0x80;

        /* If we can process the size field directly, do so, otherwise set up to process it. */
        if (m_buffered_bytes <= BlockSizeWithoutSizeField) {
            /* Clear up to size field. */
            std::memset(m_buffer + m_buffered_bytes, 0, BlockSizeWithoutSizeField - m_buffered_bytes);
        } else {
            /* Consume full block */
            std::memset(m_buffer + m_buffered_bytes, 0, BlockSize - m_buffered_bytes);
            this->ProcessBlock(m_buffer);

            /* Clear up to size field. */
            std::memset(m_buffer, 0, BlockSizeWithoutSizeField);
        }

        /* Store the size field. */
        util::StoreBigEndian<u64>(reinterpret_cast<u64 *>(m_buffer + BlockSizeWithoutSizeField), m_bits_consumed);

        /* Process the final block. */
        this->ProcessBlock(m_buffer);
    }

}
#endif