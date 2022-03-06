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

        constexpr auto NumRounds = 24;

        constexpr const u64 IotaRoundConstant[NumRounds] = {
            UINT64_C(0x0000000000000001), UINT64_C(0x0000000000008082),
            UINT64_C(0x800000000000808A), UINT64_C(0x8000000080008000),
            UINT64_C(0x000000000000808B), UINT64_C(0x0000000080000001),
            UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008009),
            UINT64_C(0x000000000000008A), UINT64_C(0x0000000000000088),
            UINT64_C(0x0000000080008009), UINT64_C(0x000000008000000A),
            UINT64_C(0x000000008000808B), UINT64_C(0x800000000000008B),
            UINT64_C(0x8000000000008089), UINT64_C(0x8000000000008003),
            UINT64_C(0x8000000000008002), UINT64_C(0x8000000000000080),
            UINT64_C(0x000000000000800A), UINT64_C(0x800000008000000A),
            UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008080),
            UINT64_C(0x0000000080000001), UINT64_C(0x8000000080008008)
        };

        constexpr const int RhoShiftBit[NumRounds] = {
             1,  3,  6, 10, 15, 21, 28, 36,
            45, 55,  2, 14, 27, 41, 56,  8,
            25, 43, 62, 18, 39, 61, 20, 44
        };

        constexpr const int RhoNextIndex[NumRounds] = {
            10,  7, 11, 17, 18,  3,  5, 16,
             8, 21, 24,  4, 15, 23, 19, 13,
            12,  2, 20, 14, 22,  9,  6,  1
        };

    }

    template<size_t HashSize>
    void Sha3Impl<HashSize>::Initialize() {
        /* Clear internal state. */
        std::memset(m_internal_state, 0, sizeof(m_internal_state));

        /* Reset buffered bytes. */
        m_buffered_bytes = 0;

        /* Set state. */
        m_state = State_Initialized;
    }

    template<size_t HashSize>
    void Sha3Impl<HashSize>::Update(const void *data, size_t size) {
        /* Verify we're in a state to update. */
        AMS_ASSERT(m_state == State_Initialized);

        /* Process we have anything buffered. */
        const u8 *data8 = static_cast<const u8 *>(data);
        size_t remaining = size;
        if (m_buffered_bytes > 0) {
            /* Determine how much we can copy. */
            const size_t copy_size = std::min(BlockSize - m_buffered_bytes, remaining);

            /* Mix the bytes into our state. */
            u8 *dst8 = reinterpret_cast<u8 *>(m_internal_state) + m_buffered_bytes;
            for (size_t i = 0; i < copy_size; ++i) {
                dst8[i] ^= data8[i];
            }

            /* Advance. */
            data8            += copy_size;
            remaining        -= copy_size;
            m_buffered_bytes += copy_size;

            /* Process a block, if we filled one. */
            if (m_buffered_bytes == BlockSize) {
                this->ProcessBlock();
                m_buffered_bytes = 0;
            }
        }

        /* Process blocks, if we have any. */
        while (remaining >= BlockSize) {
            /* Mix the bytes into our state. */
            u8 *dst8 = reinterpret_cast<u8 *>(m_internal_state);
            for (size_t i = 0; i < BlockSize; ++i) {
                dst8[i] ^= data8[i];
            }

            this->ProcessBlock();

            data8     += BlockSize;
            remaining -= BlockSize;
        }

        /* Copy any leftover data to our buffer. */
        if (remaining > 0) {
            u8 *dst8 = reinterpret_cast<u8 *>(m_internal_state);
            for (size_t i = 0; i < remaining; ++i) {
                dst8[i] ^= data8[i];
            }

            m_buffered_bytes = remaining;
        }
    }

    template<size_t HashSize>
    void Sha3Impl<HashSize>::GetHash(void *dst, size_t size) {
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
        std::memcpy(dst, m_internal_state, HashSize);
    }

    template<size_t HashSize>
    void Sha3Impl<HashSize>::InitializeWithContext(const Sha3Context *context) {
        /* Check the context is for the right hash size. */
        AMS_ASSERT(context->hash_size == HashSize);

        /* Set buffered bytes. */
        m_buffered_bytes = context->buffered_bytes;

        /* Copy state in from the context. */
        std::memcpy(m_internal_state, context->internal_state, sizeof(m_internal_state));

        /* Reset other fields. */
        m_state = State_Initialized;
    }

    template<size_t HashSize>
    void Sha3Impl<HashSize>::GetContext(Sha3Context *context) const {
        /* Check our state. */
        AMS_ASSERT(m_state == State_Initialized);

        /* Set the output hash size. */
        context->hash_size = HashSize;

        /* Set buffered bytes. */
        context->buffered_bytes = m_buffered_bytes;

        /* Copy out the context. */
        std::memcpy(context->internal_state, m_internal_state, sizeof(context->internal_state));
    }

    template<size_t HashSize>
    void Sha3Impl<HashSize>::ProcessBlock() {
        /* Ensure correct endianness. */
        if constexpr (util::IsBigEndian()) {
            for (size_t i = 0; i < util::size(m_internal_state); ++i) {
                m_internal_state[i] = util::LoadLittleEndian<u64>(m_internal_state + i);
            }
        }

        /* Perform all rounds. */
        uint64_t tmp, C[5];
        for (auto round = 0; round < NumRounds; ++round) {
            /* Handle theta. */
            for (size_t i = 0; i < 5; ++i) {
                C[i] = m_internal_state[i] ^ m_internal_state[i + 5] ^ m_internal_state[i + 10] ^ m_internal_state[i + 15] ^ m_internal_state[i + 20];
            }

            for (size_t i = 0; i < 5; ++i) {
                tmp = C[(i + 4) % 5] ^ util::RotateLeft<u64>(C[(i + 1) % 5], 1);
                for (size_t j = 0; j < 5; ++j) {
                    m_internal_state[5 * j + i] ^= tmp;
                }
            }

            /* Handle rho/pi. */
            tmp = m_internal_state[1];
            for (size_t i = 0; i < NumRounds; ++i) {
                const auto rho_next_idx = RhoNextIndex[i];
                C[0] = m_internal_state[rho_next_idx];
                m_internal_state[rho_next_idx] = util::RotateLeft<u64>(tmp, RhoShiftBit[i]);
                tmp = C[0];
            }

            /* Handle chi. */
            for (size_t i = 0; i < 5; ++i) {
                for (size_t j = 0; j < 5; ++j) {
                    C[j] = m_internal_state[5 * i + j];
                }
                for (size_t j = 0; j < 5; ++j) {
                    m_internal_state[5 * i + j] ^= (~C[(j + 1) % 5]) & C[(j + 2) % 5];
                }
            }

            /* Handle iota. */
            m_internal_state[0] ^= IotaRoundConstant[round];
        }
        /* Ensure correct endianness. */
        if constexpr (util::IsBigEndian()) {
            for (size_t i = 0; i < util::size(m_internal_state); ++i) {
                util::StoreLittleEndian<u64>(m_internal_state + i, m_internal_state[i]);
            }
        }
    }

    template<size_t HashSize>
    void Sha3Impl<HashSize>::ProcessLastBlock() {
        /* Mix final bits (011) into our state. */
        reinterpret_cast<u8 *>(m_internal_state)[m_buffered_bytes] ^= 0b110;

        /* Mix in the high bit of the last word in our block. */
        constexpr u64 FinalMask = UINT64_C(0x8000000000000000);
        m_internal_state[(BlockSize / sizeof(u64)) - 1] ^= FinalMask;

        /* Process the last block. */
        this->ProcessBlock();
    }

    /* Explicitly instantiate the supported hash sizes. */
    template class Sha3Impl<224 / BITSIZEOF(u8)>;
    template class Sha3Impl<256 / BITSIZEOF(u8)>;
    template class Sha3Impl<384 / BITSIZEOF(u8)>;
    template class Sha3Impl<512 / BITSIZEOF(u8)>;

}
