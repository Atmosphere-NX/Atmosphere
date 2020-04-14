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
#include <vapours/crypto/crypto_rsa_calculator.hpp>
#include <vapours/crypto/crypto_rsa_pss_verifier.hpp>
#include <vapours/crypto/crypto_sha256_generator.hpp>

namespace ams::crypto {

    namespace impl {

        template<size_t Bits>
        using RsaNPssSha256Verifier = ::ams::crypto::RsaPssVerifier<Bits / BITSIZEOF(u8), ::ams::crypto::Sha256Generator>;

    }

    using Rsa2048PssSha256Verifier = ::ams::crypto::impl::RsaNPssSha256Verifier<2048>;
    using Rsa4096PssSha256Verifier = ::ams::crypto::impl::RsaNPssSha256Verifier<4096>;

    inline bool VerifyRsa2048PssSha256(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size) {
        return Rsa2048PssSha256Verifier::Verify(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
    }

    inline bool VerifyRsa2048PssSha256(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, void *work_buf, size_t work_buf_size) {
        return Rsa2048PssSha256Verifier::Verify(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size, work_buf, work_buf_size);
    }

    inline bool VerifyRsa2048PssSha256WithHash(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *hash, size_t hash_size) {
        return Rsa2048PssSha256Verifier::VerifyWithHash(sig, sig_size, mod, mod_size, exp, exp_size, hash, hash_size);
    }

    inline bool VerifyRsa2048PssSha256WithHash(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *hash, size_t hash_size, void *work_buf, size_t work_buf_size) {
        return Rsa2048PssSha256Verifier::VerifyWithHash(sig, sig_size, mod, mod_size, exp, exp_size, hash, hash_size, work_buf, work_buf_size);
    }

    inline bool VerifyRsa4096PssSha256(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size) {
        return Rsa4096PssSha256Verifier::Verify(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
    }

    inline bool VerifyRsa4096PssSha256(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, void *work_buf, size_t work_buf_size) {
        return Rsa4096PssSha256Verifier::Verify(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size, work_buf, work_buf_size);
    }

    inline bool VerifyRsa4096PssSha256WithHash(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *hash, size_t hash_size) {
        return Rsa4096PssSha256Verifier::VerifyWithHash(sig, sig_size, mod, mod_size, exp, exp_size, hash, hash_size);
    }

    inline bool VerifyRsa4096PssSha256WithHash(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *hash, size_t hash_size, void *work_buf, size_t work_buf_size) {
        return Rsa4096PssSha256Verifier::VerifyWithHash(sig, sig_size, mod, mod_size, exp, exp_size, hash, hash_size, work_buf, work_buf_size);
    }

}
