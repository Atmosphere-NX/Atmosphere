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
#include <vapours.hpp>
#include <stratosphere/gc/impl/gc_types.hpp>

namespace ams::gc::impl {

    class GcCrypto {
        NON_COPYABLE(GcCrypto);
        NON_MOVEABLE(GcCrypto);
        public:
            static constexpr size_t GcRsaKeyLength            = crypto::Rsa2048PssSha256Verifier::ModulusSize;
            static constexpr size_t GcRsaPublicExponentLength = 3;
            static constexpr size_t GcAesKeyLength            = crypto::AesEncryptor128::KeySize;
            static constexpr size_t GcAesCbcIvLength          = crypto::Aes128CbcEncryptor::IvSize;
            static constexpr size_t GcHmacKeyLength           = 0x20;
            static constexpr size_t GcCvConstLength           = 0x10;
            static constexpr size_t GcTitleKeyKekIndexMax     = 0x10;
            static constexpr size_t GcSha256HashLength        = crypto::Sha256Generator::HashSize;
        public:
            static bool CheckDevelopmentSpl();
            static Result DecryptAesKeySpl(void *dst, size_t dst_size, const void *src, size_t src_size, s32 generation, u32 option);

            static Result VerifyCardHeader(const void *header_buffer, size_t header_size, const void *modulus, size_t modulus_size);

            static Result EncryptCardHeader(void *header, size_t header_size);
            static Result DecryptCardHeader(void *header, size_t header_size);

            static Result VerifyT1CardCertificate(const void *cert_buffer, size_t cert_size);

            static Result VerifyCa10Certificate(const void *cert_buffer, size_t cert_size);

            static Result DecryptCardInitialData(void *dst, size_t dst_size, const void *initial_data, size_t data_size, size_t kek_index);
    };

}
