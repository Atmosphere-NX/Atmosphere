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


namespace ams::crypto::impl {

    class Sha1Impl {
        public:
            static constexpr size_t HashSize  = 0x14;
            static constexpr size_t BlockSize = 0x40;
        private:
            struct State {
                u32 intermediate_hash[HashSize / sizeof(u32)];
                u8  buffer[BlockSize];
                u64 bits_consumed;
                size_t num_buffered;
                bool finalized;
            };
        private:
            State m_state;
        public:
            Sha1Impl() { /* ... */ }
            ~Sha1Impl() {
                static_assert(std::is_trivially_destructible<State>::value);
                ClearMemory(std::addressof(m_state), sizeof(m_state));
            }

            void Initialize();
            void Update(const void *data, size_t size);
            void GetHash(void *dst, size_t size);
    };

    /* static_assert(HashFunction<Sha1Impl>); */

}
