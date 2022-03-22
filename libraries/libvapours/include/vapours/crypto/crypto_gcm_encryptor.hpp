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
#include <vapours/crypto/impl/crypto_gcm_mode_impl.hpp>

namespace ams::crypto {

    /* TODO: C++20 BlockCipher concept */

    template<typename BlockCipher>
    class GcmEncryptor {
        NON_COPYABLE(GcmEncryptor);
        NON_MOVEABLE(GcmEncryptor);
        private:
            using Impl = impl::GcmModeImpl<BlockCipher>;
        public:
            static constexpr size_t KeySize   = Impl::KeySize;
            static constexpr size_t BlockSize = Impl::BlockSize;
            static constexpr size_t MacSize   = Impl::MacSize;
        private:
            Impl m_impl;
        public:
            GcmEncryptor() { /* ... */ }

            void Initialize(const BlockCipher *cipher, const void *iv, size_t iv_size) {
                m_impl.Initialize(cipher);
                m_impl.Reset(iv, iv_size);
            }

            void Reset(const void *iv, size_t iv_size) {
                m_impl.Reset(iv, iv_size);
            }

            size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                return m_impl.Update(dst, dst_size, src, src_size);
            }

            void UpdateAad(const void *aad, size_t aad_size) {
                return m_impl.UpdateAad(aad, aad_size);
            }

            void GetMac(void *dst, size_t dst_size) {
                return m_impl.GetMac(dst, dst_size);
            }
    };

}
