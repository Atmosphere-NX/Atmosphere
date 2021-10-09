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
#include <vapours/crypto/impl/crypto_ctr_mode_impl.hpp>

namespace ams::crypto {

    /* TODO: C++20 BlockCipher concept */

    template<typename BlockCipher>
    class CtrDecryptor {
        NON_COPYABLE(CtrDecryptor);
        NON_MOVEABLE(CtrDecryptor);
        private:
            using Impl = impl::CtrModeImpl<BlockCipher>;
        public:
            static constexpr size_t KeySize   = Impl::KeySize;
            static constexpr size_t BlockSize = Impl::BlockSize;
            static constexpr size_t IvSize    = Impl::IvSize;
        private:
            Impl m_impl;
        public:
            CtrDecryptor() { /* ... */ }

            void Initialize(const BlockCipher *cipher, const void *iv, size_t iv_size) {
                m_impl.Initialize(cipher, iv, iv_size);
            }

            void Initialize(const BlockCipher *cipher, const void *iv, size_t iv_size, s64 offset) {
                m_impl.Initialize(cipher, iv, iv_size, offset);
            }

            void SwitchMessage(const void *iv, size_t iv_size) {
                m_impl.SwitchMessage(iv, iv_size);
            }

            size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                return m_impl.Update(dst, dst_size, src, src_size);
            }
    };

}
