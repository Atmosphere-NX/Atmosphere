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
#include <vapours/crypto/crypto_ctr_encryptor.hpp>
#include <vapours/crypto/crypto_ctr_decryptor.hpp>

namespace ams::crypto {

    namespace impl {

        template<template<typename> typename _CtrImpl, typename _AesImpl>
        class AesCtrCryptor {
            NON_COPYABLE(AesCtrCryptor);
            NON_MOVEABLE(AesCtrCryptor);
            private:
                using AesImpl = _AesImpl;
                using CtrImpl = _CtrImpl<AesImpl>;
            public:
                static constexpr size_t KeySize   = AesImpl::KeySize;
                static constexpr size_t BlockSize = CtrImpl::BlockSize;
                static constexpr size_t IvSize    = CtrImpl::BlockSize;
            private:
                AesImpl m_aes_impl;
                CtrImpl m_ctr_impl;
            public:
                AesCtrCryptor() { /* ... */ }

                void Initialize(const void *key, size_t key_size, const void *iv, size_t iv_size) {
                    this->Initialize(key, key_size, iv, iv_size, 0);
                }

                void Initialize(const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset) {
                    AMS_ASSERT(key_size == KeySize);
                    AMS_ASSERT(iv_size  == IvSize);
                    AMS_ASSERT(offset   >= 0);

                    m_aes_impl.Initialize(key, key_size);
                    m_ctr_impl.Initialize(std::addressof(m_aes_impl), iv, iv_size, offset);
                }

                void SwitchMessage(const void *iv, size_t iv_size) {
                    return m_ctr_impl.SwitchMessage(iv, iv_size);
                }

                size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                    return m_ctr_impl.Update(dst, dst_size, src, src_size);
                }
        };

    }

    using Aes128CtrEncryptor = impl::AesCtrCryptor<CtrEncryptor, AesEncryptor128>;
    using Aes192CtrEncryptor = impl::AesCtrCryptor<CtrEncryptor, AesEncryptor192>;
    using Aes256CtrEncryptor = impl::AesCtrCryptor<CtrEncryptor, AesEncryptor256>;

    using Aes128CtrDecryptor = impl::AesCtrCryptor<CtrDecryptor, AesEncryptor128>;
    using Aes192CtrDecryptor = impl::AesCtrCryptor<CtrDecryptor, AesEncryptor192>;
    using Aes256CtrDecryptor = impl::AesCtrCryptor<CtrDecryptor, AesEncryptor256>;

    size_t EncryptAes128Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    size_t EncryptAes192Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    size_t EncryptAes256Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);

    size_t DecryptAes128Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    size_t DecryptAes192Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    size_t DecryptAes256Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);

    size_t EncryptAes128CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size);
    size_t EncryptAes192CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size);
    size_t EncryptAes256CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size);

    size_t DecryptAes128CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size);
    size_t DecryptAes192CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size);
    size_t DecryptAes256CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size);

}
