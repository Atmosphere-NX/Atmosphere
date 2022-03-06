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
#include <vapours/crypto/impl/crypto_sha3_impl.hpp>

namespace ams::crypto {

    struct Sha3Context {
        u32 hash_size;
        u32 buffered_bytes;
        u64 internal_state[25];
    };

    namespace impl {

        template<size_t HashSize>
        struct Sha3Asn1IdentifierByte;

        template<> struct Sha3Asn1IdentifierByte<224 / BITSIZEOF(u8)> { static constexpr u8 Value = 0x07; };
        template<> struct Sha3Asn1IdentifierByte<256 / BITSIZEOF(u8)> { static constexpr u8 Value = 0x08; };
        template<> struct Sha3Asn1IdentifierByte<384 / BITSIZEOF(u8)> { static constexpr u8 Value = 0x09; };
        template<> struct Sha3Asn1IdentifierByte<512 / BITSIZEOF(u8)> { static constexpr u8 Value = 0x0A; };

    }

    template <size_t _HashSize>
    class Sha3Generator {
        NON_COPYABLE(Sha3Generator);
        NON_MOVEABLE(Sha3Generator);
        private:
            using Impl = impl::Sha3Impl<_HashSize>;
        public:
            static constexpr size_t HashSize  = Impl::HashSize;
            static constexpr size_t BlockSize = Impl::BlockSize;

            static constexpr inline u8 Asn1Identifier[] = {
                0x30, 0x31, /* Sequence, size 0x31 */
                    0x30, 0x0D, /* Sequence, size 0x0D */
                        0x06, 0x09, /* Object Identifier */
                            0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, impl::Sha3Asn1IdentifierByte<HashSize>::Value,  /* SHA3-*** */
                        0x05, 0x00, /* Null */
                    0x04, 0x20, /* Octet string, size 0x20 */
            };
            static constexpr size_t Asn1IdentifierSize = util::size(Asn1Identifier);
        private:
            Impl m_impl;
        public:
            Sha3Generator() { /* ... */ }

            void Initialize() {
                m_impl.Initialize();
            }

            void Update(const void *data, size_t size) {
                m_impl.Update(data, size);
            }

            void GetHash(void *dst, size_t size) {
                m_impl.GetHash(dst, size);
            }

            void InitializeWithContext(const Sha3Context *context) {
                m_impl.InitializeWithContext(context);
            }

            void GetContext(Sha3Context *context) const {
                m_impl.GetContext(context);
            }
    };

    using Sha3224Generator = Sha3Generator<224 / BITSIZEOF(u8)>;
    using Sha3256Generator = Sha3Generator<256 / BITSIZEOF(u8)>;
    using Sha3384Generator = Sha3Generator<384 / BITSIZEOF(u8)>;
    using Sha3512Generator = Sha3Generator<512 / BITSIZEOF(u8)>;

    inline void GenerateSha3224(void *dst, size_t dst_size, const void *src, size_t src_size) {
        Sha3224Generator generator;

        generator.Initialize();
        generator.Update(src, src_size);
        generator.GetHash(dst, dst_size);
    }

    inline void GenerateSha3256(void *dst, size_t dst_size, const void *src, size_t src_size) {
        Sha3256Generator generator;

        generator.Initialize();
        generator.Update(src, src_size);
        generator.GetHash(dst, dst_size);
    }

    inline void GenerateSha3384(void *dst, size_t dst_size, const void *src, size_t src_size) {
        Sha3384Generator generator;

        generator.Initialize();
        generator.Update(src, src_size);
        generator.GetHash(dst, dst_size);
    }

    inline void GenerateSha3512(void *dst, size_t dst_size, const void *src, size_t src_size) {
        Sha3512Generator generator;

        generator.Initialize();
        generator.Update(src, src_size);
        generator.GetHash(dst, dst_size);
    }

}
