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
#include <vapours/crypto/impl/crypto_rsa_oaep_impl.hpp>
#include <vapours/crypto/crypto_sha256_generator.hpp>

namespace ams::crypto {

    inline size_t DecodeRsa2048OaepSha256(void *dst, size_t dst_size, const void *label_digest, size_t label_digest_size, const void *src, size_t src_size) {
        constexpr size_t BlockSize = 2048 / BITSIZEOF(u8);
        AMS_ABORT_UNLESS(src_size == BlockSize);

        impl::RsaOaepImpl<Sha256Generator> oaep;
        u8 enc[BlockSize];
        ON_SCOPE_EXIT { ClearMemory(enc, sizeof(enc)); };

        std::memcpy(enc, src, src_size);
        return oaep.Decode(dst, dst_size, label_digest, label_digest_size, enc, sizeof(enc));
    }

    inline size_t DecodeRsa4096OaepSha256(void *dst, size_t dst_size, const void *label_digest, size_t label_digest_size, const void *src, size_t src_size) {
        constexpr size_t BlockSize = 4096 / BITSIZEOF(u8);
        AMS_ABORT_UNLESS(src_size == BlockSize);

        impl::RsaOaepImpl<Sha256Generator> oaep;
        u8 enc[BlockSize];
        ON_SCOPE_EXIT { ClearMemory(enc, sizeof(enc)); };

        std::memcpy(enc, src, src_size);
        return oaep.Decode(dst, dst_size, label_digest, label_digest_size, enc, sizeof(enc));
    }

}
