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
#include <vapours/crypto/crypto_rsa_calculator.hpp>
#include <vapours/crypto/crypto_rsa_oaep_decryptor.hpp>
#include <vapours/crypto/crypto_sha256_generator.hpp>

namespace ams::crypto {

    namespace impl {

        template<size_t Bits>
        using RsaNOaepSha256Decryptor = ::ams::crypto::RsaOaepDecryptor<Bits / BITSIZEOF(u8), ::ams::crypto::Sha256Generator>;

    }

    using Rsa2048OaepSha256Decryptor = ::ams::crypto::impl::RsaNOaepSha256Decryptor<2048>;
    using Rsa4096OaepSha256Decryptor = ::ams::crypto::impl::RsaNOaepSha256Decryptor<4096>;

    inline size_t DecryptRsa2048OaepSha256(void *dst, size_t dst_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, const void *lab, size_t lab_size) {
        return Rsa2048OaepSha256Decryptor::Decrypt(dst, dst_size, mod, mod_size, exp, exp_size, msg, msg_size, lab, lab_size);
    }

    inline size_t DecryptRsa2048OaepSha256(void *dst, size_t dst_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, const void *lab, size_t lab_size, void *work_buf, size_t work_buf_size) {
        return Rsa2048OaepSha256Decryptor::Decrypt(dst, dst_size, mod, mod_size, exp, exp_size, msg, msg_size, lab, lab_size, work_buf, work_buf_size);
    }

    inline size_t DecryptRsa4096OaepSha256(void *dst, size_t dst_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, const void *lab, size_t lab_size) {
        return Rsa4096OaepSha256Decryptor::Decrypt(dst, dst_size, mod, mod_size, exp, exp_size, msg, msg_size, lab, lab_size);
    }

    inline size_t DecryptRsa4096OaepSha256(void *dst, size_t dst_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, const void *lab, size_t lab_size, void *work_buf, size_t work_buf_size) {
        return Rsa4096OaepSha256Decryptor::Decrypt(dst, dst_size, mod, mod_size, exp, exp_size, msg, msg_size, lab, lab_size, work_buf, work_buf_size);
    }

}
