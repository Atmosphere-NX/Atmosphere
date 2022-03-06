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
#include <vapours/crypto/impl/crypto_hash_function.hpp>
#include <vapours/crypto/crypto_memory_clear.hpp>

namespace ams::crypto {

    struct Sha3Context;

}

namespace ams::crypto::impl {

    template<size_t _HashSize>
    class Sha3Impl {
        public:
            static constexpr size_t InternalStateSize = 200;
            static constexpr size_t HashSize  = _HashSize;
            static constexpr size_t BlockSize = InternalStateSize - 2 * HashSize;
        private:
            enum State {
                State_None,
                State_Initialized,
                State_Done,
            };
        private:
            u64 m_internal_state[InternalStateSize / sizeof(u64)];
            size_t m_buffered_bytes;
            State m_state;
        public:
            Sha3Impl() : m_state(State_None) { /* ... */ }
            ~Sha3Impl() {
                ClearMemory(this, sizeof(*this));
            }

            void Initialize();
            void Update(const void *data, size_t size);
            void GetHash(void *dst, size_t size);

            void InitializeWithContext(const Sha3Context *context);
            void GetContext(Sha3Context *context) const;
        private:
            void ProcessBlock();
            void ProcessLastBlock();
    };

    static_assert(HashFunction<Sha3Impl<224 / BITSIZEOF(u8)>>);
    static_assert(HashFunction<Sha3Impl<256 / BITSIZEOF(u8)>>);
    static_assert(HashFunction<Sha3Impl<384 / BITSIZEOF(u8)>>);
    static_assert(HashFunction<Sha3Impl<512 / BITSIZEOF(u8)>>);

}
