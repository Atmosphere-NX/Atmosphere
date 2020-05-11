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

#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util.hpp>
#include <vapours/crypto/impl/crypto_hash_function.hpp>

namespace ams::crypto::impl {

    template<typename Hash> requires HashFunction<Hash>
    class RsaOaepImpl {
        NON_COPYABLE(RsaOaepImpl);
        NON_MOVEABLE(RsaOaepImpl);
        public:
            static constexpr size_t HashSize = Hash::HashSize;
        private:
            static constexpr u8 HeadMagic = 0x00;
        private:
            static void ComputeHashWithPadding(void *dst, Hash *hash, const void *salt, size_t salt_size) {
                /* Initialize our buffer. */
                u8 buf[8 + HashSize];
                std::memset(buf, 0, 8);
                hash->GetHash(buf + 8, HashSize);
                ON_SCOPE_EXIT { ClearMemory(buf, sizeof(buf)); };


                /* Calculate our hash. */
                hash->Initialize();
                hash->Update(buf, sizeof(buf));
                hash->Update(salt, salt_size);
                hash->GetHash(dst, HashSize);
            }

            static void ApplyMGF1(u8 *dst, size_t dst_size, const void *src, size_t src_size) {
                u8 buf[HashSize];
                ON_SCOPE_EXIT { ClearMemory(buf, sizeof(buf)); };

                const size_t required_iters = (dst_size + HashSize - 1) / HashSize;
                for (size_t i = 0; i < required_iters; i++) {
                    Hash hash;
                    hash.Initialize();
                    hash.Update(src, src_size);

                    const u32 tmp = util::ConvertToBigEndian(static_cast<u32>(i));
                    hash.Update(std::addressof(tmp), sizeof(tmp));

                    hash.GetHash(buf, HashSize);

                    const size_t start = HashSize * i;
                    const size_t end   = std::min(dst_size, start + HashSize);
                    for (size_t j = start; j < end; j++) {
                        dst[j] ^= buf[j - start];
                    }
                }
            }
        public:
            RsaOaepImpl() { /* ... */ }

            void Encode(void *dst, size_t dst_size, Hash *hash, const void *src, size_t src_size, const void *salt, size_t salt_size) {
                u8 label_digest[HashSize];
                ON_SCOPE_EXIT { ClearMemory(label_digest, HashSize); };

                hash->GetHash(label_digest, HashSize);
                return this->Encode(dst, dst_size, label_digest, sizeof(label_digest), src, src_size, salt, salt_size);
            }

            void Encode(void *dst, size_t dst_size, const void *label_digest, size_t label_digest_size, const void *src, size_t src_size, const void *salt, size_t salt_size) {
                /* Check our preconditions. */
                AMS_ASSERT(dst_size >= 2 * HashSize + 2 + src_size);
                AMS_ASSERT(salt_size > 0);
                AMS_ASSERT(salt_size == HashSize);
                AMS_ASSERT(label_digest_size == HashSize);

                u8 *buf = static_cast<u8 *>(dst);
                buf[0] = HeadMagic;

                u8 *seed = buf + 1;
                std::memcpy(seed, salt, HashSize);

                u8 *db = seed + HashSize;
                std::memcpy(db, label_digest, HashSize);
                std::memset(db + HashSize, 0, dst_size - 2 * HashSize - 2 - src_size);

                u8 *msg = buf + dst_size - src_size - 1;
                *(msg++) = 0x01;
                std::memcpy(msg, src, src_size);

                ApplyMGF1(db, dst_size - (1 + HashSize), seed, HashSize);
                ApplyMGF1(seed, HashSize, db, dst_size - (1 + HashSize));
            }

            size_t Decode(void *dst, size_t dst_size, const void *label_digest, size_t label_digest_size, u8 *buf, size_t buf_size) {
                /* Check our preconditions. */
                AMS_ABORT_UNLESS(dst_size > 0);
                AMS_ABORT_UNLESS(buf_size >= 2 * HashSize + 3);
                AMS_ABORT_UNLESS(label_digest_size == HashSize);

                /* Validate sanity byte. */
                bool is_valid = buf[0] == HeadMagic;

                /* Decrypt seed and masked db. */
                size_t db_len = buf_size - HashSize - 1;
                u8 *seed = buf + 1;
                u8 *db   = seed + HashSize;
                ApplyMGF1(seed, HashSize, db, db_len);
                ApplyMGF1(db, db_len, seed, HashSize);

                /* Check the label digest. */
                is_valid &= IsSameBytes(label_digest, db, HashSize);

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

    };

}
