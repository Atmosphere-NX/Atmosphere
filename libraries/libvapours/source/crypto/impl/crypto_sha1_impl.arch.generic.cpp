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

namespace ams::crypto::impl {

    namespace {

        constexpr const u32 RoundConstants[4] = {
            0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
        };

        constexpr ALWAYS_INLINE u32 Choose(u32 x, u32 y, u32 z) {
            return (x & y) ^ ((~x) & z);
        }

        constexpr ALWAYS_INLINE u32 Majority(u32 x, u32 y, u32 z) {
            return (x & y) ^ (x & z) ^ (y & z);
        }

        constexpr ALWAYS_INLINE u32 Parity(u32 x, u32 y, u32 z) {
            return x ^ y ^ z;
        }

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

        /* Process blocks, while we have any. */
        while (remaining >= BlockSize) {
            this->ProcessBlock(data8);
            data8     += BlockSize;
            remaining -= BlockSize;
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

    void Sha1Impl::ProcessBlock(const void *data) {
        /* Load work variables. */
        u32 a = m_intermediate_hash[0];
        u32 b = m_intermediate_hash[1];
        u32 c = m_intermediate_hash[2];
        u32 d = m_intermediate_hash[3];
        u32 e = m_intermediate_hash[4];
        u32 tmp;
        size_t i;

        /* Copy the input. */
        u32 w[80];
        if constexpr (util::IsLittleEndian()) {
            static_assert(BlockSize % sizeof(u32) == 0);

            const u32 *src_32 = static_cast<const u32 *>(data);
            for (size_t i = 0; i < BlockSize / sizeof(u32); ++i) {
                w[i] = util::LoadBigEndian<u32>(src_32 + i);
            }
        } else {
            std::memcpy(w, data, BlockSize);
        }

        /* Initialize the rest of w. */
        for (i = BlockSize / sizeof(u32); i < util::size(w); ++i) {
            const u32 *prev = w + (i - BlockSize / sizeof(u32));
            w[i] = util::RotateLeft<u32>(prev[0] ^ prev[2] ^ prev[8] ^ prev[13], 1);
        }

        /* Perform rounds. */
        for (i = 0; i < 20; ++i) {
            tmp = util::RotateLeft<u32>(a, 5) + Choose(b, c, d) + e + w[i] + RoundConstants[0];
            e = d;
            d = c;
            c = util::RotateLeft<u32>(b, 30);
            b = a;
            a = tmp;
        }

        for (/* ... */; i < 40; ++i) {
            tmp = util::RotateLeft<u32>(a, 5) + Parity(b, c, d) + e + w[i] + RoundConstants[1];
            e = d;
            d = c;
            c = util::RotateLeft<u32>(b, 30);
            b = a;
            a = tmp;
        }

        for (/* ... */; i < 60; ++i) {
            tmp = util::RotateLeft<u32>(a, 5) + Majority(b, c, d) + e + w[i] + RoundConstants[2];
            e = d;
            d = c;
            c = util::RotateLeft<u32>(b, 30);
            b = a;
            a = tmp;
        }

        for (/* ... */; i < 80; ++i) {
            tmp = util::RotateLeft<u32>(a, 5) + Parity(b, c, d) + e + w[i] + RoundConstants[3];
            e = d;
            d = c;
            c = util::RotateLeft<u32>(b, 30);
            b = a;
            a = tmp;
        }

        /* Update intermediate hash. */
        m_intermediate_hash[0] += a;
        m_intermediate_hash[1] += b;
        m_intermediate_hash[2] += c;
        m_intermediate_hash[3] += d;
        m_intermediate_hash[4] += e;
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
