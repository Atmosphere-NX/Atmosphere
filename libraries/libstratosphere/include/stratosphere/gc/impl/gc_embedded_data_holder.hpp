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
#include <stratosphere/gc/impl/gc_gc_crypto.hpp>

namespace ams::gc::impl {

    class EmbeddedDataHolder {
        NON_COPYABLE(EmbeddedDataHolder);
        NON_MOVEABLE(EmbeddedDataHolder);
        friend class GcCrypto;
        private:
            struct ConcatenatedGcLibraryEmbeddedKeys {
                u8 enc_hmac_key_for_cv[GcCrypto::GcHmacKeyLength];
                u8 enc_hmac_key_for_key_and_iv[GcCrypto::GcHmacKeyLength];
                u8 enc_cv_constant_value[GcCrypto::GcCvConstLength];
                u8 enc_rsa_oaep_label_hash[GcCrypto::GcSha256HashLength];
            };
            static_assert(util::is_pod<ConcatenatedGcLibraryEmbeddedKeys>::value);
            static_assert(sizeof(ConcatenatedGcLibraryEmbeddedKeys) == 0x70);
        private:
            static bool s_is_dev;
            static const void *s_ca_public_exponent;
            static const void *s_ca1_modulus;
            static const void *s_ca9_modulus;
            static const void *s_ca10_modulus;
            static const void *s_ca10_certificate_modulus;
            static const void *s_card_header_key;
        private:
            static constinit inline u8 s_titlekey_keks[GcCrypto::GcTitleKeyKekIndexMax][GcCrypto::GcAesKeyLength] = {};
        public:
            static Result SetLibraryEmbeddedKeys(bool is_dev = GcCrypto::CheckDevelopmentSpl());

            static void SetLibraryTitleKeyKek(size_t kek_index, const void *kek, size_t kek_size) {
                AMS_ASSERT(kek_index < GcCrypto::GcTitleKeyKekIndexMax);
                AMS_ASSERT(kek_size == GcCrypto::GcAesKeyLength);
                AMS_UNUSED(kek_size);

                std::memcpy(s_titlekey_keks[kek_index], kek, sizeof(s_titlekey_keks[kek_index]));
            }
        private:
            static Result DecryptoEmbeddedKeys(ConcatenatedGcLibraryEmbeddedKeys *out, size_t out_size, bool is_dev = GcCrypto::CheckDevelopmentSpl());
    };

}
