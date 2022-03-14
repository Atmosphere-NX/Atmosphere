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
#include <vapours/crypto/crypto_ccm_encryptor.hpp>
#include <vapours/crypto/crypto_ccm_decryptor.hpp>

namespace ams::crypto {

    namespace impl {

        template<template<typename> typename _CcmImpl, typename _AesImpl>
        class AesCcmCryptor {
            NON_COPYABLE(AesCcmCryptor);
            NON_MOVEABLE(AesCcmCryptor);
            private:
                using AesImpl = _AesImpl;
                using CcmImpl = _CcmImpl<AesImpl>;
            public:
                static constexpr size_t KeySize      = CcmImpl::KeySize;
                static constexpr size_t BlockSize    = CcmImpl::BlockSize;
                static constexpr size_t MaxMacSize   = CcmImpl::MaxMacSize;
                static constexpr size_t MaxNonceSize = CcmImpl::MaxNonceSize;
            private:
                AesImpl m_aes_impl;
                CcmImpl m_ccm_impl;
            public:
                AesCcmCryptor() { /* ... */ }

                void Initialize(const void *key, size_t key_size, const void *nonce, size_t nonce_size, s64 aad_size, s64 data_size, size_t mac_size) {
                    AMS_ASSERT(key_size == KeySize);
                    AMS_ASSERT(nonce_size <= MaxNonceSize);
                    AMS_ASSERT(mac_size <= MaxMacSize);

                    m_aes_impl.Initialize(key, key_size);
                    m_ccm_impl.Initialize(std::addressof(m_aes_impl), nonce, nonce_size, aad_size, data_size, mac_size);
                }

                size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                    return m_ccm_impl.Update(dst, dst_size, src, src_size);
                }

                void UpdateAad(const void *aad, size_t aad_size) {
                    return m_ccm_impl.UpdateAad(aad, aad_size);
                }

                void GetMac(void *dst, size_t dst_size) {
                    return m_ccm_impl.GetMac(dst, dst_size);
                }
        };

    }

    using Aes128CcmEncryptor = impl::AesCcmCryptor<CcmEncryptor, AesEncryptor128>;
    using Aes128CcmDecryptor = impl::AesCcmCryptor<CcmDecryptor, AesEncryptor128>;

    inline size_t EncryptAes128Ccm(void *dst, size_t dst_size, void *mac, size_t mac_buf_size, const void *key, size_t key_size, const void *nonce, size_t nonce_size, const void *src, size_t src_size, const void *aad, size_t aad_size, size_t mac_size) {
        /* Create encryptor. */
        Aes128CcmEncryptor ccm;
        ccm.Initialize(key, key_size, nonce, nonce_size, aad_size, src_size, mac_size);

        /* Process aad. */
        if (aad_size > 0) {
            ccm.UpdateAad(aad, aad_size);
        }

        /* Process data. */
        size_t processed = 0;
        if (src_size > 0) {
            processed = ccm.Update(dst, dst_size, src, src_size);
        }

        /* Get mac. */
        ccm.GetMac(mac, mac_buf_size);
        return processed;
    }

    inline size_t DecryptAes128Ccm(void *dst, size_t dst_size, void *mac, size_t mac_buf_size, const void *key, size_t key_size, const void *nonce, size_t nonce_size, const void *src, size_t src_size, const void *aad, size_t aad_size, size_t mac_size) {
        /* Create decryptor. */
        Aes128CcmDecryptor ccm;
        ccm.Initialize(key, key_size, nonce, nonce_size, aad_size, src_size, mac_size);

        /* Process aad. */
        if (aad_size > 0) {
            ccm.UpdateAad(aad, aad_size);
        }

        /* Process data. */
        size_t processed = 0;
        if (src_size > 0) {
            processed = ccm.Update(dst, dst_size, src, src_size);
        }

        /* Get mac. */
        ccm.GetMac(mac, mac_buf_size);
        return processed;
    }
}
