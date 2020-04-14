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
#include <stratosphere.hpp>
#include "erpt_srv_formatter.hpp"
#include "erpt_srv_keys.hpp"

namespace ams::erpt::srv {

    class Cipher : private Formatter {
        private:
            static constexpr u32 RsaKeySize = 0x100;
            static constexpr u32 SaltSize   = 0x20;

            static u8 s_key[crypto::Aes128CtrEncryptor::KeySize + crypto::Aes128CtrEncryptor::IvSize + crypto::Aes128CtrEncryptor::BlockSize];
            static bool s_need_to_store_cipher;

            struct Header {
                u32 magic;
                u32 field_type;
                u32 element_count;
                u32 reserved;
                u8 data[0];
            };
            static_assert(sizeof(Header) == 0x10);

            static constexpr u32 HeaderMagic = util::FourCC<'C', 'R', 'P', 'T'>::Code;
        private:
            template<typename T>
            static Result EncryptArray(Report *report, FieldId field_id, T *arr, u32 arr_size) {
                const u32 data_size = util::AlignUp(arr_size * sizeof(T), crypto::Aes128CtrEncryptor::BlockSize);

                Header *hdr = reinterpret_cast<Header *>(AllocateWithAlign(sizeof(Header) + data_size, crypto::Aes128CtrEncryptor::BlockSize));
                R_UNLESS(hdr != nullptr, erpt::ResultOutOfMemory());
                ON_SCOPE_EXIT { Deallocate(hdr); };

                hdr->magic         = HeaderMagic;
                hdr->field_type    = static_cast<u32>(FieldToTypeMap[field_id]);
                hdr->element_count = arr_size;
                hdr->reserved      = 0;

                std::memset(hdr->data, 0, data_size);
                std::memcpy(hdr->data, arr, arr_size * sizeof(T));

                crypto::EncryptAes128Ctr(hdr->data, data_size, s_key, crypto::Aes128CtrEncryptor::KeySize, s_key + crypto::Aes128CtrEncryptor::KeySize, crypto::Aes128CtrEncryptor::IvSize, hdr->data, data_size);

                ON_SCOPE_EXIT { std::memset(hdr, 0, sizeof(hdr) + data_size); s_need_to_store_cipher = true; };

                return Formatter::AddField(report, field_id, reinterpret_cast<u8 *>(hdr), sizeof(hdr) + data_size);
            }
        public:
            static Result Begin(Report *report, u32 record_count) {
                s_need_to_store_cipher = false;
                crypto::GenerateCryptographicallyRandomBytes(s_key, sizeof(s_key));

                return Formatter::Begin(report, record_count + 1);
            }

            static Result End(Report *report) {
                u8 cipher[RsaKeySize] = {};

                if (s_need_to_store_cipher) {
                    u8 salt[SaltSize];
                    crypto::RsaOaepEncryptor<RsaKeySize, crypto::Sha256Generator> oaep;
                    crypto::GenerateCryptographicallyRandomBytes(salt, sizeof(salt));

                    oaep.Initialize(GetPublicKeyModulus(), GetPublicKeyModulusSize(), GetPublicKeyExponent(), GetPublicKeyExponentSize());
                    oaep.Encrypt(cipher, sizeof(cipher), s_key, sizeof(s_key), salt, sizeof(salt));
                }

                Formatter::AddField(report, FieldId_CipherKey, cipher, sizeof(cipher));
                std::memset(s_key, 0, sizeof(s_key));

                return Formatter::End(report);
            }

            static Result AddField(Report *report, FieldId field_id, bool value) {
                return Formatter::AddField(report, field_id, value);
            }

            template<typename T>
            static Result AddField(Report *report, FieldId field_id, T value) {
                return Formatter::AddField<T>(report, field_id, value);
            }

            static Result AddField(Report *report, FieldId field_id, char *str, u32 len) {
                if (FieldToFlagMap[field_id] == FieldFlag_Encrypt) {
                    return EncryptArray<char>(report, field_id, str, len);
                } else {
                    return Formatter::AddField(report, field_id, str, len);
                }
            }

            static Result AddField(Report *report, FieldId field_id, u8 *bin, u32 len) {
                if (FieldToFlagMap[field_id] == FieldFlag_Encrypt) {
                    return EncryptArray<u8>(report, field_id, bin, len);
                } else {
                    return Formatter::AddField(report, field_id, bin, len);
                }
            }

            template<typename T>
            static Result AddField(Report *report, FieldId field_id, T *arr, u32 len) {
                if (FieldToFlagMap[field_id] == FieldFlag_Encrypt) {
                    return EncryptArray<T>(report, field_id, arr, len);
                } else {
                    return Formatter::AddField<T>(report, field_id, arr, len);
                }
            }
    };

}
