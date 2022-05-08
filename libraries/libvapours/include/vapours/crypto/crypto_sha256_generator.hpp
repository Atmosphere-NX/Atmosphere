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
#include <vapours/crypto/impl/crypto_sha256_impl.hpp>
#include <vapours/crypto/impl/crypto_sha256_impl_constexpr.hpp>

namespace ams::crypto {

    struct Sha256Context {
        u32 intermediate_hash[impl::Sha256Impl::HashSize / sizeof(u32)];
        u64 bits_consumed;
    };

    class Sha256Generator {
        NON_COPYABLE(Sha256Generator);
        NON_MOVEABLE(Sha256Generator);
        private:
            using Impl = impl::Sha256Impl;
        public:
            static constexpr size_t HashSize  = Impl::HashSize;
            static constexpr size_t BlockSize = Impl::BlockSize;

            static constexpr inline u8 Asn1Identifier[] = {
                0x30, 0x31, /* Sequence, size 0x31 */
                    0x30, 0x0D, /* Sequence, size 0x0D */
                        0x06, 0x09, /* Object Identifier */
                            0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, /* SHA-256 */
                        0x05, 0x00, /* Null */
                    0x04, 0x20, /* Octet string, size 0x20 */
            };
            static constexpr size_t Asn1IdentifierSize = util::size(Asn1Identifier);
        private:
            Impl m_impl{};
        public:
            Sha256Generator() = default;

            void Initialize() {
                m_impl.Initialize();
            }

            void Update(const void *data, size_t size) {
                m_impl.Update(data, size);
            }

            void GetHash(void *dst, size_t size) {
                m_impl.GetHash(dst, size);
            }

            void InitializeWithContext(const Sha256Context *context) {
                m_impl.InitializeWithContext(context);
            }

            size_t GetContext(Sha256Context *context) const {
                return m_impl.GetContext(context);
            }

            size_t GetBufferedDataSize() const {
                return m_impl.GetBufferedDataSize();
            }

            void GetBufferedData(void *dst, size_t dst_size) const {
                return m_impl.GetBufferedData(dst, dst_size);
            }
    };

    void GenerateSha256(void *dst, size_t dst_size, const void *src, size_t src_size);

    ALWAYS_INLINE void GenerateSha256Hash(void *dst, size_t dst_size, const void *src, size_t src_size) {
        return GenerateSha256(dst, dst_size, src, src_size);
    }

    template<typename T, typename = typename std::enable_if<std::same_as<T, u8> || std::same_as<T, s8> || std::same_as<T, char> || std::same_as<T, unsigned char>>::type>
    constexpr ALWAYS_INLINE void GenerateSha256(u8 *dst, size_t dst_size, const T *src, size_t src_size) {
        if (std::is_constant_evaluated()) {
            impl::Sha256CompileTimeImpl sha;
            sha.Initialize();
            sha.Update(src, src_size);
            sha.GetHash(dst, dst_size);
        } else {
            return GenerateSha256(static_cast<void *>(dst), dst_size, static_cast<const void *>(src), src_size);
        }
    }

}
