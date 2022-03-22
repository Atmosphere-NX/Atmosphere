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

namespace ams::crypto::impl {

    class Sha256CompileTimeImpl {
        public:
            static constexpr size_t HashSize  = 0x20;
            static constexpr size_t BlockSize = 0x40;
        private:
            enum State {
                State_None,
                State_Initialized,
                State_Done,
            };
        private:
            u32 m_intermediate_hash[HashSize / sizeof(u32)];
            u8 m_buffer[BlockSize];
            size_t m_buffered_bytes;
            u64 m_bits_consumed;
            State m_state;
        public:
            constexpr Sha256CompileTimeImpl() : m_intermediate_hash(), m_buffer(), m_buffered_bytes(), m_bits_consumed(), m_state(State_None) {
                /* ... */
            }

            constexpr void Initialize() {
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

            template<typename T, typename = typename std::enable_if<std::same_as<T, u8> || std::same_as<T, s8> || std::same_as<T, char> || std::same_as<T, unsigned char>>::type>
            constexpr void Update(const T *data, size_t size) {
                static_assert(sizeof(T) == 1);

                /* Verify we're in a state to update. */
                AMS_ASSERT(m_state == State_Initialized);

                /* Advance our input bit count. */
                m_bits_consumed += BITSIZEOF(u8) * (((m_buffered_bytes + size) / BlockSize) * BlockSize);

                /* Process anything we have buffered. */
                size_t remaining = size;

                if (m_buffered_bytes > 0) {
                    const size_t copy_size = std::min(BlockSize - m_buffered_bytes, remaining);
                    for (size_t i = 0; i < copy_size; ++i) {
                        m_buffer[m_buffered_bytes + i] = static_cast<u8>(data[i]);
                    }

                    data             += copy_size;
                    remaining        -= copy_size;
                    m_buffered_bytes += copy_size;

                    /* Process a block, if we filled one. */
                    if (m_buffered_bytes == BlockSize) {
                        this->ProcessBlock(m_buffer);
                        m_buffered_bytes = 0;
                    }
                }

                /* Process blocks, if we have any. */
                while (remaining >= BlockSize) {
                    u8 block[BlockSize] = {};
                    for (size_t i = 0; i < BlockSize; ++i) {
                        block[i] = static_cast<u8>(data[i]);
                    }
                    this->ProcessBlock(block);

                    data      += BlockSize;
                    remaining -= BlockSize;
                }

                /* Copy any leftover data to our buffer. */
                if (remaining > 0) {
                    m_buffered_bytes = remaining;
                    for (size_t i = 0; i < remaining; ++i) {
                        m_buffer[i] = static_cast<u8>(data[i]);
                    }
                }
            }

            constexpr void GetHash(u8 *dst, size_t size) {
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
                for (size_t i = 0; i < HashSize / sizeof(u32); ++i) {
                    const u32 v = m_intermediate_hash[i];

                    dst[sizeof(u32) * i + 3] = static_cast<u8>(v >> (BITSIZEOF(u8) * 0));
                    dst[sizeof(u32) * i + 2] = static_cast<u8>(v >> (BITSIZEOF(u8) * 1));
                    dst[sizeof(u32) * i + 1] = static_cast<u8>(v >> (BITSIZEOF(u8) * 2));
                    dst[sizeof(u32) * i + 0] = static_cast<u8>(v >> (BITSIZEOF(u8) * 3));
                }
            }

            constexpr size_t GetBufferedDataSize() const { return m_buffered_bytes; }

            constexpr void GetBufferedData(u8 *dst, size_t dst_size) const {
                AMS_ASSERT(dst_size >= this->GetBufferedDataSize());
                AMS_UNUSED(dst_size);

                for (size_t i = 0; i < m_buffered_bytes; ++i) {
                    dst[i] = m_buffer[i];
                }
            }
        private:
            static constexpr ALWAYS_INLINE u32 Choose(u32 x, u32 y, u32 z) {
                return (x & y) ^ ((~x) & z);
            }

            static constexpr ALWAYS_INLINE u32 Majority(u32 x, u32 y, u32 z) {
                return (x & y) ^ (x & z) ^ (y & z);
            }

            static constexpr ALWAYS_INLINE u32 LargeSigma0(u32 x) {
                return util::RotateRight<u32>(x, 2) ^ util::RotateRight<u32>(x, 13) ^ util::RotateRight<u32>(x, 22);
            }

            static constexpr ALWAYS_INLINE u32 LargeSigma1(u32 x) {
                return util::RotateRight<u32>(x, 6) ^ util::RotateRight<u32>(x, 11) ^ util::RotateRight<u32>(x, 25);
            }

            static constexpr ALWAYS_INLINE u32 SmallSigma0(u32 x) {
                return util::RotateRight<u32>(x, 7) ^ util::RotateRight<u32>(x, 18) ^ (x >> 3);
            }

            static constexpr ALWAYS_INLINE u32 SmallSigma1(u32 x) {
                return util::RotateRight<u32>(x, 17) ^ util::RotateRight<u32>(x, 19) ^ (x >> 10);
            }

            constexpr void ProcessBlock(const u8 *data) {
                /* Load work variables. */
                u32 a = m_intermediate_hash[0];
                u32 b = m_intermediate_hash[1];
                u32 c = m_intermediate_hash[2];
                u32 d = m_intermediate_hash[3];
                u32 e = m_intermediate_hash[4];
                u32 f = m_intermediate_hash[5];
                u32 g = m_intermediate_hash[6];
                u32 h = m_intermediate_hash[7];
                u32 tmp[2]{};
                size_t i = 0;

                /* Copy the input. */
                u32 w[64]{};
                for (size_t i = 0; i < BlockSize / sizeof(u32); ++i) {
                    u32 v = 0;
                    v |= static_cast<u32>(data[sizeof(u32) * i + 0]) << (BITSIZEOF(u8) * 3);
                    v |= static_cast<u32>(data[sizeof(u32) * i + 1]) << (BITSIZEOF(u8) * 2);
                    v |= static_cast<u32>(data[sizeof(u32) * i + 2]) << (BITSIZEOF(u8) * 1);
                    v |= static_cast<u32>(data[sizeof(u32) * i + 3]) << (BITSIZEOF(u8) * 0);

                    w[i] = v;
                }

                /* Initialize the rest of w. */
                for (i = BlockSize / sizeof(u32); i < util::size(w); ++i) {
                    const u32 *prev = w + (i - BlockSize / sizeof(u32));
                    w[i] = prev[0] + SmallSigma0(prev[1]) + prev[9] + SmallSigma1(prev[14]);
                }

                /* Perform rounds. */
                {
                    const u32 RoundConstants[0x40] = {
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

                    for (i = 0; i < 64; ++i) {
                        tmp[0] = h + LargeSigma1(e) + Choose(e, f, g) + RoundConstants[i] + w[i];
                        tmp[1] = LargeSigma0(a) + Majority(a, b, c);

                        h = g;
                        g = f;
                        f = e;
                        e = d + tmp[0];
                        d = c;
                        c = b;
                        b = a;
                        a = tmp[0] + tmp[1];
                    }
                }

                /* Update intermediate hash. */
                m_intermediate_hash[0] += a;
                m_intermediate_hash[1] += b;
                m_intermediate_hash[2] += c;
                m_intermediate_hash[3] += d;
                m_intermediate_hash[4] += e;
                m_intermediate_hash[5] += f;
                m_intermediate_hash[6] += g;
                m_intermediate_hash[7] += h;
            }

            constexpr void ProcessLastBlock() {
                /* Setup the final block. */
                constexpr const auto BlockSizeWithoutSizeField = BlockSize - sizeof(u64);

                /* Increment our bits consumed. */
                m_bits_consumed += BITSIZEOF(u8) * m_buffered_bytes;

                /* Add 0x80 terminator. */
                m_buffer[m_buffered_bytes++] = 0x80;

                /* If we can process the size field directly, do so, otherwise set up to process it. */
                if (m_buffered_bytes <= BlockSizeWithoutSizeField) {
                    /* Clear up to size field. */
                    for (size_t i = 0; i < BlockSizeWithoutSizeField - m_buffered_bytes; ++i) {
                        m_buffer[m_buffered_bytes + i] = 0;
                    }
                } else {
                    /* Consume full block */
                    for (size_t i = 0; i < BlockSize - m_buffered_bytes; ++i) {
                        m_buffer[m_buffered_bytes + i] = 0;
                    }

                    this->ProcessBlock(m_buffer);

                    /* Clear up to size field. */
                    for (size_t i = 0; i < BlockSizeWithoutSizeField; ++i) {
                        m_buffer[i] = 0;
                    }
                }

                /* Store the size field. */
                m_buffer[BlockSizeWithoutSizeField + 0] = static_cast<u8>((m_bits_consumed >> (BITSIZEOF(u8) * 7)) & 0xFF);
                m_buffer[BlockSizeWithoutSizeField + 1] = static_cast<u8>((m_bits_consumed >> (BITSIZEOF(u8) * 6)) & 0xFF);
                m_buffer[BlockSizeWithoutSizeField + 2] = static_cast<u8>((m_bits_consumed >> (BITSIZEOF(u8) * 5)) & 0xFF);
                m_buffer[BlockSizeWithoutSizeField + 3] = static_cast<u8>((m_bits_consumed >> (BITSIZEOF(u8) * 4)) & 0xFF);
                m_buffer[BlockSizeWithoutSizeField + 4] = static_cast<u8>((m_bits_consumed >> (BITSIZEOF(u8) * 3)) & 0xFF);
                m_buffer[BlockSizeWithoutSizeField + 5] = static_cast<u8>((m_bits_consumed >> (BITSIZEOF(u8) * 2)) & 0xFF);
                m_buffer[BlockSizeWithoutSizeField + 6] = static_cast<u8>((m_bits_consumed >> (BITSIZEOF(u8) * 1)) & 0xFF);
                m_buffer[BlockSizeWithoutSizeField + 7] = static_cast<u8>((m_bits_consumed >> (BITSIZEOF(u8) * 0)) & 0xFF);

                /* Process the final block. */
                this->ProcessBlock(m_buffer);
            }
    };

}
