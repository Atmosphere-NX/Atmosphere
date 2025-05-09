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

        alignas(Sha256Impl::BlockSize) constexpr const u32 RoundConstants[0x40] = {
            0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
            0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
            0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
            0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
            0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
            0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
            0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
            0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
            0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
            0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
            0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
            0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
            0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
            0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
            0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
            0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
        };

    }

    void Sha256Impl::Initialize() {
        /* Reset buffered bytes/bits. */
        m_buffered_bytes = 0;
        m_bits_consumed  = 0;

        /* Set intermediate hash. */
        m_intermediate_hash[0] = 0x6A09E667;
        m_intermediate_hash[1] = 0xBB67AE85;
        m_intermediate_hash[2] = 0x3C6EF372;
        m_intermediate_hash[3] = 0xA54FF53A;
        m_intermediate_hash[4] = 0x510E527F;
        m_intermediate_hash[5] = 0x9B05688C;
        m_intermediate_hash[6] = 0x1F83D9AB;
        m_intermediate_hash[7] = 0x5BE0CD19;

        /* Set state. */
        m_state = State_Initialized;
    }

    void Sha256Impl::Update(const void *data, size_t size) {
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

    void Sha256Impl::GetHash(void *dst, size_t size) {
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

    void Sha256Impl::InitializeWithContext(const Sha256Context *context) {
        /* Copy state in from the context. */
        std::memcpy(m_intermediate_hash, context->intermediate_hash, sizeof(m_intermediate_hash));
        m_bits_consumed = context->bits_consumed;

        /* Reset other fields. */
        m_buffered_bytes = 0;
        m_state = State_Initialized;
    }

    size_t Sha256Impl::GetContext(Sha256Context *context) const {
        /* Check our state. */
        AMS_ASSERT(m_state == State_Initialized);

        /* Copy out the context. */
        std::memcpy(context->intermediate_hash, m_intermediate_hash, sizeof(context->intermediate_hash));
        context->bits_consumed = m_bits_consumed;

        return m_buffered_bytes;
    }

    ALWAYS_INLINE void Sha256Impl::ProcessBlock(const void *data) {
        return this->ProcessBlocks(static_cast<const u8 *>(data), 1);
    }

    void Sha256Impl::ProcessBlocks(const u8 *data, size_t block_count) {
        /* Load previous hash with intermediate state, current hash with zeroes. */
        uint32x4_t prev_hash0 = vld1q_u32(m_intermediate_hash + 0);
        uint32x4_t prev_hash1 = vld1q_u32(m_intermediate_hash + 4);
        uint32x4_t cur_hash0  = vdupq_n_u32(0);
        uint32x4_t cur_hash1  = vdupq_n_u32(0);

        /* Process blocks. */
        do {
            uint32x4_t round_constant0, round_constant1;
            uint32x4_t data0, data1, data2, data3;
            uint32x4_t tmp0, tmp1, tmp2, tmp3;
            uint32x4_t tmp_hash;

            /* Use optimized ASM implementation to process the block. */
            __asm__ __volatile__ (
                "ldp       %q[data0], %q[data1], [%[data]], #0x20\n"
                "ldp       %q[data2], %q[data3], [%[data]], #0x20\n"
                "add       %[cur_hash0].4s, %[cur_hash0].4s, %[prev_hash0].4s\n"
                "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x00]\n"
                "add       %[cur_hash1].4s, %[cur_hash1].4s, %[prev_hash1].4s\n"
                "rev32     %[data0].16b, %[data0].16b\n"
                "rev32     %[data1].16b, %[data1].16b\n"
                "rev32     %[data2].16b, %[data2].16b\n"
                "rev32     %[data3].16b, %[data3].16b\n"
                "add       %[tmp0].4s, %[data0].4s, %[round_constant0].4s\n"
                "add       %[tmp1].4s, %[data1].4s, %[round_constant1].4s\n"
                "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x20]\n"
                "sha256su0 %[data0].4s, %[data1].4s\n"
                "mov       %[prev_hash0].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp0].4s\n"
                "mov       %[prev_hash1].16b, %[cur_hash1].16b\n"
                "sha256h2  %q[cur_hash1], %q[prev_hash0], %[tmp0].4s\n"
                "sha256su0 %[data1].4s, %[data2].4s\n"
                "sha256su1 %[data0].4s, %[data2].4s, %[data3].4s\n"
                "add       %[tmp2].4s, %[data2].4s, %[round_constant0].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp1].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp1].4s\n"
                "sha256su0 %[data2].4s, %[data3].4s\n"
                "sha256su1 %[data1].4s, %[data3].4s, %[data0].4s\n"
                "add       %[tmp3].4s, %[data3].4s, %[round_constant1].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x40]\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp2].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp2].4s\n"
                "sha256su0 %[data3].4s, %[data0].4s\n"
                "sha256su1 %[data2].4s, %[data0].4s, %[data1].4s\n"
                "add       %[tmp0].4s, %[data0].4s, %[round_constant0].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp3].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp3].4s\n"
                "sha256su0 %[data0].4s, %[data1].4s\n"
                "sha256su1 %[data3].4s, %[data1].4s, %[data2].4s\n"
                "add       %[tmp1].4s, %[data1].4s, %[round_constant1].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x60]\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp0].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp0].4s\n"
                "sha256su0 %[data1].4s, %[data2].4s\n"
                "sha256su1 %[data0].4s, %[data2].4s, %[data3].4s\n"
                "add       %[tmp2].4s, %[data2].4s, %[round_constant0].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp1].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp1].4s\n"
                "sha256su0 %[data2].4s, %[data3].4s\n"
                "sha256su1 %[data1].4s, %[data3].4s, %[data0].4s\n"
                "add       %[tmp3].4s, %[data3].4s, %[round_constant1].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x80]\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp2].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp2].4s\n"
                "sha256su0 %[data3].4s, %[data0].4s\n"
                "sha256su1 %[data2].4s, %[data0].4s, %[data1].4s\n"
                "add       %[tmp0].4s, %[data0].4s, %[round_constant0].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp3].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp3].4s\n"
                "sha256su0 %[data0].4s, %[data1].4s\n"
                "sha256su1 %[data3].4s, %[data1].4s, %[data2].4s\n"
                "add       %[tmp1].4s, %[data1].4s, %[round_constant1].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0xA0]\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp0].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp0].4s\n"
                "sha256su0 %[data1].4s, %[data2].4s\n"
                "sha256su1 %[data0].4s, %[data2].4s, %[data3].4s\n"
                "add       %[tmp2].4s, %[data2].4s, %[round_constant0].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp1].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp1].4s\n"
                "sha256su0 %[data2].4s, %[data3].4s\n"
                "sha256su1 %[data1].4s, %[data3].4s, %[data0].4s\n"
                "add       %[tmp3].4s, %[data3].4s, %[round_constant1].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0xC0]\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp2].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp2].4s\n"
                "sha256su0 %[data3].4s, %[data0].4s\n"
                "sha256su1 %[data2].4s, %[data0].4s, %[data1].4s\n"
                "add       %[tmp0].4s, %[data0].4s, %[round_constant0].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp3].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp3].4s\n"
                "sha256su1 %[data3].4s, %[data1].4s, %[data2].4s\n"
                "add       %[tmp1].4s, %[data1].4s, %[round_constant1].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0xE0]\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp0].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp0].4s\n"
                "add       %[tmp2].4s, %[data2].4s, %[round_constant0].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp1].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp1].4s\n"
                "add       %[tmp3].4s, %[data3].4s, %[round_constant1].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp2].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp2].4s\n"
                "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
                "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp3].4s\n"
                "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp3].4s\n"
                : [data0]"=w"(data0), [data1]"=w"(data1), [data2]"=w"(data2), [data3]"=w"(data3),
                  [tmp0]"=w"(tmp0), [tmp1]"=w"(tmp1), [tmp2]"=w"(tmp2), [tmp3]"=w"(tmp3),
                  [round_constant0]"=w"(round_constant0), [round_constant1]"=w"(round_constant1),
                  [cur_hash0]"+w"(cur_hash0), [cur_hash1]"+w"(cur_hash1),
                  [prev_hash0]"+w"(prev_hash0), [prev_hash1]"+w"(prev_hash1),
                  [tmp_hash]"=w"(tmp_hash), [data]"+r"(data)
                : "m"(*(const u8 (*)[block_count*BlockSize])data), [round_constants]"r"(RoundConstants)
                :
            );
        } while (--block_count != 0);

        /* Add hashes together, and store. */
        cur_hash0 = vaddq_u32(prev_hash0, cur_hash0);
        cur_hash1 = vaddq_u32(prev_hash1, cur_hash1);
        vst1q_u32(m_intermediate_hash + 0, cur_hash0);
        vst1q_u32(m_intermediate_hash + 4, cur_hash1);
    }

    void Sha256Impl::ProcessLastBlock() {
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