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

        struct Md5Constants {
            static constexpr const u32 A = 0x67452301;
            static constexpr const u32 B = 0xEFCDAB89;
            static constexpr const u32 C = 0x98BADCFE;
            static constexpr const u32 D = 0x10325476;

            static constexpr const u32 T[] = {
                0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
                0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
                0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
                0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
                0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
                0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
                0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
                0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
                0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
                0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
                0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
                0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
                0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
                0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
                0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
                0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391,
            };

            static constexpr u32 K[] = {
                0x1, 0x6, 0xB, 0x0,
                0x5, 0xA, 0xF, 0x4,
                0x9, 0xE, 0x3, 0x8,
                0xD, 0x2, 0x7, 0xC,
                0x5, 0x8, 0xB, 0xE,
                0x1, 0x4, 0x7, 0xA,
                0xD, 0x0, 0x3, 0x6,
                0x9, 0xC, 0xF, 0x2,
                0x0, 0x7, 0xE, 0x5,
                0xC, 0x3, 0xA, 0x1,
                0x8, 0xF, 0x6, 0xD,
                0x4, 0xB, 0x2, 0x9,
            };

            static constexpr u8 Padding[] = {
                0x80
            };
        };

        constexpr ALWAYS_INLINE u32 F(u32 x, u32 y, u32 z) { return (x & y) | ((~x) & z); }
        constexpr ALWAYS_INLINE u32 G(u32 x, u32 y, u32 z) { return (x & z) | (y & (~z)); }
        constexpr ALWAYS_INLINE u32 H(u32 x, u32 y, u32 z) { return x ^ y ^ z; }
        constexpr ALWAYS_INLINE u32 I(u32 x, u32 y, u32 z) { return y ^ (x | (~z)); }

        constexpr ALWAYS_INLINE u32 CalculateRound1(u32 a, u32 b, u32 c, u32 d, u32 x, u32 s, u32 t) { return b + util::RotateLeft<u32>(a + F(b, c, d) + x + t, s); }
        constexpr ALWAYS_INLINE u32 CalculateRound2(u32 a, u32 b, u32 c, u32 d, u32 x, u32 s, u32 t) { return b + util::RotateLeft<u32>(a + G(b, c, d) + x + t, s); }
        constexpr ALWAYS_INLINE u32 CalculateRound3(u32 a, u32 b, u32 c, u32 d, u32 x, u32 s, u32 t) { return b + util::RotateLeft<u32>(a + H(b, c, d) + x + t, s); }
        constexpr ALWAYS_INLINE u32 CalculateRound4(u32 a, u32 b, u32 c, u32 d, u32 x, u32 s, u32 t) { return b + util::RotateLeft<u32>(a + I(b, c, d) + x + t, s); }

        void Encode(u32 *dst, const u32 *src, size_t size) {
            if constexpr (util::IsBigEndian()) {
                for (size_t i = 0; i < size; i += sizeof(u32)) {
                    util::StoreLittleEndian(dst + i, src[i]);
                }
            } else {
                std::memcpy(dst, src, size);
            }
        }

        void Decode(u32 *dst, const u32 *src, size_t size) {
            if constexpr (util::IsBigEndian()) {
                for (size_t i = 0; i < size; i += sizeof(u32)) {
                    dst[i] = util::LoadLittleEndian(src + i);
                }
            } else {
                std::memcpy(dst, src, size);
            }
        }

    }

    void Md5Impl::Initialize() {
        /* Set constants. */
        m_x.p.a = Md5Constants::A;
        m_x.p.b = Md5Constants::B;
        m_x.p.c = Md5Constants::C;
        m_x.p.d = Md5Constants::D;

        /* Set size. */
        m_size = 0;

        /* Set initialized. */
        m_state = State_Initialized;
    }

    void Md5Impl::Update(const void *data, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(m_state == State_Initialized);

        /* Determine how much we can process. */
        const size_t work_idx       = m_size % BlockSize;
        const size_t work_remaining = BlockSize - work_idx;

        /* Increment our size. */
        m_size += size;

        /* Copy in the data to our buffer, if we don't have a full block. */
        if (work_remaining > size) {
            if (size > 0) {
                std::memcpy(m_y + work_idx, data, size);
            }
            return;
        }

        /* Copy what we can to complete our block. */
        std::memcpy(m_y + work_idx, data, work_remaining);

        /* Process the block. */
        this->ProcessBlock();

        /* Adjust size to account for what we've processed. */
        size -= work_remaining;

        /* Process as many full blocks as we can. */
        const u8 *cur_block = static_cast<const u8 *>(data) + work_remaining;
        for (size_t i = 0; i < size / BlockSize; ++i) {
            std::memcpy(m_y, cur_block, BlockSize);
            cur_block += BlockSize;

            this->ProcessBlock();
        }

        /* Copy in any leftover data. */
        if (const auto left = size % BlockSize; left > 0) {
            std::memcpy(m_y, cur_block, left);
        }

    }

    void Md5Impl::GetHash(void *dst, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(m_state == State_Initialized || m_state == State_Done);
        AMS_ASSERT(size >= HashSize);
        AMS_UNUSED(size);

        /* If we need to, finish processing. */
        if (m_state == State_Initialized) {
            this->ProcessLastBlock();
            m_state = State_Done;
        }

        /* Encode the result. */
        Encode(static_cast<u32 *>(dst), m_x.state, HashSize);
    }

    void Md5Impl::ProcessBlock() {
        /* Declare tracking pointers for rounds. */
        u32 x[BlockSize / sizeof(u32)];
        const u32 *p_t = Md5Constants::T;
        const u32 *p_k = Md5Constants::K;
        const u32 *p_x = x;

        /* Extract current state. */
        u32 a = m_x.p.a;
        u32 b = m_x.p.b;
        u32 c = m_x.p.c;
        u32 d = m_x.p.d;

        /* Decode the block into native endian. */
        Decode(x, reinterpret_cast<const u32 *>(m_y), BlockSize);

        /* Perform round 1. */
        for (size_t i = 0; i < 4; ++i) {
            a = CalculateRound1(a, b, c, d, *p_x++,  7, *p_t++);
            d = CalculateRound1(d, a, b, c, *p_x++, 12, *p_t++);
            c = CalculateRound1(c, d, a, b, *p_x++, 17, *p_t++);
            b = CalculateRound1(b, c, d, a, *p_x++, 22, *p_t++);
        }

        /* Perform round 2. */
        for (size_t i = 0; i < 4; ++i) {
            a = CalculateRound2(a, b, c, d, x[*p_k++],  5, *p_t++);
            d = CalculateRound2(d, a, b, c, x[*p_k++],  9, *p_t++);
            c = CalculateRound2(c, d, a, b, x[*p_k++], 14, *p_t++);
            b = CalculateRound2(b, c, d, a, x[*p_k++], 20, *p_t++);
        }

        /* Perform round 3. */
        for (size_t i = 0; i < 4; ++i) {
            a = CalculateRound3(a, b, c, d, x[*p_k++],  4, *p_t++);
            d = CalculateRound3(d, a, b, c, x[*p_k++], 11, *p_t++);
            c = CalculateRound3(c, d, a, b, x[*p_k++], 16, *p_t++);
            b = CalculateRound3(b, c, d, a, x[*p_k++], 23, *p_t++);
        }

        /* Perform round 4. */
        for (size_t i = 0; i < 4; ++i) {
            a = CalculateRound4(a, b, c, d, x[*p_k++],  6, *p_t++);
            d = CalculateRound4(d, a, b, c, x[*p_k++], 10, *p_t++);
            c = CalculateRound4(c, d, a, b, x[*p_k++], 15, *p_t++);
            b = CalculateRound4(b, c, d, a, x[*p_k++], 21, *p_t++);
        }

        /* Mix the result back into our state. */
        m_x.p.a += a;
        m_x.p.b += b;
        m_x.p.c += c;
        m_x.p.d += d;
    }

    void Md5Impl::ProcessLastBlock() {
        /* Get bit count. */
        const u64 bit_count = m_size * BITSIZEOF(u8);

        /* Add padding byte unconditionally. */
        this->Update(Md5Constants::Padding, sizeof(Md5Constants::Padding));

        /* Determine remaining. */
        size_t work_idx       = m_size % BlockSize;
        size_t work_remaining = BlockSize - work_idx;

        /* We want to process 8000.....{bit count}. */
        if (work_remaining < sizeof(u64)) {
            std::memset(m_y + work_idx, 0, work_remaining);
            this->ProcessBlock();
            work_idx       = 0;
            work_remaining = BlockSize;
        }
        if (work_remaining > sizeof(u64)) {
            std::memset(m_y + work_idx, 0, work_remaining - sizeof(u64));
        }

        util::StoreLittleEndian<u64>(reinterpret_cast<u64 *>(m_y + BlockSize - sizeof(u64)), bit_count);

        this->ProcessBlock();
    }

}