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
#include <vapours/crypto/impl/crypto_hmac_impl.hpp>

namespace ams::crypto {

    template<typename Hash> /* requires HashFunction<Hash> */
    class HmacGenerator {
        NON_COPYABLE(HmacGenerator);
        NON_MOVEABLE(HmacGenerator);
        private:
            using Impl = impl::HmacImpl<Hash>;
        public:
            static constexpr size_t HashSize  = Impl::HashSize;
            static constexpr size_t BlockSize = Impl::BlockSize;
        private:
            Impl m_impl;
        public:
            HmacGenerator() { /* ... */ }

            void Initialize(const void *key, size_t key_size) {
                return m_impl.Initialize(key, key_size);
            }

            void Update(const void *data, size_t size) {
                return m_impl.Update(data, size);
            }

            void GetMac(void *dst, size_t dst_size) {
                return m_impl.GetMac(dst, dst_size);
            }
    };
}
