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
#include <vapours/crypto/crypto_cmac_generator.hpp>

namespace ams::crypto {

    class Aes128CmacGenerator {
        NON_COPYABLE(Aes128CmacGenerator);
        NON_MOVEABLE(Aes128CmacGenerator);
        public:
            static constexpr size_t MacSize = AesEncryptor128::BlockSize;
        private:
            AesEncryptor128 m_aes;
            CmacGenerator<AesEncryptor128> m_cmac_generator;
        public:
            Aes128CmacGenerator() { /* ... */ }

            void Initialize(const void *key, size_t key_size) {
                AMS_ASSERT(key_size == AesEncryptor128::KeySize);

                m_aes.Initialize(key, key_size);
                m_cmac_generator.Initialize(std::addressof(m_aes));
            }

            void Update(const void *data, size_t size) {
                m_cmac_generator.Update(data, size);
            }

            void GetMac(void *dst, size_t size) {
                m_cmac_generator.GetMac(dst, size);
            }
    };

    ALWAYS_INLINE void GenerateAes128Cmac(void *dst, size_t dst_size, const void *data, size_t data_size, const void *key, size_t key_size) {
        Aes128CmacGenerator cmac_generator;

        cmac_generator.Initialize(key, key_size);
        cmac_generator.Update(data, data_size);
        cmac_generator.GetMac(dst, dst_size);
    }

}
