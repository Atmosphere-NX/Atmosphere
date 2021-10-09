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
#include <vapours/crypto/crypto_aes_encryptor.hpp>
#include <vapours/crypto/crypto_gcm_encryptor.hpp>

namespace ams::crypto {

    namespace impl {

        template<typename _AesImpl>
        class AesGcmEncryptor {
            NON_COPYABLE(AesGcmEncryptor);
            NON_MOVEABLE(AesGcmEncryptor);
            private:
                using AesImpl = _AesImpl;
                using GcmImpl = GcmEncryptor<AesImpl>;
            public:
                static constexpr size_t KeySize   = AesImpl::KeySize;
                static constexpr size_t BlockSize = AesImpl::BlockSize;
                static constexpr size_t MacSize   = AesImpl::BlockSize;
            private:
                AesImpl m_aes_impl;
                GcmImpl m_gcm_impl;
            public:
                AesGcmEncryptor() { /* ... */ }

                void Initialize(const void *key, size_t key_size, const void *iv, size_t iv_size) {
                    m_aes_impl.Initialize(key, key_size);
                    m_gcm_impl.Initialize(std::addressof(m_aes_impl), iv, iv_size);
                }

                void Reset(const void *iv, size_t iv_size) {
                    m_gcm_impl.Reset(iv, iv_size);
                }

                size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                    return m_gcm_impl.Update(dst, dst_size, src, src_size);
                }

                void UpdateAad(const void *aad, size_t aad_size) {
                    return m_gcm_impl.UpdateAad(aad, aad_size);
                }

                void GetMac(void *dst, size_t dst_size) {
                    return m_gcm_impl.GetMac(dst, dst_size);
                }
        };

    }

    using Aes128GcmEncryptor = impl::AesGcmEncryptor<AesEncryptor128>;
    /* TODO: Validate AAD/GMAC is same for non-128 bit key using Aes192GcmEncryptor = impl::AesGcmEncryptor<AesEncryptor192>; */
    /* TODO: Validate AAD/GMAC is same for non-128 bit key using Aes256GcmEncryptor = impl::AesGcmEncryptor<AesEncryptor256>; */

}
