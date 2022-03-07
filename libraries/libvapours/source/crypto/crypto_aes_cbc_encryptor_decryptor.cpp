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
#include <vapours.hpp>

namespace ams::crypto {

    size_t EncryptAes128Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes128CbcEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t EncryptAes192Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes192CbcEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t EncryptAes256Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes256CbcEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes128Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes128CbcDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes192Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes192CbcDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes256Cbc(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes256CbcDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

}
