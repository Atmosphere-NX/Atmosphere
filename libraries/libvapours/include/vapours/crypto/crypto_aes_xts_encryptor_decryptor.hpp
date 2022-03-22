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
#include <vapours/crypto/crypto_xts_encryptor.hpp>
#include <vapours/crypto/crypto_xts_decryptor.hpp>

namespace ams::crypto {

    namespace impl {

        template<template<typename> typename _XtsImpl, typename _AesImpl1, typename _AesImpl2>
        class AesXtsCryptor {
            NON_COPYABLE(AesXtsCryptor);
            NON_MOVEABLE(AesXtsCryptor);
            private:
                using AesImpl1 = _AesImpl1;
                using AesImpl2 = _AesImpl2;
                using XtsImpl = _XtsImpl<AesImpl1>;
            public:
                static constexpr size_t KeySize   = AesImpl1::KeySize;
                static constexpr size_t BlockSize = AesImpl1::BlockSize;
                static constexpr size_t IvSize    = AesImpl1::BlockSize;

                static_assert(AesImpl1::KeySize   == AesImpl2::KeySize);
                static_assert(AesImpl1::BlockSize == AesImpl2::BlockSize);
            private:
                AesImpl1 m_aes_impl_1;
                AesImpl2 m_aes_impl_2;
                XtsImpl m_xts_impl;
            public:
                AesXtsCryptor() { /* ... */ }

                void Initialize(const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size) {
                    AMS_ASSERT(key_size == KeySize);
                    AMS_ASSERT(iv_size  == IvSize);

                    m_aes_impl_1.Initialize(key1, key_size);
                    m_aes_impl_2.Initialize(key2, key_size);
                    m_xts_impl.Initialize(std::addressof(m_aes_impl_1), std::addressof(m_aes_impl_2), iv, iv_size);
                }

                size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                    return m_xts_impl.Update(dst, dst_size, src, src_size);
                }

                size_t Finalize(void *dst, size_t dst_size) {
                    return m_xts_impl.Finalize(dst, dst_size);
                }
        };

    }

    using Aes128XtsEncryptor = impl::AesXtsCryptor<XtsEncryptor, AesEncryptor128, AesEncryptor128>;
    using Aes192XtsEncryptor = impl::AesXtsCryptor<XtsEncryptor, AesEncryptor192, AesEncryptor192>;
    using Aes256XtsEncryptor = impl::AesXtsCryptor<XtsEncryptor, AesEncryptor256, AesEncryptor256>;

    using Aes128XtsDecryptor = impl::AesXtsCryptor<XtsDecryptor, AesDecryptor128, AesEncryptor128>;
    using Aes192XtsDecryptor = impl::AesXtsCryptor<XtsDecryptor, AesDecryptor192, AesEncryptor192>;
    using Aes256XtsDecryptor = impl::AesXtsCryptor<XtsDecryptor, AesDecryptor256, AesEncryptor256>;

    inline size_t EncryptAes128Xts(void *dst, size_t dst_size, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
              u8 *dst_u8 = static_cast<u8 *>(dst);
        const u8 *src_u8 = static_cast<const u8 *>(src);

        Aes128XtsEncryptor xts;
        xts.Initialize(key1, key2, key_size, iv, iv_size);

        size_t processed = xts.Update(dst_u8, dst_size, src_u8, src_size);
        dst_u8   += processed;
        dst_size -= processed;

        processed += xts.Finalize(dst_u8, dst_size);
        return processed;
    }

    inline size_t EncryptAes192Xts(void *dst, size_t dst_size, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
              u8 *dst_u8 = static_cast<u8 *>(dst);
        const u8 *src_u8 = static_cast<const u8 *>(src);

        Aes192XtsEncryptor xts;
        xts.Initialize(key1, key2, key_size, iv, iv_size);

        size_t processed = xts.Update(dst_u8, dst_size, src_u8, src_size);
        dst_u8   += processed;
        dst_size -= processed;

        processed += xts.Finalize(dst_u8, dst_size);
        return processed;
    }

    inline size_t EncryptAes256Xts(void *dst, size_t dst_size, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
              u8 *dst_u8 = static_cast<u8 *>(dst);
        const u8 *src_u8 = static_cast<const u8 *>(src);

        Aes256XtsEncryptor xts;
        xts.Initialize(key1, key2, key_size, iv, iv_size);

        size_t processed = xts.Update(dst_u8, dst_size, src_u8, src_size);
        dst_u8   += processed;
        dst_size -= processed;

        processed += xts.Finalize(dst_u8, dst_size);
        return processed;
    }

    inline size_t DecryptAes128Xts(void *dst, size_t dst_size, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
              u8 *dst_u8 = static_cast<u8 *>(dst);
        const u8 *src_u8 = static_cast<const u8 *>(src);

        Aes128XtsDecryptor xts;
        xts.Initialize(key1, key2, key_size, iv, iv_size);

        size_t processed = xts.Update(dst_u8, dst_size, src_u8, src_size);
        dst_u8   += processed;
        dst_size -= processed;

        processed += xts.Finalize(dst_u8, dst_size);
        return processed;
    }

    inline size_t DecryptAes192Xts(void *dst, size_t dst_size, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
              u8 *dst_u8 = static_cast<u8 *>(dst);
        const u8 *src_u8 = static_cast<const u8 *>(src);

        Aes192XtsDecryptor xts;
        xts.Initialize(key1, key2, key_size, iv, iv_size);

        size_t processed = xts.Update(dst_u8, dst_size, src_u8, src_size);
        dst_u8   += processed;
        dst_size -= processed;

        processed += xts.Finalize(dst_u8, dst_size);
        return processed;
    }

    inline size_t DecryptAes256Xts(void *dst, size_t dst_size, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
              u8 *dst_u8 = static_cast<u8 *>(dst);
        const u8 *src_u8 = static_cast<const u8 *>(src);

        Aes256XtsDecryptor xts;
        xts.Initialize(key1, key2, key_size, iv, iv_size);

        size_t processed = xts.Update(dst_u8, dst_size, src_u8, src_size);
        dst_u8   += processed;
        dst_size -= processed;

        processed += xts.Finalize(dst_u8, dst_size);
        return processed;
    }

}
