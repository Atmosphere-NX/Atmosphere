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
#include <vapours/crypto/impl/crypto_xts_mode_impl.hpp>

namespace ams::crypto {

    /* TODO: C++20 BlockCipher concept */

    template<typename BlockCipher>
    class XtsEncryptor {
        NON_COPYABLE(XtsEncryptor);
        NON_MOVEABLE(XtsEncryptor);
        private:
            using Impl = impl::XtsModeImpl;
        public:
            static constexpr size_t BlockSize = Impl::BlockSize;
            static constexpr size_t IvSize    = Impl::IvSize;
        private:
            Impl m_impl;
        public:
            XtsEncryptor() { /* ... */ }

            template<typename BlockCipher2>
            void Initialize(const BlockCipher *cipher1, const BlockCipher2 *cipher2, const void *iv, size_t iv_size) {
                m_impl.InitializeEncryption(cipher1, cipher2, iv, iv_size);
            }

            size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                return m_impl.template Update<BlockCipher>(dst, dst_size, src, src_size);
            }

            size_t Finalize(void *dst, size_t dst_size) {
                return m_impl.FinalizeEncryption(dst, dst_size);
            }
    };

}
