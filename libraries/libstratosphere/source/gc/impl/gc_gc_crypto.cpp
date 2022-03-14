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
#include <stratosphere.hpp>

namespace ams::gc::impl {

    bool GcCrypto::CheckDevelopmentSpl() {
        return spl::IsDevelopment();
    }

    Result GcCrypto::DecryptAesKeySpl(void *dst, size_t dst_size, const void *src, size_t src_size, s32 generation, u32 option) {
        R_UNLESS(R_SUCCEEDED(spl::DecryptAesKey(dst, dst_size, src, src_size, generation, option)), fs::ResultGameCardSplDecryptAesKeyFailure());
        R_SUCCEED();
    }

    Result GcCrypto::VerifyCardHeader(const void *header_buffer, size_t header_size, const void *modulus, size_t modulus_size) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(header_size == sizeof(CardHeaderWithSignature));
        AMS_ABORT_UNLESS(modulus_size == GcRsaKeyLength);

        /* Get cert buffer as type. */
        const auto * const header = static_cast<const CardHeaderWithSignature *>(header_buffer);

        /* Verify the signature. */
        const void *mod       = modulus != nullptr ? modulus : EmbeddedDataHolder::s_ca10_modulus;
        const size_t mod_size = GcRsaKeyLength;
        const void *exp       = EmbeddedDataHolder::s_ca_public_exponent;
        const size_t exp_size = GcRsaPublicExponentLength;
        const void *sig       = header->signature;
        const size_t sig_size = sizeof(header->signature);
        const void *msg       = std::addressof(header->data);
        const size_t msg_size = sizeof(header->data);

        const bool is_signature_valid = crypto::VerifyRsa2048Pkcs1Sha256(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
        R_UNLESS(is_signature_valid, fs::ResultGameCardInvalidCardHeader());

        R_SUCCEED();
    }

    Result GcCrypto::VerifyT1CardCertificate(const void *cert_buffer, size_t cert_size) {
        /* Check pre-conditions. */
        AMS_ASSERT(cert_size == sizeof(T1CardCertificate));

        /* Get cert buffer as type. */
        const auto * const cert = static_cast<const T1CardCertificate *>(cert_buffer);

        /* Verify the signature. */
        const void *mod       = EmbeddedDataHolder::s_ca9_modulus;
        const size_t mod_size = GcRsaKeyLength;
        const void *exp       = EmbeddedDataHolder::s_ca_public_exponent;
        const size_t exp_size = GcRsaPublicExponentLength;
        const void *sig       = cert->signature;
        const size_t sig_size = sizeof(cert->signature);
        const void *msg       = reinterpret_cast<const u8 *>(cert) + sig_size;
        const size_t msg_size = sizeof(*cert) - (sig_size + sizeof(cert->padding));

        const bool is_signature_valid = crypto::VerifyRsa2048Pkcs1Sha256(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
        R_UNLESS(is_signature_valid, fs::ResultGameCardInvalidT1CardCertificate());

        R_SUCCEED();
    }

    Result GcCrypto::VerifyCa10Certificate(const void *cert_buffer, size_t cert_size) {
        /* Check pre-conditions. */
        AMS_ASSERT(cert_size == sizeof(Ca10Certificate));

        /* Get header buffer as type. */
        const auto * const cert = static_cast<const Ca10Certificate *>(cert_buffer);

        /* Verify the signature. */
        const void *mod       = EmbeddedDataHolder::s_ca10_certificate_modulus;
        const size_t mod_size = GcRsaKeyLength;
        const void *exp       = EmbeddedDataHolder::s_ca_public_exponent;
        const size_t exp_size = GcRsaPublicExponentLength;
        const void *sig       = cert->signature;
        const size_t sig_size = sizeof(cert->signature);
        const void *msg       = reinterpret_cast<const u8 *>(cert) + sig_size;
        const size_t msg_size = sizeof(*cert) - sig_size;

        const bool is_signature_valid = crypto::VerifyRsa2048Pkcs1Sha256(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
        R_UNLESS(is_signature_valid, fs::ResultGameCardInvalidCa10Certificate());

        R_SUCCEED();
    }

    Result GcCrypto::EncryptCardHeader(void *header_buffer, size_t header_size) {
        /* Check pre-conditions. */
        R_UNLESS(header_size == sizeof(CardHeader), fs::ResultGameCardPreconditionViolation());

        /* Get header buffer as type. */
        auto * const header = static_cast<CardHeader *>(header_buffer);

        /* Construct iv. */
        u8 iv[GcAesCbcIvLength];
        for (size_t i = 0; i < GcAesCbcIvLength; ++i) {
            iv[i] = header->iv[GcAesCbcIvLength - 1 - i];
        }

        /* Encrypt. */
        crypto::EncryptAes128Cbc(std::addressof(header->encrypted_data), sizeof(header->encrypted_data), EmbeddedDataHolder::s_card_header_key, GcAesKeyLength, iv, GcAesCbcIvLength, std::addressof(header->encrypted_data), sizeof(header->encrypted_data));
        R_SUCCEED();
    }

    Result GcCrypto::DecryptCardHeader(void *header_buffer, size_t header_size) {
        /* Check pre-conditions. */
        R_UNLESS(header_size == sizeof(CardHeader), fs::ResultGameCardPreconditionViolation());

        /* Get header buffer as type. */
        auto * const header = static_cast<CardHeader *>(header_buffer);

        /* Construct iv. */
        u8 iv[GcAesCbcIvLength];
        for (size_t i = 0; i < GcAesCbcIvLength; ++i) {
            iv[i] = header->iv[GcAesCbcIvLength - 1 - i];
        }

        /* Decrypt. */
        crypto::DecryptAes128Cbc(std::addressof(header->encrypted_data), sizeof(header->encrypted_data), EmbeddedDataHolder::s_card_header_key, GcAesKeyLength, iv, GcAesCbcIvLength, std::addressof(header->encrypted_data), sizeof(header->encrypted_data));
        R_SUCCEED();
    }

}
