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
#include <vapours/crypto/impl/crypto_xts_mode_impl.hpp>

namespace ams::crypto {

    /* TODO: C++20 BlockCipher concept */

    template<typename BlockCipher>
    class XtsDecryptor {
        NON_COPYABLE(XtsDecryptor);
        NON_MOVEABLE(XtsDecryptor);
        private:
            using Impl = impl::XtsModeImpl;
        public:
            static constexpr size_t BlockSize = Impl::BlockSize;
            static constexpr size_t IvSize    = Impl::IvSize;
        private:
            Impl impl;
        public:
            XtsDecryptor() { /* ... */ }

            template<typename BlockCipher2>
            void Initialize(const BlockCipher *cipher1, const BlockCipher2 *cipher2, const void *iv, size_t iv_size) {
                this->impl.InitializeDecryption(cipher1, cipher2, iv, iv_size);
            }

            size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                return this->impl.template Update<BlockCipher>(dst, dst_size, src, src_size);
            }

            size_t Finalize(void *dst, size_t dst_size) {
                return this->impl.FinalizeDecryption(dst, dst_size);
            }
    };

}
