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

    size_t EncryptAes128Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes128CtrEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t EncryptAes192Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes192CtrEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t EncryptAes256Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes256CtrEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes128Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes128CtrDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes192Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes192CtrDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes256Ctr(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size) {
        Aes256CtrDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t EncryptAes128CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size) {
        Aes128CtrEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size, offset);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t EncryptAes192CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size) {
        Aes192CtrEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size, offset);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t EncryptAes256CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size) {
        Aes256CtrEncryptor aes;
        aes.Initialize(key, key_size, iv, iv_size, offset);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes128CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size) {
        Aes128CtrDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size, offset);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes192CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size) {
        Aes192CtrDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size, offset);
        return aes.Update(dst, dst_size, src, src_size);
    }

    size_t DecryptAes256CtrPartial(void *dst, size_t dst_size, const void *key, size_t key_size, const void *iv, size_t iv_size, s64 offset, const void *src, size_t src_size) {
        Aes256CtrDecryptor aes;
        aes.Initialize(key, key_size, iv, iv_size, offset);
        return aes.Update(dst, dst_size, src, src_size);
    }

}
