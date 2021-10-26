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
#include <vapours/crypto/impl/crypto_md5_impl.hpp>

namespace ams::crypto {

    class Md5Generator {
        NON_COPYABLE(Md5Generator);
        NON_MOVEABLE(Md5Generator);
        private:
            using Impl = impl::Md5Impl;
        public:
            static constexpr size_t HashSize  = Impl::HashSize;
            static constexpr size_t BlockSize = Impl::BlockSize;

            static constexpr inline const u8 Asn1Identifier[] = {
                0x30, 0x20, /* Sequence, size 0x20 */
                    0x30, 0x0C, /* Sequence, size 0x0C */
                        0x06, 0x08, /* Object Identifier */
                            0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05, /* MD5 */
                        0x05, 0x00, /* Null */
                    0x04, 0x10, /* Octet string, size 0x10 */
            };
            static constexpr size_t Asn1IdentifierSize = util::size(Asn1Identifier);
        private:
            Impl m_impl;
        public:
            Md5Generator() { /* ... */ }

            void Initialize() {
                m_impl.Initialize();
            }

            void Update(const void *data, size_t size) {
                m_impl.Update(data, size);
            }

            void GetHash(void *dst, size_t size) {
                m_impl.GetHash(dst, size);
            }
    };

    void GenerateMd5Hash(void *dst, size_t dst_size, const void *src, size_t src_size);

}
