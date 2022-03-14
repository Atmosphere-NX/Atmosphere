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
#include <vapours/crypto/impl/crypto_hash_function.hpp>

namespace ams::crypto::impl {

    template<HashFunction Hash>
    class RsaPkcs1Impl {
        NON_COPYABLE(RsaPkcs1Impl);
        NON_MOVEABLE(RsaPkcs1Impl);
        public:
            static constexpr size_t HashSize = Hash::HashSize;
        public:
            RsaPkcs1Impl() { /* ... */ }
            ~RsaPkcs1Impl() { /* ... */ }

            void BuildPad(void *out_block, size_t block_size, Hash *hash) {
                AMS_ASSERT(block_size >= 2 + 1 + sizeof(Hash::Asn1Identifier) + HashSize);

                u8 *dst = static_cast<u8 *>(out_block);
                *(dst++) = 0x00;
                *(dst++) = 0x01;

                const size_t pad_len = block_size - (2 + 1 + sizeof(Hash::Asn1Identifier) + HashSize);
                std::memset(dst, 0xFF, pad_len);
                dst += pad_len;

                *(dst++) = 0x00;

                std::memcpy(dst, Hash::Asn1Identifier, sizeof(Hash::Asn1Identifier));
                dst += sizeof(Hash::Asn1Identifier);

                hash->GetHash(dst, HashSize);
            }

            bool CheckPad(const u8 *src, size_t block_size, Hash *hash) {
                /* Check that block size is minimally big enough. */
                if (block_size < 2 + 1 + sizeof(Hash::Asn1Identifier) + HashSize) {
                    return false;
                }

                /* Check that the padding if correctly of form 0001FF..FF00 */
                if (*(src++) != 0x00) {
                    return false;
                }
                if (*(src++) != 0x01) {
                    return false;
                }

                const size_t pad_len = block_size - (2 + 1 + sizeof(Hash::Asn1Identifier) + HashSize);
                for (size_t i = 0; i < pad_len; ++i) {
                    if (*(src++) != 0xFF) {
                        return false;
                    }
                }

                if (*(src++) != 0x00) {
                    return false;
                }

                /* Check that the asn1 identifier matches. */
                if (std::memcmp(src, Hash::Asn1Identifier, sizeof(Hash::Asn1Identifier)) != 0) {
                    return false;
                }

                src += sizeof(Hash::Asn1Identifier);

                /* Check the hash. */
                u8 calc_hash[HashSize];
                hash->GetHash(calc_hash, sizeof(calc_hash));

                return std::memcmp(calc_hash, src, HashSize) == 0;
            }
    };

}
