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
#include <exosphere.hpp>
#include "se_execute.hpp"

namespace ams::se {

    /* NOTE: This implementation is mostly copy/pasted from crypto::impl::RsaOaepImpl. */

    namespace {

        constexpr inline size_t HashSize = sizeof(Sha256Hash);

        constexpr inline u8 HeadMagic = 0x00;

        void ApplyMGF1(u8 *dst, size_t dst_size, const void *src, size_t src_size) {
            /* Check our pre-conditions. */
            AMS_ABORT_UNLESS(src_size <= RsaSize - (1 + HashSize));

            /* Create a buffer. */
            util::AlignedBuffer<hw::DataCacheLineSize, RsaSize - (1 + HashSize) + sizeof(u32)> buf;
            u32 counter = 0;

            while (dst_size > 0) {
                /* Setup the current hash buffer. */
                const size_t cur_size = std::min(HashSize, dst_size);
                std::memcpy(static_cast<u8 *>(buf), src, src_size);
                {
                    u32 counter_be;
                    util::StoreBigEndian(std::addressof(counter_be), counter++);
                    std::memcpy(static_cast<u8 *>(buf) + src_size, std::addressof(counter_be), sizeof(counter_be));
                }

                /* Ensure se sees correct data. */
                hw::FlushDataCache(buf, src_size + sizeof(u32));
                hw::DataSynchronizationBarrierInnerShareable();

                /* Calculate the hash. */
                Sha256Hash hash;
                se::CalculateSha256(std::addressof(hash), buf, src_size + sizeof(u32));

                /* Mask the current output. */
                const u8 *mask = hash.bytes;
                for (size_t i = 0; i < cur_size; ++i) {
                    *(dst++) ^= *(mask++);
                }

                /* Advance. */
                dst_size -= cur_size;
            }
        }

    }

    size_t DecodeRsaOaepSha256(void *dst, size_t dst_size, void *src, size_t src_size, const void *label_digest, size_t label_digest_size) {
        /* Check our preconditions. */
        AMS_ABORT_UNLESS(src_size == RsaSize);
        AMS_ABORT_UNLESS(label_digest_size == HashSize);

        /* Get a byte-readable copy of the input. */
        u8 *buf = static_cast<u8 *>(src);

        /* Validate sanity byte. */
        bool is_valid = buf[0] == HeadMagic;

        /* Decrypt seed and masked db. */
        size_t db_len = src_size - HashSize - 1;
        u8 *seed = buf + 1;
        u8 *db   = seed + HashSize;
        ApplyMGF1(seed, HashSize, db, db_len);
        ApplyMGF1(db, db_len, seed, HashSize);

        /* Check the label digest. */
        is_valid &= crypto::IsSameBytes(label_digest, db, HashSize);

        /* Skip past the label digest. */
        db     += HashSize;
        db_len -= HashSize;

        /* Verify that DB is of the form 0000...0001 < message > */
        s32 msg_ofs = 0;
        {
            int looking_for_one = 1;
            int invalid_db_padding = 0;
            int is_zero;
            int is_one;
            for (size_t i = 0; i < db_len; /* ... */) {
                is_zero = (db[i] == 0);
                is_one  = (db[i] == 1);
                msg_ofs += (looking_for_one & is_one) * (static_cast<s32>(++i));
                looking_for_one &= ~is_one;
                invalid_db_padding |= (looking_for_one & ~is_zero);
            }

            is_valid &= (invalid_db_padding == 0);
        }

        /* If we're invalid, return zero size. */
        const size_t valid_msg_size = db_len - msg_ofs;
        const size_t msg_size       = std::min(dst_size, static_cast<size_t>(is_valid) * valid_msg_size);

        /* Copy to output. */
        std::memcpy(dst, db + msg_ofs, msg_size);

        /* Return copied size. */
        return msg_size;
    }

}
