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
#include <vapours/crypto/crypto_cbc_encryptor.hpp>
#include <vapours/crypto/crypto_cbc_decryptor.hpp>

namespace ams::crypto {

    namespace impl {

        template<template<typename> typename _CbcImpl, typename _AesImpl>
        class AesCbcCryptor {
            NON_COPYABLE(AesCbcCryptor);
            NON_MOVEABLE(AesCbcCryptor);
            private:
                using AesImpl = _AesImpl;
                using CbcImpl = _CbcImpl<AesImpl>;
            public:
                static constexpr size_t KeySize   = AesImpl::KeySize;
                static constexpr size_t BlockSize = CbcImpl::BlockSize;
                static constexpr size_t IvSize    = CbcImpl::BlockSize;
            private:
                AesImpl m_aes_impl;
                CbcImpl m_cbc_impl;
            public:
                AesCbcCryptor() { /* ... */ }

                void Initialize(const void *key, size_t key_size, const void *iv, size_t iv_size) {
                    AMS_ASSERT(key_size == KeySize);
                    AMS_ASSERT(iv_size  == IvSize);

                    m_aes_impl.Initialize(key, key_size);
                    m_cbc_impl.Initialize(std::addressof(m_aes_impl), iv, iv_size);
                }

                size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                    return m_cbc_impl.Update(dst, dst_size, src, src_size);
                }

                size_t GetBufferedDataSize() const {
                    return m_cbc_impl.GetBufferedDataSize();
                }
        };

    }

    using Aes128CbcEncryptor = impl::AesCbcCryptor<CbcEncryptor, AesEncryptor128>;
    using Aes192CbcEncryptor = impl::AesCbcCryptor<CbcEncryptor, AesEncryptor192>;
    using Aes256CbcEncryptor = impl::AesCbcCryptor<CbcEncryptor, AesEncryptor256>;

    using Aes128CbcDecryptor = impl::AesCbcCryptor<CbcDecryptor, AesDecryptor128>;
    using Aes192CbcDecryptor = impl::AesCbcCryptor<CbcDecryptor, AesDecryptor192>;
    using Aes256CbcDecryptor = impl::AesCbcCryptor<CbcDecryptor, AesDecryptor256>;

    size_t EncryptAes128Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    size_t EncryptAes192Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    size_t EncryptAes256Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);

    size_t DecryptAes128Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    size_t DecryptAes192Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    size_t DecryptAes256Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);

}
