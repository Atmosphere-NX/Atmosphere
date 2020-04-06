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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util.hpp>
#include <vapours/crypto/impl/crypto_aes_impl.hpp>

namespace ams::crypto {

    template<size_t _KeySize>
    class AesDecryptor {
        NON_COPYABLE(AesDecryptor);
        NON_MOVEABLE(AesDecryptor);
        private:
            using Impl = impl::AesImpl<_KeySize>;
        public:
            static constexpr size_t KeySize      = Impl::KeySize;
            static constexpr size_t BlockSize    = Impl::BlockSize;
            static constexpr size_t RoundKeySize = Impl::RoundKeySize;
        private:
            Impl impl;
        public:
            AesDecryptor() { /* ... */ }

            void Initialize(const void *key, size_t key_size) {
                this->impl.Initialize(key, key_size, false);
            }

            void DecryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
                return this->impl.DecryptBlock(dst, dst_size, src, src_size);
            }

            const u8 *GetRoundKey() const {
                return this->impl.GetRoundKey();
            }
    };

    using AesDecryptor128 = AesDecryptor<16>;
    using AesDecryptor192 = AesDecryptor<24>;
    using AesDecryptor256 = AesDecryptor<32>;

}
