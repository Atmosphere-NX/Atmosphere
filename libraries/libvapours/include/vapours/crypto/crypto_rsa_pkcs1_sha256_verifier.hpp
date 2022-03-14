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
#include <vapours/crypto/crypto_rsa_pkcs1_verifier.hpp>
#include <vapours/crypto/crypto_sha256_generator.hpp>

namespace ams::crypto {

    namespace impl {

        template<size_t Bits>
        using RsaNPkcs1Sha256Verifier = ::ams::crypto::RsaPkcs1Verifier<Bits / BITSIZEOF(u8), ::ams::crypto::Sha256Generator>;

    }

    using Rsa2048Pkcs1Sha256Verifier = ::ams::crypto::impl::RsaNPkcs1Sha256Verifier<2048>;
    using Rsa4096Pkcs1Sha256Verifier = ::ams::crypto::impl::RsaNPkcs1Sha256Verifier<4096>;

    inline bool VerifyRsa2048Pkcs1Sha256(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size) {
        return Rsa2048Pkcs1Sha256Verifier::Verify(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
    }

    inline bool VerifyRsa2048Pkcs1Sha256(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, void *work_buf, size_t work_buf_size) {
        return Rsa2048Pkcs1Sha256Verifier::Verify(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size, work_buf, work_buf_size);
    }

    inline bool VerifyRsa4096Pkcs1Sha256(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size) {
        return Rsa4096Pkcs1Sha256Verifier::Verify(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
    }

    inline bool VerifyRsa4096Pkcs1Sha256(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, void *work_buf, size_t work_buf_size) {
        return Rsa4096Pkcs1Sha256Verifier::Verify(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size, work_buf, work_buf_size);
    }

}
